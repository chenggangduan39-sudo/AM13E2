#include "qtk_blockheap.h" 

int qtk_blockheap_init(qtk_blockheap_t *heap,int max_size,int up)
{
	int ret;

	ret = qtk_lockheap_init((qtk_lockheap_t*)heap,max_size,up);
	if(ret != 0) {
		goto end;
	}
	ret = wtk_sem_init(&heap->sem,0);
end:
	return ret;
}

void qtk_blockheap_clean(qtk_blockheap_t *heap)
{
	qtk_lockheap_clean((qtk_lockheap_t*)heap);
	wtk_sem_clean(&heap->sem);
}

int qtk_blockheap_is_empty(qtk_blockheap_t *heap)
{
	return qtk_lockheap_is_empty((qtk_lockheap_t*)heap);
}

int qtk_blockheap_is_full(qtk_blockheap_t *heap)
{
	return qtk_lockheap_is_full((qtk_lockheap_t*)heap);
}

int qtk_blockheap_push_int(qtk_blockheap_t *heap,int key,int value)
{
	int ret;

	ret = qtk_lockheap_push_int((qtk_lockheap_t*)heap,key,value);
	if(ret != 0) {
		goto end;
	}

	ret = wtk_sem_inc(&heap->sem);
end:
	return ret;
}

int qtk_blockheap_pop_int(qtk_blockheap_t *heap,int timeout,int *ret)
{
	int num;

	*ret = wtk_sem_acquire(&heap->sem,timeout);
	if(*ret != 0) {
		num = 0;
		goto end;
	}

	num = qtk_lockheap_pop_int((qtk_lockheap_t*)heap,ret);

end:
	return num;
}

int qtk_blockheap_push_dbl(qtk_blockheap_t *heap,int key,double value)
{
	int ret;

	ret = qtk_lockheap_push_dbl((qtk_lockheap_t*)heap,key,value);
	if(ret != 0) {
		goto end;
	}

	ret = wtk_sem_inc(&heap->sem);
end:
	return ret;
}

double qtk_blockheap_pop_dbl(qtk_blockheap_t *heap,int timeout,int *ret)
{
	double num;

	*ret = wtk_sem_acquire(&heap->sem,timeout);
	if(*ret != 0) {
		num = 0.0;
		goto end;
	}

	num = qtk_lockheap_pop_dbl((qtk_lockheap_t*)heap,ret);
end:
	return num;
}

int qtk_blockheap_push_obj(qtk_blockheap_t *heap,int key,void *obj)
{
	int ret;

	ret = qtk_lockheap_push_obj((qtk_lockheap_t*)heap,key,obj);
	if(ret != 0) {
		goto end;
	}

	ret = wtk_sem_inc(&heap->sem);
end:
	return ret;
}

void* qtk_blockheap_pop_obj(qtk_blockheap_t *heap,int timeout,int *ret)
{
	void *obj;

	*ret = wtk_sem_acquire(&heap->sem,timeout);
	if(*ret != 0) {
		obj = NULL;
		goto end;
	}

	obj = qtk_lockheap_pop_obj((qtk_lockheap_t*)heap,ret);
end:
	return obj;
}
