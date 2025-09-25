#include "qtk_kwdec_hash_list.h"

qtk_kwdec_hash_list_t* qtk_kwdec_hash_list_new(int size)
{
	int i;
	qtk_kwdec_hash_list_t* hash = (qtk_kwdec_hash_list_t*) wtk_malloc(
			sizeof(qtk_kwdec_hash_list_t));
	//qtk_hash_bucket_t * bucket;

	hash->size = size;
	hash->list_head = NULL;
	hash->bucket_list_tail = -1;
	hash->freed_head = NULL;
	//hash->slot=(wtk_queue_t**)wtk_calloc(nslot,sizeof(wtk_queue_t*));
	hash->bucket_q=(qtk_kwdec_hash_bucket_t**)wtk_calloc(hash->size,sizeof(qtk_kwdec_hash_bucket_t*));
	//wtk_queue_init(&(hash->bucket_q));
	hash->heap = wtk_heap_new(4096);
	for (i = 0; i < size; i++)
	{
		//bucket=hash->bucket_q[i];
		//wtk_debug("%d\n",i);
		hash->bucket_q[i]=(qtk_kwdec_hash_bucket_t*)wtk_malloc(sizeof(qtk_kwdec_hash_bucket_t));
	//	hash->bucket_q[i]=(qtk_hash_bucket_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_hash_bucket_t));
		hash->bucket_q[i]->prev_bucket=-1;
		hash->bucket_q[i]->last_elem=NULL;
		//wtk_queue_push(&(hash->bucket_q),&(bucket->q_n));
	}

	return hash;
}

void qtk_kwdec_hash_list_reset(qtk_kwdec_hash_list_t* hash)
{
	int i;
	//qtk_hash_bucket_t * bucket;

	wtk_heap_reset(hash->heap);
	hash->list_head = NULL;
	hash->bucket_list_tail = -1;
	hash->freed_head = NULL;
	//wtk_queue_init(&(hash->bucket_q));
	//TODO
	for (i = 0; i < hash->size; i++)
	{
	//	hash->bucket_q[i]=(qtk_hash_bucket_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_hash_bucket_t));
                hash->bucket_q[i]->prev_bucket=-1;
                hash->bucket_q[i]->last_elem=NULL;
		//wtk_queue_push(&(hash->bucket_q),&(bucket->q_n));
	}
}

void qtk_kwdec_hash_list_delete(qtk_kwdec_hash_list_t* hash)
{
	int i;
	for (i = 0; i < hash->size; i++)
	{
		wtk_free(hash->bucket_q[i]);
	}
	wtk_free(hash->bucket_q);
	wtk_heap_delete(hash->heap);
	wtk_free(hash);
}

qtk_kwdec_hash_elem_t* qtk_kwdec_hash_elem_new(qtk_kwdec_hash_list_t* hash)
{
	qtk_kwdec_hash_elem_t* elem;

	elem=(qtk_kwdec_hash_elem_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_kwdec_hash_elem_t));
	
	elem->token=NULL;
	elem->key=-1;
	elem->state=NULL;
	elem->tail=NULL;	

	return elem;
}

qtk_kwdec_pth_t* qtk_kwdec_hash_list_new_pth(qtk_kwdec_hash_list_t* hash,qtk_kwdec_token_t *token)
{
	qtk_kwdec_pth_t *pth=(qtk_kwdec_pth_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_kwdec_pth_t));

	pth->lbest=token;

	return pth;
}

qtk_kwdec_token_t* qtk_kwdec_hash_list_new_token(qtk_kwdec_hash_list_t* hash,int extra_cost)
{
	qtk_kwdec_token_t *token=(qtk_kwdec_token_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_kwdec_token_t));

	token->extra_cost=extra_cost;
	token->tot_cost=0.0;
	token->ac_cost=0.0;
	token->link=NULL;
	token->pth=NULL;
	token->next=NULL;
	token->hook=token;

	return token;
}

int  qtk_kwdec_hash_list_Insert(qtk_kwdec_hash_list_t* hash, int state_id,wtk_fst_state_t* state,
		qtk_kwdec_token_t* token)
{
	//wtk_debug("%d\n",hash->size);
	int ret,index = state_id % (hash->size);
//	wtk_debug("insert state:%d to bucket:%d\n",state,index);
	qtk_kwdec_hash_bucket_t *bucket,*last_bucket;
	qtk_kwdec_hash_elem_t* elem;

	//qn=wtk_queue_peek(&(hash->bucket_q),index);
	//if(!qn){ret=-1;goto end;}
//	int k;
//	for(k=0;k<1000;k++)
//	{
//		wtk_debug("%p\n",hash->bucket_q[k]);
//	}

	bucket=hash->bucket_q[index];//data_offset2(qn,qtk_hash_bucket_t,q_n);
	elem=qtk_kwdec_hash_elem_new(hash);
	//wtk_debug("yyyyyyy %p\n",elem);
	elem->key=state_id;
	elem->token=token;
	elem->state=state;
	//wtk_debug("%p %d\n",bucket,index);
	if(bucket->last_elem==NULL)
	{
//		wtk_debug("last elem null\n");
		if(hash->bucket_list_tail==-1)
		{	
//			wtk_debug("list head:state %d\n",state);
			hash->list_head=elem;
		}else
		{	
			//qn1=wtk_queue_peek(&(hash->bucket_q),hash->bucket_list_tail);
			//if(!qn1){ret=-1;goto end;}
			last_bucket=hash->bucket_q[hash->bucket_list_tail];//data_offset2(qn1,qtk_hash_bucket_t,q_n);
			last_bucket->last_elem->tail=elem;
//			wtk_debug("state:%d tail:%d\n",last_bucket->last_elem->key,elem->key);
		}
		elem->tail=NULL;
		bucket->last_elem=elem;
		bucket->prev_bucket=hash->bucket_list_tail;
		hash->bucket_list_tail=index;
//		wtk_debug("prevbucket:%d tailbucket:%d\n",bucket->prev_bucket,hash->bucket_list_tail);
	}else
	{	
//		wtk_debug("last elem not null\n");
		//wtk_debug("index:%d elem:%p bucket last:%p \n",index,elem,bucket->last_elem);
		elem->tail=bucket->last_elem->tail;
		bucket->last_elem->tail=elem;
		bucket->last_elem=elem;
	}
	ret=0;
		//wtk_debug("%p\n",hash->list_head);
		return ret;
}

