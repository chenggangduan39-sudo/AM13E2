from relay.liveness import Liveness


def analysis(ln: Liveness) -> list[bool]:
    result = []
    inplace_ops = ('Relu', 'Softmax', 'LogSoftmax', 'Not', 'Sqrt')

    def can_inplace_(idx, x):
        if x.impl.op_type in inplace_ops and\
                x.impl.input[0] not in ln.nodes[idx].out_set:
            return True
        return False
    result = [can_inplace_(idx, x) for idx, x in enumerate(ln.nodes)]

    for idx, x in enumerate(ln.nodes):
        if result[idx]:
            ln.extend_lifetime(idx, x.impl.input[0], x.impl.output[0])

    return result
