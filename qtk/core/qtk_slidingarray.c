#include "qtk/core/qtk_slidingarray.h"
#include "wtk/core/wtk_alloc.h"

qtk_slidingarray_t *qtk_slidingarray_new(int win, int step, int elem_sz) {
    qtk_slidingarray_t *a;

    a = wtk_malloc(sizeof(qtk_slidingarray_t));
    a->data = qtk_cyclearray_new(win - step, elem_sz);
    a->step = step;
    a->win = win;
    a->elem_sz = elem_sz;
    a->handler = NULL;
    a->upval = NULL;
    a->idx0 = 0;
    a->idx1 = 0;

    return a;
}

void qtk_slidingarray_set_handler(qtk_slidingarray_t *a,
                                  qtk_slidingarray_handler_t handler,
                                  void *upval) {
    a->upval = upval;
    a->handler = handler;
}

void qtk_slidingarray_delete(qtk_slidingarray_t *a) {
    qtk_cyclearray_delete(a->data);
    wtk_free(a);
}

int qtk_slidingarray_push(qtk_slidingarray_t *a, void *elem) {
    int data_len;
    if (a->idx1 == a->win) {
        a->idx1 = 0;
        a->idx0++;
        data_len = qtk_cyclearray_len(a->data);
        for (int i = 0; i < data_len; i++) {
            a->handler(a->upval, a->idx0, a->idx1++,
                       qtk_cyclearray_at(a->data, i));
        }
        qtk_cyclearray_popn(a->data, min(a->step, data_len));
    }
    a->handler(a->upval, a->idx0, a->idx1, elem);
    if (++a->idx1 >= a->step) {
        qtk_cyclearray_push(a->data, elem, NULL);
    }
    return 0;
}

int qtk_slidingarray_reset(qtk_slidingarray_t *a) {
    a->idx0 = 0;
    a->idx1 = 0;
    qtk_cyclearray_reset(a->data);
    return 0;
}
