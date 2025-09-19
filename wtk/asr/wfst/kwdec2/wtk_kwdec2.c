#include "wtk_kwdec2.h"
void wtk_kwdec2_on_feat(wtk_kwdec2_t *dec,wtk_kfeat_t *feat);
void wtk_kwdec2_on_nnet3(wtk_kwdec2_t *dec, qtk_blas_matrix_t *f1, int end,
                         int plus);

wtk_kwdec2_pth_t* wtk_kwdec2_new_pth(wtk_kwdec2_t* dec)
{
	wtk_kwdec2_pth_t *pth=(wtk_kwdec2_pth_t*)wtk_heap_malloc(dec->heap,sizeof(wtk_kwdec2_pth_t));

	pth->lbest=NULL;//token->pth;

	return pth;
}

wtk_kwdec2_token_t* wtk_kwdec2_new_token(wtk_kwdec2_t* dec,wtk_fst_state_t* state)
{
	wtk_kwdec2_token_t *token;

	token=(wtk_kwdec2_token_t*)wtk_vpool2_pop(dec->tok_pool);
	token->tot_cost=0.0;
	token->pth=NULL;
	token->state=state;

	return token;
}

void wtk_kwdec2_push_token(wtk_kwdec2_t* dec,wtk_kwdec2_token_t *tok)
{
	if(tok == dec->best_token){
		dec->flag = 0;
	}
	wtk_vpool2_push(dec->tok_pool,tok);
}

int wtk_kwdec2_active_words_check(wtk_kwdec2_cfg_t *cfg,wtk_string_t *s)
{
	int ret =-1;
	int i;
	wtk_kwdec2_words_set_t *set;
	for(i=0;i<cfg->n_networds;++i)
	{
		set = cfg->set+i;
		// wtk_debug("%s,%.*s\n",set->word,s->len,s->data);
		if(!strncmp(set->word,s->data,s->len))
		{
			ret=i;
			break;
		}
	}
	return ret;
}

wtk_robin_t* wtk_kwdec2_new_robin(int n,int nvec)
{
        wtk_robin_t *rb;
        int i;

        rb=wtk_robin_new(n);
        for(i=0;i<n;++i)
        {
                rb->r[i]=(float*)wtk_calloc(nvec,sizeof(float));
        }
        return rb;
}

void wtk_kwdec2_delete_robin(wtk_robin_t *rb)
{
        int i;

        for(i=0;i<rb->nslot;++i)
        {
                wtk_free(rb->r[i]);
        }
        wtk_robin_delete(rb);
}

void wtk_kwdec2_decwords_set(wtk_kwdec2_t* dec)
{
	int i,n;
	wtk_string_t **s;
	s=(wtk_string_t**)(dec->cfg->words->slot);
	wtk_kwdec2_words_set_t *set;
	dec->words_set = (wtk_kwdec2_words_t**)wtk_malloc(dec->cfg->words->nslot*sizeof(wtk_kwdec2_words_t*));
	for(i=0;i<dec->cfg->words->nslot;++i)
	{
		// wtk_debug("n=%d,\n",dec->cfg->words->nslot);
		if((n=wtk_kwdec2_active_words_check(dec->cfg,s[i]))>=0)
		{
			//wtk_debug("n=%d\n",n);
			set = dec->cfg->set+n;
			dec->words_set[i]=(wtk_kwdec2_words_t*)wtk_malloc(sizeof(wtk_kwdec2_words_t));
			dec->words_set[i]->word = set->word;
			dec->words_set[i]->key_id = set->key_id;
			dec->words_set[i]->pdf_conf = set->pdf_conf;
			dec->words_set[i]->pdf_conf2 = set->pdf_conf2;
			dec->words_set[i]->min_kws = set->min_kws;
		}
		else
		{
			wtk_debug("word is not in set!\n");
			// exit(0);
		}	
	}
}

