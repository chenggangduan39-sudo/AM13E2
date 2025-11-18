#include "qtk_kwfstdec_lite.h"

qtk_kwfstdec_lite_pth_t* qtk_kwfstdec_lite_new_pth(qtk_kwfstdec_lite_t* dec)
{
	qtk_kwfstdec_lite_pth_t *pth=(qtk_kwfstdec_lite_pth_t*)wtk_heap_malloc(dec->heap,sizeof(qtk_kwfstdec_lite_pth_t));

	pth->lbest=NULL;//token->pth;

	return pth;
}

qtk_kwfstdec_lite_token_t* qtk_kwfstdec_lite_new_token(qtk_kwfstdec_lite_t* dec,wtk_fst_state_t* state)
{
	qtk_kwfstdec_lite_token_t *token;

	token=(qtk_kwfstdec_lite_token_t*)wtk_vpool2_pop(dec->tok_pool);
	token->tot_cost=0.0;
	token->pth=NULL;
	token->state=state;

	return token;
}

void qtk_kwfstdec_lite_push_token(qtk_kwfstdec_lite_t* dec,qtk_kwfstdec_lite_token_t *tok)
{
	wtk_vpool2_push(dec->tok_pool,tok);
}

qtk_kwfstdec_lite_t* qtk_kwfstdec_lite_new(qtk_kwfstdec_cfg_t* cfg)
{
	qtk_kwfstdec_lite_t *kdec = (qtk_kwfstdec_lite_t*) wtk_malloc(sizeof(qtk_kwfstdec_lite_t));

	kdec->heap = wtk_heap_new(4096);
	kdec->tok_pool = wtk_vpool2_new(sizeof(qtk_kwfstdec_lite_token_t),10000);
	kdec->bins = (unsigned int*) wtk_malloc(sizeof(unsigned int) * 500);
	memset(kdec->bins, 0, sizeof(unsigned int) * 500);

	kdec->cur_tokq = wtk_queue_new();
	kdec->pre_tokq = wtk_queue_new();

	kdec->cfg = cfg;
	kdec->net = wtk_fst_net_new(&(cfg->net));
	kdec->cur_frame = 0;
	kdec->best_weight = FLT_MAX;
	kdec->best_token = NULL;

	kdec->conf = 0.0;

	return kdec;
}

qtk_kwfstdec_lite_t* qtk_kwfstdec_lite_new2(qtk_kwfstdec_cfg_t* cfg, int use_outnet)
{
	qtk_kwfstdec_lite_t *kdec = (qtk_kwfstdec_lite_t*) wtk_malloc(sizeof(qtk_kwfstdec_lite_t));

	kdec->heap = wtk_heap_new(4096);
	kdec->tok_pool = wtk_vpool2_new(sizeof(qtk_kwfstdec_lite_token_t),200);
	kdec->bins = (unsigned int*) wtk_malloc(sizeof(unsigned int) * 500);
	memset(kdec->bins, 0, sizeof(unsigned int) * 500);

	kdec->cur_tokq = wtk_queue_new();
	kdec->pre_tokq = wtk_queue_new();

	kdec->cfg = cfg;
    	if (use_outnet)
        	kdec->net = NULL;
    	else
        	kdec->net = wtk_fst_net_new(&(cfg->net));
	kdec->cur_frame = 0;
	kdec->best_weight = FLT_MAX;
	kdec->best_token = NULL;

	kdec->conf = 0.0;

	return kdec;
}

void qtk_kwfstdec_lite_reset(qtk_kwfstdec_lite_t* dec)
{
//	wtk_debug("---------reset--------\n");
	memset(dec->bins, 0, sizeof(unsigned int) * 500);

	wtk_heap_reset(dec->heap);
	wtk_vpool2_reset(dec->tok_pool);
	wtk_fst_net_reset(dec->net);
	//wtk_queue_init(&(dec->active_tok));
	//wtk_queue_init(&(dec->state_q));
	wtk_queue_init(dec->pre_tokq);
	wtk_queue_init(dec->cur_tokq);
	//wtk_queue_init(&(dec->final_cost_q_lite));
	//wtk_queue_init(&(dec->token_map_q));
	dec->cur_frame = 0;
	dec->best_token = NULL;
	dec->best_weight = FLT_MAX;

	dec->conf = 0.0;
	//dec->trans_model=dec->cfg->trans_model.trans_model;
}

