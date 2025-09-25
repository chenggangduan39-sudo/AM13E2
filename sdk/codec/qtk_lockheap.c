#include "qtk_lockheap.h" 

int qtk_lockheap_init(qtk_lockheap_t *heap,int max_size,int up)
{
	qtk_heap_init((qtk_heap_t*)heap,max_size,up);
	return wtk_lock_init(&heap->l);
}

void qtk_lockheap_clean(qtk_lockheap_t *heap)
{
	qtk_heap_clean((qtk_heap_t*)heap);
	wtk_lock_clean(&heap->l);
}

int qtk_lockheap_is_empty(qtk_lockheap_t *heap)
{
	return qtk_heap_is_empty((qtk_heap_t*)heap);
}

int qtk_lockheap_is_full(qtk_lockheap_t *heap)
{
	return qtk_heap_is_full((qtk_heap_t*)heap);
}

int qtk_lockheap_push_int(qtk_lockheap_t *heap,int key,int value)
{
	int ret;

	ret = wtk_lock_lock(&heap->l);
	if(ret != 0) {
		perror(__FUNCTION__);
		return -1;
	}
	ret = qtk_heap_push_int((qtk_heap_t*)heap,key,value);
	wtk_lock_unlock(&heap->l);

	return ret;
}

int qtk_lockheap_pop_int(qtk_lockheap_t *heap,int *ret)
{
	int lret;

	lret = wtk_lock_lock(&heap->l);
	if(lret != 0) {
		perror(__FUNCTION__);
		*ret = -1;
		return 0;
	}
	lret = qtk_heap_pop_int((qtk_heap_t*)heap,ret);
	wtk_lock_unlock(&heap->l);

	return lret;
}

int qtk_lockheap_push_dbl(qtk_lockheap_t *heap,int key,double value)
{
	int ret;

	ret = wtk_lock_lock(&heap->l);
	if(ret != 0) {
		perror(__FUNCTION__);
		return -1;
	}
	ret = qtk_heap_push_dbl((qtk_heap_t*)heap,key,value);
	wtk_lock_unlock(&heap->l);

	return ret;
}

double qtk_lockheap_pop_dbl(qtk_lockheap_t *heap,int *ret)
{
	double num;
	int lret;

	lret = wtk_lock_lock(&heap->l);
	if(lret != 0) {
		perror(__FUNCTION__);
		*ret = -1;
		return 0.0;
	}
	num = qtk_heap_pop_dbl((qtk_heap_t*)heap,ret);
	wtk_lock_unlock(&heap->l);

	return num;
}

int qtk_lockheap_push_obj(qtk_lockheap_t *heap,int key,void *obj)
{
	int ret;

	ret = wtk_lock_lock(&heap->l);
	if(ret != 0) {
		perror(__FUNCTION__);
		return -1;
	}
	ret = qtk_heap_push_obj((qtk_heap_t*)heap,key,obj);
	wtk_lock_unlock(&heap->l);

	return ret;
}

void* qtk_lockheap_pop_obj(qtk_lockheap_t *heap,int *ret)
{
	void *obj;
	int lret;

	lret = wtk_lock_lock(&heap->l);
	if(lret != 0) {
		perror(__FUNCTION__);
		*ret = -1;
		return NULL;
	}
	obj = qtk_heap_pop_obj((qtk_heap_t*)heap,ret);
	wtk_lock_unlock(&heap->l);

	return obj;
}
