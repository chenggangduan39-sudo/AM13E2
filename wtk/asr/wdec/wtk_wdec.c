#include "wtk_wdec.h" 
#include "wtk/core/wtk_strbuf.h"
void wtk_wdec_process_tok(wtk_wdec_t *w,wtk_wdec_tok_t *tok,wtk_wdec_arc_t *input_arc,int sframe);

static wtk_wdec_tok_t null_tok={
		NULL,LZERO,0
};


wtk_wdec_t* wtk_wdec_new(wtk_wdec_cfg_t *cfg)
{
	wtk_wdec_t *dec;
	int cache=100;

	//cfg->fextra.dnn.skip_frame=0;
	dec=(wtk_wdec_t*)wtk_malloc(sizeof(wtk_wdec_t));
	dec->cfg=cfg;
	dec->sil_hmm=wtk_hmmset_find_hmm_s(cfg->net.hmmset.hmmset,"sil");
	dec->net=wtk_wdec_net_new(&(cfg->net));//cfg->dict,cfg->word,(wtk_string_t**)(cfg->phn->slot),cfg->phn->nslot);
	dec->fextra=wtk_fextra_new(&(cfg->fextra));
	//dec->heap=wtk_heap_new(4096);
	dec->max_hmm_state=cfg->net.hmmset.hmmset->max_hmm_state;
	dec->prune=wtk_prune_new(&(cfg->prune));
	dec->pth_pool=wtk_vpool_new(sizeof(wtk_wdec_pth_t),cache);
	dec->inst_pool=wtk_vpool_new(sizeof(wtk_wdec_inst_t),cache);
	dec->tok_pool=wtk_vpool_new(dec->max_hmm_state*sizeof(wtk_wdec_tok_t),cache);
	wtk_queue_init(&(dec->feat_q));
	wtk_fextra_set_output_queue(dec->fextra,&(dec->feat_q));
	wtk_wdec_reset(dec);
	return dec;
}

void wtk_wdec_delete(wtk_wdec_t *w)
{
	wtk_vpool_delete(w->pth_pool);
	wtk_vpool_delete(w->inst_pool);
	wtk_vpool_delete(w->tok_pool);
	wtk_prune_delete(w->prune);
	wtk_wdec_net_delete(w->net);
	wtk_fextra_delete(w->fextra);
	//wtk_heap_delete(w->heap);
	wtk_free(w);
}

void wtk_wdec_reset(wtk_wdec_t *w)
{
	wtk_vpool_reset(w->pth_pool);
	wtk_vpool_reset(w->inst_pool);
	wtk_vpool_reset(w->tok_pool);
	w->emit_thresh=-10;
	w->found=0;
	w->wake_sframe=0;
	w->wake_eframe=0;
	w->final_pth=NULL;
	wtk_fextra_reset(w->fextra);
	//wtk_heap_reset(w->heap);
	wtk_queue_init(&(w->inst_q));
	wtk_wdec_net_reset(w->net);
	w->bk_tok=(wtk_wdec_tok_t*)wtk_vpool_pop(w->tok_pool);
}

void wtk_wdec_reset2(wtk_wdec_t *w)
{
    wtk_vpool_reset(w->pth_pool);
    wtk_vpool_reset(w->inst_pool);
    wtk_vpool_reset(w->tok_pool);
    w->emit_thresh=-10;
    w->found=0;
    w->wake_sframe=0;
    w->wake_eframe=0;
    w->final_pth=NULL;
    //wtk_fextra_reset(w->fextra);
    //wtk_heap_reset(w->heap);
    wtk_queue_init(&(w->inst_q));
    wtk_wdec_net_reset(w->net);
    w->bk_tok=(wtk_wdec_tok_t*)wtk_vpool_pop(w->tok_pool);
}

void wtk_wdec_reset_post(wtk_wdec_t *w)
{
	wtk_vpool_reset(w->pth_pool);
	wtk_vpool_reset(w->inst_pool);
	wtk_vpool_reset(w->tok_pool);
	w->emit_thresh=-10;
	w->found=0;
	w->wake_sframe=0;
	w->wake_eframe=0;
	w->final_pth=NULL;
	wtk_queue_init(&(w->inst_q));
	wtk_wdec_net_reset(w->net);
	w->bk_tok=(wtk_wdec_tok_t*)wtk_vpool_pop(w->tok_pool);
}

