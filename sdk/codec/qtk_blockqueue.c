#include "qtk_blockqueue.h"

int qtk_blockqueue_init(qtk_blockqueue_t *q)
{
	int ret;

	ret=qtk_lockqueue_init((qtk_lockqueue_t*)q);
	if(ret!=0){goto end;}
	ret=qtk_sem_init(&(q->sem),0);
end:
	return ret;
}

int qtk_blockqueue_clean(qtk_blockqueue_t *q)
{
	qtk_lockqueue_clean((qtk_lockqueue_t*)q);
	qtk_sem_clean(&(q->sem));
	return 0;
}

int qtk_blockqueue_push(qtk_blockqueue_t *q,qtk_queue_node_t *n)
{
	int ret;

	ret=qtk_lockqueue_push((qtk_lockqueue_t*)q,n);
	if(ret!=0){goto end;}
	ret=qtk_sem_inc(&(q->sem));
end:
	return ret;
}

int qtk_blockqueue_push_front(qtk_blockqueue_t *q,qtk_queue_node_t *n)
{
	int ret;

	ret=qtk_lockqueue_push_front((qtk_lockqueue_t*)q,n);
	if(ret!=0){goto end;}
	ret=qtk_sem_inc(&(q->sem));
end:
	return ret;
}

qtk_queue_node_t* qtk_blockqueue_pop(qtk_blockqueue_t *q,int ms,int *is_timeout)
{
	qtk_queue_node_t *n;
	int ret;

	n=0;
	ret=qtk_sem_acquire(&(q->sem),ms);
	if(ret==0)
	{
		n=qtk_lockqueue_pop((qtk_lockqueue_t*)q);
	}
	if(is_timeout)
	{
		*is_timeout= ret==0 ? 0 :1;
	}
	return n;
}

int qtk_blockqueue_wake(qtk_blockqueue_t *q)
{
	return qtk_sem_inc(&(q->sem));
}
