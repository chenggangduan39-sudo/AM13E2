#ifndef QBL_NN_QBL_NN_ABOF_H
#define QBL_NN_QBL_NN_ABOF_H
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/numeric/qtk_numeric_type.h"
#include "qtk/nn/qtk_nn_conv2d.h"
#include "qtk/nn/qtk_nn_softmax.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static void vm_AlignBlockOneFrame_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    uint16_t x_mic = qtk_littleEndian_uint16_from_bin(*instructions),
             x_ref = qtk_littleEndian_uint16_from_bin(*instructions + 2),
             k = qtk_littleEndian_uint16_from_bin(*instructions + 4),
             v = qtk_littleEndian_uint16_from_bin(*instructions + 6),
             ref = qtk_littleEndian_uint16_from_bin(*instructions + 8),
             mw = qtk_littleEndian_uint16_from_bin(*instructions + 10),
             mb = qtk_littleEndian_uint16_from_bin(*instructions + 12),
             rw = qtk_littleEndian_uint16_from_bin(*instructions + 14),
             rb = qtk_littleEndian_uint16_from_bin(*instructions + 16),
             cw = qtk_littleEndian_uint16_from_bin(*instructions + 18),
             cb = qtk_littleEndian_uint16_from_bin(*instructions + 20),
             y = qtk_littleEndian_uint16_from_bin(*instructions + 22),
             k_out = qtk_littleEndian_uint16_from_bin(*instructions + 24),
             v_out = qtk_littleEndian_uint16_from_bin(*instructions + 26),
             ref_out = qtk_littleEndian_uint16_from_bin(*instructions + 28);

    qtk_numeric_data_t X_MIC, X_REF, K, V1, REF, MW, MB, RW, RB, CW, CB, Y, KOUT, VOUT, REFOUT;
    X_MIC.raw = qtk_nn_get_loc_from_repr(nv, x_mic);
    X_REF.raw = qtk_nn_get_loc_from_repr(nv, x_ref);
    K.raw = qtk_nn_get_loc_from_repr(nv, k);
    V1.raw = qtk_nn_get_loc_from_repr(nv, v);
    REF.raw = qtk_nn_get_loc_from_repr(nv, ref);
    MW.raw = qtk_nn_get_loc_from_repr(nv, mw);
    MB.raw = qtk_nn_get_loc_from_repr(nv, mb);
    RW.raw = qtk_nn_get_loc_from_repr(nv, rw);
    RB.raw = qtk_nn_get_loc_from_repr(nv, rb);
    CW.raw = qtk_nn_get_loc_from_repr(nv, cw);
    CB.raw = qtk_nn_get_loc_from_repr(nv, cb);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    KOUT.raw = qtk_nn_get_loc_from_repr(nv, k_out);
    VOUT.raw = qtk_nn_get_loc_from_repr(nv, v_out);
    REFOUT.raw = qtk_nn_get_loc_from_repr(nv, ref_out);

    *instructions += 30;
    // int extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    // *instructions += 2;
    // uint8_t *extra = cast(uint8_t *, nv->extra) + extra_index;
    // int delay;
    // memcpy(&delay, extra, sizeof(int));

    uint32_t *in_shape = qtk_nn_get_shape_from_repr(nv, x_mic);
    uint8_t dilations[2] = {1,1};
    int ksize[2] = {1,1}, pads[4] = {0,0,0,0}, strides[2] = {1,1};
    int groups = 1;
    int workspace_size = 0;
    int output_h = (in_shape[2] + pads[0] + pads[2] -
                            (dilations[0] * (ksize[0] - 1) + 1)) /
                                strides[0] + 1;
    int output_w = (in_shape[3] + pads[1] + pads[3] -
                            (dilations[1] * (ksize[1] - 1) + 1)) /
                                strides[1] + 1;
    int out_chan = qtk_nn_get_init_shape(nv, mw & QBL_NN_TENSOR_INDEX_MASK)[0];

    //conv2d X 2
    int size = in_shape[1] / groups * ksize[0] * ksize[1] * output_h * output_w;
    workspace_size += sizeof(float) * size * 2;
    int size2 = in_shape[0] * out_chan * output_h * output_w;
    workspace_size += sizeof(float) * size2 * 2;

    //k_cache
    uint32_t *k_cache_shape = qtk_nn_get_shape_from_repr(nv, k);
    int row = k_cache_shape[0] * k_cache_shape[1];
    int col = (k_cache_shape[2] + 1) * k_cache_shape[3];
    int col2 = k_cache_shape[2] * k_cache_shape[3];
    int size3 = row * col;
    workspace_size += size3;
    //v
    int size4 = k_cache_shape[0] * k_cache_shape[1] * (k_cache_shape[2] + 1);
    workspace_size += size4;
    //v_cache
    uint32_t *v_cache_shape = qtk_nn_get_shape_from_repr(nv, v);
    int v_row = v_cache_shape[0] * v_cache_shape[1];
    int v_col = (v_cache_shape[2] + 1) * v_cache_shape[3];
    int v_col2 = v_cache_shape[2] * v_cache_shape[3];
    int size5 = v_row * v_col;
    workspace_size += size5;

    //v conv2d
    int ksize2[2] = {5,3}, pads2[4] = {0,1,0,1};
    uint32_t in_shape2[4];
    in_shape2[0] = v_cache_shape[0];
    in_shape2[1] = v_cache_shape[1];
    in_shape2[2] = v_cache_shape[2] + 1;
    in_shape2[3] = v_cache_shape[3];

    int output_h2 = (in_shape2[2] + pads2[0] + pads2[2] -
                            (dilations[0] * (ksize2[0] - 1) + 1)) /
                                strides[0] + 1;
    int output_w2 = (in_shape2[3] + pads2[1] + pads2[3] -
                            (dilations[1] * (ksize2[1] - 1) + 1)) /
                                strides[1] + 1;
    int out_chan2 = qtk_nn_get_init_shape(nv, cw & QBL_NN_TENSOR_INDEX_MASK)[0];
    int size6 = in_shape2[1] / groups * ksize2[0] * ksize2[1] * output_h2 * output_w2;
    workspace_size += size6;
    int size7 = in_shape2[0] * out_chan2 * output_h2 * output_w2;
    workspace_size += size7;

    int size8 = in_shape2[3];
    workspace_size += size8;

    uint32_t *ref_cache_shape = qtk_nn_get_shape_from_repr(nv, ref);
    int ref_row = ref_cache_shape[0] * ref_cache_shape[1];
    int ref_col = (ref_cache_shape[2] + 1) * ref_cache_shape[3];
    int ref_col2 = ref_cache_shape[2] * ref_cache_shape[3];
    int size9 = ref_row * ref_col;
    workspace_size += size9;

    ensure_workspace_(nv, sizeof(float) * workspace_size);
    memset(nv->workspace.f32, 0, workspace_size * sizeof(float));

    float *conv1_workspace = nv->workspace.f32;
    float *conv1_out = conv1_workspace + size;
    float *conv2_workspace = conv1_out + size2;
    float *conv2_out = conv2_workspace + size;
    float *Ku = conv2_out + size2;
    float *V = Ku + size3;
    float *V2 = V + size4;
    float *vu_workspace = V2 + size5;
    float *Vu = vu_workspace + size6;
    float *A = Vu + size7;
    float *Yu = A + size8;
    nn_conv2d_naive_(X_MIC.f32, conv1_out, MW, MB, pads, ksize, strides, out_chan, groups,
                        in_shape, dilations, conv1_workspace, 1, 0);
    nn_conv2d_naive_(X_REF.f32, conv2_out, RW, RB, pads, ksize, strides, out_chan, groups,
                        in_shape, dilations, conv2_workspace, 1, 0);
    //print_float(conv2_out,size2);
    for(int i = 0; i < k_cache_shape[0] * k_cache_shape[1]; i++){
        memcpy(Ku + i * col, K.f32 + i * col2, sizeof(float) * col2);
        memcpy(Ku + i * col + col2, conv2_out + i * k_cache_shape[3], sizeof(float) * k_cache_shape[3]);
        memcpy(KOUT.f32 + i * col2, Ku + i * col + k_cache_shape[3], sizeof(float) * col2);
    }

    float *p1,*p2,*p;
    for(int i = 0; i < k_cache_shape[0] * k_cache_shape[1]; i++){
        for(int j = 0; j < k_cache_shape[2] + 1; j++){
            p1 = conv1_out + i * k_cache_shape[3];
            p2 = Ku + i * col + j * k_cache_shape[3];
            p = V + (k_cache_shape[2] + 1) * i + j;
            for(int m = 0; m < k_cache_shape[3]; m++, p1++,p2++){
                *p += *p1 * *p2;
            }
        }
    }
    for(int i = 0; i < v_cache_shape[0] * v_cache_shape[1]; i++){
        memcpy(V2 + i * v_col, V1.f32 + i * v_col2, sizeof(float) * v_col2);
        memcpy(V2 + i * v_col + v_col2, V + i * v_cache_shape[3], sizeof(float) * v_cache_shape[3]);
        memcpy(VOUT.f32 + i * v_col2, V2 + i * v_col + v_cache_shape[3], sizeof(float) * v_col2);
    }

    nn_conv2d_naive_(V2, Vu, CW, CB, pads2, ksize2, strides, out_chan2, groups,
                        in_shape2, dilations, vu_workspace, 1, 0);
    nn_softmax_(Vu, A, 1, size8);
    for(int i = 0; i < ref_cache_shape[0] * ref_cache_shape[1]; i++){
        memcpy(Yu + i * ref_col, REF.f32 + i * ref_col2, sizeof(float) * ref_col2);
        memcpy(Yu + i * ref_col + ref_col2, X_REF.f32 + i * ref_cache_shape[3], sizeof(float) * ref_cache_shape[3]);
        memcpy(REFOUT.f32 + i * ref_col2, Yu + i * ref_col + ref_cache_shape[3], sizeof(float) * ref_col2);
    }

    //wtk_debug("%d %d %d\n",ref_cache_shape[0] * ref_cache_shape[1],ref_cache_shape[2] + 1,ref_cache_shape[3]);
    for(int i = 0; i < ref_cache_shape[0] * ref_cache_shape[1]; i++){
        for(int j = 0; j < ref_cache_shape[2] + 1; j++){
            p2 = Yu + i * ref_col + j * ref_cache_shape[3];
            p1 = A + j;
            p = Y.f32 + ref_cache_shape[3] * i;
            for(int m = 0; m < ref_cache_shape[3]; m++, p++,p2++){
                *p += *p1 * *p2;
            }
        }
    }
}

qtk_maybe_unused static int vm_AlignBlockOneFrame_infer_shape_(qtk_nn_vm_t *nv,
                                                 uint8_t *instructions) {
    //TODO
    return 0;
}

#ifdef __cplusplus
};

#endif
#endif