wtk_kwdec2_t* wtk_kwdec2_new(wtk_kwdec2_cfg_t* cfg)
{
	wtk_kwdec2_t *dec = (wtk_kwdec2_t*) wtk_malloc(sizeof(wtk_kwdec2_t));

	dec->heap = wtk_heap_new(4096);
	dec->tok_pool = wtk_vpool2_new(sizeof(wtk_kwdec2_token_t),1000);
	dec->bins = (unsigned int*) wtk_malloc(sizeof(unsigned int) * 500);
	memset(dec->bins, 0, sizeof(unsigned int) * 500);

	//wtk_queue_init(&(dec->active_tok));
	dec->cur_tokq = wtk_queue_new();
	dec->pre_tokq = wtk_queue_new();
	//wtk_queue_init(&(dec->state_q));
	//wtk_queue_init(&(dec->final_cost_q));
	//wtk_queue_init(&(dec->token_map_q));
	dec->mode_frame_cnt = 0;
	dec->switch_frame_cnt = 10000;
	dec->cfg = cfg;
	dec->trans_model=cfg->mdl.trans_model;
	dec->net = wtk_fst_net_new(&(cfg->net));
	dec->cur_frame = 0;
	dec->best_weight = FLT_MAX;
	dec->best_token = NULL;
	dec->parm=wtk_kxparm_new(&(cfg->parm));

	if (dec->cfg->use_fixpoint) {
		dec->shift =
			dec->parm->knn->rte->layers[dec->parm->knn->rte->nlayer - 1]
				->layer->shift;
		// wtk_debug("shift=%d\n",dec->shift);
	}

	if (dec->parm->cfg->use_knn) {
		wtk_kxparm_set_notify(dec->parm, dec,
								(wtk_kxparm_notify_f)wtk_kwdec2_on_feat);
	} else {
		qtk_nnet3_set_notify(
			dec->parm->nnet3,
			(qtk_nnet3_feature_notify_f)wtk_kwdec2_on_nnet3, dec);
	}
	dec->egram = NULL;
	dec->ebnf_buf = NULL;
	if (cfg->use_egram) {
		//dec->egram = wtk_egram_new(&(cfg->egram), cfg->rbin);
		dec->ebnf_buf = wtk_strbuf_new(128, 1);
	}

	dec->conf = 0.0;
	// int i;
	// int n=-1;
	dec->found = 0;
	dec->index = 0;
	dec->wake_beg_idx = 0;
	dec->wake_end_idx = 0;
	dec->wake_flag = 0;
	// dec->feat_pool=NULL;
	// dec->feat_pool =
	// wtk_vpool_new(sizeof(wtk_vector_t)*3045,dec->cfg->wdec_post_win_size);
	dec->reset = 0;
	dec->reset_frame = 0;
	// dec->out_col = 0;
	dec->words_set = NULL;
	dec->key_id = -1;
	dec->break_flag=0;
	dec->min_kws = dec->cfg->min_kws;
	dec->pdf_conf = dec->cfg->pdf_conf;
	dec->idle = 1;
	if (dec->parm->cfg->use_knn) {
		dec->out_col = dec->parm->knn->cfg->output_dim;
	} else {
		dec->out_col = 759; // 3045;//TODO
	}
	dec->trick_pdf_conf = -29.0;
	dec->trick_min_kws = 1;
    wtk_kwdec2_decwords_set(dec);
	dec->feat_rb=wtk_kwdec2_new_robin(dec->cfg->reset_time,dec->out_col);
    wtk_kwdec2_reset(dec);
	
	return dec;
}

void wtk_kwdec2_switch_mode(wtk_kwdec2_t* dec,int idle){
	dec->idle = idle;
	if(idle == 1){
	 	dec->min_kws = dec->cfg->min_kws;
	// 	dec->pdf_conf = dec->cfg->pdf_conf;
	 }else{
	 	dec->min_kws = dec->trick_min_kws;
	// 	dec->pdf_conf = dec->trick_pdf_conf;
	}
}

