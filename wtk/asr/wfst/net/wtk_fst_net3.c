#include <math.h>
#include <stdlib.h>
#ifdef WIN32
#include <float.h>
#endif
#include "wtk_fst_net3.h"
#include "wtk/asr/wfst/rec/wtk_wfstr.h"
#include "wtk/core/wtk_larray.h"
void wtk_fst_net3_print_prev(wtk_fst_net3_t *net,wtk_fst_node_t *node);

int wtk_fst_node_is_eof(wtk_fst_node_t *n)
{
	//return n->like!=-FLT_MAX;
	return n->eof;
}

//#define USE_NET_MAC
#ifdef USE_NET_MAC
//#define wtk_fst_node_add_nxt_arc(n,arc) wtk_queue2_push(&((n)->next_q),&((arc)->next_n))
//#define wtk_fst_node_set_eof(n,eofv) (n)->eof=eofv

#else
void wtk_fst_node_set_eof(wtk_fst_node_t *n,int eof)
{
	n->eof=eof;
}

void wtk_fst_node_add_nxt_arc(wtk_fst_node_t *n,wtk_fst_arc_t *arc)
{
	wtk_queue2_push(&(n->next_q),&(arc->next_n));
}
#endif

wtk_fst_net3_t* wtk_fst_net3_new(wtk_fst_net3_cfg_t *cfg,wtk_fst_net_cfg_t *net_cfg)
{
	wtk_fst_net3_t *n;

	n=(wtk_fst_net3_t*)wtk_malloc(sizeof(wtk_fst_net3_t));
	n->cfg=cfg;
	n->net_cfg=net_cfg;
	n->cnt=0;
	if(cfg->use_hg)
	{
		n->hg=wtk_prune_new(&(cfg->hg));
	}else
	{
		n->hg=NULL;
	}
	n->node_pool=wtk_vpool_new(sizeof(wtk_fst_node_t),cfg->node_cache);
	n->arc_pool=wtk_vpool_new(sizeof(wtk_fst_arc_t),cfg->trans_cache);

	n->lat_inst_pool=wtk_vpool_new(sizeof(wtk_fst_lat_inst_t),cfg->inst_cache);

	n->heap=wtk_heap_new(4096);

	//wtk_debug("use wrd align: %d\n",cfg->use_wrd_align);
	if(cfg->use_wrd_align)
	{
		n->output_inst_q=wtk_calloc(net_cfg->sym_out->nstrs,sizeof(wtk_fst_lat_inst_t));
	}else
	{
		n->output_inst_q=0;
	}
	if(cfg->use_hot_word)
	{
		n->outsym_hash=wtk_str_hash_new(100000);
	}else
	{
		n->outsym_hash=NULL;
	}

	wtk_fst_net3_reset(n);
	return n;
}

void wtk_fst_net3_delete(wtk_fst_net3_t *n)
{
	if(n->hg)
	{
		wtk_prune_delete(n->hg);
	}
	wtk_vpool_delete(n->lat_inst_pool);
	if(n->output_inst_q)
	{
		wtk_free(n->output_inst_q);
	}
	if(n->outsym_hash)
	{
		wtk_str_hash_delete(n->outsym_hash);
	}
	wtk_heap_delete(n->heap);
	wtk_vpool_delete(n->node_pool);
	wtk_vpool_delete(n->arc_pool);
	wtk_free(n);
}

int wtk_fst_net3_bytes(wtk_fst_net3_t *n)
{
	int bytes;

	bytes=0;
	bytes+=wtk_heap_bytes(n->heap);
	bytes+=wtk_vpool_bytes(n->node_pool);
	bytes+=wtk_vpool_bytes(n->arc_pool);
	bytes+=wtk_vpool_bytes(n->lat_inst_pool);
	bytes+=n->net_cfg->sym_out->nstrs*sizeof(wtk_fst_lat_inst_t);
	return bytes;
}

void wtk_fst_net3_reset(wtk_fst_net3_t *n)
{
	//wtk_debug("cal times:%d\n",n->cnt);
	n->cnt=0;
	if(n->hg)
	{
		wtk_prune_reset(n->hg);
	}
	if(n->output_inst_q)
	{
		memset(n->output_inst_q,0,sizeof(wtk_fst_lat_inst_t)*n->net_cfg->sym_out->nstrs);
	}
	if(n->outsym_hash)
	{
		wtk_str_hash_reset(n->outsym_hash);
	}
	wtk_vpool_reset(n->lat_inst_pool);

	wtk_heap_reset(n->heap);
	wtk_slist_init(&(n->list_q));
	wtk_queue_init(&(n->active_node_q));
	wtk_vpool_reset(n->node_pool);
	wtk_vpool_reset(n->arc_pool);
	n->start=NULL;
	n->end=NULL;
	n->max_like=-FLT_MAX;
	n->end_ac_like=-FLT_MAX;
	n->end_lm_like=-FLT_MAX;
	n->eof=0;
	n->end_state=NULL;
	n->null_node=NULL;
	n->null_prev_node=NULL;
}

wtk_fst_node_t* wtk_fst_net3_pop_node(wtk_fst_net3_t *n,unsigned int frame,wtk_fst_trans_t *trans,wtk_fst_state_t *state)
{
	wtk_fst_node_t *node;

	node=(wtk_fst_node_t*)wtk_vpool_pop(n->node_pool);
	node->state=state;
	wtk_queue2_init(&(node->next_q));
	wtk_queue2_init(&(node->prev_q));
#ifdef USE_LAT_FST_PATH
	wtk_queue2_init(&(node->path_q));
#endif
	node->frame=frame;
	node->hook.p=NULL;
	node->ac_like=0;//-FLT_MAX;
	node->lm_like=0;//-FLT_MAX;
	node->collect=0;
	node->touch=0;
	node->leaf=0;
	node->inst=0;

	int depth=n->cfg->depth;
	if(depth>1)
	{
		node->pre=(wtk_fst_arc_t**)wtk_heap_malloc(n->heap,depth*sizeof(wtk_fst_arc_t*));
		int x;
		for(x=0;x<depth;x++)
		{	
			node->pre[x]=NULL;
		}
	}

	if(state && state->type==WTK_FST_FINAL_STATE)
	{
		node->arc_like=state->v.weight;
		wtk_fst_node_set_eof(node,1);
	}else
	{
		node->arc_like=-FLT_MAX;
		wtk_fst_node_set_eof(node,0);
	}
	wtk_queue_push_front(&(n->active_node_q),&(node->q_n));
	return node;
}

wtk_fst_node_t* wtk_fst_net3_pop_node2(wtk_fst_net3_t *n,unsigned int frame,wtk_fst_trans_t *trans,wtk_fst_state_t *state,int num)
{
	wtk_fst_node_t *node;

	node=(wtk_fst_node_t*)wtk_vpool_pop(n->node_pool);
	node->state=state;
	wtk_queue2_init(&(node->next_q));
	wtk_queue2_init(&(node->prev_q));
#ifdef USE_LAT_FST_PATH
	wtk_queue2_init(&(node->path_q));
#endif
        int depth=n->cfg->depth;
        if(depth>1)
        {
                node->pre=(wtk_fst_arc_t**)wtk_heap_malloc(n->heap,depth*sizeof(wtk_fst_arc_t*));
		int x;
		for(x=0;x<depth;x++)
		{	
			node->pre[x]=NULL;
		}
        }

	node->frame=frame;
	node->hook.p=NULL;
	node->ac_like=0;//-FLT_MAX;
	node->lm_like=0;//-FLT_MAX;
	node->collect=0;
	node->touch=0;
	node->leaf=0;
	node->inst=0;
	node->num=num;
	if(state && state->type==WTK_FST_FINAL_STATE)
	{
		node->arc_like=state->v.weight;
		wtk_fst_node_set_eof(node,1);
	}else
	{	
		node->arc_like=-FLT_MAX;
		wtk_fst_node_set_eof(node,0);
	}
	wtk_queue_push_front(&(n->active_node_q),&(node->q_n));
	return node;
}

void wtk_fst_net3_push_node(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_slist_node_t *sn;
	wtk_slist_node_t *prev;
	wtk_fst_node_item_t *item;

	if(n->leaf)
	{
		prev=NULL;
		for(sn=net->list_q.prev;sn;sn=sn->prev)
		{
			item=data_offset2(sn,wtk_fst_node_item_t,list_n);
			if(item->node==n)
			{
				if(prev)
				{
					prev->prev=sn->prev;
				}else
				{
					net->list_q.prev=sn->prev;
				}
				break;
			}
			prev=sn;
		}
	}
	n->state=0;
	wtk_queue_remove(&(net->active_node_q),&(n->q_n));
	wtk_vpool_push(net->node_pool,n);
}


#ifdef USE_LAT_FST_PATH
void wtk_fst_net3_remove_path(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_queue2_t *q;
	wtk_queue_node_t *qn;
	wtk_wfst_path_t *pth;

	q=&(n->path_q);
	while(1)
	{
		qn=wtk_queue2_pop(q);
		if(!qn){break;}
		pth=data_offset2(qn,wtk_wfst_path_t,lat_n);
		pth->hook=NULL;
	}
}
#endif

void wtk_fst_net3_remove_node(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_queue_node_t *qn;
	wtk_queue2_t *q=&(n->prev_q);
	wtk_fst_arc_t *arc;
	wtk_fst_lat_inst_t *inst;

	if(n==net->end)
	{
		//wtk_debug("v[%d] remove end\n",net->last_frame);
		net->end=NULL;
	}
	if(n==net->null_node)
	{
		//wtk_debug("remove null\n");
		net->null_node=NULL;
	}
	if(n==net->null_prev_node)
	{
		net->null_prev_node=NULL;
	}
	while(1)
	{
		qn=wtk_queue2_pop(q);
		if(!qn){break;}
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		if(arc->from_node)
		{
			wtk_fst_net3_remove_arc_from_src_node(net,arc->from_node,arc);
		}
		if(arc->from_node && !arc->from_node->next_q.pop && !wtk_fst_node_is_eof(arc->from_node))
		{
			wtk_fst_net3_remove_node(net,arc->from_node);
		}
		wtk_fst_net3_push_arc(net,arc);
	}
	q=&(n->next_q);
	while(1)
	{
		qn=wtk_queue2_pop(q);
		if(!qn){break;}
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc->to_node)
		{
			wtk_fst_net3_remove_arc_from_dst_node(net,arc->to_node,arc);
		}
		if(arc->to_node && !arc->to_node->prev_q.pop)
		{
			wtk_fst_net3_remove_node(net,arc->to_node);
		}
		wtk_fst_net3_push_arc(net,arc);
	}
#ifdef USE_LAT_FST_PATH
	wtk_fst_net3_remove_path(net,n);
#endif
	if(n->state && n->inst)
	{
		//inst=(wtk_fst_lat_inst_t*)n->state->hook;
		inst=n->inst;
		wtk_queue2_remove(&(inst->list_q),&(n->inst_n));
	}
	wtk_fst_net3_push_node(net,n);
}

void wtk_fst_net3_remove_arc_from_src_node(wtk_fst_net3_t *net,wtk_fst_node_t *n,wtk_fst_arc_t *arc)
{
	wtk_queue2_remove(&(n->next_q),&(arc->next_n));
	//arc->next_n.prev=arc->next_n.next=0;
}

void wtk_fst_net3_remove_arc_from_dst_node(wtk_fst_net3_t *net,wtk_fst_node_t *n,wtk_fst_arc_t *arc)
{
	wtk_queue2_remove(&(n->prev_q),&(arc->prev_n));
	//arc->prev_n.prev=arc->prev_n.next=0;
}

void wtk_fst_net3_unlink_arc(wtk_fst_net3_t *net,wtk_fst_arc_t *arc)
{
	if(arc->from_node)
	{
		wtk_fst_net3_remove_arc_from_src_node(net,arc->from_node,arc);
	}
	wtk_fst_net3_remove_arc_from_dst_node(net,arc->to_node,arc);
	//wtk_debug("node=%p,prev=%p\n",arc->to_node,arc->to_node->prev);
}


void wtk_fst_net_remove_node_from_prev2(wtk_fst_net3_t *net,wtk_fst_node_t *node)
{
	wtk_queue_node_t *n,*n2;
	wtk_fst_arc_t *arc;


	for(n=node->prev_q.pop;n;n=n2)
	{
		n2=n->next;
		arc=data_offset2(n,wtk_fst_arc_t,prev_n);
		wtk_fst_net3_remove_arc_from_src_node(net,arc->from_node,arc);
		wtk_fst_net3_push_arc(net,arc);
	}
}

void wtk_fst_net_remove_node_from_prev(wtk_fst_net3_t *net,wtk_fst_node_t *node)
{
	wtk_queue_node_t *n;
	wtk_fst_arc_t *arc;
	wtk_queue2_t *q=&(node->prev_q);

	while(1)
	{
		n=wtk_queue2_pop(q);
		if(!n){break;}
		arc=data_offset2(n,wtk_fst_arc_t,prev_n);
		wtk_fst_net3_remove_arc_from_src_node(net,arc->from_node,arc);
		wtk_fst_net3_push_arc(net,arc);
	}
}

