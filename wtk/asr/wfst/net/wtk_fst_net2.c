#include "wtk_fst_net2.h"
#include "wtk/asr/model/wtk_dict.h"
#ifdef WIN32
#include <float.h>
#endif


void wtk_fst_net2_attach_array(wtk_fst_net2_t *n)
{
	n->e_array=wtk_larray_new(1024,sizeof(void*));
	//n->s_array=wtk_larray_new(1024,sizeof(void*));
	n->tmp1_array=wtk_larray_new(1024,sizeof(void*));
	//n->tmp2_array=wtk_larray_new(1024,sizeof(void*));
}

wtk_fst_net2_t* wtk_fst_net2_new(wtk_fst_net_cfg_t *cfg)
{
	wtk_fst_net2_t *n;

	n=(wtk_fst_net2_t*)wtk_malloc(sizeof(wtk_fst_net2_t));
	n->cfg=cfg;
	//n->trans_pool=wtk_vpool_new(sizeof(wtk_fst_trans_t),cfg->trans_cache);
	//n->state_pool=wtk_vpool_new(sizeof(wtk_fst_state_t),cfg->state_cache);
	n->heap=wtk_heap_new(4096);
	n->buf=wtk_strbuf_new(1024,1.0);
	n->e_array=n->tmp1_array=NULL;//n->s_array=n->tmp2_array=NULL;
	n->print=NULL;
	wtk_fst_net2_reset(n);
	return n;
}

void wtk_fst_net2_delete(wtk_fst_net2_t *net)
{
	if(net->e_array)
	{
		wtk_larray_delete(net->e_array);
		//wtk_larray_delete(net->s_array);
		wtk_larray_delete(net->tmp1_array);
		//wtk_larray_delete(net->tmp2_array);
	}
	wtk_strbuf_delete(net->buf);
	wtk_heap_delete(net->heap);
	//wtk_vpool_delete(net->trans_pool);
	//wtk_vpool_delete(net->state_pool);
	wtk_free(net);
}

void wtk_fst_net2_reset(wtk_fst_net2_t *net)
{
	//wtk_vpool_reset(net->state_pool);
	//wtk_vpool_reset(net->trans_pool);
	if(net->e_array)
	{
		wtk_larray_reset(net->e_array);
		//wtk_larray_reset(net->s_array);
		wtk_larray_reset(net->tmp1_array);
		//wtk_larray_reset(net->tmp2_array);
	}
	wtk_heap_reset(net->heap);
	net->start=0;
	net->end=0;
	net->state_id=0;
	net->trans_id=0;
	net->nbest=0;
	net->nbest_cnt=0;
}

wtk_fst_state2_t* wtk_fst_net2_pop_state(wtk_fst_net2_t *net)
{
	wtk_fst_state2_t *state;

	state=(wtk_fst_state2_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_state2_t));
	state->in_prev=NULL;
	state->type=WTK_FST_NORM_STATE;
	state->v.trans=0;
	state->id=net->state_id;
	state->hook=0;
	state->ntrans=0;
	++net->state_id;
	return state;
}

wtk_fst_trans2_t* wtk_fst_net2_pop_trans(wtk_fst_net2_t *net)
{
	wtk_fst_trans2_t *trans;

	++net->trans_id;
	trans=(wtk_fst_trans2_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans2_t));
	wtk_fst_trans_init((wtk_fst_trans_t*)trans);
	trans->hook2=NULL;
	trans->in_prev=NULL;
	//trans->hook=0;
	return trans;

	//return  (wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*n);
}




wtk_fst_trans2_t* wtk_fst_net2_link_state(wtk_fst_net2_t *net,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e,
		unsigned int frame,unsigned int in_id,unsigned int out_id,
		float lm_like,float weight)
{
	wtk_fst_trans2_t *trans2;

	//wtk_debug("in=%d/%d\n",in_id,out_id);
	/*
	if(0)
	{
		static int ki=0;

		wtk_debug("v[%d]: %d:%d %d=>%d\n",++ki,in_id,out_id,s->id,e->id);
		if(ki==17)
		{
			//exit(0);
		}
	}*/
	/*
	if(in_id==0)
	{
		if(wtk_fst_state2_has_eps_to(e,s))
		{
			wtk_debug("%d->%d\n",e->id,s->id);
			exit(0);
		}
		if(wtk_fst_state2_has_eps_to(s,e))
		{
			wtk_debug("%d->%d %d:%d\n",e->id,s->id,in_id,out_id);
			exit(0);
		}
	}*/
	//print_hex((char*)&in_id,4);
	trans2=wtk_fst_net2_pop_trans(net);
	trans2->hook.next=s->v.trans;
	s->v.trans=(wtk_fst_trans_t*)trans2;
	s->ntrans++;
	trans2->in_prev=e->in_prev;
	e->in_prev=trans2;
	trans2->from_state=(wtk_fst_state_t*)s;
	trans2->to_state=(wtk_fst_state_t*)e;
	trans2->frame=frame;
	trans2->in_id=in_id;
	trans2->out_id=out_id;
	trans2->lm_like=lm_like;
	trans2->weight=weight;
	//wtk_debug("trans=%p\n", trans2);
	return trans2;
}

void wtk_fst_net2_link_state2(wtk_fst_net2_t *net,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e,wtk_fst_trans2_t *trans2,
		unsigned int frame,unsigned int in_id,unsigned int out_id,
		float lm_like,float weight)
{
	trans2->hook.next=s->v.trans;
	s->v.trans=(wtk_fst_trans_t*)trans2;
	trans2->in_prev=e->in_prev;
	e->in_prev=trans2;
	trans2->from_state=(wtk_fst_state_t*)s;
	trans2->to_state=(wtk_fst_state_t*)e;
	trans2->frame=frame;
	trans2->in_id=in_id;
	trans2->out_id=out_id;
	trans2->lm_like=lm_like;
	trans2->weight=weight;
}

wtk_fst_trans2_t* wtk_fst_net2_link_state3(wtk_fst_net2_t *net,
		wtk_fst_state2_t *s,wtk_fst_state2_t *e,
		unsigned int frame,unsigned int in_id,unsigned int out_id,
		float lm_like,float weight)
{
	wtk_fst_trans2_t *trans2=NULL;
	wtk_fst_trans_t *tmp=NULL;

	trans2=wtk_fst_net2_pop_trans(net);
	trans2->hook.next=s->v.trans;

	s->ntrans++;
	tmp=s->v.trans;
	s->v.trans=(wtk_fst_trans_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_t)*s->ntrans);
	if(tmp)
	{
		memcpy(s->v.trans,tmp,sizeof(wtk_fst_trans_t)*(s->ntrans-1));
	}
	tmp=s->v.trans+s->ntrans-1;

	//s->v.trans=(wtk_fst_trans_t*)trans2;
	trans2->in_prev=e->in_prev;
	e->in_prev=trans2;
	trans2->from_state=(wtk_fst_state_t*)s;
	trans2->to_state=(wtk_fst_state_t*)e;
	trans2->frame=frame;
	trans2->in_id=in_id;
	trans2->out_id=out_id;
	trans2->lm_like=lm_like;
	trans2->weight=weight;
	memcpy(tmp,trans2,sizeof(wtk_fst_trans_t));

	return trans2;
}


void wtk_fst_net2_print_state22(wtk_fst_net2_t *net,wtk_fst_state_t *state,wtk_strbuf_t *buf,double w)
{
	wtk_fst_trans_t *trans;
	int pos;

	wtk_debug("[%d] %p %d\n",state->id,state,state->type);
	if(state->type==WTK_FST_FINAL_STATE)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		//wtk_strbuf_push_f(buf," %f",w+state->v.weight);
		wtk_strbuf_push_f(buf," %f nil[%d]",w,state->id);
		printf("%.*s\n",buf->pos,buf->data);
		//exit(0);
		return;
	}
	if(!state->v.trans)
	{
		wtk_debug("xxxxxx\n");
		wtk_debug("%d %p %p \n",state->id,state,state->v.trans);
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		wtk_strbuf_push_f(buf," %f nil[%d]",w,state->id);
		printf("%.*s\n",buf->pos,buf->data);
		//exit(0);
	}
	//wtk_debug("trans=%p type=%d id=%d prev=%p\n",state->v.trans,state->type,state->id,((wtk_fst_state2_t*)state)->in_prev);
	for(trans=state->v.trans;trans;trans=trans->hook.next)
	{
		pos=buf->pos;
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		if(net->cfg->sym_out)
		{
			/*
			wtk_strbuf_push_f(buf,"[%.*s:%f]",
					net->cfg->sym_out->strs[trans->out_id]->len,
					net->cfg->sym_out->strs[trans->out_id]->data,
					trans->weight
					);
			*/
			if(trans->out_id>0)
			{
				wtk_strbuf_push_f(buf,"%.*s %d %d %f",
					net->cfg->sym_out->strs[trans->out_id]->len,
					net->cfg->sym_out->strs[trans->out_id]->data,
					state->id,
					trans->to_state->id,
					trans->weight
					);
			}
		}else
		{
			wtk_strbuf_push_f(buf,"[%d:%f]",
					trans->out_id,
					trans->weight
					);
		}
		/*
		wtk_debug("trans=%p %d->%d\n",trans,
				((wtk_fst_trans2_t*)trans)->from_state->id,trans->to_state->id);
		*/
		wtk_fst_net2_print_state22(net,trans->to_state,buf,w+trans->weight);
		buf->pos=pos;
	}
}


void wtk_fst_net2_print_state(wtk_fst_net2_t *net,wtk_fst_state_t *state,wtk_strbuf_t *buf,double w)
{
	wtk_fst_trans_t *trans;
	int pos;

	//wtk_debug("[%d]\n",state->id);
	if(state->type==WTK_FST_FINAL_STATE)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		wtk_strbuf_push_f(buf," %f",w+state->v.weight);
		printf("%.*s\n",buf->pos,buf->data);
		//exit(0);
		return;
	}
	if(!state->v.trans)
	{
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		wtk_strbuf_push_f(buf," %f nil[%d]",w,state->id);
		printf("%.*s\n",buf->pos,buf->data);
		//exit(0);
	}
	//wtk_debug("trans=%p type=%d id=%d prev=%p\n",state->v.trans,state->type,state->id,((wtk_fst_state2_t*)state)->in_prev);
	for(trans=state->v.trans;trans;trans=trans->hook.next)
	{
		pos=buf->pos;
		if(buf->pos>0)
		{
			wtk_strbuf_push_c(buf,' ');
		}
		if(net->cfg->sym_out)
		{
			/*
			wtk_strbuf_push_f(buf,"[%.*s:%f]",
					net->cfg->sym_out->strs[trans->out_id]->len,
					net->cfg->sym_out->strs[trans->out_id]->data,
					trans->weight
					);
			*/
			if(trans->out_id>0)
			{
				wtk_strbuf_push_f(buf,"%.*s",
					net->cfg->sym_out->strs[trans->out_id]->len,
					net->cfg->sym_out->strs[trans->out_id]->data
					);
			}
		}else
		{
			wtk_strbuf_push_f(buf,"[%d:%f]",
					trans->out_id,
					trans->weight
					);
		}
		/*
		wtk_debug("trans=%p %d->%d\n",trans,
				((wtk_fst_trans2_t*)trans)->from_state->id,trans->to_state->id);
		*/
		wtk_fst_net2_print_state(net,trans->to_state,buf,w+trans->weight);
		buf->pos=pos;
	}
}

void wtk_fst_net2_print(wtk_fst_net2_t *net)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_debug("s=%p e=%p\n",net->start,net->end);
	wtk_fst_net2_print_state(net,(wtk_fst_state_t*)net->start,buf,0);
	wtk_strbuf_delete(buf);
}

int wtk_fst_net2_bytes(wtk_fst_net2_t *net)
{
	return wtk_heap_bytes(net->heap);
}

typedef struct
{
	wtk_string_t *v;
	int id;
}wtk_fst_sym_item_t;

int wtk_fst_net2_load_sym2(wtk_str_hash_t *hash,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_string_t *v;
	wtk_heap_t *heap=hash->heap;
	wtk_fst_sym_item_t *item;
	int ret;
	int i;

	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		v=wtk_heap_dup_string(heap,buf->data,buf->pos);
		ret=wtk_source_read_int(src,&i,1,0);
		if(ret!=0){goto end;}
		item=(wtk_fst_sym_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_sym_item_t));
		item->v=v;
		item->id=i;
		wtk_str_hash_add(hash,v->data,v->len,item);
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

wtk_str_hash_t *wtk_fst_net2_load_sym(char *fn,int nslot)
{
	wtk_str_hash_t *hash;

	hash=wtk_str_hash_new(nslot);
	wtk_source_load_file(hash,(wtk_source_load_handler_t)wtk_fst_net2_load_sym2,fn);
	return hash;
}



void wtk_fst_net2_node_attach_short_path_hook(wtk_fst_net2_t *net,wtk_fst_state_t *state)
{
	wtk_fst_net2_pthem_t *item;
	wtk_fst_trans_t *trans;

	state->hook=item=(wtk_fst_net2_pthem_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_net2_pthem_t));
	item->state=state;
	item->min_input_trans=NULL;
	item->from_state=NULL;
	item->score=FLT_MAX;
	item->r=FLT_MAX;
	item->touch=0;
	for(trans=state->v.trans;trans;trans=trans->hook.next)
	{
		if(!trans->to_state->hook)
		{
			wtk_fst_net2_node_attach_short_path_hook(net,trans->to_state);
		}
	}
}


void wtk_fst_net2_attach_short_path_hook(wtk_fst_net2_t *net)
{
	wtk_fst_net2_node_attach_short_path_hook(net,(wtk_fst_state_t*)net->start);
}

int wtk_fst_net2_shortest_path(wtk_fst_net2_t *net)
{
	wtk_fst_state_t *s;
	wtk_fst_net2_pthem_t *item;
	wtk_fst_trans_t *trans;
	float r,f,fx;
	wtk_rbtree_t tree,*ptree;
	wtk_rbnode_t *rn;

	if(!net->end || !net->start){return -1;}
	ptree=&(tree);
	wtk_rbtree_init(ptree);
	//wtk_fst_net2_print(net);
	wtk_fst_net2_attach_short_path_hook(net);
	s=(wtk_fst_state_t*)net->start;
	item=(wtk_fst_net2_pthem_t*)s->hook;
	item->score=0;
	item->r=0;
	item->touch=1;
	item->rb_n.key=item->score;
	wtk_rbtree_insert(ptree,&(item->rb_n));
	while(ptree->root)
	{
		rn=(wtk_rbnode_t*)wtk_treenode_max((wtk_treenode_t*)ptree->root);
		if(!rn){break;}
		wtk_rbtree_remove(ptree,rn);
		item=data_offset2(rn,wtk_fst_net2_pthem_t,rb_n);
		item->touch=0;
		s=item->state;
		r=item->r;
		item->r=FLT_MAX;
		/*
		++ki;
		wtk_debug("v[%d]: len=%d r=%f key=%f item=%p\n",ki,ptree->len,r,item->rb_n.key,item);
		if(item->min_input_trans)
		{
			wtk_debug("[%.*s]\n",
			net->cfg->sym_out->strs[item->min_input_trans->out_id]->len,
			net->cfg->sym_out->strs[item->min_input_trans->out_id]->data);
		}else
		{
			wtk_debug("NULL\n");
		}
		wtk_rbtree_print(ptree);
		*/
		for(trans=s->v.trans;trans;trans=trans->hook.next)
		{
			item=(wtk_fst_net2_pthem_t*)trans->to_state->hook;
			f=wtk_fst_times(r,trans->weight);
			//wtk_debug("v[%d]: r=%f w=%f f=%f trans=%p:%d/%d\n",ki,r,trans->weight,f,trans,trans->in_id,trans->out_id);
			item->r=wtk_fst_plus(f,item->r);
			fx=wtk_fst_plus(f,item->score);
			/*
			wtk_debug("%p:%p: f=%f,%f,%f [%.*s]\n",item,&(item->rb_n),f,item->score,fx,
			net->cfg->sym_out->strs[trans->out_id]->len,
			net->cfg->sym_out->strs[trans->out_id]->data);
			*/
			if(item->score!=fx)
			{
				//wtk_debug("set %f,%f\n",item->d,fx);
				//wtk_debug("ri=%p,item=%p\n",ri,item)
				item->score=fx;
				//wtk_debug("score=%f\n",item->score);
				item->min_input_trans=trans;
				item->from_state=s;
				//wtk_debug("item=%p,%f/%f/%f\n",item,item->rb_n.key,item->r,item->score)
				item->rb_n.key=item->score;
				//wtk_debug("len=%d\n",ptree->len);
				if(!item->touch)
				{
					item->touch=1;
					//wtk_fst_net2_push_list(l,trans,item->d);
					//wtk_debug("insert=%p/%f\n",item,item->score);
					wtk_rbtree_insert(ptree,&(item->rb_n));
					//wtk_slist2_push(l,&(trans->to_state->q_n));
				}else
				{
					wtk_rbtree_remove(ptree,&(item->rb_n));
					wtk_rbtree_insert(ptree,&(item->rb_n));
				}
			}
		}
		//wtk_rbtree_print(ptree);
	}
	//wtk_debug("end=%d\n",net->end->id);
	//wtk_fst_net2_print_best_path(net);
	return net->end->hook?0:-1;
}