void wtk_kwdec2_reset(wtk_kwdec2_t* dec)
{
//	wtk_debug("---------reset--------\n");
	memset(dec->bins, 0, sizeof(unsigned int) * 500);

	wtk_heap_reset(dec->heap);
	wtk_vpool2_reset(dec->tok_pool);
	wtk_fst_net_reset(dec->net);
	wtk_kxparm_reset(dec->parm);
	if (dec->egram) {
		wtk_egram_reset(dec->egram);
	}
        // wtk_queue_init(&(dec->active_tok));
        // wtk_queue_init(&(dec->state_q));
	wtk_queue_init(dec->pre_tokq);
	wtk_queue_init(dec->cur_tokq);
	//wtk_queue_init(&(dec->final_cost_q));
	//wtk_queue_init(&(dec->token_map_q));
	dec->cur_frame = 0;
	dec->flag = 1;
	dec->index = 0;
	dec->best_token = NULL;
	dec->best_weight = FLT_MAX;

	dec->conf = 0.0;
	//dec->trans_model=dec->cfg->trans_model.trans_model;
    dec->found=0;
    dec->wake_beg_idx=0;
    dec->wake_end_idx=0;
    dec->wake_flag=0;
    dec->reset = 0;
    dec->reset_frame=0;
    //dec->out_col =0;
    dec->key_id=-1;
   //dec->min_kws = 0; // dec->cfg->min_kws;
   //dec->pdf_conf=dec->cfg->pdf_conf;
	dec->break_flag=0;
	// dec->index=0;
    //wtk_vpool_reset(dec->feat_pool);
    wtk_robin_reset(dec->feat_rb);

}

void wtk_kwdec2_delete(wtk_kwdec2_t* dec)
{
	//wtk_debug("delete kwfstdec\n");
	int i;
	wtk_free(dec->bins);
	wtk_queue_delete(dec->pre_tokq);
	wtk_queue_delete(dec->cur_tokq);
	wtk_heap_delete(dec->heap);
	wtk_vpool2_delete(dec->tok_pool);
	wtk_fst_net_delete(dec->net);
	wtk_kxparm_delete(dec->parm);
	if (dec->egram) {
		wtk_egram_delete(dec->egram);
	}
	if (dec->ebnf_buf) {
		wtk_strbuf_delete(dec->ebnf_buf);
	}
	/*if(dec->trans_model)
	{
			wtk_free(dec->trans_model);
	}*/
	// wtk_vpool_delete(dec->feat_pool);
	wtk_kwdec2_delete_robin(dec->feat_rb);
	if(dec->words_set)
	{
		for(i=0;i<dec->cfg->words->nslot;++i)
		{
			wtk_free(dec->words_set[i]);
		}
		wtk_free(dec->words_set);
	}
	wtk_free(dec);
}

int wtk_kwdec2_process_none_emitting_state(wtk_kwdec2_t* dec, float cut_off);

int wtk_kwdec2_start(wtk_kwdec2_t* dec)
{
	int stateID = 0;
	wtk_kwdec2_token_t *start_tok;
	wtk_fst_state_t* state;
	//wtk_fst_trans_t *trans;
	state = wtk_fst_net_get_load_state(dec->net, stateID);
	state->frame = 0;
	start_tok = wtk_kwdec2_new_token(dec, state);
	state->hook = start_tok;
	// int xx=0,i;
	// for(xx=0;xx<1242;xx++){
	// 	state = wtk_fst_net_get_load_state(dec->net, xx);
	// 	for (i = 0, trans = state->v.trans; i < state->ntrans;++i, ++trans){
	// 		printf("%d %d %d %d %f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
	// 	}
	// 	if(state->ntrans == 0 || state->type == WTK_FST_FINAL_STATE){
	// 		printf("%d\n",state->id);
	// 	}
	// }

	wtk_queue_push(dec->cur_tokq,&(start_tok->q_n));
	wtk_kwdec2_process_none_emitting_state(dec, dec->cfg->beam);

	return 0;
}