int wtk_fst_node_nxt_arcs(wtk_fst_node_t *n)
{
	wtk_queue_node_t *qn;
	int i;

	for(i=0,qn=n->next_q.pop;qn;qn=qn->next)
	{
		++i;
	}
	return i;
}

int wtk_fst_node_pre_arcs(wtk_fst_node_t *n)
{
	wtk_queue_node_t *qn;
	int i;

	for(i=0,qn=n->prev_q.pop;qn;qn=qn->next)
	{
		++i;
	}
	return i;
}

void wtk_fst_net_cfg_print_path(wtk_fst_net_cfg_t *cfg,wtk_wfst_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_string_t *v;

	//wtk_debug("id=%d\n",pth->trans->out_id);
	if(pth->prev)
	{
		wtk_fst_net_cfg_print_path(cfg,pth->prev,buf);
	}
	if(pth->trans)// && pth->trans->out_id>0)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		/*
		v=cfg->sym_in->ids[pth->trans->in_id]->str;
		wtk_strbuf_push(buf,v->data,v->len);
		wtk_strbuf_push_c(buf,':');
		*/
		v=cfg->sym_out->strs[pth->trans->out_id];
		wtk_strbuf_push(buf,v->data,v->len);
		wtk_strbuf_push_f(buf,"[f=%d hook=%p id=%d]",pth->frame,pth->hook,pth->trans->out_id);
	}
}

void wtk_fst_net3_print_path(wtk_fst_net3_t *n,wtk_wfst_path_t *pth)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_strbuf_reset(buf);
	wtk_fst_net_cfg_print_path(n->net_cfg,pth,buf);
	wtk_debug("path:[%.*s]\n",buf->pos,buf->data);
	wtk_strbuf_delete(buf);
}

void wtk_fst_net3_push_arc(wtk_fst_net3_t *net,wtk_fst_arc_t *arc)
{
#ifdef USE_LAT_FST_PATH
#else
	if(arc->pth)
	{
		arc->pth->hook=NULL;
	}
#endif
	arc->from_node=arc->to_node=0;
	wtk_vpool_push(net->arc_pool,arc);
}

/**
 * 检查in_id;
 */
int wtk_fst_net3_is_arc_in(wtk_fst_net3_t *net,wtk_fst_node_t *s,
		wtk_fst_node_t *to_node,
		unsigned int input_id,unsigned int output_id,
		float arc_like,unsigned int frame)
{
	wtk_fst_arc_t *ax;
	wtk_queue_node_t *qn;
	int id2;

	for(qn=s->next_q.pop;qn;qn=qn->next)
	{
		ax=data_offset2(qn,wtk_fst_arc_t,next_n);
		id2=ax->trans?ax->trans->in_id:0;
		if(id2!=0 || ax->out_id!=0){return 0;}
		if(ax->to_node==to_node && id2==input_id && ax->out_id==output_id
				&& ax->frame==frame && ax->arc_like==arc_like)
		{
			//wtk_debug("frame=%d,cnt=%d\n",frame,cnt);
			return 1;
		}
	}
	return 0;
}

wtk_fst_arc_t* wtk_fst_net3_found_arc(wtk_fst_net3_t *net,wtk_fst_node_t *s,
		wtk_fst_node_t *to_node,
		unsigned int input_id,unsigned int output_id,
		float arc_like,unsigned int frame)
{
	wtk_fst_arc_t *ax;
	wtk_queue_node_t *qn;
	int id2;

	for(qn=s->next_q.pop;qn;qn=qn->next)
	{
		ax=data_offset2(qn,wtk_fst_arc_t,next_n);
		id2=ax->trans?ax->trans->in_id:0;
		if(id2!=0 || ax->out_id!=0){return 0;}
		if(ax->to_node==to_node && id2==input_id && ax->out_id==output_id
				&& ax->frame==frame && ax->arc_like==arc_like)
		{
			//wtk_debug("frame=%d,cnt=%d\n",frame,cnt);
			return ax;
		}
	}
	return NULL;
}

void wtk_fst_arc_link_path(wtk_fst_arc_t *arc,wtk_wfst_path_t *pth)
{
#ifdef USE_LAT_FST_PATH
	pth->hook=arc->to_node;
	wtk_queue2_push(&(arc->to_node->path_q),&(pth->lat_n));
#else
	pth->hook=arc;
	arc->pth=pth;
	//wtk_debug("pth=%p arc=%p %p->%p\n",pth,arc,arc->from_node,arc->to_node);
#endif
}

void wtk_fst_arc_unlink_path(wtk_fst_arc_t *arc,wtk_wfst_path_t *pth)
{
#ifdef USE_LAT_FST_PATH
	pth->hook=NULL;
	wtk_queue2_remove(&(arc->to_node->path_q),&(pth->lat_n));
#else
	pth->hook=NULL;
	arc->pth=NULL;
#endif
}

/*
wtk_fst_node_t* wtk_fst_path_get_lat_node(wtk_fst_path_t *pth)
{
#ifdef USE_LAT_FST_PATH
	return (wtk_fst_node_t*)(pth->hook);
#else
	return ((pth)->hook)?((wtk_fst_arc_t*)(pth->hook))->to_node:NULL;
#endif
}*/


wtk_fst_arc_t*	wtk_fst_net3_pop_arc(wtk_fst_net3_t *n,
		wtk_fst_node_t *s,wtk_fst_node_t *e,
		wtk_fst_trans_t *trans,
		float arc_like,float ac_like,float lm_like,
		unsigned int frame)
{
	wtk_fst_arc_t* arc;

	arc=(wtk_fst_arc_t*)wtk_vpool_pop(n->arc_pool);
	arc->trans=trans;
	if(trans)
	{
		//arc->in_id=trans->in_id;
		arc->out_id=trans->out_id;
	}else
	{
		//arc->in_id=0;
		arc->out_id=0;
	}
	arc->frame=frame;
	arc->ac_like=ac_like;
	arc->lm_like=lm_like;
	arc->arc_like=arc_like;

	arc->from_node=s;
	arc->to_node=e;
	if(s)
	{
#ifdef SORT_TRANS
		if(!trans || (trans->in_id==0 && trans->out_id==0))// && trans->out_id==0))
		{
			wtk_queue2_push_front(&(s->next_q),&(arc->next_n));
		}else
		{
			wtk_queue2_push(&(s->next_q),&(arc->next_n));
		}
#else
		wtk_queue2_push_front(&(s->next_q),&(arc->next_n));
#endif
	}else
	{
		arc->next_n.next=arc->next_n.prev=0;
	}
	wtk_queue2_push_front(&(e->prev_q),&(arc->prev_n));
	arc->hook=0;
	arc->pth=NULL;
	return arc;
}

wtk_fst_arc_t*	wtk_fst_net3_pop_arc2(wtk_fst_net3_t *n,
		wtk_fst_node_t *s,wtk_fst_node_t *e,int in,
		int out,
		float arc_like,float ac_like,float lm_like,
		unsigned int frame,int is_hot)
{
	wtk_fst_arc_t* arc;

	arc=(wtk_fst_arc_t*)wtk_vpool_pop(n->arc_pool);
	arc->trans=NULL;
	//if(trans)
	//{
		arc->in_id=in;
		arc->out_id=out;
	//}else
	//{
		//arc->in_id=0;
	//	arc->out_id=0;
	//}
	arc->frame=frame;
	arc->ac_like=ac_like;
	arc->lm_like=lm_like;
	arc->arc_like=arc_like;
	arc->hot=is_hot;

	arc->from_node=s;
	arc->to_node=e;
	if(s)
	{
#ifdef SORT_TRANS
		if(!trans || (trans->in_id==0 && trans->out_id==0))// && trans->out_id==0))
		{
			wtk_queue2_push_front(&(s->next_q),&(arc->next_n));
		}else
		{
			wtk_queue2_push(&(s->next_q),&(arc->next_n));
		}
#else
		wtk_queue2_push_front(&(s->next_q),&(arc->next_n));
#endif
	}else
	{
		arc->next_n.next=arc->next_n.prev=0;
	}
	wtk_queue2_push_front(&(e->prev_q),&(arc->prev_n));
	arc->hook=0;
	arc->pth=NULL;
	return arc;
}


wtk_fst_arc_t* wtk_fst_net3_add_link_arc(wtk_fst_net3_t *net,wtk_fst_node_t *root,wtk_fst_node_t *to,
		unsigned int out_id,float like,float arc_like,float ac_like,float lm_like,unsigned int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc3,*arc4,*arc5;
	float f1;
	//int cnt=0;

	arc3=NULL;
	for(qn=root->next_q.pop;qn;qn=qn->next)
	{
		//++cnt;
		arc4=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc4->to_node==to && arc4->out_id==out_id && arc4->frame==frame)
		{
			arc3=arc4;
			break;
		}
	}
	//wtk_debug("cnt=%d\n",cnt);
	if(arc3)
	{
		arc5=NULL;
		f1=arc_like;
		if(f1>arc3->arc_like)
		{
			arc3->arc_like=f1;
			arc3->ac_like=ac_like;
			arc3->lm_like=lm_like;
		}
	}else
	{
		//wtk_debug("arc4[%d]=%p prev=%p next=%p,root=%p n=%p\n",i,NULL,prev,n->next,root,n);
		arc5=wtk_fst_net3_pop_arc(net,root,to,NULL,arc_like,ac_like,lm_like,frame);
		arc5->out_id=out_id;
	}
	return arc5;
}


wtk_fst_arc_t* wtk_fst_net3_add_eps_link_arc(wtk_fst_net3_t *net,wtk_fst_node_t *root,wtk_fst_arc_t *arc1,
		float arc_like,float ac_like,float lm_like)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc3,*arc4,*arc5;
	float f1;

	arc3=NULL;
	if(net->cfg->use_wrd_cmp)
	{
		//int cnt=0;

		for(qn=root->next_q.pop;qn;qn=qn->next)
		{
			//++cnt;
			arc4=data_offset2(qn,wtk_fst_arc_t,next_n);
			if(arc4->to_node==arc1->to_node && arc4->out_id==arc1->out_id
						&& arc4->frame==arc1->frame)
			{
				arc3=arc4;
				break;
			}
		}
		//wtk_debug("cnt=%d\n",cnt);
	}else
	{
		for(qn=root->next_q.pop;qn;qn=qn->next)
		{
			arc4=data_offset2(qn,wtk_fst_arc_t,next_n);
			if(arc4->to_node==arc1->to_node && arc4->trans==arc1->trans //arc4->out_id==arc1->out_id
					&& arc4->frame==arc1->frame)
			{
				arc3=arc4;
				break;
			}
		}
	}
	if(arc3)
	{
		arc5=NULL;
		f1=arc1->arc_like+arc_like;
		if(f1>arc3->arc_like)
		{
			arc3->arc_like=f1;
			arc3->ac_like=arc1->ac_like+ac_like;
			arc3->lm_like=arc1->lm_like+lm_like;
		}
	}else
	{
	//	arc5=wtk_fst_net3_pop_arc(net,root,arc1->to_node,arc1->trans,
	//		arc1->arc_like+arc_like,
	//		arc1->ac_like+ac_like,
	//		arc1->lm_like+lm_like,
	//		arc1->frame);
	                arc5=wtk_fst_net3_pop_arc2(net,root,arc1->to_node,arc1->in_id,arc1->out_id,
                        arc1->arc_like+arc_like,
                        arc1->ac_like+ac_like,
                        arc1->lm_like+lm_like,
                        arc1->frame,0);

	}
	return arc5;
}

void wtk_fst_net3_add_eof_node(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_fst_node_item_t *item;

	if(!n->leaf)
	{
		wtk_fst_node_set_eof(n,1);
		n->leaf=1;
		item=(wtk_fst_node_item_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_node_item_t));
		item->node=n;
		wtk_slist_push(&(net->list_q),&(item->list_n));
	}
}

void wtk_fst_net3_update_final_state(wtk_fst_net3_t *net,wtk_fst_arc_t *arc)
{
	wtk_fst_node_item_t *item;
	wtk_fst_node_t *n1,*n2;
	float f;

	n1=arc->from_node;
	n2=arc->to_node;
	if(!wtk_fst_node_is_eof(n1))
	{
		/*
		wtk_debug("add arc[%d/%d]=[%.*s:%.*s] nxt=%d arc=%p %p=>%p\n",ki,net->last_frame,
				net->net_cfg->sym_in->ids[arc->trans?arc->trans->in_id:0]->str->len,
				net->net_cfg->sym_in->ids[arc->trans?arc->trans->in_id:0]->str->data,
				net->net_cfg->sym_out->strs[arc->out_id]->len,
				net->net_cfg->sym_out->strs[arc->out_id]->data,wtk_queue2_len(&(arc->to_node->next_q)),
				arc,arc->from_node,arc->to_node);
		*/
		wtk_fst_node_set_eof(n1,1);
		item=(wtk_fst_node_item_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_node_item_t));
		item->node=n1;
		n1->leaf=1;
		wtk_slist_push(&(net->list_q),&(item->list_n));
	}
	f=wtk_fst_times(arc->arc_like,n2->arc_like);
	if(f>n1->arc_like)
	{
		n1->ac_like=wtk_fst_times(n2->ac_like,arc->ac_like);
		n1->lm_like=wtk_fst_times(n2->lm_like,arc->lm_like);
		n1->arc_like=wtk_fst_times(n2->arc_like,arc->arc_like);
	}
}

