#include "qtk_hash_list.h"

qtk_hash_list_t* qtk_hash_list_new(int size)
{
	int i;
	qtk_hash_list_t* hash = (qtk_hash_list_t*) wtk_malloc(
			sizeof(qtk_hash_list_t));
	//qtk_hash_bucket_t * bucket;

	hash->size = size;
	hash->list_head = NULL;
	hash->list_tail = NULL;
	hash->bucket_list_tail = -1;
	hash->freed_head = NULL;
	//hash->slot=(wtk_queue_t**)wtk_calloc(nslot,sizeof(wtk_queue_t*));
	hash->bucket_q=(qtk_hash_bucket_t**)wtk_calloc(hash->size,sizeof(qtk_hash_bucket_t*));
	//wtk_queue_init(&(hash->bucket_q));
	hash->heap = wtk_heap_new(4096);
	for (i = 0; i < size; i++)
	{
		//bucket=hash->bucket_q[i];
		//wtk_debug("%d\n",i);
		hash->bucket_q[i]=(qtk_hash_bucket_t*)wtk_malloc(sizeof(qtk_hash_bucket_t));
	//	hash->bucket_q[i]=(qtk_hash_bucket_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_hash_bucket_t));
		hash->bucket_q[i]->prev_bucket=-1;
		hash->bucket_q[i]->last_elem=NULL;
		//wtk_queue_push(&(hash->bucket_q),&(bucket->q_n));
	}

	return hash;
}

void qtk_hash_list_reset(qtk_hash_list_t* hash)
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

void qtk_hash_list_delete(qtk_hash_list_t* hash)
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

qtk_hash_elem_t* qtk_hash_elem_new(qtk_hash_list_t* hash)
{
	qtk_hash_elem_t* elem;

	elem=(qtk_hash_elem_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_hash_elem_t));
	
	elem->token=NULL;
	elem->key=-1;
	elem->state=NULL;
	elem->tail=NULL;	

	return elem;
}

qtk_kwfstdec_pth_t* qtk_hash_list_new_pth(qtk_hash_list_t* hash,qtk_kwfstdec_token_t *token)
{
	qtk_kwfstdec_pth_t *pth=(qtk_kwfstdec_pth_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_kwfstdec_pth_t));

	pth->lbest=token;

	return pth;
}

qtk_kwfstdec_token_t* qtk_hash_list_new_token(qtk_hash_list_t* hash,float extra_cost)
{
	qtk_kwfstdec_token_t *token=(qtk_kwfstdec_token_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_kwfstdec_token_t));

	token->extra_cost=extra_cost;
	token->tot_cost=0.0f;
	token->ac_cost=0.0f;
	token->context_cost = 0.0f;
	token->link = NULL;
	token->pth=NULL;
	token->next=NULL;
	token->context_state = NULL;
	token->hook = token;

	return token;
}