int wtk_wdec_bytes(wtk_wdec_t *w)
{
	int i;

	i=sizeof(wtk_wdec_t);
	i+=wtk_vpool_bytes(w->pth_pool);
	i+=wtk_vpool_bytes(w->inst_pool);
	i+=wtk_vpool_bytes(w->tok_pool);
	return i;
}

void wtk_wdec_start(wtk_wdec_t *w,wtk_wdec_post_cfg_t *post)
{
	wtk_wdec_tok_t tok;

	w->post=post?post:&(w->cfg->post);
	w->nframe=0;
	tok.like=0;
	tok.dnn_like=0;
	tok.path=NULL;
	wtk_wdec_process_tok(w,&tok,NULL,0);
}

wtk_wdec_inst_t* wtk_wdec_pop_inst(wtk_wdec_t *w,int sframe,wtk_wdec_arc_t *arc)
{
	wtk_wdec_inst_t *inst;
	int i,n;

	//wtk_wdec_node_print_phn_pre(arc->pre);
	//inst=(wtk_wdec_inst_t*)wtk_heap_malloc(w->heap,sizeof(wtk_wdec_inst_t));
	inst=(wtk_wdec_inst_t*)wtk_vpool_pop(w->inst_pool);
	inst->sframe=sframe;
	inst->arc=arc;
	n=w->net->cfg->hmmset.hmmset->max_hmm_state;
	//wtk_debug("n=%d new inst=%p\n",n,inst);
	//inst->state=(wtk_wdec_tok_t*)wtk_heap_malloc(w->heap,sizeof(wtk_wdec_tok_t)*n);
	inst->state=(wtk_wdec_tok_t*)wtk_vpool_pop(w->tok_pool);
	for(i=0;i<n;++i)
	{
		inst->state[i]=null_tok;
	}
	wtk_queue3_push(&(arc->inst_q),&(inst->arc_n));

	//wtk_debug("arc=%p  %p/%p\n",inst->arc,inst->arc->pre,inst->arc->nxt);
	//wtk_wdec_node_print_phn_pre(inst->arc->pre);
	//wtk_wdec_node_print_phn_nxt(inst->arc->pre);
	//exit(0);
	//wtk_wdec_node_print_phn_pre(inst->arc->nxt);
	return inst;
}


int wtk_wdec_is_check_path(wtk_wdec_pth_t *pth)
{
	wtk_string_t v=wtk_string("sil");
	int nx=50;

	while(pth)
	{
		if(pth->frame==nx && wtk_string_cmp(pth->arc->v.hmm->name,v.data,v.len)==0)
		{
			wtk_wdec_pth_print(pth);
			wtk_debug("found\n");
			return 1;
		}
		pth=pth->prev;
	}
	return 0;
}

int wtk_wdec_is_check_inst(wtk_wdec_inst_t *inst)
{
	wtk_wdec_tok_t *tok;
	wtk_wdec_pth_t *pth;
	int i;

	for(i=0;i<inst->arc->v.hmm->num_state;++i)
	{
		tok=inst->state+i;
		pth=tok->path;
		if(wtk_wdec_is_check_path(pth))
		{
			wtk_wdec_pth_print(pth);
			wtk_debug("found i=%d\n",i);
			return 1;
		}
	}
	return 0;
}

int wtk_wdec_check_inst(wtk_wdec_t *w)
{
	wtk_queue_node_t *qn;
	wtk_wdec_inst_t *inst;

	if(w->nframe<51)
	{
		return 1;
	}
	wtk_debug("========== check frame=%d =========\n",w->nframe);
	for(qn=w->inst_q.pop;qn;qn=qn->next)
	{
		inst=data_offset2(qn,wtk_wdec_inst_t,q_n);
		if(wtk_wdec_is_check_inst(inst))
		{
			wtk_debug("=========== inst=%p ============\n",inst);
			return 1;
		}
	}
	exit(0);
	return 0;
}


void wtk_wdec_push_inst(wtk_wdec_t *w,wtk_wdec_inst_t *inst)
{
	if(wtk_wdec_is_check_inst(inst))
	{
		wtk_debug("remove taget inst\n");
		exit(0);
	}
	wtk_queue3_remove(&(inst->arc->inst_q),&(inst->arc_n));
	wtk_vpool_push(w->tok_pool,inst->state);
	wtk_vpool_push(w->inst_pool,inst);
}


