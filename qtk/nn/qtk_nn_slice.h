#ifndef G_45WT2DYB_7PSM_D7PO_M5EP_Y2WL8E2IVNON
#define G_45WT2DYB_7PSM_D7PO_M5EP_Y2WL8E2IVNON
#pragma once
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_maybe_unused static int vm_Slice_infer_shape_(qtk_nn_vm_t *nv,
                                                  uint8_t *instructions) {
    uint16_t x = qtk_littleEndian_uint16_from_bin(instructions),
             starts = qtk_littleEndian_uint16_from_bin(instructions + 2),
             ends = qtk_littleEndian_uint16_from_bin(instructions + 4),
             axes = qtk_littleEndian_uint16_from_bin(instructions + 6),
             steps = qtk_littleEndian_uint16_from_bin(instructions + 8),
             y = qtk_littleEndian_uint16_from_bin(instructions + 10);
    uint32_t *shapeX, shapeY[32];
    int argRank, rankX;
    qtk_numeric_data_t STARTS, ENDS, STEPS, AXES;
    uint32_t sliced_shape[32];
    int axes_real[32];

    shapeX = qtk_nn_get_shape_from_repr(nv, x);
    rankX = qtk_nn_get_rank_from_repr(nv, x);
    argRank = qtk_nn_get_rank_from_repr(nv, starts);

    STARTS.raw = qtk_nn_get_loc_from_repr(nv, starts);
    ENDS.raw = qtk_nn_get_loc_from_repr(nv, ends);
    STEPS.raw = qtk_nn_get_loc_from_repr(nv, steps);
    AXES.raw = qtk_nn_get_loc_from_repr(nv, axes);

    qtk_assert(QBL_NN_TENSOR_GET_ELEM_TYPE(starts) ==
               QBL_NN_VM_TENSOR_ELEM_I32);

    for (int i = 0; i < argRank; i++) {
        axes_real[i] = (AXES.i32[i] + rankX) % rankX;
    }

    // https://numpy.org/doc/stable/user/basics.indexing.html?highlight=slice#slicing-and-striding
    for (int i = 0; i < argRank; i++) {
        int real_start, real_end, q, r;
        real_start = STARTS.i32[i] >= 0 ? STARTS.i32[i]
                                        : shapeX[axes_real[i]] + STARTS.i32[i];
        real_end =
            ENDS.i32[i] >= 0 ? ENDS.i32[i] : shapeX[axes_real[i]] + ENDS.i32[i];
        q = (real_end - real_start) / STEPS.i32[i];
        r = (real_end - real_start) % STEPS.i32[i];
        sliced_shape[i] = q + (r != 0);
    }

    memcpy(shapeY, shapeX, sizeof(qtk_nn_vm_tensor_shape_type_t) * rankX);
    for (int i = 0; i < argRank; i++) {
        shapeY[axes_real[i]] = sliced_shape[i];
    }

    nn_vm_set_dynamic_shape_for_repr_(nv, shapeY, rankX, y);
    return 0;
}

static void slice_kernel_(void *x, void *y, int idx, uint32_t *starts,
                          uint32_t *ends, uint32_t *steps, uint32_t *lda_x,
                          uint32_t *lda_y, int elem_sz) {
    if (lda_x[idx] == 1) { // last dim
        for (uint32_t i = starts[idx]; i < ends[idx]; i += steps[idx]) {
            memcpy(cast(char *, y) + (i - starts[idx]) * elem_sz / steps[idx],
                   cast(char *, x) + i * elem_sz, elem_sz);
        }
    } else {
        for (uint32_t i = starts[idx]; i < ends[idx]; i += steps[idx]) {
            slice_kernel_(cast(char *, x) + i * lda_x[idx] * elem_sz,
                          cast(char *, y) + (i - starts[idx]) * lda_y[idx] * elem_sz /
                                  steps[idx],
                          idx + 1, starts, ends, steps, lda_x, lda_y, elem_sz);
        }
    }
}

