from relay.graph import Graph, Gemm, Tensor, Neg, Abs, Reshape, Max, Relu
import relay.liveness
from numpy.lib.stride_tricks import sliding_window_view
import numpy as np
from collections import namedtuple
import itertools
from dataclasses import dataclass
import weakref


ValDeps = namedtuple('ValDeps', ['producer', 'consumer'])

class Module:
    def __init__(self, graph: Graph, optimize=True):
        self.graph = graph
        self.optimizer = []
        self.node_dict = weakref.WeakValueDictionary({x.name : x for x in self.graph.node})
        self.optimizer = [
            #FuseMatMulAdd(),
            FuseConvTransposeBatchNormalization(),
            RewriteSub(),
            FuseNegAbs(),
            EliminateSlice(),
            RewirteMax(),
            RewriteTranspose(),
        ]
        if optimize:
            while self.__optimize():
                pass
            self.__cleanup_dataspace()
        self.ln = relay.liveness.Liveness(graph)

    def get_val_deps(self, name):
        producer = []
        consumer = []
        for node in self.graph.node:
            if name in node.input:
                consumer.append(node.name)
            elif name in node.output:
                producer.append(node.name)
        return ValDeps(producer, consumer)

    def __str__(self):
        return f'{self.ln}'

    def __optimize(self):
        hint = [x(self) for x in self.optimizer]
        return np.any(hint)

    def __cleanup_dataspace(self):
        initializer_symbols = set(self.graph.initializer.keys())
        used_symbols = set(itertools.chain(*[x.input for x in self.graph.node]))
        for x in initializer_symbols - used_symbols:
            del self.graph.initializer[x]


@dataclass
class PatternMatchResult:
    start_index: int
    matched_nodes: list


class SeqPattern:
    def __init__(self, seq: list[str]):
        self.seq = seq

    def match(self, graph: Graph, from_index: int = 0) -> PatternMatchResult | None:
        node_types = np.array([x.op_type for x in graph.node[from_index:]])
        if len(node_types) < len(self.seq):
            return None
        node_types_view = sliding_window_view(node_types, len(self.seq))
        for idx, sub in enumerate(node_types_view):
            idx += from_index
            if np.all(sub == self.seq):
                return PatternMatchResult(idx, graph.node[idx: idx + len(self.seq)])


class Optimizer:
    def __init__(self, pattern):
        self.pattern = pattern
        self.optimize_method = getattr(self, 'optimize')

    def __call__(self, module: Module):
        match_start = 0
        optimized = False
        while matched := self.pattern.match(module.graph, match_start):
            optimized, match_start = self.optimize_method(module, matched)
        return optimized