void wtk_wdec_pth_print2(wtk_wdec_pth_t *pth,wtk_strbuf_t *buf)
{
	if(pth->prev)
	{
		wtk_wdec_pth_print2(pth->prev,buf);
	}
	if(buf->pos>0)
	{
		wtk_strbuf_push_s(buf," ");
	}
//	if(pth->arc->nxt && pth->arc->nxt->word)
//	{
//		float wtk_wdec_pth_get_wrd_conf(wtk_wdec_t *dec,wtk_wdec_pth_t *pth)
//	}
	wtk_strbuf_push_f(buf,"%.*s[%d,%f]",pth->arc->v.hmm->name->len,pth->arc->v.hmm->name->data,pth->frame,pth->like);
	//wtk_strbuf_push_f(buf,"%.*s[%d]",pth->arc->v.hmm->name->len,pth->arc->v.hmm->name->data,pth->frame);
}

float wtk_wdec_pth_get_conf2(wtk_wdec_pth_t *pth)
{
	wtk_wdec_pth_t *s,*e,*lst;
	float conf=0;

	while(pth && wtk_string_cmp_s(pth->arc->v.hmm->name,"sil")!=0)
	{
		pth=pth->prev;
	}
	if(!pth){goto end;}
	e=pth->prev;
	pth=pth->prev;
	lst=NULL;
	while(pth && wtk_string_cmp_s(pth->arc->v.hmm->name,"sil")!=0)
	{
		lst=pth;
		pth=pth->prev;
	}
	if(!pth){goto end;}
	s=lst;
	conf=(e->like-s->like)/(e->frame-s->frame);
	//wtk_debug("%d/%f  %d/%f\n",s->frame,s->like,e->frame,e->like);
end:
	return conf;
}

void wtk_wdec_pth_print_conf(wtk_wdec_pth_t *pth)
{
	while(pth)
	{
		wtk_debug("%.*s",pth->arc->v.hmm->name->len,pth->arc->v.hmm->name->data);
		if(pth->arc->nxt && pth->arc->nxt->word)
		{
			printf(" [%.*s]",pth->arc->nxt->word->name->len,pth->arc->nxt->word->name->data);
		}
		printf(" [%d,%f]\n",pth->frame,pth->like);
		pth=pth->prev;
	}
}

float wtk_wdec_pth_get_wrd_conf(wtk_wdec_t *dec,wtk_wdec_pth_t *pth)
{
	wtk_wdec_pth_t *cur=pth;
	float f;

	//wtk_wdec_pth_print(pth);
	pth=pth->prev;
	while(pth)
	{
		if(pth->arc->nxt->word || pth->arc->v.hmm==dec->sil_hmm)
		{
			break;
		}
		pth=pth->prev;
	}
	f=(cur->like-pth->like)/(cur->frame-pth->frame);
	//wtk_debug("%f-%f/%d=%f\n",cur->like,pth->like,cur->frame-pth->frame,f);
	//wtk_debug("f=%f\n",f);
	//exit(0);
	return f;
}


float wtk_wdec_pth_get_conf(wtk_wdec_t *w,wtk_wdec_pth_t *pth)
{
	wtk_wdec_pth_t *s,*e,*lst;
	float conf=0;
	//wtk_wdec_pth_t *input=pth;

	//wtk_wdec_pth_print(pth);
	while(pth && pth->arc->v.hmm!=w->sil_hmm)
	{
		pth=pth->prev;
	}
	if(!pth){goto end;}
	e=pth->prev;
	pth=pth->prev;
	//wtk_debug("pth=%d\n",pth->frame);
	lst=NULL;
	while(pth && pth->arc->v.hmm!=w->sil_hmm)
	{
		lst=pth;
		pth=pth->prev;
	}
	if(!pth){goto end;}
	s=lst->prev;
	conf=(e->like-s->like)/(e->frame-s->frame);
//	wtk_debug("%.*s[%d,%f] %.*s[%d,%f] conf=%f\n",s->arc->v.hmm->name->len,s->arc->v.hmm->name->data,s->frame,s->like,
//			e->arc->v.hmm->name->len,e->arc->v.hmm->name->data,e->frame,e->like,conf);
	//wtk_wdec_pth_print_conf(input);
	//exit(0);
	//wtk_debug("%d/%d %f/%f %f/%f\n",s->frame,e->frame,s->like,e->like,s->dnn_like,e->dnn_like);
	//exit(0);
end:
	return conf;
}


