#ifndef QBL_NN_QBL_NN_CONV_H
#define QBL_NN_QBL_NN_CONV_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_conv1d.h"
#include "qtk/nn/qtk_nn_conv2d.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Conv_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    uint8_t dilations[16], kernel_shape[16], pads[16], strides[16];
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             b = qtk_littleEndian_uint16_from_bin(instructions + 4),
             y = qtk_littleEndian_uint16_from_bin(instructions + 6);
    uint16_t b_idx = b & QBL_NN_TENSOR_INDEX_MASK;
    uint8_t *extra;
    int conv_dim;
    uint32_t nfilter;
    uint32_t *in_shape, out_shape[32];

    extra = instructions + 8;
    conv_dim = extra[0];
    extra += 1;
    memcpy(dilations, extra, conv_dim);
    memcpy(kernel_shape, extra + conv_dim + 2, conv_dim);
    memcpy(pads, extra + conv_dim + 2 + conv_dim, conv_dim << 1);
    memcpy(strides, extra + (conv_dim << 2) + 2, conv_dim);

    in_shape = qtk_nn_get_shape_from_repr(nv, x);
    nfilter = qtk_nn_get_init_shape(nv, b_idx)[0];

    qtk_assert(dilations[0] == 1);

    if (conv_dim == 1) {
        out_shape[0] = in_shape[0];
        out_shape[1] = nfilter;
        out_shape[2] = qtk_nn_pad_dim(in_shape[2], pads[0], pads[1],
                                      kernel_shape[0], strides[0]);
        nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, 3, y);
    } else {
        out_shape[0] = in_shape[0];
        out_shape[1] = nfilter;
        out_shape[2] = qtk_nn_pad_dim(in_shape[2], pads[0], pads[2],
                                      kernel_shape[0], strides[0]);
        out_shape[3] = qtk_nn_pad_dim(in_shape[3], pads[1], pads[3],
                                      kernel_shape[0], strides[0]);
        nn_vm_set_dynamic_shape_for_repr_(nv, out_shape, 4, y);
    }
    return 0;
}

