#include "qtk_lockqueue.h"

int qtk_lockqueue_init(qtk_lockqueue_t *q)
{
	qtk_queue_init((qtk_queue_t*)q);
	return qtk_lock_init(&(q->l));
}

int qtk_lockqueue_clean(qtk_lockqueue_t *q)
{
	qtk_lock_clean(&(q->l));
	return 0;
}

int qtk_lockqueue_push(qtk_lockqueue_t *q,qtk_queue_node_t *n)
{
	int ret;

	ret=qtk_lock_lock(&(q->l));
	if(ret!=0)
	{
		return -1;
	}
	ret=qtk_queue_push((qtk_queue_t*)q,n);
	qtk_lock_unlock(&(q->l));
	return ret;
}

int qtk_lockqueue_push_front(qtk_lockqueue_t *q,qtk_queue_node_t *n)
{
	int ret;

	ret=qtk_lock_lock(&(q->l));
	if(ret!=0)
	{
		return -1;
	}
	ret=qtk_queue_push_front((qtk_queue_t*)q,n);
	qtk_lock_unlock(&(q->l));
	return ret;
}


qtk_queue_node_t* qtk_lockqueue_pop(qtk_lockqueue_t *q)
{
	qtk_queue_node_t *n;
	int ret;

	ret=qtk_lock_lock(&(q->l));
	if(ret!=0)
	{
		return 0;
	}
	n=qtk_queue_pop((qtk_queue_t*)q);
	qtk_lock_unlock(&(q->l));
	return n;
}
