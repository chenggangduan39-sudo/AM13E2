#ifdef QBL_DEBUG
#include "qtk/nn/qtk_nn_debug.h"
#include "qtk/core/qtk_binary.h"
#include "qtk/nn/qtk_nn_utils.h"
#include "qtk/os/qtk_file.h"
#include "qtk/serde/qtk_serde_np.h"

wtk_string_t qtk_nn_vm_get_symbol(qtk_nn_vm_t *nv, uint16_t repr) {
    if (QBL_NN_TENSOR_GET_TYPE(repr) == QBL_NN_VM_TENSOR_DYNAMIC) {
        return *nv->dynamic_symbols[QBL_NN_TENSOR_GET_INDEX(repr)];
    }
    return *nv->initializer_symbols[QBL_NN_TENSOR_GET_INDEX(repr)];
}

void qtk_nn_vm_save_symbol(qtk_nn_vm_t *nv, uint16_t x) {
    char path[2048];
    qtk_numeric_data_t X;
    X.raw = qtk_nn_get_loc_from_repr(nv, x);
    uint32_t *shapeX;
    int rank;
    wtk_string_t sym = qtk_nn_vm_get_symbol(nv, x);
    shapeX = qtk_nn_get_shape_from_repr(nv, x);
    rank = qtk_nn_get_rank_from_repr(nv, x);
    snprintf(path, sizeof(path), "/tmp/qnn-%.*s.npy", sym.len, sym.data);
    qtk_debug("dump %s...\n", path);
    switch (QBL_NN_TENSOR_GET_ELEM_TYPE(x)) {
    case QBL_NN_VM_TENSOR_ELEM_F32:
        qtk_serde_np_f32_tofile1(path, X.f32, cast(int *, shapeX), rank);
        break;
    case QBL_NN_VM_TENSOR_ELEM_I32:
        qtk_serde_np_i32_tofile1(path, X.i32, cast(int *, shapeX), rank);
        break;
    case QBL_NN_VM_TENSOR_ELEM_I64:
        qtk_serde_np_i64_tofile1(path, X.i64, cast(int *, shapeX), rank);
        break;
    case QBL_NN_VM_TENSOR_ELEM_BOOL:
        qtk_serde_np_i8_tofile1(path, X.boolean, cast(int *, shapeX), rank);
        break;
    case QBL_NN_VM_TENSOR_ELEM_U8:
        qtk_serde_np_u8_tofile1(path, X.u8, cast(int *, shapeX), rank);
        break;
    default: 
        qtk_assert(0);
    }
}

void qtk_nn_vm_print_symbol(qtk_nn_vm_t *nv, qtk_nn_vm_repr_t repr) {
    uint32_t *shape;
    wtk_string_t sym;
    qtk_numeric_data_t data;
    uint32_t nelem, ndisplay_elem;
    qtk_nn_vm_tensor_elem_type_t elem_type;
    char buf[128];
    int rank, offset = 0;

    sym = qtk_nn_vm_get_symbol(nv, repr);
    shape = qtk_nn_get_shape_from_repr(nv, repr);
    data.raw = qtk_nn_get_loc_from_repr(nv, repr);
    rank = qtk_nn_get_rank_from_repr(nv, repr);

    for (int i = 0; i < rank; i++) {
        offset += snprintf(buf + offset, sizeof(buf) - offset, "%d,", shape[i]);
    }

    elem_type = QBL_NN_TENSOR_GET_ELEM_TYPE(repr);
    printf("%.*s/<%s>[%d]:\n", sym.len, sym.data, buf, elem_type);

    nelem = nn_vm_tensor_get_nelem_from_repr_(nv, repr);
    ndisplay_elem = min(nelem, 50);

    switch (elem_type) {
    case QBL_NN_VM_TENSOR_ELEM_F32:
        for (int i = 0; i < ndisplay_elem; i++) {
            printf("%f, ", data.f32[i]);
        }
        break;
    case QBL_NN_VM_TENSOR_ELEM_I64:
        for (int i = 0; i < ndisplay_elem; i++) {
            printf("%ld, ", data.i64[i]);
        }
        break;
    default:
        printf("<<=======>>");
    }

    if (ndisplay_elem != nelem) {
        printf("....");
    }
    printf("\n");
}

int qtk_nn_vm_load_debug_info(qtk_nn_vm_t *nv, qtk_io_reader reader,
                              void *upval) {
    uint16_t nelem;
    nelem = nv->tensor.ninit_tensor;
    if (nelem > 0) {
        nv->initializer_symbols = wtk_calloc(nelem, sizeof(wtk_string_t *));
    } else {
        nv->initializer_symbols = NULL;
    }
    for (int i = 0; i < nelem; i++) {
        uint16_t len;
        if (qtk_io_readn(upval, reader, cast(char *, &len), sizeof(len))) {
            goto err;
        }
        nv->initializer_symbols[i] = wtk_string_new(len);
        if (qtk_io_readn(upval, reader, nv->initializer_symbols[i]->data,
                         len)) {
            goto err;
        }
    }

    nelem = nv->tensor.ndyn_tensor;
    if (nelem > 0) {
        nv->dynamic_symbols = wtk_calloc(nelem, sizeof(wtk_string_t *));
    } else {
        nv->dynamic_symbols = NULL;
    }
    for (int i = 0; i < nelem; i++) {
        uint16_t len;
        if (qtk_io_readn(upval, reader, cast(char *, &len), sizeof(len))) {
            goto err;
        }
        nv->dynamic_symbols[i] = wtk_string_new(len);
        if (qtk_io_readn(upval, reader, nv->dynamic_symbols[i]->data, len)) {
            goto err;
        }
    }
    return 0;
err:
    return -1;
}

