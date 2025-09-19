import onnx
from typing import Union, List, Sequence
from relay.data_type import elem_type_map, elem_type2onnx, elem_type_transform_map , elem_type2np
import numpy as np
import onnxsim
import onnxscript


def _shape_from_onnx(os: onnx.TensorShapeProto):
    shape = []
    for dim in os.dim:
        if not dim.HasField('dim_value'):
            return None
        else:
            shape.append(dim.dim_value)
    return shape


class ValueInfo:
    @classmethod
    def from_onnx(cls, vi: onnx.ValueInfoProto):
        name = vi.name
        doc = vi.doc_string
        if vi.type.HasField('tensor_type'):
            shape = _shape_from_onnx(vi.type.tensor_type.shape) \
                if vi.type.tensor_type.HasField('shape') else None
            elem_type = elem_type_map[vi.type.tensor_type.elem_type][0]
            return TensorInfo(name, elem_type, shape, doc)
        else:
            raise Exception('ValueInfoProto Type Not Impl')

    def __init__(self, name: str, doc: str):
        self.name = name
        self.doc = doc

    def __str__(self):
        return f'name: {self.name}'

    __repr__ = __str__


class TensorInfo(ValueInfo):
    def __init__(self, name: str, elem_type: str, shape: List[int] | None, doc: str = ''):
        self.elem_type = elem_type
        self.shape = shape
        super().__init__(name, doc)

    def __str__(self):
        return f'{{ <{super().__str__()}> shape: {self.shape}, type: {self.elem_type} }}'

    def export_onnx(self) -> onnx.ValueInfoProto:
        return onnx.helper.make_tensor_value_info(self.name, elem_type2onnx[self.elem_type], self.shape)

    __repr__ = __str__


class Tensor:
    def __init__(self, data: np.ndarray, name: str, elem_type: str, doc: str = ''):
        self.data = data
        self.doc = doc
        self.name = name
        self.elem_type = elem_type

    @classmethod
    def from_onnx(cls, ot: onnx.TensorProto):
        type_str, type_np = elem_type_map[ot.data_type]
        if len(ot.raw_data) == 0:
            values = {
                'f32': lambda ot: list(ot.float_data),
                'i64': lambda ot: list(ot.int64_data),
                'u8': lambda ot: list(ot.int32_data),
            }[type_str](ot)
            return cls(np.array(values, dtype=type_np), ot.name, type_str, ot.doc_string)
        else:
            return cls(np.frombuffer(ot.raw_data, dtype=type_np).reshape(ot.dims), ot.name, type_str, ot.doc_string)

    @classmethod
    def from_onnxscript(cls, name, tensor: onnxscript.tensor.Tensor):
        data = tensor.value
        return cls(data, name, elem_type_map[tensor.onnx_dtype][0], '')

    def tobytes(self) -> bytes:
        return self.data.tobytes()

    def export_onnx(self) -> onnx.TensorProto:
        onnx_tensor = onnx.numpy_helper.from_array(self.data)
        onnx_tensor.name = self.name
        return onnx_tensor


