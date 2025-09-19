#ifndef QTK_CORE_QTK_LOCKHEAP
#define QTK_CORE_QTK_LOCKHEAP

#include "wtk/os/wtk_lock.h"
#include "qtk_heap.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_lockheap qtk_lockheap_t;

struct qtk_lockheap
{
	QTK_HEAP
	wtk_lock_t l;
};

int qtk_lockheap_init(qtk_lockheap_t *heap,int max_size,int up);
void qtk_lockheap_clean(qtk_lockheap_t *heap);
int qtk_lockheap_is_empty(qtk_lockheap_t *heap);
int qtk_lockheap_is_full(qtk_lockheap_t *heap);

int qtk_lockheap_push_int(qtk_lockheap_t *heap,int key,int value);
int qtk_lockheap_pop_int(qtk_lockheap_t *heap,int *ret);

int qtk_lockheap_push_dbl(qtk_lockheap_t *heap,int key,double value);
double qtk_lockheap_pop_dbl(qtk_lockheap_t *heap,int *ret);

int qtk_lockheap_push_obj(qtk_lockheap_t *heap,int key,void *obj);
void* qtk_lockheap_pop_obj(qtk_lockheap_t *heap,int *ret);

#ifdef __cplusplus
};
#endif
#endif
