#include "wtk_lmgen_rec.h"
#include "wtk_lmgen.h"
int wtk_lmgen_rec_dec(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hits,int n);

wtk_lmgen_rec_t* wtk_lmgen_rec_new(wtk_lmgen_rec_cfg_t *cfg,wtk_lmgen_t *gen)
{
	wtk_lmgen_rec_t *g;
	char *p="</s>";

	g=(wtk_lmgen_rec_t*)wtk_malloc(sizeof(wtk_lmgen_rec_t));
	g->cfg=cfg;
	g->gen=gen;
	g->dict=&(gen->cfg->dict);
	g->lm=wtk_nglm_new(&(cfg->lm),g->dict);
	g->heap=wtk_heap_new(4096);
	g->tx=(wtk_lmgen_tx_t*)wtk_malloc((cfg->ntok+1)*sizeof(wtk_lmgen_tx_t));
	g->end_id=wtk_fst_insym_get_index2(g->dict->sym,p,strlen(p));
	g->buf=wtk_strbuf_new(256,1);
	wtk_lmgen_rec_reset(g);
	return g;
}

void wtk_lmgen_rec_delete(wtk_lmgen_rec_t *g)
{
	wtk_strbuf_delete(g->buf);
	wtk_free(g->tx);
	wtk_heap_delete(g->heap);
	wtk_nglm_delete(g->lm);
	wtk_free(g);
}

void wtk_lmgen_rec_reset(wtk_lmgen_rec_t *g)
{
	wtk_heap_reset(g->heap);
	wtk_nglm_reset(g->lm);
	g->max_like=-10000000000.0;
	wtk_queue_init(&(g->tok_q));
	wtk_queue_init(&(g->output_q));
}

wtk_lmgen_tok_t* wtk_lmgen_rec_new_tok(wtk_lmgen_rec_t *g)
{
	wtk_lmgen_tok_t *tok;

	tok=(wtk_lmgen_tok_t*)wtk_heap_malloc(g->heap,sizeof(wtk_lmgen_tok_t));
	tok->node=NULL;
	tok->prob=-10000000;
	tok->hit_prob=0;
	tok->nhit=0;
	tok->a=wtk_array_new_h(g->heap,20,sizeof(unsigned int));
	tok->b=wtk_array_new_h(g->heap,20,sizeof(wtk_lm_node_t*));
	return tok;
}

#include <math.h>
/*
 *
logprob= -3.20177e+06
This gives us the total logprob ignoring the 34153 unknown word tokens. The logprob does include the probabilities assigned to </s> tokens which are introduced by ngram-count(1). Thus the total number of tokens which this logprob is based on is
	words - OOVs + sentences = 1000015 - 34153 + 61031
ppl = 1311.97
This gives us the geometric average of 1/probability of each token, i.e., perplexity. The exact expression is:
	ppl = 10^(-logprob / (words - OOVs + sentences))
ppl1 = 2065.09
This gives us the average perplexity per word excluding the </s> tokens. The exact expression is:
	ppl1 = 10^(-logprob / (words - OOVs))
 */
float wtk_log2ppl(float f,int nwrd)
{
	//f => log10 -> log2
	//log2 ( 10^f)
	//math.pow(2,math.log(math.pow(10,-7.44855),2)/-3.0)
	static float fx=0;

	if(fx==0)
	{
#ifdef __ANDROID__
		fx=log(10)/log(2);
#else
		fx=log2(10);
#endif
	}
	//wtk_debug("fx=%f/%d\n",fx,nwrd);
	return pow(2,-fx*f/nwrd);
}

void wtk_lmgen_rec_tok_update_ppl2(wtk_lmgen_tok_t *tok)
{
	tok->ppl=wtk_log2ppl(tok->prob,tok->a->nslot-1);
}

void wtk_lmgen_rec_tok_print(wtk_lmgen_rec_t *g,wtk_lmgen_tok_t *tok);

void wtk_lmgen_rec_tok_update_ppl(wtk_lmgen_rec_t *g,wtk_lmgen_tok_t *tok)
{
	float f;
	int *i;

	i=(int*)tok->a->slot;
	if(i[tok->a->nslot-1]==g->end_id)
	{
		f=wtk_log2ppl(tok->prob,tok->a->nslot-1);
		//f=-tok->prob/(tok->a->nslot-1);
	}else
	{
		f=wtk_log2ppl(tok->prob,tok->a->nslot);
		//f=-tok->prob/(tok->a->nslot);
	}
	//f=-tok->prob/tok->a->nslot;
	tok->ppl1=f;
	//wtk_debug("f=%f/%f\n",f,tok->hit_prob);
	f=f-tok->hit_prob*g->cfg->hit_scale2;//tok->hit_prob*g->cfg->hit_scale;//10;
	tok->ppl=f;
	//wtk_debug("prof=%f n=%d ppl=%f %f %f\n",tok->prob,tok->a->nslot,tok->ppl,tok->ppl1,tok->hit_prob);
//	if(tok->ppl>1000)
//	{
//		wtk_debug("prof=%f n=%d ppl=%f %f %f\n",tok->prob,tok->a->nslot,tok->ppl,tok->ppl1,tok->hit_prob);
//		wtk_lmgen_rec_tok_print(g,tok);
//		//exit(0);
//	}
}