void wtk_kwdec2_find_or_add_token(wtk_kwdec2_t* dec,
		wtk_fst_trans_t *trans, float tot_cost, int *changed,
		wtk_kwdec2_token_t *pre_tok, float ac_cost)
{
//	wtk_debug("===============find or add token state:%d\n",state);
	wtk_kwdec2_token_t *token, *update_tok;
	//qtk_hash_elem_t *elem;
	//float extra_cost = 0.0;
	wtk_fst_state_t* state=trans->to_state;

        // tok_list = qtk_hash_list_new_token_list(token);
        // wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
        // elem=qtk_hash_list_find(dec->tok,state);
        // wtk_fst_net_load_state(dec->net, state);
        state = wtk_fst_net_get_load_state(dec->net, state->id);
        if (state->frame == dec->cur_frame) {
            update_tok = (wtk_kwdec2_token_t *)state->hook;
        } else {
            update_tok = NULL;
        }

        if (!update_tok)
	{
//		wtk_debug("===============find or add token state not found\n");
		token = wtk_kwdec2_new_token(dec, state);
		token->pth = wtk_kwdec2_new_pth(dec);
		token->pth->lbest = pre_tok->pth;
		token->pth->out_label = trans->out_id;
		//wtk_debug("out_label=%d\n",trans->out_id);
		token->pth->in_label = trans->in_id;
		token->pth->frame = dec->cur_frame;
		token->pth->like = ac_cost;
		token->tot_cost = tot_cost;
		wtk_queue_push(dec->cur_tokq,&(token->q_n));
		if(dec->best_token == token){
			dec->flag = 1;
		}
		if (changed)
			*changed = 0;
		//tok_list = qtk_hash_list_new_token_list(token);
		//wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
		state->frame = dec->cur_frame;
		state->hook = token;
	} else
	{
//		wtk_debug("===============find or add token state found\n");
		token = update_tok;
		if (changed)
						*changed = 1;
		if (token->tot_cost > tot_cost)
		{
			token->tot_cost = tot_cost;
			token->pth->lbest = pre_tok->pth;
			token->pth->out_label = trans->out_id;
			token->pth->in_label = trans->in_id;
			token->pth->frame = dec->cur_frame;
			token->pth->like = ac_cost;
			token->state = state;
			wtk_queue_remove(dec->cur_tokq,&(token->q_n));
			wtk_queue_push(dec->cur_tokq,&(token->q_n));
			if(dec->best_token == token){
				dec->flag = 1;
			}
			//wtk_debug("%p %p\n",token->pth->lbest,token->pth);
		}
	}

	//return token;
}