int wtk_fst_net3_eps_end_node(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_fst_arc_t *arc;

	while(!n->eof)
	{
		if(n->next_q.pop)
		{
			arc=data_offset2(n->next_q.pop,wtk_fst_arc_t,next_n);
			if(arc->out_id>0){return 0;}
			n=arc->to_node;
		}else
		{
			return 0;
		}
	}
	return n->eof;
}

/*
 * Weighted Automata Algorithms
	Mehryar Mohri

 */
void wtk_fst_net3_remove_eps_node(wtk_fst_net3_t *net,
		wtk_fst_node_t *root,wtk_fst_arc_t *arc,
		float arc_like,float ac_like,float lm_like)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc1,*arc3;
	wtk_fst_node_t *n;
	wtk_queue2_t q;

	if(arc)
	{
		n=arc->to_node;
	}else
	{
		n=root;
	}
	if(!net->eof)
	{
		//如果是周期剪枝，对于word节点到虚终节点中的节点不合并;
		if(wtk_fst_net3_eps_end_node(net,n))
		{
			//wtk_debug("found end\n");
			//wtk_fst_net3_print_next_arc(net,n,100);
			//exit(0);
			return;
		}
	}
	if(n->eof && !n->leaf)
	{
		wtk_fst_net3_add_eof_node(net,n);
		//wtk_debug("n=%p eof=%d leaf=%d\n",n,n->eof,n->leaf);
	}
	//wtk_debug("v[%d]=%p\n",ki,n);
	q=n->next_q;
	wtk_queue2_init(&(n->next_q));
	while(1)
	{
		qn=wtk_queue2_pop(&(q));
		if(!qn){break;}
		arc1=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc1->out_id==0)
		{
			//remove arc;
			if(!net->eof)
			{
				if(wtk_fst_net3_eps_end_node(net,n))
				{
					wtk_fst_node_add_nxt_arc(n,arc1);
					//wtk_queue2_push(&(n->next_q),&(arc1->next_n));
					continue;
				}
			}
			//wtk_debug("arc=%p[%d] %p->%p\n",arc1,arc1->out_id,arc1->from_node,arc1->to_node);
			if(!arc1->to_node->collect)
			{
				wtk_fst_net3_remove_eps_node(net,arc1->to_node,NULL,0.0,0.0,0.0);
			}
			if(wtk_fst_node_is_eof(arc1->to_node))
			{
				wtk_fst_net3_update_final_state(net,arc1);
			}
			wtk_fst_net3_remove_arc_from_dst_node(net,arc1->to_node,arc1);
			for(qn=arc1->to_node->next_q.pop;qn;qn=qn->next)
			{
				arc3=data_offset2(qn,wtk_fst_arc_t,next_n);
				if(arc3->out_id==0 && net->eof)
				{
					//wtk_fst_net3_print_prev_arc(net,arc1->to_node,100);
					//wtk_fst_net3_print_next_arc(net,arc1->to_node,100);
					wtk_debug("found bug arc=%p[%d:%d/%f] %p[frame=%d,eof=%d]-> %p[frame=%d,eof=%d]\n",
							arc3,arc3->trans?arc3->trans->in_id:-1,arc3->out_id,arc3->arc_like,
							arc3->from_node,arc3->from_node->frame,arc3->from_node->eof,
							arc3->to_node,arc3->to_node->frame,arc3->to_node->eof);
					exit(0);
				}
				wtk_fst_net3_add_eps_link_arc(net,n,arc3,
						arc1->arc_like,
						arc1->ac_like,
						arc1->lm_like
						);
				if(root!=n)
				{
					wtk_fst_net3_add_eps_link_arc(net,root,arc3,
						arc1->arc_like+arc_like,
						arc1->ac_like+ac_like,
						arc1->lm_like+lm_like
						);
				}
			}
#ifdef DEBUG_XEPS
			if(ki==2461)
			{
				wtk_debug("to prev=%d nxt=%d\n",wtk_queue2_len(&(arc1->to_node->prev_q)),
						wtk_queue2_len(&(arc1->to_node->next_q)));
			}
#endif
			if(!arc1->to_node->prev_q.pop)// && arc1->to_node->collect==1)
			{
				wtk_fst_net3_remove_node(net,arc1->to_node);
			}
			wtk_fst_net3_push_arc(net,arc1);
		}else
		{
			//wtk_queue2_push(&(n->next_q),&(arc1->next_n));
			wtk_fst_node_add_nxt_arc(n,arc1);
		}
	}
	if(n!=root && wtk_fst_node_is_eof(n))// && !n->next_q.pop)
	{
		wtk_fst_net3_update_final_state(net,arc);
	}
}


void wtk_fst_net3_remove_eps(wtk_fst_net3_t *net)
{
	wtk_fst_node_t *fn;
	wtk_queue_node_t *n;
	wtk_queue_t *q=&(net->active_node_q);

	//wtk_debug("len=%d\n",q->length);
	while(q->length>0)
	{
		n=wtk_queue_pop(q);
		fn=data_offset2(n,wtk_fst_node_t,q_n);
		//wtk_debug("v[%d]=%p c=%d\n",i,fn,fn->collect);
		if(fn->collect==1)
		{
			wtk_queue_push(q,n);
			break;
		}
		fn->collect=1;
		wtk_queue_push(q,n);
		wtk_fst_net3_remove_eps_node(net,fn,NULL,0.0,0.0,0.0);
		if(!fn->next_q.pop && !wtk_fst_node_is_eof(fn))
		{
			wtk_fst_net3_remove_node(net,fn);
		}
	}
	for(n=q->pop;n;n=n->next)
	{
		fn=data_offset2(n,wtk_fst_node_t,q_n);
		//wtk_debug("v[%d]=%p c=%d\n",i,fn,fn->collect);
		fn->collect=0;
		//wtk_fst_net3_check_node(net,fn);
	}
}


void wtk_fst_net3_prune_path(wtk_fst_net3_t *net)
{
	wtk_fst_node_item_t *item;
	wtk_slist_node_t *n;
	wtk_queue_node_t *qn;
	wtk_fst_node_t *node,*nil;
	wtk_fst_arc_t *arc;
	//float max_like;
	wtk_fst_state_t *end_state=net->end_state;
#ifdef USE_LAT_FST_PATH
	wtk_queue_node_t *qn2;
	wtk_wfst_path_t *pth;
#endif

	//max_like=-FLT_MAX;
	if(!net->list_q.prev)
	{
		wtk_debug("warning: end node not found.\n");
		return;
	}
	node=(wtk_fst_node_t*)wtk_fst_net3_pop_node(net,0,0,end_state);
	node->frame=net->last_frame;
	for(n=net->list_q.prev;n;n=n->prev)
	{
		item=data_offset2(n,wtk_fst_node_item_t,list_n);
		while(1)
		{
			qn=wtk_queue2_pop(&(item->node->prev_q));
			if(!qn){break;}
			arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
#ifdef USE_LAT_FST_PATH
			tn=arc->to_node;
#endif
			arc->ac_like+=item->node->ac_like;
			arc->lm_like+=item->node->lm_like;
			arc->arc_like+=item->node->arc_like;
#ifdef USE_LAT_FST_PATH
			while(1)
			{
				qn2=wtk_queue2_pop(&(tn->path_q));
				if(!qn2){break;}
				pth=data_offset(qn2,wtk_wfst_path_t,lat_n);
				pth->hook=node;
				wtk_queue2_push(&(node->path_q),&(pth->lat_n));
			}
#endif
			arc->to_node=node;
			//wtk_fst_net3_print_arc(net,arc);
			wtk_queue2_push(&(node->prev_q),&(arc->prev_n));
		}
		wtk_fst_node_set_eof(item->node,0);
	}
	wtk_fst_node_set_eof(node,0);
	net->null_prev_node=node;

	nil=(wtk_fst_node_t*)wtk_fst_net3_pop_node(net,0,0,end_state);
	arc=wtk_fst_net3_pop_arc(net,node,nil,NULL,0,0,0,node->frame);
	wtk_fst_node_set_eof(nil,1);
	net->null_node=nil;

	node=(wtk_fst_node_t*)wtk_fst_net3_pop_node(net,0,0,0);
	arc=wtk_fst_net3_pop_arc(net,node,net->start,NULL,0,0,0,0);
	//wtk_debug("start=%p node=%p\n",net->start,node);
	net->start=node;

}


float wtk_fst_plus2(float f1,float f2)
{
	return f1<f2?f2:f1;
}

float wtk_fst_net3_single_source(wtk_fst_net3_t *net,wtk_fst_node_t *s,wtk_queue_t *nodes)
{
	float p[4]={-FLT_MAX,-FLT_MAX,0,-FLT_MAX};
	wtk_heap_t *heap=net->heap;
	wtk_queue_node_t *n;
	wtk_queue_node_t *qn;
	wtk_fst_node_t *fn;
	wtk_queue_item_t *item;
	wtk_queue_t q;
	float *fp;
	float r1;
	wtk_fst_arc_t *arc;
	float f1;//,f2;

	//-0 =+inf -1=0
	//d=>0 r=>1
	for(n=nodes->pop;n;n=n->next)
	{
		fn=data_offset2(n,wtk_fst_node_t,q_n);
		memcpy(fn->hook.p,p,sizeof(float)*3);
	}
	fp=s->hook.p;
	fp[0]=0;fp[1]=0;
	fp[2]=1;
	wtk_queue_init(&(q));
	item=(wtk_queue_item_t*)wtk_heap_malloc(heap,sizeof(wtk_queue_item_t));
	item->hook=s;
	wtk_queue_push(&(q),&(item->q_n));
	while(q.length>0)
	{
		qn=wtk_queue_pop(&(q));
		item=data_offset2(qn,wtk_queue_item_t,q_n);
		fn=(wtk_fst_node_t*)item->hook;
		fp=(float*)fn->hook.p;
		fp[2]=0;
		r1=fp[1];
		fp[1]=-FLT_MAX;
		for(qn=fn->next_q.pop;qn;qn=qn->next)
		{
			arc=data_offset2(qn,wtk_fst_arc_t,next_n);
			f1=wtk_fst_times(r1,arc->arc_like);
			fp=(float*)arc->to_node->hook.p;
			if(!fp)
			{
				wtk_debug("arc=%p from=%p to=%p\n",arc,arc->from_node,arc->to_node);
				exit(0);
			}
			fp[0]=wtk_fst_plus2(fp[0],f1);
			fp[1]=wtk_fst_plus2(fp[1],f1);
			if(fp[2]==0)
			{
				item=(wtk_queue_item_t*)wtk_heap_malloc(heap,sizeof(wtk_queue_item_t));
				item->hook=arc->to_node;
				fp[2]=1;
				wtk_queue_push(&(q),&(item->q_n));
			}
		}
	}
	fp=(float*)net->null_node->hook.p;
	return fp[0];
}

