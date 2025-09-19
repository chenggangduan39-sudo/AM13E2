import relay.graph
import itertools
from typing import List, Union
import numpy as np
from enum import IntEnum, auto
import struct
from relay import ir
from backend.op import Op, Op_type, op_type_map
from backend.optimizer import shape_related_op_eliminate
from backend.optimizer import inplace as inplace_checker
from backend.memory_manager import MemoryManager
from collections import Counter
import debug


class TensorType(IntEnum):
    Init = 0
    Dynamic = 1


class ElemType(IntEnum):
    Empty = 0
    F32 = auto()
    Int64 = auto()
    Int32 = auto()
    Bool = auto()
    Uint8 = auto()
    Int8 = auto()

class Direction(IntEnum):
    Forward = 0
    Reverse = auto()
    Bidirectional = auto()

direction_map = {
        'forward': Direction.Forward,
        'reverse': Direction.Reverse,
        'bidirectional': Direction.Bidirectional
        }

class TensorRepr:
    '''
    serialize as 1 uint16: 1bit TensorType + 3bit elem_type + 12bit TensorIndex
    '''
    elem_type_map = {
        'empty': ElemType.Empty,
        'f32': ElemType.F32,
        'i64': ElemType.Int64,
        'i32': ElemType.Int32,
        'bool': ElemType.Bool,
        'u8': ElemType.Uint8,
        'i8': ElemType.Int8
    }

    def __init__(self, ttype: TensorType, index: int, elem_type: str):
        self.ttype = ttype
        self.index = index
        self.elem_type = TensorRepr.elem_type_map[elem_type]

    def dump(self):
        data = (self.ttype << 15) + (self.elem_type << 12) + self.index
        return struct.pack('<H', data)

    def __str__(self):
        return f'({self.ttype}/{self.elem_type})@{self.index}'

    __repr__ = __str__


class RTShape:
    def __init__(self, shape : None | list[int]):
        self.shape = None if shape is None else np.array(shape, dtype=np.uint32)

    def __str__(self):
        return 'None' if self.shape is None else f'({",".join(map(str, self.shape))})'

    def dump(self):
        if self.shape is None:
            raise Exception('dump invalid shape')
        return struct.pack('<B', self.shape.size), self.shape.tobytes()

    def __eq__(self, other):
        if self.shape is None:
            return True if other.shape is None else False
        return np.array_equal(self.shape, other.shape)

    def rank(self):
        return -1 if self.shape is None else self.shape.size

    __repr__ = __str__


class Instruction:
    '''
    | 8bit<op_type> | ~ | 16bit input tensor index |+ ~ | 16bit output tensor index |+ ~ | 16bit extra index |
    op_type will indicate len(inp), len(outp) and extra index
    '''

    def __init__(self, op_type: Op_type, inp: List[TensorRepr],
                 outp: List[TensorRepr], extra_index: Union[int, None] = None):
        self.op_type = op_type
        self.inp = inp
        self.outp = outp
        self.extra_index = extra_index

    def dump(self):
        data = []
        data.append(struct.pack('<B', self.op_type))
        if self.op_type in [Op_type.Concat, Op_type.Split, Op_type.Squeeze, Op_type.Slice]:
            data.append(struct.pack('<B', len(self.inp)))
        data.extend([x.dump() for x in self.inp])
        if self.op_type in [Op_type.Split]:
            data.append(struct.pack('<B', len(self.outp)))
        data.extend([x.dump() for x in self.outp])
        if self.extra_index is not None:
            data.append(struct.pack('<H', self.extra_index))
        return b''.join(data)

    @classmethod
    def nbytes(cls, op_type: Op_type):
        nbytes_dict = {
            Op_type.TensorAlloc: 3,
            Op_type.TensorFree: 3,
            Op_type.MemRef: 5,
        }
        return nbytes_dict[op_type]

    @classmethod
    def from_Op(cls, op: Op, extra_index: int | None,
                symbol_repr: dict[str, TensorRepr]):
        match op.typ:
            case Op_type.TensorAlloc:
                return cls(op.typ, [symbol_repr[op.name]], [], extra_index)
            case Op_type.TensorFree:
                return cls(op.typ, [symbol_repr[op.name]], [], extra_index)
            case Op_type.MemRef:
                return cls(
                    op.typ, [symbol_repr[op.ref]], [symbol_repr[op.val]], extra_index)
            case Op_type.ShapeInfer:
                return cls(op.typ, [], [], extra_index)
            case Op_type.Finish:
                return cls(op.typ, [], [], extra_index)
            case _:
                return cls(op.typ, [symbol_repr[x]
                                    for x in op.node.input],
                           [symbol_repr[x]
                            for x in op.node.output], extra_index)