//qtk_hash_elem_t* qtk_hash_list_find(qtk_hash_list_t* hash, int state)
//{
////	wtk_debug("find state:%d\n",state);
//	int index = state % (hash->size);
//	wtk_queue_node_t *qn,*qn1;
//	qtk_hash_bucket_t *bucket,*pre_bucket;
//	qtk_hash_elem_t *elem,*head,*tail;
//
//	pre_bucket=NULL;
//	qn=wtk_queue_peek(&(hash->bucket_q),index);
//	if(!qn){return NULL;}
//	bucket=data_offset2(qn,qtk_hash_bucket_t,q_n);
//
//	if(bucket->prev_bucket!=-1)
//	{
//		qn1=wtk_queue_peek(&(hash->bucket_q),bucket->prev_bucket);
//		if(!qn1){return NULL;}
//		pre_bucket=data_offset2(qn1,qtk_hash_bucket_t,q_n);
//	}
//
//	if(bucket->last_elem==NULL)
//	{
//		return NULL;
//	}else
//	{
/////		wtk_debug("ccc:%d\n",bucket->prev_bucket);
//		head=(!pre_bucket)?hash->list_head:pre_bucket->last_elem->tail;
////		wtk_debug("find:%p bucket:%p\n",head,pre_bucket);
////		head=hash->list_head;
//		tail=bucket->last_elem->tail;
//		for(elem=head;elem!=tail;elem=elem->tail)
//		{
//		///	wtk_debug("yyy %d\n",elem->key);
//			if(elem->key==state) return elem;
//		}
//	}
/////	wtk_debug("haha\n");
//	return NULL;
//}

void qtk_kwdec_hash_list_del_elem(qtk_kwdec_hash_list_t* hash, qtk_kwdec_hash_elem_t* elem)
{
	elem->tail=hash->freed_head;
	hash->freed_head=elem;
}

qtk_kwdec_hash_elem_t* qtk_kwdec_hash_list_clear(qtk_kwdec_hash_list_t* hash)
{
	qtk_kwdec_hash_elem_t *elem;
	int i;
//	for(qn=hash->bucket_q.pop;qn;qn=qn->next)
//	{
//		bucket=data_offset(qn,qtk_hash_bucket_t,q_n);
//		bucket->last_elem=NULL;//TODO maybe searching form bucket_list_tail is faster?
//	}
//	wtk_debug("%p\n",hash->list_head);
	for(i=0;i<hash->size;i++)
	{
		hash->bucket_q[i]->last_elem=NULL;
	}
	hash->bucket_list_tail=-1;
	elem=hash->list_head;
//	wtk_debug("%p\n",hash->list_head);
//	hash->list_head=NULL;

	return elem;
}

qtk_kwdec_link_t* qtk_kwdec_hash_list_link_new(qtk_kwdec_hash_list_t* hash,qtk_kwdec_token_t* token,int in_label,int out_label,float graph_cost,float acoustic_cost,qtk_kwdec_link_t* next)
{
	qtk_kwdec_link_t* link=NULL;
	link=(qtk_kwdec_link_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_kwdec_link_t));

	link->next_tok=token;
	link->in_label=in_label;
	link->out_label=out_label;
	link->graph_cost=graph_cost;
	link->acoustic_cost=acoustic_cost;
	link->next=next;
	return link;
}

void qtk_kwdec_hash_list_link_delete(qtk_kwdec_token_t* token)
{
	qtk_kwdec_link_t *link,*tmp;

	link=token->link;
	while(link!=NULL)
	{
		tmp=link->next;
//		wtk_free(link);//TODO2 free token???
		link=tmp;
	}
	token->link=NULL;
}

void qtk_kwdec_hash_list_del(qtk_kwdec_hash_list_t* hash, qtk_kwdec_hash_elem_t* elem)
{
	//qtk_kwfstdec_link_t *link,*tmp;

//	elem->tail=hash->list_head;
//	hash->list_head=elem;
//	while(link!=NULL)
//	{
//		tmp=link->next;
//		wtk_free(link);//TODO2 free token???
//		link=tmp;
//	}
//	token->link=NULL;
}