void wtk_lmgen_rec_tok_print(wtk_lmgen_rec_t *g,wtk_lmgen_tok_t *tok)
{
	unsigned int *pi;
	int i;
	wtk_string_t *v;


	//wtk_lm_node_print2(g->cfg->dict.sym,tok->node);
	if(0)
	{
		wtk_lm_node_t **nodes;

		nodes=(wtk_lm_node_t**)tok->b->slot;
		for(i=0;i<tok->b->nslot;++i)
		{
			printf("v[%d] ",i);
			wtk_lm_node_print2(g->dict->sym,nodes[i]);
		}
	}
	pi=(unsigned int*)(tok->a->slot);
	//wtk_debug("prob=%f\n",tok->prob);
	wtk_lmgen_rec_tok_update_ppl(g,tok);
	printf("%f/%f/%f ",tok->ppl,tok->prob,tok->hit_prob);
	for(i=0;i<tok->a->nslot;++i)
	{
		v=g->dict->sym->ids[pi[i]]->str;
		//printf("v[%d]=%d %.*s\n",i,pi[i],v->len,v->data);
		printf(" %.*s",v->len,v->data);
	}
	printf("\n");
	//wtk_debug("%f/%d\n",wtk_log2ppl(tok->prob,tok->a->nslot-1),tok->a->nslot);
}

int wtk_lmgen_rec_out_cmp(wtk_queue_node_t *src,wtk_queue_node_t *dst)
{
	wtk_lmgen_out_t *o1,*o2;

	o1=data_offset2(src,wtk_lmgen_out_t,q_n);
	o2=data_offset2(dst,wtk_lmgen_out_t,q_n);
	return (o1->ppl-o2->ppl)>0?-1:1;
}

void wtk_lmgen_rec_print(wtk_lmgen_rec_t *g)
{
	wtk_queue_node_t *qn;
	wtk_lmgen_out_t *o1;
	int i;

	for(i=0,qn=g->output_q.pop;qn;qn=qn->next,++i)
	{
		o1=data_offset2(qn,wtk_lmgen_out_t,q_n);
//		printf("v[%d]: %f/%f/%f/%f/%f %.*s\n",i,o1->ppl,o1->ppl1,o1->tok->prob,o1->tok->hit_prob,
//				o1->tok->node->prob,
//				o1->v->len,o1->v->data);
		printf("v[%d]: %f/%f-%f/%f %.*s\n",i,o1->ppl,o1->tok->prob,o1->tok->ppl1,o1->tok->hit_prob,
				o1->v->len,o1->v->data);
		//wtk_lm_node_print();
		//wtk_lm_node_print2(g->dict->sym,o1->tok->node);
	}

	for(i=0,qn=g->output_q.pop;qn;qn=qn->next,++i)
	{
		o1=data_offset2(qn,wtk_lmgen_out_t,q_n);
		wtk_lm_node_print2(g->dict->sym,o1->tok->node);
	}
}

void wtk_lmgen_rec_add_output(wtk_lmgen_rec_t *g,wtk_lmgen_tok_t *tok)
{
	unsigned int *pi;
	int i;
	float f;
	wtk_strbuf_t *buf=g->buf;
	wtk_string_t *v;
	wtk_lmgen_out_t  *o;

	//wtk_lmgen_rec_tok_print(g,tok);
	if(tok->nhit<g->min_hit)
	{
		return;
	}
	//wtk_lm_node_print2(g->dict->sym,tok->node);
//	if(tok->a->nslot==2)
//	{
//		wtk_debug("%f\n",tok->ppl);
//		exit(0);
//	}

//	if(tok->node->ngram!=g->cfg->lm.max_order)
//	{
//		return;
//	}
	//if(tok->node->ngram==g->cfg->lm.max_order)
	{
		if(tok->node->prob<g->cfg->end_thresh)
		{
			goto end;
		}
	}
	wtk_lmgen_rec_tok_update_ppl(g,tok);
	f=tok->ppl;
	//wtk_debug("%f/%f/%f/%f\n",tok->ppl,tok->ppl1,tok->prob,tok->hit_prob);
	if(f>g->cfg->end_ppl_beam){goto end;}
	//wtk_lmgen_rec_tok_print(g,tok);
	pi=(unsigned int*)(tok->a->slot);
	wtk_strbuf_reset(buf);
	for(i=0;i<tok->a->nslot;++i)
	{
		v=g->dict->sym->ids[pi[i]]->str;
		//printf("v[%d]=%d %.*s\n",i,pi[i],v->len,v->data);
		//printf(" %.*s",v->len,v->data);
		if(buf->pos>0)
		{
			wtk_strbuf_push_s(buf," ");
		}
		wtk_strbuf_push(buf,v->data,v->len);
	}
	o=(wtk_lmgen_out_t*)wtk_heap_malloc(g->heap,sizeof(wtk_lmgen_out_t));
	o->ppl=f;
	o->ppl1=tok->ppl1;
	o->tok=tok;
	o->v=wtk_heap_dup_string(g->heap,buf->data,buf->pos);
	//wtk_debug("%f/%d %.*s\n",o->ppl,tok->a->nslot,o->v->len,o->v->data);

	wtk_queue_insert(&(g->output_q),&(o->q_n),(wtk_cmp_handler_t)wtk_lmgen_rec_out_cmp);
	//wtk_debug("============= add ==================>\n");
	//wtk_lmgen_rec_print(g);
	//exit(0);'
end:
	//exit(0);
	return;
}

void wtk_lmgen_rec_add_tok(wtk_lmgen_rec_t *g,wtk_lmgen_tok_t *tok)
{
	wtk_queue_push(&(g->tok_q),&(tok->q_n));
}

