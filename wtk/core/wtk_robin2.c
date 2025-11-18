#include "wtk/core/wtk_alloc.h"
#include "wtk_robin2.h" 

wtk_robin2_t* wtk_robin2_new(int n,int vsize)
{
	wtk_robin2_t *rb;

	rb=(wtk_robin2_t*)wtk_malloc(sizeof(wtk_robin2_t));
	rb->nslot=n;
	rb->pop=0;
	rb->used=0;
	rb->slot_size=vsize;
	rb->p=(char*)wtk_malloc(n*vsize);
	return rb;
}

void wtk_robin2_delete(wtk_robin2_t *rb)
{
	wtk_free(rb->p);
	wtk_free(rb);
}

void wtk_robin2_reset(wtk_robin2_t *rb)
{
	rb->pop=0;
	rb->used=0;
}

void wtk_robin2_push(wtk_robin2_t *rb,void *p)
{
	int index;

	if(rb->used>=rb->nslot)
	{
		wtk_robin2_pop(rb);
	}
	index=(rb->pop+rb->used)%rb->nslot;
	memcpy(rb->p+index*rb->slot_size,p,rb->slot_size);
	++rb->used;
}

void* wtk_robin2_get(wtk_robin2_t *r,int index)
{
	index=(r->pop+index)%r->nslot;

	return r->p+index*r->slot_size;
}

void* wtk_robin2_pop(wtk_robin2_t *r)
{
	void *d;

	d=r->p+r->pop*r->slot_size;
	r->pop=(r->pop+1)%r->nslot;
	--r->used;
	return d;
}
