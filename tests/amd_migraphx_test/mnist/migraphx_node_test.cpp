#include "vx_amd_migraphx.h"
#include <cstring>
#include <random>
#include <fstream>
#include <algorithm>
#define MAX_STRING_LENGTH 100

using namespace std;

#define ERROR_CHECK_STATUS(status) { \
    vx_status status_ = (status); \
    if (status_ != VX_SUCCESS) { \
        printf("ERROR: failed with status = (%d) at " __FILE__ "#%d\n", status_, __LINE__); \
        exit(1); \
    } \
}

#define ERROR_CHECK_OBJECT(obj) { \
    vx_status status_ = vxGetStatus((vx_reference)(obj)); \
    if(status_ != VX_SUCCESS) { \
        printf("ERROR: failed with status = (%d) at " __FILE__ "#%d\n", status_, __LINE__); \
        exit(1); \
    } \
}

static void VX_CALLBACK log_callback(vx_context context, vx_reference ref, vx_status status, const vx_char string[])
{
    size_t len = strnlen(string, MAX_STRING_LENGTH);
    if (len > 0) {
        printf("%s", string);
        if (string[len - 1] != '\n')
            printf("\n");
        fflush(stdout);
    }
}

void read_input_digit(const int n, std::vector<float>& input_digit)
{
    std::ifstream file("../digits.txt");
    const int Digits = 10;
    const int Height = 28;
    const int Width  = 28;
    if(!file.is_open()) {
        return;
    }
    for(int d = 0; d < Digits; ++d) {
        for(int i = 0; i < Height * Width; ++i) {
            unsigned char temp = 0;
            file.read((char*)&temp, sizeof(temp));
            if(d == n) {
                float data = temp / 255.0;
                input_digit.push_back(data);
            }
        }
    }
    std::cout << std::endl;
}

int main(int argc, char **argv) {
    
    if(argc < 2) {
        std::cout << "Usage: \n ./migraphx_node_test <path-to-resnet50 ONNX model>" << std::endl;
        return -1;
    }
    
    std::string modelFileName = argv[1];

    vx_context context = vxCreateContext();
    ERROR_CHECK_OBJECT(context);
    vxRegisterLogCallback(context, log_callback, vx_false_e);

    vx_graph graph = vxCreateGraph(context);
    ERROR_CHECK_OBJECT(graph);

    // initialize variables
    vx_tensor input_tensor, output_tensor;
    vx_size input_num_of_dims;
    vx_enum input_data_format;
    vx_size input_dims[4];
    vx_size output_num_of_dims;
    vx_enum output_data_format;
    vx_size output_dims[4];
    vx_size stride[4];
    vx_map_id map_id;
    void *ptr = nullptr;
    vx_status status = 0;
    migraphx::program prog;

    status = amdMIGraphXcompile(modelFileName.c_str(), &prog,
    &input_num_of_dims, input_dims, &input_data_format,
    &output_num_of_dims, output_dims, &output_data_format, false, false);

    if (status) {
        std::cerr << "ERROR: amdMIGraphXcompile failed " << std::endl;
        return status;
    }

    input_tensor = vxCreateTensor(context, input_num_of_dims, input_dims, input_data_format, 0);
    output_tensor = vxCreateTensor(context, output_num_of_dims, output_dims, output_data_format, 0);

    std::vector<float> input_digit;
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 9);
    const int rand_digit = dist(rd);
    read_input_digit(rand_digit, input_digit);

    ERROR_CHECK_STATUS(vxMapTensorPatch(input_tensor, input_num_of_dims, nullptr, nullptr, &map_id, stride,
        (void **)&ptr, VX_WRITE_ONLY, VX_MEMORY_TYPE_HOST));
    if (status) {
        std::cerr << "ERROR: vxMapTensorPatch() failed " << std::endl;
        return status;
    }

    memcpy(ptr, static_cast<void*>(input_digit.data()), input_digit.size() * sizeof(float));

    status = vxUnmapTensorPatch(input_tensor, map_id);
    if (status) {
        std::cerr << "ERROR: vxUnmapTensorPatch() failed for " << status << ")" << std::endl;
        return status;
    }

    ERROR_CHECK_STATUS(vxLoadKernels(context, "vx_amd_migraphx"));

    vx_node node = amdMIGraphXnode(graph, &prog, input_tensor, output_tensor);
    ERROR_CHECK_OBJECT(node);

    ERROR_CHECK_STATUS(vxVerifyGraph(graph));
    ERROR_CHECK_STATUS(vxProcessGraph(graph));


    status = vxMapTensorPatch(output_tensor, output_num_of_dims, nullptr, nullptr, &map_id, stride,
        (void **)&ptr, VX_READ_ONLY, VX_MEMORY_TYPE_HOST);
    if (status) {
        std::cerr << "ERROR: vxMapTensorPatch() failed for output tensor" << std::endl;
        return status;
    }

    auto num_results = 10;
    float* max = std::max_element((float*)ptr, (float*)ptr + num_results);
    int answer = max - (float*)ptr;

    std::cout << std::endl
              << "Randomly chosen digit: " << rand_digit << std::endl
              << "Result from inference: " << answer << std::endl
              << std::endl
              << (answer == rand_digit ? "CORRECT" : "INCORRECT") << std::endl
              << std::endl;

    status = vxUnmapTensorPatch(output_tensor, map_id);
    if(status) {
        std::cerr << "ERROR: vxUnmapTensorPatch() failed for output_tensor" << std::endl;
        return status;
    }

    // release resources
    ERROR_CHECK_STATUS(vxReleaseNode(&node));
    ERROR_CHECK_STATUS(vxReleaseGraph(&graph));
    ERROR_CHECK_STATUS(vxReleaseTensor(&input_tensor));
    ERROR_CHECK_STATUS(vxReleaseTensor(&output_tensor));
    ERROR_CHECK_STATUS(vxReleaseContext(&context));

    return 0;
}
