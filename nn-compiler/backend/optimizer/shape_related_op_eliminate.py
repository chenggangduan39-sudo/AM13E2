# code eliminate for Reshape, Squeeze, Unsqueeze
from relay.ir import Module


def analysis(mod: Module, mem_ref_chain: dict[str, str]) -> list[bool]:
    candidate_op_type = ('Reshape', 'Squeeze', 'Unsqueeze')
    result = [x.impl.op_type in candidate_op_type and
              x.impl.input[0] not in x.out_set for x in mod.ln.nodes]
    for idx, x in enumerate(mod.ln.nodes):
        if result[idx]:
            mod.ln.extend_lifetime(x.impl.input[0], x.impl.output[0], mem_ref_chain)
    return result
