#ifndef QBL_NN_QBL_NN_CONVTRANSPOSE_H
#define QBL_NN_QBL_NN_CONVTRANSPOSE_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int
vm_ConvTranspose_infer_shape_(qtk_nn_vm_t *nv, uint8_t *instructions) {
    // TODO
    return 0;
}

static void conv_transpose_f32_(qtk_nn_vm_t *nv, float *X, float *Y, float *W,
                                float *B, uint8_t *dilations, uint8_t *strides,
                                uint8_t *pads, uint32_t *shapeX,
                                uint32_t *shapeY, uint32_t *shapeW, int group) {
    uint32_t n = shapeX[0];
    uint32_t c = shapeX[1];
    uint32_t h = shapeX[2];
    uint32_t w = shapeX[3];

    int stride_h = strides[0];
    int stride_w = strides[1];
    int pad_top = pads[0];
    int pad_left = pads[1];
    int ksize_h = shapeW[2];
    int ksize_w = shapeW[3];
    int nfilter = shapeW[1];
    int dilation_h = dilations[0];
    int dilation_w = dilations[1];

    int in_group_sz = c / group;
    for (uint32_t _n = 0; _n < n; _n++) {
        for (uint32_t _c = 0; _c < in_group_sz; _c++) {
            for (uint32_t _h = 0; _h < h; _h++) {
                for (uint32_t _w = 0; _w < w; _w++) {
                    for (int _g = 0; _g < group; _g++) {
                        float input_val =
                            X[_n * c * h * w + (_g * in_group_sz + _c) * h * w +
                              _h * w + _w];
                        for (uint32_t _nf = 0; _nf < nfilter; _nf++) {
                            for (int _kh = 0; _kh < ksize_h; _kh++) {
                                for (int _kw = 0; _kw < ksize_w; _kw++) {
                                    float weight_val =
                                        W[(_g * in_group_sz + _c) * nfilter *
                                              ksize_h * ksize_w +
                                          _nf * ksize_h * ksize_w +
                                          _kh * ksize_w + _kw];
                                    uint32_t dst_h =
                                        _h * stride_h + _kh * dilation_h;
                                    uint32_t dst_w =
                                        _w * stride_w + _kw * dilation_w;
                                    if (dst_h < pad_top || dst_w < pad_left) {
                                        continue;
                                    }
                                    dst_h -= pad_top;
                                    dst_w -= pad_left;
                                    if (dst_h >= shapeY[2] ||
                                        dst_w >= shapeY[3]) {
                                        continue;
                                    }
                                    Y[_n * shapeY[1] * shapeY[2] * shapeY[3] +
                                      (_g * nfilter + _nf) * shapeY[2] *
                                          shapeY[3] +
                                      dst_h * shapeY[3] + dst_w] +=
                                        input_val * weight_val;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (B) {
        for (uint32_t _n = 0; _n < n; _n++) {
            float *outptr = Y + _n * shapeY[1] * shapeY[2] * shapeY[3];
            int nelem = shapeY[3] * shapeY[2];
            for (int p = 0; p < group * nfilter; p++) {
                for (int i = 0; i < nelem; i++) {
                    outptr[i] += B[p];
                }
                outptr += nelem;
            }
        }
    }
}

static void float_fill_(float *data, int size, float value){
    for(int i = 0; i < size; i++){
        data[i] = value;
    }
}

static int ncnn_transpose_forward_(float* input, float* weight, float *bias_ptr, float *output,
    uint8_t *dilations, uint8_t *strides, uint8_t *pads, uint8_t *kernel, 
    int *in_shape, int *output_shape, int *weight_shape, int group, qtk_nn_vm_t *nv, int prelu){
    size_t elemsize = sizeof(float);
    int w = in_shape[3];
    int h = in_shape[2];
    int elempack = 1;//TODO
    int dilation_w = dilations[1];
    int dilation_h = dilations[0];
    int stride_w = strides[1];
    int stride_h = strides[0];
    int pad_left = pads[1];
    int pad_top = pads[0];
    int pad_right = pads[3];
    int pad_bottom = pads[2];
    int kernel_w = kernel[1];
    int kernel_h = kernel[0];

    int kernel_extent_w = dilation_w * (kernel_w - 1) + 1;
    int kernel_extent_h = dilation_h * (kernel_h - 1) + 1;
    int outw = (w - 1) * stride_w + kernel_extent_w;// + output_pad_right;  TODO
    int outh = (h - 1) * stride_h + kernel_extent_h;// + output_pad_bottom; TODO
    int out_elempack = 1;
    int num_output = weight_shape[1] * group;
    size_t out_elemsize = elemsize/elempack * out_elempack;
    int out_channels = num_output/out_elempack;
    
    int maxk = kernel_h * kernel_w;

    int m = maxk * num_output;
    int n = w * h;
    int k = weight_shape[0];//TOFO weight_data_size / maxk / num_output;
    float *a = weight;
    float *b = input;

    int out_size = out_elemsize * outw * outh * out_channels;//need cut to final output
    int out_gemm_size = m * n * out_elemsize;
    ensure_workspace_(nv, out_size + out_gemm_size);
    memset(nv->workspace.f32, 0, out_size + out_gemm_size);

    float *c = nv->workspace.f32;
    qtk_sgemm(1, 0, m, n, k, 1.0, a, m, b, n, 0.0, c, n);
    //print_float(c,m * n);
    float *d = nv->workspace.f32 + out_gemm_size/out_elemsize;
    int gap = (outw * stride_h - w * stride_w) * out_elempack;
    for (int p = 0; p < out_channels; p++)
    {
        float* sptr = c + p * maxk * n;//top_col2im.row(p * maxk);
        float* outm = d + p * outw * outh * out_elempack;

        float bias = !bias_ptr ? 0.f : bias_ptr[p];
        float_fill_(outm, outw * outh * out_elempack, bias);
        for (int u = 0; u < kernel_h; u++)
        {
            for (int v = 0; v < kernel_w; v++)
            {
                float* ptr = outm +  outw * (dilation_h * u) + dilation_w * v;

                for (int i = 0; i < h; i++)
                {
                    for (int j = 0; j < w; j++)
                    {
                        ptr[0] += sptr[0];

                        ptr += stride_w;
                        sptr += 1;
                    }

                    ptr += gap;
                }
            }
        }
    }
    //print_float(d, out_channels * outw * outh * out_elempack);
    float *out = output;
    if(!prelu){
        for (int p = 0; p < out_channels; p++){
            float* outm = d + p * outw * outh;
            for(int i = 0; i < output_shape[2]; i++){
                float *otmp = outm + (pad_top + i) * outw + pad_left;
                for(int j = 0; j < output_shape[3]; j++,out++,otmp++){
                    *out = *otmp;
                }
            }
        }
    }else{
        float *slope = bias_ptr + out_channels;
        for (int p = 0; p < out_channels; p++){
            float* outm = d + p * outw * outh;
            for(int i = 0; i < output_shape[2]; i++){
                float *otmp = outm + (pad_top + i) * outw + pad_left;
                for(int j = 0; j < output_shape[3]; j++,out++,otmp++){
                    if(*otmp > 0){
                        *out = *otmp;
                    }else{
                        *out = *otmp * slope[p];
                    }
                }
            }
        }
    }
    //exit(0);
    //print_float(output,output_shape[0]*output_shape[1]*output_shape[2]*output_shape[3]);
    return 0;
}

qtk_maybe_unused static void vm_ConvTranspose_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint8_t dilations[16], kernel_shape[16], pads[16], strides[16];
    int conv_dim;
    uint16_t extra_index;
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             w = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 6);
    uint16_t x_idx = x & QBL_NN_TENSOR_INDEX_MASK,
             y_idx = y & QBL_NN_TENSOR_INDEX_MASK;
    qtk_numeric_data_t X, W, B, Y; 
    uint8_t *extra;

    X.raw = qtk_nn_get_dynamic_loc(nv, x_idx);
    Y.raw = qtk_nn_get_dynamic_loc(nv, y_idx);

    W.raw = qtk_nn_get_loc_from_repr(nv, w);
    B.raw = qtk_nn_get_loc_from_repr(nv, b);

    *instructions += 8;
    extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;

    extra = cast(uint8_t *, nv->extra) + extra_index;
    conv_dim = extra[0];
    extra += 1;

    memcpy(dilations, extra, conv_dim);
    uint16_t group;
    memcpy(&group, extra + conv_dim, sizeof(group));
    memcpy(kernel_shape, extra + conv_dim + 2, conv_dim);
    memcpy(pads, extra + conv_dim + 2 + conv_dim, conv_dim << 1);
    memcpy(strides, extra + (conv_dim << 2) + 2, conv_dim);

    uint32_t *shapeX = qtk_nn_get_shape_from_repr(nv, x);
    uint32_t *shapeY = qtk_nn_get_shape_from_repr(nv, y);
    uint32_t *shapeW = qtk_nn_get_shape_from_repr(nv, w);

    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(y) == QBL_NN_VM_TENSOR_ELEM_F32);
    memset(Y.f32, 0,
           sizeof(float) * shapeY[0] * shapeY[1] * shapeY[2] * shapeY[3]);
    if(group > 1){
        conv_transpose_f32_(nv, X.f32, Y.f32, W.f32, B.f32, dilations, strides,
                    pads, shapeX, shapeY, shapeW, group);
    }else{
        uint32_t *shapeB = qtk_nn_get_shape_from_repr(nv, b);
        int prelu = 0;
        if(shapeB[0] > shapeW[0]){
            prelu = 1;
        }
        ncnn_transpose_forward_(X.f32,W.f32,B.f32,Y.f32,dilations,strides,
            pads,kernel_shape,shapeX,shapeY,shapeW,group,nv,prelu);
    }
}

#ifdef __cplusplus
};
#endif
#endif