void wtk_wdec_pth_print(wtk_wdec_pth_t *pth)
{
	wtk_strbuf_t *buf;
	float f;

	buf=wtk_strbuf_new(1024,1);
	wtk_wdec_pth_print2(pth,buf);
	f=wtk_wdec_pth_get_conf2(pth);
	wtk_debug("[%.*s] like=%f\n",buf->pos,buf->data,f);//th->like/pth->frame);
	wtk_strbuf_delete(buf);
}

float* wtk_wdec_get_post_wrd_conf(wtk_wdec_t *w,wtk_wdec_pth_t *pth)
{
	int i;
	float *wrd_conf=NULL;
	wtk_wdec_words_set_t *set;
	//wtk_wdec_pth_t *p=pth;
	while(pth)
	{
		if(pth->arc && pth->arc->label>0)
		{
			break;
		}
		pth=pth->prev;
	}
	//wtk_debug("label=%d\n",pth->arc->label);
	for(i =0;i<w->net->cfg->n_words;++i)
	{
		set = w->net->cfg->set+i;
		if(set->label==pth->arc->label)
		{
			wrd_conf = set->min_wrd_conf;
			break;
		}
	}
	return wrd_conf;
}

float wtk_wdec_get_post_conf(wtk_wdec_t *w,wtk_wdec_pth_t *pth)
{
	int i;
	wtk_wdec_words_set_t *set;
	float conf=-1;
	wtk_wdec_pth_t *p=pth;
	while(p)
	{
		if(p->arc && p->arc->label>0)
		{
			break;
		}
		p=p->prev;
	}
	//wtk_debug("label=%d\n",p->arc->label);
	for(i =0;i<w->net->cfg->n_words;++i)
	{
		set = w->net->cfg->set+i;
		if(set->label==p->arc->label)
		{
			conf = set->min_conf;
			//wtk_debug("xxxxxxx conf=%f\n",conf);
			break;
		}
	}
	return conf;
}

void wtk_wdec_add_path(wtk_wdec_t *w,wtk_wdec_tok_t *tok,wtk_wdec_arc_t *input_arc)
{
	wtk_wdec_pth_t *pth;

	//pth=(wtk_wdec_pth_t*)wtk_heap_malloc(w->heap,sizeof(wtk_wdec_pth_t));
	pth=(wtk_wdec_pth_t*)wtk_vpool_pop(w->pth_pool);
	pth->prev=tok->path;
	pth->dnn_like=tok->dnn_like;
	pth->like=tok->like;
	pth->frame=w->nframe;
	pth->arc=input_arc;
	tok->path=pth;
	//wtk_debug("%.*s=%f\n",pth->arc->v.hmm->name->len,pth->arc->v.hmm->name->data,tok->like);
	//exit(0);
}



int wtk_wdec_pth_depth(wtk_wdec_pth_t *pth)
{
	int n=0;

	//wtk_wdec_node_print_pre(pth->arc->nxt);
	while(pth)
	{
		if(pth->arc->nxt->word)
		{
			++n;
		}
		pth=pth->prev;
	}
	return n;
}

