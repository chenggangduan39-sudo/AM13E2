#include "wtk_lmgen2.h"
#include <math.h>

wtk_lmgen2_t* wtk_lmgen2_new(wtk_lmgen2_cfg_t *cfg,wtk_rbin2_t *rbin)
{
	wtk_lmgen2_t *g;

	g=(wtk_lmgen2_t*)wtk_malloc(sizeof(wtk_lmgen2_t));
	g->cfg=cfg;
	g->rbin=rbin;
	g->lm=wtk_nglm_new(&(cfg->lm),&(cfg->dict));
	g->prune=wtk_prune_new(&(cfg->prune));
	g->predict_prune=wtk_prune_new(&(cfg->predict_prune));
	//g->pth_heap=wtk_bit_heap_new(sizeof(wtk_lmgen2_path_t),4096/sizeof(wtk_lmgen2_path_t),1024*8,1);
	g->pth_heap=wtk_vpool_new(sizeof(wtk_lmgen2_path_t),1024);
	//g->tok_heap=wtk_bit_heap_new(sizeof(wtk_lmgen2_tok_t),4096/sizeof(wtk_lmgen2_tok_t),1024*8,1);
	g->tok_heap=wtk_vpool_new(sizeof(wtk_lmgen2_tok_t),1024);
	g->buf=wtk_strbuf_new(256,1);
	g->filter=wtk_larray_new(10,sizeof(int));
	wtk_lmgen2_reset(g);
	wtk_queue_init(&(g->tok_q));
	return g;
}

void wtk_lmgen2_delete(wtk_lmgen2_t *g)
{
	wtk_prune_delete(g->predict_prune);
	wtk_larray_delete(g->filter);
	wtk_strbuf_delete(g->buf);
	wtk_vpool_delete(g->pth_heap);
	wtk_vpool_delete(g->tok_heap);
	wtk_prune_delete(g->prune);
	wtk_nglm_delete(g->lm);
	wtk_free(g);
}

void wtk_lmgen2_reset(wtk_lmgen2_t *g)
{
	g->index=0;
	wtk_larray_reset(g->filter);
	wtk_strbuf_reset(g->buf);
	g->is_last=0;
	g->max=1e10;
	g->last_max=0;
	wtk_nglm_reset(g->lm);
	wtk_vpool_reset(g->pth_heap);
	wtk_vpool_reset(g->tok_heap);
	wtk_queue_init(&(g->tok_q));
}

wtk_lmgen2_path_t* wtk_lmgen2_pop_path(wtk_lmgen2_t *g,wtk_lm_node_t *node,float prob)
{
	wtk_lmgen2_path_t *pth;

	pth=(wtk_lmgen2_path_t*)wtk_vpool_pop(g->pth_heap);
	pth->node=node;
	pth->prob=prob;
	pth->prev=NULL;
	pth->ppl=0;
	pth->depth=0;
	//wtk_debug("prob=%f\n",prob);
	//exit(0);
	return pth;
}

void wtk_lmgen2_push_path(wtk_lmgen2_t *g,wtk_lmgen2_path_t *pth)
{
	wtk_vpool_push(g->pth_heap,pth);
}

wtk_lmgen2_tok_t* wtk_lmgen2_pop_tok(wtk_lmgen2_t *g)
{
	wtk_lmgen2_tok_t *tok;

	tok=(wtk_lmgen2_tok_t*)wtk_vpool_pop(g->tok_heap);
	tok->pth=NULL;
	return tok;
}

void wtk_lmgen2_push_tok(wtk_lmgen2_t *g,wtk_lmgen2_tok_t *tok)
{
	wtk_vpool_push(g->tok_heap,tok);
}

void wtk_lmgen2_start(wtk_lmgen2_t *g,float max_ppl)
{
	wtk_lmgen2_tok_t *tok;

	tok=wtk_lmgen2_pop_tok(g);
	tok->pth=wtk_lmgen2_pop_path(g,g->lm->s_node,g->lm->s_node->prob);
	tok->pth->depth=1;
	g->max=0;
	g->max_ppl=max_ppl;
	wtk_queue_push(&(g->tok_q),&(tok->q_n));
}

