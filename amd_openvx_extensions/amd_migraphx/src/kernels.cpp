/*
Copyright (c) 2015 - 2022 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "kernels.h"

////////////////////////////////////////////////////////////////////////////
//! \brief The module entry point for publishing kernel.
SHARED_PUBLIC vx_status VX_API_CALL vxPublishKernels(vx_context context) {
    // register kernels
    ERROR_CHECK_STATUS(amd_vx_migraphx_node_publish(context));
    return VX_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////
//! \brief The module entry point for unpublishing kernel.
SHARED_PUBLIC vx_status VX_API_CALL vxUnpublishKernels(vx_context context) {
    // TBD: remove kernels
    return VX_SUCCESS;
}

vx_node createMIGraphXNode(vx_graph graph, const char *kernelName, vx_reference params[], vx_uint32 num) {
    vx_status status = VX_SUCCESS;
    vx_node node = 0;
    vx_context context = vxGetContext((vx_reference)graph);
    vx_kernel kernel = vxGetKernelByName(context, kernelName);
    if (kernel) {
        node = vxCreateGenericNode(graph, kernel);
        if (node) {
            vx_uint32 p = 0;
            for (p = 0; p < num; p++) {
                if (params[p]) {
                    status = vxSetParameterByIndex(node, p, params[p]);
                    if (status != VX_SUCCESS) {
                        vxAddLogEntry((vx_reference)graph, status, "MIGraphXNode: vxSetParameterByIndex(%s, %d, 0x%p) => %d\n", kernelName, p, params[p], status);
                        vxReleaseNode(&node);
                        node = 0;
                        break;
                    }
                }
            }
        }
        else {
            vxAddLogEntry((vx_reference)graph, VX_ERROR_INVALID_PARAMETERS, "Failed to create node with kernel %s\n", kernelName);
            status = VX_ERROR_NO_MEMORY;
        }
        vxReleaseKernel(&kernel);
    }
    else {
        vxAddLogEntry((vx_reference)graph, VX_ERROR_INVALID_PARAMETERS, "failed to retrieve kernel %s\n", kernelName);
        status = VX_ERROR_NOT_SUPPORTED;
    }
    return node;
}

vx_enum get_vx_type(migraphx_shape_datatype_t migraphx_type) {

    vx_enum vx_type = VX_TYPE_INVALID;

    switch (migraphx_type) {
        case migraphx_shape_bool_type:
            vx_type = VX_TYPE_BOOL;
            break;
        case migraphx_shape_half_type:
            vx_type = VX_TYPE_FLOAT16;
            break;
        case migraphx_shape_float_type:
            vx_type = VX_TYPE_FLOAT32;
            break;
        case migraphx_shape_double_type:
            vx_type = VX_TYPE_FLOAT64;
            break;
        case migraphx_shape_uint8_type:
            vx_type = VX_TYPE_UINT8;
            break;
        case migraphx_shape_int8_type:
            vx_type = VX_TYPE_INT8;
            break;
        case migraphx_shape_uint16_type:
            vx_type = VX_TYPE_UINT16;
            break;
        case migraphx_shape_int16_type:
            vx_type = VX_TYPE_INT16;
            break;
        case migraphx_shape_int32_type:
            vx_type = VX_TYPE_INT32;
            break;
        case migraphx_shape_int64_type:
            vx_type = VX_TYPE_INT64;
            break;
        case migraphx_shape_uint32_type:
            vx_type = VX_TYPE_UINT32;
            break;
        case migraphx_shape_uint64_type:
            vx_type = VX_TYPE_UINT64;
            break;
        default:
            vx_type = VX_TYPE_INVALID;
    }

    return vx_type;
}