float wtk_kwdec2_get_cutoff2(wtk_kwdec2_t* dec,
		float *adaptive_beam, wtk_kwdec2_token_t **best_elem)
{
	//wtk_debug("=============get cut off\n");
	memset(dec->bins, 0, sizeof(unsigned int) * 500);
	float best_weight = FLT_MAX, w = 0.0, beam_cutoff = FLT_MAX,
			min_active_cutoff = FLT_MAX, max_active_cutoff = FLT_MAX;
	int count = 0, idx;
	wtk_kwdec2_token_t *e = NULL;
	wtk_queue_node_t *qn;

	count = dec->cur_tokq->length;
	if (dec->cfg->max_active == UINT_MAX && dec->cfg->min_active == 0)
	{
		for (qn = dec->cur_tokq->pop; qn; qn = qn->next)
		{
			e = data_offset2(qn,wtk_kwdec2_token_t,q_n);
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
			e = data_offset2(qn,wtk_kwdec2_token_t,q_n);
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
//		wtk_debug("=============get cut off min_active_cutoff:%f beam_cutoff:%f\n",min_active_cutoff,beam_cutoff);
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

float wtk_kwdec2_process_emitting_state(wtk_kwdec2_t* dec,wtk_vector_t *obs)
{
	//wtk_debug("=====================process emitting state\n");
	//wtk_debug("=====================process emitting state %d\n",frame);
	wtk_kwdec2_token_t* best_elem = NULL;//, *elem, *e_tail;
	int dnn_cnt, i;
	float adaptive_beam = FLT_MAX;
	float cur_cutoff;

	cur_cutoff = wtk_kwdec2_get_cutoff2(dec,
			&adaptive_beam, &best_elem);
	//cur_cutoff=qtk_kwfstdec_get_cutoff(dec,final_toks,&tok_cnt, &adaptive_beam, &best_elem);
	//wtk_debug("=====================process emitting: cur_cutoff:%f adaptive_beam:%f\n",cur_cutoff,adaptive_beam);
	//NEED RESIZE HASH???
	float next_cutoff = FLT_MAX;
	float tot_cost = FLT_MAX;
	float cost_offset = 0.0;
	float new_weight = FLT_MAX, likelihood, ac_cost, graph_cost, cur_cost;
	//float lmscale = dec->cfg->lm_scale;
	wtk_fst_state_t *state;
	wtk_fst_trans_t *trans;
	wtk_kwdec2_token_t *token = NULL;
	wtk_queue_t *tmp_q;
	wtk_queue_node_t *qn;

	if (best_elem)
	{
		//wtk_debug("=====================process emitting has best element: %d\n",best_elem->key);
		//int state_id=best_elem->key;
		//wtk_kwdec2_token_t *tok = best_elem->token;
		cost_offset = -best_elem->tot_cost;
		state = best_elem->state;//wtk_fst_net_get_load_state(dec->net,state_id);
		for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans)
		{
			if (trans->in_id != 0)
			{
				dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);
				//likelihood=(*(obs+dnn_cnt+1));///10;
				if(dec->cfg->use_fixpoint)
				{
					short* t =(short*)obs;
					likelihood = FIX2FLOAT_ANY(*(t + dnn_cnt),dec->shift);
				}else
				{
					likelihood = (*(obs + dnn_cnt));	
				}	
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
		token = data_offset(qn,wtk_kwdec2_token_t,q_n);//elem->token;
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

					//		if(!trans->to_state)
					//		{
					//			continue;
					//		}
					//wtk_debug("=======process emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
					dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);
					if(dec->cfg->use_fixpoint)
					{
						short* t =(short*)obs;
						likelihood = FIX2FLOAT_ANY(*(t + dnn_cnt),dec->shift);
					}else
					{
						likelihood = (*(obs + dnn_cnt));
					}
					//likelihood=(*(obs+dnn_cnt+1));///10;
					//wtk_debug("%d likehood:%f\n",dnn_cnt+1,likelihood);
					//wtk_debug("%d likehood:%f\n",trans->in_id,likelihood);
					ac_cost = (cost_offset - likelihood)*dec->cfg->ac_scale;
					graph_cost = -trans->weight;
					cur_cost = token->tot_cost;
					
					tot_cost = cur_cost + ac_cost + graph_cost;

					//wtk_debug("cost_offset:%f likehood:%f adaptive_beam:%f\n",cost_offset,likelihood,adaptive_beam);
					//wtk_debug("ac_cost:%f  graph_cost:%f  cur_cost:%f\n",ac_cost,graph_cost,cur_cost);
					//wtk_debug("tot_cost:%f  next_cutoff:%f\n",tot_cost,next_cutoff);
					if (tot_cost > next_cutoff)
						continue;	//{wtk_debug("continue prune\n");continue;}
					else if (tot_cost + adaptive_beam < next_cutoff)
						next_cutoff = tot_cost + adaptive_beam;
//					wtk_debug("search or add state:%d\n",trans->to_state->id)
					wtk_kwdec2_find_or_add_token(dec,
							trans, tot_cost, NULL, token,likelihood);
				}
			}
		}

		wtk_kwdec2_push_token(dec,token);
	}
	return next_cutoff;
}

