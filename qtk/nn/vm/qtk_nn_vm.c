#include "wtk/core/wtk_alloc.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/core/qtk_binary.h"
#include "qtk/core/qtk_type.h"

#ifdef QBL_DEBUG
#include "qtk/nn/qtk_nn_debug.h"
#endif

#include "qtk/math/qtk_math.h"
#include "qtk/math/qtk_vector.h"
#include "qtk/nn/qtk_nn_abs.h"
#include "qtk/nn/qtk_nn_add.h"
#include "qtk/nn/qtk_nn_allocator.h"
#include "qtk/nn/qtk_nn_atan.h"
#include "qtk/nn/qtk_nn_batchnormalization.h"
#include "qtk/nn/qtk_nn_cast.h"
#include "qtk/nn/qtk_nn_clip.h"
#include "qtk/nn/qtk_nn_concat.h"
#include "qtk/nn/qtk_nn_constant_of_shape.h"
#include "qtk/nn/qtk_nn_conv.h"
#include "qtk/nn/qtk_nn_convq.h"
#include "qtk/nn/qtk_nn_convtranspose.h"
#include "qtk/nn/qtk_nn_cos.h"
#include "qtk/nn/qtk_nn_div.h"
#include "qtk/nn/qtk_nn_equal.h"
#include "qtk/nn/qtk_nn_exp.h"
#include "qtk/nn/qtk_nn_expand.h"
#include "qtk/nn/qtk_nn_gather.h"
#include "qtk/nn/qtk_nn_gather_elements.h"
#include "qtk/nn/qtk_nn_gemm.h"
#include "qtk/nn/qtk_nn_greater.h"
#include "qtk/nn/qtk_nn_greater_or_equal.h"
#include "qtk/nn/qtk_nn_gru.h"
#include "qtk/nn/qtk_nn_less.h"
#include "qtk/nn/qtk_nn_log.h"
#include "qtk/nn/qtk_nn_log_softmax.h"
#include "qtk/nn/qtk_nn_matmul.h"
#include "qtk/nn/qtk_nn_max.h"
#include "qtk/nn/qtk_nn_maxpool.h"
#include "qtk/nn/qtk_nn_mul.h"
#include "qtk/nn/qtk_nn_neg.h"
#include "qtk/nn/qtk_nn_not.h"
#include "qtk/nn/qtk_nn_pad.h"
#include "qtk/nn/qtk_nn_pow.h"
#include "qtk/nn/qtk_nn_prelu.h"
#include "qtk/nn/qtk_nn_quantize.h"
#include "qtk/nn/qtk_nn_quantizemul.h"
#include "qtk/nn/qtk_nn_range.h"
#include "qtk/nn/qtk_nn_reduce_mean.h"
#include "qtk/nn/qtk_nn_reduce_sum.h"
#include "qtk/nn/qtk_nn_relu.h"
#include "qtk/nn/qtk_nn_reshape.h"
#include "qtk/nn/qtk_nn_scatter_nd.h"
#include "qtk/nn/qtk_nn_shape.h"
#include "qtk/nn/qtk_nn_shape_infer.h"
#include "qtk/nn/qtk_nn_sigmoid.h"
#include "qtk/nn/qtk_nn_sin.h"
#include "qtk/nn/qtk_nn_slice.h"
#include "qtk/nn/qtk_nn_softmax.h"
#include "qtk/nn/qtk_nn_split.h"
#include "qtk/nn/qtk_nn_sqrt.h"
#include "qtk/nn/qtk_nn_squeeze.h"
#include "qtk/nn/qtk_nn_sub.h"
#include "qtk/nn/qtk_nn_tanh.h"
#include "qtk/nn/qtk_nn_topk.h"
#include "qtk/nn/qtk_nn_transpose.h"
#include "qtk/nn/qtk_nn_unsqueeze.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/nn/qtk_nn_where.h"
#include "qtk/nn/qtk_nn_sfe.h"
#include "qtk/nn/qtk_nn_erb.h"
#include "qtk/nn/qtk_nn_alignblockoneframe.h"
#include "qtk/numeric/qtk_numeric_type.h"