void wtk_lmgen2_print_path(wtk_lmgen2_t *g,wtk_lmgen2_path_t *pth)
{
	wtk_string_t *v;

	if(pth->prev)
	{
		wtk_lmgen2_print_path(g,pth->prev);
	}
	v=g->cfg->dict.sym->ids[pth->node->id]->str;
	wtk_debug("[%.*s]=%f/%f n=%d\n",v->len,v->data,pth->prob,pth->ppl,pth->node->ngram);
}

void wtk_lmgen2_path_update_ppl(wtk_lmgen2_path_t *pth)
{
	if(pth->ppl==0)
	{
		pth->ppl=pow(pow(10.0,pth->prob),-1.0/(pth->depth));
	}
}

int wtk_lmgen2_tok_node_cmp(wtk_queue_node_t *qn1,wtk_queue_node_t *qn2)
{
	wtk_lmgen2_tok_t *tok1,*tok2;

	tok1=data_offset2(qn1,wtk_lmgen2_tok_t,q_n);
	if(tok1->pth->ppl==0)
	{
		wtk_lmgen2_path_update_ppl(tok1->pth);
	}
	tok2=data_offset2(qn2,wtk_lmgen2_tok_t,q_n);
	if(tok2->pth->ppl==0)
	{
		wtk_lmgen2_path_update_ppl(tok2->pth);
	}
	return tok1->pth->ppl>tok2->pth->ppl?-1:1;
}

int wtk_lmgen2_is_filter(wtk_lmgen2_t *g,int id)
{
	int *v;
	int i;

	//wtk_debug("id=%d\n",id);
	v=(int*)g->filter->slot;
	for(i=0;i<g->filter->nslot;++i)
	{
		if(v[i]==id)
		{
			return 1;
		}
	}
	return 0;
}

wtk_lmgen2_tok_t* wtk_lmgen2_add_tok(wtk_lmgen2_t *g,wtk_lmgen2_path_t *prev,wtk_lm_node_t *node,float prob_pen)
{
	wtk_lmgen2_tok_t *tok;
	float prob;
	float ppl;

	if(wtk_lmgen2_is_filter(g,node->id))
	{
		return NULL;
	}
	prob=prev->prob+node->prob+prob_pen;
	ppl=pow(pow(10.0,prob),-1.0/(prev->depth+1));
	//wtk_debug("ppl=%f\n",ppl);
	if(g->index>1)
	{
		if(ppl-g->last_max>g->cfg->step_ppl_thresh)
		{
			return NULL;
		}
		if(g->is_last && ppl>g->max_ppl)
		{
			return NULL;
		}
	}else
	{
		if(ppl>g->cfg->first_ppl_thresh)
		{
			return NULL;
		}
	}
	//ppl=prob/(prev->depth+1);
	tok=wtk_lmgen2_pop_tok(g);
	tok->pth=wtk_lmgen2_pop_path(g,node,prob);
	tok->pth->prev=prev;
	tok->pth->depth=prev->depth+1;
	tok->pth->ppl=ppl;
	if(ppl<g->max)
	{
		g->max=ppl;
		g->max_pth=tok->pth;
	}
//	{
//		wtk_string_t *v;
//
//		v=g->cfg->dict.sym->ids[node->id]->str;
//		wtk_debug("prob=%f [%.*s]\n",tok->pth->prob,v->len,v->data);
//	}
	//wtk_lmgen2_print_path(g,tok->pth);
	if(g->is_last)
	{
		wtk_queue_insert(&(g->tok_q),&(tok->q_n),(wtk_cmp_handler_t)wtk_lmgen2_tok_node_cmp);
	}else
	{
		wtk_queue_push(&(g->tok_q),&(tok->q_n));
	}
	return tok;
}


