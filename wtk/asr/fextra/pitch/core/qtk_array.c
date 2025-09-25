#include "wtk/asr/fextra/pitch/core/qtk_array.h"
#include "wtk/core/wtk_alloc.h"

static int _array_expand(qtk_array_t *a, uint32_t expect_cap) {
    void *new_data;

    a->cap = max(a->cap * 2, expect_cap);
    new_data = wtk_calloc(a->cap, a->elem_sz);
    if (a->dim) {
        memcpy(new_data, a->data, a->dim * a->elem_sz);
    }
    wtk_free(a->data);
    a->data = new_data;
    return 0;
}

static int _array_shrink(qtk_array_t *a, uint32_t sz) {
    void *new_data;
    uint32_t cpyed;

    a->cap = max(sz, 2);
    new_data = wtk_calloc(a->cap, a->elem_sz);
    cpyed = min(a->dim, sz);
    if (cpyed > 0) {
        memcpy(new_data, a->data, cpyed * a->elem_sz);
    }
    wtk_free(a->data);
    a->data = new_data;

    return 0;
}

qtk_array_t *qtk_array_new(uint32_t cap, uint32_t elem_sz) {
    qtk_array_t *a;

    a = wtk_malloc(sizeof(qtk_array_t));
    a->dim = 0;
    a->cap = cap;
    a->elem_sz = elem_sz;
    //a->tag = NULL;
    a->data = wtk_calloc(a->cap, elem_sz);

    return a;
}

void qtk_array_delete(qtk_array_t *a) {
    wtk_free(a->data);
    wtk_free(a);
}

int qtk_array_resize(qtk_array_t *a, uint32_t sz) {
    if (sz > a->cap) {
        _array_expand(a, sz);
    } else if (sz < a->cap / 2 && a->cap > 128) {
        _array_shrink(a, sz);
    }
    a->dim = sz;
    return 0;
}

int qtk_array_push(qtk_array_t *a, void *p) {
    if (a->dim + 1 > a->cap) {
        _array_expand(a, a->dim + 1);
    }
    memcpy(cast(char *, a->data) + a->dim * a->elem_sz, p, a->elem_sz);
    a->dim++;
    return 0;
}

void qtk_array_insert(qtk_array_t *a, void *p, int index)
{
	memcpy(cast(char *, a->data) + index * a->elem_sz, p, a->elem_sz);
}

void *qtk_array_get(qtk_array_t *a, uint32_t idx) {
    return cast(char *, a->data) + idx * a->elem_sz;
}

void qtk_array_swap(qtk_array_t *a, qtk_array_t *b) {
    qtk_array_t tmp;

    tmp = *a;
    *a = *b;
    *b = tmp;
}

void qtk_array_clear(qtk_array_t *a) { a->dim = 0; }

int qtk_array_resize1(qtk_array_t *a, uint32_t sz,
                      void (*initializer)(void *elem)) {
    int idx;
    int old_dim = a->dim;
    qtk_array_resize(a, sz);

    for (idx = old_dim; idx < sz; idx++) {
        initializer(qtk_array_get(a, idx));
    }

    return 0;
}