void qtk_nn_vm_dump_debug_info(qtk_nn_vm_t *nv, uint8_t *cur_instruction) {
    qtk_nn_vm_op_t op = *cur_instruction;
    cur_instruction++;
    uint16_t y = 0;
    uint8_t ninput;

    switch (op) {
    case QBL_NN_VM_OP_TensorAlloc:
    case QBL_NN_VM_OP_TensorFree:
    case QBL_NN_VM_OP_ShapeInfer:
    case QBL_NN_VM_OP_MemRef:
        return;
    case QBL_NN_VM_OP_Conv:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        int rankB = qtk_nn_get_rank_from_repr(nv, y);
        if (rankB == 1) {
            y = qtk_littleEndian_uint16_from_bin(cur_instruction + 6);
        }
        break;
    case QBL_NN_VM_OP_BatchNormalization:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 10);
        return;
    case QBL_NN_VM_OP_Relu:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_MaxPool:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Transpose:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Shape:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Gather:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        break;
    case QBL_NN_VM_OP_Unsqueeze:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        break;
    case QBL_NN_VM_OP_Concat:
        ninput = cur_instruction[0];
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 1 +
                                             (ninput << 1));
        break;
    case QBL_NN_VM_OP_Reshape:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        break;
    case QBL_NN_VM_OP_Slice:{
        uint8_t ninp = cur_instruction[0];
        if(ninp == 5) {
            y = qtk_littleEndian_uint16_from_bin(cur_instruction + 11);
        }else{
            y = qtk_littleEndian_uint16_from_bin(cur_instruction + 9);
        }
        break;
    }
    case QBL_NN_VM_OP_Softmax:
    case QBL_NN_VM_OP_LogSoftmax:
    case QBL_NN_VM_OP_Atan:
    case QBL_NN_VM_OP_Cos:
    case QBL_NN_VM_OP_Sin:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Finish:
        return;
    case QBL_NN_VM_OP_Range:
    case QBL_NN_VM_OP_Clip:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 6);
        break;
    case QBL_NN_VM_OP_ConstantOfShape:
    case QBL_NN_VM_OP_Cast:
    case QBL_NN_VM_OP_Not:
    case QBL_NN_VM_OP_Sqrt:
    case QBL_NN_VM_OP_Sigmoid:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Mul:
    case QBL_NN_VM_OP_Equal:
    case QBL_NN_VM_OP_Expand:
    case QBL_NN_VM_OP_GreaterOrEqual:
    case QBL_NN_VM_OP_Sub:
    case QBL_NN_VM_OP_MatMul:
    case QBL_NN_VM_OP_Add:
    case QBL_NN_VM_OP_Pow:
    case QBL_NN_VM_OP_Div:
    case QBL_NN_VM_OP_Pad:
    case QBL_NN_VM_OP_Greater:
    case QBL_NN_VM_OP_Less:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        break;
    case QBL_NN_VM_OP_Split: {
        uint8_t ninp = cur_instruction[0];
        uint8_t noutp = cur_instruction[1 + (ninp << 1)];
        for (int i = 0; i < noutp - 1; i++) {
            y = qtk_littleEndian_uint16_from_bin(cur_instruction + 1 +
                                                 (ninp << 1) + 1 + (i << 1));
            qtk_nn_vm_save_symbol(nv, y);
        }
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 1 + (ninp << 1) +
                                             1 + ((noutp - 1) << 1));
    } break;
    case QBL_NN_VM_OP_Squeeze: {
        uint8_t ninp = cur_instruction[0];
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 1 + (ninp << 1));
    } break;
    case QBL_NN_VM_OP_Where:
    case QBL_NN_VM_OP_Gemm:
    case QBL_NN_VM_OP_ScatterND:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 6);
        break;
    case QBL_NN_VM_OP_TopK: {
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        qtk_nn_vm_save_symbol(nv, y);
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 6);
    } break;
    case QBL_NN_VM_OP_ReduceSum:
    case QBL_NN_VM_OP_ReduceMean:
    case QBL_NN_VM_OP_PRelu:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        break;
    case QBL_NN_VM_OP_GRU:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 12);
        qtk_nn_vm_save_symbol(nv, y);
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 14);
        break;
    case QBL_NN_VM_OP_ConvTranspose:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 6);
        break;
    case QBL_NN_VM_OP_Max:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        break;
    case QBL_NN_VM_OP_Abs:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Neg:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Log:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_GatherElements:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        break;
    case QBL_NN_VM_OP_Tanh:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_Exp:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    case QBL_NN_VM_OP_DynamicQuantizeLinear:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        qtk_nn_vm_save_symbol(nv, y);
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 4);
        qtk_nn_vm_save_symbol(nv, y);
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 6);
        break;
    case QBL_NN_VM_OP_ConvInteger:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 8);
        break;
    case QBL_NN_VM_OP_MatMulInteger:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 8);
        break;
    case QBL_NN_VM_OP_AlignBlockOneFrame:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 22);
        qtk_nn_vm_save_symbol(nv, y);
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 24);
        qtk_nn_vm_save_symbol(nv, y);
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 26);
        qtk_nn_vm_save_symbol(nv, y);
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 28);
        break;
    case QBL_NN_VM_OP_Erb:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 8);
        break;
    case QBL_NN_VM_OP_Sfe:
        y = qtk_littleEndian_uint16_from_bin(cur_instruction + 2);
        break;
    default:
        qtk_debug("%d\n", op);
        qtk_assert(0);
    }

    qtk_nn_vm_save_symbol(nv, y);
}

void qtk_nn_vm_dump_input_debug_info(qtk_nn_vm_t *nv) {
    for (int i = 0; i < nv->nin; i++) {
        qtk_nn_vm_save_symbol(nv, nv->input_idx[i]);
    }
}

#endif
