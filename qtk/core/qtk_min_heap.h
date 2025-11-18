#ifndef __QBL_CORE_QBL_MIN_HEAP_H__
#define __QBL_CORE_QBL_MIN_HEAP_H__
#pragma once
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_min_heap qtk_min_heap_t;
typedef int (*qtk_min_heap_cmp_f)(void *a, void *b);

struct qtk_min_heap {
    int elem_sz;
    int nelem;
    int cap;
    void *buf; // can hold only one element
    void *array;
    qtk_min_heap_cmp_f cmp;
    unsigned mem_ref : 1;
};

qtk_min_heap_t *qtk_min_heap_init(void *data, int *len, int elem_sz, int cap,
                                  qtk_min_heap_cmp_f cmp);
void qtk_min_heap_clean(qtk_min_heap_t *h);
int qtk_min_heap_pop(qtk_min_heap_t *h, void *val);
int qtk_min_heap_push(qtk_min_heap_t *h, void *d);
void *qtk_min_heap_peak(qtk_min_heap_t *h);

int qtk_min_heap_init2(qtk_min_heap_t *h, int elem_sz, int init_cap,
                       qtk_min_heap_cmp_f cmp);

#define qtk_min_heap_reset(h) (h)->nelem = 0
#define qtk_min_heap_clean2(h) qtk_min_heap_clean(h)

#ifdef __cplusplus
};
#endif
#endif