void wtk_fst_net2_print_best_path(wtk_fst_net2_t *net)
{
	wtk_fst_net2_pthem_t *item;
	wtk_fst_trans2_t *trans2;
	double ac,ac1,lm;

	ac=0;lm=0;
	item=(wtk_fst_net2_pthem_t*)net->end->hook;
	while(item && item->min_input_trans)
	{
		//wtk_debug("r=%f,d=%f,t=%d trans=%p\n",item->r,item->d,item->touch,item->min_input_trans);
		/*
		wtk_debug("[%.*s:%.*s]\n",
				net->cfg->sym_in->strs[item->min_input_trans->in_id]->len,
				net->cfg->sym_in->strs[item->min_input_trans->in_id]->data,
				net->cfg->sym_out->strs[item->min_input_trans->out_id]->len,
				net->cfg->sym_out->strs[item->min_input_trans->out_id]->data
				);
		*/
		trans2=(wtk_fst_trans2_t*)item->min_input_trans;
		lm+=trans2->lm_like;
		ac1=trans2->weight-trans2->lm_like;
		ac+=ac1;
		wtk_debug("[%.*s] score=%f ac=%f/%f lm=%f/%f\n",
				net->cfg->sym_out->strs[item->min_input_trans->out_id]->len,
				net->cfg->sym_out->strs[item->min_input_trans->out_id]->data,
				item->score,ac1,ac,trans2->lm_like,lm
				);
		item=(wtk_fst_net2_pthem_t*)item->from_state->hook;
	}
	//exit(0);
}

int wtk_fst_net2_get_history(wtk_fst_net2_t *net,int *ids,int max)
{
	wtk_fst_trans_t *trans;
	wtk_fst_net2_pthem_t *item;
	//wtk_string_t **strs=net->cfg->sym_out->strs;
	int cnt=0;

	if(!net->end)
	{
		goto end;
	}
	item=(wtk_fst_net2_pthem_t*)net->end->hook;
	while(item && item->min_input_trans)
	{
		trans=item->min_input_trans;
		//if(trans->out_id>2)
		if(wtk_fst_net_cfg_is_visible_wrd(net->cfg,trans->out_id))
		{
			ids[cnt++]=trans->out_id;
			//wtk_debug("out_id=%d %.*s\n",trans->out_id,net->cfg->sym_out->strs[trans->out_id]->len,net->cfg->sym_out->strs[trans->out_id]->data);
			if(cnt>=max)
			{
				break;
			}
		}
		item=(wtk_fst_net2_pthem_t*)item->from_state->hook;
	}
end:
	return cnt;
}

void wtk_fst_net2_get_short_one_best_path(wtk_fst_net2_t *net,wtk_strbuf_t *buf,char *sep,int bytes)
{
	wtk_fst_net2_pthem_t *item;
	wtk_fst_trans_t *trans;
	wtk_string_t **strs=net->cfg->sym_out->strs;
	wtk_string_t *v;

	//wtk_fst_net2_print_best_path(net);
	wtk_strbuf_reset(buf);
	if(!net->end)
	{
		return;
	}
	item=(wtk_fst_net2_pthem_t*)net->end->hook;
	//wtk_debug("prob=%f r=%f\n",item->score,item->r);
	while(item && item->min_input_trans)
	{
		trans=item->min_input_trans;
		//if(trans->out_id>2)
		if(wtk_fst_net_cfg_is_visible_wrd(net->cfg,trans->out_id))
		{
			v=strs[trans->out_id];
			if(bytes>0 && buf->pos>0)
			{
				wtk_strbuf_push_front(buf,sep,bytes);
			}
			wtk_strbuf_push_front(buf,v->data,v->len);
			//wtk_debug("v[%d]=[%.*s]\n",((wtk_fst_trans2_t*)trans)->frame,v->len,v->data);
		}
		item=(wtk_fst_net2_pthem_t*)item->from_state->hook;
		//wtk_debug("prob=%f r=%f\n",item->score,item->r);
	}
	//exit(0);
}

void wtk_fst_net2_get_short_one_best_path3(wtk_fst_net2_t *net,wtk_strbuf_t *buf,char *sep,int bytes,wtk_string_t **hot_sym)
{
	wtk_fst_net2_pthem_t *item;
	wtk_fst_trans_t *trans;
	wtk_string_t **strs=net->cfg->sym_out->strs;
	wtk_string_t *v;
	int last_in=-1,frame_cnt=1;
	int cnt=0;
	//wtk_fst_net2_print_best_path(net);
	wtk_strbuf_reset(buf);
	if(!net->end)
	{
		return;
	}
	item=(wtk_fst_net2_pthem_t*)net->end->hook;
	//wtk_debug("prob=%f r=%f\n",item->score,item->r);
	while(item && item->min_input_trans)
	{
		trans=item->min_input_trans;
		//if(trans->out_id>2)
		//wtk_debug("%p %d %d\n",trans,trans->out_id,trans->in_id);
		if(last_in==trans->in_id)
		{
			frame_cnt++;
		}else
		{
			cnt=frame_cnt;
			last_in=trans->in_id;
			frame_cnt=1;
		}
		if(wtk_fst_net_cfg_is_visible_wrd(net->cfg,trans->out_id))
		{
			//wtk_debug("%p %d\n",trans,trans->out_id);	
			if(trans->hot)
			{
				v=hot_sym[trans->out_id];
			}else
			{
				v=strs[trans->out_id];
			}
			if(bytes>0 && buf->pos>0)
			{
				wtk_strbuf_push_front(buf,sep,bytes);
			}
			
			wtk_strbuf_push_front(buf,v->data,v->len);
			if(wtk_str_equal_s(v->data,v->len,"二"))
			{
				//wtk_debug("xxx %d %d\n",frame_cnt,cnt);
				if(cnt>=16)
				{
					wtk_strbuf_push_front(buf,v->data,v->len);
					if(cnt>=24)
					{
						wtk_strbuf_push_front(buf,v->data,v->len);
					}
					if(cnt>=32)
					{
						wtk_strbuf_push_front(buf,v->data,v->len);
					}
				}
			}
			//wtk_debug("v[%d]=[%.*s]\n",((wtk_fst_trans2_t*)trans)->frame,v->len,v->data);
		}
		item=(wtk_fst_net2_pthem_t*)item->from_state->hook;
		//wtk_debug("prob=%f r=%f\n",item->score,item->r);
	}
	//exit(0);
}

//-------------------------- nbest -----------------------

/*
 * network will sorted like this:
 *
 *           o(2)  - o(3)
 *          /            \
 *        o(1)            o(6)
 *       /  \            / \
 *      /    o(4)  - o(5)   \
 *    o(0)                   o(13)
 *      \    o(8)  - o(9)   /
 *       \  /            \ /
 *        o(7)            o(12)
 *          \            /
 *           o(10) - o(11)
 *
 */
void wtk_fst_nbest_item_mark_back(wtk_fst_nbest_node_t *item,int *nn)
{
	wtk_queue_node_t *qn;
	wtk_fst_nbest_arc_t *arc;

	item->n=-2;
	for(qn=item->prev_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_fst_nbest_arc_t,prev_n);
		if(arc->from->n==-1)
		{
			wtk_fst_nbest_item_mark_back(arc->from,nn);
		}
	}
	item->n=(*nn)++;
}

int wtk_fst_net2_attach_nbest(wtk_fst_net2_t *net,wtk_fst_state_t *state,wtk_queue_t *q)
{
	wtk_fst_nbest_node_t *item,*item2;
	wtk_fst_trans_t *trans;
	//wtk_fst_trans2_t *trans2;
	wtk_fst_nbest_arc_t *arc;
	wtk_heap_t *heap=net->heap;
	int cnt,t;

	if(state->type!=WTK_FST_FINAL_STATE && !state->v.trans)
	{
		//wtk_debug("found nil type=%d trans=%p state=%p\n",state->type,state->v.trans,state);
		return 0;
	}
	cnt=0;
	state->hook=item=(wtk_fst_nbest_node_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_nbest_node_t));
	item->state=state;
	wtk_queue_init(&(item->next_q));
	wtk_queue_init(&(item->prev_q));
	wtk_queue_push(q,&(item->q_n));
	if(state->type==WTK_FST_FINAL_STATE)
	{
		//wtk_debug("found end\n");
		return 1;
	}
	for(trans=state->v.trans;trans;trans=trans->hook.next)
	{
		if(!trans->to_state->hook)
		{
			t=wtk_fst_net2_attach_nbest(net,trans->to_state,q);
			if(t==0)
			{
				//wtk_debug("t=%d\n",t);
				continue;
			}
			++cnt;
		}else
		{
			++cnt;
		}
		item2=(wtk_fst_nbest_node_t*)trans->to_state->hook;
		arc=(wtk_fst_nbest_arc_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_nbest_arc_t));
		arc->trans=trans;
		arc->from=item;
		wtk_queue_push(&(item->next_q),&(arc->next_n));
		arc->to=item2;
		wtk_queue_push(&(item2->prev_q),&(arc->prev_n));
	}
	//wtk_debug("cnt=%d\n",cnt);
	if(cnt==0)
	{
		state->hook=NULL;
	}
	return cnt;
}

wtk_fst_nbest_node_t** wtk_fst_net2_nbest_queue_to_array(wtk_fst_net2_t *net,wtk_queue_t *q)
{
	wtk_fst_nbest_node_t **items;
	wtk_fst_nbest_node_t *item;
	wtk_queue_node_t *qn;
	int i;

	items=(wtk_fst_nbest_node_t**)wtk_heap_malloc(net->heap,sizeof(wtk_fst_nbest_node_t*)*q->length);
	for(i=0,qn=q->pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_fst_nbest_node_t,q_n);
		if(item->state->type==WTK_FST_FINAL_STATE)
		{
			item->score=0;
		}else
		{
			item->score=FLT_MAX;
		}
		item->n=-1;
		items[i++]=item;
	}
	return items;
}

void wtk_fst_nbest_ent_print2(wtk_fst_nbest_ent_t *n,wtk_fst_net_cfg_t *cfg)
{
	wtk_string_t *v;

	if(n->path_prev)
	{
		wtk_fst_nbest_ent_print2(n->path_prev,cfg);
		printf(" ");
	}
	v=cfg->sym_out->strs[n->arc->trans->out_id];
	printf("%.*s[%p]",v->len,v->data,n->node->state);//,n->score);
}

void wtk_fst_nbest_ent_print(wtk_fst_nbest_ent_t *n,wtk_fst_net_cfg_t *cfg)
{
	wtk_fst_nbest_ent_print2(n,cfg);
	printf("\n");
}


int wtk_fst_nbest_ent_next_len(wtk_fst_nbest_ent_t *ent)
{
	int i=0;

	while(ent)
	{
		++i;
		ent=ent->next;
	}
	return i;
}

int wtk_fst_nbest_ent_prev_len(wtk_fst_nbest_ent_t *ent)
{
	int i=0;

	while(ent)
	{
		++i;
		ent=ent->prev;
	}
	return i;
}

wtk_fst_nbest_ent_t* wtk_fst_nbe_word_prev_wrd(wtk_fst_nbest_ent_t *src,wtk_fst_net_cfg_t *cfg)
{
	//while(src && src->arc->trans->out_id<2)
	while(src && (!wtk_fst_net_cfg_is_visible_wrd(cfg,src->arc->trans->out_id)))
	{
		src=src->path_prev;
	}
	return src;
}

int wtk_fst_nbe_word_match(wtk_fst_nbest_ent_t *cmp,wtk_fst_nbest_ent_t *ans,wtk_fst_net_cfg_t *cfg)
{
	if(cmp==ans)
	{
		return 1;
	}
	cmp=wtk_fst_nbe_word_prev_wrd(cmp,cfg);
	ans=wtk_fst_nbe_word_prev_wrd(ans,cfg);
	if(!cmp || !ans)
	{
		return 0;
	}else if(cmp->arc->trans->out_id!=ans->arc->trans->out_id)
	{
		return 0;
	}else
	{
		return wtk_fst_nbe_word_match(cmp->path_prev,ans->path_prev,cfg);
	}
}

void wtk_fst_net2_add_nbe(wtk_fst_net2_t *net,
		wtk_fst_nbest_ent_t *best,wtk_fst_nbest_node_t *node,wtk_rbtree_t *tree)
{
	wtk_queue_node_t *qn;
	wtk_fst_nbest_arc_t *arc;
	wtk_heap_t *heap=net->heap;
	double like,score;
	wtk_fst_nbest_ent_t *ent;//,*pos;

	for(qn=node->next_q.push;qn;qn=qn->prev)
	{
		arc=data_offset2(qn,wtk_fst_nbest_arc_t,next_n);
		like=arc->trans->weight;
		if(best)
		{
			//wtk_debug("best=%f\n",best->score);
			like+=best->like;
		}
		score=like+arc->to->score;
		//wtk_debug("w=%f like=%f/%f\n",arc->trans->weight,like,arc->to->score);
		//wtk_debug("score=%f\n",score);
		if(score<LSMALL){continue;}
		//wtk_debug("%d:%d\n",arc->trans->in_id,arc->trans->out_id);
		/*
		++i;
		wtk_debug("v[%d]=[%.*s] s=%f len=%d\n",i,net->cfg->sym_out->strs[arc->trans->out_id]->len,
				net->cfg->sym_out->strs[arc->trans->out_id]->data,score,
				wtk_fst_nbest_ent_next_len(head));
		*/
		ent=(wtk_fst_nbest_ent_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_nbest_ent_t));
		ent->score=score;
		ent->like=like;
		ent->node=arc->to;
		ent->arc=arc;
		ent->path_prev=best;
		ent->rb_n.key=score;
		ent->hook2=NULL;
		wtk_rbtree_insert(tree,&(ent->rb_n));
		/*
		for(pos=head->next;score>pos->score;pos=pos->next);
		ent->prev=pos->prev;
		ent->next=pos;
		ent->prev->next=ent->next->prev=ent;
		*/
		//wtk_debug("len=%d\n",wtk_fst_nbest_ent_next_len(head));
	}
}


