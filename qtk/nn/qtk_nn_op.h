#ifndef G_85KGNOQQ_PUF1_VO50_OCAF_1QUNJNN7Z8H4
#define G_85KGNOQQ_PUF1_VO50_OCAF_1QUNJNN7Z8H4
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    QBL_NN_VM_OP_TensorAlloc,
    QBL_NN_VM_OP_TensorFree,
    QBL_NN_VM_OP_ShapeInfer,
    QBL_NN_VM_OP_Conv,
    QBL_NN_VM_OP_BatchNormalization,
    QBL_NN_VM_OP_Relu,
    QBL_NN_VM_OP_MaxPool,
    QBL_NN_VM_OP_Transpose,
    QBL_NN_VM_OP_Shape,
    QBL_NN_VM_OP_Constant,
    QBL_NN_VM_OP_Gather,
    QBL_NN_VM_OP_Unsqueeze,
    QBL_NN_VM_OP_Concat,
    QBL_NN_VM_OP_Reshape,
    QBL_NN_VM_OP_Slice,
    QBL_NN_VM_OP_Softmax,
    QBL_NN_VM_OP_Range,
    QBL_NN_VM_OP_ConstantOfShape,
    QBL_NN_VM_OP_Mul,
    QBL_NN_VM_OP_Equal,
    QBL_NN_VM_OP_Where,
    QBL_NN_VM_OP_Expand,
    QBL_NN_VM_OP_Cast,
    QBL_NN_VM_OP_GreaterOrEqual,
    QBL_NN_VM_OP_Not,
    QBL_NN_VM_OP_Sub,
    QBL_NN_VM_OP_MatMul,
    QBL_NN_VM_OP_Add,
    QBL_NN_VM_OP_ReduceMean,
    QBL_NN_VM_OP_Pow,
    QBL_NN_VM_OP_Sqrt,
    QBL_NN_VM_OP_Div,
    QBL_NN_VM_OP_Sigmoid,
    QBL_NN_VM_OP_Split,
    QBL_NN_VM_OP_Squeeze,
    QBL_NN_VM_OP_ReduceSum,
    QBL_NN_VM_OP_LogSoftmax,
    QBL_NN_VM_OP_TopK,
    QBL_NN_VM_OP_PRelu,
    QBL_NN_VM_OP_GRU,
    QBL_NN_VM_OP_ConvTranspose,
    QBL_NN_VM_OP_Max,
    QBL_NN_VM_OP_Abs,
    QBL_NN_VM_OP_Neg,
    QBL_NN_VM_OP_Exp,
    QBL_NN_VM_OP_Log,
    QBL_NN_VM_OP_GatherElements,
    QBL_NN_VM_OP_Tanh,
    QBL_NN_VM_OP_DynamicQuantizeLinear,
    QBL_NN_VM_OP_ConvInteger,
    QBL_NN_VM_OP_MatMulInteger,
    QBL_NN_VM_OP_MemRef,
    QBL_NN_VM_OP_Gemm,
    QBL_NN_VM_OP_LSTM,
    QBL_NN_VM_OP_Pad,
    QBL_NN_VM_OP_Atan,
    QBL_NN_VM_OP_Greater,
    QBL_NN_VM_OP_Less,
    QBL_NN_VM_OP_Cos,
    QBL_NN_VM_OP_Sin,
    QBL_NN_VM_OP_Clip,
    QBL_NN_VM_OP_ScatterND,
    QBL_NN_VM_OP_Finish,
    QBL_NN_VM_OP_Erb,
    QBL_NN_VM_OP_Sfe,
    QBL_NN_VM_OP_AlignBlockOneFrame,
} qtk_nn_vm_op_t;

typedef enum {
    QTK_NN_VM_DIRECTION_FORWARD,
    QTK_NN_VM_DIRECTION_REVERSE,
    QTK_NN_VM_DIRECTION_BIDIRECTIONAL,
} qtk_nn_vm_direction_t;

const char *qtk_nn_vm_get_opname(qtk_nn_vm_op_t op);

#ifdef __cplusplus
};
#endif
#endif /* G_85KGNOQQ_PUF1_VO50_OCAF_1QUNJNN7Z8H4 */
