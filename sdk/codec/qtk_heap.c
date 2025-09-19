#include "qtk_heap.h"

static void qtk_heap_adjust(qtk_heap_t *heap)
{
	qtk_heap_node_t tmp;
	int n,child;

	n = (heap->capacity - 1) / 2;
	while(n >= 0) {
		child = 2*n + 1;
		if(child < heap->capacity && heap->compare_func(heap->list[n],heap->list[child])) {
			tmp = heap->list[n];
			heap->list[n] = heap->list[child];
			heap->list[child] = tmp;
		}

		child = 2*n + 2;
		if(child < heap->capacity && heap->compare_func(heap->list[n],heap->list[child])) {
			tmp = heap->list[n];
			heap->list[n] = heap->list[child];
			heap->list[child] = tmp;
		}
		--n;
	}
}

static int qtk_heap_compare_max(qtk_heap_node_t n1,qtk_heap_node_t n2)
{
	if(n1.key > n2.key) {
		return 1;
	}
	return 0;
}

static int qtk_heap_compare_min(qtk_heap_node_t n1,qtk_heap_node_t n2)
{
	if(n1.key < n2.key) {
		return 1;
	}
	return 0;
}

void qtk_heap_init(qtk_heap_t*heap,int max_size,int up)
{
	heap->max_size = max_size;
	heap->capacity = 0;
	heap->list = (qtk_heap_node_t*)wtk_calloc(heap->max_size,sizeof(qtk_heap_node_t));
	if(up) {
		heap->compare_func = (qtk_heap_compare_func)qtk_heap_compare_min;
	} else {
		heap->compare_func = (qtk_heap_compare_func)qtk_heap_compare_max;
	}
}

void qtk_heap_clean(qtk_heap_t *heap)
{
	wtk_free(heap->list);
}

int qtk_heap_is_empty(qtk_heap_t *heap)
{
	return heap->capacity==0 ? 1 : 0;
}

int qtk_heap_is_full(qtk_heap_t *heap)
{
	return heap->capacity==heap->max_size ? 1 : 0;
}

int qtk_heap_push_int(qtk_heap_t *heap,int key,int value)
{
	if(qtk_heap_is_full(heap)) {
		return -1;
	}

	heap->list[heap->capacity].key  = key;
	heap->list[heap->capacity].value.num = value;
	++heap->capacity;

	qtk_heap_adjust(heap);
	return 0;
}

int qtk_heap_pop_int(qtk_heap_t *heap,int *ret)
{
	int num;

	if(qtk_heap_is_empty(heap)) {
		*ret = -1;
		return 0;
	}

	*ret = 0;
	num = heap->list[0].value.num;
	--heap->capacity;

	heap->list[0] = heap->list[heap->capacity];

	if(!qtk_heap_is_empty(heap)) {
		qtk_heap_adjust(heap);
	}

	return num;
}

int qtk_heap_push_dbl(qtk_heap_t *heap,int key,double value)
{
	if(qtk_heap_is_full(heap)) {
		return -1;
	}

	heap->list[heap->capacity].key  = key;
	heap->list[heap->capacity].value.dbl = value;
	++heap->capacity;
	qtk_heap_adjust(heap);

	return 0;
}

double qtk_heap_pop_dbl(qtk_heap_t *heap,int *ret)
{
	double num;

	if(qtk_heap_is_empty(heap)) {
		*ret = -1;
		return 0.0;
	}

	*ret = 0;
	num = heap->list[0].value.dbl;
	--heap->capacity;
	heap->list[0] = heap->list[heap->capacity];
	if(!qtk_heap_is_empty(heap)) {
		qtk_heap_adjust(heap);
	}

	return num;
}

int qtk_heap_push_obj(qtk_heap_t *heap,int key,void *obj)
{
	if(qtk_heap_is_full(heap)) {
		return -1;
	}

	heap->list[heap->capacity].key  = key;
	heap->list[heap->capacity].value.obj = obj;
	++heap->capacity;
	qtk_heap_adjust(heap);

	return 0;
}

void* qtk_heap_pop_obj(qtk_heap_t *heap,int *ret)
{
	void *obj;

	if(qtk_heap_is_empty(heap)) {
		*ret = -1;
		return NULL;
	}

	*ret = 0;
	obj = heap->list[0].value.obj;
	--heap->capacity;
	heap->list[0] = heap->list[heap->capacity];
	if(!qtk_heap_is_empty(heap)) {
		qtk_heap_adjust(heap);
	}

	return obj;
}