int wtk_fst_net2_nbest_path(wtk_fst_net2_t *net,int k,int max_search)
{
	wtk_fst_nbest_node_t **nodes,*node;
	wtk_heap_t *heap;
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	wtk_fst_nbest_arc_t *arc;
	int i,len,n;
	int *order;
	float score;
	wtk_fst_nbest_ent_t *best,**ans;
	int cnt;
	wtk_rbtree_t tree,*ptree;
	wtk_rbnode_t *rn;
	int ret;

	if(!net->start || !net->end){return -1;}
	ptree=&(tree);
	wtk_rbtree_init(ptree);
	heap=net->heap;
	wtk_queue_init(&(q));
	wtk_fst_net2_attach_nbest(net,(wtk_fst_state_t*)net->start,&(q));
	len=q.length;
	nodes=wtk_fst_net2_nbest_queue_to_array(net,&(q));
	for(n=0,i=0;i<len;++i)
	{
		node=nodes[i];
		if(node->n==-1)
		{
			wtk_fst_nbest_item_mark_back(node,&n);
		}
	}
	order=(int*)wtk_heap_malloc(heap,sizeof(int)*len);
	for(i=0;i<len;++i)
	{
		order[nodes[i]->n]=i;
	}
	for(i=len-1;i>=0;--i)
	{
		node=nodes[order[i]];
		//wtk_debug("v[%d]=%d\n",item->n,item->state->id);
		for(qn=node->prev_q.pop;qn;qn=qn->next)
		{
			arc=data_offset2(qn,wtk_fst_nbest_arc_t,prev_n);
			score=node->score+arc->trans->weight;
			//score=-score;
			if(score<arc->from->score)
			{
				arc->from->score=score;
			}
		}
	}
	for(i=0;i<len;++i)
	{
		node=nodes[i];
		if(node->prev_q.length>0){continue;}
		//wtk_debug("prev=%d\n",node->prev_q.length);
		wtk_fst_net2_add_nbe(net,NULL,node,ptree);
	}
	ans=(wtk_fst_nbest_ent_t**)wtk_heap_zalloc(heap,sizeof(wtk_fst_nbest_ent_t*)*k);
	//for(n=0,best=head.next;n<k&&best!=&(tail);best=head.next)
	for(cnt=0,n=0;n<k && ptree->len>0;)
	{
		++cnt;
		if(max_search>0 && cnt>max_search)
		{
			//wtk_debug("cnt=%d,max=%d\n",cnt,max_search);
			break;
		}
		rn=(wtk_rbnode_t *)wtk_treenode_min((wtk_treenode_t*)ptree);
		if(!rn){break;}
		wtk_rbtree_remove(ptree,rn);
		best=data_offset2(rn,wtk_fst_nbest_ent_t,rb_n);
		//wtk_debug("best=%p %f\n",best,best->rb_n.key);
		//wtk_debug("check best=%p h=%p:%p\n",best,&head,&tail);
		//best->next->prev=best->prev;
		//best->prev->next=best->next;
		//wtk_debug("v[%d/%d]=%d\n",j,best->node->state->id,best->node->next_q.length);//wtk_fst_nbest_ent_next_len(&(head)));
		if(best->node->next_q.length>0)
		{
			wtk_fst_net2_add_nbe(net,best,best->node,ptree);
			continue;
		}
		for(i=0;i<=n;++i)
		{
			if(wtk_fst_nbe_word_match(best,ans[i],net->cfg))
			{
				//wtk_debug("============ match [%d] ===========\n",n);
				//wtk_fst_nbest_ent_print(best,net->cfg);
				best=0;
				break;
			}
		}
		if(best)
		{
			//wtk_fst_nbest_ent_print(best,net->cfg);
			ans[n++]=best;
			cnt=0;
		}
	}
	net->nbest_cnt=n;
	if(n==0)
	{
		net->nbest=0;
		wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
		ret=wtk_fst_net2_shortest_path(net);
	}else
	{
		net->nbest=ans;
		ret=0;
	}
	return ret;
}

void wtk_fst_net2_add_nbe2(wtk_fst_net2_t *net,wtk_fst_nbest_ent_t *head,
		wtk_fst_nbest_ent_t *best,wtk_fst_nbest_node_t *node)
{
	wtk_queue_node_t *qn;
	wtk_fst_nbest_arc_t *arc;
	wtk_heap_t *heap=net->heap;
	double like,score;
	wtk_fst_nbest_ent_t *ent,*pos;

	for(qn=node->next_q.push;qn;qn=qn->prev)
	{
		arc=data_offset2(qn,wtk_fst_nbest_arc_t,next_n);
		like=arc->trans->weight;
		if(best)
		{
			//wtk_debug("best=%f\n",best->score);
			like+=best->like;
		}
		score=like+arc->to->score;
		//wtk_debug("w=%f like=%f/%f\n",arc->trans->weight,like,arc->to->score);
		//wtk_debug("score=%f\n",score);
		if(score<LSMALL){continue;}
		//wtk_debug("%d:%d\n",arc->trans->in_id,arc->trans->out_id);
		/*
		++i;
		wtk_debug("v[%d]=[%.*s] s=%f len=%d\n",i,net->cfg->sym_out->strs[arc->trans->out_id]->len,
				net->cfg->sym_out->strs[arc->trans->out_id]->data,score,
				wtk_fst_nbest_ent_next_len(head));
		*/
		ent=(wtk_fst_nbest_ent_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_nbest_ent_t));
		ent->score=score;
		ent->like=like;
		ent->node=arc->to;
		ent->arc=arc;
		ent->path_prev=best;
		for(pos=head->next;score>pos->score;pos=pos->next);
		ent->prev=pos->prev;
		ent->next=pos;
		ent->prev->next=ent->next->prev=ent;
		//wtk_debug("len=%d\n",wtk_fst_nbest_ent_next_len(head));
	}
}


void wtk_fst_net2_nbest_path2(wtk_fst_net2_t *net,int k)
{
	wtk_fst_nbest_node_t **nodes,*node;
	wtk_heap_t *heap;
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	wtk_fst_nbest_arc_t *arc;
	int i,len,n;
	int *order;
	float score;
	wtk_fst_nbest_ent_t head,tail,*best,**ans;
	int cnt;
	double t;

	t=time_get_ms();
	heap=net->heap;
	wtk_queue_init(&(q));
	wtk_fst_net2_attach_nbest(net,(wtk_fst_state_t*)net->start,&(q));
	len=q.length;
	nodes=wtk_fst_net2_nbest_queue_to_array(net,&(q));
	for(n=0,i=0;i<len;++i)
	{
		node=nodes[i];
		if(node->n==-1)
		{
			wtk_fst_nbest_item_mark_back(node,&n);
		}
	}
	order=(int*)wtk_heap_malloc(heap,sizeof(int)*len);
	for(i=0;i<len;++i)
	{
		order[nodes[i]->n]=i;
	}
	for(i=len-1;i>=0;--i)
	{
		node=nodes[order[i]];
		//wtk_debug("v[%d]=%d\n",item->n,item->state->id);
		for(qn=node->prev_q.pop;qn;qn=qn->next)
		{
			arc=data_offset2(qn,wtk_fst_nbest_arc_t,prev_n);
			score=node->score+arc->trans->weight;
			//score=-score;
			if(score<arc->from->score)
			{
				arc->from->score=score;
			}
		}
	}
	head.path_prev=tail.path_prev=NULL;
	head.next=&(tail);head.prev=NULL;
	tail.next=0;tail.prev=&(head);
	head.score=tail.score=FLT_MAX;
	for(i=0;i<len;++i)
	{
		node=nodes[i];
		if(node->prev_q.length>0){continue;}
		wtk_fst_net2_add_nbe2(net,&head,NULL,node);
	}
	ans=(wtk_fst_nbest_ent_t**)wtk_heap_zalloc(heap,sizeof(wtk_fst_nbest_ent_t*)*k);
	for(cnt=0,n=0,best=head.next;n<k&&best!=&(tail);best=head.next)
	{
#ifdef DEBUG_X
		static int j=0;

		++j;
#endif
		++cnt;
		if(cnt>10000)
		{
			break;
		}
		best->next->prev=best->prev;
		best->prev->next=best->next;
		//wtk_debug("v[%d/%d]=%d\n",j,best->node->state->id,best->node->next_q.length);//wtk_fst_nbest_ent_next_len(&(head)));
		if(best->node->next_q.length>0)
		{
			wtk_fst_net2_add_nbe2(net,&head,best,best->node);
			continue;
		}
		for(i=0;i<=n;++i)
		{
			if(wtk_fst_nbe_word_match(best,ans[i],net->cfg))
			{
				//wtk_debug("============ match [%d] ===========\n",n);
				//wtk_fst_nbest_ent_print(best,net->cfg);
				best=0;
				break;
			}
		}
		if(best)
		{
			//wtk_fst_nbest_ent_print(best,net->cfg);
			ans[n++]=best;
			cnt=0;
		}
	}
	t=time_get_ms()-t;
	wtk_debug("time=%f\n",t);
	net->nbest_cnt=n;
	net->nbest=ans;
}

int wtk_fst_state_has_trans2(wtk_fst_state_t *s,wtk_fst_state_t *e,int out_id)
{
	wtk_fst_trans_t *trans;
	int b=0;

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->to_state==e && trans->out_id==out_id)
		{
			b=1;
			break;
		}
	}
	return b;
}


wtk_fst_state_t* wtk_fst_nbest_ent_to_net(wtk_fst_nbest_ent_t *n,wtk_fst_net2_t *dst,int *frame)
{
	//wtk_fst_net_cfg_t *cfg=dst->cfg;
	wtk_fst_trans2_t *trans,*t;
	wtk_fst_state_t *s,*e;
	wtk_fst_state_t *rs;
	int b;

	if(n->path_prev)
	{
		rs=wtk_fst_nbest_ent_to_net(n->path_prev,dst,frame);
	}else
	{
		rs=NULL;
	}
	trans=(wtk_fst_trans2_t*)(n->arc->trans);
	if(trans->out_id==0)
	{
		//wtk_debug("[%d/%d]\n",trans->in_id,trans->out_id);
	}else
	{
		//wtk_debug("rs=%p\n",rs);
		if(!rs)
		{
			if(!trans->from_state->hook)
			{
				s=(wtk_fst_state_t*)wtk_fst_net2_pop_state(dst);
				trans->from_state->hook=s;
			}else
			{
				s=(wtk_fst_state_t*)trans->from_state->hook;
			}
		}else
		{
			s=rs;
		}
		if(!trans->to_state->hook)
		{
			e=(wtk_fst_state_t*)wtk_fst_net2_pop_state(dst);
			trans->to_state->hook=e;
		}else
		{
			e=(wtk_fst_state_t*)trans->to_state->hook;
		}
		/*
		wtk_debug("[%.*s]:%d %d=%d hook=%p frame=%d s=%p e=%p\n",
				cfg->sym_out->strs[trans->out_id]->len,
				cfg->sym_out->strs[trans->out_id]->data,
				trans->frame,trans->from_state->id,trans->to_state->id,trans->from_state->hook,
				trans->frame,s,e);
		*/
		b=wtk_fst_state_has_trans2(s,e,trans->out_id);
		if(!b)
		{
			t=wtk_fst_net2_pop_trans(dst);
			t->from_state=s;
			t->to_state=e;
			t->frame=trans->frame;
			t->hook.next=s->v.trans;
			t->in_id=trans->in_id;
			t->out_id=trans->out_id;
			t->weight=trans->weight;
			s->v.trans=(wtk_fst_trans_t*)t;
			if(n->hook2)
			{
				t->hook2=n->hook2;
			}
		}
		rs=e;
	}
	if(frame && trans->out_id>0 && trans->frame>*frame)
	{
		*frame=trans->frame;
	}
	return rs;
}

wtk_fst_state_t* wtk_fst_net2_attached_state(wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;
	wtk_fst_state_t *t;

	if(s->hook)
	{
		return (wtk_fst_state_t*)s->hook;
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(trans->in_id==0)
		{
			t=wtk_fst_net2_attached_state(trans->to_state);
			if(t)
			{
				return t;
			}
		}
	}
	return NULL;
}

int wtk_fst_net2_nbest_mark_wrd(wtk_fst_net2_t *net,int index)
{
	wtk_fst_nbest_ent_t *ent;
	int i,j;
	int cnt;

	//wtk_debug("mark %d\n",net->nbest_cnt);
	if(net->nbest_cnt<=0)
	{
		return -1;
	}
	cnt=0;
	for(i=0;i<net->nbest_cnt;++i)
	{
		ent=net->nbest[i];
		j=0;
		while(ent && ent->path_prev)
		{
			if(ent->arc->trans->out_id)
			{
				++j;
				if(j==index)
				{
					//wtk_debug("mark ent %p\n",ent);
					ent->hook2=ent;
					++cnt;
					break;
				}
			}
			ent=ent->path_prev;
		}
	}
	return cnt;
}

int wtk_fst_net2_nbest_to_net(wtk_fst_net2_t *net,wtk_fst_net2_t *dst)
{
	wtk_fst_state_t *s;
	wtk_fst_trans2_t *t;
	int i,frame,b;

	if(net->nbest_cnt<=0)
	{
		wtk_debug("none nbest\n");
		return -1;
	}
	wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
	dst->start=dst->end=NULL;
	for(i=0;i<net->nbest_cnt;++i)
	{
		frame=0;
		s=wtk_fst_nbest_ent_to_net(net->nbest[i],dst,&frame);
		if(s)
		{
			if(!dst->end)
			{
				dst->end=wtk_fst_net2_pop_state(dst);
				dst->end->type=WTK_FST_FINAL_STATE;
				dst->end->v.weight=0;
			}
			b=wtk_fst_state_has_trans2(s,(wtk_fst_state_t*)dst->end,0);
			if(!b)
			{
				t=wtk_fst_net2_pop_trans(dst);
				t->from_state=s;
				t->frame=frame;
				t->to_state=(wtk_fst_state_t*)dst->end;
				t->hook.next=s->v.trans;
				t->in_id=0;
				t->out_id=0;
				t->weight=0;
				s->v.trans=(wtk_fst_trans_t*)t;
			}
		}
	}
	//dst->start=(wtk_fst_state_t*)net->start->hook;
	dst->start=(wtk_fst_state2_t*)wtk_fst_net2_attached_state((wtk_fst_state_t*)net->start);
	//wtk_debug("s=%d t=%d %p=>%p\n",dst->state_id,dst->trans_id,dst->start,dst->end);
	//wtk_fst_net2_print(dst);
	//wtk_fst_net2_print_lat(dst,stdout);
	wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
	if(dst->end && dst->start)
	{
		return 0;
	}else
	{
		wtk_debug("none end or start %p=>%p\n",dst->start,dst->end);
		return -1;
	}
}