void qtk_kwfstdec_lite_delete(qtk_kwfstdec_lite_t* dec)
{
	//wtk_debug("delete kwfstdec\n");
	wtk_free(dec->bins);
	wtk_queue_delete(dec->pre_tokq);
	wtk_queue_delete(dec->cur_tokq);
	wtk_heap_delete(dec->heap);
	wtk_vpool2_delete(dec->tok_pool);
	wtk_fst_net_delete(dec->net);
	//if(dec->trans_model)
	//{
	//	wtk_free(dec->trans_model);
	//}

	wtk_free(dec);
}

int qtk_kwfstdec_lite_process_none_emitting_state(qtk_kwfstdec_lite_t* dec, float cut_off);

int qtk_kwfstdec_lite_start(qtk_kwfstdec_lite_t* dec)
{
	int stateID = 0;
	qtk_kwfstdec_lite_token_t *start_tok;
	wtk_fst_state_t* state;

	state = wtk_fst_net_get_load_state(dec->net, stateID);
	state->frame = 0;
	dec->decoding_finalized = 0;
	start_tok = qtk_kwfstdec_lite_new_token(dec, state);
	state->hook = start_tok;
	wtk_queue_push(dec->cur_tokq,&(start_tok->q_n));
	qtk_kwfstdec_lite_process_none_emitting_state(dec, dec->cfg->beam);

	return 0;
}

qtk_kwfstdec_lite_token_t* qtk_kwfstdec_lite_find_or_add_token(qtk_kwfstdec_lite_t* dec,
		wtk_fst_trans_t *trans, float tot_cost, int *changed,
		qtk_kwfstdec_lite_token_t *pre_tok, float ac_cost)
{
//	wtk_debug("===============find or add token state:%d\n",state);
	qtk_kwfstdec_lite_token_t *token, *update_tok;
	//qtk_hash_elem_t *elem;
	//float extra_cost = 0.0;
	wtk_fst_state_t* state=trans->to_state;

	//tok_list = qtk_hash_list_new_token_list(token);
	//wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
	//elem=qtk_hash_list_find(dec->tok,state);
	//wtk_fst_net_load_state(dec->net, state);
	if(dec->net->nrbin_states <= 0)
	{
		wtk_fst_net_load_state(dec->net, state);
	}else
	{
		state = wtk_fst_net_get_load_state(dec->net,state->id);
	}

	if (state->frame == dec->cur_frame)
	{
		update_tok = (qtk_kwfstdec_lite_token_t*) state->hook;
	} else
	{
		update_tok = NULL;
	}

	if (!update_tok)
	{
		//wtk_debug("===============find or add token state not found\n");
		token = qtk_kwfstdec_lite_new_token(dec, state);
		token->pth = qtk_kwfstdec_lite_new_pth(dec);
		token->pth->lbest = pre_tok->pth;
		token->pth->out_label = trans->out_id;
		token->pth->in_label = trans->in_id;
		token->pth->like = ac_cost;
		token->tot_cost = tot_cost;
		wtk_queue_push(dec->cur_tokq,&(token->q_n));
		if (changed)
			*changed = 0;
		//tok_list = qtk_hash_list_new_token_list(token);
		//wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
		state->frame = dec->cur_frame;
		state->hook = token;
	} else
	{
		//wtk_debug("===============find or add token state found\n");
		token = update_tok;
		if (changed)
						*changed = 1;
		if (token->tot_cost > tot_cost)
		{
			token->tot_cost = tot_cost;
			token->pth->lbest = pre_tok->pth;
			token->pth->out_label = trans->out_id;
			token->pth->in_label = trans->in_id;
			token->pth->like = ac_cost;
			token->state = state;
			wtk_queue_remove(dec->cur_tokq,&(token->q_n));
			wtk_queue_push(dec->cur_tokq,&(token->q_n));
			//wtk_debug("%p %p\n",token->pth->lbest,token->pth);
		}
	}

	return token;
}