void wtk_fst_net3_prune_node(wtk_fst_net3_t *net,wtk_fst_node_t *root,float beam,int hg_nodes)
{
	wtk_heap_t *heap=net->heap;
	wtk_prune_t *hg=net->hg;
	wtk_queue_node_t *n1;
	wtk_queue_t *q=&(net->active_node_q);
	wtk_fst_node_t *fn;
	float like,best,f,thresh;
	float *fp;
	int i=0;
	int count;

	for(n1=q->pop;n1;n1=n1->next)
	{
		fn=data_offset2(n1,wtk_fst_node_t,q_n);
		fn->collect=0;
		fn->hook.p=(float*)wtk_heap_malloc(heap,sizeof(float)*5);
		//wtk_debug("v[%d]: fn=%p,%d:%d\n",cnt,fn,fn->frame,fn->state->id);
	}
	best=wtk_fst_net3_single_source(net,net->start,q);
	for(n1=q->pop;n1;n1=n1->next)
	{
		fn=data_offset2(n1,wtk_fst_node_t,q_n);
		fp=(float*)fn->hook.p;
		fp[3]=fp[0];
	}
	if(hg)
	{
		wtk_prune_reset(hg);
	}
	//thresh=best*net->cfg->prune_thresh;
	thresh=best-beam;//net->cfg->prune_beam;
	//wtk_debug("best=%f thresh=%f\n",best,thresh);
	//wtk_debug("s=%p e=%p n=%p\n",net->start,net->end,net->null_node);
	while(1)
	{
		//wtk_debug("len=%d\n",q->length);
		n1=wtk_queue_pop(q);
		fn=data_offset2(n1,wtk_fst_node_t,q_n);
		//wtk_debug("collect=%p c=%d t=%d\n",fn,fn->collect,fn->touch);
		if(fn->collect==1)// || fn->touch==1)
		{
			wtk_queue_push(q,n1);
			break;
		}
		fn->collect=1;
		wtk_queue_push(q,n1);
		//wtk_debug("check fn=%p/%p c=%d t=%d\n",fn,net->start,fn->collect,fn->touch);
		if(fn!=net->start && !fn->touch)
		{
			++i;
			like=wtk_fst_net3_single_source(net,fn,q);
			fp=(float*)fn->hook.p;
			f=fp[3]+like;
			//wtk_debug("f=%f/%f/%f\n",f,thresh,like);
			//wtk_debug("v[%d]=%d %f/%f/%f\n",i,i,f,thresh,best);
			if(f<thresh)
			{
				wtk_fst_net3_remove_node(net,fn);
			}else
			{
				if(hg)
				{
					f-=best;
					fp[4]=f;
					//wtk_debug("v[%d]=%d %f/%f/%f/%f\n",i,i,f,thresh,best,f);
					wtk_prune_add(hg,f);
				}
				fn->touch=1;
			}
		}
	}
	//wtk_debug("pid=%d net=%p frame=%d nodes=%d %f\n",(int)getpid(),net,net->last_frame,q->length,q->length*1.0/net->last_frame);
	if(hg && (q->length>net->cfg->hg_min_node_thresh||hg_nodes>0))
	{
		if(hg_nodes<=0)
		{
			if(net->cfg->use_fix_nodes)
			{
				count=net->cfg->hg_fix_nodes;
			}else
			{
				count=net->cfg->hg_nodes_per_frame*net->last_frame+net->cfg->hg_nodes_per_frame_b;
				if(count<=0)
				{
					count=net->cfg->hg_min_node_thresh;
				}
			}
		}else
		{
			count=hg_nodes;
		}
		//wtk_debug("count=%d %f/%d\n",count,net->cfg->hg_nodes_per_frame,net->last_frame);
		thresh=wtk_prune_get_thresh2(hg,count);
		//thresh=hg->thresh;
		while(1)
		{
			//wtk_debug("len=%d\n",q->length);
			n1=wtk_queue_pop(q);
			fn=data_offset2(n1,wtk_fst_node_t,q_n);
			//wtk_debug("collect=%p c=%d t=%d\n",fn,fn->collect,fn->touch);
			if(fn->collect==0)// || fn->touch==1)
			{
				wtk_queue_push(q,n1);
				break;
			}
			fn->collect=0;
			wtk_queue_push(q,n1);
			if(fn!=net->start && fn!=net->null_node)// && !fn->touch)
			{
				fp=(float*)fn->hook.p;
				if(fp[4]<thresh)
				{
					wtk_fst_net3_remove_node(net,fn);
				}
			}
		}
		//wtk_debug("n=%d\n",q->length);
		//wtk_debug("[%f]\n",hg->thresh);
	}
	//wtk_debug("len=[%d]\n",q->length);
	//exit(0);
	//wtk_debug("s=%p e=%p n=%p\n",net->start,net->end,net->null_node);
}

void wtk_fst_net3_update_prune_eof(wtk_fst_net3_t *net)
{
	wtk_queue_node_t *n;
	wtk_fst_node_t *fn;
	wtk_queue_t *q=&(net->active_node_q);

	for(n=q->pop;n;n=n->next)
	{
		fn=data_offset2(n,wtk_fst_node_t,q_n);
		wtk_fst_node_set_eof(fn,0);
		fn->collect=0;
		fn->leaf=0;

		//fn->arc_like=-FLT_MAX;
		fn->ac_like=-FLT_MAX;
		fn->lm_like=-FLT_MAX;
		if(fn->state && fn->state->type==WTK_FST_FINAL_STATE)
		{
			fn->arc_like=fn->state->v.weight;
		}else
		{
			fn->arc_like=-FLT_MAX;
		}
	}
}

void wtk_fst_net3_update_eof(wtk_fst_net3_t *net)
{
	wtk_queue_node_t *n;
	wtk_fst_node_t *fn;
	wtk_queue_t *q=&(net->active_node_q);

	for(n=q->pop;n;n=n->next)
	{
		fn=data_offset2(n,wtk_fst_node_t,q_n);
		//wtk_debug("v[%d]=%p\n",i,fn);
		fn->collect=0;
		fn->touch=0;

		fn->leaf=0;
		if(fn->state && fn->state->type==WTK_FST_FINAL_STATE)
		{
			if(!fn->eof)
			{
				wtk_fst_node_set_eof(fn,1);
			}
			//wtk_debug("set eof=%p:%d\n",fn,fn->eof);
		}else if(fn->eof)
		{
			wtk_fst_node_set_eof(fn,0);
		}
	}
	net->eof=1;
}


void wtk_fst_net3_collect_node2(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	wtk_queue2_t *q;

	n->collect=1;
	if(!n->next_q.pop)
	{
		if(!wtk_fst_node_is_eof(n))
		{
			wtk_fst_net3_remove_node(net,n);
		}
		return;
	}
	q=&(n->next_q);
	while(1)
	{
		qn=wtk_queue2_pop(q);
		if(!qn){break;}
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc->hook)
		{
			wtk_queue2_push(q,qn);
			break;
		}
		arc->hook=(void*)1;
		wtk_queue2_push(q,qn);
		if(!arc->to_node->collect)
		{
			wtk_fst_net3_collect_node2(net,arc->to_node);
		}
	}
	for(qn=q->pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		arc->hook=NULL;
	}
}

void wtk_fst_net3_collect_node(wtk_fst_net3_t *net,wtk_fst_node_t *fn)
{
	wtk_queue_node_t *n;
	wtk_queue_t *q=&(net->active_node_q);

	wtk_fst_net3_collect_node2(net,fn);
	while(q->length>0)
	{
		n=wtk_queue_pop(q);
		fn=data_offset2(n,wtk_fst_node_t,q_n);
		if(!fn)
		{
			wtk_debug("found bug[len=%d]\n",q->length);
			//exit(0);
			return;
		}
		if(fn->collect==0)
		{
			wtk_queue_push(q,n);
			break;
		}
		fn->collect=0;
		wtk_queue_push(q,n);
		fn->hook.p=NULL;
		if(!fn->next_q.pop && !wtk_fst_node_is_eof(fn))
		{
			wtk_fst_net3_remove_node(net,fn);
		}
	}
}


void wtk_fst_net3_finish(wtk_fst_net3_t *net)
{
	wtk_slist_init(&(net->list_q));
	if(net->active_node_q.length==0){return;}
	wtk_fst_net3_collect_node(net,net->start);
	if(net->active_node_q.length==0){return;}
	wtk_fst_net3_remove_eps(net);
	if(net->active_node_q.length==0){return;}
	wtk_fst_net3_prune_path(net);
	if(net->active_node_q.length==0 || !net->null_node){return;}
	if(net->cfg->prune_end_beam>0)
	{
		wtk_fst_net3_prune_node(net,net->start,net->cfg->prune_end_beam,-1);
	}
}


void wtk_fst_lat_inst_reset(wtk_fst_lat_inst_t *inst)
{
	wtk_queue2_init(&(inst->list_q));
}

wtk_fst_lat_inst_t* wtk_fst_net3_pop_lat_inst(wtk_fst_net3_t *n)
{
	wtk_fst_lat_inst_t *inst;

	inst=(wtk_fst_lat_inst_t*)wtk_vpool_pop(n->lat_inst_pool);
	wtk_fst_lat_inst_reset(inst);
	return inst;
}


void wtk_fst_net3_push_lat_inst(wtk_fst_net3_t *n,wtk_fst_lat_inst_t *inst)
{
	wtk_vpool_push(n->lat_inst_pool,inst);
}

wtk_fst_lat_inst_t* wtk_fst_net3_get_trans_inst(wtk_fst_net3_t *net,wtk_fst_trans_t *trans)
{
	wtk_fst_lat_inst_t *inst;

	if(net->output_inst_q && trans->out_id>0)
	{
		inst=net->output_inst_q+trans->out_id;
	}else
	{
		if(!trans->to_state->hook)
		{
			trans->to_state->hook=inst=wtk_fst_net3_pop_lat_inst(net);
		}else
		{
			inst=(wtk_fst_lat_inst_t*)trans->to_state->hook;
		}
	}
	return inst;
}

wtk_fst_lat_inst_t* wtk_fst_net3_get_trans_inst2(wtk_fst_net3_t *net,wtk_fst_trans_t *trans)
{
	wtk_fst_lat_inst_t *inst;

	if(!trans->to_state->hook)
	{
		trans->to_state->hook=inst=wtk_fst_net3_pop_lat_inst(net);
	}else
	{
		inst=(wtk_fst_lat_inst_t*)trans->to_state->hook;
	}
	return inst;
}



void wtk_fst_net3_print_lat_inst2(wtk_fst_net3_t *net,wtk_fst_lat_inst_t *inst)
{
	wtk_queue_node_t *qn;
	wtk_fst_node_t *fn;
	int i;

	i=0;
	wtk_debug("====================== lat inst =======================\n");
	for(qn=inst->list_q.pop;qn;qn=qn->next)
	{
		++i;
		fn=data_offset2(qn,wtk_fst_node_t,inst_n);
		wtk_debug("v[%d]: %p:%d\n",i,fn,fn->frame);
	}
	wtk_debug("=========================================================\n");
}

void wtk_fst_lat_inst_check_frame(wtk_fst_lat_inst_t *inst,int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_node_t *node;

	for(qn=inst->list_q.pop;qn;qn=qn->next)
	{
		node=data_offset2(qn,wtk_fst_node_t,inst_n);
		if(node->frame==frame)
		{
			wtk_debug("found frame=%d\n",frame);
			exit(0);
		}
	}
}

wtk_fst_node_t* wtk_fst_net3_get_trans_node(wtk_fst_net3_t *net,wtk_fst_trans_t *trans,unsigned int frame)
{
	wtk_fst_lat_inst_t *inst;
	wtk_fst_node_t *node;

	inst=wtk_fst_net3_get_trans_inst(net,trans);
	if(inst->list_q.pop)
	{
		node=data_offset2(inst->list_q.pop,wtk_fst_node_t,inst_n);
		if(node->frame!=frame)
		{
			//wtk_fst_lat_inst_check_frame(inst,frame);
			node=wtk_fst_net3_pop_node(net,frame,trans,trans->to_state);
			node->inst=inst;
			wtk_queue2_push_front(&(inst->list_q),&(node->inst_n));
		}
	}else
	{
		node=wtk_fst_net3_pop_node(net,frame,trans,trans->to_state);
		node->inst=inst;
		wtk_queue2_push_front(&(inst->list_q),&(node->inst_n));
	}
	//wtk_queue_check(&(net->active_node_q));
	return node;
}


typedef struct
{
	int n;
	int l;
}wtk_fst_net3_info_t;

int wtk_fst_node_sort_out_arc(wtk_fst_node_t *n)
{
	wtk_queue2_t q;
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int i;

	wtk_queue2_init(&(q));
	i=0;
	while(1)
	{
		qn=wtk_queue2_pop(&(n->next_q));
		if(!qn){break;}
		++i;
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc->out_id==0)
		{
			wtk_queue2_push_front(&(q),&(arc->next_n));
		}else
		{
			wtk_queue2_push(&(q),&(arc->next_n));
		}
	}
	n->next_q=q;
	return i;
}


wtk_fst_net3_info_t wtk_fst_net3_nodes(wtk_fst_net3_t *net,wtk_fst_node_t *node,wtk_larray_t *a)
{
	wtk_queue_node_t *n1,*n2;
	wtk_fst_node_t *fn;
	wtk_fst_arc_t *arc;
	wtk_fst_arc_item_t *item;
	wtk_fst_net3_info_t info;

	info.n=0;
	info.l=0;
	for(n1=net->active_node_q.pop;n1;n1=n1->next)
	{
		fn=data_offset2(n1,wtk_fst_node_t,q_n);
		wtk_fst_node_sort_out_arc(fn);
		for(n2=fn->next_q.pop;n2;n2=n2->next)
		{
			arc=data_offset2(n2,wtk_fst_arc_t,next_n);
			item=(wtk_fst_arc_item_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_arc_item_t));
			item->net_cfg=net->net_cfg;
			item->arc=arc;
			arc->hook=item;
			++info.n;
			//wtk_debug("item=%p\n",item);
			wtk_larray_push2(a,&(item));
			info.l+=wtk_fst_node_nxt_arcs(arc->to_node);
		}
	}
	return info;
}

float wtk_fst_net3_cmp_arc(wtk_fst_net3_t *net,wtk_fst_arc_item_t **src,wtk_fst_arc_item_t **dst)
{
	wtk_fst_arc_item_t *item,*item2;

	item=*src;
	item2=*dst;
	//wtk_debug("item=%p/%p %p/%p\n",item,item2,src,dst);
	if(item2->arc->frame>item->arc->frame)
	{
		return -1;
	}else if(item2->arc->frame==item->arc->frame)
	{
		if(item2->arc->out_id==net->net_cfg->eps_id)
		{
			if(item->arc->out_id==net->net_cfg->snt_end_id)
			{
				return -1;
			}
		}
		if(item->arc->out_id==net->net_cfg->eps_id)
		{
			if(item2->arc->out_id==net->net_cfg->snt_start_id)
			{
				return -1;
			}
		}
		if(item2->arc->out_id>item->arc->out_id)
		{
			return -1;
		}
	}
	return 1;
}