qtk_maybe_unused static void vm_Slice_(qtk_nn_vm_t *nv,
                                       uint8_t **instructions) {
    uint8_t ninp = *instructions[0];
    uint16_t x, starts, ends, axes, steps, y;
    uint8_t narg;
    uint32_t starts_detail[32];
    uint32_t ends_detail[32];
    uint32_t steps_detail[32],step_d[32];
    uint8_t axes_hint[32];
    uint8_t axes_merge[32];
    uint8_t skiped;
    uint32_t *in_shape,*out_shape;
    uint32_t shape_merge[32];
    uint32_t shape_merge_y[32];
    uint32_t lda_merge_x[32];
    uint32_t lda_merge_y[32];
    uint32_t lda_x = 1, lda_y = 1;
    int ndim;
    int merge_idx = 0;
    int idx_merging = 0;
    uint32_t merge_dim = 1;
    qtk_numeric_data_t X, Y, STARTS, ENDS, STEPS, AXES;
    if(ninp == 5){
        x = qtk_littleEndian_uint16_from_bin(*instructions + 1);
        starts = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (1 << 1));
        ends = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (2 << 1));
        axes = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (3 << 1));
        steps = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (4 << 1));
        y = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (5 << 1));
        STEPS.raw = qtk_nn_get_loc_from_repr(nv, steps);
        narg = qtk_nn_get_rank_from_repr(nv, starts);
        memcpy(step_d, STEPS.i32, sizeof(int32_t)*narg);
        *instructions += 13;
    }else{
        x = qtk_littleEndian_uint16_from_bin(*instructions + 1);
        starts = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (1 << 1));
        ends = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (2 << 1));
        axes = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (3 << 1));
        //steps = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (4 << 1));
        y = qtk_littleEndian_uint16_from_bin(*instructions + 1 + (4 << 1));
        narg = qtk_nn_get_rank_from_repr(nv, starts);
        for(int i=0;i<narg;i++){
            step_d[i] = 1;
        }
        *instructions += 11;
    }

    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    Y.raw = qtk_nn_get_loc_from_repr(nv, y);
    STARTS.raw = qtk_nn_get_loc_from_repr(nv, starts);
    ENDS.raw = qtk_nn_get_loc_from_repr(nv, ends);
    AXES.raw = qtk_nn_get_loc_from_repr(nv, axes);
    in_shape = qtk_nn_get_shape_from_repr(nv, x);
    out_shape = qtk_nn_get_shape_from_repr(nv, y);
    ndim = qtk_nn_get_rank_from_repr(nv, x); 

    for (int i = 0; i < ndim; i++) {
        int found = 0;
        for (int j = 0; j < narg; j++) {
            int axes = AXES.i32[j];
            if(axes < 0){
                AXES.i32[j] = ndim - 1;
            }
            if (axes == i) {
                found = 1;
                break;
            }
        }
        axes_hint[i] = found == 1 ? 1 : 0;
    }

    for (int i = 0; i < ndim; i++) {
        if (axes_hint[i] == 0) {
            idx_merging = 1;
            merge_dim *= in_shape[i];
        } else {
            if (idx_merging) {
                shape_merge[merge_idx++] = merge_dim;
                merge_dim = 1;
            }
            shape_merge[merge_idx++] = in_shape[i];
            idx_merging = 0;
        }
    }

    if (idx_merging) {
        shape_merge[merge_idx++] = merge_dim;
    }

    if(narg == 1 && AXES.i32[0] == 0){
        axes_merge[0] = 0;
    }else{
        for (int i = 0; i < narg; i++) {
            skiped = 0;
            for (int j = 0; j < AXES.i32[i]; j++) {
                skiped += axes_hint[i] == 0;
            }
            axes_merge[i] = AXES.i32[i] - skiped + 1;
        }
    }

    for (int i = merge_idx - 1; i >= 0; i--) {
        uint32_t start = 0;
        uint32_t end = shape_merge[i];
        uint32_t step = 1;
        for (int j = 0; j < narg; j++) {
            if (axes_merge[j] == i) {
                start = STARTS.i32[j] >= 0 ? STARTS.i32[j]
                                           : STARTS.i32[j] + shape_merge[i];

                end = ENDS.i32[j] >= 0 ? ENDS.i32[j]
                                       : ENDS.i32[j] + shape_merge[i];
                if (end > shape_merge[i]) {
                    end = shape_merge[i];
                }
                step = step_d[j];//STEPS.i32[j];
            }
        }
        starts_detail[i] = start;
        ends_detail[i] = end;
        steps_detail[i] = step;
        shape_merge_y[i] = (end - start) / step;
        lda_merge_x[i] = lda_x;
        lda_merge_y[i] = lda_y;
        //wtk_debug("%d %d %d %d %d %d\n",start,end,step,(end - start) / step,lda_x,lda_y);
        lda_x *= shape_merge[i];
        lda_y *= shape_merge_y[i];
    }

    slice_kernel_(X.raw, Y.raw, 0, starts_detail, ends_detail, steps_detail,
                  lda_merge_x, lda_merge_y,
                  nn_vm_tensor_elem_sz_[QBL_NN_TENSOR_GET_ELEM_TYPE(y)]);
}

#ifdef __cplusplus
};
#endif
#endif /* G_45WT2DYB_7PSM_D7PO_M5EP_Y2WL8E2IVNON */