wtk_lmgen_hit_t* wtk_lmgen_rec_get_hit2(wtk_lmgen_rec_t *g,unsigned int id)
{
	int i;

	for(i=0;i<g->nhit;++i)
	{
		if(g->hits[i].hit_id==id)
		{
			//g->hist[i].use=1;
			return g->hits+i;
		}
	}
	return NULL;
}

int wtk_lmgen_rec_tok_has_hit(wtk_lmgen_tok_t *tok,unsigned int id)
{
	unsigned int *pi;
	int i;

	pi=(unsigned int*)tok->a->slot;
	for(i=0;i<tok->a->nslot;++i)
	{
		if(pi[i]==id)
		{
			return 1;
		}
	}
	return 0;
}

void wtk_lmgen_rec_step_node(wtk_lmgen_rec_t *g,wtk_lm_node_t *node,wtk_lmgen_tok_t *tok,int bk)
{
	wtk_lmgen_tx_t *tx=g->tx;
	wtk_lm_node_t *nx;
	wtk_lmgen_hit_t *hit;
	wtk_lmgen_tok_t *tk;
	int n=0;
	int i,j;
	static int ki=0;
	float f,scale;

	//wtk_debug("in=%d n=%d prob=%f\n",node->id,tok->a->nslot,tok->prob);
	++ki;
	if(tok && tok->a->nslot>2)
	{
		//wtk_debug("update %d\n",tok->a->nslot);
		wtk_lmgen_rec_tok_update_ppl(g,tok);
		if(tok->ppl>g->cfg->step_ppl_beam)
		{
			//wtk_debug("ppl[%d]=%f/%f\n",ki,tok->ppl,g->cfg->step_ppl_beam);
//			if(ki==293)
//			{
//				exit(0);
//			}
			return;
		}
	}
//	wtk_debug("ki=%d\n",ki);
	wtk_nglm_touch_node(g->lm,node);
//	wtk_lm_node_print2(g->cfg->dict.sym,node);
//	exit(0);
	//wtk_lm_node_print_child(g->cfg->dict.sym,node);
	//wtk_debug("nchild=%d\n",node->nchild);
	for(i=0;i<node->nchild;++i)
	{
		nx=node->childs[i];
		hit=wtk_lmgen_rec_get_hit2(g,nx->id);
		if(hit)
		{
			if(hit->stop)
			{
				scale=g->cfg->stop_scale;
			}else if(tok && wtk_lmgen_rec_tok_has_hit(tok,nx->id))
			{
				scale=g->cfg->has_hit_scale;
				//wtk_debug("hit[%.*s] %f\n",hit->hit_key.len,hit->hit_key.data,scale);
			}else
			{
				scale=g->cfg->hit_scale;
				hit->use=1;
			}
			f=hit->prob*scale+nx->prob;
//			wtk_debug("hit[%.*s] %f,%f s=%f/%f tot=%f\n",hit->hit_key.len,hit->hit_key.data,
//					nx->prob,hit->prob*scale,scale,
//					hit->prob,f);
		}else
		{
			f=nx->prob;
		}
		for(j=n-1;j>=0;--j)
		{
			if(f>tx[j].prob)
			{
				tx[j+1]=tx[j];
			}else
			{
				++j;
				break;
			}
		}
		//wtk_debug("j=%d n=%d prob=%f\n",j,n,nx->prob);
		if(j<g->cfg->ntok)
		{
			if(n<g->cfg->ntok)
			{
				++n;
			}
			if(j<0){j=0;}
			//wtk_debug("set j=%d\n",j);
			tx[j].node=nx;
			tx[j].prob=f;
			tx[j].hit_prob=f-nx->prob;//(hit&&hit->use)?hit->prob:0;
//			if(hit)
//			{
//				wtk_debug("hit=%f %.*s\n",tx[j].hit_prob,hit->hit_key.len,hit->hit_key.data);
//			}
		}
//		for(j=0;j<n;++j)
//		{
//			wtk_debug("v[%d/%d]==> %f\n",i,j,nodes[j]->prob);
//		}
//		if(i==1)
//		{
//			//exit(0);
//		}
	}
//	for(i=0;i<n;++i)
//	{
//		wtk_string_t *p;
//
//		p=g->cfg->dict.sym->ids[tx[i].node->id]->str;
//		wtk_debug("[%.*s]=%f/%f/%f\n",p->len,p->data,tx[i].prob,tx[i].hit_prob,tx[i].node->prob);
//	}
	//wtk_debug("n=%d\n",n);
	for(i=0;i<n;++i)
	{
		//wtk_lm_node_print2(g->cfg->dict.sym,nodes[i]);
		tk=wtk_lmgen_rec_new_tok(g);
		tk->node=tx[i].node;
		if(tok)
		{
			void *p;

			tk->prob=tok->prob;
			tk->hit_prob=tok->hit_prob;
			tk->nhit=tok->nhit;
			p=wtk_array_push_n(tk->a,tok->a->nslot);
			memcpy(p,tok->a->slot,tok->a->nslot*tok->a->slot_size);
			wtk_array_push2(tk->a,&(tk->node->id));

			p=wtk_array_push_n(tk->b,tok->b->nslot);
			memcpy(p,tok->b->slot,tok->b->nslot*tok->b->slot_size);
			wtk_array_push2(tk->b,&(tk->node));
		}else
		{
			wtk_array_push2(tk->a,&(node->id));
			wtk_array_push2(tk->a,&(tk->node->id));
			tk->prob=0;
			tk->hit_prob=0;
			tk->nhit=0;

			wtk_array_push2(tk->b,&(node));
			wtk_array_push2(tk->b,&(tk->node));
		}
		tk->prob+=tk->node->prob;
		if(tx[i].hit_prob>0)
		{
			++tk->nhit;
			tk->hit_prob+=tx[i].hit_prob;
		}
		//wtk_debug("%f/%f\n",tk->prob,tk->hit_prob);
		//wtk_debug("id=%d/%d\n",tx[i].node->id,g->end_id);
		if(tx[i].node->id==g->end_id)
		{
			wtk_lmgen_rec_add_output(g,tk);
		}else
		{
			//wtk_debug("node=%p tok=%p %p/%p\n",node,tok,tk->node,tk);
			//wtk_lmgen_rec_step_node(g,tk->node,tk);
			wtk_lmgen_rec_add_tok(g,tk);
		}
	}
	if(bk && tok && tok->a->nslot>1)
	{
		int ng;
		wtk_lm_node_t *nx;
		int *pi;

		//do bow;
		pi=(int*)(tok->a->slot);
		ng=tok->node->ngram-1;
		//wtk_debug("ng=%d\n",ng);
		if(tok->node->ngram>1)
		{
			nx=wtk_nglm_get_node(g->lm,pi+tok->a->nslot-ng,ng);
			if(nx)
			{
				tok->prob+=tok->node->bow;
				//wtk_debug("prob=%f\n",tok->prob);
				//tok->prob=tok->prob-tok->node->prob+nx->prob;//+tok->node->bow;
				//wtk_debug("prob2=%f\n",tok->prob);
				//wtk_lmgen_rec_tok_print(g,tok);
				//wtk_debug("ngram=%d bow=%f/%f\n",nx->ngram,nx->prob+tok->node->bow,tok->node->bow);
				//tok->node=nx;
				//wtk_debug("node=%p tok=%p %p/%p\n",node,tok,nx,tok);
				//wtk_lmgen_rec_step_node(g,nx,tok);
				tok->node=nx;
				//wtk_debug("tok=%p\n",tok);
				//wtk_lmgen_rec_add_tok(g,tk,);
				wtk_lmgen_rec_step_node(g,nx,tok,0);
			}
		}
//		wtk_lm_node_print2(g->cfg->dict.sym,nx);
//
//		wtk_lmgen_rec_tok_print(g,tok);
//		wtk_debug("found back wrod\n");
//		exit(0);
	}
}