void wtk_fst_nbest_ent_print_buf(wtk_fst_nbest_ent_t *n,wtk_fst_net_cfg_t *cfg,wtk_strbuf_t *buf,
		char *sep,int sep_bytes)
{
	wtk_string_t *v;

	if(n->path_prev)
	{
		wtk_fst_nbest_ent_print_buf(n->path_prev,cfg,buf,sep,sep_bytes);
	}
	if(wtk_fst_net_cfg_is_visible_wrd(cfg,n->arc->trans->out_id))
	{
		//wtk_debug("id=%d,nid=%d\n",n->arc->trans->out_id,cfg->sym_out->nstrs);
		v=cfg->sym_out->strs[n->arc->trans->out_id];
		if(sep_bytes>0 && buf->pos>0)
		{
			wtk_strbuf_push(buf,sep,sep_bytes);
		}
		wtk_strbuf_push(buf,v->data,v->len);
		//wtk_strbuf_push_f(buf,"[%p]",n->arc->trans);
	}
}

void wtk_fst_net2_get_nbest_one_path(wtk_fst_net2_t *net,wtk_strbuf_t *buf,char *sep,int bytes)
{
	wtk_strbuf_reset(buf);
	if(net->nbest_cnt<=0){return;}
	wtk_fst_nbest_ent_print_buf(net->nbest[0],net->cfg,buf,sep,bytes);
}

wtk_fst_rec_trans_t* wtk_fst_net2_get_short_one_best_path2(wtk_fst_net2_t *net,wtk_heap_t *heap,char *sep,int bytes)
{
	wtk_fst_net2_pthem_t *item;
	wtk_strbuf_t *buf=net->buf;
	wtk_fst_trans_t *trans;
	wtk_fst_rec_trans_t *rtrans;
	wtk_string_t **strs=net->cfg->sym_out->strs;
	wtk_string_t *v;
	wtk_fst_rec_item_t *itemx;
	double d;

	//wtk_fst_net2_print_best_path(net);
	if(!net->end ||!net->end->hook)
	{
		return NULL;
	}
	wtk_strbuf_reset(buf);
	item=(wtk_fst_net2_pthem_t*)net->end->hook;
	d=0;
	while(item->min_input_trans)
	{
		trans=item->min_input_trans;
		d+=trans->weight;
		//if(trans->out_id>2)
		if(wtk_fst_net_cfg_is_visible_wrd(net->cfg,trans->out_id))
		{
			v=strs[trans->out_id];
			if(bytes>0 && buf->pos>0)
			{
				wtk_strbuf_push_front(buf,sep,bytes);
			}
			wtk_strbuf_push_front(buf,v->data,v->len);
		}
		item=(wtk_fst_net2_pthem_t*)item->from_state->hook;
	}
	rtrans=(wtk_fst_rec_trans_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_rec_trans_t));
	rtrans->n=1;
	rtrans->recs=(wtk_fst_rec_item_t**)wtk_heap_malloc(heap,sizeof(wtk_fst_rec_item_t*)*rtrans->n);
	itemx=wtk_fst_rec_item_new(heap,buf->data,buf->pos);
	itemx->score=d;
	itemx->conf=-1;
	rtrans->recs[0]=itemx;
	return  rtrans;
}



wtk_fst_rec_trans_t* wtk_fst_net2_get_nbest(wtk_fst_net2_t *net,wtk_heap_t *heap,char *sep,int bytes)
{
	wtk_fst_rec_trans_t *trans;
	wtk_strbuf_t *buf=net->buf;
	wtk_fst_rec_item_t *item;
	int i;

	if(net->nbest_cnt<=0)
	{
		return wtk_fst_net2_get_short_one_best_path2(net,heap,sep,bytes);
	}
	trans=(wtk_fst_rec_trans_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_rec_trans_t));
	trans->n=net->nbest_cnt;
	trans->recs=(wtk_fst_rec_item_t**)wtk_heap_malloc(heap,sizeof(wtk_fst_rec_item_t*)*trans->n);
	for(i=0;i<trans->n;++i)
	{
		wtk_strbuf_reset(buf);
		wtk_fst_nbest_ent_print_buf(net->nbest[i],net->cfg,buf,sep,bytes);
		item=wtk_fst_rec_item_new(heap,buf->data,buf->pos);
		item->score=net->nbest[i]->score;
		item->conf=-1;
		trans->recs[i]=item;
	}
	return trans;
}

void wtk_fst_net3_print_nbest(wtk_fst_net2_t *net)
{
	wtk_strbuf_t *buf;
	int i;

	buf=wtk_strbuf_new(256,1);
	for(i=0;i<net->nbest_cnt;++i)
	{
		wtk_strbuf_reset(buf);
		wtk_fst_nbest_ent_print_buf(net->nbest[i],net->cfg,buf," ",1);
		wtk_debug("v[%d]=[%.*s] %f\n",i,buf->pos,buf->data,net->nbest[i]->like);
	}
	wtk_strbuf_delete(buf);
}

wtk_fst_rec_item_t* wtk_fst_rec_trans_find(wtk_fst_rec_trans_t *trans,char *data,int bytes)
{
	wtk_fst_rec_item_t *item;
	int i;

	for(i=0;i<trans->n;++i)
	{
		item=trans->recs[i];
		if(wtk_string_cmp(item->str,data,bytes)==0)
		{
			return item;
		}
	}
	return NULL;
}

void wtk_fst_rec_trans_print(wtk_fst_rec_trans_t *trans)
{
	int i;

	wtk_debug("==========================================\n");
	for(i=0;i<trans->n;++i)
	{
		if(trans->recs[i] && trans->recs[i]->str)
		{
			printf("v[%d]=[%.*s] %f\n",i,trans->recs[i]->str->len,trans->recs[i]->str->data,
				trans->recs[i]->score);
		}else
		{
			printf("v[%d]=[nil] %f\n",i,trans->recs[i]->score);
		}
	}
}


//----------------------------- net2 unique ----------------------
void wtk_fst_net_check_dup_trans(wtk_fst_net2_t *net,wtk_fst_trans_t *list,wtk_fst_trans_t *t)
{
	wtk_fst_trans_t *trans;

	for(trans=list;trans;trans=trans->hook.next)
	{
		if(trans->out_id==t->out_id && trans->weight==t->weight && trans->to_state==t->to_state)
		{
			wtk_debug("[%.*s]: %f to=[%p:%p]\n",
					net->cfg->sym_out->strs[t->out_id]->len,
					net->cfg->sym_out->strs[t->out_id]->data,
					t->weight,trans->to_state,t->to_state);
			exit(0);
		}
	}
}

void wtk_fst_net2_check_state(wtk_fst_net2_t *net,wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;

	s->hook=(void*)s;
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		wtk_fst_net_check_dup_trans(net,trans->hook.next,trans);
	}

	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(!trans->to_state->hook)
		{
			wtk_fst_net2_check_state(net,trans->to_state);
		}
	}
}

int wtk_fst_net2_unique(wtk_fst_net2_t *net)
{
	wtk_fst_net2_check_state(net,(wtk_fst_state_t*)net->start);
	return 0;
}

//----------------------- link net --------------------

typedef struct
{
	wtk_fst_state_t *s;
	wtk_fst_state_t *e;
	wtk_larray_t *e_array;
	//wtk_larray_t *s_array;
}wtk_fst_net2_dup_info_t;

typedef enum
{
	WTK_FST_NET2_DUP_INIT,
	WTK_FST_NET2_DUP_APPEND,
	WTK_FST_NET2_DUP_END,
	WTK_FST_NET2_DUP_ALL,
}wtk_fst_net2_dup_type_t;

wtk_fst_state_t* wtk_fst_net2_dup_state(wtk_fst_net2_t *net2,wtk_fst_net2_dup_info_t *info,wtk_fst_state_t *src,wtk_heap_t *heap,
		wtk_fst_net2_dup_type_t type)
{
	wtk_fst_state_t *dst;
	wtk_fst_trans2_t *trans,*t;

	//wtk_debug("src=%p\n",src);
	dst=(wtk_fst_state_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_state_t));
	*dst=*src;
	dst->hook=NULL;
	src->hook=dst;
	//wtk_debug("id=%d type=%d\n",src->id,src->type);
	dst->id=net2->state_id++;
	//wtk_debug("id=%d src=%d\n",dst->id,src->id);
	if(src->id==0)
	{
		info->s=dst;
	}
	if(src->type==WTK_FST_NORM_STATE)
	{
		dst->v.trans=NULL;
		for(trans=(wtk_fst_trans2_t*)(src->v.trans);trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
		{
			t=(wtk_fst_trans2_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_trans2_t));
			*t=*trans;
			t->frame=trans->frame;
			t->hook.next=dst->v.trans;
			dst->v.trans=(wtk_fst_trans_t*)t;
			++net2->trans_id;
			if(trans->to_state->hook)
			{
				t->to_state=(wtk_fst_state_t*)trans->to_state->hook;
			}else
			{
				t->to_state=wtk_fst_net2_dup_state(net2,info,trans->to_state,heap,type);
			}
			t->from_state=src;
			if(t->out_id==net2->cfg->snt_end_id)//1)
			{
				wtk_larray_push2(info->e_array,&t);
			}
			switch(type)
			{
			case WTK_FST_NET2_DUP_INIT:
				/*
				if(t->out_id==1)
				{
					t->out_id=0;
					t->in_id=0;
				}*/
				break;
			case WTK_FST_NET2_DUP_APPEND:
				if(t->out_id==net2->cfg->snt_start_id) //2)// || t->out_id==1)
				{
					t->out_id=0;
					t->in_id=0;
				}
				break;
			case WTK_FST_NET2_DUP_END:
				if(t->out_id==net2->cfg->snt_start_id)//2)
				{
					t->out_id=0;
					t->in_id=0;
				}
				break;
			case WTK_FST_NET2_DUP_ALL:
				break;
			}
		}
	}else if(src->type==WTK_FST_FINAL_STATE)
	{
		/*
		if(type==WTK_FST_NET2_DUP_END || type==WTK_FST_NET2_DUP_ALL)
		{
			dst->type=WTK_FST_FINAL_STATE;
		}else
		{
			dst->type=WTK_FST_NORM_STATE;
		}*/
		//dst->type=WTK_FST_FINAL_STATE;
		//wtk_debug("found end %p t=%d\n",dst,dst->type);
		info->e=dst;
	}
	return dst;
}

void wtk_fst_net2_dup_state2(wtk_fst_net2_t *net2,wtk_fst_net2_dup_info_t *info,wtk_fst_state_t *src,
		wtk_heap_t *heap,wtk_fst_net2_dup_type_t type)
{
	//wtk_fst_trans_t *trans;

	//wtk_debug("src=%p\n",src);
	wtk_fst_net2_dup_state(net2,info,src,heap,type);
	if(info->s && info->s->v.trans && !info->s->v.trans->hook.next && info->s->v.trans->out_id==0)
	{
		info->s=info->s->v.trans->to_state;
	}
	/*
	if(info->s)
	{
		for(trans=info->s->v.trans;trans;trans=trans->hook.next)
		{
			wtk_larray_push2(info->s_array,&trans);
		}
	}*/
}

void wtk_fst_net2_link_node(wtk_fst_net2_t *net,wtk_fst_trans2_t **ptrans,int ntrans,wtk_fst_state_t *e)
{
	wtk_fst_trans2_t *trans,*t,*t2;
	wtk_heap_t *heap=net->heap;
	wtk_fst_state_t *s;
	int i,j;

	for(i=0;i<ntrans;++i)
	{
		trans=ptrans[i];
		s=trans->from_state;
		for(j=0,t=(wtk_fst_trans2_t*)e->v.trans;t;t=(wtk_fst_trans2_t*)t->hook.next,++j)
		{

			wtk_debug("v[%d/%d]=%p [%.*s] %f/%f\n",i,j,trans,
					net->cfg->sym_out->strs[t->out_id]->len,
					net->cfg->sym_out->strs[t->out_id]->data,
					trans->weight,t->weight);
			if(j==0)
			{
				trans->to_state=t->to_state;
				trans->in_id=t->in_id;
				trans->out_id=t->out_id;
				trans->weight+=t->weight;
			}else
			{
				t2=(wtk_fst_trans2_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_trans2_t));
				t2->from_state=s;
				t2->to_state=t->to_state;
				t2->in_id=t->in_id;
				t2->out_id=t->out_id;
				t2->weight=trans->weight+t->weight;
				t2->hook.next=s->v.trans;
				s->v.trans=(wtk_fst_trans_t*)t2;
			}
		}
	}
}


int wtk_fst_net2_link_remove_s(wtk_fst_net2_t *net,wtk_fst_trans2_t **ptrans,int ntrans)
{
	wtk_fst_trans2_t *trans;
	int i;
	int frame;

	if(ntrans>0)
	{
		frame=ptrans[0]->frame;
	}else
	{
		frame=-1;
	}
	for(i=0;i<ntrans;++i)
	{
		trans=ptrans[i];
		trans->in_id=trans->out_id=0;
	}
	return frame;
}

int wtk_fst_net2_link(wtk_fst_net2_t *net1,wtk_fst_net2_t *net2,int is_end)
{
	wtk_fst_net2_dup_info_t info;
	wtk_fst_net2_dup_type_t type;
	wtk_fst_trans2_t *trans;
	int frame;

	//wtk_debug("s=%p\n",net2->start);
	//wtk_fst_net2_print(net2);
	//wtk_debug("is_end=%d s=%p\n",is_end,net1->s_array);
	if(!net1->e_array)
	{
		wtk_fst_net2_attach_array(net1);
	}
	//wtk_debug("is_end=%d s=%p/%p/%p\n",is_end,net1->s_array,net1->tmp1_array,net1->tmp2_array);
	info.s=(wtk_fst_state_t*)net2->start;
	info.e=NULL;
	//info.s_array=net1->tmp1_array;
	info.e_array=net1->tmp1_array;
	//wtk_larray_reset(info.s_array);
	wtk_larray_reset(info.e_array);
	if(!net1->start)
	{
		if(is_end)
		{
			type=WTK_FST_NET2_DUP_ALL;
		}else
		{
			type=WTK_FST_NET2_DUP_INIT;
		}
		//wtk_debug("========= use =========\n");
		wtk_fst_net2_dup_state2(net1,&info,(wtk_fst_state_t*)net2->start,net1->heap,type);
		net1->start=(wtk_fst_state2_t*)info.s;
		net1->end=(wtk_fst_state2_t*)info.e;
	}else
	{
		if(is_end)
		{
			type=WTK_FST_NET2_DUP_END;
		}else
		{
			type=WTK_FST_NET2_DUP_APPEND;
		}
		wtk_fst_net2_dup_state2(net1,&info,(wtk_fst_state_t*)net2->start,net1->heap,type);
		//wtk_fst_net2_link_node(net1,net1->end,info.s);
		/*
		wtk_fst_net2_link_node(net1,(wtk_fst_trans2_t**)net1->e_array->slot,
				net1->e_array->nslot,info.s);
		*/
		//wtk_debug("s=%p e=%p\n",info.s,info.e);
		frame=wtk_fst_net2_link_remove_s(net1,(wtk_fst_trans2_t**)net1->e_array->slot,
				net1->e_array->nslot);
		trans=(wtk_fst_trans2_t*)wtk_heap_malloc(net1->heap,sizeof(wtk_fst_trans2_t));
		trans->frame=frame;
		++net1->trans_id;
		trans->from_state=(wtk_fst_state_t*)net1->end;
		trans->to_state=(wtk_fst_state_t*)info.s;
		trans->hook.next=NULL;
		trans->in_id=0;
		trans->out_id=0;
		trans->weight=0;
		net1->end->v.trans=(wtk_fst_trans_t*)trans;
		net1->end->type=WTK_FST_NORM_STATE;
		net1->end=(wtk_fst_state2_t*)info.e;
	}
	if(net1->end)
	{
		net1->end->type=WTK_FST_FINAL_STATE;
	}
	net1->tmp1_array=net1->e_array;
	//net1->tmp2_array=net1->s_array;
	//net1->s_array=info.s_array;
	net1->e_array=info.e_array;
	//net1->end->type=WTK_FST_FINAL_STATE;
	//wtk_fst_net2_print(net1);
	//exit(0);
	//wtk_debug("net1=%p s=%p e=%p:%d\n",net1,net1->start,net1->end,net1->end->type);
	//exit(0);
	return 0;
}