static int vm_ShapeInfer_(qtk_nn_vm_t *nv, uint8_t **instructions) {
    qtk_nn_vm_op_t op;
    uint16_t extra_index = qtk_littleEndian_uint16_from_bin(*instructions);
    uint8_t *infer_instruction = cast(uint8_t *, nv->extra) + extra_index;

    *instructions += 2;
    op = infer_instruction[0];
    infer_instruction += 1;
#ifdef QBL_DEBUG
    qtk_debug("infer for op: %d\n", op);
#endif

#define SHAPE_INFER_CASE_OP(op)                                                \
    case CAT(QBL_NN_VM_OP_, op):                                               \
        if (CAT(CAT(vm_, op), _infer_shape_)(nv, infer_instruction)) {         \
            goto err;                                                          \
        }                                                                      \
        break

    switch (op) {
        SHAPE_INFER_CASE_OP(Shape);
        SHAPE_INFER_CASE_OP(Gather);
        SHAPE_INFER_CASE_OP(Concat);
        SHAPE_INFER_CASE_OP(Reshape);
        SHAPE_INFER_CASE_OP(Range);
        SHAPE_INFER_CASE_OP(ConstantOfShape);
        SHAPE_INFER_CASE_OP(Mul);
        SHAPE_INFER_CASE_OP(Unsqueeze);
        SHAPE_INFER_CASE_OP(Equal);
        SHAPE_INFER_CASE_OP(Conv);
        SHAPE_INFER_CASE_OP(Where);
        SHAPE_INFER_CASE_OP(Expand);
        SHAPE_INFER_CASE_OP(Cast);
        SHAPE_INFER_CASE_OP(GreaterOrEqual);
        SHAPE_INFER_CASE_OP(Not);
        SHAPE_INFER_CASE_OP(Sub);
        SHAPE_INFER_CASE_OP(Relu);
        SHAPE_INFER_CASE_OP(Transpose);
        SHAPE_INFER_CASE_OP(MatMul);
        SHAPE_INFER_CASE_OP(Add);
        SHAPE_INFER_CASE_OP(Slice);
        SHAPE_INFER_CASE_OP(ReduceMean);
        SHAPE_INFER_CASE_OP(ReduceSum);
        SHAPE_INFER_CASE_OP(Pow);
        SHAPE_INFER_CASE_OP(Sqrt);
        SHAPE_INFER_CASE_OP(Div);
        SHAPE_INFER_CASE_OP(Sigmoid);
        SHAPE_INFER_CASE_OP(Softmax);
        SHAPE_INFER_CASE_OP(LogSoftmax);
        SHAPE_INFER_CASE_OP(Split);
        SHAPE_INFER_CASE_OP(Squeeze);
        SHAPE_INFER_CASE_OP(TopK);
        SHAPE_INFER_CASE_OP(GRU);
        SHAPE_INFER_CASE_OP(PRelu);
        SHAPE_INFER_CASE_OP(ConvTranspose);
        SHAPE_INFER_CASE_OP(Max);
        SHAPE_INFER_CASE_OP(Abs);
        SHAPE_INFER_CASE_OP(Neg);
        SHAPE_INFER_CASE_OP(Exp);
        SHAPE_INFER_CASE_OP(Log);
        SHAPE_INFER_CASE_OP(GatherElements);
        SHAPE_INFER_CASE_OP(Tanh);
        SHAPE_INFER_CASE_OP(DynamicQuantizeLinear);
        SHAPE_INFER_CASE_OP(ConvInteger);
        SHAPE_INFER_CASE_OP(MatMulInteger);
        SHAPE_INFER_CASE_OP(AlignBlockOneFrame);
        SHAPE_INFER_CASE_OP(Sfe);
        SHAPE_INFER_CASE_OP(Erb);
    case QBL_NN_VM_OP_Finish:
        qtk_assert(0);
    default:
        qtk_assert(0);
    }

#undef SHAPE_INFER_CASE_OP

    return 0;
err:
    return -1;
}