void wtk_lmgen_rec_print_queue(wtk_lmgen_rec_t *g)
{
	wtk_queue_node_t *qn;
	wtk_lmgen_tok_t *tok;

	wtk_debug("============ depth=%d ============\n",g->depth);
	for(qn=g->tok_q.pop;qn;qn=qn->next)
	{
		tok=data_offset2(qn,wtk_lmgen_tok_t,q_n);
		wtk_lmgen_rec_tok_print(g,tok);
	}
}

int wtk_lmgen_tok_queue_cmp(wtk_queue_node_t *qn1,wtk_queue_node_t *qn2)
{
	wtk_lmgen_tok_t *tok1,*tok2;

	tok1=data_offset2(qn1,wtk_lmgen_tok_t,q_n);
	tok2=data_offset2(qn2,wtk_lmgen_tok_t,q_n);
	return (tok1->ppl-tok2->ppl)>0?1:-1;
}

void wtk_lmgen_update_tok(wtk_lmgen_rec_t *g)
{
	wtk_queue_node_t *qn;
	wtk_lmgen_tok_t *tok;

	for(qn=g->tok_q.pop;qn;qn=qn->next)
	{
		tok=data_offset2(qn,wtk_lmgen_tok_t,q_n);
		wtk_lmgen_rec_tok_update_ppl(g,tok);
	}
	wtk_queue_sort(&(g->tok_q),(wtk_queue_node_cmp_f)wtk_lmgen_tok_queue_cmp);
	if(g->cfg->max_tok>0)
	{
		while(g->tok_q.length>g->cfg->max_tok)
		{
			wtk_queue_pop_back(&(g->tok_q));
		}
	}
}

void wtk_lmgen_rec_process(wtk_lmgen_rec_t *g)
{
	wtk_queue_node_t *qn;
	wtk_lmgen_tok_t *tok;
	wtk_queue_t q;

	++g->depth;
	//wtk_lmgen_rec_print_queue(g);
	q=g->tok_q;
	wtk_queue_init(&(g->tok_q));
	while(q.length>0)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		tok=data_offset2(qn,wtk_lmgen_tok_t,q_n);
		wtk_lmgen_rec_step_node(g,tok->node,tok,1);
		//wtk_debug("len=%d /n=%d\n",g->tok_q.length,tok->node->nchild);
//		if(g->tok_q.length==0)
//		{
//			exit(0);
//		}
	}
	//wtk_debug("len=%d\n",g->tok_q.length);
	wtk_lmgen_update_tok(g);
	//wtk_debug("len=%d\n",g->tok_q.length);
	if(g->cfg->nbest>0)
	{
		while(g->output_q.length>g->cfg->nbest)
		{
			wtk_queue_pop_back(&(g->output_q));
		}
	}
}

