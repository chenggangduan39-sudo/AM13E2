from relay.graph import Graph, Node
import copy


class TreeNode:
    def __init__(self,
                 name: str,
                 impl_node: Node | None):
        self.deps: list[TreeNode] = []
        self.name = name
        self.impl_node = impl_node
        self.input_degree = 0
        self.parent: list[TreeNode] = []

    def update_dep(self, deps, symbols, graph, processed):
        processed[self.name] = self
        self.input_degree = len(deps)
        for dep in deps:
            if dep not in symbols:
                raise Exception(f'update_dep for {self.name} failed')
            impl_node = symbols[dep]
            if dep in processed:
                tn = processed[dep]
                tn.parent.append(self)
                self.deps.append(tn)
            else:
                tn = TreeNode(dep, impl_node)
                tn.parent.append(self)
                self.deps.append(tn)
                initializer_name = [x.name for x in graph.initializer]
                input_name = [x.name for x in graph.input]
                filter_out = initializer_name + input_name
                tn.update_dep(
                    [x for x in impl_node.input if x not in filter_out],
                    symbols, graph, processed)

    @classmethod
    def construct(cls,
                  symbols: dict,
                  graph: Graph):
        processed: dict[str, TreeNode] = dict()
        root = cls('__exit', None)
        graph_output_names = [x.name for x in graph.output]
        root.update_dep(graph_output_names, symbols, graph, processed)
        return root


class Cfg:
    def __init__(self, graph: Graph):
        symbols = {}
        for n in graph.node:
            for out_sym in n.output:
                symbols[out_sym] = n
        self.tree = TreeNode.construct(symbols, graph)

    def visualization(self, out_dir: str, view: bool = True):
        import graphviz
        dot = graphviz.Digraph('ControlFlowGraph')
        processed = set()

        def _dot(tn):
            if tn.name in processed:
                return
            processed.add(tn.name)
            dot.node(tn.name)
            for dep in tn.deps:
                dot.node(dep.name)
                dot.edge(dep.name, tn.name)
                _dot(dep)
        _dot(self.tree)
        dot.render('out', out_dir, view=view)

    def schedule(self):
        tree = copy.deepcopy(self.tree)
        result = []

        while tree.input_degree > 0:
            this_cycle = []
            processed = set()

            def _find_available(tn):
                processed.add(tn)
                if tn.input_degree > 0:
                    [_find_available(x)
                     for x in tn.deps if x not in processed]
                else:
                    this_cycle.append(tn)

            [_find_available(x) for x in tree.deps]

            def _sequence_growing(start, seq, end_set):
                processed.add(start)
                for p in start.parent:
                    if p in end_set:
                        continue
                    p.input_degree -= 1
                    if p.input_degree == 0:
                        seq.append(p)
                        if p not in processed:
                            _sequence_growing(p, seq, end_set)

            for tn in this_cycle:
                for p in tn.parent:
                    p.input_degree -= 1
            result.append([x.impl_node for x in this_cycle])
        return result


if __name__ == '__main__':
    import sys
    graph = Graph.from_onnx(sys.argv[1])
    cfg = Cfg(graph)
    for idx, x in enumerate(cfg.schedule()):
        print(idx, x)