int wtk_kwdec2_process_none_emitting_state(wtk_kwdec2_t* dec, float cutoff) {
	//wtk_debug("============process none emitting state %f\n",cutoff);
	//qtk_hash_elem_t *elem;
	//st_q_t *st, *st1;
	wtk_kwdec2_token_t *token;
	//wtk_kwdec2_token_t *new_token;
	wtk_queue_node_t *qn;
	wtk_fst_state_t *state;					//need free?
	wtk_fst_trans_t *trans;					//need free?
	float tot_cost = FLT_MAX;
	float cur_cost = FLT_MAX;
	float graph_cost;
	//float lmscale = dec->cfg->lm_scale;
	int changed = 0, i;
	//for (elem = dec->tok->list_head; elem != NULL; elem = elem->tail)
//	for (qn = dec->cur_tokq->pop; qn; qn = qn->next)
//	{
//		token = data_offset2(qn,wtk_kwdec2_token_t,q_n);
//		st = (st_q_t*) wtk_heap_malloc(dec->heap, sizeof(st_q_t));
//		//st->state_id = elem->key;
//		st->state = token->state;
//		wtk_queue_push(&(dec->state_q), &(st->q_n));
//	}
	//wtk_debug("============process none emitting queue len: %d\n",dec->cur_tokq->length);
	if (dec->cur_tokq->length == 0)
	{
		// wtk_debug("warning!!!!!!!!!\n");
	}

	for (qn = dec->cur_tokq->pop; qn;)
	{
		//wtk_debug("%p\n",qn);
		token = data_offset2(qn,wtk_kwdec2_token_t,q_n);
		state = token->state;
//		qn = wtk_queue_pop_back(&(dec->state_q));
//		if (!qn)
//		{
//			break;
//		}
		//st = data_offset(qn, st_q_t, q_n);
		//wtk_debug("%d %p\n",st->state_id,qtk_hash_list_find(dec->tok,st->state_id));
		//state = st->state;	//wtk_fst_net_get_load_state(dec->net,st->state_id);
		//token = (wtk_kwdec2_token_t*) state->hook;
		//token=(qtk_hash_list_find(dec->tok,st->state_id)->token);
	    qn = qn->next;
		cur_cost = token->tot_cost;
		//wtk_debug("============process none emitting state:%d cur_cost:%f cutoff:%f\n",st->state_id,cur_cost,cutoff);
		if (cur_cost > cutoff)
			continue;
		//qtk_hash_list_link_delete(token);
		//state=wtk_fst_net_get_load_state(dec->net,st->state_id);
		for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans)
		{
			//wtk_debug("%p\n",trans);
			if (trans->in_id == 0)
			{
				//wtk_debug("============process none emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
//		wtk_debug("%d %p\n",st->state_id,trans->to_state);
				graph_cost = -(trans->weight);
				tot_cost = cur_cost + graph_cost;
				//			wtk_debug("============process none emitting state:%d to:%d ilabel:%d olabel:%d weight:%f tot_cost:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight,tot_cost);
				//			wtk_debug("============process none emitting cur_cost:%f graph_cost:%f\n",cur_cost,graph_cost);
				//wtk_debug("tot_cost:%f cutoff:%f\n",tot_cost,cutoff);
				if (tot_cost < cutoff)
				{
					//wtk_debug("process none emitting try add token\n");
					wtk_kwdec2_find_or_add_token(dec,
							trans, tot_cost, &changed,
							token, 0);
				}
			}
		}
	}

	return 0;
}

