#ifndef G_USHPNPOL_U9MS_X2CP_ZW3K_M3KZ0W2A0R51
#define G_USHPNPOL_U9MS_X2CP_ZW3K_M3KZ0W2A0R51
#pragma once
#include "qtk/core/qtk_cyclearray.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_slidingarray qtk_slidingarray_t;
typedef int (*qtk_slidingarray_handler_t)(void *upval, int idx0, int idx1,
                                          void *elem);

struct qtk_slidingarray {
    int win;
    int step;
    int elem_sz;
    qtk_slidingarray_handler_t handler;
    void *upval;
    int idx0;
    int idx1;
    qtk_cyclearray_t *data;
};

qtk_slidingarray_t *qtk_slidingarray_new(int win, int step, int elem_sz);
void qtk_slidingarray_set_handler(qtk_slidingarray_t *a,
                                  qtk_slidingarray_handler_t handler,
                                  void *upval);
void qtk_slidingarray_delete(qtk_slidingarray_t *a);
int qtk_slidingarray_push(qtk_slidingarray_t *a, void *elem);
int qtk_slidingarray_reset(qtk_slidingarray_t *a);

#ifdef __cplusplus
};
#endif
#endif /* G_USHPNPOL_U9MS_X2CP_ZW3K_M3KZ0W2A0R51 */