int wtk_fst_net2_from_lat(wtk_fst_net2_t *net,wtk_lat_t *lat,wtk_str_hash_t *sym)
{
	wtk_heap_t *heap=net->heap;
	wtk_fst_state2_t *state;
	wtk_fst_trans_t *trans;
	wtk_lnode_t *n;
	wtk_larc_t *arc;
	int i,j;
	int id;
	wtk_fst_state2_t *root;
	int cnt=0;
	wtk_string_t *v;

	//wtk_debug("N=%d,L=%d\n",lat->n,lat->l);
	root=(wtk_fst_state2_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_state2_t)*lat->nn);
	//net->start=root;
	for(i=0,j=1;i<lat->nn;++i)
	{
		//wtk_debug("i=%d/%d\n",i,lat->n)
		state=root+i;
		state->type=WTK_FST_NORM_STATE;
		state->v.trans=0;
		state->hook=NULL;
		n=lat->lnodes+i;
		n->hook=state;
		if(!n->pred)
		{
			net->start=state;
			state->id=0;
		}else
		{
			state->id=j;
			++j;
		}
	}
	for(i=0;i<lat->nn;++i)
	{
		//wtk_debug("i=%d/%d\n",i,lat->n)
		//state=root+i;
		n=lat->lnodes+i;
		state=(wtk_fst_state2_t*)n->hook;
		//wtk_debug("v[%d]=%d,%p\n",i,state->id,n->foll);
		for(arc=n->foll;arc;arc=arc->farc)
		{
			trans=(wtk_fst_trans_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_trans_t));
			n=arc->end;
			if(n->info.word && n->info.word->name)
			{
				v=(wtk_string_t*)wtk_str_hash_find(sym,n->info.word->name->data,
						n->info.word->name->len);
				if(v)
				{
					id=wtk_str_atoi(v->data,v->len);
					//wtk_debug("[%.*s]=%d\n",n->info.word->name->len,n->info.word->name->data,id);
				}else
				{
					id=0;
				}
			}else
			{
				id=0;
			}
			//wtk_debug("id=%d, [%.*s]\n",id,n->wrd->len,n->wrd->data);
			trans->in_id=id;
			trans->out_id=id;
			//trans->weight=-(arc->ac_like+arc->lm_like*14.0);
			trans->weight=(arc->aclike);
			//trans->from_state=state;
			//wtk_debug("S=%d E=%d a=%f l=%f\n",i,arc->end->i,arc->ac_like,arc->lm_like);
			trans->to_state=(wtk_fst_state_t*)n->hook;
			trans->hook.next=state->v.trans;
			state->v.trans=trans;
			++cnt;
		}
		if(!n->foll)
		{
			state=(wtk_fst_state2_t*)n->hook;
			state->type=WTK_FST_FINAL_STATE;
			state->v.weight=0;
			net->end=state;
		}
		/*
		if(!n->pred)
		{
			net->start=(wtk_fst_state_t*)n->hook;
		}*/
	}
	net->trans_id=cnt;
	net->state_id=lat->nn;
	//state=root+lat->nn-1;
	//state->type=WTK_FST_FINAL_STATE;
	//state->v.weight=0;
	//net->end=state;
	//exit(0);
	//wtk_debug("%p\n",net->start);
	//exit(0);
	return 0;
}


int wtk_fst_net2_from_lat2(wtk_fst_net2_t *net,wtk_lat_t *lat)
{
	wtk_heap_t *heap=net->heap;
	wtk_fst_state2_t *state;
	//wtk_fst_trans2_t *trans;
	wtk_lnode_t *n;
	wtk_larc_t *arc;
	int i,j;
	int id;
	wtk_fst_state2_t *root;
	int cnt=0;

	//wtk_debug("N=%d,L=%d\n",lat->n,lat->l);
	root=(wtk_fst_state2_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_state2_t)*lat->nn);
	//net->start=root;
	for(i=0,j=1;i<lat->nn;++i)
	{
		//wtk_debug("i=%d/%d\n",i,lat->n)
		state=root+i;
		state->type=WTK_FST_NORM_STATE;
		state->v.trans=0;
		state->hook=NULL;
		n=lat->lnodes+i;
		n->hook=state;
		if(!n->pred)
		{
			net->start=state;
			state->id=0;
		}else
		{
			state->id=j;
			++j;
		}
	}
	for(i=0;i<lat->nn;++i)
	{
		//wtk_debug("i=%d/%d\n",i,lat->n)
		//state=root+i;
		n=lat->lnodes+i;
		state=(wtk_fst_state2_t*)n->hook;
		//wtk_debug("v[%d]=%d,%p\n",i,state->id,n->foll);
		for(arc=n->foll;arc;arc=arc->farc)
		{
			n=arc->end;
			if(n->info.word && n->info.word->name)
			{
				id=(long)n->info.word->aux;
			}else
			{
				id=0;
			}
			//wtk_debug("id=%d, [%.*s]\n",id,n->wrd->len,n->wrd->data);
			wtk_fst_net2_link_state(net,state,(wtk_fst_state2_t*)n->hook,0,id,id,arc->lmlike,arc->aclike);
			++cnt;
		}
		if(!n->foll)
		{
			state=(wtk_fst_state2_t*)n->hook;
			state->type=WTK_FST_FINAL_STATE;
			state->v.weight=0;
			net->end=state;
		}
		/*
		if(!n->pred)
		{
			net->start=(wtk_fst_state_t*)n->hook;
		}*/
	}
	net->trans_id=cnt;
	net->state_id=lat->nn;
	//state=root+lat->nn-1;
	//state->type=WTK_FST_FINAL_STATE;
	//state->v.weight=0;
	//net->end=state;
	//exit(0);
	//wtk_debug("%p\n",net->start);
	//exit(0);
	return 0;
}

int wtk_fst_trans_cmp(const void *src1,const void *src2)
{
	wtk_fst_trans_t *trans1,*trans2;

	trans1=*(wtk_fst_trans_t**)src1;
	trans2=*(wtk_fst_trans_t**)src2;

	//wtk_debug("[%d/%d]\n",trans1->in_id,trans2->in_id);
	if(trans1->in_id>trans2->in_id)
	{
		return 1;
	}else
	{
		return -1;
	}
}

void wtk_fst_net2_print_fsm_state(wtk_fst_net2_t *net,wtk_fst_state_t *s,wtk_strbuf_t *buf,wtk_larray_t *a)
{
	wtk_fst_trans_t *trans;
	wtk_fst_trans_t **ptrans;
	//wtk_string_t *v;
	int i;

	//wtk_debug("%d=%d hook=%p\n",s->id,s->type,s->hook)
	s->hook=s;
	if(s->type==WTK_FST_FINAL_STATE)
	{
		//printf("%d\n",s->id);
		wtk_strbuf_push_f(buf,"%d\n",s->id);
		return;
	}
	wtk_larray_reset(a);
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		wtk_larray_push2(a,&(trans));
		/*
		if(trans->weight!=0)
		{
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
		}else
		{
			//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
			wtk_strbuf_push_f(buf,"%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
		}*/
	}
	ptrans=(wtk_fst_trans_t**)a->slot;
	qsort(ptrans,a->nslot,sizeof(wtk_fst_trans_t*),wtk_fst_trans_cmp);
	for(i=0;i<a->nslot;++i)
	{
		trans=ptrans[i];
		if(trans->weight!=0)
		{
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
//			if(net->print)
//			{
//				v=wtk_fst_net_print_get_outsym(net->print,trans->out_id);
//				wtk_strbuf_push_f(buf,"%d %d %d %.*s %f\n",s->id,trans->to_state->id,trans->in_id,
//									v->len, v->data,trans->weight);
//			}else
//			{
				wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,
									trans->out_id,trans->weight);
//			}

		}else
		{
			//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
//			if(net->print)
//			{
//				v=wtk_fst_net_print_get_outsym(net->print,trans->out_id);
//				wtk_strbuf_push_f(buf,"%d %d %d %.*s\n",s->id,trans->to_state->id,trans->in_id,
//									v->len, v->data);
//			}else
//			{
				wtk_strbuf_push_f(buf,"%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,
									trans->out_id);
//			}
		}
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		//wtk_debug("to=%p\n",trans->to_state->hook);
		if(!trans->to_state->hook)
		{
			wtk_fst_net2_print_fsm_state(net,trans->to_state,buf,a);
		}
	}
}

/**
 * add by dmd
 * support start and end due to same node.
 */
void wtk_fst_net2_print_fsm_state2(wtk_fst_net2_t *net,wtk_fst_state_t *s,wtk_strbuf_t *buf,wtk_larray_t *a)
{
	wtk_fst_trans_t *trans;
	wtk_fst_trans_t **ptrans;
	//wtk_string_t *v;
	int i;

	//wtk_debug("%d=%d hook=%p\n",s->id,s->type,s->hook)
	if (s->hook) return;
	s->hook=s;
	if(s->type==WTK_FST_FINAL_STATE)
	{
		//printf("%d\n",s->id);
		wtk_strbuf_push_f(buf,"%d\n",s->id);
	}
	wtk_larray_reset(a);
	for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
	{
		wtk_larray_push2(a,&(trans));
		/*
		if(trans->weight!=0)
		{
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
			wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
		}else
		{
			//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
			wtk_strbuf_push_f(buf,"%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
		}*/
	}
	ptrans=(wtk_fst_trans_t**)a->slot;
	qsort(ptrans,a->nslot,sizeof(wtk_fst_trans_t*),wtk_fst_trans_cmp);
	//wtk_debug("nslot=%d\n", a->nslot);
	for(i=0;i<a->nslot;++i)
	{
		trans=ptrans[i];
		if(trans->weight!=0)
		{
			//printf("%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
//			if(net->print)
//			{
//				v=wtk_fst_net_print_get_outsym(net->print,trans->out_id);
//				wtk_strbuf_push_f(buf,"%d %d %d %.*s %f\n",s->id,trans->to_state->id,trans->in_id,
//									v->len, v->data,trans->weight);
//			}else
//			{
				wtk_strbuf_push_f(buf,"%d %d %d %d %f\n",s->id,trans->to_state->id,trans->in_id,
									trans->out_id,trans->weight);
//			}

		}else
		{
			//printf("%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,trans->out_id);
//			if(net->print)
//			{
//				v=wtk_fst_net_print_get_outsym(net->print,trans->out_id);
//				wtk_strbuf_push_f(buf,"%d %d %d %.*s\n",s->id,trans->to_state->id,trans->in_id,
//									v->len, v->data);
//			}else
//			{
				wtk_strbuf_push_f(buf,"%d %d %d %d\n",s->id,trans->to_state->id,trans->in_id,
									trans->out_id);
//			}
		}
	}
	//wtk_debug("trans=%d\n", s->ntrans);
	for(i=0,trans=s->v.trans;i<s->ntrans;++i,++trans)
	{
		//wtk_debug("to=%p\n",trans->to_state->hook);
		if(!trans->to_state->hook)
		{
			wtk_fst_net2_print_fsm_state2(net,trans->to_state,buf,a);
		}
	}
}

void wtk_fst_net2_print_fsm(wtk_fst_net2_t *net,wtk_strbuf_t *buf)
{
	wtk_larray_t *a;

	wtk_fst_net2_clean_hook2(net);
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_t *));
	wtk_strbuf_reset(buf);
	wtk_fst_net2_print_fsm_state(net,(wtk_fst_state_t*)net->start,buf,a);
	//wtk_strbuf_push_s(buf,"X==A");
	wtk_larray_delete(a);
}

void wtk_fst_net2_print_fsm2(wtk_fst_net2_t *net)
{
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_fst_net2_print_fsm(net,buf);
	printf("%.*s\n",buf->pos,buf->data);
	wtk_strbuf_delete(buf);
}

/**
 * add by dmd
 */
void wtk_fst_net2_print_fsm3(wtk_fst_net2_t *net)
{
	wtk_larray_t *a;
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(1024,1);
	wtk_fst_state_clean_hook(((wtk_fst_net_t*)net)->init_state);   //using tag isvisited.
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_t *));
	wtk_strbuf_reset(buf);
	wtk_fst_net2_print_fsm_state2(net,((wtk_fst_net_t*)net)->init_state,buf,a);
	//wtk_strbuf_push_s(buf,"X==A");
	wtk_larray_delete(a);
	printf("########\n%.*s\n",buf->pos,buf->data);
	wtk_strbuf_delete(buf);
}

//--------------------- print lattice --------------------
typedef struct
{
	int n;
	int l;
}wtk_fst_net2_info_t;

typedef struct wtk_fst_trans_lat_item wtk_fst_trans_lat_item_t;

struct wtk_fst_trans_lat_item
{
	wtk_fst_net_cfg_t *net_cfg;
	wtk_fst_trans2_t *trans;
	wtk_fst_state_t *from;
	wtk_fst_trans_lat_item_t *next;
	int index;
};

int wtk_fst_state_next_trans(wtk_fst_state_t *s)
{
	wtk_fst_trans_t* trans;
	int i;

	if(s->type!=WTK_FST_NORM_STATE){return 0;}
	for(i=0,trans=s->v.trans;trans;trans=trans->hook.next,++i)
	{
	}
	return i;
}

wtk_fst_net2_info_t wtk_fst_net2_nodes(wtk_fst_net2_t *net,wtk_fst_state_t *s,
		wtk_fst_state_t **ps,int n,wtk_larray_t *a)
{
	wtk_fst_trans_lat_item_t *item;
	wtk_fst_trans_t *trans;
	wtk_fst_net2_info_t info,t;

	//wtk_debug("[%d]\n",s->id);
	ps[s->id]=s;
	info.n=1;
	info.l=0;
	if(s->type==WTK_FST_NORM_STATE)
	{
		for(trans=s->v.trans;trans;trans=trans->hook.next)
		{
			//wtk_debug("s->id=%d s->tostate=%d in_id=%d out_id=%d\n", s->id, trans->to_state->id, trans->in_id, trans->out_id);
			item=(wtk_fst_trans_lat_item_t*)wtk_heap_malloc(net->heap,sizeof(wtk_fst_trans_lat_item_t));
			item->from=s;
			item->trans=(wtk_fst_trans2_t*)trans;
			item->net_cfg=net->cfg;
			item->index=0;
			if (a->nslot > a->slot_alloc-1)
			{
				wtk_debug("warning:over info max: %d\n", a->nslot);
				break;
			}
			wtk_larray_push2(a,&(item));
			if(!ps[trans->to_state->id])
			{
				t=wtk_fst_net2_nodes(net,trans->to_state,ps,n,a);
				info.n+=t.n;
				info.l+=t.l;
			}
			info.l+=wtk_fst_state_next_trans(trans->to_state);
		}
	}
	//wtk_debug("info: n=%d l=%d\n", info.n, info.l);

	return info;
}