int wtk_kwdec2_isFiniteNumber2(double d)
{
	return (d < FLT_MAX && d > -FLT_MAX);
}

void wtk_kwdec2_log(float *p, int n)
{
	float *pe;

	pe = p + n;
	while (p < pe)
	{
		*p = log(*p);
		++p;
	}
}

int wtk_kwdec2_feed_decode(wtk_kwdec2_t* dec, wtk_vector_t *v, int index)
{
	float *v2;
	v2 = v;
	int ret = -1;
	float cost_cutoff;
	dec->mode_frame_cnt++;
	if(dec->mode_frame_cnt > dec->switch_frame_cnt){
		wtk_kwdec2_switch_mode(dec,1);
	}
    // wtk_debug("==========feed frame %d=============\n", index);
    dec->cur_frame = index;
	//print_float(v,10);
	cost_cutoff = wtk_kwdec2_process_emitting_state(dec, v2);
	wtk_kwdec2_process_none_emitting_state(dec, cost_cutoff);
	ret = wtk_kwdec2_post_feed(dec,v2);
    if(ret==0)
    {
        dec->found=1;
		dec->parm->parm->stop_flag=1;
		dec->mode_frame_cnt = 0;
		wtk_kwdec2_switch_mode(dec,0);
    }
	return ret;
}

void wtk_kwdec2_on_feat(wtk_kwdec2_t *dec,wtk_kfeat_t *feat)
{
	//if(dec->cfg->use_fixpoint)
	//{
	//	short*fv = (short*)feat->v;
	//}
	// wtk_debug("=======FEAT=======idx=%d==============\n",feat->index);
    // print_float(feat->v,dec->out_col);
	// exit(0);
	if(dec->found==1) return;
	wtk_kwdec2_feed_decode(dec,feat->v,feat->index);
}

void wtk_kwdec2_on_nnet3(wtk_kwdec2_t *dec, qtk_blas_matrix_t *f1, int end,
                         int plus) {
    // if(dec->cfg->use_fixpoint)
    //{
    //	short*fv = (short*)feat->v;
    //}
    // wtk_debug("=======FEAT=======idx=%d
    // %d==============\n",dec->index,dec->found);
    // print_float(feat->v,dec->out_col);
    // exit(0);
    if (dec->found == 1 || end == 1){
        return;
	}
	int i;
	for(i=0;i<f1->row;i++){
		wtk_kwdec2_feed_decode(dec, f1->m+f1->col*i, dec->index);
		dec->index++;
	}
}

int wtk_kwdec2_feed(wtk_kwdec2_t* dec, char *data,int len, int is_end)
{
#ifdef USE_LOG
	static wtk_wavfile_t *log=NULL;
	{
		if(!log)
		{
			static int ki=0;
			char tmp[256];

			++ki;
			sprintf(tmp,"kwdec%d.wav",ki);
			log=wtk_wavfile_new(16000);
			wtk_wavfile_open(log,tmp);
			log->max_pend=0;
		}
		wtk_wavfile_write(log,data,len);
		if(log && (is_end||w->waked))
		{
			wtk_wavfile_close(log);
			wtk_wavfile_delete(log);
			log=NULL;
		}
	}
#endif
	wtk_kxparm_feed(dec->parm,(short*)data,len/2,is_end);
	if(dec->found==1)
	{
		return 1;
	}

	return 0;
}

int wtk_kwdec2_feed2(wtk_kwdec2_t* dec, short *data,int len, int is_end)
{
#ifdef USE_LOG
	static wtk_wavfile_t *log=NULL;
	{
		if(!log)
		{
			static int ki=0;
			char tmp[256];

			++ki;
			sprintf(tmp,"kwdec%d.wav",ki);
			log=wtk_wavfile_new(16000);
			wtk_wavfile_open(log,tmp);
			log->max_pend=0;
		}
		wtk_wavfile_write(log,(char*)data,len*2);
		if(log && (is_end||w->waked))
		{
			wtk_wavfile_close(log);
			wtk_wavfile_delete(log);
			log=NULL;
		}
	}
#endif
    wtk_kxparm_feed(dec->parm,data,len,is_end);
    if(dec->found==1)
    {
    	return 1;
    }

	return 0;
}


