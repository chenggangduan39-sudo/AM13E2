#ifndef __QTK_CORE_QTK_ARRAY_H__
#define __QTK_CORE_QTK_ARRAY_H__
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

#define qtk_array_back(a) qtk_array_get(a, (a)->dim - 1)

typedef struct qtk_array qtk_array_t;
struct qtk_array {
    void *data;
    //float *tag;//array name or statistics value
    uint32_t cap;
    uint32_t dim;
    uint32_t elem_sz;
};

qtk_array_t *qtk_array_new(uint32_t cap, uint32_t elem_sz);
void qtk_array_delete(qtk_array_t *a);
int qtk_array_resize(qtk_array_t *a, uint32_t sz);
int qtk_array_resize1(qtk_array_t *a, uint32_t sz,
                      void (*initializer)(void *elem));
int qtk_array_push(qtk_array_t *a, void *p);
void qtk_array_insert(qtk_array_t *a, void *p, int index);
void *qtk_array_get(qtk_array_t *a, uint32_t idx);
void qtk_array_swap(qtk_array_t *a, qtk_array_t *b);
void qtk_array_clear(qtk_array_t *a);

#ifdef __cplusplus
};
#endif
#endif