int wtk_lmgen2_feed_tok(wtk_lmgen2_t *g,wtk_lmgen2_path_t *pth,unsigned int id,int depth)
{
	wtk_nglm_t *lm=g->lm;
	wtk_lm_node_t *node;
	int i,n;
	wtk_lmgen2_path_t *pth2;
	int b=0,x;
	float f,ppl;
	wtk_lmgen2_tok_t *tok;

	//wtk_debug("id=%d\n",id);
	if(depth>g->cfg->depth)
	{
		return 0;
	}
	f=pth->ppl-g->last_max;
	if(f>g->cfg->step_ppl_thresh2)
	{
		return 0;
	}
	//wtk_debug("id=%d\n",id);
	node=wtk_nglm_get_child(lm,pth->node,id);
	//wtk_debug("node=%p\n",node);
	if(node)
	{
//		{
//			wtk_string_t *v;
//
//			v=g->cfg->dict.sym->ids[pth->node->id]->str;
//			wtk_debug("depth=%d [%.*s]\n",depth,v->len,v->data);
//		}
		//f=pth->prob+node->prob;
		tok=wtk_lmgen2_add_tok(g,pth,node,0);
		if(tok)
		{
			b=1;
			//wtk_lmgen2_print_path(g,tok->pth);
			wtk_prune_add(g->prune,g->last_max-tok->pth->ppl);
			//return 1;
		}else
		{
			//wtk_debug("searc failed\n");
			return 0;
		}
	}
	n=pth->node->nchild;
	for(i=0;i<n;++i)
	{
		node=pth->node->childs[i];
		if(node->id==id)
		{
			continue;
		}
		if(wtk_lmgen2_is_filter(g,node->id))
		{
			//wtk_debug("continue\n");
			continue;
		}
		f=pth->prob+node->prob+g->cfg->skip_pen;
		ppl=pow(pow(10.0,f),-1.0/(pth->depth+1));
		if(pth->ppl-g->last_max>g->cfg->step_ppl_thresh2)
		{
			continue;
		}
		//wtk_debug("v[%d]=%f\n",i,node->prob);
		//wtk_debug("id=%d\n",node->id);
		pth2=wtk_lmgen2_pop_path(g,node,f);
		pth2->prev=pth;
		pth2->depth=pth->depth+1;
		//pth2->ppl=pth2->prob/pth2->depth;//
		pth2->ppl=ppl;//pow(pow(10.0,pth2->prob),-1.0/pth2->depth);
		x=wtk_lmgen2_feed_tok(g,pth2,id,depth+1);
		if(x==0)
		{
			wtk_lmgen2_push_path(g,pth2);
		}else if(b==0)
		{
			b=1;
		}
	}
	//exit(0);
	if(b==0)
	{
		double bow;

		node=wtk_nglm_get_child_prob(lm,pth->node,id,&bow);
//		if(depth==0)
//		{
//			wtk_string_t *v;
//
//			v=g->cfg->dict.sym->ids[pth->node->id]->str;
//			wtk_debug("return node=%p [%.*s] id=%d\n",node,v->len,v->data,id);
//		}
		if(node)
		{
			tok=wtk_lmgen2_add_tok(g,pth,node,bow+g->cfg->bow_pen);
			if(tok)
			{
				b=1;
				//wtk_lmgen2_print_path(g,tok->pth);
				//wtk_debug("ppl=%f last=%f %f/%f/%f\n",tok->pth->ppl,g->last_max,tok->pth->prob,pth->prob,bow);
				wtk_prune_add(g->prune,-tok->pth->ppl+g->last_max);
			}
		}
	}
	return b;
}