class FuseConvTransposeBatchNormalization(Optimizer):
    def __init__(self):
        super().__init__(SeqPattern(['ConvTranspose', 'BatchNormalization']))

    def optimize(self, module: Module, matched: PatternMatchResult) -> tuple[bool, int]:
        idx = matched.start_index
        conv_tanspose_node, bn_node = matched.matched_nodes
        prev_out = conv_tanspose_node.output[0]
        prev_out_consumer = module.get_val_deps(prev_out).consumer
        if len(prev_out_consumer) != 1 or prev_out_consumer[0] != bn_node.name:
            return False, idx + 2
        if conv_tanspose_node.group != 1:
            return False, idx + 2
        cvt_w_name, cvt_b_name = conv_tanspose_node.input[1:]
        bn_scale_name, bn_B_name, bn_mean_name, bn_var_name = bn_node.input[1:]
        if set((cvt_w_name, cvt_b_name, bn_scale_name, bn_B_name, bn_mean_name, bn_var_name)) - set(module.graph.initializer.keys()):
            return False, idx + 2
        cvt_w = module.graph.initializer[cvt_w_name].data
        cvt_b = module.graph.initializer[cvt_b_name].data
        cvt_w_type_str = module.graph.initializer[cvt_w_name].elem_type
        cvt_b_type_str = module.graph.initializer[cvt_b_name].elem_type
        cvt_w_type = module.graph.initializer[cvt_w_name].data.dtype
        cvt_b_type = module.graph.initializer[cvt_b_name].data.dtype
        bn_scale = module.graph.initializer[bn_scale_name].data
        bn_B = module.graph.initializer[bn_B_name].data
        bn_mean = module.graph.initializer[bn_mean_name].data
        bn_var = module.graph.initializer[bn_var_name].data
        bn_eps = bn_node.epsilon

        bn_std_scale = 1 / np.sqrt(bn_var + bn_eps)
        C = len(cvt_b)
        new_cvt_w = cvt_w.copy()
        for c in range(C):
            new_cvt_w[:,c,:,:] *= bn_std_scale[c] * bn_scale[c]
        new_cvt_b = (cvt_b - bn_mean) * bn_std_scale * bn_scale + bn_B

        new_cvt_w_name = f'fused_{cvt_w_name}_{bn_node.name}_w'
        new_cvt_b_name = f'fused_{cvt_b_name}_{bn_node.name}_w'
        module.graph.initializer[new_cvt_w_name] = Tensor(new_cvt_w.astype(cvt_w_type), new_cvt_w_name, cvt_w_type_str)
        module.graph.initializer[new_cvt_b_name] = Tensor(new_cvt_b.astype(cvt_b_type), new_cvt_b_name, cvt_b_type_str)
        conv_tanspose_node.input[1] = new_cvt_w_name
        conv_tanspose_node.input[2] = new_cvt_b_name
        conv_tanspose_node.output = bn_node.output

        del module.graph.value_info[prev_out]

        module.graph.node.pop(idx + 1)
        return True, idx + 1


class FuseMatMulAdd(Optimizer):
    def __init__(self):
        super().__init__(SeqPattern(['MatMul', 'Add']))

    def optimize(self, module: Module, matched: PatternMatchResult) -> tuple[bool, int]:
        idx = matched.start_index
        matmul_node, add_node = matched.matched_nodes
        prev_out = matmul_node.output[0]
        prev_out_consumer = module.get_val_deps(prev_out).consumer
        if len(prev_out_consumer) != 1 or prev_out_consumer[0] != add_node.name:
            return False, idx + 2
        bias, = set(add_node.input) - {prev_out}
        gemm_node = Gemm(f'fused_{matmul_node.name}_{add_node.name}', [matmul_node.input[0], matmul_node.input[1], bias], add_node.output)
        del module.graph.value_info[prev_out]
        module.node_dict[gemm_node.name] = gemm_node
        module.graph.node[idx] = gemm_node
        module.graph.node.pop(idx + 1)
        return True, idx + 1


class RewriteSub(Optimizer):
    def __init__(self):
        super().__init__(SeqPattern(['Sub']))

    def optimize(self, module: Module, matched: PatternMatchResult) -> tuple[bool, int]:
        idx = matched.start_index
        sub_node = module.graph.node[idx]
        A_name, B_name = sub_node.input
        y = sub_node.output[0]
        if A_name in module.graph.initializer and \
                np.allclose(module.graph.initializer[A_name].data, 0): # Sub(0, B) -> y ==> Neg(B) -> y
            neg_node = Neg(f'rewrite_neg_{sub_node.name}', B_name, y)
            module.node_dict[neg_node.name] = neg_node
            module.graph.node[idx] = neg_node
            return True, idx + 1
        return False, idx + 1


class RewirteMax(Optimizer):
    def __init__(self):
        super().__init__(SeqPattern(['Max']))

    def optimize(self, module: Module, matched: PatternMatchResult) -> tuple[bool, int]:
        idx = matched.start_index
        max_node = module.graph.node[idx]
        if len(max_node.input) != 2:
            return False, idx + 1
        x, y = max_node.input
        relu_input = x
        if x in module.graph.initializer:
            assert(y not in module.graph.initializer)
            if not np.allclose(module.graph.initializer[x].data, 0):
                return False, idx + 1
            relu_input = y
        if y in module.graph.initializer:
            assert(x not in module.graph.initializer)
            if not np.allclose(module.graph.initializer[y].data, 0):
                return False, idx + 1
            relu_input = x
        relu_node = Relu(f'rewrite_relu_{max_node.name}', [relu_input], max_node.output)
        module.node_dict[relu_node.name] = relu_node
        module.graph.node[idx] = relu_node;
        return True, idx + 1