int wtk_fst_net3_cmp_arc2(const void *p1,const void *p2)
{
	wtk_fst_arc_item_t **src=(wtk_fst_arc_item_t**)p1;
	wtk_fst_arc_item_t **dst=(wtk_fst_arc_item_t**)p2;
	wtk_fst_arc_item_t *item,*item2;
	wtk_fst_net_cfg_t *net_cfg=(*src)->net_cfg;

	item=*src;
	item2=*dst;
	//wtk_debug("item=%p/%p %p/%p\n",item,item2,src,dst);
	if(item2->arc->frame>item->arc->frame)
	{
		return -1;
	}else if(item2->arc->frame==item->arc->frame)
	{
		if(item2->arc->out_id==net_cfg->eps_id)
		{
			if(item->arc->out_id==net_cfg->snt_end_id)
			{
				return -1;
			}
		}
		if(item->arc->out_id==net_cfg->eps_id)
		{
			if(item2->arc->out_id==net_cfg->snt_start_id)
			{
				return -1;
			}
		}
		if(item2->arc->out_id>item->arc->out_id)
		{
			return -1;
		}
	}
	return 1;
}

typedef struct
{
	wtk_array_t *arcs;
	unsigned int out_id;
	unsigned int frame;
	int index;
}wtk_fst_net3_merge_item_t;

wtk_fst_net3_merge_item_t* wtk_fst_net3_new_merge_item(wtk_fst_net3_t *net,unsigned int out_id,unsigned int frame,int index)
{
	wtk_heap_t *heap=net->heap;
	wtk_fst_net3_merge_item_t *item;

	item=(wtk_fst_net3_merge_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_net3_merge_item_t));
	item->arcs=wtk_array_new_h(heap,20,sizeof(wtk_fst_arc_t*));
	item->out_id=out_id;
	item->frame=frame;
	item->index=index;
	//wtk_array_t* wtk_array_new_h(wtk_heap_t* h,uint32_t n,uint32_t size)
	return item;
}

void wtk_fst_net3_merge_item_add_arc(wtk_fst_net3_merge_item_t *item,wtk_fst_arc_t *arc)
{
	wtk_array_push2(item->arcs,&(arc));
}

wtk_larray_t* wtk_fst_net3_merge_arc(wtk_fst_net3_t *net,wtk_fst_arc_item_t **pitem,int n)
{
	wtk_fst_arc_item_t *item;
	wtk_fst_net3_merge_item_t *mi=NULL;
	unsigned int last_id=0;
	unsigned int last_frame=0;
	int i;
	wtk_larray_t *a;
	int index=0;

	a=wtk_larray_new(n,sizeof(wtk_fst_net3_merge_item_t*));
	for(i=0;i<n;++i)
	{
		item=pitem[i];
		if(i==0)
		{
			item->index=index;
			last_id=item->arc->out_id;
			last_frame=item->arc->frame;
			mi=wtk_fst_net3_new_merge_item(net,last_id,last_frame,index);
			wtk_fst_net3_merge_item_add_arc(mi,item->arc);
			wtk_larray_push2(a,&(mi));
		}else
		{
			if(last_id!=item->arc->out_id || last_frame!=item->arc->frame )
			{
				++index;
				item->index=index;
				last_id=item->arc->out_id;
				last_frame=item->arc->frame;
				mi=wtk_fst_net3_new_merge_item(net,last_id,last_frame,index);
				wtk_fst_net3_merge_item_add_arc(mi,item->arc);
				wtk_larray_push2(a,&(mi));
			}else
			{
				item->index=index;
				wtk_fst_net3_merge_item_add_arc(mi,item->arc);
			}
		}
	}
	return a;
}

void wtk_fst_net3_print_lat(wtk_fst_net3_t *net,FILE *log)
{
	wtk_fst_net3_merge_item_t **pmi;
	wtk_fst_net3_merge_item_t *mi;
	wtk_fst_arc_t **arcs;
	int j;
	wtk_fst_net3_info_t info;
	wtk_queue_node_t *qn;
	wtk_fst_arc_item_t **pi;
	wtk_fst_arc_item_t *item=NULL;
	wtk_fst_arc_item_t *item2;
	wtk_string_t null=wtk_string("!NULL");
	wtk_string_t *v;
	wtk_fst_arc_t *arc=NULL;
	int index=0;
	wtk_larray_t *a,*b;
	int i;

	a=wtk_larray_new(1024,sizeof(wtk_fst_arc_item_t*));
	info=wtk_fst_net3_nodes(net,net->start,a);//&list);
	//wtk_qsort2(a->slot,a->nslot,sizeof(wtk_fst_arc_item_t*),(wtk_qsort_cmp_f)wtk_fst_net3_cmp_arc,net);
	qsort(a->slot,a->nslot,sizeof(wtk_fst_arc_item_t*),wtk_fst_net3_cmp_arc2);
	pi=(wtk_fst_arc_item_t**)a->slot;
	b=wtk_fst_net3_merge_arc(net,pi,a->nslot);
	wtk_larray_delete(a);
	a=b;
	fprintf(log,"VERSION=1.0\n");
	fprintf(log,"lmscale=%.2f wdpenalty=%.2f prscale=1.00 acscale=1.00\n",net->net_cfg->lmscale,
			net->net_cfg->wordpen);
	fprintf(log,"N=%d L=%d\n",a->nslot,info.l);

	pmi=(wtk_fst_net3_merge_item_t**)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		mi=pmi[i];
		if(mi->out_id==0)// && item->arc->to_node==net->null_node)
		{
			v=&(null);
		}else
		{
			v=net->net_cfg->sym_out->strs[mi->out_id];
		}
#ifndef USE_X
		fprintf(log,"I=%d t=%.2f W=%.*s v=1\n",mi->index,mi->frame*0.01,v->len,v->data);
#else
		fprintf(log,"I=%d t=%.2f W=%.*s v=1 arc=%p %p=>%p\n",item->index,item->arc->frame*0.01,v->len,v->data,
				arc,arc->from_node,arc->to_node);
#endif
	}
	index=0;
	for(i=0;i<a->nslot;++i)
	{
		mi=pmi[i];
		arcs=(wtk_fst_arc_t**)mi->arcs->slot;
		for(j=0;j<mi->arcs->nslot;++j)
		{
			arc=arcs[j];
			item=(wtk_fst_arc_item_t*)arc->hook;
			for(qn=arc->to_node->next_q.pop;qn;qn=qn->next)
			{
				arc=data_offset2(qn,wtk_fst_arc_t,next_n);
				item2=(wtk_fst_arc_item_t*)arc->hook;
				fprintf(log,"J=%d S=%d E=%d a=%f l=%f\n",index,item->index,item2->index,
						arc->ac_like,arc->lm_like/net->net_cfg->lmscale);
				++index;
			}
		}

	}
	wtk_larray_delete(a);
}

void wtk_fst_net3_write_lat(wtk_fst_net3_t *net,char *fn)
{
	FILE *f;

	f=fopen(fn,"w");
	wtk_fst_net3_print_lat(net,f);
	fclose(f);
}


void wtk_fst_net3_print_lat2(wtk_fst_net3_t *net,FILE *log)
{
	wtk_fst_net3_info_t info;
	wtk_queue_node_t *qn;
	wtk_fst_arc_item_t **pi;
	wtk_fst_arc_item_t *item;
	wtk_fst_arc_item_t *item2;
	wtk_string_t null=wtk_string("!NULL");
	wtk_string_t *v;
	wtk_fst_arc_t *arc;
	int index=0;
	wtk_larray_t *a;
	int i;

	wtk_fst_net3_clean_hook(net);
	a=wtk_larray_new(1024,sizeof(wtk_fst_arc_item_t*));
	info=wtk_fst_net3_nodes(net,net->start,a);//&list);
	//wtk_qsort2(a->slot,a->nslot,sizeof(wtk_fst_arc_item_t*),(wtk_qsort_cmp_f)wtk_fst_net3_cmp_arc,net);
	qsort(a->slot,a->nslot,sizeof(wtk_fst_arc_item_t*),wtk_fst_net3_cmp_arc2);
	fprintf(log,"VERSION=1.0\n");
	fprintf(log,"lmscale=%.2f wdpenalty=%.2f prscale=1.00 acscale=1.00\n",net->net_cfg->lmscale,
			net->net_cfg->wordpen);
	fprintf(log,"N=%d L=%d\n",info.n,info.l);
	//wtk_debug("N=%d,L=%d\n",info.n,info.l);
	pi=(wtk_fst_arc_item_t**)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		item=pi[i];
		if(item->arc->out_id==0)// && item->arc->to_node==net->null_node)
		{
			v=&(null);
		}else
		{
			v=net->net_cfg->sym_out->strs[item->arc->out_id];
		}
		//wtk_debug("[%.*s]\n",v->len,v->data);
		item->index=index;
		++index;
		arc=item->arc;
#ifndef USE_X
		fprintf(log,"I=%d t=%.2f W=%.*s\n",item->index,item->arc->frame*0.01,v->len,v->data);
#else
		fprintf(log,"I=%d t=%.2f W=%.*s v=1 arc=%p %p=>%p\n",item->index,item->arc->frame*0.01,v->len,v->data,
				arc,arc->from_node,arc->to_node);
#endif
	}
	//wtk_debug("index=%d\n",index);
	index=0;
	for(i=0;i<a->nslot;++i)
	{
		item=pi[i];
		for(qn=item->arc->to_node->next_q.pop;qn;qn=qn->next)
		{
			arc=data_offset2(qn,wtk_fst_arc_t,next_n);
			item2=(wtk_fst_arc_item_t*)arc->hook;
			if(net->net_cfg->lmscale)
			{
				fprintf(log,"J=%d S=%d E=%d a=%f l=%f\n",index,item->index,item2->index,
					arc->ac_like,arc->lm_like/net->net_cfg->lmscale);
			}else
			{
				fprintf(log,"J=%d S=%d E=%d a=%f\n",index,item->index,item2->index,
					arc->ac_like);
			}
			++index;
		}
	}
	wtk_larray_delete(a);
	wtk_fst_net3_clean_hook(net);
}

void wtk_fst_net3_write_lat2(wtk_fst_net3_t *net,char *fn)
{
	FILE *f;

	wtk_debug("write lat [%s]\n",fn);
	f=fopen(fn,"w");
	wtk_fst_net3_print_lat2(net,f);//stdout);
	fclose(f);
	//wtk_debug("write close\n");
	//exit(0);
}


void wtk_fst_net_state_check_trans(wtk_fst_state_t *s,wtk_fst_state_t *e,
		unsigned int in_id,unsigned int out_id,float weight)
{
	wtk_fst_trans_t *trans;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->to_state==e && trans->in_id==in_id && trans->out_id==out_id
				&& trans->weight==weight)
		{
			wtk_debug("[%d:%d]=%f\n",in_id,out_id,weight);
			exit(0);
		}
	}
}

int wtk_fst_net3_to_net2(wtk_fst_net3_t *net,wtk_fst_net2_t *net2)
{
	//wtk_fst_net3_print(net);
	wtk_heap_t *heap=net2->heap;
	wtk_queue_t *q=&(net->active_node_q);
	wtk_queue_node_t *qn,*n2;
	wtk_fst_state2_t *states,*s,*e;
	wtk_fst_node_t *fn;
	wtk_fst_arc_t *arc;
	wtk_fst_trans_t *trans;
	wtk_fst_trans2_t *t2;
	int i,len,cnt,j;

	//wtk_debug("s=%p e=%p n=%p len=%d\n",net->start,net->end,net->null_node,q->length);
	if(!net->start || !net->null_node ||q->length==0){return -1;}
	net2->start=0;
	states=(wtk_fst_state2_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_state2_t)*q->length);
	for(i=0,j=1,qn=q->pop;qn;qn=qn->next,++i)
	{
		fn=data_offset2(qn,wtk_fst_node_t,q_n);
		len=wtk_fst_node_sort_out_arc(fn);
		s=states+i;
		s->type=WTK_FST_NORM_STATE;
		s->v.trans=0;
		s->ntrans=len;
		s->hook=NULL;
		s->in_prev=NULL;
		if(fn==net->start)
		{
			s->id=0;
		}else
		{
			s->id=j;
			++j;
		}
		fn->hook.p=s;
	}
	//wtk_debug("%p:%p %p:%p\n",net->start,net->start->hook.p,net->null_node,net->null_node->hook.p);
	net2->start=(wtk_fst_state2_t*)net->start->hook.p;
	//wtk_debug("hell end:%p\n",net->null_node->hook.p);
	net2->end=(wtk_fst_state2_t*)net->null_node->hook.p;
	net2->end->type=WTK_FST_FINAL_STATE;
	net2->end->v.weight=0;
	net2->state_id=j;