void wtk_lmgen_rec_backward_wrd(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hit)
{
	wtk_lmgen_tok_t *tok;
	wtk_lm_node_t *node;
	double bow;
	float scale;

	if(1)
	{
		tok=wtk_lmgen_rec_new_tok(r);
		//node=wtk_nglm_get_uni_node(r->lm,hit->hit_id);
		if(0)
		{
			node=wtk_nglm_get_child_prob(r->lm,r->lm->s_node,hit->hit_id,&bow);
			wtk_debug("node=%p\n",node);
			if(!node)
			{
				return;
			}
			tok=wtk_lmgen_rec_new_tok(r);
			tok->node=node;
			tok->prob=node->prob+bow;
			tok->hit_prob=r->cfg->hit_scale*hit->prob;
			wtk_array_push2(tok->a,&(r->lm->s_node->id));
			wtk_array_push2(tok->b,&(r->lm->s_node));
			wtk_array_push2(tok->a,&(node->id));
			wtk_array_push2(tok->b,&(node));
			//wtk_debug("prob=%f\n",tok->prob);
			wtk_lm_node_print2(r->dict->sym,node);
			wtk_lmgen_rec_step_node(r,node,tok,1);
			wtk_debug("len=%d\n",r->tok_q.length);
		}
		//exit(0);

		if(1)
		{
			node=wtk_nglm_get_uni_node(r->lm,hit->hit_id);
			tok->node=node;
			tok->prob=node->prob;//+r->lm->s_node->bow;
			if(hit->stop)
			{
				scale=r->cfg->stop_scale;
			}else
			{
				scale=r->cfg->hit_scale;
			}
			tok->hit_prob=scale*hit->prob;
			tok->nhit=1;
			wtk_array_reset(tok->a);
			wtk_array_reset(tok->b);
			wtk_array_push2(tok->a,&(r->lm->s_node->id));
			wtk_array_push2(tok->b,&(r->lm->s_node));
			wtk_array_push2(tok->a,&(node->id));
			wtk_array_push2(tok->b,&(node));
			wtk_lmgen_rec_step_node(r,node,tok,1);
		}
	}else
	{
		node=wtk_nglm_get_uni_node(r->lm,hit->hit_id);
		wtk_lmgen_rec_step_node(r,node,NULL,1);
	}
}

int wtk_lmgen_rec_backward(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hits,int n)
{
	wtk_lmgen_hit_t *hit;
	int i;
	int index;
	wtk_queue_node_t *last_qn,*qn;
	int cnt=0;

	r->min_hit=1;
	r->hits=hits;
	r->nhit=n;
	r->depth=1;
	for(i=0;i<n;i+=1)
	{
		index=i;
		hit=r->hits+index;
		if(hit->stop)
		{
			continue;
		}
		//wtk_debug("check [%.*s]\n",hit->hit_key.len,hit->hit_key.data);
		++cnt;
		hit->use=1;
		wtk_lmgen_rec_backward_wrd(r,hit);
	}
	if(cnt==0)
	{
		wtk_lmgen_rec_backward_wrd(r,r->hits+0);
	}
	//for(i=0;i<5;++i)
	last_qn=r->output_q.pop;
	cnt=0;
	while(r->tok_q.length>0)
	{
		//wtk_debug("i=%d\n",i);
		//wtk_debug("depth=%d\n",r->depth);
		//wtk_debug("len=%d\n",r->tok_q.length);
		wtk_lmgen_rec_process(r);
		//wtk_debug("len=%d\n",r->tok_q.length);
		if(r->depth>r->cfg->max_depth)
		{
			break;
		}
		qn=r->output_q.pop;
		if(qn != last_qn)
		{
			last_qn=qn;
			cnt=0;
		}else if(last_qn)
		{
			wtk_lmgen_out_t  *out;

			out=data_offset2(last_qn,wtk_lmgen_out_t,q_n);
			++cnt;
			if((cnt>3 && out->tok->hit_prob>0) || out->tok->ppl<5)
			{
				//wtk_debug("break %d\n",cnt);
				break;
			}
		}else
		{
			cnt=0;
			last_qn=qn;
		}
	}
	return 0;
}

void wtk_lmgen_rec_add_tok_hist2(wtk_lmgen_rec_t *r,int *ids,int nid,wtk_lmgen_tok_t *back_tok)
{
	wtk_nglm_t *lm=r->lm;
	wtk_lm_node_t *node=NULL;
	wtk_lmgen_tok_t *tok;
	wtk_lmgen_hit_t *hit;
	int i;
	double bow,scale;
	int id;

	tok=wtk_lmgen_rec_new_tok(r);
	tok->prob=0;
	tok->hit_prob=0;
	tok->nhit=0;
	node=lm->s_node;
	wtk_array_push2(tok->a,&(node->id));
	wtk_array_push2(tok->b,&(node));
	tok->node=node;
	//nid-=1;
	for(i=1;i<nid-1;++i)
	{
		id=ids[nid-i-1];
		//p=r->dict->sym->ids[id]->str;
		//wtk_debug("id=%d %.*s\n",id,p->len,p->data);
		//continue;
		if(i==0)
		{
			node=wtk_nglm_get_uni_node(lm,id);
			if(!node){goto end;}
			tok->node=node;
			tok->prob=node->prob;
			hit=wtk_lmgen_rec_get_hit2(r,id);
			if(hit)
			{
				tok->hit_prob=r->cfg->hit_scale*hit->prob;
			}
			wtk_array_push2(tok->a,&(lm->s_node->id));
			wtk_array_push2(tok->b,&(lm->s_node));
			wtk_array_push2(tok->a,&(node->id));
			wtk_array_push2(tok->b,&(node));
		}else
		{
			bow=0;
			node=wtk_nglm_get_child_prob(lm,tok->node,id,&bow);
			if(!node)
			{
				goto end;
			}
			//wtk_debug("node=%f %f %f\n",tok->prob,node->prob,bow);
			//exit(0);
			//wtk_lm_node_print2(lm->dict_cfg->sym,node);
			tok->prob+=node->prob+bow;
			tok->node=node;
			hit=wtk_lmgen_rec_get_hit2(r,id);
			if(hit)
			{
				if(hit->stop)
				{
					scale=r->cfg->stop_scale;
				}else if(wtk_lmgen_rec_tok_has_hit(tok,id))
				{
					scale=r->cfg->has_hit_scale;
				}else
				{
					scale=r->cfg->hit_scale;
				}
				tok->hit_prob+=hit->prob*scale;
				++tok->nhit;
			}
			wtk_array_push2(tok->a,&(node->id));
			wtk_array_push2(tok->b,&(node));
		}
	}
	//tok->prob+=back_tok->prob*0.01;
	//wtk_debug("tok=%f /%d\n",tok->prob,tok->a->nslot);
	wtk_lmgen_rec_tok_update_ppl(r,tok);
	wtk_lmgen_rec_add_tok(r,tok);
end:
	//wtk_debug("ppl=%f/%f/%f\n",tok->prob,tok->ppl,tok->hit_prob);
	//exit(0);
	return;
}