qtk_maybe_unused static void vm_Conv_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint8_t dilations[16], kernel_shape[16], pads[16], strides[16];
    int ksize[2];
    int groups;
    int conv_dim;
    int st[2], pd[4];
    uint32_t *in_shape, *shapeY, *shapeB;
    uint32_t out_chan;
    uint16_t extra_index;
    uint16_t x = qtk_littleEndian_uint16_from_bin(*instructions),
             w = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             b = qtk_littleEndian_uint16_from_bin(*instructions + 4);
    uint16_t y;
    int rankB = qtk_nn_get_rank_from_repr(nv, b);
    qtk_numeric_data_t B;
    qtk_numeric_data_t X, W, Y;
    uint8_t *extra;
    int hasB = 1;
    int prelu = 0;
    if(rankB != 1){
        y = b;
        hasB = 0;
        *instructions += 6;
    }else{
        B.raw = qtk_nn_get_loc_from_repr(nv, b);
        y = qtk_littleEndian_uint16_from_bin(*instructions + 6);
        *instructions += 8;
    }
    
    uint16_t x_idx = x & QBL_NN_TENSOR_INDEX_MASK,
             w_idx = w & QBL_NN_TENSOR_INDEX_MASK,
             y_idx = y & QBL_NN_TENSOR_INDEX_MASK;
    X.raw = qtk_nn_get_dynamic_loc(nv, x_idx);
    Y.raw = qtk_nn_get_dynamic_loc(nv, y_idx);
    W.raw = qtk_nn_get_loc_from_repr(nv, w);

    out_chan = qtk_nn_get_init_shape(nv, w_idx)[0];

    in_shape = qtk_nn_get_shape_from_repr(nv, x);
    shapeY = qtk_nn_get_shape_from_repr(nv, y);

    extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    *instructions += 2;

    extra = cast(uint8_t *, nv->extra) + extra_index;
    conv_dim = extra[0];
    extra += 1;
    memcpy(dilations, extra, conv_dim);
    groups = qtk_littleEndian_uint16_from_bin(extra + conv_dim);
    memcpy(kernel_shape, extra + conv_dim + 2, conv_dim);
    memcpy(pads, extra + conv_dim + 2 + conv_dim, conv_dim << 1);
    memcpy(strides, extra + (conv_dim << 2) + 2, conv_dim);
    st[0] = strides[0];
    st[1] = strides[1];
    pd[0] = pads[0];
    pd[1] = pads[1];
    pd[2] = pads[2];
    pd[3] = pads[3];
    if (conv_dim == 2) {
        ksize[0] = kernel_shape[0];
        ksize[1] = kernel_shape[1];
        // fallback naive implementation
        if (dilations[0] || dilations[1]) {
            const int output_h = (in_shape[2] + pads[0] + pads[2] -
                                  (dilations[0] * (ksize[0] - 1) + 1)) /
                                     st[0] +
                                 1;
            const int output_w = (in_shape[3] + pads[1] + pads[3] -
                                  (dilations[1] * (ksize[1] - 1) + 1)) /
                                     st[1] +
                                 1;
            ensure_workspace_(nv, sizeof(float) * in_shape[1] / groups *
                                      ksize[0] * ksize[1] * output_h *
                                      output_w);
            if(hasB){
                shapeB = qtk_nn_get_shape_from_repr(nv, b);
                if(shapeB[0] != out_chan){
                    prelu = 1;
                }
            }
            nn_conv2d_naive_(X.f32, Y.f32, W, B, pd, ksize, st, out_chan, groups,
                             in_shape, dilations, nv->workspace.f32, hasB, prelu);
            return;
        }
        qtk_assert(dilations[0] == dilations[1] && dilations[0] == 1 &&
                   ksize[1] == kernel_shape[1]);
        int workspace_size= 0;
        float *conv_in = X.f32;
        float **conv_idx;
        int nin_w;
        int nin_h;
        int pad_sz;
        uint32_t insha[4];
        memcpy(insha, in_shape, 4 * sizeof(uint32_t));
        if(pd[0] != 0 || pd[1] != 0){
            nin_w = in_shape[3] + pd[1] * 2;
            nin_h = in_shape[2] + pd[0] * 2;
            insha[2] = nin_h;
            insha[3] = nin_w;
            pad_sz = nin_w * nin_h * in_shape[1] * in_shape[0];
            workspace_size +=  pad_sz * sizeof(float);//pad
        }

        if (ksize[0] != 1 || ksize[1] != 1) {
            int channels_col = in_shape[1]/groups * ksize[0] * ksize[1];
            int height_col = (in_shape[2] + pd[0] * 2 - ksize[0]) / strides[0] + 1;
            workspace_size += sizeof(float*) * channels_col * height_col;
        }
        ensure_workspace_(nv, workspace_size);
        if(pd[0] != 0 || pd[1] != 0){
            nn_conv2d_pad(nv->workspace.f32, X.f32, in_shape, pd[0], pd[1]);
            conv_in = nv->workspace.f32;
            conv_idx = (float**)(nv->workspace.f32 + pad_sz);
        }else{
            conv_idx = nv->workspace.fp32;
        }

        nn_conv2d_(conv_in, conv_idx, Y, W, B, ksize, st, out_chan, groups,
                insha, hasB);
    } else if (conv_dim == 1) {
        ensure_workspace_(nv, sizeof(float) * out_chan / groups *
                                  kernel_shape[0] *
                                  qtk_nn_pad_dim(in_shape[2], pads[0], pads[1],
                                                 kernel_shape[0], strides[0]));
        nn_conv1d_(X, Y, W, B, pads[0], kernel_shape[0], strides[0], out_chan,
                   groups, in_shape, nv->workspace);
    } else {
        qtk_assert(0);
    }
}

#ifdef __cplusplus
};
#endif
#endif