class RewriteTranspose(Optimizer):
    def __init__(self):
        super().__init__(SeqPattern(['Transpose']))

    def optimize(self, module: Module, matched: PatternMatchResult) -> tuple[bool, int]:
        idx = matched.start_index
        transpose_node = module.graph.node[idx]
        input_shape = module.graph.value_info[transpose_node.input[0]].shape
        last_permuted_axis = 0
        for perm in transpose_node.perm:
            if input_shape[perm] == 1:
                continue
            if perm < last_permuted_axis:
                return False, idx + 1
            last_permuted_axis = perm
        output_shape = module.graph.value_info[transpose_node.output[0]].shape
        reshape_node_name = f'rewrite_reshape_{transpose_node.name}'
        shape_name = f'{reshape_node_name}_shape'
        module.graph.initializer[shape_name] = Tensor(np.array([output_shape], dtype=np.int32), shape_name, 'i32')
        reshape_node = Reshape(reshape_node_name, [transpose_node.input[0], shape_name], transpose_node.output)
        module.node_dict[reshape_node_name] = reshape_node
        module.graph.node[idx] = reshape_node
        return False, idx + 1


class FuseNegAbs(Optimizer):
    def __init__(self):
        super().__init__(SeqPattern(['Neg', 'Abs']))

    '''
    neg(x) -> y + abs(y) -> z ==> abs(x) -> z
    '''
    def optimize(self, module: Module, matched: PatternMatchResult) -> tuple[bool, int]:
        idx = matched.start_index
        neg_node, abs_node = matched.matched_nodes
        x = neg_node.input[0]
        y = neg_node.output[0]
        z = abs_node.output[0]
        y_consumer = module.get_val_deps(y).consumer
        if len(y_consumer) != 1 or y_consumer[0] != abs_node.name:
            return False, idx + 2
        new_abs = Abs(f'fused_{neg_node.name}_{abs_node.name}', x, z)
        module.graph.node[idx] = new_abs
        module.graph.node.pop(idx + 1)
        module.node_dict[new_abs.name] = new_abs
        del module.graph.value_info[y]
        return True, idx + 1


class EliminateSlice(Optimizer):
    def __init__(self):
        super().__init__(SeqPattern(['Slice'])) 

    def optimize(self, module: Module, matched: PatternMatchResult) -> tuple[bool, int]:
        idx = matched.start_index
        slice_node = module.graph.node[idx]
        x = slice_node.input[0]
        y = slice_node.output[0]
        if x not in module.graph.value_info or y not in module.graph.value_info:
            return False, idx + 1
        shapex = module.graph.value_info[x]
        shapey = module.graph.value_info[y]
        if None in shapex.shape or None in shapey.shape:
            return False, idx + 1
        if shapex.shape == shapey.shape:
            x_consumers = module.get_val_deps(x).consumer
            for x_consumer in x_consumers:
                if x_consumer == slice_node.name:
                    continue
                consumer_node = module.node_dict[x_consumer]
                for i, cinp in enumerate(consumer_node.input):
                    if cinp == x:
                        consumer_node.input[i] = y
                        break
                else:
                    assert False
            x_producer, = module.get_val_deps(x).producer
            x_producer_node = module.node_dict[x_producer]
            for i, x_producer_out in enumerate(x_producer_node.output):
                if x == x_producer_out:
                    x_producer_node.output[i] = y
            module.graph.node.pop(idx)
            del module.graph.value_info[x]
            return True, idx
        return False, idx + 1


if __name__ == '__main__':
    import sys
    import onnx
    module = Module(Graph.from_onnx(sys.argv[1]))
    onnx.save_model(module.graph.export_onnx(), 'g.onnx')
