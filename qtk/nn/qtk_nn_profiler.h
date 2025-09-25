#ifndef G_O76EJW7X_TCO8_0N35_4KB1_TBIM25RIQBZ1
#define G_O76EJW7X_TCO8_0N35_4KB1_TBIM25RIQBZ1
#pragma once
#include "qtk/core/qtk_type.h"
#include <time.h>
#ifdef __cplusplus
extern "C" {
#endif

struct qtk_nn_vm;

typedef struct qtk_nn_profiler qtk_nn_profiler_t;

struct qtk_nn_profiler {
    struct qtk_nn_vm *vm;
    double *elapsed;
    uint32_t *times;
    double t;
};

int qtk_nn_profiler_init(qtk_nn_profiler_t *p, struct qtk_nn_vm *vm);
void qtk_nn_profiler_clean(qtk_nn_profiler_t *p);
void qtk_nn_profiler_op_start(qtk_nn_profiler_t *p);
void qtk_nn_profiler_op_end(qtk_nn_profiler_t *p, int op);
void qtk_nn_profiler_start(qtk_nn_profiler_t *p);
void qtk_nn_profiler_end(qtk_nn_profiler_t *p);

#ifdef __cplusplus
};
#endif
#endif /* G_O76EJW7X_TCO8_0N35_4KB1_TBIM25RIQBZ1 */