float qtk_kwfstdec_lite_get_cutoff2(qtk_kwfstdec_lite_t* dec,
		float *adaptive_beam, qtk_kwfstdec_lite_token_t **best_elem)
{
	//wtk_debug("=============get cut off\n");
	memset(dec->bins, 0, sizeof(unsigned int) * 500);
	float best_weight = FLT_MAX, w = 0.0, beam_cutoff = FLT_MAX,
			min_active_cutoff = FLT_MAX, max_active_cutoff = FLT_MAX;
	int count = 0, idx;
	qtk_kwfstdec_lite_token_t *e = NULL;
	wtk_queue_node_t *qn;

	count = dec->cur_tokq->length;
	if (dec->cfg->max_active == UINT_MAX && dec->cfg->min_active == 0)
	{
		for (qn = dec->cur_tokq->pop; qn; qn = qn->next)
		{
			e = data_offset2(qn,qtk_kwfstdec_lite_token_t,q_n);
			if (e->state->id == 0)
				continue;
//			wtk_debug("=============get cut off searching elem state:%d cost:%f\n",e->key,e->token->tot_cost);
			w = e->tot_cost;
			if (w < best_weight)
			{
				best_weight = w;
				if (best_elem)
					*best_elem = e;
			}
		}
//		wtk_debug("=============get cut off elem count:%d\n",count);
		if (adaptive_beam != NULL)
			*adaptive_beam = dec->cfg->beam;
		return best_weight + dec->cfg->beam;
	} else
	{
		for (qn = dec->cur_tokq->pop; qn; qn = qn->next)
		{
			e = data_offset2(qn,qtk_kwfstdec_lite_token_t,q_n);
			if (e->state->id == 0)
				continue;
			//wtk_debug("=============get cut off searching elem state:%d cost:%f\n",e->key,e->token->tot_cost);
			w = e->tot_cost;
			float ww = 0.0;
			ww = w * 10.0 + 200;
			if (ww < 0)
			{
				idx = 0;
			} else if (ww >= 500)
			{
				idx = 499;
			} else
			{
				idx = (int) (ww);
			}

			*(dec->bins + idx) += 1;
			if (w < best_weight)
			{
				best_weight = w;
				if (best_elem)
					*best_elem = e;
			}
		}
		//wtk_debug("=============get cut off elem count:%d\n",count);
		int k, cnt = 0;
		if (count > dec->cfg->max_active)
		{
			for (k = 0; k < 500; k++)
			{
				cnt += dec->bins[k];
				//wtk_debug("%d %d %f\n",k,dec->bins[k],((k-200)*1.0)/10);
				if (cnt > dec->cfg->max_active)
				{
					max_active_cutoff = ((k - 199) * 1.0) / 10;
					break;
				}
			}
		}
		beam_cutoff = best_weight + dec->cfg->beam;
		//wtk_debug("=============get cut off max_active_cutoff:%f beam_cutoff:%f\n",max_active_cutoff,beam_cutoff);
		if (max_active_cutoff < beam_cutoff)
		{
			if (adaptive_beam)
				*adaptive_beam = max_active_cutoff - best_weight
						+ dec->cfg->beam_delta;
			//wtk_debug("return max_active_cutoff\n");
			return max_active_cutoff;
		}
		if (count > dec->cfg->min_active)
		{
			if (dec->cfg->min_active == 0)
			{
				min_active_cutoff = best_weight;
			} else {
				//qn=wtk_queue_peek(&(dec->tmp_q),dec->cfg->min_active);
				//t=(tmp_q_t*)data_offset(qn,tmp_q_t,q_n);
				for (k = 0; k < 500; k++)
				{
					cnt += dec->bins[k];
					//wtk_debug("%d\n",cnt);
					if (cnt > dec->cfg->min_active)
					{
						//wtk_debug("%d %d %f\n",k,dec->bins[k],((k-200)*1.0)/10);
						min_active_cutoff = ((k - 199) * 1.0) / 10;
						break;
					}
				}
			}
		}
		//wtk_debug("=============get cut off min_active_cutoff:%f beam_cutoff:%f\n",min_active_cutoff,beam_cutoff);
		if (min_active_cutoff > beam_cutoff)
		{
			if (adaptive_beam)
				*adaptive_beam = min_active_cutoff - best_weight
						+ dec->cfg->beam_delta;
			//wtk_debug("return min_active_cutoff\n");
			return min_active_cutoff;
		} else
		{
			*adaptive_beam = dec->cfg->beam;
			//wtk_debug("return beam cutoff\n");
			return beam_cutoff;
		}
	}

	return 0;
}