void wtk_lmgen2_prune(wtk_lmgen2_t *g)
{
	wtk_prune_t *p=g->prune;
	int b;
	float thresh,f;
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	wtk_lmgen2_tok_t *tok;

	b=wtk_prune_want_prune(p);
	if(b==0){return;}
	thresh=wtk_prune_get_thresh(p);
	//wtk_debug("[%d]=%f\n",g->tok_q.length,thresh);
	q=g->tok_q;
	wtk_queue_init(&(g->tok_q));
	while(1)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
		f=-tok->pth->ppl+g->last_max;
		if(f>=thresh)
		{
			wtk_queue_push(&(g->tok_q),qn);
		}else
		{
			wtk_lmgen2_push_tok(g,tok);
		}
	}
}


void wtk_lmgen2_feed2(wtk_lmgen2_t *g,unsigned int id)
{
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	wtk_lmgen2_tok_t *tok;
	wtk_prune_t *p=g->prune;
	int b;

	//wtk_debug("%d/%d [%.*s]\n",id,g->tok_q.length,g->cfg->dict.sym->ids[id]->str->len,g->cfg->dict.sym->ids[id]->str->data);
	g->max_pth=NULL;
	if(g->tok_q.length<=0){return;}
	++g->index;
	//wtk_debug("[%.*s]=%d tok=%d last=%f/%f\n",v_len,v,id,g->tok_q.length,g->last_max,g->max);
	wtk_prune_reset(p);
	g->last_max=g->max;
	g->max=1e10;
	q=g->tok_q;
	wtk_queue_init(&(g->tok_q));
	while(1)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
		b=wtk_lmgen2_feed_tok(g,tok->pth,id,0);
//		{
//			static int ki=0;
//			++ki;
//			wtk_debug("v[%d]=%d\n",ki,b);
//		}
		if(b==0)
		{
//			wtk_queue_push(&(g->tok_q),&(tok->q_n));
//			wtk_prune_add(g->prune,-tok->pth->ppl+g->last_max);
		}else
		{
			wtk_lmgen2_push_tok(g,tok);
		}
	}
	//wtk_debug("tok=%d\n",g->tok_q.length);
	wtk_lmgen2_prune(g);
	//wtk_debug("tok=%d\n",g->tok_q.length);
}

void wtk_lmgen2_feed_last(wtk_lmgen2_t *g)
{
	g->is_last=1;
	wtk_lmgen2_feed2(g,g->cfg->dict.snt_end_id);
}

void wtk_lmgen2_add_filter(wtk_lmgen2_t *g,char *v,int v_len)
{
	int id;

	id=wtk_fst_insym_get_index2(g->cfg->dict.sym,v,v_len);
	//wtk_debug("[%.*s]=%d\n",v_len,v,id);
	if(id>=0)
	{
		//wtk_debug("id=%d\n",id);
		wtk_larray_push2(g->filter,&(id));
	}
}

void wtk_lmgen2_add_filter2(wtk_lmgen2_t *g,char *v,int v_len)
{
	wtk_strkv_parser_t parser;
	int ret;

	wtk_strkv_parser_init(&(parser),v,v_len);
	while(1)
	{
		ret=wtk_strkv_parser_next(&(parser));
		if(ret!=0){break;}
		wtk_lmgen2_add_filter(g,parser.k.data,parser.k.len);
	}
}

void wtk_lmgen2_feed(wtk_lmgen2_t *g,char *v,int v_len)
{
	int id;
	char *s,*e,*p;
	int b;

	id=wtk_fst_insym_get_index2(g->cfg->dict.sym,v,v_len);
	if(id<0)
	{
		s=v;
		e=s+v_len;
		while(s<e)
		{
			p=e;
			b=0;
			while(p>s)
			{
				//wtk_debug("[%.*s] %p/%p/%p\n",(int)(p-s),s,p,s,e);
				id=wtk_fst_insym_get_index2(g->cfg->dict.sym,s,p-s);
				if(id>=0)
				{
					//wtk_debug("[%.*s]=[%.*s] [%.*s]\n",(int)(p-s),s,v_len,v,(int)(e-p),p);
					wtk_lmgen2_feed2(g,id);
					s=p;
					b=1;
					break;
				}
				--p;
			}
			if(b==0)
			{
				++s;
			}
		}
	}else
	{
		wtk_lmgen2_feed2(g,id);
	}
}