void wtk_lmgen_rec_step_node2(wtk_lmgen_rec_t *g,wtk_lm_node_t *node,wtk_lmgen_tok_t *tok
		,int bk,int id,wtk_queue_t *q)
{
	wtk_lm_node_t *nx;
	wtk_lmgen_hit_t *hit;
	wtk_lmgen_tok_t *tk;
	static int ki=0;
	float f,scale;

	if(tok && tok->a->nslot>1)
	{
		wtk_lmgen_rec_tok_update_ppl(g,tok);
		if(tok->ppl>g->cfg->step_ppl_beam)
		{
			return;
		}
	}
	++ki;
//	wtk_debug("ki=%d\n",ki);
	wtk_nglm_touch_node(g->lm,node);
//	wtk_lm_node_print2(g->cfg->dict.sym,node);
//	exit(0);
	//wtk_lm_node_print_child(g->cfg->dict.sym,node);
	//wtk_debug("nchild=%d\n",node->nchild);
	nx=wtk_nglm_get_child(g->lm,node,id);
	wtk_debug("id=%d nx=%p\n",id,nx);
	if(nx)
	{
		hit=wtk_lmgen_rec_get_hit2(g,nx->id);
		if(hit)
		{
			if(tok && wtk_lmgen_rec_tok_has_hit(tok,nx->id))
			{
				scale=g->cfg->has_hit_scale;
			}else
			{
				scale=g->cfg->hit_scale;
				hit->use=1;
			}
			f=hit->prob*scale+nx->prob;

		}else
		{
			f=nx->prob;
		}
		tk=wtk_lmgen_rec_new_tok(g);
		tk->node=nx;
		if(tok)
		{
			void *p;

			tk->prob=tok->prob;
			tk->hit_prob=tok->hit_prob;
			p=wtk_array_push_n(tk->a,tok->a->nslot);
			memcpy(p,tok->a->slot,tok->a->nslot*tok->a->slot_size);
			wtk_array_push2(tk->a,&(tk->node->id));

			p=wtk_array_push_n(tk->b,tok->b->nslot);
			memcpy(p,tok->b->slot,tok->b->nslot*tok->b->slot_size);
			wtk_array_push2(tk->b,&(tk->node));
		}else
		{
			wtk_array_push2(tk->a,&(node->id));
			wtk_array_push2(tk->a,&(tk->node->id));
			tk->prob=0;
			tk->hit_prob=0;

			wtk_array_push2(tk->b,&(node));
			wtk_array_push2(tk->b,&(tk->node));
		}
		tk->prob+=tk->node->prob;
		f-=nx->prob;
		if(f>0)
		{
			tk->hit_prob+=f;
			++tk->nhit;
		}
		//wtk_debug("%f/%f\n",tk->prob,tk->hit_prob);
		if(tk->node->id==g->end_id)
		{
			wtk_lmgen_rec_add_output(g,tk);
		}else
		{
			wtk_queue_push(q,&(tk->q_n));
		}
	}
	wtk_debug("len=%d\n",q->length);
	//wtk_lm_node_print2(g->dict->sym,node);
	wtk_debug("bk=%d tok=%p\n",bk,tok);
	if(bk && tok)
	{
		int ng;
		wtk_lm_node_t *nx;
		int *pi;

		//do bow;
		pi=(int*)(tok->a->slot);
		ng=tok->node->ngram-1;
		wtk_debug("ng=%d\n",ng);
		if(tok->node->ngram>1)
		{
			nx=wtk_nglm_get_node(g->lm,pi+tok->a->nslot-ng,ng);
			wtk_debug("nx=%p\n",nx);
			if(nx)
			{
				tok->prob+=tok->node->bow;
				//tok->node=nx;
				//wtk_debug("node=%p tok=%p %p/%p\n",node,tok,nx,tok);
				//wtk_lmgen_rec_step_node(g,nx,tok);
				tok->node=nx;
				//wtk_debug("tok=%p\n",tok);
				//wtk_lmgen_rec_add_tok(g,tk,);
				wtk_lmgen_rec_step_node2(g,nx,tok,0,id,q);
			}
		}
//		wtk_lm_node_print2(g->cfg->dict.sym,nx);
//
//		wtk_lmgen_rec_tok_print(g,tok);
//		wtk_debug("found back wrod\n");
//		exit(0);
	}
}