float qtk_kwfstdec_lite_process_emitting_state(qtk_kwfstdec_lite_t* dec,wtk_vector_t *obs)
{
	//wtk_debug("=====================process emitting state %d\n",frame);
	qtk_kwfstdec_lite_token_t* best_elem = NULL;//, *elem, *e_tail;
	int dnn_cnt, i;
	float adaptive_beam = FLT_MAX;
	float cur_cutoff;

	cur_cutoff = qtk_kwfstdec_lite_get_cutoff2(dec,
			&adaptive_beam, &best_elem);
	//wtk_debug("=====================process emitting: cur_cutoff:%f adaptive_beam:%f\n",cur_cutoff,adaptive_beam);
	float next_cutoff = FLT_MAX;
	float tot_cost = FLT_MAX;
	float cost_offset = 0.0;
	float new_weight = FLT_MAX, likelihood, ac_cost, graph_cost, cur_cost;
	float lmscale = dec->cfg->lm_scale;
	wtk_fst_state_t *state;
	wtk_fst_trans_t *trans;
	qtk_kwfstdec_lite_token_t *token = NULL;
	wtk_queue_t *tmp_q;
	wtk_queue_node_t *qn;

	if (best_elem)
	{
		//wtk_debug("=====================process emitting has best element: %d\n",best_elem->key);
		cost_offset = -best_elem->tot_cost;
		state = best_elem->state;//wtk_fst_net_get_load_state(dec->net,state_id);
		for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans)
		{
			if (trans->in_id != 0)
			{
				dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);
				//likelihood=(*(obs+dnn_cnt+1));///10;
				likelihood = (*(obs + dnn_cnt));		///10;
				//wtk_debug("mahaha %f %d\n",likelihood,trans->in_id);
				//new_weight=tok->tot_cost-trans->weight+likelihood;
				new_weight = -trans->weight - likelihood;
				if (new_weight + adaptive_beam < next_cutoff)
					next_cutoff = new_weight + adaptive_beam;
			}
		}
	}
	tmp_q = dec->pre_tokq;
	dec->pre_tokq = dec->cur_tokq;
	dec->cur_tokq = tmp_q;
	//wtk_debug("process emitting next cutoff:%f\n",cost_offset);
	//wtk_debug("process element1\n");
	while (1)
	{
		qn=wtk_queue_pop(dec->pre_tokq);
		if(!qn){break;}

		//wtk_debug("process element state:%d %p %p \n",elem->key,elem->token,elem);
		token = data_offset(qn,qtk_kwfstdec_lite_token_t,q_n);//elem->token;
		state = token->state;	//wtk_fst_net_get_load_state(dec->net,state_id);
		//wtk_debug("token tot_cost:%f cur_cutoff:%f\n",token->tot_cost,cur_cutoff);
		if (token->tot_cost <= cur_cutoff)
		{
			for (i = 0, trans = state->v.trans; i < state->ntrans;++i, ++trans)
			{
				//	if(!trans)
				//		continue;

				if (trans->in_id != 0)
				{
					//wtk_debug("=======process emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
					dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);
					likelihood = (*(obs + dnn_cnt));					///10;
					//likelihood=(*(obs+dnn_cnt+1));///10;
					//wtk_debug("%d likehood:%f\n",dnn_cnt+1,likelihood);
					//wtk_debug("%d likehood:%f\n",trans->in_id,likelihood);
					ac_cost = cost_offset - likelihood;
					graph_cost = -trans->weight;
					cur_cost = token->tot_cost;
					
					tot_cost = cur_cost + ac_cost + graph_cost*lmscale;

					//wtk_debug("cost_offset:%f likehood:%f adaptive_beam:%f\n",cost_offset,likelihood,adaptive_beam);
					//wtk_debug("ac_cost:%f  graph_cost:%f  cur_cost:%f\n",ac_cost,graph_cost,cur_cost);
					//wtk_debug("tot_cost:%f  next_cutoff:%f\n",tot_cost,next_cutoff);
					if (tot_cost > next_cutoff)
						continue;	//{wtk_debug("continue prune\n");continue;}
					else if (tot_cost + adaptive_beam < next_cutoff)
						next_cutoff = tot_cost + adaptive_beam;
//					wtk_debug("search or add state:%d\n",trans->to_state->id)
					qtk_kwfstdec_lite_find_or_add_token(dec,
							trans, tot_cost, NULL, token,likelihood);
				}
			}
		}
		qtk_kwfstdec_lite_push_token(dec,token);
	}
	return next_cutoff;
}