int wtk_fst_net2_cmp_arc(const void *p1,const void *p2)
{
	wtk_fst_trans_lat_item_t **src=(wtk_fst_trans_lat_item_t**)p1;
	wtk_fst_trans_lat_item_t **dst=(wtk_fst_trans_lat_item_t**)p2;
	wtk_fst_trans_lat_item_t *item,*item2;
	wtk_fst_net_cfg_t *cfg;

	item=*src;
	item2=*dst;
	cfg=item->net_cfg;
	//wtk_debug("item=%p/%p %p/%p\n",item,item2,src,dst);
	if(item2->trans->frame>item->trans->frame)
	{
		return -1;
	}else if(item2->trans->frame==item->trans->frame)
	{
		if(item2->trans->out_id==cfg->eps_id)
		{
			if(item->trans->out_id==cfg->snt_end_id)
			{
				return -1;
			}
		}
		if(item->trans->out_id==cfg->eps_id)
		{
			if(item2->trans->out_id==cfg->snt_start_id)
			{
				return -1;
			}
		}
		if(item2->trans->out_id>item->trans->out_id)
		{
			return -1;
		}
	}
	return 1;
}

/**
 * add path for leak function
 * only for single path.
 * by dmd 2020.02.21
 */
int wtk_fst_net2_addleakpath(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post)
{
	wtk_fst_trans_lat_item_t **pitem,*item,*item2;
	wtk_larray_t *a;
	wtk_fst_net2_info_t info;
	int i, j, n,  skip, tskip;
	wtk_fst_state_t **ps;
	wtk_fst_state2_t *s, *e;
	int in_id, out_id;
	wtk_fst_trans2_t* t;
	float lmlike;
	wtk_dict_word_t *dw;

	n=net->state_id;
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_lat_item_t*));
	//wtk_debug("trans=%d state=%d\n",net->trans_id,net->state_id);
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	info=wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
//	qsort(pitem,a->nslot,sizeof(wtk_fst_trans_lat_item_t*),wtk_fst_net2_cmp_arc);   //some bug

	if (info.n-4 >= xbnf_post->min_len)
	{
		skip = min((int)(xbnf_post->pct *  info.n-4), xbnf_post->max_skip);
		skip = max(skip, 1);
	}else
		skip = 0;
	//wtk_debug("skip=%d\n", skip);
	/**
	 * <s> </s> no consider
	 */
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];
		if(item->trans->from_state->id==0 || item->trans->to_state->type==WTK_FST_FINAL_STATE) continue;

		s = (wtk_fst_state2_t*)item->trans->from_state;
		//wtk_debug("from=%d to=%d in_id=%d\n",s->id, ((wtk_fst_state2_t*)item->trans->to_state)->id, item->trans->in_id);
		for(j=i+1, tskip = skip; j < a->nslot && tskip > 0; j++, tskip--)
		{
			item2=pitem[j];
			e = (wtk_fst_state2_t*)item2->trans->to_state;
			if(e->type == WTK_FST_FINAL_STATE) break;
			in_id = item2->trans->in_id;
			out_id = item2->trans->out_id;
			lmlike = xbnf_post->wrdpen + (skip-tskip+1) * xbnf_post->step_wrdpen;
			if(item2->trans->hook2)
			{
				dw = (wtk_dict_word_t*)item2->trans->hook2;
				if(dw->maxnphn<=2){
					lmlike*= xbnf_post->mpscale;
				}
			}
			//wtk_debug("lmlike=%f\n", lmlike*net->cfg->lmscale);
			t=wtk_fst_net2_link_state(net, s,e,0,in_id,out_id,lmlike * net->cfg->lmscale,0);
			//wtk_debug("add path[%d]: from_id=%d to_id=%d in_id=%d out_in_id=%d\n", j, s->id, e->id, in_id, out_id);
			t->hook2=item2->trans->hook2;
		}
	}

	wtk_free(ps);
	wtk_larray_delete(a);

	return 0;
}

/**
 * add path for leak self-loop
 * only for single path.
 * by dmd 2020.08.05
 */
int wtk_fst_net2_selfloop(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post)
{
	wtk_fst_trans_lat_item_t **pitem,*item, *titem;
	wtk_larray_t *a;
	int i, n, k;
	wtk_fst_state_t **ps;
	wtk_fst_state2_t *s, *e;
	int in_id, out_id;
	wtk_fst_trans2_t* t;
	float lmlike;

	in_id=out_id=-1;
	n=net->state_id;
	a=wtk_larray_new(56,sizeof(wtk_fst_trans_lat_item_t*));
	//wtk_debug("trans=%d state=%d\n",net->trans_id,net->state_id);
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
//	qsort(pitem,a->nslot,sizeof(wtk_fst_trans_lat_item_t*),wtk_fst_net2_cmp_arc);   //some bug

	/**
	 * <s> </s> no consider
	 */
	s=e=NULL;
	k=0;
	titem=NULL;
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];
		if(item->trans->from_state->id==0 || item->trans->to_state->type==WTK_FST_FINAL_STATE) continue;
		if(!s)
		{
			//s = (wtk_fst_state2_t*)item->trans->from_state;
			s = (wtk_fst_state2_t*)item->trans->to_state;
			in_id = item->trans->in_id;
			out_id = item->trans->out_id;
		}

		e = (wtk_fst_state2_t*)item->trans->to_state;
		//e = (wtk_fst_state2_t*)item->trans->from_state;
		k++;
		titem=item;
	}
	if(s && e){
		lmlike = xbnf_post->wrdpen + k * xbnf_post->step_wrdpen;
		t=wtk_fst_net2_link_state(net,e,s,0,in_id,out_id,lmlike* net->cfg->lmscale,0);
		//wtk_debug("add path[%d]: from_id=%d to_id=%d in_id=%d out_in_id=%d\n", i, s->id, e->id, in_id, out_id);
		t->hook2=titem->trans->hook2;
	}

	wtk_free(ps);
	wtk_larray_delete(a);

	return 0;
}

/**
 * expand for leak self-loop
 * only for single path.
 * only for single wrd, mang times -> wrd no.
 * by dmd 2020.08.13
 */
int wtk_fst_net2_expselfloop(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post)
{
	wtk_fst_trans_lat_item_t **pitem,*item,*titem;
	wtk_fst_trans_t *trans;
	wtk_fst_trans2_t *tran;
	wtk_larray_t *a;
	int i, n;
	wtk_fst_state_t **ps;
	wtk_fst_state2_t *s, *e, *si,*s_first, *newn, *eps,*es;
	int in_id, out_id;
	wtk_fst_trans2_t* t;
	float lmlike;

	n=net->state_id;
	a=wtk_larray_new(64,sizeof(wtk_fst_trans_lat_item_t*));
	//wtk_debug("trans=%d state=%d\n",net->trans_id,net->state_id);
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
//	qsort(pitem,a->nslot,sizeof(wtk_fst_trans_lat_item_t*),wtk_fst_net2_cmp_arc);   //some bug

	/**
	 * <s> </s> no consider
	 */
	s=e=es=s_first=NULL;
	titem=NULL;
	//for first word
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];

//		if (item->trans->to_state->type==WTK_FST_FINAL_STATE)
//		{
//			es=(wtk_fst_state2_t*)item->trans->to_state;
//			tran=item->trans;
//			t=wtk_fst_net2_link_state(net, es,es, 0, tran->in_id, tran->out_id, tran->lm_like, tran->weight);
//			t->hook2=tran->hook2;
//		}


		if(item->trans->from_state->id==0 || item->trans->to_state->type==WTK_FST_FINAL_STATE) continue;
		if(!s)
		{
			//s = (wtk_fst_state2_t*)item->trans->from_state;
			s = (wtk_fst_state2_t*)item->trans->to_state;
			in_id = item->trans->in_id;
			out_id = item->trans->out_id;
			titem=item;
			break;
		}
	}
	//expand net
	if(s){
		i=1;
		if (xbnf_post->max_selfloop > 1) {
			i++;
			newn=wtk_fst_net2_pop_state(net);
			eps=wtk_fst_net2_pop_state(net);
			for(trans=s->v.trans;trans;trans=trans->hook.next)
			{
				tran=(wtk_fst_trans2_t*)trans;
				t=wtk_fst_net2_link_state(net, eps,(wtk_fst_state2_t*)tran->to_state, 0, tran->in_id, tran->out_id, tran->lm_like, tran->weight);
				//wtk_debug("in_id=%d out_id=%d\n", tran->in_id, tran->out_id);
				//t=wtk_fst_net2_link_state(net,newn,(wtk_fst_state2_t*)tran->to_state, 0, 0, 0, 0, 0);
				t->hook2=tran->hook2;
			}
			//
			s->v.trans=0;
			t=wtk_fst_net2_link_state(net,s,eps,0,0,0,0,0);
			//
			if (i>xbnf_post->min_selfloop){
				t=wtk_fst_net2_link_state(net,newn,eps,0,0,0,0,0);
			}
			s_first=newn;
		}
		for (si=s_first, i++;i<=xbnf_post->max_selfloop; i++)
		{
			newn=wtk_fst_net2_pop_state(net);
			lmlike = xbnf_post->wrdpen + (i-1) * xbnf_post->step_wrdpen;
			t=wtk_fst_net2_link_state(net,si,newn,0,in_id,out_id,lmlike* net->cfg->lmscale,0);
			//wtk_debug("add path[%d]: from_id=%d to_id=%d in_id=%d out_in_id=%d\n", j, s->id, e->id, in_id, out_id);
			t->hook2=titem->trans->hook2;
			if (i>xbnf_post->min_selfloop){
				t=wtk_fst_net2_link_state(net,newn,eps,0,0,0,0,0);
			}
			si=newn;
		}
		//
		if (s_first){
			lmlike = xbnf_post->wrdpen + 1 * xbnf_post->step_wrdpen;
			t=wtk_fst_net2_link_state(net,s,s_first,0,in_id,out_id,lmlike* net->cfg->lmscale,0);
			t->hook2=titem->trans->hook2;
		}
	}

	wtk_free(ps);
	wtk_larray_delete(a);

	return 0;
}


int wtk_fst_net2_addselfloop(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post)
{
	int ret;

	if (xbnf_post->use_selfloop_limit)
		ret=wtk_fst_net2_expselfloop(net, xbnf_post);
	else
		ret=wtk_fst_net2_selfloop(net, xbnf_post);

	return ret;
}

/**
 * add path for review
 * only for single path.
 * by dmd 2021.08.18
 */
int wtk_fst_net2_addreview(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post)
{
	wtk_fst_trans_lat_item_t **pitem,*item,*item2;
	wtk_larray_t *a;
	wtk_fst_net2_info_t info;
	int i, j, n,  skip, tskip;
	wtk_fst_state_t **ps;
	wtk_fst_state2_t *s, *e;
	int in_id, out_id;
	wtk_fst_trans2_t* t;
	float lmlike;
	wtk_dict_word_t *dw;

	n=net->state_id;
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_lat_item_t*));
	//wtk_debug("trans=%d state=%d\n",net->trans_id,net->state_id);
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	info=wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
//	qsort(pitem,a->nslot,sizeof(wtk_fst_trans_lat_item_t*),wtk_fst_net2_cmp_arc);   //some bug

	//wtk_debug("nslot=%d\n", a->nslot);
	if (info.n-4 >= xbnf_post->min_len)
	{
		skip = min((int)(xbnf_post->pct *  info.n-4), xbnf_post->max_skip);
		skip = max(skip, 1);
	}else
		skip = 0;
	//wtk_debug("skip=%d\n", skip);
	/**
	 * <s> </s> no consider
	 */
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];
		//skip <s> </s>
		if(item->trans->from_state->id==0 || item->trans->to_state->type==WTK_FST_FINAL_STATE) continue;

		s = (wtk_fst_state2_t*)item->trans->from_state;
		in_id = item->trans->in_id;
		out_id = item->trans->out_id;
		//wtk_debug("in_id=%d out_id=%d\n", in_id, out_id);
		//skip unvalid words
		if (out_id<=0)continue;

		//self-loop
		lmlike = xbnf_post->wrdpen + 1 * xbnf_post->step_wrdpen;
		t=wtk_fst_net2_link_state(net, s,s,0,in_id,out_id,lmlike * net->cfg->lmscale,0);
		t->hook2=item->trans->hook2;

		//wtk_debug("from=%d to=%d in_id=%d\n",s->id, ((wtk_fst_state2_t*)item->trans->to_state)->id, item->trans->in_id);
		for(j=i+1, tskip = skip; j < a->nslot && tskip > 0; j++, tskip--)
		{
			item2=pitem[j];
			e = (wtk_fst_state2_t*)item2->trans->to_state;
			if(e->type == WTK_FST_FINAL_STATE) break;

			lmlike = xbnf_post->wrdpen + (skip-tskip+1) * xbnf_post->step_wrdpen;
			if(item2->trans->hook2)
			{
				dw = (wtk_dict_word_t*)item2->trans->hook2;
				if(dw->maxnphn<=2){
					lmlike*= xbnf_post->mpscale;
				}
			}
			//wtk_debug("lmlike=%f\n", lmlike*net->cfg->lmscale);
			t=wtk_fst_net2_link_state(net, e,s,0,in_id,out_id,lmlike * net->cfg->lmscale,0);
			//wtk_debug("add path[%d]: from_id=%d to_id=%d in_id=%d out_in_id=%d\n", j, s->id, e->id, in_id, out_id);
			t->hook2=item->trans->hook2;
		}
	}

	wtk_free(ps);
	wtk_larray_delete(a);

	return 0;
}

/**
 * 1. build forward-path and reverse-path between any two nodes.
 * 2. build connect with give state-node(e-start,e-end) dating from other net.
 * Notes:
 * <s> node has no input-path; </s> node has no output-path.
 * weight of build path: log(1.0/nwrds), don't contain <s> / </s>
 * add by dmd
 */
int wtk_fst_net2_addwrdloop(wtk_fst_net2_t *net, wtk_fst_state2_t *es, wtk_fst_state2_t *ee)
{
	wtk_fst_trans_lat_item_t **pitem, *item, *item2;
	wtk_larray_t *a;
	wtk_fst_net2_info_t info;
	int i, j, n;
	wtk_fst_state_t **ps;
	wtk_fst_state2_t *s, *e;
	wtk_fst_trans2_t* t;
	float lmlike, weight=0.0f;

	n=net->state_id;
	a=wtk_larray_new(64,sizeof(wtk_fst_trans_lat_item_t*));
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	info=wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];
		s = (wtk_fst_state2_t*)item->trans->from_state;
		e = (wtk_fst_state2_t*)item->trans->to_state;
		lmlike=log(1.0/(info.n-2))*net->cfg->lmscale;
		if (i==a->nslot-1)
			lmlike=0;
		/*add expand wrd net*/
		if (i>0 && es && ee)
		{
			t=wtk_fst_net2_link_state(net, s , es, 0, 0, 0, 0, 0);
			t=wtk_fst_net2_link_state(net, ee, e, 0,item->trans->in_id, item->trans->out_id, lmlike, weight);
			t->hook2=item->trans->hook2;
		}

		if(item->trans->from_state->id==0 || item->trans->to_state->type==WTK_FST_FINAL_STATE) continue;
		if (item->trans->out_id<=0)continue;

		//self-loop
		if (i>0)
		{
			t=wtk_fst_net2_link_state(net, e,e,0,item->trans->in_id,item->trans->out_id, lmlike ,weight);
			t->hook2=item->trans->hook2;
		}

		for(j=i+1; j < a->nslot; j++)
		{
			item2=pitem[j];
			e = (wtk_fst_state2_t*)item2->trans->to_state;
			if(e->type == WTK_FST_FINAL_STATE) break;
			if (j==a->nslot-1)
				lmlike=0;
			//forward
			t=wtk_fst_net2_link_state(net, s,e,0,item2->trans->in_id,item2->trans->out_id, lmlike,weight);
			t->hook2=item2->trans->hook2;
			//reverse
			t=wtk_fst_net2_link_state(net, e,s,0,item->trans->in_id,item->trans->out_id,lmlike,weight);
		    t->hook2=item->trans->hook2;
		}
	}

	wtk_free(ps);
	wtk_larray_delete(a);

	return 0;
}