//	wtk_debug("[%d=>%d]\n",net2->start->id,net2->end->id);
	cnt=0;
	for(qn=q->pop;qn;qn=qn->next,++i)
	{
		fn=data_offset2(qn,wtk_fst_node_t,q_n);
		s=(wtk_fst_state2_t*)fn->hook.p;
		for(n2=fn->next_q.pop;n2;n2=n2->next)
		{
			arc=data_offset2(n2,wtk_fst_arc_t,next_n);
			t2=(wtk_fst_trans2_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_trans2_t));
			t2->from_state=(wtk_fst_state_t*)s;
			t2->frame=arc->frame;
			t2->hook2=NULL;
			trans=(wtk_fst_trans_t*)t2;
			trans->in_id=arc->in_id;
			trans->out_id=arc->out_id;
			//wtk_debug("%p\n",trans);
			//wtk_debug("%d \n",arc->out_id);
			if(arc->hot)
			{
			//	wtk_debug("trans hot set 1 %p\n",trans);
				trans->hot=1;
			}else
			{
				trans->hot=0;
			}
			//trans->weight=-2.5*arc->ac_like;
			//trans->weight=1.0*arc->ac_like;
			trans->weight=-1.0*arc->ac_like+arc->lm_like;

			//trans->weight+=arc->lm_like;///net->net_cfg->lmscale;
			//if(net->cfg->lm_scale!=0)
			//{
				//wtk_debug("%f/%f\n",arc->lm_like,net->cfg->lm_scale);
				//trans->weight+=arc->lm_like*net->cfg->lm_scale;///net->net_cfg->lmscale;
			//	trans->weight+=arc->lm_like;///net->net_cfg->lmscale;
			//}
			//trans->from_state=s;
			e=(wtk_fst_state2_t*)arc->to_node->hook.p;
			//wtk_debug("hell %p\n",e);
			trans->to_state=(wtk_fst_state_t*)e;
			trans->hook.next=s->v.trans;
			t2->in_prev=e->in_prev;
			e->in_prev=t2;

			t2->lm_like=0;
			s->v.trans=trans;
			++cnt;
//			wtk_debug("node id:%d %p %p \n",s->id,s->v.trans,s);
//			if(trans->out_id==1)
			//{
//				wtk_debug("%d to %d\n",s->id,e->id);
			//}
		}
	}
	net2->trans_id=cnt;
	//wtk_fst_net2_print(net2);
	//exit(0);
	//wtk_debug("%d/%d\n",net2->state_id,net2->trans_id);
	return 0;
}

//void wtk_fst_net3_add_extra_pth(wtk_fst_net3_t *net,wtk_queue_t class)
//{
//	wtk_queue_node_t *qn;
//}

wtk_fst_net3_combine_t* wtk_fst_net3_combine_new(wtk_fst_net3_t *net,wtk_fst_node_t *start,wtk_fst_node_t *end,char *data,int len,float lm_like,float ac_like)
{
	wtk_fst_net3_combine_t *combine;
	wtk_string_t *s;

	combine=(wtk_fst_net3_combine_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_net3_combine_t));
	combine->start=start;
	combine->end=end;

	s=(wtk_string_t*)wtk_heap_malloc(net->heap,len+sizeof(*s));
	s->len=len;
	if(len>0)
	{
		s->data=(char*)s+sizeof(*s);
	}else
	{
		s->data=0;
	}

	if(s && data)
	{
		memcpy(s->data,data,len);
	}
	combine->key=s;
	combine->lm_like=lm_like;
	combine->ac_like=ac_like;


	return combine;
}

void wtk_fst_net3_get_sym_combination_state(wtk_fst_net3_t *net,wtk_fst_node_t *node,float ax_like,float lmx_like)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc,*pre2;
	wtk_fst_node_t *cur_node;
	wtk_fst_net3_combine_t *combine;
	wtk_fst_sym_t *outsym=net->net_cfg->sym_out;
	wtk_strbuf_t *merge;
	int depth;
	float ac_like,lm_like;

	if(net->cnt>4500)
	{
		return;
	}
	
	merge=wtk_strbuf_new(1024,1);
	for(qn=node->next_q.pop;qn;qn=qn->next)
	{
		net->cnt++;
		depth=1;
		wtk_strbuf_reset(merge);
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc->out_id==1)
		{
			continue;
		}
		if(arc->out_id==0)
		{
			arc->to_node->pre=node->pre;
			ax_like+=arc->ac_like;
			lmx_like+=arc->lm_like;
		}else if(arc->out_id!=2)
		{
		lm_like=arc->lm_like;
		ac_like=arc->ac_like;
		cur_node=arc->to_node;
		//wtk_debug("%p from %d to %d ac_like:%f lm_like:%f\n",arc,arc->from_node->num,arc->to_node->num,arc->ac_like,arc->lm_like);
		if(!arc->hook2)
		{
			combine=wtk_fst_net3_combine_new(net,arc->from_node,arc->to_node,outsym->strs[arc->out_id]->data,outsym->strs[arc->out_id]->len,lm_like+lmx_like,ac_like+ac_like);
			//wtk_debug("add combie:%d %d %.*s\n",arc->from_node->num,arc->to_node->num,outsym->strs[arc->out_id]->len,outsym->strs[arc->out_id]->data);
			wtk_str_hash_add(net->outsym_hash,combine->key->data,combine->key->len,combine);
			//if(!cur_node->pre[0])
			//{
			//	cur_node->pre[0]=(wtk_fst_arc_t*)wtk_malloc(sizeof(wtk_fst_arc_t));
			//}
			cur_node->pre[0]=arc;
			//wtk_debug("arc %d\n",arc->out_id);
			arc->hook2=combine;
		}
		wtk_strbuf_push(merge,outsym->strs[arc->out_id]->data,outsym->strs[arc->out_id]->len);
	//	wtk_debug("%d %p %p %p\n",node->num,node->pre[0],node->pre[1],node->pre[2]);
	//	wtk_debug("%d %p %p %p\n",cur_node->num,cur_node->pre[0],cur_node->pre[1],cur_node->pre[2]);
		if(!cur_node->pre[1])
		{
			//pre=cur_node->pre[1];
			pre2=node->pre[0];
			while(pre2 && depth<net->cfg->depth)
			{
				//wtk_debug("%p\n",pre2);
				//wtk_debug("xxx %p %d to %d\n",pre2,pre2->from_node->num,pre2->to_node->num);
				wtk_strbuf_push_front(merge,outsym->strs[pre2->out_id]->data,outsym->strs[pre2->out_id]->len);
				lm_like+=pre2->lm_like;
				ac_like+=pre2->ac_like;
				combine=wtk_fst_net3_combine_new(net,pre2->from_node,arc->to_node,merge->data,merge->pos,lm_like+lmx_like,ac_like+ax_like);
				wtk_str_hash_add(net->outsym_hash,combine->key->data,combine->key->len,combine);
				//wtk_debug("add combie:%d %d %.*s\n",pre2->from_node->num,arc->to_node->num,merge->pos,merge->data);
				//if(!pre)
				//{
				//	pre=(wtk_fst_arc_t*)wtk_malloc(sizeof(wtk_fst_arc_t));
				//}
				//pre=pre2;
				cur_node->pre[depth]=node->pre[depth-1];
				depth++;
				pre2=node->pre[depth-1];
				//pre=cur_node->pre[depth];
			}
		}else
		{
			int is_continue=1;
			for(depth=1;depth<net->cfg->depth;depth++)
			{
				if(!node->pre[depth-1])
				{
					break;
				}

				pre2=node->pre[depth-1];
				//wtk_debug("xxx %p %d to %d\n",pre2,pre2->from_node->num,pre2->to_node->num);
				wtk_strbuf_push_front(merge,outsym->strs[pre2->out_id]->data,outsym->strs[pre2->out_id]->len);
				lm_like+=pre2->lm_like;
				ac_like+=pre2->ac_like;
				if(!cur_node->pre[depth])
				{
					is_continue=0;
					combine=wtk_fst_net3_combine_new(net,pre2->from_node,arc->to_node,merge->data,merge->pos,lm_like+lmx_like,ac_like+ax_like);
					wtk_str_hash_add(net->outsym_hash,combine->key->data,combine->key->len,combine);
					//wtk_debug("add combie:%d %d %.*s\n",pre2->from_node->num,arc->to_node->num,merge->pos,merge->data);
					//cur_node->pre[depth]=(wtk_fst_arc_t*)wtk_malloc(sizeof(wtk_fst_arc_t));
					cur_node->pre[depth]=pre2;
				}else
				{
					if(cur_node->pre[depth]!=node->pre[depth-1])
					{
						is_continue=0;
						combine=wtk_fst_net3_combine_new(net,pre2->from_node,arc->to_node,merge->data,merge->pos,lm_like+lmx_like,ac_like+ax_like);
						wtk_str_hash_add(net->outsym_hash,combine->key->data,combine->key->len,combine);
						//wtk_debug("add combie:%d %d %.*s\n",pre2->from_node->num,arc->to_node->num,merge->pos,merge->data);
						cur_node->pre[depth]=node->pre[depth-1];
					}
				}
			}
			if(is_continue)
			{
				//wtk_debug("continue\n");
				continue;
			}
		}
		}
		wtk_fst_net3_get_sym_combination_state(net,arc->to_node,ax_like,lmx_like);
	}
	wtk_strbuf_delete(merge);
}

void wtk_fst_net3_get_sym_combination(wtk_fst_net3_t *net)
{
	//wtk_fst_net3_get_sym_combination_state(net,net->start);
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;

	for(qn=net->start->next_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		//wtk_debug("%d %d\n",arc->from_node->num,arc->to_node->num);
		wtk_fst_net3_get_sym_combination_state(net,arc->to_node,0.0,0.0);
	}
}

void wtk_fst_net3_print_state(wtk_fst_net3_t *net,wtk_fst_node_t *node,wtk_strbuf_t *buf)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int pos;
	
	wtk_debug("%d %p\n",node->num,node);
	//wtk_debug("[%d:%f]\n",node->state->id,node->like);
	if(node->state && (node->arc_like!= -FLT_MAX || node->state->type==WTK_FST_FINAL_STATE || !node->next_q.pop))
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		pos=buf->pos;
		wtk_strbuf_push_f(buf,"[%f]",0.0);
		wtk_debug("%.*s [%f:%d:%p] ==>\n",buf->pos,buf->data,node->arc_like, node->state->type,node->next_q.pop);
		buf->pos=pos;
		//exit(0);
		//return;
	}
	//wtk_debug("trans=%p\n",state->v.trans);
	//wtk_debug("%d\n",wtk_queue2_len(&node->next_q));
	for(qn=node->next_q.pop;qn;qn=qn->next)
	{	
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		wtk_debug("from %d %p to %d %p\n",arc->from_node->num,arc->from_node,arc->to_node->num,arc->to_node);
		pos=buf->pos;
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		if(net->net_cfg && net->net_cfg->sym_out)
		{
#ifndef DEBUG_X
			wtk_strbuf_push_f(buf,"[%.*s:%d:%f:%f:%f,arc=%p,to=%p,%d=>%d]",
					net->net_cfg->sym_out->strs[arc->out_id]->len,
					net->net_cfg->sym_out->strs[arc->out_id]->data,
					arc->to_node->frame,
					arc->arc_like,arc->ac_like,arc->lm_like,arc,
					arc->to_node,arc->from_node->state?arc->from_node->state->id:-1,
					arc->to_node->state?arc->to_node->state->id:-1
					);
#else
			printf("[%.*s:%d:%f]\n",
					net->net_cfg->sym_out->strs[arc->out_id]->len,
					net->net_cfg->sym_out->strs[arc->out_id]->data,
					arc->to_node->frame,
					arc->ac_like
					);
#endif
		}else
		{
			wtk_strbuf_push_f(buf,"[%d:%f]",
					arc->out_id,
					arc->ac_like
					);
		}
		wtk_fst_net3_print_state(net,arc->to_node,buf);
		buf->pos=pos;
	}
}

void wtk_fst_net3_print(wtk_fst_net3_t *net)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_fst_net3_print_state(net,net->start,buf);
	wtk_strbuf_delete(buf);
}

void wtk_fst_net3_print_prev_state(wtk_fst_net3_t *net,wtk_fst_node_t *node,wtk_strbuf_t *buf)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int pos;

	if(!node)
	{
		printf("%.*s==>\n",buf->pos,buf->data);
		return ;
	}
	//wtk_debug("trans=%p\n",state->v.trans);
	for(qn=node->prev_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		pos=buf->pos;
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		if(net->net_cfg && net->net_cfg->sym_out)
		{
#ifndef DEBUG_xxxX
			//if(arc->out_id!=0)
			{
				wtk_strbuf_push_f(buf,"[%.*s:%.*s:f=%d a=%f l=%f s=%f]",
						net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->len,
						net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->data,
						net->net_cfg->sym_out->strs[arc->out_id]->len,
						net->net_cfg->sym_out->strs[arc->out_id]->data,
						arc->to_node->frame,arc->ac_like,arc->lm_like,arc->arc_like);
			}
#else
			printf("[%.*s:%d:%f]\n",
					net->net_cfg->sym_out->strs[arc->out_id]->len,
					net->net_cfg->sym_out->strs[arc->out_id]->data,
					arc->to_node->frame,
					arc->ac_like
					);
#endif
		}else
		{
			wtk_strbuf_push_f(buf,"[%d:%f]",
					arc->out_id,
					arc->ac_like
					);
		}
		wtk_fst_net3_print_prev_state(net,arc->from_node,buf);
		buf->pos=pos;
	}
}

