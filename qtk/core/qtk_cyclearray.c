#include "qtk/core/qtk_cyclearray.h"
#include "qtk/core/qtk_type.h"
#include "wtk/core/wtk_alloc.h"

qtk_cyclearray_t *qtk_cyclearray_new(int cap, int elem_sz) {
    qtk_cyclearray_t *a = wtk_malloc(sizeof(qtk_cyclearray_t) + elem_sz * cap);
    if (a == NULL) {
        return NULL;
    }
    a->data = cast(void *, a + 1);
    a->cap = cap;
    a->r = 0;
    a->len = 0;
    a->elem_sz = elem_sz;
    return a;
}

int qtk_cyclearray_push(qtk_cyclearray_t *a, void *elem, void *poped_elem) {
    int poped = 0;
    if (a->len == a->cap) {
        poped = 1;
        if (poped_elem) {
            memcpy(poped_elem, qtk_cyclearray_at(a, 0), a->elem_sz);
        }
        a->r = (a->r + 1) % a->cap;
        a->len--;
    }
    memcpy(qtk_cyclearray_at(a, a->len++), elem, a->elem_sz);
    return poped;
}

void *qtk_cyclearray_popn(qtk_cyclearray_t *a, int n) {
    void *res;
    if (a->len < n) {
        return NULL;
    }
    res = qtk_cyclearray_at(a, 0);
    a->r = (a->r + n) % a->cap;
    a->len -= n;
    return res;
}

int qtk_cyclearray_reset(qtk_cyclearray_t *a) {
    a->r = 0;
    a->len = 0;
    return 0;
}