class ExtraSerializer:
    def Conv(self, compute_node: relay.graph.Conv) -> bytes:
        conv_dim = len(compute_node.dilations)
        fmt = '<B' + conv_dim * 'B' + 'H' + conv_dim * \
            'B' + 2 * conv_dim * 'B' + conv_dim * 'B'
        return struct.pack(fmt, len(compute_node.dilations),
                           *compute_node.dilations,
                           compute_node.group, *compute_node.kernel_shape,
                           *compute_node.pads, *compute_node.strides)

    def ConvInteger(self, compute_node: relay.graph.Conv) -> bytes:
        conv_dim = len(compute_node.dilations)
        fmt = '<B' + conv_dim * 'B' + 'H' + conv_dim * \
            'B' + 2 * conv_dim * 'B' + conv_dim * 'B'
        return struct.pack(fmt, len(compute_node.dilations),
                           *compute_node.dilations,
                           compute_node.group, *compute_node.kernel_shape,
                           *compute_node.pads, *compute_node.strides)

    def MaxPool(self, compute_node: relay.graph.MaxPool) -> bytes:
        assert (len(compute_node.dilations) == 2)

        # dilations + ksize + pad + stride + ceil_mode
        fmt = '<' + 'B' * 2 + 'B' * 2 + 'B' * 4 + 'B' * 2 + 'B'
        return struct.pack(fmt, *compute_node.dilations,
                           *compute_node.kernel_shape,
                           *compute_node.pads,
                           *compute_node.strides,
                           compute_node.ceil_mode)

    def Transpose(self, compute_node: relay.graph.Transpose) -> bytes:
        return struct.pack('<B' + 'B' * len(compute_node.perm),
                           len(compute_node.perm), *compute_node.perm)

    def Concat(self, compute_node: relay.graph.Concat) -> bytes:
        return struct.pack('<b', compute_node.axis)

    def Softmax(self, compute_node: relay.graph.Softmax) -> bytes:
        return struct.pack('<b', compute_node.axis)

    def LogSoftmax(self, compute_node: relay.graph.Softmax) -> bytes:
        return struct.pack('<b', compute_node.axis)

    def Shape(self, compute_node: relay.graph.Shape) -> bytes:
        return struct.pack('<bb', compute_node.start, compute_node.end)

    def Gather(self, compute_node: relay.graph.Gather) -> bytes:
        return struct.pack('<b', compute_node.axis)

    def Cast(self, compute_node: relay.graph.Cast) -> bytes:
        return struct.pack('<B', compute_node.to)

    def Split(self, compute_node: relay.graph.Split) -> bytes:
        return struct.pack('<b', compute_node.axis)
        
    def GatherElements(self, compute_node: relay.graph.GatherElements) -> bytes:
        return struct.pack('<b', compute_node.axis)

    def TopK(self, compute_node: relay.graph.TopK) -> bytes:
        return struct.pack('<bBB', compute_node.axis,
                           compute_node.largest, compute_node.is_sorted)

    def GRU(self, compute_node: relay.graph.GRU) -> bytes:
        return struct.pack('<iBBi', compute_node.hidden_size, compute_node.layout, direction_map[compute_node.direction], compute_node.linear_before_reset)

    def ConvTranspose(self, compute_node: relay.graph.ConvTranspose) -> bytes:
        conv_dim = len(compute_node.dilations)
        fmt = '<B' + conv_dim * 'B' + 'H' + conv_dim * \
            'B' + 2 * conv_dim * 'B' + conv_dim * 'B'
        return struct.pack(fmt, len(compute_node.dilations),
                           *compute_node.dilations,
                           compute_node.group, *compute_node.kernel_shape,
                           *compute_node.pads, *compute_node.strides)

    def Gemm(self, compute_node: relay.graph.Gemm) -> bytes:
        return struct.pack('<BBff', compute_node.transA, compute_node.transB, compute_node.alpha, compute_node.beta)

    def BatchNormalization(self, compute_node: relay.graph.BatchNormalization) -> bytes:
        return struct.pack('f', compute_node.epsilon)


