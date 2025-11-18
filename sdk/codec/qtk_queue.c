#include "qtk_queue.h"

int qtk_queue_push(qtk_queue_t *q,qtk_queue_node_t *n)
{
	n->prev=q->push;
	if(q->push)
	{
		q->push->next=n;
	}
	n->next=0;
	q->push=n;
	if(!q->pop)
	{
		q->pop=n;
	}
	if(q->listener)
	{
		q->listener(q->data);
	}
	++q->length;
	return 0;
}

int qtk_queue_push_front(qtk_queue_t *q,qtk_queue_node_t *n)
{
	n->next=q->pop;
	if(q->pop)
	{
		q->pop->prev=n;
	}
	n->prev=0;
	q->pop=n;
	if(!q->push)
	{
		q->push=n;
	}
	if(q->listener)
	{
		q->listener(q->data);
	}
	++q->length;
	return 0;
}

qtk_queue_node_t* qtk_queue_pop(qtk_queue_t *q)
{
	qtk_queue_node_t* n;

	if(q->length<=0){return 0;}
	n=0;
	if(!q->pop){goto end;}
	n=q->pop;
	q->pop=q->pop->next;
	if(q->pop)
	{
		q->pop->prev=0;
	}else
	{
		q->push=0;
	}
	--q->length;
end:
	return n;
}

int qtk_queue_remove(qtk_queue_t *q,qtk_queue_node_t *n)
{
	if(q->length<=0){return 0;}
	if(n->prev)
	{
		n->prev->next=n->next;
	}else
	{
		q->pop=n->next;
	}
	if(n->next)
	{
		n->next->prev=n->prev;
	}else
	{
		q->push=n->prev;
	}
	n->prev=n->next=0;
    --q->length;
	return 0;
}