/**
 * support all net adjust.
 */
int wtk_fst_net2_addpath(wtk_fst_net2_t *net, qtk_xbnf_post_cfg_t *xbnf_post, int use_leak, int use_addre, int use_selfloop)
{
	wtk_fst_trans_lat_item_t **pitem,*item,*item2,*first=NULL, *end=NULL;
	wtk_larray_t *a;
	wtk_fst_net2_info_t info;
	int i, j, n, skip, tskip;
	wtk_fst_state_t **ps;
	wtk_fst_state2_t *s, *e;
	wtk_fst_trans2_t* t;
	float lmlike;
	wtk_dict_word_t *dw;
	int ret=0;

	if (!use_leak && !use_addre && !use_selfloop )
		return ret;

	n=net->state_id;
	a=wtk_larray_new(64,sizeof(wtk_fst_trans_lat_item_t*));
	//wtk_debug("trans=%d state=%d\n",net->trans_id,net->state_id);
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	info=wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
//	qsort(pitem,a->nslot,sizeof(wtk_fst_trans_lat_item_t*),wtk_fst_net2_cmp_arc);   //some bug

	//skip valid for leak or add review.
	if ((use_leak || use_addre) && info.n-4 >= xbnf_post->min_len)
	{
		skip = min((int)(xbnf_post->pct *  info.n-4), xbnf_post->max_skip);
		skip = max(skip, 1);
	}else
		skip = 0;

	if (0==skip && 0==use_selfloop)
	{
		return ret;
	}

	//wtk_debug("skip=%d\n", skip);
	/**
	 * <s> </s> no consider, it frome_state id=0, or type=WTK_FST_FINAL_STATE in fact.
	 */
	//wtk_debug("nslot=%d\n", a->nslot);  nslot considered <s> </s>
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];
		if(item->trans->from_state->id==0 || item->trans->to_state->type==WTK_FST_FINAL_STATE) continue;

		s = (wtk_fst_state2_t*)item->trans->from_state;
		e = (wtk_fst_state2_t*)item->trans->to_state;
		if(!first)
		{
			first=item;
		}
		end=item;
		item2=end;
		//skip invalid words
		if (item->trans->out_id<=0)continue;

		if (use_addre)
		{
			lmlike=xbnf_post->wrdpen * net->cfg->lmscale;
			t=wtk_fst_net2_link_state(net, e, e, 0, item->trans->in_id, item->trans->out_id, lmlike,0);
			//wtk_debug("w=%f trans=%p\n", t->lm_like, t);
			t->hook2=item->trans->hook2;
		}

		for(j=i+1, tskip = skip; j < a->nslot && tskip > 0; j++, tskip--)
		{
			item2=pitem[j];
			e = (wtk_fst_state2_t*)item2->trans->to_state;
			if(e->type == WTK_FST_FINAL_STATE) break;
			lmlike = (xbnf_post->wrdpen + (skip-tskip+1) * xbnf_post->step_wrdpen) * net->cfg->lmscale;
			//lmlike=lmlike*0.1;
			//wtk_debug("lmlike=%f lmscale=%f\n", lmlike,net->cfg->lmscale);
			if(item2->trans->hook2)
			{
				dw = (wtk_dict_word_t*)item2->trans->hook2;
				if(dw->maxnphn<=2){
					lmlike*= xbnf_post->mpscale;
					//wtk_debug("lmlike=%f\n", lmlike);
				}
			}

			if(use_leak)
			{
				t=wtk_fst_net2_link_state(net, s,e,0,item2->trans->in_id,item2->trans->out_id,lmlike,0);
				t->hook2=item2->trans->hook2;
			}

			if (use_addre)
			{
				t=wtk_fst_net2_link_state(net, e,s,0,item->trans->in_id,item->trans->out_id,lmlike,0);
			    t->hook2=item->trans->hook2;
			}
		}
	}

	//check existed with use_addre
	if (use_selfloop && first && end && (!use_addre || info.n-4>skip))
	{
		if (xbnf_post->use_selfloop_limit)
		{
			ret=wtk_fst_net2_expselfloop(net, xbnf_post);
			return ret;
		}else
		{
			s = (wtk_fst_state2_t*)first->trans->to_state;
			e = (wtk_fst_state2_t*)end->trans->to_state;
			lmlike = (xbnf_post->wrdpen + 1 * xbnf_post->step_wrdpen) * net->cfg->lmscale;
			t=wtk_fst_net2_link_state(net,e,s,0,first->trans->in_id,first->trans->out_id,lmlike,0);
			//wtk_debug("add path[%d]: from_id=%d to_id=%d in_id=%d out_in_id=%d\n", i, s->id, e->id, in_id, out_id);
			t->hook2=first->trans->hook2;
		}
	}

	wtk_free(ps);
	wtk_larray_delete(a);

	return 0;
}

/*
 * add by dmd 2020.02.24
 * wtk_fst_net2_print_lat maybe exist error for multi path in net.
 *
 * */
int wtk_fst_net2_print_lat2(wtk_fst_net2_t *net,FILE *log)
{
	wtk_fst_trans_lat_item_t **pitem,*item;
	wtk_fst_state_t **ps, *si;
	wtk_fst_net2_info_t info;
	wtk_larray_t *a;
	int i,n;
	int index;

	n=net->state_id;
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_lat_item_t*));
	//wtk_debug("trans=%d state=%d\n",net->trans_id,net->state_id);
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	info=wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	//fprintf(log,"N=%d L=%d\n",info.n,a->nslot);
	fprintf(log,"N=%d L=%d l=%d n=%d\n",info.n,a->nslot, info.l,n);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
//	qsort(pitem,a->nslot,sizeof(wtk_fst_trans_lat_item_t*),wtk_fst_net2_cmp_arc);
	for(i=1;i<info.n;++i)   //0: start 1:<s>
	{
		si=ps[i];
		fprintf(log,"I=%d id=%d\n",i, si->id);
	}

	index=0;
	for(i=0;i<a->nslot;++i)
	{
//		wtk_debug("i=%d info.l=%d\n", i, info.l);
		item=pitem[i];
		if(item->trans->to_state->type==WTK_FST_FINAL_STATE)
		{
			continue;
		}
		fprintf(log,"J=%d S=%d E=%d a=%f\n",index,item->from->id,item->trans->to_state->id,
				-item->trans->weight);
		++index;
	}
	wtk_free(ps);
	wtk_larray_delete(a);
	return 0;
}

int wtk_fst_net2_print_lat(wtk_fst_net2_t *net,FILE *log)
{
	wtk_fst_trans_lat_item_t **pitem,*item,*item2;
	wtk_fst_trans2_t *trans;
	wtk_fst_state_t **ps;
	wtk_fst_net2_info_t info;
	wtk_larray_t *a;
	int i,n;
	wtk_string_t *v;
	wtk_fst_net_cfg_t *cfg=net->cfg;
	int index;

	n=net->state_id;
	a=wtk_larray_new(1024,sizeof(wtk_fst_trans_lat_item_t*));
	//wtk_debug("trans=%d state=%d\n",net->trans_id,net->state_id);
	ps=(wtk_fst_state_t**)wtk_calloc(n,sizeof(wtk_fst_state_t*));
	info=wtk_fst_net2_nodes(net,(wtk_fst_state_t*)net->start,ps,net->state_id,a);
	fprintf(log,"N=%d L=%d\n",a->nslot,info.l);
	pitem=(wtk_fst_trans_lat_item_t**)a->slot;
	qsort(pitem,a->nslot,sizeof(wtk_fst_trans_lat_item_t*),wtk_fst_net2_cmp_arc);
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];
		item->index=i;
		if(item->trans->out_id==0)
		{
			v=NULL;
		}else
		{
			if(net->print)
			{
				v=wtk_fst_net_print_get_outsym(net->print,item->trans->out_id);
			}else
			{
				v=cfg->sym_out->strs[item->trans->out_id];
			}
		}
		//wtk_debug("%d:%d\n",item->trans->in_id,item->trans->out_id);
		//wtk_debug("v[%d]=%.*s[%d]\n",i,v->len,v->data,item->trans->hook2.frame);
		trans=item->trans;
		trans->hook2=item;
		if(v)
		{
			//fprintf(log,"I=%d t=%.2f W=%.*s[%d]\n",item->index,trans->frame*0.01,v->len,v->data,item->trans->out_id);
			//fprintf(log,"I=%d t=%.2f W=%.*s trans=%p\n",item->index,trans->frame*0.01,v->len,v->data,trans);
			fprintf(log,"I=%d t=%.2f W=%.*s\n",item->index,trans->frame*0.01,v->len,v->data);
		}else
		{
			fprintf(log,"I=%d t=%.2f W=!NULL\n",item->index,trans->frame*0.01);
		}
		/*
		if(trans->frame<0)
		{
			wtk_debug("pop trans=%d\n",trans->frame);
			exit(0);
		}*/
	}
	index=0;
	for(i=0;i<a->nslot;++i)
	{
		item=pitem[i];
		if(item->trans->to_state->type==WTK_FST_FINAL_STATE)
		{
			continue;
		}
		for(trans=(wtk_fst_trans2_t*)item->trans->to_state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
		{
			item2=trans->hook2;
			fprintf(log,"J=%d S=%d E=%d a=%f\n",index,item->index,item2->index,
					-trans->weight);
			++index;
		}
	}
	wtk_free(ps);
	wtk_larray_delete(a);
	return 0;
}


int wtk_fst_net2_write_lat(wtk_fst_net2_t *net,char *fn)
{
	FILE *f;

	wtk_debug("write lat %s\n",fn);
	f=fopen(fn,"w");
	wtk_fst_net2_print_lat(net,f);
	fclose(f);
	return 0;
}
/*
void wtk_fst_net2_print_lat(wtk_fst_net2_t *net,FILE *log)
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
		item->index=index;
		++index;
		arc=item->arc;
#ifndef USE_X
		fprintf(log,"I=%d t=%.2f W=%.*s v=1\n",item->index,item->arc->frame*0.01,v->len,v->data);
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
			fprintf(log,"J=%d S=%d E=%d a=%f l=%f\n",index,item->index,item2->index,
					arc->ac_like,arc->lm_like/net->net_cfg->lmscale);
			++index;
		}
	}
	wtk_larray_delete(a);
}
*/

//-------------- result rec string  ---------------------
wtk_fst_rec_item_t* wtk_fst_rec_item_new(wtk_heap_t *heap,char *data,int len)
{
	wtk_fst_rec_item_t *item;

	item=(wtk_fst_rec_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_rec_item_t));
	if(len>0)
	{
		item->str=wtk_heap_dup_string(heap,data,len);
	}else
	{
		item->str=NULL;
	}
	item->conf=-1;
	item->score=-FLT_MAX;
	return item;
}

wtk_string_t* wtk_fst_rec_item_get_str(wtk_fst_rec_item_t *item)
{
	return item?item->str:NULL;
}

//--------------------shortest path ----------------
void wtk_fst_net2_print_shortest_path(wtk_fst_net2_t *net)
{
	wtk_fst_net2_shortest_path(net);
	wtk_fst_net2_print_best_path(net);
}


//--------------------------- determinize ---------------------------
typedef struct wtk_fst_net_dm_state_item wtk_fst_net_dm_state_item_t;
typedef struct wtk_fst_net_dm_trans_item wtk_fst_net_dm_trans_item_t;

struct wtk_fst_net_dm_state_item
{
	wtk_queue_node_t q_n;
	//wtk_fst_net_dm_state_item_t *next;
	wtk_fst_state2_t *state;
	float w;
};

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_node_t eof_n;
	wtk_queue_node_t state_n;
	//wtk_fst_net_dm_state_item_t *state_list;
	wtk_queue_t state_q;
	wtk_fst_state2_t *output_state;
	float weight;
	unsigned eof:1;
}wtk_fst_net_dm_state_t;

struct wtk_fst_net_dm_trans_item
{
	wtk_fst_net_dm_trans_item_t *next;
	wtk_fst_trans2_t *trans2;
	wtk_fst_net_dm_state_item_t *state_item;
};

typedef struct
{
	wtk_fst_net_dm_trans_item_t *trans_list;
}wtk_fst_net_dm_trans_t;

wtk_fst_net_dm_trans_t* wtk_fst_net_dm_trans_new(wtk_heap_t *heap)
{
	wtk_fst_net_dm_trans_t *trans;

	trans=(wtk_fst_net_dm_trans_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_net_dm_trans_t));
	trans->trans_list=NULL;
	return trans;
}

wtk_fst_net_dm_trans_t* wtk_fst_net_dm_add_trans(wtk_larray_t *a,wtk_heap_t *heap,
		wtk_fst_net_dm_state_item_t *state_item,
		wtk_fst_trans2_t *trans)
{
	wtk_fst_net_dm_trans_t **ptrans,*dmt,*dmt1;
	wtk_fst_net_dm_trans_item_t *item;
	int i;

	dmt=NULL;
	if(a->nslot>0)
	{
		ptrans=(wtk_fst_net_dm_trans_t**)a->slot;
		for(i=0;i<a->nslot;++i)
		{
			dmt1=ptrans[i];
			if(dmt1->trans_list->trans2->out_id==trans->out_id)
			{
				dmt=dmt1;
				break;
			}
		}
	}
	if(!dmt)
	{
		dmt=wtk_fst_net_dm_trans_new(heap);
		wtk_larray_push2(a,&(dmt));
	}
	item=(wtk_fst_net_dm_trans_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_net_dm_trans_item_t));
	item->state_item=state_item;
	item->trans2=trans;
	item->next=dmt->trans_list;
	dmt->trans_list=item;
	return dmt;
}


void wtk_fst_net_dm_state_update_trans(wtk_fst_net_dm_state_t *s,wtk_larray_t *a,wtk_heap_t *heap)
{
	wtk_fst_net_dm_state_item_t *item;
	wtk_queue_node_t *qn;
	wtk_fst_trans2_t *trans2;

	wtk_larray_reset(a);
	for(qn=s->state_q.pop;qn;qn=qn->next)
	{
		item=data_offset(qn,wtk_fst_net_dm_state_item_t,q_n);
		for(trans2=(wtk_fst_trans2_t*)(item->state->v.trans);trans2;
				trans2=(wtk_fst_trans2_t*)(trans2->hook.next))
		{
			wtk_fst_net_dm_add_trans(a,heap,item,trans2);
			//wtk_debug("%d/%d\n",trans2->in_id,trans2->out_id);
		}
	}
}

wtk_fst_net_dm_state_t* wtk_fst_net_dm_state_new(wtk_heap_t *heap,wtk_fst_state2_t *state,float w)
{
	wtk_fst_net_dm_state_t *dm;
	wtk_fst_net_dm_state_item_t *s;

	dm=(wtk_fst_net_dm_state_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_net_dm_state_t));
	wtk_queue_init(&(dm->state_q));
	dm->eof=0;
	dm->weight=0;
	if(state)
	{
		s=(wtk_fst_net_dm_state_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_net_dm_state_item_t));
		s->state=state;
		s->w=w;
		wtk_queue_push(&(dm->state_q),&(s->q_n));
		if(state->type==WTK_FST_FINAL_STATE)
		{
			dm->eof=1;
		}
	}
	return dm;
}