static int load_instructions_(qtk_nn_vm_t *nv, qtk_io_reader reader,
                              void *upval) {
    uint32_t sz;
    if (qtk_io_readn(upval, reader, cast(char *, &sz), sizeof(sz))) {
        goto err;
    }
    nv->instructions = wtk_malloc(sz);
    if (qtk_io_readn(upval, reader, nv->instructions, sz)) {
        goto err;
    }
    return 0;
err:
    wtk_debug("Failed\n");
    return -1;
}

static int load_io_desc_(qtk_nn_vm_t *nv, qtk_io_reader reader, void *upval) {
    if (qtk_io_readn(upval, reader, cast(char *, &nv->nin), sizeof(nv->nin))) {
        goto err;
    }
    nv->input_idx = qtk_new_vec(uint16_t, nv->nin);
    if (qtk_io_readn(upval, reader, cast(char *, nv->input_idx),
                     nv->nin * sizeof(uint16_t))) {
        goto err;
    }
    if (qtk_io_readn(upval, reader, cast(char *, &nv->nout),
                     sizeof(nv->nout))) {
        goto err;
    }
    nv->output_idx = qtk_new_vec(uint16_t, nv->nout);
    if (qtk_io_readn(upval, reader, cast(char *, nv->output_idx),
                     nv->nout * sizeof(uint16_t))) {
        goto err;
    }
    return 0;
err:
    wtk_debug("Failed\n");
    return -1;
}

static int load_extra_(qtk_nn_vm_t *nv, qtk_io_reader reader, void *upval) {
    uint32_t sz;
    if (qtk_io_readn(upval, reader, cast(char *, &sz), sizeof(sz))) {
        goto err;
    }

    if (sz > 0) {
        nv->extra = wtk_malloc(sz);
        if (qtk_io_readn(upval, reader, cast(char *, nv->extra), sz)) {
            goto err;
        }
    }

    return 0;
err:
    wtk_debug("Failed\n");
    return -1;
}

static int nn_vm_prepare_input_(qtk_nn_vm_t *nv) {
    qtk_nn_vm_op_t op;
    uint8_t *instructions = nv->instructions;
    for (;;) {
        op = instructions[0];
        instructions += 1;
        switch (op) {
        case QBL_NN_VM_OP_TensorAlloc:
            vm_TensorAlloc_(nv, &instructions);
            break;
        case QBL_NN_VM_OP_TensorFree:
            vm_TensorFree_(nv, &instructions);
            break;
        case QBL_NN_VM_OP_ShapeInfer:
            vm_ShapeInfer_(nv, &instructions);
            break;
        default:
            nv->pc = instructions - 1;
            return 0;
        }
    }
    return -1;
}

int qtk_nn_vm_load(qtk_nn_vm_t *nv, qtk_io_reader reader, void *upval) {
    memset(nv, 0, sizeof(qtk_nn_vm_t));
    if (load_io_desc_(nv, reader, upval) ||
        qtk_nn_vm_tensor_load_initializer(&nv->tensor, reader, upval) ||
        qtk_nn_vm_tensor_load_dynamic(&nv->tensor, reader, upval) ||
        load_instructions_(nv, reader, upval) || load_extra_(nv, reader, upval)
#ifdef QBL_DEBUG
        || qtk_nn_vm_load_debug_info(nv, reader, upval)
#endif
    ) {
        goto err;
    }
    nv->pc = nv->instructions;
    nv->use_profile = 0;
    if (nv->tensor.ncomptime_tensor == 0) {
        nn_vm_prepare_input_(nv);
    }
    return 0;
err:
    qtk_debug("load failed\n");
    qtk_nn_vm_clean(nv);
    return -1;
}

void qtk_nn_vm_clean(qtk_nn_vm_t *nv) {
    qtk_nn_vm_tensor_clean(&nv->tensor);
    if (nv->instructions) {
        wtk_free(nv->instructions);
    }
    if (nv->input_idx) {
        wtk_free(nv->input_idx);
    }
    if (nv->output_idx) {
        wtk_free(nv->output_idx);
    }
    if (nv->extra) {
        wtk_free(nv->extra);
    }
    if (nv->workspace.raw) {
        wtk_free(nv->workspace.raw);
    }
    if (nv->use_profile) {
        qtk_nn_profiler_end(&nv->profiler);
        qtk_nn_profiler_clean(&nv->profiler);
    }
#if QBL_DEBUG
    if (nv->initializer_symbols) {
        for (int i = 0; i < nv->tensor.ninit_tensor; i++) {
            wtk_string_delete(nv->initializer_symbols[i]);
        }
        wtk_free(nv->initializer_symbols);
    }
    if (nv->dynamic_symbols) {
        for (int i = 0; i < nv->tensor.ndyn_tensor; i++) {
            wtk_string_delete(nv->dynamic_symbols[i]);
        }
        wtk_free(nv->dynamic_symbols);
    }
#endif
}