void wtk_wdec_process_tok(wtk_wdec_t *w,wtk_wdec_tok_t *tok,wtk_wdec_arc_t *input_arc,int sframe)
{
	wtk_queue_node_t *qn,*qn2;
	wtk_wdec_arc_t *arc;
	wtk_wdec_inst_t *inst,*inst2;
	float f;
	wtk_wdec_node_t *node;
	int depth;
	float *wrd_conf;
	float conf;

	if(input_arc)
	{
		//wtk_debug("======== next=%.*s ==========\n",input_arc->v.hmm->name->len,input_arc->v.hmm->name->data);
		node=input_arc->nxt;
	}else
	{
		node=w->net->phn_start;
	}
	//wtk_wdec_node_print_phn_nxt(node);
	if(input_arc)
	{
		if(w->cfg->add_path)
		{
			wtk_wdec_add_path(w,tok,input_arc);
		}
		if(input_arc->nxt->word)
		{
			if(1)
			{
				f=wtk_wdec_pth_get_wrd_conf(w,tok->path);
			}else
			{
				f=tok->like/(w->nframe-sframe);
			}
			//wtk_debug("%d,%.*s f=%f\n",input_arc->label,input_arc->nxt->word->name->len,input_arc->nxt->word->name->data,f);
			depth=wtk_wdec_pth_depth(tok->path);
			if(w->net->cfg->set)
			{
				wrd_conf=wtk_wdec_get_post_wrd_conf(w,tok->path);
				if(wrd_conf==NULL)
				{
					wrd_conf =w->post->min_wrd_conf;
				}
			}else
			{
				wrd_conf =w->post->min_wrd_conf;
			}
			//wtk_debug("wrd_conf=%f,%f,%f,%f\n",wrd_conf[0],wrd_conf[1],wrd_conf[2],wrd_conf[3]);
			//wtk_debug("depth=%d f=%f\n",depth,f);
			if(f<wrd_conf[depth-1])
			{
				return;
			}
		}		
	}
	if(node->output_q.len<=0)
	{
		//f=tok->like/(w->nframe-sframe);
		f=wtk_wdec_pth_get_conf(w,tok->path);
		//int min_conf;
		//wtk_debug("aaaaaaaaa %f\n",f);
		if(w->net->cfg->set)
		{
			conf=wtk_wdec_get_post_conf(w,tok->path);
			if(conf<0)
			{
				conf=w->post->min_conf;
			}
		}else
		{
			conf=w->post->min_conf;
		}
		//wtk_debug("v[%d]=%f %f\n",w->nframe,f,tok->like);
		if(f>=conf)
		{
			// wtk_debug("min_conf=%f\n",conf);
			//wtk_wdec_pth_print(tok->path);
			//f=wtk_wdec_pth_get_conf(w,tok->path);
			//wtk_debug("%f\n",f);
			//exit(0);
			w->found=1;
			w->conf=f;
			w->final_pth=tok->path;
			w->wake_sframe=sframe;
			w->wake_eframe=w->nframe;
		}
		//exit(0);
		return;
	}
	f=tok->like;
	//wtk_debug("node=%p\n",node);
	for(qn=node->output_q.pop;qn;qn=qn->next)
	{
		arc=data_offset2(qn,wtk_wdec_arc_t,output_n);
		//wtk_debug("v[%d]=%.*s\n",sframe,arc->v.hmm->name->len,arc->v.hmm->name->data);
		inst2=NULL;
		for(qn2=arc->inst_q.pop;qn2;qn2=qn2->next)
		{
			inst=data_offset2(qn2,wtk_wdec_inst_t,arc_n);
			//wtk_debug("sframe=%d/%d\n",inst->sframe,sframe);
			if(inst->sframe==sframe)
			{
				inst2=inst;
				break;
			}
		}
		if(!inst2)
		{
			inst2=wtk_wdec_pop_inst(w,sframe,arc);
			wtk_queue_push(&(w->inst_q),&(inst2->q_n));
		}
		if(f>inst2->state->like)
		{
			//wtk_debug("set like=%f\n",f);
			inst2->state->like=f;
			inst2->state->dnn_like=tok->dnn_like;
			inst2->state->path=tok->path;
		}
	}
	//wtk_debug("input=%d\n",w->inst_q.length);
}