int qtk_kwfstdec_lite_process_none_emitting_state(qtk_kwfstdec_lite_t* dec, float cutoff)
{
	//wtk_debug("============process none emitting state %f\n",cutoff);
	//qtk_hash_elem_t *elem;
	//st_q_lite_t *st, *st1;
	qtk_kwfstdec_lite_token_t *token;
	wtk_queue_node_t *qn;
	wtk_fst_state_t *state;					//need free?
	wtk_fst_trans_t *trans;					//need free?
	float tot_cost = FLT_MAX;
	float cur_cost = FLT_MAX;
	float graph_cost;
	float lmscale = dec->cfg->lm_scale;
	int changed = 0, i;

	if (dec->cur_tokq->length == 0)
	{
		wtk_debug("warning!!!!!!!!!\n");
	}

	for (qn = dec->cur_tokq->pop; qn;)
	{
		//wtk_debug("%p\n",qn);
		token = data_offset2(qn,qtk_kwfstdec_lite_token_t,q_n);
		state = token->state;

	    qn = qn->next;
		cur_cost = token->tot_cost;
		//wtk_debug("============process none emitting state:%d cur_cost:%f cutoff:%f\n",st->state_id,cur_cost,cutoff);
		if (cur_cost > cutoff)
			continue;
		for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans)
		{
			//wtk_debug("%p\n",trans);
			if (trans->in_id == 0)
			{
//				wtk_debug("============process none emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
//		wtk_debug("%d %p\n",st->state_id,trans->to_state);
				graph_cost = -(trans->weight);
				tot_cost = cur_cost + graph_cost*lmscale;
				//			wtk_debug("============process none emitting state:%d to:%d ilabel:%d olabel:%d weight:%f tot_cost:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight,tot_cost);
				//wtk_debug("============process none emitting cur_cost:%f graph_cost:%f\n",cur_cost,graph_cost);
				//wtk_debug("tot_cost:%f cutoff:%f\n",tot_cost,cutoff);
				if (tot_cost < cutoff)
				{
					qtk_kwfstdec_lite_find_or_add_token(dec,
							trans, tot_cost, &changed,
							token, 0);
				}
			}
		}
	}

	return 0;
}

int lite_isFiniteNumber(double d)
{
	return (d < FLT_MAX && d > -FLT_MAX);
}

void qtk_kwfstdec_lite_compute_final_cost(qtk_kwfstdec_lite_t* dec)
{
	//qtk_hash_elem_t *elem, *next;
	qtk_kwfstdec_lite_token_t *token;
	wtk_queue_node_t *qn;
	//final_cost_q_lite_t *fc;
	float best_cost_with_final = FLT_MAX;
	float best_cost = FLT_MAX;
	float final_cost, cost, cost_with_final;
	wtk_fst_state_t *state;
	dec->best_weight = FLT_MAX;
//	wtk_debug("compute final cost\n");
	//elem = qtk_hash_list_clear(dec->tok);
	for (qn = dec->cur_tokq->pop; qn; qn = qn->next)
	{
		token = data_offset2(qn,qtk_kwfstdec_lite_token_t,q_n);
		//token = elem->token;
		//next = elem->tail;
		state = token->state;//wtk_fst_net_get_load_state(dec->net,elem->key);
		if (state->type == WTK_FST_FINAL_STATE)
		{
			final_cost = state->weight;
		} else
		{
			final_cost = FLT_MAX;
		}
//		wtk_debug("%d: %f\n",state->id,final_cost);
		//final_cost=    TODO get single weight!
		cost = token->tot_cost;
		cost_with_final = cost + final_cost;
		best_cost = min(cost, best_cost);
		best_cost_with_final = min(cost_with_final, best_cost_with_final);
//		wtk_debug("%f\n",best_cost_with_final);
		if (lite_isFiniteNumber(final_cost))
		{
			cost += final_cost;
			if (cost < dec->best_weight)
			{
				dec->best_weight = cost;
				dec->best_token = token;
				//dec->best_final_cost = final_cost;
			}
//			wtk_debug("%p state->id:%d push best:%f\n",token,state->id,final_cost);
		}
		//elem = next;
	}
}