void wtk_lmgen2_print_path2(wtk_lmgen2_t *g,wtk_lmgen2_path_t *pth,wtk_strbuf_t *buf)
{
	wtk_string_t *v;

	if(pth->prev)
	{
		wtk_lmgen2_print_path2(g,pth->prev,buf);
	}
	if(pth->node->id!=g->cfg->dict.snt_start_id && pth->node->id!=g->cfg->dict.snt_end_id)
	{
		v=g->cfg->dict.sym->ids[pth->node->id]->str;
		if(buf->pos>0)
		{
			wtk_strbuf_push_s(buf," ");
		}
		wtk_strbuf_push(buf,v->data,v->len);
	}
	//wtk_debug("[%.*s]=%f/%f n=%d\n",v->len,v->data,pth->prob,pth->ppl,pth->node->ngram);
}

wtk_string_t wtk_lmgen2_get_result(wtk_lmgen2_t *g)
{
	wtk_queue_node_t *qn;
	wtk_lmgen2_tok_t *tok;
	wtk_lmgen2_path_t *pth;
	wtk_string_t v;
	int i;
	wtk_strbuf_t *buf=g->buf;

	//wtk_debug("len=%d\n",g->tok_q.length);
	if(g->tok_q.length>0)
	{
		i=wtk_random(0,2);
		if(i==1 || g->tok_q.length==1)
		{
			qn=wtk_queue_peek(&(g->tok_q),0);
			tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
			pth=tok->pth;
		}else
		{
			i=min(g->tok_q.length,g->cfg->nbest);
			i=wtk_random(1,i-1);
			qn=wtk_queue_peek(&(g->tok_q),i);
			tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
			pth=tok->pth;
			//wtk_debug("%f/%f\n",g->max_pth->ppl,pth->ppl);
		}
		wtk_debug("prob=%f ppl=%f tok=%d\n",pth->prob,pth->ppl,g->tok_q.length);
		wtk_strbuf_reset(buf);
		wtk_lmgen2_print_path2(g,pth,buf);
		wtk_string_set(&(v),buf->data,buf->pos);
	}else
	{
		wtk_string_set(&(v),0,0);
	}
	return v;
}

void wtk_lmgen2_print(wtk_lmgen2_t *g)
{
	wtk_queue_node_t *qn;
	wtk_lmgen2_tok_t *tok;

	for(qn=g->tok_q.pop;qn;qn=qn->next)
	{
		tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
		wtk_lmgen2_print_path(g,tok->pth);
	}
	if(g->max_pth)
	{
		wtk_debug("prob=%f ppl=%f\n",g->max_pth->prob,g->max_pth->ppl);
		wtk_lmgen2_print_path(g,g->max_pth);
	}
}