class VM:
    def __init__(self, module: ir.Module, use_runtime_alloc: bool = False, align_sz: int = 16):
        self.mem_ref_chain: dict[str, str] = {}
        ops = []
        inplace_hint = inplace_checker.analysis(module, self.mem_ref_chain)
        eliminate_hint = shape_related_op_eliminate.analysis(module, self.mem_ref_chain)
        input_names = list(module.graph.input.keys())
        ops += [Op(Op_type.TensorAlloc, {'name': x}) for x in input_names]
        tensor_holded = set(input_names)
        self.namespace = module.graph.input | module.graph.output | module.graph.value_info
        self.align_sz = align_sz
        self.mod = module
        for idx, n in enumerate(module.ln.nodes):
            free_set = tensor_holded - n.in_set
            ops += [Op(Op_type.TensorFree, {'name': x}) for x in free_set]
            tensor_holded = tensor_holded - free_set
            inplace = inplace_hint[idx]
            eliminate = eliminate_hint[idx]
            need_shape_infer = False
            for x in n.impl.output:
                if x not in self.namespace:
                    continue
                shape = self.namespace[x].shape
                if shape is None:
                    need_shape_infer = True
            if need_shape_infer:
                ops.append(Op(Op_type.ShapeInfer,
                              {'op': Op(op_type_map[n.impl.op_type],
                                        {'node': n.impl})}))
            if eliminate:
                ops.append(Op(Op_type.MemRef,
                              {'val': n.impl.output[0],
                               'ref': n.impl.input[0]}))
                continue
            if inplace:
                ops.append(Op(Op_type.MemRef,
                              {'val': n.impl.output[0],
                               'ref': n.impl.input[0]}))
            else:
                ops += [Op(Op_type.TensorAlloc,
                           {'name': x}) for x in n.impl.output]
                tensor_holded |= set(n.impl.output)
            ops.append(Op(op_type_map[n.impl.op_type], {'node': n.impl}))
        ops += [Op(Op_type.TensorFree, {'name': x}) for x in tensor_holded - module.ln.nodes[-1].out_set]
        self.ops = ops
        self.__prune_initializer()
        with MemoryManager(module, ops, eliminate_hint, use_runtime_alloc, self.align_sz) as mem_manager:
            mem_manager.analysis()
            self.ops = mem_manager.ops
            self.dynamic_comptime_symbols = list(mem_manager.mem_loc.keys())
            self.dynamic_runtime_symbols = mem_manager.mem_runtime
            self.data_blocks = mem_manager.data_blocks
            self.data_mem_loc = mem_manager.data_mem_loc
            self.mem_blocks = mem_manager.mem_blocks
            self.mem_loc = mem_manager.mem_loc
            self.theoretical_mem_peak = mem_manager.theoretical_mem_peak
        self.dynamic_symbols = self.dynamic_comptime_symbols + self.dynamic_runtime_symbols # Note: dynamic_comptime_symbols must appear first

    def __prune_initializer(self):
        used_input_symbols = set()
        for op in self.ops:
            if hasattr(op, 'node'):
                used_input_symbols |= set(op.node.input)
        orphan_symbols = set(module.graph.initializer.keys()) - used_input_symbols
        for sym in orphan_symbols:
            del module.graph.initializer[sym]
        self.initializer_symbols = list(module.graph.initializer.keys())

    def _pack_extra(self, symbol_repr):
        extra_serializer = ExtraSerializer()
        res = [getattr(extra_serializer, op.typ.name)(op.node)
               if hasattr(extra_serializer, op.typ.name) else b''
               for op in self.ops]
        for idx, op in enumerate(self.ops):
            if op.typ != Op_type.ShapeInfer:
                continue
            ref_instruction = Instruction.from_Op(op, None, symbol_repr)
            ref_extra = \
                getattr(extra_serializer, op.typ.name)(op.op.node) \
                if hasattr(extra_serializer, op.op.typ.name) else b''
            res[idx] = ref_instruction.dump() + ref_extra
        return res

    def _serialize_data(self, sections):
        shape = [
            RTShape(module.graph.initializer[x].data.shape)
            for x in self.initializer_symbols]
        blocks = self.data_blocks
        loc = [self.data_mem_loc[x] for x in self.initializer_symbols]

        sections.append(struct.pack('<I', len(blocks)))
        for ib in blocks:
            sections.append(struct.pack('<I', len(ib)))
            sections.append(ib)
        sections.append(struct.pack('<I', len(shape)))

        rank_bins, shape_bins = zip(*[x.dump() for x in shape])
        rank = [x.rank() for x in shape]
        sections.append(struct.pack('<H', sum(rank)))
        offset = [0] + list(itertools.accumulate(rank))[:-1]
        offset_bin = struct.pack('<' + 'H' * len(offset), *offset)
        sections.extend(itertools.chain(rank_bins, shape_bins))
        sections.append(offset_bin)
        sections.extend([struct.pack('<II', x.blockId, x.offset) for x in loc])

    def _serialize_comptime_dynamic(self, sections):
        shape = [RTShape(self.namespace[x].shape) for x in self.dynamic_symbols]
        blocks = self.mem_blocks
        loc = [self.mem_loc[x] for x in self.dynamic_comptime_symbols]

        sections.append(struct.pack('<I', len(blocks)))
        for db in blocks:
            sections.append(struct.pack('<I', db.sz))
        sections.append(struct.pack('<I', len(shape)))
        sections.append(struct.pack('<I', len(loc)))

        rank_bins, shape_bins = zip(*[x.dump() for x in shape])
        rank = [x.rank() for x in shape]
        sections.append(struct.pack('<H', sum(rank)))
        offset = [0] + list(itertools.accumulate(rank))[:-1]
        offset_bin = struct.pack('<' + 'H' * len(offset), *offset)
        sections.extend(itertools.chain(rank_bins, shape_bins))
        sections.append(offset_bin)

        sections.extend([struct.pack('<II', x.blockId, x.offset) for x in loc])

    def _serialize_dynamic(self, sections):
        sections.append(struct.pack('<I', len(self.dynamic_symbols)))
        self._serialize_comptime_dynamic(sections)

    def _serialize_io_desc(self, sections, symbol_repr):
        input_repr = [symbol_repr[x] for x in self.mod.graph.input.keys()]
        output_repr = [symbol_repr[x] for x in self.mod.graph.output.keys()]

        sections.append(struct.pack('<I', len(input_repr)))
        sections.append(b''.join([x.dump() for x in input_repr]))
        sections.append(struct.pack('<I', len(output_repr)))
        sections.append(b''.join([x.dump() for x in output_repr]))

    def _serialize_instructions(self, sections, symbol_repr):
        extra_index = [0] + list(itertools.accumulate([
            len(x) for x in self.extra_list]))
        instructions = [Instruction.from_Op(op, None if
                                            len(self.extra_list[idx]) == 0
                                            else extra_index[idx], symbol_repr)
                        for idx, op in enumerate(self.ops)]
        instructions.append(Instruction(Op_type.Finish, [], [], None))
        data = b''.join([x.dump() for x in instructions])
        sections.extend([struct.pack('<I', len(data)), data])

    def _serialize_extra(self, sections):
        extra = b''.join(self.extra_list)
        sections.extend([struct.pack('<I', len(extra)), extra])

    def _serialize_symbols(self, sections):
        sections.append(b''.join([struct.pack(f'<H{len(x)}s', len(
            x), x.encode()) for x in self.initializer_symbols]))
        sections.append(b''.join([struct.pack(f'<H{len(x)}s', len(
            x), x.encode()) for x in self.dynamic_symbols]))

    def serialize(self, debug_info: bool = False) -> bytes:
        dynamic_elem_types = [self.namespace[x].elem_type
                              for x in self.dynamic_symbols]
        initializer_elem_types = [self.mod.graph.initializer[x].elem_type
                                  for x in self.initializer_symbols]
        symbol_repr = {
            x: TensorRepr(TensorType.Init, idx, initializer_elem_types[idx])
            for idx, x in enumerate(self.initializer_symbols)}
        symbol_repr.update({
            x: TensorRepr(TensorType.Dynamic, idx, dynamic_elem_types[idx])
            for idx, x in enumerate(self.dynamic_symbols)})
        symbol_repr[''] = TensorRepr(TensorType.Init, 0, 'empty')

        sections: list[bytes] = []
        self.extra_list = self._pack_extra(symbol_repr)
        self._serialize_io_desc(sections, symbol_repr)
        self._serialize_data(sections)
        self._serialize_dynamic(sections)
        self._serialize_instructions(sections, symbol_repr)
        self._serialize_extra(sections)

        if debug_info:
            self._serialize_symbols(sections)

        return b''.join(sections)

    def summary(self):
        print('dynamice_symbols: '
              f'runtime {len(self.dynamic_runtime_symbols)}'
              f' + comptime {len(self.dynamic_comptime_symbols)}')
        print(f'initializer symbols: {len(self.initializer_symbols)}')
        parameter_sz = debug.size2str(sum([len(x) for x in self.data_blocks]))
        tensor_sz = debug.size2str(sum([x.sz for x in self.mem_blocks]))
        print(f'parameter: {parameter_sz}')
        print(f'tensor: {tensor_sz} / {debug.size2str(self.theoretical_mem_peak)}')
        print(f'nops: {len(self.ops)}')
        counter = Counter([x.typ.name for x in self.ops])
        for k in counter:
            print(f'{k}: {counter[k]}')