void wtk_lmgen_rec_add_tok_hist(wtk_lmgen_rec_t *r,int *ids,int nid)
{
	wtk_queue_t q,q2;
	wtk_queue_node_t *qn;
	wtk_lm_node_t *node;
	wtk_lmgen_tok_t *tok;
	int i;

	wtk_queue_init(&(q));
	wtk_queue_init(&(q2));
	node=wtk_nglm_get_uni_node(r->lm,ids[0]);
	if(!node){goto end;}
	tok=wtk_lmgen_rec_new_tok(r);
	wtk_array_push2(tok->a,&(node->id));
	wtk_array_push2(tok->b,&(node));
	tok->node=node;
	tok->prob=node->prob;
	wtk_lmgen_rec_step_node2(r,node,tok,1,ids[1],&(q));
	wtk_debug("len=%d id=%d\n",q.length,ids[1]);
	//exit(0);
	for(i=2;i<nid;++i)
	{
		wtk_debug("len=%d id=%d\n",q.length,ids[i]);
		q2=q;
		wtk_queue_init(&(q));
		while(q2.length>0)
		{
			qn=wtk_queue_pop(&(q2));
			if(!qn){break;}
			tok=data_offset2(qn,wtk_lmgen_tok_t,q_n);
			wtk_lmgen_rec_step_node2(r,tok->node,tok,1,ids[i],&(q));
		}
	}
	while(1)
	{
		qn=wtk_queue_pop(&(q));
		if(!qn){break;}
		wtk_queue_push(&(r->tok_q),qn);
	}
end:
	return;
}

void wtk_lmgen_rec_forward_wrd(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hit)
{
	wtk_lmgen_tok_t *tok;
	wtk_lm_node_t *node;
	double bow;
	//float scale;


	node=wtk_nglm_get_child_prob(r->lm,r->lm->s_node,hit->hit_id,&bow);
	//wtk_debug("node=%p\n",node);
	wtk_lm_node_print2(r->dict->sym,node);
	if(!node)// || node->prob<-0.18)
	{
		return;
	}

	tok=wtk_lmgen_rec_new_tok(r);
	tok->node=node;
	tok->prob=node->prob+bow;
	tok->hit_prob=r->cfg->hit_scale*hit->prob;
	wtk_array_push2(tok->a,&(r->lm->s_node->id));
	wtk_array_push2(tok->b,&(r->lm->s_node));
	wtk_array_push2(tok->a,&(node->id));
	wtk_array_push2(tok->b,&(node));
	//wtk_debug("prob=%f\n",tok->prob);
	//wtk_lm_node_print2(r->dict->sym,node);
	wtk_lmgen_rec_step_node(r,node,tok,1);
	//wtk_debug("len=%d\n",r->tok_q.length);
}

void wtk_lmgen_rec_forward_wrd2(wtk_lmgen_rec_t *r)
{
	wtk_lmgen_tok_t *tok;
	wtk_lm_node_t *node;
	//double bow;
	//float scale;

	node=r->lm->s_node;
	tok=wtk_lmgen_rec_new_tok(r);
	tok->node=node;
	tok->prob=0;
	tok->hit_prob=0;
	wtk_array_push2(tok->a,&(node->id));
	wtk_array_push2(tok->b,&(node));
	//wtk_debug("prob=%f\n",tok->prob);
	//wtk_lm_node_print2(r->dict->sym,node);
	wtk_lmgen_rec_step_node(r,node,tok,1);
	//wtk_debug("len=%d\n",r->tok_q.length);
}


int wtk_lmgen_rec_forward(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hits,int n,wtk_queue_t *backward_q)
{
	wtk_lmgen_out_t *out;
	wtk_queue_node_t *qn,*last_qn;
	int cnt;
	wtk_lmgen_hit_t *hit;
	int i;

	r->min_hit=1;
	r->hits=hits;
	r->nhit=n;
	r->depth=1;
	for(qn=backward_q->pop;qn;qn=qn->next)
	{
		out=data_offset2(qn,wtk_lmgen_out_t,q_n);
		//wtk_debug("oput=%p\n",out);
		wtk_lmgen_rec_add_tok_hist2(r,(int *)out->tok->a->slot,out->tok->a->nslot,out->tok);
		wtk_debug("depth=%d tok=%d output=%d\n",r->depth,r->tok_q.length,r->output_q.length);
		//exit(0);
	}
	//wtk_lmgen_rec_forward_wrd2(r);
	for(i=0;i<n;++i)
	{
		hit=r->hits+i;
		if(hit->stop)
		{
			continue;
		}
		//wtk_debug("check [%.*s]\n",hit->hit_key.len,hit->hit_key.data);
		wtk_lmgen_rec_forward_wrd(r,hit);
	}
	wtk_debug("depth=%d tok=%d output=%d\n",r->depth,r->tok_q.length,r->output_q.length);
	last_qn=r->output_q.pop;
	cnt=0;
	while(r->tok_q.length>0)
	{
		//wtk_debug("i=%d\n",i);
		//++r->depth;
		//wtk_debug("depth=%d tok=%d output=%d\n",r->depth,r->tok_q.length,r->output_q.length);
		//exit(0);
		wtk_lmgen_rec_process(r);
		//wtk_debug("depth=%d tok=%d output=%d\n",r->depth,r->tok_q.length,r->output_q.length);
		//wtk_debug("len=%d\n",r->tok_q.length);
		if(r->depth>r->cfg->max_depth)
		{
			break;
		}
		qn=r->output_q.pop;
		if(qn != last_qn)
		{
			last_qn=qn;
			cnt=0;
		}else if(last_qn)
		{
			wtk_lmgen_out_t  *out;

			out=data_offset2(last_qn,wtk_lmgen_out_t,q_n);
			++cnt;
			if((out->tok->hit_prob>0 && cnt>4))
			{
				//wtk_debug("break cnt=%d...\n",cnt);
				break;
			}
		}else
		{
			last_qn=qn;
			cnt=0;
		}
	}
	wtk_debug("tok=%d %d\n",r->tok_q.length,r->output_q.length);
	return 0;
}