void wtk_wdec_step_hmm(wtk_wdec_t *w,wtk_wdec_inst_t *inst)
{
	wtk_hmm_t *hmm=inst->arc->v.hmm;
	short **pp,*p;
	wtk_wdec_tok_t *cur,*res,*tgt,*end;
	int i,endi,j,n;
	float *pf;
	float score,f;
	wtk_dnn_state_t *s;
	float thresh,max_score;
	int nactive=0;
	int is_start;

	max_score=LZERO;
	thresh=-1e5;
	//wtk_debug("inst=%p hmm=[%.*s] frame=%d\n",inst,hmm->name->len,hmm->name->data,(inst->state+inst->arc->v.hmm->num_state-1)->path?(inst->state+inst->arc->v.hmm->num_state-1)->path->frame:0);
	n=hmm->num_state;
	pp=w->net->cfg->hmmset.hmmset->seIndexes[hmm->tIdx];
	res=w->bk_tok+1;
	is_start=inst->state->path?0:1;
	for(j=2;j<n;++j,++res)
	{
		p=pp[j];
		i=p[0];endi=p[1];
		pf=hmm->transP[j];
		cur=inst->state+i-1;
		tgt=cur;
		score=cur->like+pf[i];
		//wtk_debug("like[%d,%d]=%f pf=%f score=%f\n",ki,i,cur->like,pf[i],score);
		//print_float(pf+i,endi-i+1);
		for(++cur,++i;i<=endi;++i,++cur)
		{
			f=cur->like+pf[i];
			if(f>score)
			{
				tgt=cur;
				score=f;
			}
		}
		//wtk_debug("score[%d,%d]=%f/%f\n",w->nframe,j,score,thresh);
		if(score<thresh)
		{
			*res=null_tok;
			continue;
		}
		//wtk_debug("score=%f\n",score);
		s=hmm->pState[j]->dnn;
		score+=w->obs[s->index]+s->gconst;
		//wtk_debug("v[%d]=%f/%f/%f\n",w->nframe,tgt->like,score,w->obs[s->index]+s->gconst);
		f=score/(w->nframe-inst->sframe);
		//wtk_debug("score[%d,%d]=%f/%f score=%f/%f/%f\n",w->nframe,j,f,w->cfg->min_hmm_conf,score,w->obs[s->index],s->gconst);
		if(score<w->post->min_hmm_conf)
		{
			*res=null_tok;
			continue;
		}
		if(is_start)
		{
			if(f<w->emit_thresh)
			{
				*res=null_tok;
				continue;
			}
			wtk_prune_add(w->prune,f);
		}
		res->path=tgt->path;
		res->like=score;
		res->dnn_like=tgt->dnn_like+w->obs[s->index];
		//wtk_debug("like=-%f\n",score);
		//wtk_debug("like=%f\n",res->dnn_like);
//		if(res->path)
//		{
//			res->dnn_like=res->path->dnn_like+w->obs[s->index];
//		}else
//		{
//			res->dnn_like=w->obs[s->index];
//		}
		//exit(0);
		//wtk_debug("set score[%d,%d]=%f dnn=%f\n",w->nframe,j,score,score/(w->nframe-inst->sframe));
		++nactive;
		//wtk_debug("nactive=%d\n",nactive);
		if(score>max_score)
		{
			max_score=score;
		}
	}
	res=inst->state;
	inst->state=w->bk_tok;
	w->bk_tok=res;
	inst->state[0]=null_tok;
	inst->nactive=nactive;
	res=inst->state+n-1;
	if(max_score>LZERO)
	{
		p=pp[n];
		res=inst->state+n-1;
		cur=inst->state+p[0]-1;
		end=inst->state+p[1]-1;
		pf=hmm->transP[n]+p[0];
		score=cur->like+*(pf);
		//wtk_debug("========= 111111 %d/%d nactive=%d  ===========\n",p[0]-1,p[1]-1,inst->nactive);
		//wtk_debug("%f/%f/%f\n",cur->like,*pf,score);
		tgt=cur;
		for(++cur;cur<=end;++cur)
		{
			if((f=cur->like+*(++pf))>score)
			{
				//wtk_debug("%f/%f/%f\n",cur->like,*(pf-1),f);
				tgt=cur;
				score=f;
			}
		}
		if(score <= LZERO)
		{
			*res=null_tok;
		}else
		{
			res->like=score;
			//wtk_debug("set like=%f\n",res->like);
			res->path=tgt->path;
			++inst->nactive;
			res->dnn_like=tgt->dnn_like;
		}
	}else
	{
		*res=null_tok;
	}
	//wtk_debug("v[3]=%f state=%p\n",inst->state[2].like,inst->state+2);
	//wtk_debug("nactive=%d\n",inst->nactive);
}


void wtk_wdec_step_inst1(wtk_wdec_t *w)
{
	wtk_queue_t q=w->inst_q;
	wtk_queue_node_t *qn;
	wtk_wdec_inst_t *inst;

	qn=q.pop;
	wtk_queue_init(&(w->inst_q));
	while(qn)
	{
		inst=data_offset2(qn,wtk_wdec_inst_t,q_n);
//		wtk_debug("v[%d/%d]=%.*s like=%f/%f/%f/%f/%f\n",w->nframe,inst->sframe,inst->arc->v.hmm->name->len,inst->arc->v.hmm->name->data,inst->state->like,inst->state[1].like,inst->state[2].like,inst->state[3].like,inst->state[4].like);
//		if(inst->state->path)
//		{
//			wtk_wdec_pth_print(inst->state->path);
//		}
		wtk_wdec_step_hmm(w,inst);
		qn=qn->next;
		if(inst->nactive>0)
		{
			wtk_queue_push(&(w->inst_q),&(inst->q_n));
		}else
		{
			wtk_wdec_push_inst(w,inst);
		}
	}
}