class Node:
    def __init__(self, name: str, inp: Sequence, outp: Sequence, op_type: str, doc: str = ''):
        self.name = name
        self.input = inp
        self.output = outp
        self.op_type = op_type
        self.doc = doc

    def __str__(self):
        input_str = ','.join(self.input)
        output_str = ','.join(self.output)
        return f'{self.op_type}({input_str}) -> {output_str}'

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        node_from_onnx = {
            'Conv': Conv.from_onnx,
            'BatchNormalization': BatchNormalization.from_onnx,
            'Relu': Relu.from_onnx,
            'MaxPool': MaxPool.from_onnx,
            'Transpose': Transpose.from_onnx,
            'Shape': Shape.from_onnx,
            'Constant': Constant.from_onnx,
            'Gather': Gather.from_onnx,
            'Unsqueeze': Unsqueeze.from_onnx,
            'Concat': Concat.from_onnx,
            'Reshape': Reshape.from_onnx,
            'Slice': Slice.from_onnx,
            'Softmax': Softmax.from_onnx,
            'LogSoftmax': LogSoftmax.from_onnx,
            'Range': Range.from_onnx,
            'ConstantOfShape': ConstantOfShape.from_onnx,
            'Mul': Mul.from_onnx,
            'Equal': Equal.from_onnx,
            'Where': Where.from_onnx,
            'Expand': Expand.from_onnx,
            'Cast': Cast.from_onnx,
            'GreaterOrEqual': GreaterOrEqual.from_onnx,
            'Not': Not.from_onnx,
            'Sub': Sub.from_onnx,
            'MatMul': MatMul.from_onnx,
            'Add': Add.from_onnx,
            'ReduceMean': ReduceMean.from_onnx,
            'ReduceSum': ReduceSum.from_onnx,
            'Pow': Pow.from_onnx,
            'Sqrt': Sqrt.from_onnx,
            'Div': Div.from_onnx,
            'Sigmoid': Sigmoid.from_onnx,
            'Split': Split.from_onnx,
            'Squeeze': Squeeze.from_onnx,
            'TopK': TopK.from_onnx,
            'PRelu': PRelu.from_onnx,
            'GRU': GRU.from_onnx,
            'ConvTranspose': ConvTranspose.from_onnx,
            'Max':Max.from_onnx,
            'Abs':Abs.from_onnx,
            'Neg':Neg.from_onnx,
            'Exp':Exp.from_onnx,
            'Log':Log.from_onnx,
            'GatherElements':GatherElements.from_onnx,
            'Tanh':Tanh.from_onnx,
            'DynamicQuantizeLinear':DynamicQuantizeLinear.from_onnx,
            'ConvInteger':ConvInteger.from_onnx,
            'MatMulInteger':MatMulInteger.from_onnx,
            'Pad': Pad.from_onnx,
            'Atan': Atan.from_onnx,
            'Greater': Greater.from_onnx,
            'Less': Less.from_onnx,
            'Cos': Cos.from_onnx,
            'Sin': Sin.from_onnx,
            'Clip': Clip.from_onnx,
            'Gemm': Gemm.from_onnx,
            'ScatterND': ScatterND.from_onnx,
        }
        return node_from_onnx[on.op_type](on, **kwargs)

    def export_onnx(self) -> onnx.NodeProto:
        onnx_node = onnx.helper.make_node(self.op_type, self.input, self.output, name = self.name, doc_string=self.doc)
        return onnx_node

    def forward(self, ns: dict[str, Tensor]):
        raise Exception(f'not implemented for {self}')

    __repr__ = __str__