void wtk_kwdec2_get_wake_time(wtk_kwdec2_t *dec,float *fs,float *fe)
{
	float dur=dec->parm->parm->cfg->frame_step_ms/1000;
	int beg_idx;
	int end_idx;
        float pad = 0.0;
        beg_idx = dec->wake_beg_idx;
        end_idx = dec->wake_end_idx;
        if (dec->parm->cfg->use_knn) {
            *fs = beg_idx * dur * (dec->parm->knn->cfg->skip + 1);
            *fe = end_idx * dur * (dec->parm->knn->cfg->skip + 1) + pad;
        } else {
            *fs = beg_idx * dur * (dec->parm->nnet3->cfg->frame_per_chunk);
            *fe =
                end_idx * dur * (dec->parm->nnet3->cfg->frame_per_chunk) + pad;
        }
        // wtk_debug("====%f,beg=%d,end=%d\n",dur,beg_idx,end_idx);
}

float wtk_kwdec2_get_conf(wtk_kwdec2_t *dec) { return dec->conf; }

int wtk_kwdec2_set_words(wtk_kwdec2_t* dec,char *words,int len){
	int ret = -1;
	int i;
	if(dec->cfg->use_egram){
	dec->egram = wtk_egram_new(&(dec->cfg->egram), dec->cfg->rbin);
	wtk_strbuf_reset(dec->ebnf_buf);
	//wtk_strbuf_push_s(dec->ebnf_buf,"$main=");
	wtk_strbuf_push(dec->ebnf_buf,words,len);
	//wtk_strbuf_push_s(dec->ebnf_buf,";(\\<s\\>($main)\\<\\/s\\>)");
	//wtk_debug("%.*s\n",dec->ebnf_buf->pos,dec->ebnf_buf->data);
	//ret = wtk_egram_ebnf2fst(dec->egram, dec->ebnf_buf->data, dec->ebnf_buf->pos);
	ret = wtk_egram_ebnf2fst_kwdec(dec->egram, dec->ebnf_buf->data, dec->ebnf_buf->pos);
	if(ret == -1){
		return ret;
	}
	wtk_egram_dump_kwdec(dec->egram, dec->net);
	//exit(0);
	//ret = wtk_egram_dump2(dec->egram, dec->net);

	if (dec->words_set) {
		for (i = 0; i < dec->cfg->words->nslot; ++i) {
			wtk_free(dec->words_set[i]);
		}
		wtk_free(dec->words_set);
	}
	//wtk_kwdec2_cfg_clean_words_set(dec->cfg);
	//wtk_kwdec2_cfg_set_words_set(dec->cfg, words, len);
	//wtk_kwdec2_decwords_set(dec);
	//dec->cfg->ebnf_dump = 1;
	wtk_egram_delete(dec->egram);
	dec->egram = NULL;
	// dec->words_set[0]->pdf_conf = -40.0;
	// dec->words_set[0]->min_kws = 0;
	}
	return ret;
}

void wtk_kwdec2_set_words_cfg(wtk_kwdec2_t* dec,char *words,int len){
	wtk_kwdec2_cfg_clean_words_set(dec->cfg);
	wtk_kwdec2_cfg_set_words_set(dec->cfg, words, len);
	dec->cfg->ebnf_dump = 1;
}

int wtk_kwdec2_set_context(wtk_kwdec2_t* dec,char *words,int len){
	int ret = 0;
	ret = wtk_kwdec2_set_words(dec, words, len);
	wtk_kwdec2_set_words_cfg(dec, words, len);
	wtk_kwdec2_decwords_set(dec);
	return ret;
}