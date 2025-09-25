#ifndef QTK_CORE_QTK_HEAP
#define QTK_CORE_QTK_HEAP

#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_alloc.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_heap qtk_heap_t;

typedef struct  {
	int key;
	union {
		int    num;
		double dbl;
		void*  obj;
	}value;
}qtk_heap_node_t;

typedef int(*qtk_heap_compare_func)(qtk_heap_node_t n1,qtk_heap_node_t n2);

#define QTK_HEAP \
	qtk_heap_node_t *list; \
	int max_size; \
	int capacity; \
	qtk_heap_compare_func compare_func; \

struct qtk_heap
{
	QTK_HEAP
};

void qtk_heap_init(qtk_heap_t *heap,int max_size,int up);
void qtk_heap_clean(qtk_heap_t *heap);
int qtk_heap_is_empty(qtk_heap_t *heap);
int qtk_heap_is_full(qtk_heap_t *heap);

int qtk_heap_push_int(qtk_heap_t *heap,int key,int value);
int qtk_heap_pop_int(qtk_heap_t *heap,int *ret);

int qtk_heap_push_dbl(qtk_heap_t *heap,int key,double value);
double qtk_heap_pop_dbl(qtk_heap_t *heap,int *ret);

int qtk_heap_push_obj(qtk_heap_t *heap,int key,void *obj);
void* qtk_heap_pop_obj(qtk_heap_t *heap,int *ret);

#ifdef __cplusplus
};
#endif
#endif
