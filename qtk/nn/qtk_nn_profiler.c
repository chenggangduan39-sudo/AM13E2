#include "qtk_nn_profiler.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/nn/qtk_nn_op.h"
#include "qtk/nn/vm/qtk_nn_vm.h"
#include "qtk/os/qtk_tick.h"

#define PROFILER_TAG "[Profiler]"

int qtk_nn_profiler_init(qtk_nn_profiler_t *p, qtk_nn_vm_t *vm) {
    p->vm = vm;
    p->elapsed = wtk_malloc(sizeof(p->elapsed[0]) * QBL_NN_VM_OP_Finish);
    p->times = wtk_malloc(sizeof(p->times[0]) * QBL_NN_VM_OP_Finish);
    return 0;
}

void qtk_nn_profiler_clean(qtk_nn_profiler_t *p) {
    wtk_free(p->elapsed);
    wtk_free(p->times);
}

void qtk_nn_profiler_op_start(qtk_nn_profiler_t *p) {
    p->t = qtk_get_tick_ms_d();
}

void qtk_nn_profiler_op_end(qtk_nn_profiler_t *p, int op) {
    qtk_assert(op < QBL_NN_VM_OP_Finish);
    double t = qtk_get_tick_ms_d();
    p->elapsed[op] += t - p->t;
    p->times[op] += 1;
}

void qtk_nn_profiler_start(qtk_nn_profiler_t *p) {
    memset(p->elapsed, 0, sizeof(p->elapsed[0]) * QBL_NN_VM_OP_Finish);
    memset(p->times, 0, sizeof(p->times[0]) * QBL_NN_VM_OP_Finish);
}

void qtk_nn_profiler_end(qtk_nn_profiler_t *p) {
    double sum = 0;
    for (int i = 0; i < QBL_NN_VM_OP_Finish; i++) {
        if (p->elapsed[i] == 0) {
            continue;
        }
        printf(PROFILER_TAG " %s<%d>: %lf\n", qtk_nn_vm_get_opname(i),
               p->times[i], p->elapsed[i]);
        sum += p->elapsed[i];
    }
    qtk_debug(">>>>> Tot: %lf\n", sum);
}
