#ifndef QTK_CORE_QTK_BLOCKHEAP
#define QTK_CORE_QTK_BLOCKHEAP

#include "qtk_lockheap.h"
#include "wtk/os/wtk_sem.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_blockheap qtk_blockheap_t;
struct qtk_blockheap {
    QTK_HEAP
    wtk_lock_t l;
    wtk_sem_t sem;
};

int qtk_blockheap_init(qtk_blockheap_t *heap, int max_size, int up);
void qtk_blockheap_clean(qtk_blockheap_t *heap);

int qtk_blockheap_is_empty(qtk_blockheap_t *heap);
int qtk_blockheap_is_full(qtk_blockheap_t *heap);

int qtk_blockheap_push_int(qtk_blockheap_t *heap, int key, int value);
int qtk_blockheap_pop_int(qtk_blockheap_t *heap, int timeout, int *ret);

int qtk_blockheap_push_dbl(qtk_blockheap_t *heap, int key, double value);
double qtk_blockheap_pop_dbl(qtk_blockheap_t *heap, int timeout, int *ret);

int qtk_blockheap_push_obj(qtk_blockheap_t *heap, int key, void *obj);
void *qtk_blockheap_pop_obj(qtk_blockheap_t *heap, int timeout, int *ret);

#ifdef __cplusplus
};
#endif
#endif