void wtk_wdec_step_inst2(wtk_wdec_t *w)
{
	wtk_queue_t q=w->inst_q;
	wtk_queue_node_t *qn;
	wtk_wdec_inst_t *inst;
	wtk_wdec_tok_t *tok;
	float f;
	float conf=w->post->min_step_conf;

	//wtk_debug("inst=%d nframe=%d\n",w->inst_q.length,w->nframe);
	qn=q.pop;
	wtk_queue_init(&(w->inst_q));
	while(qn)
	{
		inst=data_offset2(qn,wtk_wdec_inst_t,q_n);
		tok=inst->state+inst->arc->v.hmm->num_state-1;
		f=tok->like/(w->nframe-inst->sframe);
		//wtk_wdec_node_print_phn_nxt(inst->arc->pre);
//		if(tok->path)
//		{
//			wtk_wdec_pth_print(tok->path);
//		}
		//wtk_debug("v[%d]=%f/%f\n",w->nframe,f,conf);
		if(f>=conf)
		{
			//wtk_debug("%f\n",tok->like/(w->nframe-inst->sframe));
			wtk_wdec_process_tok(w,tok,inst->arc,inst->sframe);
			if(w->found)
			{
				return;
			}
			--inst->nactive;
		}
		qn=qn->next;
		if(inst->nactive>0)
		{
			wtk_queue_push(&(w->inst_q),&(inst->q_n));
		}else
		{
			wtk_wdec_push_inst(w,inst);
		}
	}
	//wtk_debug("inst=%d\n",w->inst_q.length);
	//exit(0);
}

void wtk_wdec_print_inst(wtk_wdec_t *w)
{
	wtk_queue_node_t *qn;
	wtk_wdec_inst_t *inst;
	float f;

	for(qn=w->inst_q.pop;qn;qn=qn->next)
	{
		inst=data_offset2(qn,wtk_wdec_inst_t,q_n);
		f=inst->state[inst->arc->v.hmm->num_state-1].like/(w->nframe-inst->sframe);
		wtk_debug("inst[%d]=%f\n",inst->sframe,f);
		if(inst->state[inst->arc->v.hmm->num_state-1].path)
		{
			wtk_wdec_pth_print(inst->state[inst->arc->v.hmm->num_state-1].path);
		}
	}
}

void wtk_wdec_feed_feat(wtk_wdec_t *w,wtk_feat_t *feat)
{
	++w->nframe;
	//wtk_debug("======== nframe=%d inst=%d heap=%f M ============\n",w->nframe,w->inst_q.length,wtk_wdec_bytes(w)*1.0/(1024*1024));
	//wtk_debug("v[%d]: inst=%d v=%f\n",feat->index,w->inst_q.length,feat->rv[1]);
	w->obs=feat->rv;
	//wtk_vector_print(w->obs);
	//exit(0);
	wtk_wdec_step_inst1(w);
	if(w->inst_q.length>0)
	{
		wtk_wdec_step_inst2(w);
	}
	if(w->cfg->step>=0&&(w->cfg->step==1 || w->nframe%w->cfg->step==1))
	{
		wtk_wdec_tok_t tok2;

		//wtk_debug("nframe=%d\n",w->nframe);
		tok2.like=0;
		tok2.dnn_like=0;
		tok2.path=NULL;
		wtk_wdec_process_tok(w,&tok2,NULL,w->nframe);
	}
	w->emit_thresh=wtk_prune_get_thresh(w->prune);
	//wtk_debug("emit=%f\n",w->emit_thresh);
	wtk_prune_reset(w->prune);
//	if(w->nframe==230)
//	{
//		wtk_wdec_print_inst(w);
//		exit(0);
//	}
//	if(w->inst_q.length>300)
//	{
//		wtk_wdec_print_inst(w);
//		wtk_debug("thresh=%f\n",w->emit_thresh);
//		exit(0);
//	}
}

