import relay
import os


class LivenessNode:
    def __init__(self, impl: relay.graph.Node, initlizer_name):
        self.in_set = set()
        self.out_set = set()
        self.use_set = {x for x in impl.input if x not in initlizer_name}
        self.def_set = {x for x in impl.output}
        self.impl = impl

    @classmethod
    def nop(cls, use_set: set[str], def_set: set[str]):
        node = cls.__new__(LivenessNode)
        setattr(node, 'use_set', use_set)
        setattr(node, 'def_set', def_set)
        setattr(node, 'in_set', set())
        setattr(node, 'out_set', set())
        return node

    def __str__(self):
        return f"{self.impl}\n\t{{ {','.join(self.in_set)} }} In" + \
            f" <=> {{ {','.join(self.out_set)} }} Out"

    __repr__ = __str__


class Liveness:
    def __init__(self, graph: relay.graph.Graph):
        initlizer_name = list(graph.initializer.keys())
        nodes = [LivenessNode(x, initlizer_name) for x in graph.node]
        nodes.append(LivenessNode.nop(set(graph.output.keys()), set()))
        liveness_quit = False
        [setattr(x, 'succ', nodes[idx+1]) for idx, x in enumerate(nodes[:-1])]
        while not liveness_quit:
            for n in nodes:
                n.in_set_ = n.in_set.copy()
                n.out_set_ = n.out_set.copy()
                n.in_set = n.use_set | (n.out_set - n.def_set)
                if hasattr(n, 'succ'):
                    n.out_set = n.succ.in_set
            liveness_quit = True
            for n in nodes:
                if n.in_set != n.in_set_ or n.out_set != n.out_set_:
                    liveness_quit = False
                    break
        for n in nodes:
            delattr(n, 'in_set_')
            delattr(n, 'out_set_')
        nodes = nodes[:-1]
        self.nodes = nodes

    def extend_lifetime(self, ref: str, var: str, extend_chain: dict[str, str]):
        extend_chain[var] = ref
        while var in extend_chain:
            ref = extend_chain[var]
            for ln in self.nodes:
                if var in ln.in_set:
                    ln.in_set.add(ref)
                if var in ln.out_set:
                    ln.out_set.add(ref)
            var = ref

    def __str__(self):
        return os.linesep.join(map(str, self.nodes))
