#include "qtk/core/qtk_min_heap.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/core/qtk_type.h"

static int _get_mem_require(int elem_sz, int nelem) {
    return sizeof(qtk_min_heap_t) + elem_sz * nelem + elem_sz;
}

qtk_min_heap_t *qtk_min_heap_init(void *data, int *len, int elem_sz, int cap,
                                  qtk_min_heap_cmp_f cmp) {
    int mem_require;
    qtk_min_heap_t *res;
    int mem_ref = 1;

    mem_require = _get_mem_require(elem_sz, cap);
    if (data == NULL || *len < mem_require) {
        *len = mem_require;
        return NULL;
    }

    res = cast(qtk_min_heap_t *, data);
    res->buf = res + 1;
    res->array = cast(char *, res->buf) + elem_sz;
    res->elem_sz = elem_sz;
    res->nelem = 0;
    res->mem_ref = mem_ref;
    res->cmp = cmp;
    res->cap = cap;
    return res;
}

void qtk_min_heap_clean(qtk_min_heap_t *h) {
    if (h->mem_ref == 1) {
        wtk_free(h);
    } else {
        wtk_free(h->buf);
        wtk_free(h->array);
    }
}

// Don't check boundary
static QTK_INLINE void *_geti(qtk_min_heap_t *h, int idx) {
    return cast(char *, h->array) + h->elem_sz * idx;
}

// Don't check boundary
static QTK_INLINE void _assigni(qtk_min_heap_t *h, int idx, void *d) {
    memcpy(_geti(h, idx), d, h->elem_sz);
}

// Don't check boundary
static QTK_INLINE void _swap_ptr(qtk_min_heap_t *h, void *ptr1, void *ptr2) {
    memcpy(h->buf, ptr1, h->elem_sz);
    memcpy(ptr1, ptr2, h->elem_sz);
    memcpy(ptr2, h->buf, h->elem_sz);
}

static void _down(qtk_min_heap_t *h) {
    int f = 0;
    int k;
    void *ptr_l, *ptr_r, *ptr_min, *ptr_p;

    while ((k = (f << 1) + 1) < h->nelem) {
        ptr_min = ptr_l = _geti(h, k);
        ptr_r = NULL;
        ptr_p = _geti(h, f);
        if (k < h->nelem - 1) {
            ptr_r = cast(char *, ptr_l) + h->elem_sz;
            if (h->cmp(ptr_l, ptr_r) > 0) {
                ptr_min = ptr_r;
                k++;
            }
        }
        if (h->cmp(ptr_p, ptr_min) <= 0) {
            break;
        }
        _swap_ptr(h, ptr_min, ptr_p);
        f = k;
    }
}

static void _up(qtk_min_heap_t *h) {
    int f;
    int j = h->nelem - 1;
    void *ptr_p, *ptr_c;
    while ((f = (j - 1) >> 1) >= 0) {
        ptr_c = _geti(h, j);
        ptr_p = _geti(h, f);
        if (h->cmp(ptr_p, ptr_c) <= 0) {
            break;
        }
        _swap_ptr(h, ptr_p, ptr_c);
        j = f;
    }
}

int qtk_min_heap_pop(qtk_min_heap_t *h, void *val) {
    if (h->nelem == 0) {
        return -1;
    }
    memcpy(val, h->array, h->elem_sz);
    memcpy(h->array, _geti(h, h->nelem - 1), h->elem_sz);
    h->nelem--;
    _down(h);
    return 0;
}

static int _min_heap_expand(qtk_min_heap_t *h) {
    int new_cap;
    void *array;

    if (h->nelem > 1000) {
        new_cap = h->cap << 1;
    } else {
        new_cap = h->cap + (h->cap >> 1);
    }

    array = wtk_malloc(new_cap * h->elem_sz);
    if (array == NULL) {
        return -1;
    }
    memcpy(array, h->array, h->elem_sz * h->nelem);
    wtk_free(h->array);
    h->array = array;
    h->cap = new_cap;

    return 0;
}

int qtk_min_heap_push(qtk_min_heap_t *h, void *d) {
    if (h->nelem >= h->cap) {
        if (h->mem_ref || _min_heap_expand(h) != 0) {
            return -1;
        }
    }
    _assigni(h, h->nelem, d);
    h->nelem++;
    _up(h);
    return 0;
}

void *qtk_min_heap_peak(qtk_min_heap_t *h) {
    if (h->nelem == 0) {
        return NULL;
    }
    return h->array;
}

int qtk_min_heap_init2(qtk_min_heap_t *h, int elem_sz, int init_cap,
                       qtk_min_heap_cmp_f cmp) {
    h->buf = wtk_malloc(elem_sz);
    if (h->buf == NULL) {
        return -1;
    }
    h->array = wtk_malloc(elem_sz * init_cap);
    if (h->array == NULL) {
        wtk_free(h->buf);
        return -1;
    }
    h->elem_sz = elem_sz;
    h->nelem = 0;
    h->mem_ref = 0;
    h->cmp = cmp;
    h->cap = init_cap;
    return 0;
}
