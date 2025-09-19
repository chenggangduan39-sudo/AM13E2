from relay.ir import Module
from relay.data_type import elem_size_map

# only support node.output[0] reuse node.input[0] memory for now
class InplaceChecker:
    def __init__(self, mod: Module):
        self.mod = mod

    def inplace_checker_simple(self, idx) -> bool:
        node = self.mod.ln.nodes[idx]
        return node.impl.op_type in (
                'Relu',
                'Softmax',
                'LogSoftmax',
                'Not',
                'Sqrt',
                'PRelu',
                'Abs',
                'Neg',
                'Exp',
                'ScatterND',
                'Log')

    def inplace_checker_binary_op(self, idx) -> bool:
        node = self.mod.ln.nodes[idx]
        if node.impl.op_type not in (
                'Add',
                'Mul',
                'Sub',
                'Div'):
            return False
        A = node.impl.input[0]
        B = node.impl.input[1]
        C = node.impl.output[0]
        if A not in self.mod.graph.value_info or C not in self.mod.graph.value_info:
            return False
        shapeA = self.mod.graph.value_info[A].shape
        shapeB = self.mod.graph.value_info[B].shape if B in self.mod.graph.value_info else list(self.mod.graph.initializer[B].data.shape)
        shapeC = self.mod.graph.value_info[C].shape
        if shapeA != shapeC:
            return False
        if shapeB and shapeA != shapeB and shapeB != [1]:
            return False
        return True
        
    def inplace_checker_misc(self, idx) -> bool:
        node = self.mod.ln.nodes[idx]
        if node.impl.op_type != 'Cast':
            return False
        A = node.impl.input[0]
        B = node.impl.output[0]
        if A not in self.mod.graph.value_info or B not in self.mod.graph.value_info:
            return False
        elem_typeA, elem_typeB = (self.mod.graph.value_info[x].elem_type for x in (A, B))
        return elem_size_map[elem_typeA] == elem_size_map[elem_typeB]

    def __call__(self, idx: int, mem_ref_chain: dict[str, str]) -> bool:
        node = self.mod.ln.nodes[idx]
        if node.impl.input[0] in self.mod.ln.nodes[idx].out_set:
            return False
        checkers = [getattr(self, x) for x in InplaceChecker.__dict__ if x.startswith('inplace_checker')]
        for checker in checkers:
            if checker(idx):
                break
        else:
            return False
        in_operand, out_operand = node.impl.input[0], node.impl.output[0]
        self.mod.ln.extend_lifetime(in_operand, out_operand, mem_ref_chain)
        return True


def analysis(mod: Module, mem_ref_chain: dict[str, str]) -> list[bool]:
    checker = InplaceChecker(mod)
    return [checker(idx, mem_ref_chain) for idx, _ in enumerate(mod.ln.nodes)]
