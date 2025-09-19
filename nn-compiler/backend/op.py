from enum import IntEnum, auto
from relay.graph import Node
from typing_extensions import Self


class Op_type(IntEnum):
    TensorAlloc = 0
    TensorFree = auto()
    ShapeInfer = auto()
    Conv = auto()
    BatchNormalization = auto()
    Relu = auto()
    MaxPool = auto()
    Transpose = auto()
    Shape = auto()
    Constant = auto()
    Gather = auto()
    Unsqueeze = auto()
    Concat = auto()
    Reshape = auto()
    Slice = auto()
    Softmax = auto()
    Range = auto()
    ConstantOfShape = auto()
    Mul = auto()
    Equal = auto()
    Where = auto()
    Expand = auto()
    Cast = auto()
    GreaterOrEqual = auto()
    Not = auto()
    Sub = auto()
    MatMul = auto()
    Add = auto()
    ReduceMean = auto()
    Pow = auto()
    Sqrt = auto()
    Div = auto()
    Sigmoid = auto()
    Split = auto()
    Squeeze = auto()
    ReduceSum = auto()
    LogSoftmax = auto()
    TopK = auto()
    PRelu = auto()
    GRU = auto()
    ConvTranspose = auto()
    Max = auto()
    Abs = auto()
    Neg = auto()
    Exp = auto()
    Log = auto()
    GatherElements = auto()
    Tanh = auto()    
    DynamicQuantizeLinear = auto()
    ConvInteger = auto()
    MatMulInteger = auto()
    MemRef = auto()
    Gemm = auto()
    LSTM = auto()
    Pad = auto()
    Atan = auto()
    Greater = auto()
    Less = auto()
    Cos = auto()
    Sin = auto()
    Clip = auto()
    ScatterND = auto()
    Finish = auto()


op_type_map = { x.name: x for x in Op_type }


class Op:
    name : str
    ref : str
    val : str
    node : Node
    op : Self
    def __init__(self, typ: Op_type, attr: dict | None = None):
        self.typ = typ
        if attr:
            for k in attr.keys():
                setattr(self, k, attr[k])

    def __str__(self):
        attr = [x for x in self.__dict__.keys() if x != 'typ']
        if self.typ in [Op_type.TensorAlloc, Op_type.TensorFree,
                        Op_type.MemRef, Op_type.Finish, Op_type.ShapeInfer]:
            attr_desc = ' '.join([f'{k} = {self.__dict__[k]}' for k in attr])
            return f'{self.typ.name}: {attr_desc}'
        else:
            return f'call: {self.node}'

    __repr__ = __str__