if __name__ == '__main__':
    from relay.graph import Graph, Tensor
    from relay.data_type import elem_type2np
    import argparse

    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('src', type=str)
    arg_parser.add_argument('dst', type=str)
    arg_parser.add_argument('--debug_info', action='store_true')
    arg_parser.add_argument('--use_runtime_alloc', action='store_true')
    arg_parser.add_argument('--disable_res_verify', action='store_true')
    arg_parser.add_argument('--verify_tolerance', type=float, default=1e-5)
    arg_parser.add_argument('--align_sz', type=int, default=16)
    args = arg_parser.parse_args()

    graph = Graph.from_onnx(args.src)
    module = ir.Module(graph, optimize=True)
    vm = VM(module, args.use_runtime_alloc, args.align_sz)
    vm.summary()
    bin = vm.serialize(args.debug_info)

    if graph.dynamic_shape and not args.disable_res_verify:
        print('skip verify for dynamic shape')
        args.disable_res_verify = True

    def random_input(name, val):
        data = np.random.rand(*val.shape).astype(elem_type2np[val.elem_type])
        return Tensor(data, name, val.elem_type)

    if not args.disable_res_verify:
        import pyqtk
        from scipy.spatial.distance import cosine
        orignal_graph = Graph.from_onnx(args.src, dt_transform=False)
        input_dict = {k: random_input(k, v) for k, v in orignal_graph.input.items()}
        ns = orignal_graph.forward(input_dict)
        qnn = pyqtk.QNN()
        qnn.load_bytes(bin)
        input_vec = [input_dict[k].data for k in orignal_graph.input.keys()]
        result = qnn.invoke(input_vec)
        for idx, x in enumerate(orignal_graph.output.keys()):
            a = result[idx]
            b = ns[x].data
            dis = cosine(a.flatten(), b.flatten())
            if dis > args.verify_tolerance:
                raise Exception(f'output verify failed cosine distance too large: {dis}')

    with open(args.dst, 'wb') as f:
        f.write(bin)
