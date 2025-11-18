#ifndef __QBL_CORE_QBL_CYCLEARRAY_H__
#define __QBL_CORE_QBL_CYCLEARRAY_H__
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_cyclearray qtk_cyclearray_t;

struct qtk_cyclearray {
    int cap;
    int r;
    int len;
    int elem_sz;
    void *data;
};

#define qtk_cyclearray_len(a) ((a)->len)
#define qtk_cyclearray_at(a, i)                                                \
    cast(void *,                                                               \
         cast(char *, (a)->data) + (((a)->r + (i)) % (a)->cap) * (a)->elem_sz)
#define qtk_cyclearray_delete(a) wtk_free(a)
#define qtk_cyclearray_pop(a) qtk_cyclearray_popn(a, 1);

qtk_cyclearray_t *qtk_cyclearray_new(int cap, int elem_sz);
int qtk_cyclearray_push(qtk_cyclearray_t *a, void *elem, void *poped);
void *qtk_cyclearray_popn(qtk_cyclearray_t *a, int n);
int qtk_cyclearray_reset(qtk_cyclearray_t *a);

#ifdef __cplusplus
};
#endif
#endif
