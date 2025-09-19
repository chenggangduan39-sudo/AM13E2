/*
 * qtk_test_queue.c
 *
 *  Created on: Jul 3, 2023
 *      Author: dm
 */
#include "wtk/os/wtk_blockqueue.h"
#include "qtk_test_queue.h"
#include "wtk/core/wtk_alloc.h"

qtk_test_queue_t* qtk_test_queue_new(int isblock)
{
	qtk_test_queue_t* q;

	q = wtk_malloc(sizeof(*q));
	q->isblock = isblock;
	if (isblock)
	{
		q->q = wtk_malloc(sizeof(wtk_blockqueue_t));
		wtk_blockqueue_init(q->q);
	}
	else
	{
		q->q = wtk_malloc(sizeof(wtk_queue_t));
		wtk_queue_init(q->q);
	}

	return q;
}

void qtk_test_queue_delete(qtk_test_queue_t* q)
{
	if (q->isblock)
		wtk_blockqueue_clean(q->q);
	free(q->q);
	free(q);
}

int qtk_test_queue_push(qtk_test_queue_t* q,void *n)
{
	int ret;
	if (q->isblock)
		ret=wtk_blockqueue_push(q->q, n);
	else
		ret=wtk_queue_push(q->q, n);

	return ret;
}

void* qtk_test_queue_pop(qtk_test_queue_t *q,int ms,int *is_timeout)
{
	if (q->isblock)
		return wtk_blockqueue_pop(q->q, ms, is_timeout);
	else
		return wtk_queue_pop(q->q);
}
