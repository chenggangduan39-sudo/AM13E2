from backend.op import Op, Op_type
from collections import namedtuple
import numpy as np
from relay import data_type
from collections import ChainMap
import itertools
from relay.ir import Module
from typing import Mapping

TensorLoc = namedtuple('TensorLoc', ['blockId', 'offset'])


def get_align_sz(sz, n):
    return sz + n - 1 & -n

def block_align_free_sz(offset, sz, n):
    return offset + sz - get_align_sz(offset, n)

class MemBlock:
    def __init__(self, sz, align_sz):
        self.sz = sz
        self.cursor = 0
        self.ref = 0
        self.free_list = [[0, sz]]
        self.align_sz = align_sz

    def __merge_free_list(self):
        if len(self.free_list) == 0:
            return
        self.free_list.sort(key = lambda x: x[0])
        merged_free_list = [self.free_list[0]]
        for offset, sz in self.free_list[1:]:
            if merged_free_list[-1][0] + merged_free_list[-1][1] == offset:
                merged_free_list[-1][1] += sz
            else:
                merged_free_list.append([offset, sz])
        self.free_list = merged_free_list
 
    def free_sz(self):
        if len(self.free_list) == 0:
            return 0
        return np.max([block_align_free_sz(x, y, self.align_sz) for x, y in self.free_list])

    def alloc(self, sz):
        self.free_list.sort(key = lambda x: x[1])
        for idx, (fl_offset, fl_sz) in enumerate(self.free_list):
            if block_align_free_sz(fl_offset, fl_sz, self.align_sz) >= sz:
                fl_idx = idx
                break
        else:
            raise RuntimeError('alloc failed')
        if fl_sz == sz:
            return self.free_list.pop(fl_idx)[0]
        fl_offset, fl_sz = self.free_list[fl_idx]
        align_offset = get_align_sz(fl_offset, self.align_sz)
        self.free_list[fl_idx] = [align_offset + sz, fl_offset + fl_sz - align_offset - sz]
        if align_offset != fl_offset:
            self.free_list.append([fl_offset, align_offset - fl_offset])
        return align_offset

    def free(self, offset, sz):
        self.free_list.append([offset, sz])
        self.__merge_free_list()

    def add_ref(self):
        self.ref += 1

    def __str__(self):
        return f'{self.cursor}@{self.sz} with {self.ref}'

    __repr__ = __str__


class _MB:
    def __init__(self, org_idx: int, block: MemBlock):
        self.org_idx = org_idx
        self.block = block

    def free_sz(self):
        return self.block.free_sz()