//--------------------- decoder ---------------------------

int wtk_lmgen_rec_add_node(wtk_lmgen_rec_t *r,wtk_lmgen_tx_t *tx,int n,wtk_lm_node_t *node,float f)
{
	return 0;
}

void wtk_lmgen_tx_print(wtk_lmgen_rec_t *r,wtk_lmgen_tx_t *tx,int n)
{
	wtk_lmgen_tx_t *t;
	//wtk_string_t *v;
	int i;

	for(i=0;i<n;++i)
	{
		t=tx+i;
		//v=r->dict->sym->ids[t->node->id]->str;
		//wtk_debug("v[%d]=[%.*s/%f]\n",i,v->len,v->data,t->prob);
		wtk_lm_node_print2(r->dict->sym,t->node);
	}
}

void wtk_lmgen_rec_add_next_node(wtk_lmgen_rec_t *r,wtk_lmgen_tok_t *tok)
{
	wtk_nglm_t *lm=r->lm;
	wtk_lm_node_t *node,*nx,*nx2;
	wtk_lmgen_hit_t *hit;
	double bow;
	int i;
	wtk_lmgen_tx_t *tx=r->tx;
	//int cnt;
	int j,n;
	float f,f2;
	int b;

	node=tok->node;
	wtk_nglm_touch_node(lm,node);
	n=0;
	for(i=0;i<r->nhit;++i)
	{
		hit=r->hits+i;
		if(wtk_lmgen_rec_tok_has_hit(tok,hit->hit_id))
		{
			wtk_debug("has hit\n");
			continue;
		}
		nx=wtk_nglm_get_child_prob(lm,node,hit->hit_id,&bow);
		if(!nx)
		{
			wtk_debug("child prob\n");
			continue;
		}
		//wtk_debug("[%.*s]=%f/%f\n",hit->hit_key.len,hit->hit_key.data,nx->prob,bow);
		f=tok->prob+nx->prob+bow;//+hit->prob*r->cfg->hit_scale;
		f=wtk_log2ppl(f,tok->a->nslot);//+hit->prob*r->cfg->hit_scale;
		wtk_debug("[%.*s]=%f/%f ppl=%f\n",hit->hit_key.len,hit->hit_key.data,nx->prob,bow,f);
		n=wtk_lmgen_rec_add_node(r,tx,n,nx,f);
	}
	wtk_lmgen_tx_print(r,tx,n);
	wtk_debug("============> n=%d hint=%d\n",n,r->nhit);
	for(i=0;i<node->nchild;++i)
	{
		nx=node->childs[i];
		b=0;
		for(j=0;j<n;++j)
		{
			if(tx[j].node==nx)
			{
				b=1;
			}
		}
		//wtk_debug("i=%d b=%d\n",i,b);
		if(b){continue;}
		if(0)
		{
			wtk_string_t *v;

			v=r->dict->sym->ids[nx->id]->str;
			wtk_debug("[%.*s]=%f\n",v->len,v->data,nx->prob);
		}
		f=tok->prob+nx->prob;
		//f=wtk_log2ppl(f,tok->a->nslot+1);
		//n=wtk_lmgen_rec_add_node(r,tx,n,nx,f);
		wtk_nglm_touch_node(lm,node);
		for(j=0;j<r->nhit;++j)
		{
			hit=r->hits+j;
			if(wtk_lmgen_rec_tok_has_hit(tok,hit->hit_id))
			{
				continue;
			}
			nx2=wtk_nglm_get_child_prob(lm,nx,hit->hit_id,&bow);
			if(!nx2)
			{
				continue;
			}
			f2=f+nx->prob+bow;//+hit->prob*r->cfg->hit_scale;
			f2=wtk_log2ppl(f2,tok->a->nslot+2);
			//wtk_debug("[%.*s]=%f/%f\n",hit->hit_key.len,hit->hit_key.data,nx->prob,bow);
			n=wtk_lmgen_rec_add_node(r,tx,n,nx2,f2);
		}
	}
	wtk_lmgen_tx_print(r,tx,n);
	//exit(0);
}

int wtk_lmgen_rec_dec(wtk_lmgen_rec_t *r,wtk_lmgen_hit_t *hits,int n)
{
	wtk_lm_node_t *node;
	wtk_lmgen_tok_t *tok;

	r->hits=hits;
	r->nhit=n;
	node=r->lm->s_node;
	tok=wtk_lmgen_rec_new_tok(r);
	tok->node=node;
	tok->prob=0;
	tok->hit_prob=0;
	wtk_array_push2(tok->a,&(node->id));
	wtk_array_push2(tok->b,&(node));

	wtk_lmgen_rec_add_next_node(r,tok);
	return 0;
}