void wtk_fst_net3_print_prev(wtk_fst_net3_t *net,wtk_fst_node_t *node)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_strbuf_push_s(buf,"<==");
	wtk_fst_net3_print_prev_state(net,node,buf);
	wtk_strbuf_delete(buf);
}

void wtk_fst_net3_print_next(wtk_fst_net3_t *net,wtk_fst_node_t *node)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_strbuf_push_s(buf,"<==");
	wtk_fst_net3_print_state(net,node,buf);
	wtk_strbuf_delete(buf);
}


void wtk_fst_net3_print_prev_arc_depth(wtk_fst_net3_t *net,wtk_fst_node_t *fn,int depth,int want)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int i=0,j;

	if(!fn->prev_q.pop)
	{
		if(fn!=net->start)
		{
			wtk_debug("============= print next ==============\n");
			wtk_fst_net3_print_next_arc(net,fn,1);
			wtk_debug("found bug fn=%p %d:%d\n",fn,depth,want);
			exit(0);
		}
	}
	for(i=0,qn=fn->prev_q.pop;qn;qn=qn->next,++i)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		for(j=0;j<want-depth;++j)
		{
			printf("\t");
		}
		printf("v[%d]: a=%f l=%f [%d:%.*s:%.*s] %d:%d=>%d:%d arc=%p [ %p(eof=%d) => %p(eof=%d) ] trans=%p\n",
				i,arc->ac_like,arc->lm_like,arc->frame,
				net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->len,
				net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->data,
				net->net_cfg->sym_out->strs[arc->out_id]->len,
				net->net_cfg->sym_out->strs[arc->out_id]->data,
				(arc->from_node&&arc->from_node->state)?arc->from_node->state->id:-1,
				arc->from_node?arc->from_node->frame:0,
				arc->to_node->state?arc->to_node->state->id:-1,arc->to_node->frame,
				arc,arc->from_node,arc->from_node->eof,arc->to_node,arc->to_node->eof,
				arc->trans);
		if(arc->from_node && depth>1)
		{
			wtk_fst_net3_print_prev_arc_depth(net,arc->from_node,depth-1,want);
		}
	}
}

void wtk_fst_net3_print_prev_arc(wtk_fst_net3_t *net,wtk_fst_node_t *fn,int depth)
{
	wtk_debug("===================== prev arc[node=%p] ====================\n",fn);
	wtk_fst_net3_print_prev_arc_depth(net,fn,depth,depth);
	wtk_debug("====================================================\n");
}

int wtk_fst_node_prev_has_output(wtk_fst_node_t *fn,unsigned int out_id,unsigned int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int b;

	for(qn=fn->prev_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		if(arc->out_id==out_id && arc->frame==frame)
		{
			return 1;
		}
		if(arc->from_node)
		{
			b=wtk_fst_node_prev_has_output(arc->from_node,out_id,frame);
			if(b)
			{
				return 1;
			}
		}
	}
	return 0;
}

int wtk_fst_node_prev_has_output2(wtk_fst_node_t *fn,unsigned int out_id)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int b;

	for(qn=fn->prev_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		if(arc->out_id==out_id)
		{
			return 1;
		}
		if(arc->from_node)
		{
			b=wtk_fst_node_prev_has_output2(arc->from_node,out_id);
			if(b)
			{
				return 1;
			}
		}
	}
	return 0;
}

int wtk_fst_node_nxt_has_output(wtk_fst_node_t *fn,unsigned int out_id,unsigned int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int b;

	for(qn=fn->next_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc->out_id==out_id && arc->frame==frame)
		{
			//wtk_debug("==============> found arc=%p %p=>%p\n",arc,arc->from_node,arc->to_node);
			return 1;
		}
		if(arc->frame<=frame)
		{
			b=wtk_fst_node_nxt_has_output(arc->to_node,out_id,frame);
			if(b)
			{
				return 1;
			}
		}
	}
	return 0;
}

int wtk_fst_node_nxt_has_output2(wtk_fst_node_t *fn,unsigned int out_id)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int b;

	for(qn=fn->next_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc->out_id==out_id)
		{
			//wtk_debug("==============> found arc=%p %p=>%p\n",arc,arc->from_node,arc->to_node);
			return 1;
		}
		b=wtk_fst_node_nxt_has_output2(arc->to_node,out_id);
		if(b)
		{
			return 1;
		}
	}
	return 0;
}


void wtk_fst_net3_print_next_arc_depth(wtk_fst_net3_t *net,wtk_fst_node_t *fn,int depth,int want)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int i=0,j;

	for(i=0,qn=fn->next_q.pop;qn;qn=qn->next,++i)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		for(j=0;j<want-depth;++j)
		{
			printf("\t");
		}
		/*
		printf("v[%d:%d]: s=%f a=%f l=%f [f=%d:%.*s:%.*s] %d:%d=>%d:%d arc=%p [ %p(eof=%d) =>%p(eof=%d)] trans=%p\n",
				want-depth,i,arc->arc_like,arc->ac_like,arc->lm_like,arc->frame,
				net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->len,
				net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->data,
				net->net_cfg->sym_out->strs[arc->out_id]->len,
				net->net_cfg->sym_out->strs[arc->out_id]->data,
				arc->from_node->state?arc->from_node->state->id:-1,arc->from_node?arc->from_node->frame:0,
				arc->to_node->state?arc->to_node->state->id:-1,arc->to_node->frame,
				arc,arc->from_node,arc->from_node->eof,
				arc->to_node,arc->to_node->eof,
				arc->trans);
		*/
		printf("v[%d:%d]: [f=%d:%.*s:%.*s] arc=%p %p=>%p trans=%p[%d:%d => %d:%d hook=%p to=%p]\n",
				want-depth,i,arc->frame,
				net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->len,
				net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->data,
				net->net_cfg->sym_out->strs[arc->out_id]->len,
				net->net_cfg->sym_out->strs[arc->out_id]->data,
				arc,arc->from_node,arc->to_node,
				arc->trans,
				arc->from_node->state?arc->from_node->state->id:-1,arc->from_node->frame,
				arc->to_node->state?arc->to_node->state->id:-1,arc->to_node->frame,
				arc->trans?arc->trans->to_state->hook:NULL,
				arc->to_node);
		/*
		if(arc->out_id>net->net_cfg->snt_end_id && wtk_queue2_len(&(arc->to_node->path_q))<=0)
		{
			exit(0);
		}*/
		if(arc->to_node && depth>1)
		{
			wtk_fst_net3_print_next_arc_depth(net,arc->to_node,depth-1,want);
		}
	}
}

void wtk_fst_net3_print_next_arc(wtk_fst_net3_t *net,wtk_fst_node_t *fn,int depth)
{
	wtk_debug("===================== next arc [node=%p start=%p] ====================\n",fn,net->start);
	wtk_fst_net3_print_next_arc_depth(net,fn,depth,depth);
	wtk_debug("======================================================================\n");
}

void wtk_fst_net3_print_lat_inst(wtk_fst_net3_t *net,wtk_fst_lat_inst_t *inst)
{
	wtk_queue_node_t *qn;
	wtk_fst_node_t *fn;

	for(qn=inst->list_q.pop;qn;qn=qn->next)
	{
		fn=data_offset2(qn,wtk_fst_node_t,inst_n);
		wtk_fst_net3_print_prev_arc(net,fn,0);
	}
}

void wtk_fst_net3_print_arc_x(wtk_fst_net3_t *net,wtk_fst_arc_t *arc,const char *f,int l)
{

	printf("%s:%d: %p=>%p [%d:%.*s:%.*s] s=%f a=%f l=%f\n",f,l,
			arc->from_node,arc->to_node,
			arc->frame,
			net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->len,
			net->net_cfg->sym_in->ids[wtk_fst_arc_in_id(arc)]->str->data,
			net->net_cfg->sym_out->strs[arc->out_id]->len,
			net->net_cfg->sym_out->strs[arc->out_id]->data,
			arc->arc_like,arc->ac_like,arc->lm_like);
}


int wtk_fst_net3_reach_end(wtk_fst_net3_t *net,wtk_fst_node_t *fn)
{
	wtk_fst_arc_t *arc;
	wtk_queue_node_t *qn;
	int b=0;

	qn=fn->next_q.pop;
	if(!qn)
	{
		if(fn->eof){return 1;}
		return 0;
	}else
	{
		for(qn=fn->next_q.pop;qn;qn=qn->next)
		{
			arc=data_offset2(qn,wtk_fst_arc_t,next_n);
			b=wtk_fst_net3_reach_end(net,arc->to_node);
			if(b)
			{
				return 1;
			}
		}
	}
	return 0;
}

int wtk_fst_net3_reach_start(wtk_fst_net3_t *net,wtk_fst_node_t *fn)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;
	int b;

	if(fn->prev_q.pop)
	{
		for(qn=fn->prev_q.pop;qn;qn=qn->next)
		{
			arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
			if(arc->from_node)
			{
				b=wtk_fst_net3_reach_start(net,arc->from_node);
				if(!b)
				{
					return 0;
				}
			}
		}
		return 1;
	}else
	{
		if(fn==net->start)
		{
			return 1;
		}else
		{
			return 0;
		}
	}
}


void wtk_fst_net3_check_active_node(wtk_fst_net3_t *net)
{
	wtk_queue_node_t *qn,*qn2;
	wtk_fst_node_t *fn;
	wtk_fst_arc_t *arc;

	for(qn2=net->active_node_q.pop;qn2;qn2=qn2->next)
	{
		fn=data_offset2(qn2,wtk_fst_node_t,q_n);
		for(qn=fn->next_q.pop;qn;qn=qn->next)
		{
			arc=data_offset2(qn,wtk_fst_arc_t,next_n);
			if(!arc->to_node || arc->from_node!=fn)
			{
				wtk_debug("found bug arc=%p %p=>%p\n",arc,arc->from_node,arc->to_node);
				exit(0);
			}
		}
	}
}


void wtk_fst_net3_clean_hook(wtk_fst_net3_t *net)
{
	wtk_queue_node_t *qn,*qn2;
	wtk_fst_node_t *n;
	wtk_fst_arc_t *arc;

	for(qn=net->active_node_q.pop;qn;qn=qn->next)
	{
		n=data_offset(qn,wtk_fst_node_t,q_n);
		n->hook.p=NULL;
		for(qn2=n->next_q.pop;qn2;qn2=qn2->next)
		{
			arc=data_offset(qn2,wtk_fst_arc_t,next_n);
			arc->hook=NULL;
			/*
			if(arc->out_id==net->net_cfg->snt_end_id)
			{
				//remove </s>
				arc->out_id=net->net_cfg->eps_id;
				arc->trans=NULL;
			}*/
		}
	}
}

//------------------ pruning by net2 ---------------------------
void wtk_fst_node_clean_hook(wtk_fst_node_t *n)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;

	n->hook.p=NULL;
	for(qn=n->next_q.pop;qn;qn=qn->next)
	{
		arc=data_offset(qn,wtk_fst_arc_t,next_n);
		arc->hook=NULL;
		if(arc->to_node->hook.p)
		{
			wtk_fst_node_clean_hook(arc->to_node);
		}
	}
}


wtk_fst_arc_t* wtk_fst_node_found_arc(wtk_fst_net3_t *net,wtk_fst_node_t *n,int out_id,int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;

	for(qn=n->next_q.pop;qn;qn=qn->next)
	{
		arc=data_offset(qn,wtk_fst_arc_t,next_n);
		wtk_debug("%d/%d [%.*s] frame=%d/%d\n",arc->out_id,out_id,
				net->net_cfg->sym_out->strs[arc->out_id]->len,
				net->net_cfg->sym_out->strs[arc->out_id]->data,
				frame,arc->frame
				);
		if(arc->out_id==out_id && arc->frame==frame)
		{
			return arc;
		}
	}
	return NULL;
}

void wtk_fst_net3_mark_next(wtk_fst_net3_t *net,wtk_fst_node_t *n,wtk_fst_state_t *s)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;

	//wtk_debug("n=%p:%p\n",n,s);
	n->hook.p=s;
	if(n->eof){return;}
	for(qn=n->next_q.pop;qn;qn=qn->next)
	{
		arc=data_offset(qn,wtk_fst_arc_t,next_n);
		arc->hook=s;
		if(!arc->to_node->hook.p)
		{
			wtk_fst_net3_mark_next(net,arc->to_node,s);
		}
	}
}