class MemoryManager:
    def __init__(self, module: Module, ops: list[Op], eleminate_hint, use_runtime_alloc=False, align_sz:int = 16):
        self.dataspace = module.graph.initializer
        self.namespace = module.graph.input | module.graph.output | module.graph.value_info
        self.ops = ops
        self._hold_tensor: set = set()
        self._runtime_hold_tensor: set = set()
        self._mem_ref: dict[str, str] = {}
        self.mem_blocks: list[MemBlock] = []
        self.mem_loc: dict[str, TensorLoc] = {}
        self.data_blocks: list[bytes] = []
        self.data_mem_loc: Mapping[str, TensorLoc] = {}
        self.graph_output = module.graph.output
        self.pending_alloc = []
        self.theoretical_mem_peak = 0
        self.cur_mem_usage = 0
        self.eleminate_hint = eleminate_hint
        self.use_runtime_alloc = use_runtime_alloc
        self.module = module
        self.mem_runtime = list(self.namespace) if use_runtime_alloc else []
        self.align_sz = align_sz

    def __get_tensor_size(self, name: str) -> int:
        info = self.namespace[name]
        shape = np.array(info.shape)
        return -1 if shape is None else int(data_type.elem_size_map[info.elem_type] * np.prod(shape))

    def __flush_pending(self):
        if self.pending_alloc:
            tensor_sz = [self.__get_tensor_size(x) for x in self.pending_alloc]
            total_sz = tensor_sz[0]
            for tsz in tensor_sz[1:]:
                total_sz = get_align_sz(total_sz, self.align_sz) + tsz
            self.mem_blocks.append(MemBlock(int(total_sz), self.align_sz))
            [self._alloc_tensor(x) for x in self.pending_alloc]
            self.pending_alloc = []

    def _alloc_tensor(self, name: str) -> bool:
        sz = self.__get_tensor_size(name)
        assert(sz > 0)
        if sz < 0:
            return False
        for mb in sorted([_MB(idx, x)
                          for idx, x in enumerate(self.mem_blocks)],
                         key=_MB.free_sz):
            if mb.free_sz() >= sz:
                self.mem_loc[name] = TensorLoc(mb.org_idx, mb.block.alloc(sz))
                self._hold_tensor.add(name)
                self.cur_mem_usage += sz
                self.theoretical_mem_peak = max(self.cur_mem_usage, self.theoretical_mem_peak)
                break
        else:
            self.pending_alloc.append(name)
        return True

    def _free_tensor(self, name: str) -> bool:
        self.__flush_pending()
        if name in self._hold_tensor:
            self._hold_tensor.remove(name)
            blkId, offset = self.mem_loc[name]
            tensor_sz = self.__get_tensor_size(name)
            self.mem_blocks[blkId].free(offset, tensor_sz)
            self.cur_mem_usage -= tensor_sz
            return True
        self._runtime_hold_tensor.remove(name)
        return False

    def analysis_ops(self) -> list[Op]:
        ops = self.ops
        eliminate_hint = []
        mem_ref_op_idx = {}
        for idx, op in enumerate(ops):
            eliminate = False
            match op.typ:
                case Op_type.TensorAlloc:
                    eliminate = self._alloc_tensor(op.name)
                case Op_type.TensorFree:
                    eliminate = self._free_tensor(op.name)
                case Op_type.MemRef:
                    self._mem_ref[op.val] = op.ref
                    mem_ref_op_idx[op.val] = idx
            eliminate_hint.append(eliminate)
        self.__flush_pending()
        for val, ref in self._mem_ref.items():
            if ref in self.mem_loc:
                self.mem_loc[val] = self.mem_loc[ref]
                eliminate_hint[mem_ref_op_idx[val]] = True
        return [x for idx, x in enumerate(self.ops) if not eliminate_hint[idx]]

    def analysis_data(self):
        blocks = []
        dtype_blocks_id = {}
        for name in self.dataspace.keys():
            tensor = self.dataspace[name]
            dtype = tensor.data.dtype
            if dtype in dtype_blocks_id:
                blocks[dtype_blocks_id[dtype]].append((name, tensor.data))
            else:
                dtype_blocks_id[dtype] = len(blocks)
                blocks.append([(name, tensor.data)])

        self.data_blocks = [b''.join([x[1].tobytes() for x in blk])
                            for blk in blocks]

        def _pack_blk_tensor_loc(idx, blk) -> dict[str, TensorLoc]:
            offset = [0] + \
                list(itertools.accumulate([x[1].nbytes for x in blk]))[:-1]
            return {blk[x][0]: TensorLoc(idx, offset[x])
                    for x in range(len(offset))}

        self.data_mem_loc = ChainMap(*[_pack_blk_tensor_loc(idx, blk)
                                       for idx, blk in enumerate(blocks)])
    def analysis(self):
        if not self.use_runtime_alloc:
            self.ops = self.analysis_ops()
        self.analysis_data()

    def __mem_ref_propagate(self, val):
        ref_chain = []
        while val in self._mem_ref:
            ref_chain.append(val)
            val = self._mem_ref[val]
        return val, ref_chain

    def __check_mem_leak(self):
        output_set = {self.__mem_ref_propagate(x)[0] for x in self.graph_output.keys()}
        comptime_leak_set = self._hold_tensor - output_set
        runtime_leak_set = self._runtime_hold_tensor - output_set

        if runtime_leak_set:
            raise Exception(
                f'memory leak: {" ".join(runtime_leak_set)}')

        if comptime_leak_set:
            raise Exception(
                f'memory leak: {" ".join(comptime_leak_set)}')

    def __check_mem_reuse(self):
        input_names = list(self.module.graph.input.keys())
        blocks = [np.zeros(x.sz, dtype=np.uint8) for x in self.mem_blocks]
        for x in self.module.graph.input.keys():
            loc = self.mem_loc[x]
            sz = self.__get_tensor_size(x)
            blocks[loc.blockId][loc.offset:loc.offset + sz] = 0xff

        tensor_holded = set(input_names)

        for idx, n in enumerate(self.module.ln.nodes):
            if self.eleminate_hint[idx]:
                continue
            free_set = tensor_holded - n.in_set
            tensor_holded = tensor_holded - free_set
            tensor_holded |= set(n.impl.output)
            for name in free_set:
                loc = self.mem_loc[name]
                sz = self.__get_tensor_size(name)
                blocks[loc.blockId][loc.offset:loc.offset + sz] = 0
                continue
            for _name in n.impl.output:
                name, _ = self.__mem_ref_propagate(_name)
                loc = self.mem_loc[name]
                sz = self.__get_tensor_size(name)
                if _name not in self._mem_ref:
                    if np.any(blocks[loc.blockId][loc.offset:loc.offset + sz]):
                        raise Exception(f'malform tensor for name: {_name} / {name} node: {n.impl} block: {loc.blockId} offset: {loc.offset}')
                blocks[loc.blockId][loc.offset:loc.offset + sz] = 0xff

        for name in self.module.graph.output.keys():
            loc = self.mem_loc[name]
            sz = self.__get_tensor_size(name)
            if not np.sum(blocks[loc.blockId][loc.offset:loc.offset + sz]) == sz * 255:
                raise Exception(f'malform output: {name} block: {loc.blockId} offset: {loc.offset}')

    # do some check: memory leak
    def __exit__(self, typ, val, traceback):
        if self.use_runtime_alloc:
            return
        self.__check_mem_leak()
        self.__check_mem_reuse()

    def __enter__(self):
        return self