int wtk_fst_net_dm_state_has_item(wtk_fst_net_dm_state_t *s,wtk_fst_net_dm_state_item_t *item)
{
	wtk_queue_node_t *qn;
	wtk_fst_net_dm_state_item_t *si;

	for(qn=s->state_q.pop;qn;qn=qn->next)
	{
		si=data_offset(qn,wtk_fst_net_dm_state_item_t,q_n);
		if(si->state==item->state && si->w==item->w)
		{
			return 1;
		}
	}
	return 0;
}

int wtk_fst_net_dm_state_cmp(wtk_fst_net_dm_state_t *s1,wtk_fst_net_dm_state_t *s2)
{
	wtk_queue_node_t *qn;
	wtk_fst_net_dm_state_item_t *si;

	if(s1->weight!=s2->weight)
	{
		return -1;
	}
	if(s1->eof!=s2->eof)
	{
		return -1;
	}
	if(s1->state_q.length!=s2->state_q.length)
	{
		return -1;
	}
	for(qn=s1->state_q.pop;qn;qn=qn->next)
	{
		si=data_offset(qn,wtk_fst_net_dm_state_item_t,q_n);
		if(wtk_fst_net_dm_state_has_item(s2,si)==0)
		{
			return -1;
		}
	}
	return 0;
}

wtk_fst_net_dm_state_t* wtk_queue_get_same_dms(wtk_queue_t *q,wtk_fst_net_dm_state_t *s)
{
	wtk_queue_node_t *qn;
	wtk_fst_net_dm_state_t *t;

	//for(qn=q->pop;qn;qn=qn->next)
	for(qn=q->push;qn;qn=qn->prev)
	{
		t=data_offset(qn,wtk_fst_net_dm_state_t,state_n);
		if(wtk_fst_net_dm_state_cmp(t,s)==0)
		{
			//wtk_debug("has dms\n");
			return t;
		}
	}
	return NULL;
}

wtk_fst_net_dm_state_item_t* wtk_fst_net_dm_state_add_state(wtk_fst_net_dm_state_t *dms,
		wtk_heap_t *heap,
		wtk_fst_net_dm_trans_item_t *trans_item)
{
	//wtk_queue_node_t *qn;
	wtk_fst_net_dm_state_item_t *si;
	wtk_fst_state_t *state=trans_item->trans2->to_state;
	double f;

	/*
	for(qn=dms->state_q.pop;qn;qn=qn->next)
	{
		si=data_offset(qn,wtk_fst_net_dm_state_item_t,q_n);
		if(si->state==state)
		{
			return si;
		}
	}*/
	si=(wtk_fst_net_dm_state_item_t*)wtk_heap_malloc(heap,sizeof(wtk_fst_net_dm_state_item_t));
	si->state=(wtk_fst_state2_t*)state;
	si->w=0;
	wtk_queue_push(&(dms->state_q),&(si->q_n));
	if(state->type==WTK_FST_FINAL_STATE)
	{
		f=wtk_fst_times(trans_item->state_item->w,state->v.weight);
		dms->weight=wtk_fst_plus(f,dms->weight);
		//wtk_debug("w=%f/%f,%f,%f\n",dms->weight,f,trans_item->state_item->w,state->weight);
		dms->eof=1;
	}
	return si;
}

int wtk_fst_net2_determinize(wtk_fst_net2_t *net,wtk_fst_net2_t *output)
{
	wtk_heap_t *heap=output->heap;
	wtk_fst_net_dm_state_t *dms,*dms1,*dms2;
	wtk_queue_node_t *qn;
	wtk_queue_t q;
	wtk_larray_t *a;
	int i;
	wtk_fst_net_dm_trans_item_t *trans_item;
	wtk_fst_net_dm_state_item_t *state_item;
	wtk_fst_net_dm_trans_t **ptrans,*dm_trans;
	wtk_fst_trans2_t *trans2;
	double f,w1;
	wtk_queue_t eof_q;
	wtk_queue_t state_q;
//#define DEBUG_DM
	int ki=0;

	wtk_queue_init(&(state_q));
	wtk_queue_init(&(eof_q));
	a=wtk_larray_new(1024,sizeof(void*));
	wtk_queue_init(&(q));
	dms=wtk_fst_net_dm_state_new(heap,net->start,0);
	dms->output_state=wtk_fst_net2_pop_state(output);
	wtk_queue_push(&(state_q),&(dms->state_n));
	output->start=dms->output_state;
	wtk_queue_push(&(q),&(dms->q_n));
	while(q.length>0)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		dms=data_offset(qn,wtk_fst_net_dm_state_t,q_n);
		wtk_fst_net_dm_state_update_trans(dms,a,heap);
		++ki;
		//wtk_debug("nslot=%d\n",a->nslot);
		ptrans=(wtk_fst_net_dm_trans_t**)a->slot;
		for(i=0;i<a->nslot;++i)
		{
			dm_trans=ptrans[i];
			w1=0;
			dms1=wtk_fst_net_dm_state_new(heap,NULL,0);
			//dms1->output_state=wtk_fst_net2_pop_state(output);
			//wtk_debug("trans=%p\n",dm_trans->trans_list);
			for(trans_item=dm_trans->trans_list;trans_item;trans_item=trans_item->next)
			{
				trans2=trans_item->trans2;
				f=wtk_fst_times(trans_item->state_item->w,trans2->weight);
				w1=wtk_fst_plus(w1,f);
#ifdef DEBUG_DM
				wtk_debug("v[%d/%d][%.*s]/%f w1=%f s=%f s=%p\n",ki,trans2->to_state->id,
						net->cfg->sym_out->strs[trans2->out_id]->len,
						net->cfg->sym_out->strs[trans2->out_id]->data,
						trans2->weight,w1,trans_item->state_item->w,trans_item->state_item);
#endif

			}
			//wtk_debug("trans=%p\n",dm_trans->trans_list);
#ifdef DEBUG_DM
			wtk_debug("w1=%f\n",w1);
#endif
			for(trans_item=dm_trans->trans_list;trans_item;trans_item=trans_item->next)
			{
				state_item=wtk_fst_net_dm_state_add_state(dms1,heap,trans_item);
				trans2=trans_item->trans2;
				f=wtk_fst_times(trans_item->state_item->w,trans_item->trans2->weight);
				//f=wtk_fst_times(f,1/w1);
				f-=w1;
				if(0 && state_item->w!=0)
				{
					wtk_debug("v[%d/%d][%.*s]/%f f=%f/%f s=%p\n",ki,trans2->to_state->id,
							net->cfg->sym_out->strs[trans2->out_id]->len,
							net->cfg->sym_out->strs[trans2->out_id]->data,
							trans2->weight,f,state_item->w,state_item);
					exit(0);
				}
				state_item->w=f;//wtk_fst_plus(f,state_item->w);
#ifdef DEBUG_DM
				wtk_debug("v[%d/%d][%.*s]/%f f=%f/%f s=%p\n",ki,trans2->to_state->id,
						net->cfg->sym_out->strs[trans2->out_id]->len,
						net->cfg->sym_out->strs[trans2->out_id]->data,
						trans2->weight,f,state_item->w,state_item);
#endif
			}
			dms2=wtk_queue_get_same_dms(&(state_q),dms1);
#ifdef DEBUG_DM
			wtk_debug("dms2=%p\n",dms2);
#endif
			if(dms2)
			{
				dms1->output_state=dms2->output_state;
			}else
			{
				wtk_queue_push(&(state_q),&(dms1->state_n));
				dms1->output_state=wtk_fst_net2_pop_state(output);
				if(dms1->eof)
				{
					wtk_queue_push(&(eof_q),&(dms1->eof_n));
				}
				wtk_queue_push(&(q),&(dms1->q_n));
			}
			trans2=wtk_fst_net2_pop_trans(output);
			trans2->from_state=(wtk_fst_state_t*)dms->output_state;
			trans2->to_state=(wtk_fst_state_t*)dms1->output_state;
			trans2->weight=w1;
			trans2->in_id=dm_trans->trans_list->trans2->in_id;
			trans2->out_id=trans2->in_id;
			trans2->frame=dm_trans->trans_list->trans2->frame;
			trans2->hook.next=dms->output_state->v.trans;
			dms->output_state->v.trans=(wtk_fst_trans_t*)trans2;
#ifdef DEBUG_DM
			wtk_debug("v[%d][%.*s]/%f\n\n",ki,
					net->cfg->sym_out->strs[trans2->out_id]->len,
					net->cfg->sym_out->strs[trans2->out_id]->data,
					trans2->weight);
#endif
			if(ki==25)
			{
				//exit(0);
			}
		}
		//exit(0);
	}
	//wtk_debug("eof=%d\n",eof_q.length);
	output->end=wtk_fst_net2_pop_state(output);
	output->end->type=WTK_FST_FINAL_STATE;
	output->end->v.weight=0;
	for(qn=eof_q.pop;qn;qn=qn->next)
	{
		dms=data_offset(qn,wtk_fst_net_dm_state_t,eof_n);
		trans2=wtk_fst_net2_pop_trans(output);
		trans2->from_state=(wtk_fst_state_t*)dms->output_state;
		trans2->to_state=(wtk_fst_state_t*)output->end;
		trans2->weight=0;//dms->weight;
		trans2->in_id=0;
		trans2->out_id=0;
		//wtk_debug("trans=%p\n",dms->output_state->trans);
		trans2->hook.next=dms->output_state->v.trans;
		dms->output_state->v.trans=(wtk_fst_trans_t*)trans2;
	}
	//wtk_fst_net2_print(output);
	//exit(0);
	//wtk_fst_net2_print_lat(output,stdout);
	//wtk_fst_net2_print_fsm2(output);
	//wtk_fst_net2_write_lat(output,"test.lat");
	wtk_larray_delete(a);
	return 0;
}

void wtk_fst_state_remove_trans(wtk_fst_state_t *s,wtk_fst_trans_t *trans)
{
	wtk_fst_trans_t *t,*prev;

	prev=NULL;
	for(t=s->v.trans;t;t=(wtk_fst_trans_t*)t->hook.next)
	{
		if(t==trans)
		{
			if(prev)
			{
				prev->hook.next=t->hook.next;
			}else
			{
				s->v.trans=t->hook.next;
			}
			break;
		}
		prev=t;
	}
}

void wtk_fst_net2_remove_nil_state(wtk_fst_net2_t *net,wtk_fst_state2_t *s,wtk_slist_t *l)
{
	wtk_fst_trans2_t *trans,*t2;

	for(trans=s->in_prev;trans;trans=t2)
	{
		t2=trans->in_prev;
		wtk_fst_state_remove_trans((wtk_fst_state_t*)trans->from_state,(wtk_fst_trans_t*)trans);
		if(!trans->from_state->v.trans && !trans->from_state->hook)
		{
			//trans->from_state->hook=NULL;
			wtk_slist_remove(l,&(trans->from_state->q_n));
			wtk_fst_net2_remove_nil_state(net,(wtk_fst_state2_t*)trans->from_state,l);
		}
	}
}

void wtk_fst_net2_clean_hook_list(wtk_slist_t *l)
{
	wtk_slist_node_t *n;
	wtk_fst_state2_t *s;

	for(n=l->prev;n;n=n->prev)
	{
		s=data_offset(n,wtk_fst_state2_t,q_n);
		s->hook=NULL;
	}
}


void wtk_fst_net2_set_list_hook(wtk_slist_t *l)
{
	wtk_slist_node_t *n;
	wtk_fst_state2_t *s;

	for(n=l->prev;n;n=n->prev)
	{
		s=data_offset(n,wtk_fst_state2_t,q_n);
		s->hook=s;
	}
}


void wtk_fst_net2_remove_nil_node2(wtk_fst_net2_t *net,wtk_slist_t *l)
{
	wtk_slist_t tmp;
	wtk_slist_node_t *n;
	wtk_fst_state2_t *s;

	wtk_fst_net2_set_list_hook(l);
	wtk_slist_init(&(tmp));
	while(1)
	{
		n=wtk_slist_pop(l);
		if(!n){break;}
		s=data_offset(n,wtk_fst_state2_t,q_n);
		if(s->type==WTK_FST_FINAL_STATE)
		{
			wtk_slist_push(&(tmp),&(s->q_n));
			continue;
		}
		wtk_debug("type=%d id=%d trans=%p in_prev=%p\n",s->type,s->id,s->v.trans,s->in_prev);
		if(!s->v.trans)
		{
			wtk_fst_net2_remove_nil_state(net,s,&(tmp));
		}else
		{
			s->hook=NULL;
			wtk_slist_push(&(tmp),&(s->q_n));
		}
	}
	*l=tmp;
	wtk_fst_net2_clean_hook_list(l);
}

void wtk_fst_net2_remove_nil_state2(wtk_fst_state2_t *s)
{
	wtk_fst_trans2_t *trans;
	wtk_fst_state2_t *s2;
	wtk_fst_trans2_t *t2,*prev;

	s->hook=s;
	if(s->type==WTK_FST_FINAL_STATE)
	{
		return;
	}
	if(s->v.trans)
	{
		prev=NULL;
		for(trans=(wtk_fst_trans2_t*)s->v.trans;trans;trans=t2)
		{
			t2=(wtk_fst_trans2_t*)trans->hook.next;
			s2=(wtk_fst_state2_t*)trans->to_state;
			if(s2->type!=WTK_FST_FINAL_STATE)
			{
				if(s2->v.trans && !s2->hook)
				{
					wtk_fst_net2_remove_nil_state2(s2);
				}
				if(!s2->v.trans)
				{
					if(prev)
					{
						prev->hook.next=(wtk_fst_trans_t*)t2;
					}else
					{
						s->v.trans=(wtk_fst_trans_t*)t2;
					}
				}else
				{
					prev=trans;
				}
			}else
			{
				prev=trans;
			}
		}
	}
}

int wtk_fst_net2_nstate(wtk_fst_net2_t *net,wtk_fst_state_t *s)
{
	wtk_fst_trans_t *trans;
	int cnt=1;

	s->hook=s;
	if(s->type==WTK_FST_FINAL_STATE)
	{
		return cnt;
	}
	for(trans=s->v.trans;trans;trans=trans->hook.next)
	{
		if(!trans->to_state->hook)
		{
			cnt+=wtk_fst_net2_nstate(net,trans->to_state);
			//trans->to_state->hook=trans;
		}
	}
	return cnt;
}

int wtk_fst_net2_states(wtk_fst_net2_t *net)
{
	int n;

	wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
	n=wtk_fst_net2_nstate(net,(wtk_fst_state_t*)net->start);
	wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
	return n;
}

void wtk_fst_net2_remove_nil_node(wtk_fst_net2_t *net)
{
	wtk_fst_state2_t *state;

	//wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
	state=net->start;
	wtk_fst_net2_remove_nil_state2(state);
	wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
}

void wtk_fst_net2_clean_state_hook(wtk_fst_state2_t *state)
{
	wtk_fst_trans2_t *trans;

	if(state->hook)
	{
		state->hook=NULL;
	}
	for(trans=(wtk_fst_trans2_t*)state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next)
	{
		if(trans->hook2)
		{
			trans->hook2=NULL;
			wtk_fst_net2_clean_state_hook((wtk_fst_state2_t*)trans->to_state);
		}
	}
}

void wtk_fst_net2_clean_hook(wtk_fst_net2_t *net)
{
	wtk_fst_net2_clean_state_hook(net->start);
}

void wtk_fst_net2_clean_hook2(wtk_fst_net2_t *net)
{
	//wtk_debug("start=%p\n",net->start->hook);
	if(net->start)
	{
		wtk_fst_state_clean_hook((wtk_fst_state_t*)net->start);
	}
}