int qtk_nn_vm_run(qtk_nn_vm_t *nv) {
    uint8_t *instructions = cast(uint8_t *, nv->pc);
    qtk_nn_vm_op_t op;
#ifdef QBL_DEBUG
    uint8_t *cur_instruction;
    qtk_nn_vm_dump_input_debug_info(nv);
#endif

#define RUN_CASE_OP(op)                                                        \
    case CAT(QBL_NN_VM_OP_, op):                                               \
        if (nv->use_profile)                                                   \
            qtk_nn_profiler_op_start(&nv->profiler);                           \
        CAT(CAT(vm_, op), _)(nv, &instructions);                               \
        if (nv->use_profile)                                                   \
            qtk_nn_profiler_op_end(&nv->profiler, CAT(QBL_NN_VM_OP_, op));     \
        break

    for (;;) {
        op = instructions[0];
#ifdef QBL_DEBUG
        cur_instruction = instructions;
        qtk_debug("........ Execute %s\n", qtk_nn_vm_get_opname(op));
#endif
        instructions += 1;
        switch (op) {
            RUN_CASE_OP(Transpose);
            RUN_CASE_OP(Shape);
            RUN_CASE_OP(MaxPool);
            RUN_CASE_OP(Gather);
            RUN_CASE_OP(Concat);
            RUN_CASE_OP(Reshape);
            RUN_CASE_OP(Range);
            RUN_CASE_OP(Slice);
            RUN_CASE_OP(Relu);
            RUN_CASE_OP(Unsqueeze);
            RUN_CASE_OP(Softmax);
            RUN_CASE_OP(ConstantOfShape);
            RUN_CASE_OP(Where);
            RUN_CASE_OP(Equal);
            RUN_CASE_OP(Expand);
            RUN_CASE_OP(Cast);
            RUN_CASE_OP(GreaterOrEqual);
            RUN_CASE_OP(Not);
            RUN_CASE_OP(MatMul);
            RUN_CASE_OP(Add);
            RUN_CASE_OP(LogSoftmax);
            RUN_CASE_OP(ReduceMean);
            RUN_CASE_OP(ReduceSum);
            RUN_CASE_OP(Sub);
            RUN_CASE_OP(Pow);
            RUN_CASE_OP(Sqrt);
            RUN_CASE_OP(Div);
            RUN_CASE_OP(Sigmoid);
            RUN_CASE_OP(Mul);
            RUN_CASE_OP(Conv);
            RUN_CASE_OP(Split);
            RUN_CASE_OP(Squeeze);
            RUN_CASE_OP(TopK);
            RUN_CASE_OP(GRU);
            RUN_CASE_OP(PRelu);
            RUN_CASE_OP(ConvTranspose);
            RUN_CASE_OP(Max);
            RUN_CASE_OP(Abs);
            RUN_CASE_OP(Neg);
            RUN_CASE_OP(Exp);
            RUN_CASE_OP(Log);
            RUN_CASE_OP(GatherElements);
            RUN_CASE_OP(Tanh);
            RUN_CASE_OP(DynamicQuantizeLinear);
            RUN_CASE_OP(ConvInteger);
            RUN_CASE_OP(MatMulInteger);
            RUN_CASE_OP(Atan);
            RUN_CASE_OP(Clip);
            RUN_CASE_OP(Cos);
            RUN_CASE_OP(Greater);
            RUN_CASE_OP(Less);
            RUN_CASE_OP(Sin);
            RUN_CASE_OP(Pad);
            RUN_CASE_OP(TensorAlloc);
            RUN_CASE_OP(TensorFree);
            RUN_CASE_OP(ShapeInfer);
            RUN_CASE_OP(MemRef);
            RUN_CASE_OP(BatchNormalization);
            RUN_CASE_OP(Gemm);
            RUN_CASE_OP(ScatterND);
            RUN_CASE_OP(AlignBlockOneFrame);
            RUN_CASE_OP(Sfe);
            RUN_CASE_OP(Erb);
        case QBL_NN_VM_OP_Finish:
            goto end;
            break;
        default:
            qtk_assert(0);
        }
#ifdef QBL_DEBUG
        qtk_nn_vm_dump_debug_info(nv, cur_instruction);
#endif
    }

#undef RUN_CASE_OP
end:
    return 0;
}