int  qtk_hash_list_Insert(qtk_hash_list_t* hash, int state_id,wtk_fst_state_t* state,
		qtk_kwfstdec_token_t* token)
{
	//wtk_debug("%d\n",hash->size); 
	int ret,index = state_id % (hash->size);
//	wtk_debug("insert state:%d to bucket:%d\n",state,index);
	qtk_hash_bucket_t *bucket,*last_bucket;
	qtk_hash_elem_t* elem;

	//qn=wtk_queue_peek(&(hash->bucket_q),index);
	//if(!qn){ret=-1;goto end;}
//	int k;
//	for(k=0;k<1000;k++)
//	{
//		wtk_debug("%p\n",hash->bucket_q[k ]);
//	}

	bucket=hash->bucket_q[index];//data_offset2(qn,qtk_hash_bucket_t,q_n);
//	elem=qtk_hash_elem_new(hash);
	elem=qtk_hash_list_new_elem(hash);
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

qtk_hash_elem_t*  qtk_hash_list_new_elem(qtk_hash_list_t* hash)
{
	qtk_hash_elem_t *elem;
	if(hash->freed_head)
	{
		elem = hash->freed_head;
		hash->freed_head = elem->tail;
		elem->tail = NULL;
	}else
		elem = qtk_hash_elem_new(hash);
	return elem;
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

void qtk_hash_list_del_elem(qtk_hash_list_t* hash, qtk_hash_elem_t* elem)
{
	//NOTE: uninit, use must give value
	elem->tail=hash->freed_head;
	hash->freed_head=elem;
}

qtk_hash_elem_t* qtk_hash_list_clear(qtk_hash_list_t* hash)
{
	qtk_hash_elem_t *elem;
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
	//wtk_debug("%p\n",hash->list_head);
	//hash->list_head=NULL;
	hash->list_head=NULL;   // reused for bug, by dmd

	return elem;
}

qtk_kwfstdec_link_t* qtk_hash_list_link_new(qtk_hash_list_t* hash,qtk_kwfstdec_token_t* token,int in_label,int out_label,float graph_cost,float acoustic_cost,qtk_kwfstdec_link_t* next)
{
	qtk_kwfstdec_link_t* link=NULL;
	link=(qtk_kwfstdec_link_t*)wtk_heap_malloc(hash->heap,sizeof(qtk_kwfstdec_link_t));

	link->next_tok=token;
	link->in_label=in_label;
	link->out_label=out_label;
	link->graph_cost=graph_cost;
	link->acoustic_cost=acoustic_cost;
	link->next=next;
	link->context_score = 0.0;
	return link;
}

void qtk_hash_list_link_delete(qtk_kwfstdec_token_t* token)
{
	qtk_kwfstdec_link_t *link,*tmp;

	link=token->link;
	while(link!=NULL)
	{
		tmp=link->next;
//		wtk_free(link);//TODO2 free token???
		link=tmp;
	}
	token->link=NULL;
}

void qtk_hash_list_del(qtk_hash_list_t* hash, qtk_hash_elem_t* elem)
{
	//kaldi
	elem->tail = hash->freed_head;
	hash->freed_head = elem;
	//===================
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

//####################for speed and mem. optimization2 by dmd#########################
qtk_hash_list_t* qtk_hash_list_new2(int size)
{
	int i;
	qtk_hash_list_t* hash = (qtk_hash_list_t*) wtk_malloc(
			sizeof(qtk_hash_list_t));
	//qtk_hash_bucket_t * bucket;

	hash->size = size;
	hash->list_head = NULL;
	hash->list_tail = NULL;
	hash->bucket_list_tail = -1;
	hash->freed_head = NULL;
//	hash->freed_tail = NULL;
	//hash->slot=(wtk_queue_t**)wtk_calloc(nslot,sizeof(wtk_queue_t*));
	hash->bucket_q=(qtk_hash_bucket_t**)wtk_malloc(hash->size*sizeof(qtk_hash_bucket_t));
	//wtk_queue_init(&(hash->bucket_q));
	hash->heap = wtk_heap_new(4096);
	hash->bucket_q_bak=(qtk_hash_bucket_t*)wtk_malloc(hash->size*sizeof(qtk_hash_bucket_t));
	for (i = 0; i < size; i++)
	{
		hash->bucket_q_bak[i].prev_bucket=-1;
		hash->bucket_q_bak[i].last_elem=NULL;
	}
	memcpy(hash->bucket_q, hash->bucket_q_bak, size* sizeof(qtk_hash_bucket_t));

	return hash;
}

void qtk_hash_list_reset2(qtk_hash_list_t* hash)
{
	//int i;
	//qtk_hash_bucket_t * bucket;

	wtk_heap_reset(hash->heap);
	hash->list_head = NULL;
	hash->list_tail = NULL;
	hash->bucket_list_tail = -1;
	hash->freed_head = NULL;
	//wtk_queue_init(&(hash->bucket_q));
	memcpy(hash->bucket_q, hash->bucket_q_bak, hash->size*sizeof(qtk_hash_bucket_t));
}


void qtk_hash_list_delete2(qtk_hash_list_t* hash)
{
	wtk_free(hash->bucket_q);
	wtk_free(hash->bucket_q_bak);
	wtk_heap_delete(hash->heap);
	wtk_free(hash);
}

qtk_hash_elem_t* qtk_hash_elem_new2(qtk_hash_list_t* hash, int size)
{
	qtk_hash_elem_t* elem;
	int i;

	elem=(qtk_hash_elem_t*)wtk_heap_malloc(hash->heap,size * sizeof(qtk_hash_elem_t));
	memset(elem,0, size * sizeof(qtk_hash_elem_t));
	for(i=0; i<size-1; i++)
		elem[i].tail = &(elem[i+1]);

	return elem;
}

qtk_hash_elem_t*  qtk_hash_list_new_elem2(qtk_hash_list_t* hash, int size)
{
	qtk_hash_elem_t *elem;
	if(!hash->freed_head)
	{
		hash->freed_head = qtk_hash_elem_new2(hash, size);
		//hash->freed_head = qtk_hash_elem_new(hash);
	}
	elem = hash->freed_head;
	hash->freed_head = elem->tail;

	return elem;
}

int  qtk_hash_list_Insert2(qtk_hash_list_t* hash, int state_id,wtk_fst_state_t* state,
		qtk_kwfstdec_token_t* token)
{
	//wtk_debug("%d\n",hash->size);
	int ret,index = state_id % (hash->size);
//	wtk_debug("insert state:%d to bucket:%d\n",state,index);
	qtk_hash_bucket_t *bucket,*last_bucket;
	qtk_hash_elem_t* elem;

	//qn=wtk_queue_peek(&(hash->bucket_q),index);
	//if(!qn){ret=-1;goto end;}
//	int k;
//	for(k=0;k<1000;k++)
//	{
//		wtk_debug("%p\n",hash->bucket_q[k]);
//	}

	bucket=&(((qtk_hash_bucket_t*)hash->bucket_q)[index]);//data_offset2(qn,qtk_hash_bucket_t,q_n);
	//elem=qtk_hash_elem_new(hash);
	elem=qtk_hash_list_new_elem(hash);
	//elem=qtk_hash_list_new_elem2(hash, 100);
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
			last_bucket=&(((qtk_hash_bucket_t*)hash->bucket_q)[hash->bucket_list_tail]);//data_offset2(qn1,qtk_hash_bucket_t,q_n);
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

qtk_hash_elem_t* qtk_hash_list_clear2(qtk_hash_list_t* hash)
{
	qtk_hash_elem_t *elem;
//	int i;
//	for(qn=hash->bucket_q.pop;qn;qn=qn->next)
//	{
//		bucket=data_offset(qn,qtk_hash_bucket_t,q_n);
//		bucket->last_elem=NULL;//TODO maybe searching form bucket_list_tail is faster?
//	}
//	wtk_debug("%p\n",hash->list_head);
	memcpy(hash->bucket_q, hash->bucket_q_bak, hash->size*sizeof(qtk_hash_bucket_t));
	hash->bucket_list_tail=-1;
	elem=hash->list_head;
//	wtk_debug("%p\n",hash->list_head);
	hash->list_head=NULL;

	return elem;
}

/*
 * one time free whole list
 * Note: it's not improve speed. but save mem.
 */
void qtk_hash_list_free(qtk_hash_list_t* hash, qtk_hash_elem_t *head, qtk_hash_elem_t *tail)
{
	if(hash->freed_head && tail)
	{
		tail->tail = hash->freed_head;
	}
	hash->freed_head = head;
}

//####################for speed and mem optimization3 by dmd#########################
qtk_hash_list_t* qtk_hash_list_new3(int size)
{
	qtk_hash_list_t* hash = (qtk_hash_list_t*) wtk_malloc(
				sizeof(qtk_hash_list_t));
	hash->heap=wtk_heap_new(4096*100);
	hash->size=0;
	hash->list_head = NULL;
	hash->freed_head = NULL;

	return hash;
}

void qtk_hash_list_reset3(qtk_hash_list_t* hash)
{
	wtk_heap_reset(hash->heap);
	hash->list_head = NULL;
	hash->list_tail = NULL;
	hash->freed_head = NULL;
}

void qtk_hash_list_delete3(qtk_hash_list_t* hash)
{
	wtk_heap_delete(hash->heap);
	wtk_free(hash);
}

qtk_hash_elem_t*  qtk_hash_list_new_elem3(qtk_hash_list_t* hash, int size)
{
	qtk_hash_elem_t *elem;
	int i;

	if(NULL == hash->freed_head)
	{
		elem=(qtk_hash_elem_t*)wtk_heap_malloc(hash->heap,size * sizeof(qtk_hash_elem_t));  //100 is a good test value.
		//memset(elem,0, size * sizeof(qtk_hash_elem_t));
		for(i=0; i<size-1; i++)
			elem[i].tail = &(elem[i+1]);
		elem[i].tail=NULL;
		hash->freed_head=elem;
	}
	elem = hash->freed_head;
	hash->freed_head = elem->tail;

	return elem;
}

void qtk_hash_list_Insert3(qtk_hash_list_t* hash, int state_id,wtk_fst_state_t* state,
		qtk_kwfstdec_token_t* token)
{
	qtk_hash_elem_t* elem;

//	elem=qtk_hash_list_new_elem(hash);
	elem=qtk_hash_list_new_elem3(hash,100);
	elem->key=state_id;
	elem->token=token;
	elem->state=state;
	elem->tail=NULL;

	if (NULL == hash->list_head)
	{
		hash->list_head=elem;
		hash->list_tail=elem;
		//elem->tail = NULL;
	}else
	{
		elem->tail = hash->list_head;
		hash->list_head = elem;

//		hash->list_tail->tail = elem;
//		hash->list_tail = elem;
	}
}

qtk_hash_elem_t* qtk_hash_list_clear3(qtk_hash_list_t* hash)
{
	qtk_hash_elem_t *elem;

	elem=hash->list_head;
	hash->list_head=NULL;
	hash->list_tail=NULL;

	return elem;
}

///////////////////other
int qtk_hash_list_bytes(qtk_hash_list_t* hash)
{
	int bytes;

	bytes=sizeof(qtk_hash_list_t);
	bytes=hash->size*sizeof(qtk_hash_bucket_t);
	bytes+=wtk_heap_bytes(hash->heap);

	return bytes;
}
