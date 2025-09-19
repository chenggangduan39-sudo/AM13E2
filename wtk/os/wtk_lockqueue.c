#include "wtk_lockqueue.h"

int wtk_lockqueue_init(wtk_lockqueue_t *q)
{
	wtk_queue_init((wtk_queue_t*)q);
#ifdef HEXAGON
	wtk_lock_init(&(q->l));
	return 0;
#else
	return wtk_lock_init(&(q->l));
#endif
}

int wtk_lockqueue_clean(wtk_lockqueue_t *q)
{
	wtk_lock_clean(&(q->l));
	return 0;
}

int wtk_lockqueue_push(wtk_lockqueue_t *q,wtk_queue_node_t *n)
{
	int ret;

#ifdef HEXAGON
	wtk_lock_lock(&(q->l));
	ret=0;
#else
	ret=wtk_lock_lock(&(q->l));
#endif
	if(ret!=0)
	{
		perror(__FUNCTION__);
		return -1;
	}
	ret=wtk_queue_push((wtk_queue_t*)q,n);
	wtk_lock_unlock(&(q->l));
	return ret;
}

int wtk_lockqueue_push_front(wtk_lockqueue_t *q,wtk_queue_node_t *n)
{
	int ret;

#ifdef HEXAGON
	wtk_lock_lock(&(q->l));
	ret=0;
#else
	ret=wtk_lock_lock(&(q->l));
#endif
	if(ret!=0)
	{
		perror(__FUNCTION__);
		return -1;
	}
	ret=wtk_queue_push_front((wtk_queue_t*)q,n);
	wtk_lock_unlock(&(q->l));
	return ret;
}


wtk_queue_node_t* wtk_lockqueue_pop(wtk_lockqueue_t *q)
{
	wtk_queue_node_t *n;
	int ret;

#ifdef HEXAGON
	wtk_lock_lock(&(q->l));
	ret=0;
#else
	ret=wtk_lock_lock(&(q->l));
#endif
	if(ret!=0)
	{
		//wtk_debug("pop queue node failed len=%d\n",q->length);
		//perror(__FUNCTION__);
		//exit(0);
		return 0;
	}
	n=wtk_queue_pop((wtk_queue_t*)q);
	wtk_lock_unlock(&(q->l));
	return n;
}
