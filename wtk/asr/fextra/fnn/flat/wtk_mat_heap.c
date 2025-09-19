#include "wtk_mat_heap.h"

wtk_mat_heap_t* wtk_mat_heap_new(int max_row,int max_col)
{
	wtk_mat_heap_t *h;

	h=(wtk_mat_heap_t*)wtk_malloc(sizeof(wtk_mat_heap_t));
	h->heap=wtk_heap_new(4096);
	h->max_row=max_row;
	h->max_col=max_col;
	h->nslot=max_col;
	h->slot=(wtk_queue_t**)wtk_calloc(h->nslot,sizeof(wtk_queue_t*));
	h->node_pool=wtk_vpool_new(sizeof(wtk_mat_heap_node_t),1024);
	return h;
}

void wtk_mat_heap_delete(wtk_mat_heap_t *h)
{
	wtk_free(h->slot);
	wtk_heap_delete(h->heap);
	wtk_free(h);
}

wtk_mati_t* wtk_mat_heap_pop(wtk_mat_heap_t *h,int row,int col)
{
	wtk_mati_t *m;
	wtk_mat_heap_node_t *node;
	wtk_queue_t *q;
	wtk_queue_node_t *qn;
	int index;

	index=((row<<16)+col)%(h->nslot);
	//wtk_debug("row=%d col=%d index=%d\n",row,col,index);
	if(!h->slot[index])
	{
		q=(wtk_queue_t*)wtk_heap_malloc(h->heap,sizeof(wtk_queue_t));
		wtk_queue_init(q);
		h->slot[index]=q;
		return wtk_mati_new(row,col);
	}
	q=h->slot[index];
	if(q->length<=0)
	{
		return wtk_mati_new(row,col);
	}
	for(qn=q->pop;qn;qn=qn->next)
	{
		node=data_offset(qn,wtk_mat_heap_node_t,q_n);
		if(node->m->row==row && node->m->col==col)
		{
			m=node->m;
			wtk_queue_remove(q,qn);
			wtk_vpool_push(h->node_pool,node);
			return m;
		}
	}
	return wtk_mati_new(row,col);
}

void wtk_mat_heap_push(wtk_mat_heap_t *h,wtk_mati_t *m)
{
	wtk_mat_heap_node_t *node;
	wtk_queue_t *q;
	int index;

	index=((m->row<<16)+m->col)%(h->nslot);
	//wtk_debug("row=%d col=%d index=%d\n",m->row,m->col,index);
	q=h->slot[index];
	node=(wtk_mat_heap_node_t*)wtk_vpool_pop(h->node_pool);
	node->m=m;
	wtk_queue_push(q,&(node->q_n));
}