void wtk_lmgen2_predict_next(wtk_lmgen2_t *g,wtk_lmgen2_path_t *pth,wtk_queue_t *snt_q)
{
	wtk_nglm_t *lm=g->lm;
	int i,n;
	wtk_lm_node_t *node,*node2;
	double f;
	wtk_lmgen2_path_t *pth2;
	wtk_lmgen2_tok_t *tok;
	double bow=0;
	int eof;
	double ppl=0;

	node=pth->node;
	wtk_nglm_touch_node(g->lm,node);
	n=node->nchild;
	if(n==0)
	{
		int ids[30];
		int cnt;

		cnt=wtk_lm_node_trace_id(node,ids);
		if(cnt<=1){return;}
		//wtk_debug("cnt=%d\n",cnt);
		bow=node->bow;
		//wtk_debug("cnt=%d/%d\n",cnt,ids[1]);
		node=wtk_nglm_get_node(lm,ids+1,cnt-1);
		if(!node){return;}
		wtk_nglm_touch_node(g->lm,node);
		n=node->nchild;
	}
	//wtk_debug("n=%d nchild=%d\n",node->ngram,n);
	for(i=0;i<n;++i)
	{
		node2=node->childs[i];
		if(wtk_lmgen2_is_filter(g,node2->id))
		{
			continue;
		}
		f=pth->prob+bow+node2->prob;
		//wtk_debug("v[%d]=%f\n",i,node->prob);
		//wtk_debug("id=%d\n",node->id);
		eof=node2->id==g->lm->dict_cfg->snt_end_id;
		if(eof)
		{
			ppl=pow(pow(10.0,f),-1.0/(pth->depth+1));
			if(ppl>g->max_ppl)
			{
				continue;
			}
		}else
		{
			if(f-g->last_max<g->cfg->predict_step_prob)
			{
				continue;
			}
			if(f>g->max)
			{
				g->max=f;
			}
		}
		wtk_prune_add(g->predict_prune,f-g->last_max);
		pth2=wtk_lmgen2_pop_path(g,node2,f);
		pth2->prob=f;
		pth2->prev=pth;
		pth2->depth=pth->depth+1;
		tok=wtk_lmgen2_pop_tok(g);
		tok->pth=pth2;
		if(eof)
		{
			tok->pth->ppl=ppl;
			//wtk_queue_push(snt_q,&(tok->q_n));
			wtk_queue_insert(snt_q,&(tok->q_n),(wtk_cmp_handler_t)wtk_lmgen2_tok_node_cmp);
		}else
		{
			wtk_queue_push(&(g->tok_q),&(tok->q_n));
		}
		//wtk_debug("v[%d]=%f\n",i,f);
		//wtk_prune_add(g->prune,g->last_max-tok->pth->ppl);
	}
}

void wtk_lmgen2_prune2(wtk_lmgen2_t *g,wtk_queue_t *snt_q)
{
	wtk_prune_t *p=g->predict_prune;
	int b;
	float thresh,f;
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	wtk_lmgen2_tok_t *tok;

	b=wtk_prune_want_prune(p);
	if(b==0)
	{
		return;
	}
	thresh=wtk_prune_get_thresh(p);
	//wtk_debug("[%d]=%f\n",g->tok_q.length,thresh);
	q=g->tok_q;
	wtk_queue_init(&(g->tok_q));
	while(1)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
		f=tok->pth->prob-g->last_max;
		//wtk_debug("f=%f/%f %f/%f\n",f,thresh,tok->pth->prob,g->last_max);
		if(f>=thresh)
		{
			wtk_queue_push(&(g->tok_q),qn);

		}else
		{
			wtk_lmgen2_push_tok(g,tok);
		}
	}
}


void wtk_lmgen2_flush(wtk_lmgen2_t *g,wtk_queue_t *snt_q)
{
	wtk_queue_t q;
	wtk_queue_node_t *qn;
	wtk_lmgen2_tok_t *tok;
	wtk_prune_t *p=g->predict_prune;

	q=g->tok_q;
	//wtk_debug("len=%d\n",q.length);
	wtk_queue_init(&(g->tok_q));
	wtk_prune_reset(p);
	g->last_max=g->max;
	g->max=-1e10;
	//wtk_debug("len=%d\n",q.length);
	while(1)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
		//wtk_debug("xxxxxxxxxxxxxxxxxxx\n");
		wtk_lmgen2_predict_next(g,tok->pth,snt_q);
	}
	//wtk_debug("tok=%d\n",g->tok_q.length);
	wtk_lmgen2_prune2(g,snt_q);
	//wtk_debug("tok=%d\n",g->tok_q.length);
	//exit(0);
}