class Clip(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Clip')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Greater(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Greater')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Cos(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Cos')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Sin(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Sin')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Less(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Less')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Pad(Node):
    def __init__(self, name, inp, outp, mode='constant', value=0.0):
        assert mode == 'constant' and value == 0
        super().__init__(name, inp, outp, 'Pad')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        if len(on.input) == 3:
            assert on.input[2] == ''
        return cls(on.name, on.input[:2], on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp, mode='constant')
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Atan(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Atan')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Pow(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Pow')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Sqrt(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Sqrt')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Div(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Div')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Sigmoid(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Sigmoid')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Squeeze(Node):
    def __init__(self, name, inp, outp, doc=''):
        super().__init__(name, inp, outp, 'Squeeze', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output, doc=on.doc_string)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


# Opset 13 axes is an attribute, replace with Opset 18 definition for compatibility with ReduceSum semantics
class ReduceMean(Node):
    def __init__(self, name, inp, outp, *, doc='', keepdims=1):
        super().__init__(name, inp, outp, 'ReduceMean', doc)
        self.keepdims = keepdims
        assert (keepdims == 1)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        input_names = list(on.input)
        for attr in on.attribute:
            if attr.name == 'keepdims':
                extra_arg['keepdims'] = attr.i
            if attr.name == 'axes':
                axes = np.array(list(attr.ints), dtype=np.int64)
                axes_name = f'{on.name}_axes__'
                initializer = kwargs['initializer']
                initializer += [Tensor(axes, axes_name, 'i64')]
                input_names += [axes_name]
        return cls(on.name, input_names, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset18.ReduceMean(*inp, keepdims=self.keepdims)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class TopK(Node):
    def __init__(self, name, inp, outp, *, doc='', largest, axis, is_sorted):
        super().__init__(name, inp, outp, 'TopK', doc)
        self.axis = axis
        self.is_sorted = is_sorted
        self.largest = largest

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'axis':
                extra_arg['axis'] = attr.i
            if attr.name == 'largest':
                extra_arg['largest'] = attr.i
            if attr.name == 'sorted':
                extra_arg['is_sorted'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)


class ReduceSum(Node):
    def __init__(self, name, inp, outp, *, doc='', keepdims=1, noop_with_empty_axes):
        super().__init__(name, inp, outp, 'ReduceSum', doc)
        self.keepdims = keepdims
        self.noop_with_empty_axes = noop_with_empty_axes

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'noop_with_empty_axes':
                extra_arg['noop_with_empty_axes'] = attr.i
            if attr.name == 'keepdims':
                extra_arg['keepdims'] = attr.i
        if 'noop_with_empty_axes' not in extra_arg:
            extra_arg['noop_with_empty_axes'] = 0
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        inp[1] = inp[1].astype(np.int64)
        output = onnxscript.opset13.ReduceSum(*inp, keepdims=self.keepdims, noop_with_empty_axes=self.noop_with_empty_axes)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Split(Node):
    def __init__(self, name, inp, outp, *, doc='', axis):
        super().__init__(name, inp, outp, 'Split', doc)
        self.axis = axis

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'axis':
                extra_arg['axis'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)


class Conv(Node):
    def __init__(self, name, inp, outp, *, dilations, group, kernel_shape, strides, pads=None, doc=''):
        super().__init__(name, inp, outp, 'Conv', doc)
        self.dilations = dilations
        self.group = group
        self.kernel_shape = kernel_shape
        self.pads = pads if pads else [0] * len(kernel_shape)
        self.strides = strides

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'dilations':
                extra_arg['dilations'] = attr.ints
            elif attr.name == 'group':
                extra_arg['group'] = attr.i
            elif attr.name == 'kernel_shape':
                extra_arg['kernel_shape'] = attr.ints
            elif attr.name == 'pads':
                extra_arg['pads'] = attr.ints
            elif attr.name == 'strides':
                extra_arg['strides'] = attr.ints
            elif attr.name == 'auto_pad':
                assert(attr.s == b'NOTSET')
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, onnxscript.tensor.Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.Conv(*inp,
                                         dilations=self.dilations,
                                         group=self.group,
                                         kernel_shape=self.kernel_shape,
                                         strides=self.strides,
                                         pads=self.pads)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class ConvInteger(Node):
    def __init__(self, name, inp, outp, *, dilations, group, kernel_shape, strides, pads=None, doc=''):
        super().__init__(name, inp, outp, 'ConvInteger', doc)
        self.dilations = dilations
        self.group = group
        self.kernel_shape = kernel_shape
        self.pads = pads if pads else [0] * len(kernel_shape)
        self.strides = strides

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'dilations':
                extra_arg['dilations'] = attr.ints
            elif attr.name == 'group':
                extra_arg['group'] = attr.i
            elif attr.name == 'kernel_shape':
                extra_arg['kernel_shape'] = attr.ints
            elif attr.name == 'pads':
                extra_arg['pads'] = attr.ints
            elif attr.name == 'strides':
                extra_arg['strides'] = attr.ints
            elif attr.name == 'auto_pad':
                assert(attr.s == b'NOTSET')
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, onnxscript.tensor.Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.ConvInteger(*inp,
                                                dilations=self.dilations,
                                                group=self.group,
                                                kernel_shape=self.kernel_shape,
                                                strides=self.strides,
                                                pads=self.pads)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class BatchNormalization(Node):
    def __init__(self, name, inp, outp, *, epsilon=1e-5, momentum=0.9, doc=''):
        self.epsilon = epsilon
        self.momentum = momentum
        super().__init__(name, inp, outp, 'BatchNormalization', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'epsilon':
                extra_arg['epsilon'] = attr.f
            elif attr.name == 'momentum':
                extra_arg['momentum'] = attr.f
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, onnxscript.tensor.Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.BatchNormalization(*inp, epsilon=self.epsilon, momentum=self.momentum)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Relu(Node):
    def __init__(self, name, inp, outp, doc=''):
        super().__init__(name, inp, outp, 'Relu', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output, doc=on.doc_string)


class MaxPool(Node):
    def __init__(self, name, inp, outp, *, ceil_mode=0, dilations, kernel_shape, pads, strides, doc=''):
        super().__init__(name, inp, outp, 'MaxPool', doc)
        self.ceil_mode = ceil_mode
        self.dilations = dilations
        self.kernel_shape = kernel_shape
        self.pads = pads
        self.strides = strides

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'dilations':
                extra_arg['dilations'] = attr.ints
            elif attr.name == 'ceil_mode':
                extra_arg['ceil_mode'] = attr.i
            elif attr.name == 'kernel_shape':
                extra_arg['kernel_shape'] = attr.ints
            elif attr.name == 'pads':
                extra_arg['pads'] = attr.ints
            elif attr.name == 'strides':
                extra_arg['strides'] = attr.ints
            elif attr.name == 'storage_order':
                assert (attr.i == 0)
        if 'pads' not in extra_arg:
            extra_arg['pads'] = [0] * len(extra_arg['kernel_shape']) * 2
        if 'dilations' not in extra_arg:
            extra_arg['dilations'] = [1] * len(extra_arg['kernel_shape'])
        return cls(on.name, on.input, on.output, **extra_arg)


class Transpose(Node):
    def __init__(self, name, inp, outp, *, perm, doc=''):
        super().__init__(name, inp, outp, 'Transpose', doc)
        self.perm = perm

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'perm':
                extra_arg['perm'] = attr.ints
        return cls(on.name, on.input, on.output,  **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = ns[self.input[0]].data
        output = onnxscript.opset13.Transpose(inp, perm=self.perm)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Shape(Node):
    def __init__(self, name, inp, outp, *, start=0, end=None, doc=''):
        super().__init__(name, inp, outp, 'Shape', doc)
        self.start = start
        self.end = end

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'start':
                extra_arg['start'] = attr.i
            elif attr_name == 'end':
                extra_arg['end'] = attr.i
        if 'start' not in extra_arg:
            extra_arg['start'] = 0
        if 'end' not in extra_arg:
            extra_arg['end'] = 127  # runtime will check max_input_dim
        return cls(on.name, on.input, on.output, **extra_arg)


class Constant(Node):
    def __init__(self, name, inp, outp, *, val: Tensor, doc=''):
        super().__init__(name, inp, outp, 'Constant', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'value':
                extra_arg['val'] = Tensor.from_onnx(attr.t)
            else:
                raise Exception(f'Not Impl For {attr.name}')
        return cls(on.name, on.input, on.output, **extra_arg)


class Gather(Node):
    def __init__(self, name, inp, outp, *, axis=0, doc=''):
        super().__init__(name, inp, outp, 'Gather', doc)
        self.axis = axis

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'axis':
                extra_arg['axis'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.Gather(*inp, axis=self.axis)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Unsqueeze(Node):
    def __init__(self, name, inp, outp, doc=''):
        super().__init__(name, inp, outp, 'Unsqueeze', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        input_names = list(on.input)
        if len(on.input) == 1:
            for attr in on.attribute:
                if attr.name == 'axes':
                    axes = np.array(list(attr.ints), dtype=np.int64)
                    break
            else:
                raise Exception('axes not found')
            initializer = kwargs['initializer']
            axes_name = f'{on.name}_axes'
            initializer += [Tensor(axes, axes_name, 'i64')]
            input_names += [axes_name]
        return cls(on.name, input_names, on.output, doc=on.doc_string)

    def forward(self, ns: dict[str, Tensor]):
        x = ns[self.input[0]].data
        axes = ns[self.input[1]].data.astype(np.int64)
        output = onnxscript.opset13.Unsqueeze(x, axes)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Concat(Node):
    def __init__(self, name, inp, outp, *, axis=0, doc=''):
        super().__init__(name, inp, outp, 'Concat', doc)
        self.axis = axis

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'axis':
                extra_arg['axis'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.Concat(*inp, axis=self.axis)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Reshape(Node):
    def __init__(self, name, inp, outp, *, allowzero=0, doc=''):
        super().__init__(name, inp, outp, 'Reshape', doc)
        self.allowzero = allowzero

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'allowzero':
                extra_arg['allowzero'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        inp[1] = inp[1].astype(np.int64)
        output = onnxscript.opset13.Reshape(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Slice(Node):
    def __init__(self, name, inp, outp, doc=''):
        super().__init__(name, inp, outp, 'Slice', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        initializer = kwargs['initializer']
        extra_arg = {'doc': on.doc_string}
        input_set = list(on.input)
        if len(on.input) == 1:
            starts, ends, axes, steps = None, None, None, None
            for attr in on.attribute:
                if attr.name == 'starts':
                    starts = np.array(list(attr.ints), dtype=np.int64)
                elif attr.name == 'ends':
                    ends = np.array(list(attr.ints), dtype=np.int64)
                elif attr.name == 'axes':
                    axes = np.array(list(attr.ints), dtype=np.int64)
                elif attr.name == 'steps':
                    steps = np.array(list(attr.ints), dtype=np.int64)
            if steps is None:
                steps = np.array([1] * len(starts), dtype=np.int64)
            name = [f'{on.name}_{x}' for x in [
                'starts', 'ends', 'axes', 'steps']]
            input_set += name
            name_val = zip(name, [starts, ends, axes, steps])
            initializer += [Tensor(v, n, 'i64') for n, v in name_val]
        return cls(on.name, input_set, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.Slice(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Softmax(Node):
    def __init__(self, name, inp, outp, *, axis=-1, doc=''):
        super().__init__(name, inp, outp, 'Softmax', doc)
        self.axis = axis

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'axis':
                extra_arg['axis'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.Softmax(*inp,  axis=self.axis)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class LogSoftmax(Node):
    def __init__(self, name, inp, outp, *, axis=-1, doc=''):
        super().__init__(name, inp, outp, 'LogSoftmax', doc)
        self.axis = axis

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'axis':
                extra_arg['axis'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.LogSoftmax(*inp,  axis=self.axis)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Not(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Not", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)


class Sub(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Sub", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Cast(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], *, to, doc: str = ''):
        super().__init__(name, inp, outp, "Cast", doc)
        self.to = to

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'to':
                extra_arg['to'] = attr.i
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        output = onnxscript.opset13.Cast(ns[self.input[0]].data, to=self.to)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class GreaterOrEqual(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "GreaterOrEqual", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Where(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Where", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Expand(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Expand", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        inp[1] = inp[1].astype(np.int64)
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)

class Equal(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Equal", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Range(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Range", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)


class ConstantOfShape(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], *, value, doc: str = ''):
        super().__init__(name, inp, outp, "ConstantOfShape", doc)
        self.value = value

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'value':
                extra_arg['value'] = Tensor.from_onnx(attr.t)
        return cls(on.name, on.input, on.output, **extra_arg)


class Mul(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Mul", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.Mul(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Add(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "Add", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class MatMul(Node):
    def __init__(self, name: str, inp: Sequence[str], outp: Sequence[str], doc: str = ''):
        super().__init__(name, inp, outp, "MatMul", doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.MatMul(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class PRelu(Node):
    def __init__(self, name, inp, outp, doc=''):
        super().__init__(name, inp, outp, 'PRelu', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output, doc=on.doc_string)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.PRelu(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class GRU(Node):
    def __init__(self, name: str, inp: List[str], outp: List[str], *,
                 hidden_size, linear_before_reset,
                 layout = 0,
                 direction = 'forward',
                 clip = None,
                 activation_beta = None,
                 activation_alpha = None,
                 activation = None,
                 doc: str = ''):
        super().__init__(name, inp, outp, "GRU", doc)
        self.hidden_size = hidden_size
        self.linear_before_reset = linear_before_reset
        self.layout = layout
        self.direction = direction
        self.clip = clip
        self.activation_beta = activation_beta
        self.activation_alpha = activation_alpha
        self.activation = activation
        assert(self.clip is None and
               self.activation is None and
               self.activation_beta is None and
               self.activation_alpha is None)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        attr_extractor = {
                'hidden_size': lambda x: x.i,
                'activation_alpha': lambda x: x.floats,
                'activation_beta': lambda x: x.floats,
                'activation': lambda x: x.strings,
                'clip': lambda x: x.f,
                'direction': lambda x: x.s,        
                'layout': lambda x: x.i,
                'linear_before_reset': lambda x: x.i
                }
        for attr in on.attribute:
            if attr.name in attr_extractor:
                extra_arg[attr.name] = attr_extractor[attr.name](attr)
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.GRU(*inp, hidden_size=self.hidden_size, linear_before_reset=self.linear_before_reset, direction=self.direction, clip=self.clip, activation_beta=self.activation_beta, activations=self.activation, activation_alpha=self.activation_alpha)
        for k, v in zip(self.output, output):
            ns[k] = Tensor.from_onnxscript(k, v)


class ConvTranspose(Node):
    def __init__(self, name, inp, outp, *, dilations, group, kernel_shape, strides, pads=None, auto_pad='NOTSET', doc='', 
                 output_padding = None, output_shape = None):
        super().__init__(name, inp, outp, 'ConvTranspose', doc)
        self.dilations = dilations
        self.group = group
        self.kernel_shape = kernel_shape
        self.pads = pads if pads else [0] * len(kernel_shape)
        self.strides = strides
        assert(auto_pad == 'NOTSET')
        assert(output_padding is None)
        assert(output_shape is None)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        attr_extractor = {
                'dilations': lambda x: x.ints,
                'group': lambda x: x.i,
                'kernel_shape': lambda x: x.ints,
                'pads': lambda x: x.ints,
                'strides': lambda x: x.ints,
                'auto_pad': lambda x: x.s,
                'output_shape': lambda x: x.ints,
                'output_padding': lambda x: x.ints,
                }
        for attr in on.attribute:
            if attr.name in attr_extractor:
                extra_arg[attr.name] = attr_extractor[attr.name](attr)

        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, onnxscript.tensor.Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.ConvTranspose(*inp,
                                         dilations=self.dilations,
                                         group=self.group,
                                         kernel_shape=self.kernel_shape,
                                         strides=self.strides,
                                         pads=self.pads)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Max(Node):
    def __init__(self, name, inp, outp, doc = ''):
        super().__init__(name, inp, outp, 'Max', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output, doc=on.doc_string)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Abs(Node):
    def __init__(self, name:str, inp: str, outp: str):
        super().__init__(name, inp, outp, 'Abs')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Neg(Node):
    def __init__(self, name: str, inp: str, outp: str):
        super().__init__(name, inp, outp, 'Neg')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Exp(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Exp')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Log(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Log')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)
        

class GatherElements(Node):
    def __init__(self, name, inp, outp, *, doc='', axis):
        super().__init__(name, inp, outp, 'GatherElements', doc)
        self.axis = axis
    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        for attr in on.attribute:
            if attr.name == 'axis':
                extra_arg['axis'] = attr.i
        #return Node(on.name, on.input, on.output, **extra_arg)
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.GatherElements(*inp,  axis=self.axis)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Tanh(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'Tanh')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class ScatterND(Node):
    def __init__(self, name, inp, outp):
        super().__init__(name, inp, outp, 'ScatterND')

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, Tensor]):
        inp = [ns[x].data for x in self.input]
        output = getattr(onnxscript.opset13, self.op_type)(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class DynamicQuantizeLinear(Node):
    def __init__(self, name, inp, outp, *, doc=''):
        super().__init__(name, inp, outp, 'DynamicQuantizeLinear', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output, doc=on.doc_string)

    def forward(self, ns: dict[str, onnxscript.tensor.Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.DynamicQuantizeLinear(*inp)
        ns |= {k: Tensor.from_onnxscript(k, v) for k, v in zip(self.output, output)}
    

class MatMulInteger(Node):
    def __init__(self, name, inp, outp, *, doc=''):
        super().__init__(name, inp, outp, 'MatMulInteger', doc)

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        return cls(on.name, on.input, on.output)

    def forward(self, ns: dict[str, onnxscript.tensor.Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.MatMulInteger(*inp)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class Gemm(Node):
    def __init__(self, name, inp, outp, *, alpha = 1.0, beta = 1.0, transA = False, transB = False, doc = ''):
        super().__init__(name, inp, outp, 'Gemm', doc)
        self.alpha = alpha
        self.beta = beta
        self.transA = transA
        self.transB = transB

    @classmethod
    def from_onnx(cls, on: onnx.NodeProto, **kwargs):
        extra_arg = {'doc': on.doc_string}
        attr_extractor = {
                'alpha': lambda x: x.f,
                'beta': lambda x: x.f,
                'transA': lambda x: bool(x.i),
                'transB': lambda x: bool(x.i),
                }
        for attr in on.attribute:
            if attr.name in attr_extractor:
                extra_arg[attr.name] = attr_extractor[attr.name](attr)
        return cls(on.name, on.input, on.output, **extra_arg)

    def forward(self, ns: dict[str, onnxscript.tensor.Tensor]):
        inp = [ns[x].data for x in self.input]
        output = onnxscript.opset13.Gemm(*inp,
                                         alpha=self.alpha,
                                         beta=self.beta,
                                         transA=self.transA, transB=self.transB)
        ns[self.output[0]] = Tensor.from_onnxscript(self.output[0], output)


class LSTM(Node):
    def __init__(self, name, inp, outp, *, hidden_size,
                 activation_alpha = None,
                 activation_beta = None,
                 activations = None,
                 clip = None,
                 direction = 'forward',
                 input_forget = 0,
                 layout = 0):
        assert(activations is None)
        assert(clip is None)
        assert(activation_alpha is None)
        assert(activation_beta is None)
        assert(input_forget == 0)
        assert(layout == 0)
        super().__init__(name, inp, outp, 'LSTM', doc='')
        self.hidden_size = hidden_size


class Graph:
    def __init__(self, name: str, inp: List[TensorInfo], outp: List[TensorInfo], initializer: List[Tensor] = []):
        self.name = name
        self.input = {x.name: x for x in inp}
        self.output = {x.name: x for x in outp}
        self.initializer = {x.name: x for x in initializer}
        self.value_info = {}
        self.node = []
        self.value_name = []
        self.sorted = False
        self.dynamic_shape = False

    def __transform(self):
        for name, ti in self.input.items():
            if ti.elem_type in elem_type_transform_map:
                self.input[name].elem_type = elem_type_transform_map[ti.elem_type]
        for name, ti in self.output.items():
            if ti.elem_type in elem_type_transform_map:
                self.output[name].elem_type = elem_type_transform_map[ti.elem_type]
        for name, ti in self.value_info.items():
            if ti.elem_type in elem_type_transform_map:
                self.value_info[name].elem_type = elem_type_transform_map[ti.elem_type]
        for name, tensor in self.initializer.items():
            if tensor.elem_type in elem_type_transform_map:
                out_dtype = elem_type2np[elem_type_transform_map[tensor.elem_type]]
                tensor.elem_type = elem_type_transform_map[tensor.elem_type]
                tensor.data = np.clip(tensor.data, np.iinfo(out_dtype).min, np.iinfo(out_dtype).max).astype(out_dtype)

    def add_node(self, *nodes):
        self.node += nodes

    def get_value_info(self, value_name):
        return self.value_info[self.value_name.index(value_name)]

    def __str__(self):
        return f'{self.name}({",".join(map(str, self.input))}) -> {",".join(map(str, self.output))}'

    @classmethod
    def from_onnx(cls, mdl: Union[str, onnx.ModelProto], dt_transform=True):
        if isinstance(mdl, str):
            mdl = onnx.load_model(mdl)
        opset_version = 0
        for oi in mdl.opset_import:
            if oi.domain in ['', 'ai.onnx']:
                opset_version = oi.version
                break
        assert(opset_version >= 11)
        if opset_version != 13:
            print('opset version mismatch, convert to 13 ...')
            try:
                mdl = onnx.version_converter.convert_version(mdl, 13)
            except Exception as e:
                print(f'version convert failed: {e}, fallback to original opset version: {opset_version}')
        try:
            print(f'try simplify ...')
            model_simp, check = onnxsim.simplify(mdl)
            if check:
                mdl = model_simp
                print('use simplified model')
        except Exception as e:
            print(f'onnxsim failed: {e}, fallback to original model')

        onnx_graph = mdl.graph
        name = onnx_graph.name
        initializer = [Tensor.from_onnx(x) for x in onnx_graph.initializer]
        inp = [ValueInfo.from_onnx(x) for x in onnx_graph.input]
        outp = [ValueInfo.from_onnx(x) for x in onnx_graph.output]
        node = [Node.from_onnx(x, initializer=initializer, opset_version=opset_version)
                for x in onnx_graph.node]
        graph = Graph(name, inp, outp, initializer)
        graph.add_node(*node)
        graph.value_info = {x.name : ValueInfo.from_onnx(
            x) for x in mdl.graph.value_info}
        graph.sorted = True
        for inp_vi in inp:
            if inp_vi.shape is None:
                graph.dynamic_shape = True
                break
        if dt_transform:
            graph.__transform()
        return graph

    def export_onnx(self) -> onnx.ModelProto:
        nodes = [x.export_onnx() for x in self.node]
        inputs = [x.export_onnx() for x in self.input.values()]
        outputs = [x.export_onnx() for x in self.output.values()]
        initializer = [x.export_onnx() for x in self.initializer.values()]
        value_info = [x.export_onnx() for x in self.value_info.values()]
        graph = onnx.helper.make_graph(
                nodes = nodes,
                name = self.name,
                inputs = inputs,
                outputs = outputs,
                initializer = initializer,
                value_info = value_info
                )
        return onnx.helper.make_model(graph)

    def forward(self, input_dict):
        namespace = input_dict | self.initializer
        namespace[''] = Tensor(None, '', '')
        for n in self.node:
            n.forward(namespace)
        return namespace

if __name__ == '__main__':
    import sys
    graph = Graph.from_onnx(sys.argv[1])
    def random_input(name, val):
        data = np.random.rand(*val.shape).astype(elem_type2np[val.elem_type])
        return Tensor(data, name, val.elem_type)
    graph.forward({k: random_input(k, v) for k, v in graph.input.items()})