int qtk_nn_vm_get_input(qtk_nn_vm_t *nv, int *nin, void **in_addr) {
    if (*nin < nv->nin) {
        return -1;
    }
    *nin = nv->nin;
    for (int i = 0; i < nv->nin; i++) {
        in_addr[i] = qtk_nn_get_dynamic_loc(
            nv, QBL_NN_TENSOR_GET_INDEX(nv->input_idx[i]));
    }
    return 0;
}

int qtk_nn_vm_get_output(qtk_nn_vm_t *nv, int *nout, void **out_addr) {
    if (*nout < nv->nout) {
        return -1;
    }
    *nout = nv->nout;
    for (int i = 0; i < nv->nout; i++) {
        out_addr[i] = qtk_nn_get_dynamic_loc(
            nv, QBL_NN_TENSOR_GET_INDEX(nv->output_idx[i]));
    }
    return 0;
}

int qtk_nn_vm_shape_propagation(qtk_nn_vm_t *nv, int **input_shape) {
    // TODO infer shape, dead code eliminate, memory management
    return 0;
}

int qtk_nn_vm_prepare(qtk_nn_vm_t *nv, int **input_shape) {
    for (int i = 0; i < nv->nin; i++) {
        int rank;
        for (rank = 0; input_shape[i][rank] > 0; rank++)
            ;
        nn_vm_set_dynamic_shape_for_repr_(nv, cast(uint32_t *, input_shape[i]),
                                          rank, nv->input_idx[i]);
    }
    return nn_vm_prepare_input_(nv);
}

int qtk_nn_vm_reset(qtk_nn_vm_t *nv) {
    qtk_nn_vm_tensor_reset(&nv->tensor);
    nv->pc = nv->instructions;
    if (nv->tensor.ncomptime_tensor == 0) {
        nn_vm_prepare_input_(nv);
    }
    return 0;
}

uint32_t *qtk_nn_vm_get_output_shape(qtk_nn_vm_t *nv, int out_idx, int *rank) {
    uint32_t *SHAPE;

    SHAPE = qtk_nn_get_shape_from_repr(nv, nv->output_idx[out_idx]);
    *rank = qtk_nn_get_rank_from_repr(nv, nv->output_idx[out_idx]);

    return SHAPE;
}

uint32_t *qtk_nn_vm_get_input_shape(qtk_nn_vm_t *nv, int in_idx, int *rank) {
    uint32_t *SHAPE;

    SHAPE = qtk_nn_get_shape_from_repr(nv, nv->input_idx[in_idx]);
    *rank = qtk_nn_get_rank_from_repr(nv, nv->input_idx[in_idx]);
    return SHAPE;
}

void qtk_nn_vm_enable_profile(qtk_nn_vm_t *nv) {
    if (nv->use_profile == 0) {
        nv->use_profile = 1;
        qtk_nn_profiler_init(&nv->profiler, nv);
        qtk_nn_profiler_start(&nv->profiler);
    }
}

qtk_nn_vm_tensor_elem_type_t qtk_nn_vm_get_input_elem_type(qtk_nn_vm_t *nv,
                                                           int in_idx) {
    return QBL_NN_TENSOR_GET_ELEM_TYPE(nv->input_idx[in_idx]);
}

qtk_nn_vm_tensor_elem_type_t qtk_nn_vm_get_output_elem_type(qtk_nn_vm_t *nv,
                                                            int out_idx) {
    return QBL_NN_TENSOR_GET_ELEM_TYPE(nv->output_idx[out_idx]);
}