int wtk_fst_node_compose(wtk_fst_net3_t *net,wtk_fst_node_t *n,wtk_fst_state_t *s)
{
	wtk_queue_node_t *qn;
	wtk_fst_trans2_t *trans;
	wtk_fst_arc_t *arc;
	int cnt,x;

	cnt=0;
	//wtk_debug("n=%p/%d s=%p/%d\n",n,wtk_queue2_len(&(n->next_q)),s,wtk_fst_state_ntrans(s));
	if(s->type==WTK_FST_FINAL_STATE)
	{
		if(n->eof)
		{
			//wtk_debug("n=%p null=%p end=%p\n",n,net->null_node,net->end);
			n->hook.p=s;
			//wtk_debug("n %p=%p\n",n,n->hook.p);
			cnt+=1;
		}
		return cnt;
	}
	for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		if(trans->out_id==0){continue;}

		if(trans->hook2)
		{
			wtk_fst_net3_mark_next(net,n,s);
			cnt+=1;
			continue;
		}
		for(qn=n->next_q.pop;qn;qn=qn->next)
		{
			arc=data_offset(qn,wtk_fst_arc_t,next_n);
			if(arc->out_id==trans->out_id && arc->frame==trans->frame)
			{
				x=wtk_fst_node_compose(net,arc->to_node,trans->to_state);
				if(x>0)
				{
					/*
					wtk_debug("%.*s v[%d]=%p %p=>%p %d/%d\n",
							net->net_cfg->sym_out->strs[arc->out_id]->len,
							net->net_cfg->sym_out->strs[arc->out_id]->data,
							arc->frame,arc,arc->from_node,arc->to_node,arc->out_id,trans->out_id);
					*/
					//wtk_debug("arc=%p %p=>%p\n",arc,arc->from_node,arc->to_node);
					arc->hook=s;
					cnt+=x;
				}
			}
		}
	}

	for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		if(trans->out_id!=0 && trans->out_id!=net->net_cfg->snt_end_id){continue;}
		//wtk_debug("trans out id=%d\n",trans->out_id);
		x=wtk_fst_node_compose(net,n,trans->to_state);
		if(x>0)
		{
			cnt+=x;
		}
	}

	for(qn=n->next_q.pop;qn;qn=qn->next)
	{
		arc=data_offset(qn,wtk_fst_arc_t,next_n);
		if(arc->out_id==0 || arc->out_id==net->net_cfg->snt_end_id)
		{
			//wtk_debug("arc out id=%d\n",arc->out_id);
			x=wtk_fst_node_compose(net,arc->to_node,s);
			if(x>0)
			{
				/*
				wtk_debug("%p:%p %.*s v[%d]=%p %p=>%p %d/%d arc=%p\n",n,s,
						net->net_cfg->sym_out->strs[arc->out_id]->len,
						net->net_cfg->sym_out->strs[arc->out_id]->data,
						arc->frame,arc,arc->from_node,arc->to_node,arc->out_id,x,arc);
				*/
				//wtk_debug("arc=%p %p=>%p\n",arc,arc->from_node,arc->to_node);
				arc->hook=s;
				cnt+=x;
			}
		}
	}
	if(cnt>0 && !n->hook.p)
	{
		n->hook.p=s;
	}
	return cnt;
}

int wtk_fst_node_prev_has_output3(wtk_fst_node_t *fn,unsigned int out_id,unsigned int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;

	for(qn=fn->prev_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		if(arc->out_id==out_id && arc->frame==frame)
		{
			return 1;
		}
	}
	return 0;
}



void wtk_fst_net3_remove_unlink_node2(wtk_fst_net3_t *net)
{
	wtk_queue_node_t *qn,*qn2,*qn3;
	wtk_fst_node_t *n;
	wtk_fst_arc_t *arc;
	wtk_queue_t q;
#ifdef USE_LAT_FST_PATH
	wtk_wfst_path_t *pth;
	int b;
#endif

	wtk_queue_init(&(q));
	//wtk_slist_init(&(net->list_q));
	while(1)
	{
		qn2=wtk_queue_pop(&(net->active_node_q));
		if(!qn2){break;}
		n=data_offset(qn2,wtk_fst_node_t,q_n);
		if(n->hook.p)
		{
			for(qn=n->next_q.pop;qn;qn=qn3)
			{
				qn3=qn->next;
				arc=data_offset(qn,wtk_fst_arc_t,next_n);
				if(!arc->hook)
				{
					//wtk_fst_net3_remove_arc_from_src_node(net,n,arc);
					wtk_fst_net3_unlink_arc(net,arc);
					wtk_fst_net3_push_arc(net,arc);
				}else
				{
					/*
					wtk_debug("%.*s v[%d]=%p\n",
							net->net_cfg->sym_out->strs[arc->out_id]->len,
							net->net_cfg->sym_out->strs[arc->out_id]->data,
							arc->frame,arc);
					*/
					arc->hook=NULL;
					//wtk_debug("arc=%p %p=>%p\n",arc,arc->from_node,arc->to_node);
					/*
					if(arc->out_id==net->net_cfg->snt_end_id)
					{
						arc->out_id=net->net_cfg->eps_id;
					}*/
				}
			}
#ifdef USE_LAT_FST_PATH
			for(qn=n->path_q.pop;qn;qn=qn3)
			{
				qn3=qn->next;
				pth=data_offset(qn,wtk_wfst_path_t,lat_n);
				if(pth->trans->out_id>0)
				{
					b=wtk_fst_node_prev_has_output3(n,pth->trans->out_id,pth->frame);
					if(!b)
					{
						wtk_queue2_remove(&(n->path_q),&(pth->lat_n));
					}
					//exit(0);
				}else
				{
					wtk_queue2_remove(&(n->path_q),&(pth->lat_n));
				}
			}
#endif
			n->hook.p=NULL;
			//wtk_debug("pop node=%p next=%d prev=%d\n",n,wtk_queue2_len(&(n->next_q)),wtk_queue2_len(&(n->prev_q)));
			wtk_queue_push(&(q),&(n->q_n));
		}else
		{
			//n->hook.p=NULL;
			wtk_queue_push(&(net->active_node_q),&(n->q_n));
			wtk_fst_net3_remove_node(net,n);
		}
	}
	net->active_node_q=q;
}

void wtk_fst_net3_print_arcs(wtk_fst_net3_t *net)
{
	wtk_queue_node_t *qn,*qn2;
	wtk_fst_node_t *n;
	wtk_fst_arc_t *arc;

	for(qn=net->active_node_q.pop;qn;qn=qn->next)
	{
		n=data_offset(qn,wtk_fst_node_t,q_n);
		if(n->hook.p)
		{
			wtk_debug("v[%d]=%p:%d\n",n->frame,n,wtk_queue2_len(&(n->next_q)));
			if(0)
			{
				for(qn2=n->next_q.pop;qn2;qn2=qn2->next)
				{
					arc=data_offset(qn2,wtk_fst_arc_t,next_n);
					wtk_debug("%.*s v[%d]=%p\n",
							net->net_cfg->sym_out->strs[arc->out_id]->len,
							net->net_cfg->sym_out->strs[arc->out_id]->data,
							arc->frame,arc);
				}
			}
		}
	}
}


int wtk_fst_net3_compose(wtk_fst_net3_t *net,wtk_fst_net2_t *net2)
{
	if(!net2->start || !net->start){return -1;}
	//wtk_debug("len=%d\n",wtk_slist_len(&(net->list_q)));
	wtk_fst_net3_clean_hook(net);
	//wtk_fst_node_clean_hook(net->start);
	wtk_fst_node_compose(net,net->start,(wtk_fst_state_t*)net2->start);
	wtk_fst_net3_remove_unlink_node2(net);
	//wtk_fst_net3_compose_expand(net);
	//wtk_debug("len=%d\n",wtk_slist_len(&(net->list_q)));
	//wtk_fst_net3_print_lat2(net,stdout);
	//wtk_fst_net3_clean_hook(net);
	//wtk_debug("active=%d\n",net->active_node_q.length);

	//wtk_fst_net3_print(net);
	return 0;
}

void wtk_fst_net3_remove_node2(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_queue_node_t *qn;
	wtk_queue2_t *q=&(n->prev_q);
	wtk_fst_arc_t *arc;
	wtk_fst_lat_inst_t *inst;

	if(n==net->end)
	{
		//wtk_debug("v[%d] remove end\n",net->last_frame);
		net->end=NULL;
	}
	if(n==net->null_node)
	{
		//wtk_debug("remove null\n");
		net->null_node=NULL;
	}
	if(n==net->null_prev_node)
	{
		net->null_prev_node=NULL;
	}
	while(1)
	{
		qn=wtk_queue2_pop(q);
		if(!qn){break;}
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		if(arc->from_node)
		{
			wtk_fst_net3_remove_arc_from_src_node(net,arc->from_node,arc);
		}
		wtk_fst_net3_push_arc(net,arc);
	}
	q=&(n->next_q);
	while(1)
	{
		qn=wtk_queue2_pop(q);
		if(!qn){break;}
		arc=data_offset2(qn,wtk_fst_arc_t,next_n);
		if(arc->to_node)
		{
			wtk_fst_net3_remove_arc_from_dst_node(net,arc->to_node,arc);
		}
		wtk_fst_net3_push_arc(net,arc);
	}
	if(n->state && n->inst)
	{
		//inst=(wtk_fst_lat_inst_t*)n->state->hook;
		inst=n->inst;
		wtk_queue2_remove(&(inst->list_q),&(n->inst_n));
	}
	wtk_fst_net3_push_node(net,n);
}

void wtk_fst_net3_remove_virutal_node(wtk_fst_net3_t *net)
{
	if(net->null_node)
	{
		wtk_fst_net3_remove_node2(net,net->null_node);
		net->null_node=NULL;
	}
	if(net->null_prev_node)
	{
		wtk_fst_net3_remove_node2(net,net->null_prev_node);
		net->null_prev_node=NULL;
	}
}

void wtk_fst_net3_check_node(wtk_fst_net3_t *net,wtk_fst_node_t *n)
{
	wtk_queue_node_t *qn2;
	wtk_fst_arc_t *arc;

	for(qn2=n->next_q.pop;qn2;qn2=qn2->next)
	{
		arc=data_offset(qn2,wtk_fst_arc_t,next_n);
		if(arc->out_id==0 && arc->frame==174)
		{
			//21687:0/174
			wtk_debug("found %p:%p=%d:%d/%d\n",n,arc,arc->trans->in_id,arc->out_id,arc->frame);
			exit(0);
		}
	}
}

void wtk_fst_net3_check_arc(wtk_fst_net3_t *net)
{
	wtk_queue_node_t *qn;
	wtk_fst_node_t *n;

	for(qn=net->active_node_q.pop;qn;qn=qn->next)
	{
		n=data_offset(qn,wtk_fst_node_t,q_n);
		wtk_fst_net3_check_node(net,n);
	}
}

void wtk_fst_net3_check_collect(wtk_fst_net3_t *net,int c)
{
	wtk_queue_node_t *qn;
	wtk_fst_node_t *n;
	int i=0;

	for(qn=net->active_node_q.pop;qn;qn=qn->next)
	{
		++i;
		n=data_offset(qn,wtk_fst_node_t,q_n);
		if(n->collect==c)
		{
			wtk_debug("found v[%d/%d nxt=%p]: f=%d/%p c=%d\n",i,net->active_node_q.length,qn->next,
					n->frame,n,n->collect);
			exit(0);
		}
	}
}

wtk_fst_arc_t* wtk_fst_node_prev_found_arc(wtk_fst_node_t *fn,unsigned int out_id,unsigned int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;

	for(qn=fn->prev_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		if(arc->out_id==out_id && arc->frame==frame)
		{
			return arc;
		}
	}
	return NULL;
}

wtk_fst_arc_t* wtk_fst_node_prev_found_arc2(wtk_fst_node_t *fn,unsigned int out_id,unsigned int in_id,unsigned int frame)
{
	wtk_queue_node_t *qn;
	wtk_fst_arc_t *arc;

	for(qn=fn->prev_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_arc_t,prev_n);
		if(arc->trans && arc->trans->in_id==in_id && arc->out_id==out_id && arc->frame==frame)
		{
			return arc;
		}
	}
	return NULL;
}

void wtk_fst_net3_remove_unlink_path(wtk_fst_net3_t *net)
{
#ifdef USE_LAT_FST_PATH
	wtk_queue_node_t *qn,*qn2,*qn3;
	wtk_fst_node_t *n;
	wtk_wfst_path_t *pth;
	wtk_fst_arc_t *arc;

	for(qn=net->active_node_q.pop;qn;qn=qn->next)
	{
		n=data_offset(qn,wtk_fst_node_t,q_n);
		for(qn2=n->path_q.pop;qn2;qn2=qn3)
		{
			qn3=qn2->next;
			pth=data_offset(qn2,wtk_wfst_path_t,lat_n);
			if(pth->trans->out_id>0)
			{
				arc=wtk_fst_node_prev_found_arc(n,pth->trans->out_id,pth->frame);
				if(!arc)
				{
					//wtk_debug("remove\n");
					wtk_queue2_remove(&(n->path_q),&(pth->lat_n));
					pth->hook=NULL;
				}
			}else
			{
				/*
				wtk_debug("[%d/%d] eof=%d type=%d\n",pth->trans->in_id,pth->trans->out_id,n->eof,
						n->state?n->state->type:-1);
				*/
				arc=wtk_fst_node_prev_found_arc2(n,pth->trans->in_id,pth->trans->out_id,pth->frame);
				if(!arc)
				{
					wtk_queue2_remove(&(n->path_q),&(pth->lat_n));
					pth->hook=NULL;
				}
			}
		}
	}
#endif
}