//int ccc=1;
int qtk_kwfstdec_lite_feed(qtk_kwfstdec_lite_t *dec, wtk_feat_t *f)
{
	float cost_cutoff;
//	wtk_debug("==========feed frame %d=============\n",ccc);
//	ccc++;
	wtk_vector_print(f->rv);
	dec->cur_frame = f->index;
//	if(index%dec->cfg->prune_interval==0)
//		qtk_kwfstdec_lite_prune_active_tokens(dec,dec->cfg->beam*dec->cfg->prune_scale);
	cost_cutoff = qtk_kwfstdec_lite_process_emitting_state(dec, f->rv);
	qtk_kwfstdec_lite_process_none_emitting_state(dec, cost_cutoff);
//	wtk_feat_push_back(f);
	return 0;
}

void qtk_kwfstdec_lite_log(float *p, int n)
{
	float *pe;

	pe = p + n;
	while (p < pe)
	{
		*p = log(*p);
		++p;
	}
}

int qtk_kwfstdec_lite_feed2(qtk_kwfstdec_lite_t* dec, wtk_vector_t *v, int index)
{
	float *v2;
	v2 = v;

	float cost_cutoff;
	//wtk_debug("==========feed frame %d=============\n", index);
	dec->cur_frame = index;
	cost_cutoff = qtk_kwfstdec_lite_process_emitting_state(dec, v2);
	qtk_kwfstdec_lite_process_none_emitting_state(dec, cost_cutoff);

	return 0;
}

void qtk_kwfstdec_lite_get_result(qtk_kwfstdec_lite_t* dec, wtk_strbuf_t *buf)
{
	//wtk_debug("=====================get rec result \n");

	if (dec->cur_frame == 0)
	{
		return;
	}

	wtk_fst_sym_t *sym_out;
	qtk_kwfstdec_lite_token_t  *best_token = NULL;
	qtk_kwfstdec_lite_pth_t* pth;
	wtk_string_t *v;

	sym_out = dec->net->cfg->sym_out;
	qtk_kwfstdec_lite_compute_final_cost(dec);
	//qtk_kwfstdec_lite_best_path_end(dec);
	best_token = dec->best_token;
	short cnt=0;
	float best=0.0;
	if (best_token)
	{
        if (best_token->pth != NULL)
        {
            for (pth = best_token->pth; pth;)
            {
            	//wtk_debug("%p %p\n",pth,pth->lbest);
            	//exit(0);
//				wtk_debug("=====================get rec result process best\n");
				if (pth->out_label != 0)
				{
  					 if(pth->out_label > dec->cfg->remove_label)
					{
						//wtk_debug("%d %f\n",pth->out_label,best_token->tot_cost);
						if(dec->net->print)
						{
							v=dec->net->print->get_outsym(dec->net->print->ths,pth->out_label);
						}else
						{
							v = sym_out->strs[pth->out_label];
						}
						if (buf->pos > 0)
						{
							wtk_strbuf_push_front(buf, " ", 1);
						}
						wtk_strbuf_push_front(buf, v->data, v->len);
						if(pth->in_label !=1 && pth->in_label !=2 )
						{
							best+=pth->like;
							cnt++;
						}
					}
				}else
				{
					if(pth->in_label > 2 )
					{
						best+=pth->like;
						cnt++;
					}
				}
				//if (pth->lbest)
				//{
				pth = pth->lbest;
				//}
				//wtk_debug("%d\n",cnt);
            }
        }
        //wtk_debug("%f %d\n",best,cnt);
        if(cnt < 3)
        {
            dec->conf = 0.0;
        }else
        {
            dec->conf = best/cnt;
        }
	}
//	printf(" %.*s\n",buf->pos,buf->data);
//	printf(" %.*s\n",buf->pos,buf->data);*/
}