wtk_string_t wtk_lmgen2_predict(wtk_lmgen2_t *g,char *v,int v_len,int max_wrd,float max_ppl)
{
	wtk_nglm_t *lm=g->lm;
	wtk_lm_node_t *node;
	wtk_string_t x;
	int id;
	char *s,*e,*p;
	int b;
	double prob,bow;
	int index=0;
	wtk_lmgen2_path_t *pth=NULL,*tmp;
	wtk_lmgen2_tok_t *tok;
	wtk_queue_t snt_q;
	int i;

	wtk_queue_init(&(snt_q));
	//wtk_lmgen2_start(g);
	//wtk_lmgen2_reset(g);
	g->max_ppl=max_ppl;
	node=g->lm->s_node;
	g->max=0;
	g->last_max=0;
	wtk_string_set(&(x),0,0);
	prob=0;
	id=wtk_fst_insym_get_index2(g->cfg->dict.sym,v,v_len);
	if(id<0)
	{
		s=v;
		e=s+v_len;
		while(s<e)
		{
			p=e;
			b=0;
			while(p>s)
			{
				//wtk_debug("[%.*s] %p/%p/%p\n",(int)(p-s),s,p,s,e);
				id=wtk_fst_insym_get_index2(g->cfg->dict.sym,s,p-s);
				if(id>=0)
				{
					node=wtk_nglm_get_child_prob(lm,node,id,&bow);
					if(!node){goto end;}
					prob+=node->prob+bow;
					tmp=wtk_lmgen2_pop_path(g,node,prob);
					tmp->prev=pth;
					pth=tmp;
					++index;
					s=p;
					b=1;
					break;
				}
				--p;
			}
			if(b==0)
			{
				++s;
			}
		}
	}else
	{
		node=wtk_nglm_get_child_prob(lm,node,id,&bow);
		if(!node){goto end;}
		prob+=node->prob+bow;
		++index;
		tmp=wtk_lmgen2_pop_path(g,node,prob);
		tmp->prev=pth;
		pth=tmp;
	}
	//wtk_debug("[%f]=%d\n",prob,index);
	tok=wtk_lmgen2_pop_tok(g);
	tok->pth=pth;
	pth->depth=index+1;
	g->max=pth->prob;
	wtk_queue_push(&(g->tok_q),&(tok->q_n));
	//wtk_lmgen2_predict_next(g,pth);
	for(i=0;i<max_wrd;++i)
	{
		wtk_lmgen2_flush(g,&(snt_q));
		if(g->tok_q.length==0 || snt_q.length>g->cfg->nbest)
		{
			break;
		}
	}
	//wtk_debug("snt=%d\n",snt_q.length);
	if(snt_q.length>0)
	{
		g->tok_q=snt_q;
	}else
	{
		wtk_queue_node_t *qn;
		wtk_queue_t q;
		double bow;

		//wtk_queue_sort(&(g->tok_q),(wtk_queue_node_cmp_f)wtk_lmgen2_tok_node_cmp);
		q=g->tok_q;
		wtk_queue_init(&(g->tok_q));
		while(1)
		{
			qn=wtk_queue_pop(&(q));
			if(!qn){break;}
			tok=data_offset2(qn,wtk_lmgen2_tok_t,q_n);
			node=wtk_nglm_get_child_prob(g->lm,tok->pth->node,g->cfg->dict.snt_end_id,&bow);
			tok->pth->prob+=node->prob+bow;
			tok->pth->ppl=pow(pow(10.0,tok->pth->prob),-1.0/(tok->pth->depth+1));
			//wtk_debug("len=%f\n",tok->pth->ppl);
			if(tok->pth->ppl>g->max_ppl)
			{
				//break;
			}else
			{
				//wtk_debug("prob=%f ngram=%d\n",bow,tok->pth->node->ngram);
				wtk_queue_insert(&(g->tok_q),&(tok->q_n),(wtk_cmp_handler_t)wtk_lmgen2_tok_node_cmp);
			}
		}
		//wtk_queue_sort(&(g->tok_q),(wtk_queue_node_cmp_f)wtk_lmgen2_tok_node_cmp);
		//wtk_debug("tok=%d\n",g->tok_q.length);
	}
	//wtk_lmgen2_print(g);
	x=wtk_lmgen2_get_result(g);
	//wtk_debug("[%.*s]\n",x.len,x.data);
end:
	wtk_lmgen2_reset(g);
	//exit(0);
	return x;
}