int wtk_wdec_feed(wtk_wdec_t *w,char *data,int len,int is_end)
{
	wtk_queue_t *q=&(w->feat_q);
	wtk_queue_node_t *qn;
	wtk_feat_t *feat;

	wtk_fextra_feed2(w->fextra,data,len,is_end);
	while(1)
	{
		qn=wtk_queue_pop(q);
		if(!qn){break;}
		feat=data_offset2(qn,wtk_feat_t,queue_n);
		//wtk_debug("index=%d skip=%d inst=%d\n",feat->index,w->fextra->dnn->cfg->skip_frame,w->inst_q.length);
		if(!w->found)
		{
			if(feat->app_hook)
			{
				wtk_wdec_feed_feat(w,(wtk_feat_t*)(feat->app_hook));
			}else
			{
				wtk_wdec_feed_feat(w,feat);
			}
		}
		if((w->cfg->fextra.dnn.skip_frame != 0 && feat->index %2 == 1))
		{
			--feat->used;
		}
		wtk_feat_push_back(feat);
//		if(w->found)
//		{
//			break;
//		}
	}
	//wtk_debug("conf=%f\n",w->post->min_conf);
	return w->found?1:0;
}

void wtk_wdec_print_conf(wtk_wdec_t *w,wtk_wdec_pth_t *pth)
{
	float f;

	while(pth)
	{
		if(pth->arc->nxt && pth->arc->nxt->word)
		{
			f=wtk_wdec_pth_get_wrd_conf(w,pth);
			wtk_debug("v[%.*s]=%f %d\n",pth->arc->nxt->word->name->len,pth->arc->nxt->word->name->data,f,pth->arc->label);
		}
		pth=pth->prev;
	}
}

float wtk_wdec_get_conf(wtk_wdec_t *w)
{
	if(w->final_pth)
	{
		return  wtk_wdec_pth_get_conf(w,w->final_pth);
	}else
	{
		return 0;
	}
}

void wtk_wdec_print(wtk_wdec_t *w)
{
	float f;

	if(w->final_pth)
	{
		wtk_wdec_print_conf(w,w->final_pth);
		wtk_wdec_pth_print(w->final_pth);
		f=wtk_wdec_pth_get_conf(w,w->final_pth);
		wtk_debug("========== conf=%f ======\n",f);
	}
}

int  wtk_wdec_get_final_time(wtk_wdec_t *w,float *fs,float *fe)
{
	float dur=w->fextra->cfg->frame_dur;
	wtk_wdec_pth_t *pth,*lst;

	if(w->found)
	{
		if(w->final_pth)
		{
			//skip start and end sil;
			//wtk_wdec_pth_print(w->final_pth);
			pth=w->final_pth;
			while(pth && pth->arc->v.hmm!=w->sil_hmm)
			{
				pth=pth->prev;
			}
			pth=pth->prev;
			*fe=pth->frame*dur;
			lst=NULL;
			while(pth && pth->arc->v.hmm!=w->sil_hmm)
			{
				//wtk_debug("pth=%p [%.*s]\n",pth,pth->arc->v.hmm->name->len,pth->arc->v.hmm->name->data);
				lst=pth;
				pth=pth->prev;
			}
			//wtk_debug("pth=%p\n",lst);
			*fs=lst->frame*dur;
			//wtk_debug("%.*s\n",pth->arc->v.hmm->name->len,pth->arc->v.hmm->name->data);
			//wtk_wdec_pth_print(pth);
			return 0;
		}else
		{
			*fs=w->wake_sframe*dur;
			*fe=w->wake_eframe*dur;
		}
		return 0;
	}else
	{
		*fs=0;
		*fe=0;
		return -1;
	}
}

void wtk_wdec_set_words(wtk_wdec_t *w, char *words,int n){
	wtk_wdec_net_cfg_set_words(&(w->cfg->net),words,n);
	wtk_wdec_net_delete(w->net);
	w->net = wtk_wdec_net_new(&(w->cfg->net));
}
int wtk_wdec_reset_check(wtk_wdec_t *w){
    int depth;
    wtk_queue_node_t *qn;
    wtk_wdec_inst_t *inst;
    // float f;

    for(qn=w->inst_q.pop;qn;qn=qn->next)
    {
        inst=data_offset2(qn,wtk_wdec_inst_t,q_n);
        // f=inst->state[inst->arc->v.hmm->num_state-1].like/(w->nframe-inst->sframe);
        if(inst->state[inst->arc->v.hmm->num_state-1].path)
        {
            depth = wtk_wdec_pth_depth(inst->state[inst->arc->v.hmm->num_state-1].path);
            if(depth >= 2){
                return 0;
            }
        }
    }
    // wtk_wdec_reset(w);
    //wtk_wdec_start(w,NULL);
    return 1;
}
