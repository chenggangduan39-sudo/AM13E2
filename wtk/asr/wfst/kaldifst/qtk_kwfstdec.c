#include "qtk_kwfstdec.h"
qtk_kwfstdec_token_list_t* qtk_hash_list_new_token_list(qtk_kwfstdec_t* dec,
		qtk_kwfstdec_token_t* tok)
{
	qtk_kwfstdec_token_list_t *list;
	list = (qtk_kwfstdec_token_list_t*) wtk_heap_malloc(dec->heap,
			sizeof(qtk_kwfstdec_token_list_t));

	list->token = tok;
	list->must_prune_forward_links = 1;
	list->must_prune_tokens = 1;
	return list;
}

int qtk_kwfstdec_set_hot_words(qtk_kwfstdec_t* dec, char *str)
{
	int ret = 0;
	wtk_array_t *a, *b;
	wtk_string_t **strs, **strs2, *v;
	qtk_kwfstdec_classes_t *class;
	int i, len, word_num = 0, out_id;
	wtk_queue_node_t *qn;

	if (str)
	{
		len = strlen(str);
		wtk_heap_reset(dec->heap2);
		wtk_queue_init(&(dec->hot_words));
		a = wtk_str_to_array(dec->heap2, str, len, ';');
		strs = (wtk_string_t**) a->slot;
		for (i = 0; i < a->nslot; i++)
		{
			v = strs[i];
			//wtk_debug("%.*s\n",v->len,v->data);
			class = (qtk_kwfstdec_classes_t*) wtk_heap_malloc(dec->heap2,
					sizeof(qtk_kwfstdec_classes_t));
			b = wtk_str_to_array(dec->heap2, v->data, v->len, ',');
			//wtk_debug("%d\n",b->nslot);
			strs2 = (wtk_string_t**) b->slot;
			class->class_n = strs2[0];
			class->word = b;
			wtk_queue_push(&(dec->hot_words), &(class->q_n));
			word_num += b->nslot - 1;
		}
	}
	dec->hot_sym = (wtk_string_t**) wtk_calloc(word_num + 3,sizeof(wtk_string_t *)); //pay attention skip 0 1 2
	out_id = 3;
	for (qn = dec->hot_words.pop; qn; qn = qn->next)
	{
		class = (qtk_kwfstdec_classes_t*) data_offset(qn,qtk_kwfstdec_classes_t, q_n);
		strs = (wtk_string_t**) class->word->slot;
		for (i = 1; i < class->word->nslot; i++)
		{
			dec->hot_sym[out_id] = strs[i];
			//wtk_debug("set hotsym %.*s %d\n",strs[i]->len,strs[i]->data,out_id);
			out_id++;
		}
	}

	return ret;
}

qtk_kwfstdec_t* qtk_kwfstdec_new(qtk_kwfstdec_cfg_t* cfg)
{
	qtk_kwfstdec_t *kdec = (qtk_kwfstdec_t*) wtk_malloc(sizeof(qtk_kwfstdec_t));

	kdec->heap = wtk_heap_new(4096);
	kdec->heap2 = wtk_heap_new(4096);
	kdec->bins = (unsigned int*) wtk_malloc(sizeof(unsigned int) * 500);
	memset(kdec->bins, 0, sizeof(unsigned int) * 500);

	wtk_queue_init(&(kdec->active_tok));
	wtk_queue_init(&(kdec->hot_words));
	wtk_queue_init(&(kdec->state_q));
	wtk_queue_init(&(kdec->tmp_q));
	wtk_queue_init(&(kdec->final_cost_q));
	//wtk_queue_init(&(kdec->token_map_q));
	wtk_queue_init(&(kdec->topsorted_list_q));

	kdec->cfg = cfg;
	//kdec->trans_model=cfg->trans_model.trans_model;
	kdec->net = wtk_fst_net_new(&(cfg->net));
	kdec->hot_sym = NULL;
	kdec->lat_support = NULL;
	if (cfg->use_lat)
	{
		//r->use_ntok=0;
		kdec->lat_net = wtk_fst_net3_new(&(cfg->lat_net), &(cfg->net));
	} else
	{
		kdec->lat_net = 0;
	}
	if (cfg->use_rescore)
	{
		kdec->lat_rescore = wtk_rescore_new(&(cfg->rescore), NULL);
	} else
	{
		kdec->lat_rescore = 0;
	}
	kdec->num_toks = 0;
	kdec->cur_frame = -1;
	kdec->tok = qtk_hash_list_new(cfg->size);
	kdec->best_weight = FLT_MAX;
	kdec->best_elem = NULL;
	kdec->best_token = NULL;
	kdec->best_final_cost = FLT_MAX;
	kdec->cur_token = NULL;
	kdec->final_relative_cost = FLT_MAX;
	kdec->final_best_cost = FLT_MAX;
	kdec->cost_offset = (float*) wtk_malloc(6000 * sizeof(float));
	kdec->offset_count = 0;
	kdec->input_net = wtk_fst_net2_new(&(cfg->net));
	if (cfg->use_context) {
		kdec->context_net = wtk_fst_net_new(&(cfg->net));
	} else {
		kdec->context_net = NULL;
	}

	kdec->path_cost = NULL;
	kdec->path_id = NULL;
	kdec->path_out_cnt = NULL;
	if(cfg->use_av_conf){
		kdec->path_cost = wtk_strbuf_new(1024,1);
		kdec->path_id = wtk_strbuf_new(1024,1);	
		kdec->path_out_cnt = wtk_strbuf_new(1024,1);	
		kdec->path_out_id = wtk_strbuf_new(1024,1);
	}

	kdec->conf = 0.0;
	if (cfg->use_hot)
	{
		qtk_kwfstdec_set_hot_words(kdec, kdec->cfg->hot_words);
	}
	//GetInstance(&(kdec->handle));
	int i;
	float *p = kdec->cost_offset;
	for (i = 0; i < 6000; i++)
	{
		*p = 0.0;
		p++;
	}

	kdec->onnx_dec = 0;
	kdec->idle = 1;
	kdec->vad_index = 0;
	kdec->valid_index = 0;
	kdec->recommand_conf = kdec->cfg->idle_conf;
	return kdec;
}

//for outer net
qtk_kwfstdec_t* qtk_kwfstdec_new2(qtk_kwfstdec_cfg_t* cfg, int use_outnet)
{
	qtk_kwfstdec_t *kdec = (qtk_kwfstdec_t*) wtk_malloc(sizeof(qtk_kwfstdec_t));

	kdec->heap = wtk_heap_new(4096);
	kdec->heap2 = wtk_heap_new(4096);
	kdec->bins = (unsigned int*) wtk_malloc(sizeof(unsigned int) * 500);
	memset(kdec->bins, 0, sizeof(unsigned int) * 500);

	wtk_queue_init(&(kdec->active_tok));
	wtk_queue_init(&(kdec->hot_words));
	wtk_queue_init(&(kdec->state_q));
	wtk_queue_init(&(kdec->tmp_q));
	wtk_queue_init(&(kdec->final_cost_q));
	//wtk_queue_init(&(kdec->token_map_q));
	wtk_queue_init(&(kdec->topsorted_list_q));

	kdec->cfg = cfg;
	//kdec->trans_model=cfg->trans_model.trans_model;
	if (use_outnet)
		kdec->net = NULL;
	else
		kdec->net = wtk_fst_net_new(&(cfg->net));
	kdec->hot_sym = NULL;
	kdec->lat_support = NULL;
	if (cfg->use_lat)
	{
		//r->use_ntok=0;
		kdec->lat_net = wtk_fst_net3_new(&(cfg->lat_net), &(cfg->net));
	} else
	{
		kdec->lat_net = 0;
	}
	if (cfg->use_rescore)
	{
		kdec->lat_rescore = wtk_rescore_new(&(cfg->rescore), NULL);
	} else
	{
		kdec->lat_rescore = 0;
	}
	kdec->num_toks = 0;
	kdec->cur_frame = -1;
	kdec->tok = qtk_hash_list_new(cfg->size);
	kdec->best_weight = FLT_MAX;
	kdec->best_elem = NULL;
	kdec->best_token = NULL;
	kdec->best_final_cost = FLT_MAX;
	kdec->cur_token = NULL;
	kdec->final_relative_cost = FLT_MAX;
	kdec->final_best_cost = FLT_MAX;
	kdec->cost_offset = (float*) wtk_malloc(6000 * sizeof(float));
	kdec->offset_count = 0;
	kdec->input_net = wtk_fst_net2_new(&(cfg->net));
	kdec->conf = 0.0;
	if (cfg->use_hot)
	{
		qtk_kwfstdec_set_hot_words(kdec, kdec->cfg->hot_words);
	}
	//GetInstance(&(kdec->handle));
	int i;
	float *p = kdec->cost_offset;
	for (i = 0; i < 6000; i++)
	{
		*p = 0.0;
		p++;
	}
	
	return kdec;
}

void qtk_kwfstdec_reset(qtk_kwfstdec_t* dec)
{
//	wtk_debug("---------reset--------\n");
	memset(dec->bins, 0, sizeof(unsigned int) * 500);

	wtk_heap_reset(dec->heap);
	if(dec->net)
		wtk_fst_net_reset(dec->net);

	if (dec->context_net) {
		wtk_fst_net_reset(dec->context_net);
	}

	wtk_fst_net2_reset(dec->input_net);
	qtk_hash_list_reset(dec->tok);

	if(dec->path_cost){
		wtk_strbuf_reset(dec->path_cost);
		wtk_strbuf_reset(dec->path_id);
		wtk_strbuf_reset(dec->path_out_cnt);
		wtk_strbuf_reset(dec->path_out_id);
	}

	wtk_queue_init(&(dec->active_tok));
	wtk_queue_init(&(dec->state_q));
	wtk_queue_init(&(dec->tmp_q));
	wtk_queue_init(&(dec->final_cost_q));
	//wtk_queue_init(&(dec->token_map_q));
	wtk_queue_init(&(dec->topsorted_list_q));
	if (dec->cfg->use_lat)
	{
		wtk_fst_net3_reset(dec->lat_net);
		//dec->use_rescore=1;
	}
	if (dec->lat_rescore)
	{
		wtk_rescore_reset(dec->lat_rescore);
	}
	dec->num_toks = 0;
	dec->cur_frame = -1;
	dec->best_token = NULL;
	dec->best_elem = NULL;
	dec->best_weight = FLT_MAX;
	dec->best_final_cost = FLT_MAX;
	dec->cur_token = NULL;
	dec->final_relative_cost = FLT_MAX;
	dec->final_best_cost = FLT_MAX;

	if(dec->lat_support)
	{
		free(dec->lat_support);
		dec->lat_support = NULL;
	}

	int i;
	float *p = dec->cost_offset;
	for (i = 0; i < 6000; i++)
	{
		*p = 0.0;
		p++;
	}
	dec->offset_count = 0;
	dec->conf = 0.0;
	//dec->trans_model=dec->cfg->trans_model.trans_model;
}

void qtk_kwfstdec_delete(qtk_kwfstdec_t* dec)
{
	//wtk_debug("delete kwfstdec\n");
	if (dec->lat_rescore)
	{
		wtk_rescore_delete(dec->lat_rescore);
	}
	wtk_free(dec->bins);
	wtk_free(dec->cost_offset);
	wtk_fst_net2_delete(dec->input_net);
	wtk_heap_delete(dec->heap);
	wtk_heap_delete(dec->heap2);
	if (dec->net)
		wtk_fst_net_delete(dec->net);

	if (dec->context_net) {
		wtk_fst_net_delete(dec->context_net);
	}

	if(dec->path_cost){
		wtk_strbuf_delete(dec->path_cost);
		wtk_strbuf_delete(dec->path_id);
		wtk_strbuf_delete(dec->path_out_cnt);
		wtk_strbuf_delete(dec->path_out_id);
	}

	qtk_hash_list_delete(dec->tok);
	if (dec->cfg->use_lat)
	{
		wtk_fst_net3_delete(dec->lat_net);
	}
	if (dec->hot_sym)
	{
		wtk_free(dec->hot_sym);
	}
	wtk_free(dec);
}

int qtk_kwfstdec_process_none_emitting_state(qtk_kwfstdec_t* dec, float cut_off);

int qtk_kwfstdec_start(qtk_kwfstdec_t* dec)
{
	int stateID = 0;
	qtk_kwfstdec_token_t* start_tok;
	qtk_kwfstdec_token_list_t* tok_list;
	wtk_fst_state_t* state;

	dec->num_toks = 1;
	dec->decoding_finalized = 0;
	start_tok = qtk_hash_list_new_token(dec->tok, 0);
	//wtk_debug("xxxxxxx:%p\n",start_tok);
	tok_list = qtk_hash_list_new_token_list(dec, start_tok); 
	state = wtk_fst_net_get_load_state(dec->net, stateID);
	state->frame = -1;
	state->hook = start_tok;
	qtk_hash_list_Insert(dec->tok, stateID, state, start_tok);
            
	if(dec->cfg->has_filler){
		int fillerID = dec->cfg->norm_filler_id;
		qtk_kwfstdec_token_t* start_tok_filler;
		wtk_fst_state_t* state_filler;
		if(dec->cfg->use_multi_filler){
			if(dec->idle != 0){
				fillerID = dec->cfg->idle_filler_id;
			}else{
				fillerID = dec->cfg->norm_filler_id;
			}
			//wtk_debug("%d %f\n",fillerID,tmp_confxx);
		}
		start_tok_filler = qtk_hash_list_new_token(dec->tok, 0);
		state_filler = wtk_fst_net_get_load_state(dec->net, fillerID);
		state_filler->frame = -1;
		state_filler->hook = start_tok_filler;
		qtk_hash_list_Insert(dec->tok, fillerID, state_filler, start_tok_filler);	
	}
 
	if (dec->context_net) {
		start_tok->context_state =
			wtk_fst_net_get_load_state(dec->context_net, stateID);
	}
	wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
	dec->cur_token_list = tok_list;
	qtk_kwfstdec_process_none_emitting_state(dec, dec->cfg->beam);

	return 0;
}

qtk_kwfstdec_token_t* qtk_kwfstdec_find_or_add_token(qtk_kwfstdec_t* dec,
		wtk_fst_trans_t* trans, int fram_plus_one, float tot_cost, int *changed,
		qtk_kwfstdec_token_t *pre_tok, float ac_cost)
{
	//wtk_debug("===============find or add token state:%d\n",state);
	qtk_kwfstdec_token_t *token, *update_tok;
	qtk_kwfstdec_token_list_t*tok_list;
	//qtk_hash_elem_t *elem;
	float extra_cost = 0.0f;
	wtk_fst_state_t* state=trans->to_state;
	//tok_list = qtk_hash_list_new_token_list(token);
	//wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
	//elem=qtk_hash_list_find(dec->tok,state);

	if(dec->net->nrbin_states <= 0)
	{
		wtk_fst_net_load_state(dec->net, state);
	}else
	{
		state = wtk_fst_net_get_load_state(dec->net,state->id);
	}
	if (state->frame == dec->cur_frame)
	{
		update_tok = (qtk_kwfstdec_token_t*) state->hook;
	} else
	{
		update_tok = NULL;
	}

	if (!update_tok)
	{
		//wtk_debug("===============find or add token state not found\n");
		tok_list = dec->cur_token_list;
		token = qtk_hash_list_new_token(dec->tok, extra_cost);
		token->pth = qtk_hash_list_new_pth(dec->tok, pre_tok);
		token->pth->in_label = trans->in_id;
		token->pth->out_label = trans->out_id;
		token->pth->frame = dec->cur_frame;
		token->pth->ac_cost = ac_cost;
		token->ac_cost = ac_cost;
		token->tot_cost = tot_cost;
		if (tok_list->token != NULL)
			token->next = tok_list->token;
		else
			token->next = NULL;
		dec->num_toks++;
		qtk_hash_list_Insert(dec->tok, state->id, state, token);
		if (changed)
			*changed = 1;
		//tok_list = qtk_hash_list_new_token_list(token);
		//wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
		tok_list->token = token;
		state->frame = dec->cur_frame;
		state->hook = token;
	} else
	{
		//wtk_debug("===============find or add token state found\n");
		token = update_tok;
		if (token->tot_cost > tot_cost)
		{
			token->tot_cost = tot_cost;
			token->ac_cost = ac_cost;
			token->pth->lbest = pre_tok;
			token->pth->in_label = trans->in_id;
			token->pth->out_label = trans->out_id;
			token->pth->ac_cost = ac_cost;
			if (changed)
				*changed = 1;
		} else
		{
			if (changed)
				*changed = 0;
		}
	}

	return token;
}

int qtk_kwfstdec_tmp_cost_queue_cmp(wtk_queue_node_t *qn1,wtk_queue_node_t *qn2)
{
	tmp_q_t *tok1, *tok2;
	tok1 = NULL;
	tok2 = NULL;

	tok1 = data_offset2(qn1, tmp_q_t, q_n);
	tok2 = data_offset2(qn2, tmp_q_t, q_n);
	return (tok1->cost - tok2->cost) < 0 ? 1 : -1;
}

float qtk_kwfstdec_get_cutoff(qtk_kwfstdec_t* dec, qtk_hash_elem_t *list_head,
		int *tok_count, float *adaptive_beam, qtk_hash_elem_t **best_elem)
{
//	wtk_debug("=============get cut off\n");
	float best_weight = FLT_MAX, w = FLT_MAX, beam_cutoff = FLT_MAX,
			min_active_cutoff = FLT_MAX, max_active_cutoff = FLT_MAX;
	int count = 0;
	qtk_hash_elem_t *e;
	tmp_q_t *tmp, *t;
	wtk_queue_node_t *qn;

	if (dec->cfg->max_active == UINT_MAX && dec->cfg->min_active == 0)
	{
		for (e = list_head; e != NULL; e = e->tail, count++)
		{
			if (e->key == 0)
				continue;
//			wtk_debug("=============get cut off searching elem state:%d cost:%f\n",e->key,e->token->tot_cost);
			w = e->token->tot_cost;
			if (w < best_weight)
			{
				best_weight = w;
				if (best_elem)
					*best_elem = e;
			}
		}
		//wtk_debug("=============get cut off elem count:%d\n",count);
		if (tok_count != NULL)
			*tok_count = count;
		if (adaptive_beam != NULL)
			*adaptive_beam = dec->cfg->beam;
		return best_weight + dec->cfg->beam;
	} else
	{
		wtk_queue_init(&(dec->tmp_q));		//??? need free node ???
		for (e = list_head; e != NULL; e = e->tail, count++)
		{
//			wtk_debug("=============get cut off searching elem state:%d cost:%f\n",e->key,e->token->tot_cost);
			w = e->token->tot_cost;
			tmp = (tmp_q_t*) wtk_heap_malloc(dec->heap, sizeof(tmp_q_t));
			tmp->cost = w;
			//wtk_queue_push(&(dec->tmp_q), &(tmp->q_n));
			wtk_queue_sort_insert(&(dec->tmp_q), &(tmp->q_n),
					(wtk_queue_node_cmp_f) qtk_kwfstdec_tmp_cost_queue_cmp);//TODO!!!!! dont forget wtk_queue
			//void wtk_queue_sort_insert(wtk_queue_t *q,wtk_queue_node_t *qn1,wtk_queue_node_cmp_f cmp)
			//wtk_debug("%f %f\n",w,best_weight);
			if (w < best_weight)
			{
				best_weight = w;
				if (best_elem)
					*best_elem = e;
			}
		}
//			wtk_debug("=============get cut off elem count:%d\n",count);
		if (tok_count != NULL)
			*tok_count = count;
		if (dec->tmp_q.length > dec->cfg->max_active)
		{
			qn = wtk_queue_peek(&(dec->tmp_q), dec->cfg->max_active);
			t = (tmp_q_t*) data_offset(qn, tmp_q_t, q_n);
			max_active_cutoff = t->cost;
			//		wtk_debug("%f\n",max_active_cutoff);
		}
		beam_cutoff = best_weight + dec->cfg->beam;
//		wtk_debug("=============get cut off max_active_cutoff:%f beam_cutoff:%f\n",max_active_cutoff,beam_cutoff);
		if (max_active_cutoff < beam_cutoff)
		{
			if (adaptive_beam)
				*adaptive_beam = max_active_cutoff - best_weight
						+ dec->cfg->beam_delta;
//			wtk_debug("return max_active_cutoff\n");
			return max_active_cutoff;
		}
		if (dec->tmp_q.length > dec->cfg->min_active)
		{
			if (dec->cfg->min_active == 0)
				min_active_cutoff = best_weight;
			else
			{
				qn = wtk_queue_peek(&(dec->tmp_q), dec->cfg->min_active);
				t = (tmp_q_t*) data_offset(qn, tmp_q_t, q_n);

				min_active_cutoff = t->cost;
				//			wtk_debug("%f\n",min_active_cutoff);
			}
		}
//		wtk_debug("=============get cut off min_active_cutoff:%f beam_cutoff:%f\n",min_active_cutoff,beam_cutoff);
		if (min_active_cutoff > beam_cutoff)
		{
			if (adaptive_beam)
				*adaptive_beam = min_active_cutoff - best_weight
						+ dec->cfg->beam_delta;
//			wtk_debug("return min_active_cutoff\n");
			return min_active_cutoff;
		} else
		{
			*adaptive_beam = dec->cfg->beam;
//		      wtk_debug("return beam cutoff\n");
			return beam_cutoff;
		}
	}

	return 0;
}

float qtk_kwfstdec_get_cutoff2(qtk_kwfstdec_t* dec, qtk_hash_elem_t *list_head,
		int *tok_count, float *adaptive_beam, qtk_hash_elem_t **best_elem)
{
	float best_weight=FLT_MAX, w=0.0, beam_cutoff=FLT_MAX,
			min_active_cutoff=FLT_MAX, max_active_cutoff=FLT_MAX;
	unsigned int  *bins, cnt = 0;
	qtk_hash_elem_t *e=NULL, *te=NULL;
	register float ww;
	int idx,count=0;

	bins=dec->bins;
	memset(bins, 0, sizeof(unsigned int) * 500);
	if (dec->cfg->max_active == UINT_MAX && dec->cfg->min_active == 0)
	{
		for (e = list_head,count=0; e != NULL; e = e->tail, count++)
		{
			if (e->key == 0)
				continue;
//			wtk_debug("=============get cut off searching elem state:%d cost:%f\n",e->key,e->token->tot_cost);
			w = e->token->tot_cost;
			if (w < best_weight)
			{
				best_weight = w;
				if (best_elem)
					*best_elem = e;
			}
		}
		//wtk_debug("=============get cut off elem count:%d\n",count);
		if (tok_count != NULL)
			*tok_count = count;
		if (adaptive_beam != NULL)
			*adaptive_beam = dec->cfg->beam;
		return best_weight + dec->cfg->beam;
	} else
	{
		for (e = list_head,count=0; e != NULL; e = e->tail, count++)
		{
			if (!e->key)
				continue;
			w = e->token->tot_cost;
			ww = w * 10.0f + 200.0f;
			if(dec->onnx_dec==1)
			{
				ww = w;
			}
			if (ww < 0.0f)
			{
				idx = 0;
			} else if (ww >= 500.0f)
			{
				idx = 499;
			} else
			{
				idx = (int) (ww);
			}
			*(bins + idx) += 1;
			if (w < best_weight)
			{
				best_weight = w;
				//wtk_debug("best_weight=%f\n", w);
				te=e;
			}
		}
		//wtk_debug("count=%d\n", count);
		if (best_elem)
			*best_elem = te;
		if (tok_count != NULL)
			*tok_count = count;
		if (count > dec->cfg->max_active)
		{
			for (cnt=0,idx = 0; idx < 500; idx++)
			{
				cnt += dec->bins[idx];
				if (cnt > dec->cfg->max_active)
				{
					//max_active_cutoff = ((idx - 199) * 1.0) / 10;
					max_active_cutoff = idx*0.1-19.9;//((k - 199) * 1.0) / 10; ((k+1 - 200) * 1.0) / 10
					if(dec->onnx_dec==1)
					{
						max_active_cutoff = idx;
					}
					break;
				}
			}
		}
		beam_cutoff = best_weight + dec->cfg->beam;
		//wtk_debug("=============get cut off max_active_cutoff:%f beam_cutoff:%f cnt=%d\n",max_active_cutoff,beam_cutoff, cnt);
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
				for (cnt=0,idx = 0; idx < 500; idx++)
				{
					cnt += bins[idx];
					//wtk_debug("%d\n",cnt);
					if (cnt > dec->cfg->min_active)
					{
						//wtk_debug("%d %d %f\n",k,dec->bins[k],((k-200)*1.0)/10);
						//min_active_cutoff = ((idx - 199) * 1.0) / 10;
						min_active_cutoff = idx*0.1-19.9;//((k - 199) * 1.0) / 10; ((k+1 - 200) * 1.0) / 10
						if(dec->onnx_dec == 1)
						{
							min_active_cutoff = idx;
						}
						//wtk_debug("%f\n",min_active_cutoff);
						break;
					}
				}
			}
		}
		//wtk_debug("=============get cut off min_active_cutoff:%f beam_cutoff:%f cnt=%d\n",min_active_cutoff,beam_cutoff,cnt);
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

void qtk_kwfstdec_process_context_state(qtk_kwfstdec_t *dec,
                                        qtk_kwfstdec_token_t *pre_tok,
                                        qtk_kwfstdec_token_t *tok, int olabel,
                                        float *context_score) {
    int i;
    wtk_fst_trans_t *trans;
    wtk_fst_state_t *state = pre_tok->context_state;
    *context_score = 0.0;
    for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans) {
        if (trans->out_id == olabel) {
            tok->context_state = trans->to_state; // TODO
            tok->context_cost += trans->weight;
            *context_score = trans->weight;
            tok->tot_cost -= 5.0;
            return;
        }
    }
    tok->context_state = wtk_fst_net_get_load_state(dec->context_net, 0);
    tok->context_cost = 0.0;
}

float qtk_kwfstdec_process_emitting_state(qtk_kwfstdec_t* dec,wtk_vector_t *obs)
{
	//wtk_debug("=====================process emitting state\n");
	int frame = dec->active_tok.length - 1;
	//wtk_debug("=====================process emitting state %d\n",frame);
	qtk_hash_elem_t *final_toks = qtk_hash_list_clear(dec->tok);
	//wtk_debug("=====================process emitting get element: state %d\n",final_toks->key);
	qtk_hash_elem_t* best_elem = NULL, *elem, *e_tail;
	int tok_cnt, dnn_cnt, i;
	float adaptive_beam = FLT_MAX;
	float cur_cutoff;

	cur_cutoff = qtk_kwfstdec_get_cutoff2(dec, final_toks, &tok_cnt,
			&adaptive_beam, &best_elem);
        float laws_cutoff;
        // cur_cutoff=qtk_kwfstdec_get_cutoff(dec,final_toks,&tok_cnt,
        // &adaptive_beam, &best_elem); wtk_debug("=====================process
        // emitting: cur_cutoff:%f adaptive_beam:%f\n",cur_cutoff,adaptive_beam);
        // NEED RESIZE HASH???
        float next_cutoff = FLT_MAX;
	float tot_cost = FLT_MAX;
	float cost_offset = 0.0;
	float new_weight = FLT_MAX, likelihood, ac_cost, graph_cost, cur_cost;
	float lmscale = dec->cfg->lm_scale;
	//float acscale = dec->cfg->ac_scale;
	wtk_fst_state_t *state;
	wtk_fst_trans_t *trans;
	qtk_kwfstdec_token_t *token = NULL, *next_tok;
	qtk_kwfstdec_token_list_t*tok_list;
	//tok_list = qtk_hash_list_new_token_list(token);
	//wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));

	tok_list = qtk_hash_list_new_token_list(dec, token);
	wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
	dec->cur_token_list = tok_list;

	if (best_elem)
	{
		dec->best_elem = best_elem;
		//wtk_debug("=====================process emitting has best element: %d\n",best_elem->key);
		//int state_id=best_elem->key;
		qtk_kwfstdec_token_t *tok = best_elem->token;
		cost_offset = -tok->tot_cost;
		state = best_elem->state;//wtk_fst_net_get_load_state(dec->net,state_id);
		for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans)
		{
			if (trans->in_id != 0)
			{
				//dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);

				//likelihood=(*(obs+dnn_cnt+1));///10;
						///10;
				//wtk_debug("test: ac_cost=%f %d\n",likelihood,trans->in_id);
				//new_weight=tok->tot_cost-trans->weight+likelihood;
				if(dec->onnx_dec == 1)
				{
					dnn_cnt= trans->in_id-1;
					likelihood = (*(obs + dnn_cnt));
					new_weight = -trans->weight - likelihood*3.0;
				}else
				{
					dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);
					likelihood = (*(obs + dnn_cnt));
					new_weight = -trans->weight - likelihood;
				}
				if (new_weight + adaptive_beam < next_cutoff)
					next_cutoff = new_weight + adaptive_beam;
			}
			//miaomiaomiao?
		}
	}
	*(dec->cost_offset + dec->offset_count) = cost_offset;
	dec->offset_count++;
	//wtk_debug("process emitting next cutoff:%f\n",cost_offset);
	//wtk_debug("process element1\n");
	for (elem = final_toks; elem != NULL; elem = e_tail)
	{
		//wtk_debug("process element state:%d %p %p \n",elem->key,elem->token,elem);
		token = elem->token;
		state = elem->state;	//wtk_fst_net_get_load_state(dec->net,state_id);
		//wtk_debug("state=%d token tot_cost:%f cur_cutoff:%f\n",state->id, token->tot_cost,cur_cutoff);
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
//					wtk_debug("=======process emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
	//				dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);

					//likelihood=(*(obs+dnn_cnt+1));///10;
					//wtk_debug("%d likehood:%f\n",dnn_cnt+1,likelihood);
					//				wtk_debug("%d likehood:%f\n",trans->in_id,likelihood);
					if(dec->onnx_dec == 1)
					{
						dnn_cnt= trans->in_id-1;
						likelihood = (*(obs + dnn_cnt));
						ac_cost = cost_offset - likelihood*3.0;
					}else
					{
						dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);
						likelihood = (*(obs + dnn_cnt));
						ac_cost = cost_offset - likelihood;
					}
					graph_cost = -(trans->weight);
					cur_cost = token->tot_cost;
					
					tot_cost = cur_cost + ac_cost + graph_cost*lmscale;
					//tac_cost = token->ac_cost - likelihood;
					//wtk_debug("cost_offset:%f likehood:%f adaptive_beam:%f\n",cost_offset,likelihood,adaptive_beam);
					//wtk_debug("state:%d tot_cost:%f ac_cost:%f  graph_cost:%f  cur_cost:%f\n",state->id, tot_cost, ac_cost,graph_cost,cur_cost);
					//wtk_debug("tot_cost:%f  next_cutoff:%f\n",tot_cost,next_cutoff);
                                        laws_cutoff =
                                            next_cutoff - dec->cfg->laws_beam;
                                        if (tot_cost > next_cutoff)
                                            continue; //{wtk_debug("continue
                                                      //prune\n");continue;}
					else if (tot_cost + adaptive_beam < next_cutoff)
						next_cutoff = tot_cost + adaptive_beam;

                                        if (dec->cfg->use_laws_beam &&
                                            tot_cost >= laws_cutoff &&
                                            trans->to_state->load == 0)
                                            continue;
                                        //					wtk_debug("search
                                        //or add
                                        //state:%d\n",trans->to_state->id)
                                        next_tok = qtk_kwfstdec_find_or_add_token(dec,
							trans, frame + 1, tot_cost, NULL, token, likelihood);
                                        if (dec->context_net) {
                                            float context_score = 0;
                                            if (trans->out_id == 0) {
                                                next_tok->context_cost =
                                                    token->context_cost;
                                                next_tok->context_state =
                                                    token->context_state;
                                            } else {
                                                qtk_kwfstdec_process_context_state(
                                                    dec, token, next_tok,
                                                    trans->out_id,
                                                    &context_score);
                                            }

                                            token->link =
                                                qtk_hash_list_link_new(
                                                    dec->tok, next_tok,
                                                    trans->in_id, trans->out_id,
                                                    graph_cost - context_score,
                                                    ac_cost, token->link);
                                            token->link->context_score =
                                                context_score;
                                        } else {
                                            token->link =
                                                qtk_hash_list_link_new(
                                                    dec->tok, next_tok,
                                                    trans->in_id, trans->out_id,
                                                    graph_cost, ac_cost,
                                                    token->link);
                                        }
                                }
                        }
		}
		e_tail = elem->tail;
		if (e_tail)
		{
			qtk_hash_list_del(dec->tok, elem);
		}
	}
	return next_cutoff;
}

int qtk_kwfstdec_process_none_emitting_state(qtk_kwfstdec_t* dec, float cutoff) {
	//wtk_debug("============process none emitting state %f\n",cutoff);
	qtk_hash_elem_t *elem;
	st_q_t *st, *st1;
	qtk_kwfstdec_token_t *token;
	qtk_kwfstdec_token_t *new_token;
	wtk_queue_node_t *qn;
	wtk_fst_state_t *state;					//need free?
	wtk_fst_trans_t *trans;					//need free?
	float tot_cost = FLT_MAX;
	float cur_cost = FLT_MAX;
	float graph_cost;
	int frame = dec->active_tok.length - 2;
	float lmscale = dec->cfg->lm_scale;
	int changed = 0, i;
        float laws_cutoff = cutoff - dec->cfg->laws_beam;

        for (elem = dec->tok->list_head; elem != NULL; elem = elem->tail) {
            st = (st_q_t *)wtk_heap_malloc(dec->heap, sizeof(st_q_t));
            st->state_id = elem->key;
            st->state = elem->state;
            wtk_queue_push(&(dec->state_q), &(st->q_n));
        }
//	wtk_debug("============process none emitting queue len: %d\n",dec->state_q.length);
	if (dec->state_q.length == 0)
	{
		wtk_debug("warning!!!!!!!!!\n");
	}

	//token=qtk_hash_list_find(dec->tok,);
	while (1)
	{
		qn = wtk_queue_pop_back(&(dec->state_q));
		if (!qn)
		{
			break;
		}
		st = data_offset(qn, st_q_t, q_n);
		//wtk_debug("%d %p\n",st->state_id,qtk_hash_list_find(dec->tok,st->state_id));
		state = st->state;	//wtk_fst_net_get_load_state(dec->net,st->state_id);
		token = (qtk_kwfstdec_token_t*) state->hook;
		//token=(qtk_hash_list_find(dec->tok,st->state_id)->token);

		cur_cost = token->tot_cost;
		//wtk_debug("============process none emitting state:%d cur_cost:%f cutoff:%f\n",st->state_id,cur_cost,cutoff);
		if (cur_cost > cutoff)
			continue;
		qtk_hash_list_link_delete(token);
		token->link = NULL;
		//state=wtk_fst_net_get_load_state(dec->net,st->state_id);
		for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans)
		{
			//	if(!trans)
			//	{
			//		continue;
			//	}
			//wtk_debug("%p\n",trans);
			if (trans->in_id == 0)
			{
				//	if(!trans->to_state)
				//	{
				//		continue;
				//	}
				//wtk_debug("============process none emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
				graph_cost = -(trans->weight);
				tot_cost = cur_cost + graph_cost*lmscale;
				//wtk_debug("============process none emitting state:%d to:%d ilabel:%d olabel:%d weight:%f tot_cost:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight,tot_cost);
				//wtk_debug("tot_cost:%f cutoff:%f\n",tot_cost,cutoff);
				if (tot_cost < cutoff)
				{
                                    if (dec->cfg->use_laws_beam &&
                                        tot_cost >= laws_cutoff &&
                                        trans->to_state->load == 0) {
                                        continue;
                                    }
//					wtk_debug("process none emitting try add token\n");
					new_token = qtk_kwfstdec_find_or_add_token(dec,
							trans, frame + 1, tot_cost, &changed,
							token, 0.0);
                                        if (dec->context_net) {
                                        float context_score = 0;
                                        if (trans->out_id == 0) {
                                                new_token->context_cost =
                                                    token->context_cost;
                                                new_token->context_state =
                                                    token->context_state;
                                        } else {
                                                qtk_kwfstdec_process_context_state(
                                                    dec, token, new_token,
                                                    trans->out_id,
                                                    &context_score);
                                        }

                                            token->link =
                                                qtk_hash_list_link_new(
                                                    dec->tok, new_token, 0,
                                                    trans->out_id,
                                                    graph_cost - context_score,
                                                    0, token->link);
                                            token->link->context_score =
                                                context_score;
                                        } else {
                                            token->link =
                                                qtk_hash_list_link_new(
                                                    dec->tok, new_token, 0,
                                                    trans->out_id, graph_cost,
                                                    0, token->link);
                                        }
                                        if (changed) {
                                            st1 = (st_q_t *)wtk_heap_malloc(
                                                dec->heap, sizeof(st_q_t));
                                            st1->state_id = trans->to_state->id;
                                            st1->state = trans->to_state;
                                            wtk_queue_push(&(dec->state_q),
                                                           &(st1->q_n));
                                        }
                                }
			}
		}
	}

	return 0;
}

int isFiniteNumber(double d)
{
	return (d < FLT_MAX && d > -FLT_MAX);
}

void qtk_kwfstdec_prune_tokens(qtk_kwfstdec_t* dec,qtk_kwfstdec_token_list_t* link, int frame)
{
	if (!link)
		return;
	qtk_kwfstdec_token_t *token = link->token;
	qtk_kwfstdec_token_t *tok, *next_tok, *prev_tok = NULL;
	int count = 0;
	int flag = 0;
	for (tok = token; tok != NULL; tok = next_tok)
	{
		next_tok = tok->next;
		if (!isFiniteNumber(tok->extra_cost))
		{
			if (prev_tok != NULL)
				prev_tok->next = tok->next;
			else
				tok = tok->next;
			//  wtk_free(tok);
			dec->num_toks--;
			count++;
		} else {
			if (flag == 0)
			{
				flag = 1;
				link->token = tok;
			}
			//wtk_debug("not prune tok\n");
			prev_tok = tok;
		}
	}
	//wtk_debug("prune token num:%d %p\n",count,link->token);
}

void qtk_kwfstdec_prune_forward_links(qtk_kwfstdec_t* dec,
		qtk_kwfstdec_token_list_t* tlist, int *extra_costs_changed,
		int *links_pruned, float delta) {
	//wtk_debug("====prune forwardlink====\n");
	int changed = 1;
	qtk_kwfstdec_token_t* token, *next_token;
	qtk_kwfstdec_link_t *link, *prev_link, *next_link;
	float tok_extra_cost, link_extra_cost;

	*extra_costs_changed = 0;
	*links_pruned = 0;
	tok_extra_cost = FLT_MAX;	//??? may be cause error
	while (changed)
	{
//		wtk_debug("changed\n");
		changed = 0;
		for (token = tlist->token; token != NULL; token = token->next)
		{
//			wtk_debug("processing token:%f\n",token->tot_cost);
			tok_extra_cost = FLT_MAX;
			prev_link = NULL;
			for (link = token->link; link != NULL;) {
//				wtk_debug("processing link:%d %d\n",link->in_label,link->out_label);
				next_token = link->next_tok;
				link_extra_cost = next_token->extra_cost
						+ ((token->tot_cost + link->acoustic_cost
								+ link->graph_cost) - next_token->tot_cost);
//				wtk_debug("link_extra_cost:%f next_tok->extra_cost:%f tok->tot_cost:%f link->acoustic_cost:%f link->graph_cost:%f next_tok->tot_cost:%f\n",link_extra_cost,next_token->extra_cost,token->tot_cost,link->acoustic_cost,link->graph_cost,next_token->tot_cost);
				if (link_extra_cost > dec->cfg->lattice_beam)
				{
//					wtk_debug("prune link %d \n",jj);
					next_link = link->next;
					if (prev_link != NULL)
					{
						prev_link->next = next_link;
					} else
					{
						token->link = next_link;
					}
					//wtk_free(link);
					link = next_link;
					*links_pruned = 1;
				} else
				{
//					wtk_debug("not prune link\n");
					if (link_extra_cost < 0.0)
						link_extra_cost = 0.0;
					if (link_extra_cost < tok_extra_cost)
						tok_extra_cost = link_extra_cost;
					prev_link = link;  // move to next link
					link = link->next;
				}
			}
			//	wtk_debug("tok_extra_cost:%f extra_cost:%f fabs:%f delta:%f\n",tok_extra_cost,token->extra_cost,fabs(tok_extra_cost - token->extra_cost),delta);
			if (fabs(tok_extra_cost - token->extra_cost) > delta)
				changed = 1;
			//wtk_debug("%f\n",tok_extra_cost);
			token->extra_cost = tok_extra_cost;
		}
		if (changed)
			*extra_costs_changed = 1;
	}
//	wtk_debug("final prune:%d not:%d total:%d changed:%d \n",jj,oo,qq,cg);
}

void qtk_kwfstdec_prune_active_tokens(qtk_kwfstdec_t* dec, float delta)
{
	//wtk_debug("=======prune active token=======\n");
	int cur_frame = dec->active_tok.length - 1;  //???
	//int num_toks_begin=dec->num_toks;
	int i, extra_costs_changed = 0, links_pruned = 0;
	qtk_kwfstdec_token_list_t* tlist, *pre_tlist, *next_tlist;
	wtk_queue_node_t *qn;

	for (i = cur_frame - 1; i >= 0; i--)
	{
		qn = wtk_queue_peek(&(dec->active_tok), i);
		tlist = (qtk_kwfstdec_token_list_t*) data_offset(qn,
				qtk_kwfstdec_token_list_t, q_n);
		if (tlist->must_prune_forward_links)
		{
			//		wtk_debug("prune link:%d\n",i);
			qtk_kwfstdec_prune_forward_links(dec, tlist, &extra_costs_changed,
					&links_pruned, delta);
			if (extra_costs_changed && i > 1)
			{
				qn = wtk_queue_peek(&(dec->active_tok), i - 1);
				pre_tlist = (qtk_kwfstdec_token_list_t*) data_offset(qn,
						qtk_kwfstdec_token_list_t, q_n);
				pre_tlist->must_prune_forward_links = 1;
			}
			if (links_pruned)
			{
				tlist->must_prune_tokens = 1;
			}
			tlist->must_prune_forward_links = 0;
		}
		qn = wtk_queue_peek(&(dec->active_tok), i + 1);
		next_tlist = (qtk_kwfstdec_token_list_t*) data_offset(qn,
				qtk_kwfstdec_token_list_t, q_n);
		next_tlist->must_prune_forward_links = 1;
		if (i + 1 < cur_frame && next_tlist->must_prune_tokens)
		{
			//		wtk_debug("prune token:%d\n",i+1);
			qtk_kwfstdec_prune_tokens(dec, next_tlist, 0);
			next_tlist->must_prune_tokens = 0;
		}
	}
}

void qtk_kwfstdec_compute_final_cost(qtk_kwfstdec_t* dec)
{
	qtk_hash_elem_t *elem, *next;
	qtk_kwfstdec_token_t *token;
	final_cost_q_t *fc;
	float best_cost_with_final = FLT_MAX;
	float best_cost = FLT_MAX;
	float final_cost, cost, cost_with_final;
	wtk_fst_state_t *state;
	dec->best_weight = FLT_MAX;
	//wtk_debug("compute final cost\n");
	//elem = qtk_hash_list_clear(dec->tok);
	elem = qtk_hash_list_clear3(dec->tok);
	while (elem != NULL)
	{
		token = elem->token;
		next = elem->tail;
		state = elem->state;//wtk_fst_net_get_load_state(dec->net,elem->key);
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
		if (isFiniteNumber(final_cost))
		{
			fc = (final_cost_q_t*) wtk_heap_malloc(dec->heap,
					sizeof(final_cost_q_t));
			//fc=(final_cost_q_t*)wtk_malloc(sizeof(final_cost_q_t));
			fc->cost = final_cost;
			fc->token = token;
			fc->state = state->id;
			wtk_queue_push(&(dec->final_cost_q), &(fc->q_n));
			cost += final_cost;
			if (cost < dec->best_weight)
			{
				dec->best_weight = cost;
				dec->best_token = token;
				dec->best_final_cost = final_cost;
			}
			//wtk_debug("%p state->id:%d push best:%f cost=%f\n",token,state->id,final_cost, cost);
		}
		elem = next;
	}

	if (isFiniteNumber(best_cost) && isFiniteNumber(best_cost_with_final))
		dec->final_relative_cost = FLT_MAX;
	else
		dec->final_relative_cost = best_cost_with_final - best_cost;

	if (isFiniteNumber(best_cost_with_final))
		dec->final_best_cost = best_cost_with_final;
	else
		dec->final_best_cost = best_cost;
}

int qtk_kwfstdec_tok_cmp(qtk_kwfstdec_token_t *tok, final_cost_q_t *fc)
{
	if (tok == fc->token)
	{
		return 0;
	} else
	{
		return -1;
	}
}

int qtk_kwfstdec_tmap_cmp(qtk_kwfstdec_token_t *tok, token_map_t *fc)
{
	if (tok == fc->token)
	{
		return 0;
	} else
	{
		return -1;
	}
}

int ApproxEqual(float a, float b, float relative_tolerance)
{
	// a==b handles infinities.
	if (a == b)
		return 1;
	float diff = abs((int)(a - b));
	if (isFiniteNumber(diff) || diff != diff)
		return 0;  // diff is +inf or nan.
	return (diff <= relative_tolerance * (abs((int)a) + abs((int)b)));
}

int qtk_kwfstdec_prune_forword_link_final(qtk_kwfstdec_t* dec)
{
	//wtk_debug("prune forword link final\n");
	int changed = 1;
	float delta = 0.00005;
	float final_cost;
	qtk_kwfstdec_token_t* token, *next_token;
	qtk_kwfstdec_link_t *link, *prev_link, *next_link;
	float tok_extra_cost, link_extra_cost;
	qtk_kwfstdec_token_list_t* tlist;
	wtk_queue_node_t *qn;
	final_cost_q_t* fc;

	qtk_kwfstdec_compute_final_cost(dec);

	//wtk_debug("prune link final for frame %d",dec->active_tok.length-1);
	qn = wtk_queue_peek(&(dec->active_tok), dec->active_tok.length - 1);
	tlist = (qtk_kwfstdec_token_list_t*) data_offset(qn,
			qtk_kwfstdec_token_list_t, q_n);
	if (!tlist)
	{
		return -1;
	}
	while (changed)
	{
//		wtk_debug("changed\n");
		changed = 0;
		//wtk_debug("%p\n",tlist);
		for (token = tlist->token; token != NULL; token = token->next)
		{//wtk_debug("%p token_tot:%f\n",token,token->tot_cost);
			prev_link = NULL;
			if (dec->final_cost_q.length == 0)
			{
//				wtk_debug("final_cost:0\n");
				final_cost = 0.0;
			} else
			{
				//phone=wtk_queue_find(&(t_model->entry_q),offsetof(qtk_trans_phone_t,q_n),(wtk_cmp_handler_t)qtk_trans_phone_cmp,&(triple->phone_id));
				fc = (final_cost_q_t*) wtk_queue_find(&(dec->final_cost_q),
						offsetof(final_cost_q_t, q_n),
						(wtk_cmp_handler_t) qtk_kwfstdec_tok_cmp, token);
				if (fc)
				{
					////wtk_debug("find token\n");
					final_cost = fc->cost;
				} else
				{
					////wtk_debug("not find\n");
					final_cost = FLT_MAX;
				}
			}
			tok_extra_cost = token->tot_cost + final_cost
					- dec->final_best_cost;
//			wtk_debug("tok_extra_cost:%f token->tot_cost:%f final_cost:%f dec->final_best_cost:%f\n",tok_extra_cost,token->tot_cost,final_cost,dec->final_best_cost);
			for (link = token->link; link != NULL;)
			{
//				wtk_debug("process link:%d %d\n",link->in_label,link->out_label);
				next_token = link->next_tok;
				link_extra_cost = next_token->extra_cost
						+ ((token->tot_cost + link->acoustic_cost
								+ link->graph_cost) - next_token->tot_cost);
//				wtk_debug("===link_extra_cost:%f next_token->extra_cost:%f token->tot_cost:%f link->acoustic_cost:%f link->graph_cost:%f next_token->tot_cost:%f\n",link_extra_cost,next_token->extra_cost,token->tot_cost,link->acoustic_cost,link->graph_cost,next_token->tot_cost);
				if (link_extra_cost > dec->cfg->lattice_beam)
				{
					next_link = link->next;
					if (prev_link != NULL)
						prev_link->next = next_link;
					else
						token->link = next_link;
					//free(link);
					link = next_link;
				} else {
					if (link_extra_cost < 0.0)
					{
						link_extra_cost = 0.0;
					}
					if (link_extra_cost < tok_extra_cost)
						tok_extra_cost = link_extra_cost;
					prev_link = link;
					link = link->next;
				}
			}
			if (tok_extra_cost > dec->cfg->lattice_beam)
				tok_extra_cost = FLT_MAX;
//			wtk_debug("%f %f %f\n",token->extra_cost,tok_extra_cost,delta);
			if (!ApproxEqual(token->extra_cost, tok_extra_cost, delta))	//pay attention TODO
				changed = 1;
//			{	changed=1;wtk_debug("cccccccc\n");}
			//	wtk_debug("extra_cost:%f\n",tok_extra_cost);
			token->extra_cost = tok_extra_cost;
		}
	}
	//wtk_debug("prune link final:%d\n for frame %d",xx,dec->active_tok.length-1);
	return 0;
}

int qtk_kwfstdec_finalize_decoding(qtk_kwfstdec_t* dec)
{
	int ret = 0;
	int frame;
	int extra_costs_changed = 0, links_pruned = 0;
	float delta = 0.0;
	qtk_kwfstdec_token_list_t *tlist, *tlist2;
	wtk_queue_node_t *qn;

	ret = qtk_kwfstdec_prune_forword_link_final(dec);
	if (ret != 0)
	{
		return ret;
	}
	for (frame = dec->cur_frame; frame >= 0; frame--)
	{
		//	wtk_debug("prune frame:%d\n",frame);
		qn = wtk_queue_peek(&(dec->active_tok), frame);
		tlist = (qtk_kwfstdec_token_list_t*) data_offset(qn,
				qtk_kwfstdec_token_list_t, q_n);
		qn = wtk_queue_peek(&(dec->active_tok), frame + 1);
		tlist2 = (qtk_kwfstdec_token_list_t*) data_offset(qn,
				qtk_kwfstdec_token_list_t, q_n);
		qtk_kwfstdec_prune_forward_links(dec, tlist, &extra_costs_changed,
				&links_pruned, delta);
		qtk_kwfstdec_prune_tokens(dec, tlist2, frame + 1);
	}
	qn = wtk_queue_peek(&(dec->active_tok), 0);
	tlist = (qtk_kwfstdec_token_list_t*) data_offset(qn,
			qtk_kwfstdec_token_list_t, q_n);
	qtk_kwfstdec_prune_tokens(dec, tlist, 0);

	return 0;
}
//int ccc=1;
int qtk_kwfstdec_feed(qtk_kwfstdec_t *dec, wtk_feat_t *f)
{
	float cost_cutoff;
//	wtk_debug("==========feed frame %d=============\n",ccc);
//	ccc++;
	//wtk_vector_print(f->rv);
	dec->cur_frame = f->index;
//	if(index%dec->cfg->prune_interval==0)
//		qtk_kwfstdec_prune_active_tokens(dec,dec->cfg->beam*dec->cfg->prune_scale);
	cost_cutoff = qtk_kwfstdec_process_emitting_state(dec, f->rv);
	qtk_kwfstdec_process_none_emitting_state(dec, cost_cutoff);
//	wtk_feat_push_back(f);
	return 0;
}
int qtk_kwfstdec_feed2(qtk_kwfstdec_t* dec, wtk_vector_t *v, int index)
{
	float *v2;
	v2 = v;
	if (dec->cfg->add_softmax)
	{
		v2 = (float*) wtk_malloc(sizeof(float) * 3045);
		memcpy(v2, v, sizeof(float) * 3045);
		wtk_softmax(v2, 3045);
		wtk_nnet3_log(v2, 3045);
	}
	float cost_cutoff;
	//wtk_debug("==========feed frame %d=============\n", index);
	dec->cur_frame = index;
	if (index % dec->cfg->prune_interval == 0)
		qtk_kwfstdec_prune_active_tokens(dec,dec->cfg->beam * dec->cfg->prune_scale);
	cost_cutoff = qtk_kwfstdec_process_emitting_state(dec, v2);
	qtk_kwfstdec_process_none_emitting_state(dec, cost_cutoff);
	if (dec->cfg->add_softmax)
	{
		wtk_free(v2);
	}

	return 0;
}

void qtk_kwfstdec_best_path_end(qtk_kwfstdec_t *dec);

int qtk_kwfstdec_state_queue_cmp(wtk_queue_node_t *qn1, wtk_queue_node_t *qn2)
{
	token_map_t *tok1, *tok2;
	tok1 = NULL;
	tok2 = NULL;

	tok1 = data_offset2(qn1, token_map_t, q_n);
	tok2 = data_offset2(qn2, token_map_t, q_n);
	return (tok1->stateID - tok2->stateID) < 0 ? 1 : -1;
}
void qtk_kwfstdec_top_sort_tok(qtk_kwfstdec_t *dec, qtk_kwfstdec_token_t* token)
{
	wtk_queue_t token2pos;
	wtk_queue_t reprocess;
	wtk_queue_t reprocess_vec;
	qtk_kwfstdec_token_t* tok;
	qtk_kwfstdec_link_t *link;
	token_map_t* repro;
	token_map_t* t2p;
	token_map_t* tmp = NULL;
	token_map_t* tmp2 = NULL;
	wtk_queue_node_t *qn;
	int num_toks = 0;
	int cur_pos = 0;
	int pos = 0;
	int next_pos = 0;

	wtk_queue_init(&(token2pos));
	wtk_queue_init(&(reprocess));
	wtk_queue_init(&(reprocess_vec));

	for (tok = token; tok != NULL; tok = tok->next)
	{
		//wtk_debug("%p\n",tok);
		num_toks++;
	}
	//wtk_debug("hell1:%d\n",num_toks)
	for (tok = token; tok != NULL; tok = tok->next)
	{
		pos = num_toks - ++cur_pos;
		t2p = (token_map_t*) wtk_heap_malloc(dec->heap, sizeof(token_map_t));
		t2p->stateID = pos;
		t2p->token = tok;
		wtk_queue_push_front(&(token2pos), &(t2p->q_n));
	}

	for (qn = token2pos.pop; qn; qn = qn->next)
	{
		t2p = (token_map_t*) data_offset(qn, token_map_t, q_n);
		pos = t2p->stateID;
		tok = t2p->token;
		for (link = tok->link; link != NULL; link = link->next)
		{
			if (link->in_label == 0)
			{
				tmp = (token_map_t*) wtk_queue_find(&(token2pos),
						offsetof(token_map_t, q_n),
						(wtk_cmp_handler_t) qtk_kwfstdec_tmap_cmp,
						link->next_tok);

				if (tmp)
				{
					next_pos = tmp->stateID;
					if (next_pos < pos)
					{
						tmp->stateID = cur_pos++;
						repro = (token_map_t*) wtk_heap_malloc(dec->heap,
								sizeof(token_map_t));
						repro->token = link->next_tok;
						wtk_queue_push(&(reprocess), &(repro->q_n));
					}
				}
			}
		}
		tmp2 = (token_map_t*) wtk_queue_find(&(reprocess),
				offsetof(token_map_t, q_n),
				(wtk_cmp_handler_t) qtk_kwfstdec_tmap_cmp, tok);
		if (tmp2 != NULL)
		{
			wtk_queue_remove(&reprocess, &(tmp2->q_n));
		}
	}

	int max_loop = 1000000;
	int loop_count = 0;
	for (loop_count = 0; reprocess.length != 0 && loop_count < max_loop;++loop_count)
	{
		wtk_queue_init(&(reprocess_vec));
		for (qn = reprocess.pop; qn; qn = qn->next)
		{
			t2p = (token_map_t*) data_offset(qn, token_map_t, q_n);
			wtk_queue_push(&(reprocess_vec), &(t2p->q_n));
		}
		wtk_queue_init(&(reprocess));
		for (qn = reprocess_vec.pop; qn; qn = qn->next)
		{
			tmp = (token_map_t*) data_offset(qn, token_map_t, q_n);
			tok = tmp->token;
			tmp2 = (token_map_t*) wtk_queue_find(&(token2pos),
					offsetof(token_map_t, q_n),
					(wtk_cmp_handler_t) qtk_kwfstdec_tmap_cmp, tok);
			pos = tmp2->stateID;

			for (link = tok->link; link != NULL; link = link->next)
			{
				if (link->in_label == 0)
				{
					tmp2 = (token_map_t*) wtk_queue_find(&(token2pos),
							offsetof(token_map_t, q_n),
							(wtk_cmp_handler_t) qtk_kwfstdec_tmap_cmp,
							link->next_tok);

					if (tmp2)
					{
						next_pos = tmp2->stateID;
						if (next_pos < pos) 
						{
							tmp2->stateID = cur_pos++;
							repro = (token_map_t*) wtk_heap_malloc(dec->heap,
									sizeof(token_map_t));
							repro->token = link->next_tok;
							wtk_queue_push(&(reprocess), &(repro->q_n));
						}
					}
				}
			}
		}
	}

	wtk_queue_init(&dec->topsorted_list_q);
	for (qn = token2pos.pop; qn; qn = qn->next)
	{
		t2p = (token_map_t*) data_offset(qn, token_map_t, q_n);
		repro = (token_map_t*) wtk_heap_malloc(dec->heap, sizeof(token_map_t));
		repro->token = t2p->token;
		repro->stateID = t2p->stateID;
		//wtk_queue_push(&(reprocess), &(repro->q_n));
		wtk_queue_sort_insert(&(dec->topsorted_list_q), &(repro->q_n),
				(wtk_queue_node_cmp_f) qtk_kwfstdec_state_queue_cmp);//TODO!!!!! dont forget wtk_queue
	}

}
/*
 void qtk_kwfstdec_gen_lat(qtk_kwfstdec_t *dec)
 {
 int f=0;
 int next_stateID,stateID;
 qtk_kwfstdec_token_list_t *list,*map_list;
 qtk_kwfstdec_token_t *token;
 qtk_kwfstdec_link_t *link;
 wtk_queue_node_t *qn;
 wtk_queue_node_t *qn2;
 token_map_t  *t_map,*next_tmap;
 token_map_t *t2p=NULL;
 qtk_lat_arc_t* arc;
 final_cost_q_t* fc;

 //vector_t* tok_list=NULL;
 //type_register(qtk_kwfstdec_token_t, NULL, NULL, NULL, NULL);
 //tok_list=create_vector(qtk_kwfstdec_token_t);
 //vector_init(tok_list);
 int i;
 int handle;
 handle=dec->handle;
 //GetInstance(&handle);
 qtk_fsthelper_delete_state(handle);

 for(qn=dec->active_tok.pop;qn;qn=qn->next)
 {
 list=(qtk_kwfstdec_token_list_t*)data_offset(qn,qtk_kwfstdec_token_list_t,q_n);
 qtk_kwfstdec_top_sort_tok(dec,list->token);
 //qtk_kwfstdec_top_sort_tok(dec,list->token,dec->lat_token_q);
 //size=vector_size(tok_list);
 //wtk_debug("%d\n",dec->topsorted_list_q.length);
 for(qn2=dec->topsorted_list_q.pop;qn2;qn2=qn2->next)
 {
 t2p=(token_map_t*)data_offset(qn2,token_map_t,q_n);
 token=t2p->token;
 t_map=(token_map_t*)wtk_heap_malloc(dec->heap,sizeof(token_map_t));
 t_map->token=token;
 t_map->stateID=qtk_fsthelper_add_state(handle);
 //	wtk_debug("add state:totcost:%f extra_cost:%f stateid:%d\n",token->tot_cost,token->extra_cost,t_map->stateID);
 //	wtk_debug("push:%p\n",token);
 wtk_queue_push(&(dec->token_map_q), &(t_map->q_n));
 }
 }
 //	for(qn=dec->lat_token_q.pop;qn;qn=qn->next)
 //	{
 //		map_list=(qtk_kwfstdec_token_list_t*)data_offset(qn,qtk_kwfstdec_token_list_t,q_n);
 //		t_map=(token_map_t*)wtk_heap_malloc(dec->heap,sizeof(token_map_t));
 //		qtk_lat_add_state(dec->lat,&(t_map->stateID));
 //		wtk_queue_push(&(dec->token_map_q), &(t_map->q_n));
 //	}
 float weight=0.0;
 float cost_offset=0.0;
 int xx=0;

 qtk_fsthelper_set_start(handle);

 for(qn=dec->active_tok.pop;qn;qn=qn->next)
 {
 list=(qtk_kwfstdec_token_list_t*)data_offset(qn,qtk_kwfstdec_token_list_t,q_n);
 for(token=list->token;token;token=token->next)
 {
 //	wtk_debug("find:%p\n",token);
 t_map=(token_map_t*)wtk_queue_find(&(dec->token_map_q),offsetof(token_map_t,q_n),(wtk_cmp_handler_t)qtk_kwfstdec_tmap_cmp,token);
 stateID=t_map->stateID;
 for(link=token->link;link;link=link->next)
 {
 //	wtk_debug("find:%p\n",link->next_tok);
 next_tmap=(token_map_t*)wtk_queue_find(&(dec->token_map_q),offsetof(token_map_t,q_n),(wtk_cmp_handler_t)qtk_kwfstdec_tmap_cmp,link->next_tok);
 next_stateID=next_tmap->stateID;

 if(link->in_label!=0)
 {
 //fc=(final_cost_q_t*)wtk_queue_find(&(dec->final_cost_q),offsetof(final_cost_q_t,q_n),(wtk_cmp_handler_t)qtk_kwfstdec_tok_cmp,tok);

 cost_offset=*(dec->cost_offset+xx);
 }
 weight=link->graph_cost+link->acoustic_cost-cost_offset;
 //arc=qtk_lat_new_arc(dec->lat,link->in_label,link->out_label,weight,next_stateID);
 //wtk_debug("arc form:%d to %d in:%d out:%d weight:%f %f\n",stateID,next_stateID,link->in_label,link->out_label,link->graph_cost,link->acoustic_cost-cost_offset);
 qtk_fsthelper_add_arc(handle,stateID,link->in_label,link->out_label,link->graph_cost,link->acoustic_cost,cost_offset,next_stateID);
 }
 if(qn->next==NULL)
 {
 if(dec->final_cost_q.length!=0)
 {
 //wtk_debug("woc! %p\n",token);
 fc=(final_cost_q_t*)wtk_queue_find(&(dec->final_cost_q),offsetof(final_cost_q_t,q_n),(wtk_cmp_handler_t)qtk_kwfstdec_tok_cmp,token);
 if(fc)
 {
 qtk_fsthelper_set_final(handle,stateID,fc->cost);
 wtk_debug("set final:%d\n",stateID);
 }
 }
 }
 }
 //			wtk_debug("coffset:%f\n",cost_offset);
 xx++;
 }
 qtk_fsthelper_write_fst(handle);
 }*/
//generate net3
void qtk_kwfstdec_gen_lat2(qtk_kwfstdec_t *dec)
{
	int next_stateID, stateID;
	qtk_kwfstdec_token_list_t *list;
	qtk_kwfstdec_token_t *token;
	qtk_kwfstdec_link_t *link;
	wtk_queue_node_t *qn;
	wtk_queue_node_t *qn2;
	//token_map_t *t_map, *next_tmap;
	token_map_t *t2p = NULL;
	//qtk_lat_arc_t* arc;
	final_cost_q_t* fc;
	wtk_fst_net3_t *lat_net = dec->lat_net;

	wtk_fst_node_t *node, *from_node, *final_node;

	int node_num = 0;
	int first = 1;
	node = wtk_fst_net3_pop_node2(lat_net, 0, 0, 0, node_num); //frame,trans,state
//	wtk_debug("pop node:%p %d\n",node,node_num);
	lat_net->start = node;
	//wtk_debug("11111 %d\n",dec->active_tok.length);
	for (qn = dec->active_tok.pop; qn; qn = qn->next)
	{
		list = (qtk_kwfstdec_token_list_t*) data_offset(qn,qtk_kwfstdec_token_list_t, q_n);
		qtk_kwfstdec_top_sort_tok(dec, list->token);
		//wtk_debug("22222  %d\n",dec->topsorted_list_q.length);
		for (qn2 = dec->topsorted_list_q.pop; qn2; qn2 = qn2->next)
		{
			t2p = (token_map_t*) data_offset(qn2, token_map_t, q_n);
			token = t2p->token;
			//t_map = (token_map_t*) wtk_heap_malloc(dec->heap,
			//		sizeof(token_map_t));
			//t_map->token = token;
			node_num++;
			//t_map->stateID = node_num;
			token->state = node_num;
			//wtk_debug("stateID:%d\n",t_map->stateID);
			node = wtk_fst_net3_pop_node2(lat_net, 0, 0, 0, node_num);//frame,trans,state
			if (first == 1)
			{
				first = 0;
				wtk_fst_net3_pop_arc2(lat_net, lat_net->start, node, 0, 2, 0.0,0.0, 0.0, 0, 0);
				//wtk_debug("pop arc:frome %p %d to %p %d\n",lat_net->start,lat_net->start->num,node,node->num);	
			}
			//qtk_fsthelper_add_state(handle);
			//	wtk_debug("add state:totcost:%f extra_cost:%f stateid:%d\n",token->tot_cost,token->extra_cost,t_map->stateID);
			//	wtk_debug("push:%p\n",token);
			//wtk_queue_push(&(dec->token_map_q), &(t_map->q_n));
		}
	}
	//wtk_debug("3333 %d %d",node_num,dec->cur_frame);
	node_num++;
	dec->lat_support = (wtk_fst_node_t**)wtk_calloc(node_num+1,sizeof(wtk_fst_node_t*));
	final_node = wtk_fst_net3_pop_node2(lat_net, 0, 0, 0, node_num);

	
	wtk_fst_node_t* tmp_node;
	int tmp_cnt=0;
	for (qn = lat_net->active_node_q.pop; qn; qn = qn->next)
	{
		tmp_node = (wtk_fst_node_t*) data_offset(qn,
                wtk_fst_node_t, q_n);
		dec->lat_support[tmp_cnt] = tmp_node;
		tmp_cnt++;
	}
//	nil_node=wtk_fst_net3_pop_node2(lat_net,0,0,0,node_num);
//	arc=wtk_fst_net3_pop_arc2(lat_net,final_node,nil_node,0,0.0,0.0,0.0,0);
//	wtk_debug("pop node:%p %d\n",final_node,node_num);
	lat_net->end = final_node;
	lat_net->null_node = final_node;
//	lat_net->null_prev_node=final_node;
	wtk_fst_node_set_eof(final_node, 1);
	float cost_offset = 0.0;
	int xx = 0;
	int frame = 0;

	for (qn = dec->active_tok.pop; qn; qn = qn->next)
	{
		list = (qtk_kwfstdec_token_list_t*) data_offset(qn,
				qtk_kwfstdec_token_list_t, q_n);
		//wtk_debug("????? %p\n",list->token);
		for (token = list->token; token; token = token->next)
		{
			//wtk_debug("find:%p\n",token);
			//t_map = (token_map_t*) wtk_queue_find(&(dec->token_map_q),
			//		offsetof(token_map_t, q_n),
			//		(wtk_cmp_handler_t) qtk_kwfstdec_tmap_cmp, token);
			//stateID = t_map->stateID;
			stateID = token->state;
			for (link = token->link; link; link = link->next)
			{
				//wtk_debug("find:%p\n",link->next_tok);
				//next_tmap = (token_map_t*) wtk_queue_find(&(dec->token_map_q),
				//		offsetof(token_map_t, q_n),
				//		(wtk_cmp_handler_t) qtk_kwfstdec_tmap_cmp,
				//		link->next_tok);
				//next_stateID = next_tmap->stateID;
				next_stateID = link->next_tok->state;
				cost_offset = 0.0;
				if (link->in_label != 0)
				{
					//fc=(final_cost_q_t*)wtk_queue_find(&(dec->final_cost_q),offsetof(final_cost_q_t,q_n),(wtk_cmp_handler_t)qtk_kwfstdec_tok_cmp,tok);

					cost_offset = *(dec->cost_offset + xx);
				}
				//weight=link->graph_cost+link->acoustic_cost-cost_offset;
				//arc=qtk_lat_new_arc(dec->lat,link->in_label,link->out_label,weight,next_stateID);
				//wtk_debug("arc form:%d to %d in:%d out:%d weight:%f %f\n",stateID,next_stateID,link->in_label,link->out_label,link->graph_cost,link->acoustic_cost-cost_offset);
				//qn2 = wtk_queue_peek(&(lat_net->active_node_q),
				//		node_num - stateID);
				//from_node = (wtk_fst_node_t*) data_offset(qn2, wtk_fst_node_t,
				//		q_n);
				from_node = dec->lat_support[node_num - stateID];
				//wtk_debug("node:%p %p\n",from_node,tmp_node);
				//qn2 = wtk_queue_peek(&(lat_net->active_node_q),
				//		node_num - next_stateID);
				//node = (wtk_fst_node_t*) data_offset(qn2, wtk_fst_node_t, q_n);
				node = dec->lat_support[node_num - next_stateID];
				//arc=wtk_fst_net3_pop_arc2(lat_net,from_node,node,link->out_label,0.0,-link->acoustic_cost+cost_offset,0.0,0);//TODO from node and to node
				wtk_fst_net3_pop_arc2(lat_net, from_node, node, link->in_label, link->out_label,
						0.0, -link->acoustic_cost + cost_offset,
						link->graph_cost, frame, 0);	//TODO from node and to node
				//arc=wtk_fst_net3_pop_arc2(lat_net,from_node,node,link->out_label,0.0,-link->acoustic_cost+cost_offset,0.0,0,0);//TODO from node and to node
				//last_id=node_num-stateID;
				//wtk_debug("pop arc:frome %p %d to %p %d\n",from_node,from_node->num,node,node->num);
				//wtk_debug("pop arc:from %d to %d %f %f ilabel:%d olabel:%d \n",from_node->num,node->num,link->graph_cost,link->acoustic_cost-cost_offset,link->in_label,link->out_label);
				//qtk_fsthelper_add_arc(handle,stateID,link->in_label,link->out_label,link->graph_cost,link->acoustic_cost,cost_offset,next_stateID);
			}
			if (qn->next == NULL)
			{
				if (dec->final_cost_q.length != 0)
				{
					//wtk_debug("woc! %p\n",token);
					fc = (final_cost_q_t*) wtk_queue_find(&(dec->final_cost_q),
							offsetof(final_cost_q_t, q_n),
							(wtk_cmp_handler_t) qtk_kwfstdec_tok_cmp, token);
					if (fc)
					{
						//qtk_fsthelper_set_final(handle,stateID,fc->cost);
						//qn2 = wtk_queue_peek(&(lat_net->active_node_q),
						//		node_num - stateID);
						//node = (wtk_fst_node_t*) data_offset(qn2,
						//		wtk_fst_node_t, q_n);
						node = dec->lat_support[node_num - stateID];
						wtk_fst_net3_pop_arc2(lat_net, node, final_node,0, 1, 0.0,
								0.0, fc->cost, frame, 0);//TODO from node and to node
						//arc=wtk_fst_net3_pop_arc2(lat_net,node,final_node,1,0.0,0.0,0.0,0,0);//TODO from node and to node
						//wtk_debug("pop arc:frome %p %d to %p %d\n",node,node->num,final_node,final_node->num);

						//wtk_debug("set final:%d %f\n",node->num,fc->cost);
					}
				}
			}
		}
		//			wtk_debug("coffset:%f\n",cost_offset);
		xx++;
		frame++;
	}
	//qtk_fsthelper_write_fst(handle);
}

void qtk_kwfstdec_add_extra_pth(qtk_kwfstdec_t* dec)
{
	wtk_queue_node_t *qn, *qn2, *qn3;
	wtk_str_hash_t* hash;
	hash_str_node_t *node, *node2;
	wtk_str_hash_it_t it;
	qtk_kwfstdec_classes_t *class;
	wtk_fst_net3_combine_t *combine;
	hash_str_node_t **nodes;
	int node_num = -1;
	float max_like = 0.0;

	nodes = (hash_str_node_t**) wtk_malloc(100 * sizeof(hash_str_node_t*));
	int i, j, out_id = 2;
	float strlike = 0;
	hash = dec->lat_net->outsym_hash;
	//wtk_chnlike_reset(dec->chnlike); //TODO is nesscessary?
	for (qn = dec->hot_words.pop; qn; qn = qn->next)
	{
		class = (qtk_kwfstdec_classes_t*) data_offset(qn,
				qtk_kwfstdec_classes_t, q_n);
		//strs = (wtk_string_t**) class->word->slot;
		for (i = 1; i < class->word->nslot; i++)
		{
			out_id++;
			it = wtk_str_hash_iterator(hash);
			//v = strs[i];
			max_like = 0.0;
			node_num = -1;
			//wtk_debug("============== %.*s \n",v->len,v->data);
			while (1)
			{
				qn3 = it.cur_n;
				if (!qn3)
				{
					break;
				}
				it.cur_n = 0;
				wtk_str_hash_it_move(&(it));
				//node=wtk_str_hash_it_next(&(it));
				node = (hash_str_node_t*) data_offset(qn3, hash_str_node_t, n);
				//wtk_debug("%.*s %.*s\n",node->key.len,node->key.data,v->len,v->data);
				//wtk_debug("%.*s \n",v->len,v->data);
				strlike = 0.0;//wtk_chnlike_like(dec->chnlike, node->key.data,
						//node->key.len, v->data, v->len, NULL);
				//wtk_debug("%f\n",strlike);
				if (strlike >= dec->cfg->hot_thresh)
				{

					if (strlike > max_like)
					{
						node_num = 0;
						nodes[node_num] = node;
						max_like = strlike;
					} else if (strlike == max_like)
					{
						node_num++;
						nodes[node_num] = node;
					}
				}
			}
			//wtk_debug("node_num:%d\n",node_num);
			for (j = 0; j <= node_num; j++)
			{
				for (qn2 = &(nodes[j]->n); qn2; qn2 = qn2->next)
				{
					node2 = (hash_str_node_t*) data_offset(qn2, hash_str_node_t,n);
					combine = (wtk_fst_net3_combine_t*) node2->value;
					//wtk_debug("pop arc:%d to %d ac_cost:%f lm_like:%f  %d\n",
					//		combine->start->num, combine->end->num,
					//		combine->ac_cost, combine->lm_like, out_id);
					//wtk_fst_net3_pop_arc2(dec->lat_net,combine->start,combine->end,0,combine->ac_cost,combine->lm_like,0.0,0,1);//TODO need out id and weight
					wtk_fst_net3_pop_arc2(dec->lat_net, combine->start,
							combine->end, 0, out_id, combine->ac_like,
							combine->lm_like * 5.0, 0.0, 0, 1);	//TODO need out id and weight
				}
			}
		}
	}
	wtk_free(nodes);
}

void qtk_kwfstdec_get_hint_result(qtk_kwfstdec_t* dec, wtk_strbuf_t *buf)
{
	wtk_fst_sym_t *sym_out;
	qtk_kwfstdec_token_t *token, *best_token = NULL;
	wtk_string_t *v;
	sym_out = dec->net->cfg->sym_out;

	//qtk_kwfstdec_compute_final_cost(dec);
	//best_token=dec->best_token;
	if (dec->best_elem)
	{
		best_token = dec->best_elem->token;
	}
	if (best_token)
	{
		dec->conf = dec->best_weight / dec->cur_frame;

		if (best_token->pth != NULL)
		{
			for (token = best_token; token;)
			{
				if (token->pth)
				{
					if (token->pth->out_label != 0)
					{
						v = sym_out->strs[token->pth->out_label];
						wtk_strbuf_push_front(buf, v->data, v->len);
						wtk_strbuf_push_front(buf, " ", 1);
					}
					if (token->pth->lbest)
					{
						token = token->pth->lbest;
					}
				} else
				{
					token = NULL;
				}
			}
		}
	}
}

int qtk_kwfstdec_get_sym_cnt(wtk_string_t *v){
	int cnt = 0,wrd_cnt = 0;
	char *s = v->data;
	char *e = v->data + v->len;

	while(s < e){
		cnt = wtk_utf8_bytes(*s);
		wrd_cnt++;
		s += cnt;
	}
	return wrd_cnt;
}

void qtk_kwfstdec_get_filter_result(qtk_kwfstdec_t* dec, wtk_strbuf_t *buf, float conf){
	qtk_kwfstdec_token_t *token, *best_token = NULL;
	wtk_string_t *v;
	wtk_fst_sym_t *sym_out = dec->net->cfg->sym_out;
	int in = 0,cnt = 0;

	qtk_kwfstdec_compute_final_cost(dec);
	best_token = dec->best_token;
	if (best_token)
	{
		if (best_token->pth != NULL)
		{
			for (token = best_token; token;)
			{
				if (token->pth) {
					//wtk_debug("=====================get rec result process best\n");
					//if(token->pth->in_label > 1){
					//wtk_debug("%d %d %f\n",token->pth->in_label,token->pth->out_label,token->pth->ac_cost);//}
					if(token->pth->out_label != 0)
					{
						if (token->pth->out_label > dec->cfg->remove_label)
						{
							if(dec->net->print)
							{
								v=dec->net->print->get_outsym(dec->net->print->ths,token->pth->out_label);
							}else
							{
								v = sym_out->strs[token->pth->out_label];
							}
							if (buf->pos > 0) {
								wtk_strbuf_push_front(buf," ", 1);
							}
							wtk_strbuf_push_front(buf,v->data,v->len);
							cnt = qtk_kwfstdec_get_sym_cnt(v);
							wtk_strbuf_push_int_front(dec->path_out_cnt,&cnt,1);
							wtk_strbuf_push_int_front(dec->path_out_id,&token->pth->out_label,1);
						}
					}

					if(token->pth->in_label > 1)
					{
						in = token->pth->in_label;
						wtk_strbuf_push_float_front(dec->path_cost,&token->pth->ac_cost,1);
						wtk_strbuf_push_int_front(dec->path_id,&in,1);
					}

					if (token->pth->lbest)
					{
						token = token->pth->lbest;
					}
				} else
				{
					token = NULL;
				}
			}
		}

		//int* in_id = (int*)dec->path_id->data;
		float* cost = (float*)dec->path_cost->data;
		int* out_cnt = (int*)dec->path_out_cnt->data;
		int* out_id = (int*)dec->path_out_id->data;
		float sum = 0.0,tmp_conf = 0.0;
		int i,j,n,out;
		if(dec->path_out_cnt->pos <= 4){
			for(i = 0;i < *out_cnt;i++,cost++){
				sum += *cost;
			}
			dec->conf = sum/(*out_cnt);
			if(dec->conf < conf){
				wtk_strbuf_reset(buf);
			}
		}else{
			wtk_strbuf_reset(buf);
			cnt = dec->path_out_cnt->pos/sizeof(int);
			for(i = 0;i < cnt; i++,out_cnt++){
				n = *(out_cnt);
				sum = 0.0;
				for(j = 0;j < n; j++,cost++){
					sum += *cost;
				}
				tmp_conf = sum/n;
				if(tmp_conf > conf){
					out = *(out_id + i);
					v = sym_out->strs[out];
					wtk_strbuf_push(buf,v->data,v->len);
					wtk_strbuf_push(buf," ", 1);
				}
			}
		}
	}	
}

void qtk_kwfstdec_get_result(qtk_kwfstdec_t* dec, wtk_strbuf_t *buf)
{
	//wtk_debug("=====================get rec result \n");

	if (dec->cur_frame == 0)
	{
		return;
	}

	int ret,cnt;
	wtk_fst_sym_t *sym_out;
	qtk_kwfstdec_token_t *token, *best_token = NULL;
	wtk_string_t *v;
	wtk_string_t v2;
	float best;

	cnt=0;
	best=0.0;

	sym_out = dec->net->cfg->sym_out;

	//wtk_debug("=====================get rec result get element: state %d\n",final_toks->key);
	//qtk_kwfstdec_gen_lat(dec);
	if (dec->cfg->use_rescore)
	{
		ret = qtk_kwfstdec_finalize_decoding(dec);
		if (ret != 0) {
			return;
		}
		wtk_string_t *sep;
		sep = wtk_string_new(100);
		wtk_string_set(sep, " ", 1);

		qtk_kwfstdec_gen_lat2(dec);

		dec->lat_net->eof = 1;
		//wtk_fst_net3_remove_eps(dec->lat_net);
		//wtk_fst_net3_prune_path(dec->lat_net);
		//wtk_fst_net3_print(dec->lat_net);

		ret = wtk_fst_net3_to_net2(dec->lat_net, dec->input_net);
		//wtk_fst_net2_print(dec->input_net);
		ret = wtk_rescore_process(dec->lat_rescore, dec->input_net);
		//wtk_fst_net2_print(dec->lat_rescore->output_net);
		wtk_rescore_get_result(dec->lat_rescore, &v2, sep->data, sep->len);
		wtk_strbuf_push(buf, v2.data, v2.len);
		wtk_string_delete(sep);
	} else if (dec->cfg->use_hot)	//TODO get final result using hot words
	{
		ret = qtk_kwfstdec_finalize_decoding(dec);
		if (ret != 0)
		{
			return;
		}
		wtk_string_t *sep;
		sep = wtk_string_new(100);
		wtk_string_set(sep, " ", 1);
		qtk_kwfstdec_gen_lat2(dec);

		dec->lat_net->eof = 1;
		// wtk_fst_net3_remove_eps(dec->lat_net);
		// wtk_fst_net3_prune_path(dec->lat_net);

		// wtk_fst_net3_print(dec->lat_net);

		wtk_fst_net3_get_sym_combination(dec->lat_net);
		qtk_kwfstdec_add_extra_pth(dec);
		wtk_fst_net3_to_net2(dec->lat_net, dec->input_net);

		ret = wtk_fst_net2_shortest_path(dec->input_net);
		if (ret == 0)
		{
			wtk_fst_net2_get_short_one_best_path3(dec->input_net, buf,
					sep->data, sep->len, dec->hot_sym);
		}
		//wtk_fst_net2_print(dec->input_net);
		//wtk_strbuf_push(buf,v2.data,v2.len);
		wtk_string_delete(sep);

		qtk_kwfstdec_compute_final_cost(dec);
		best_token = dec->best_token;
		dec->conf = -best_token->ac_cost/dec->cur_frame;
	} else
	{
		qtk_kwfstdec_compute_final_cost(dec);
		//qtk_kwfstdec_best_path_end(dec);
		best_token = dec->best_token;
		//wtk_debug("%f %d\n",best_token->ac_cost,dec->cur_frame);
		//wtk_debug("conf:%f\n",-best_token->ac_cost/dec->cur_frame);
		//wtk_debug("best tok:%p\n",best_token);
		if (best_token)
		{
			if (best_token->pth != NULL)
			{
				for (token = best_token; token;)
				{
					if (token->pth) {
						//wtk_debug("=====================get rec result process best\n");
						//wtk_debug("%d %d %f\n",token->pth->in_label,token->pth->out_label,token->pth->ac_cost);
						if(token->pth->out_label != 0)
						{
							if (token->pth->out_label > dec->cfg->remove_label)
							{
								if(dec->net->print)
								{
									v=dec->net->print->get_outsym(dec->net->print->ths,token->pth->out_label);
								}else
								{
									v = sym_out->strs[token->pth->out_label];
								}
                                                                 if (buf->pos >
                                                                     0) {
                                                                     wtk_strbuf_push_front(
                                                                         buf,
                                                                         " ", 1);
                                                                 }
                                                                wtk_strbuf_push_front(
                                                                    buf,
                                                                    v->data,
                                                                    v->len);

                                                                if (token->pth
                                                                            ->in_label !=
                                                                        0 &&
                                                                    token->pth
                                                                            ->in_label !=
                                                                        1 &&
                                                                    token->pth
                                                                            ->in_label !=
                                                                        2) {
                                                                    // wtk_debug("ac_cost=%f\n",
                                                                    // token->pth->ac_cost);
                                                                    best +=
                                                                        token
                                                                            ->pth
                                                                            ->ac_cost;
                                                                    cnt++;
			                    }
							}
						}else
						{
			                if(token->pth->in_label!=0 && token->pth->in_label !=1 && token->pth->in_label !=2 )
			                {
			                	//wtk_debug("ac_cost=%f\n", token->pth->ac_cost);
			                    best+=token->pth->ac_cost;
			                    cnt++;
			                }
						}

						if (token->pth->lbest)
						{
							token = token->pth->lbest;
						}
					} else
					{
						token = NULL;
					}
				}
				if(cnt != 0)
				{
					//wtk_debug("conf=%f/%d\n", best, cnt);
					dec->conf = best/cnt;
				}else
				{
					dec->conf = 0.0;
				}
			}
		}
		if(dec->cfg->has_filler && dec->cfg->use_multi_filler && dec->conf>dec->recommand_conf){
			if(buf->pos > 0){
				dec->valid_index = dec->vad_index;
				//printf("%.*s\n",buf->pos,buf->data);
			}
		}
	}
	//wtk_strbuf_delete(buf);
}

//for eval model
void qtk_kwfstdec_get_result3(qtk_kwfstdec_t* dec, wtk_strbuf_t *buf)
{
	//wtk_debug("=====================get rec result \n");

	if (dec->cur_frame == 0)
	{
		return;
	}

	int cnt;
	wtk_fst_sym_t *sym_out;
	qtk_kwfstdec_token_t  *best_token = NULL;
	qtk_kwfstdec_pth_t *pth;
	wtk_string_t *v;
	float best;

	cnt=0;
	best=0.0;

	sym_out = dec->net->cfg->sym_out;

	qtk_kwfstdec_compute_final_cost(dec);
	//qtk_kwfstdec_best_path_end(dec);
	best_token = dec->best_token;
	//wtk_debug("%f %d\n",best_token->ac_cost,dec->cur_frame);
	//wtk_debug("conf:%f\n",-best_token->ac_cost/dec->cur_frame);
	//wtk_debug("best tok:%p\n",best_token);
	if (best_token)
	{
		if (best_token->pth != NULL)
		{
			pth = best_token->pth;
			while (pth) {
				//wtk_debug("=====================get rec result process best\n");
				if(pth->out_label != 0)
				{
					if (pth->out_label > 2)
					{
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

	                    if(pth->in_label !=0 && pth->in_label !=1 && pth->in_label !=2 )
	                    {
	                    	//wtk_debug("ac_cost=%f\n", token->pth->ac_cost);
	                        best+=pth->ac_cost;
	                        cnt++;
	                    }
					}
				}else
				{
	                if(pth->in_label!=0 && pth->in_label !=1 && pth->in_label !=2 )
	                {
	                	//wtk_debug("ac_cost=%f\n", token->pth->ac_cost);
	                    best+=pth->ac_cost;
	                    cnt++;
	                }
				}

				pth = (qtk_kwfstdec_pth_t*)pth->lbest;
			}
		}
		if(cnt != 0)
		{
			//wtk_debug("conf=%f/%d\n", best, cnt);
			dec->conf = best/cnt;
		}else
		{
			dec->conf = 0.0;
		}
	}
//	printf(" %.*s\n",buf->pos,buf->data);*/
	//wtk_strbuf_delete(buf);
}

void qtk_kwfstdec_get_phn_info(qtk_kwfstdec_t *dec,wtk_strbuf_t *buf)
{
	qtk_kwfstdec_token_t *token, *best_token = NULL;
	qtk_kwfstdec_trans2phn *trans=NULL,*tran;
	trans = dec->cfg->trans2phn;

	if (dec->cur_frame == 0 || !trans)
	{
		return;
	}

	qtk_kwfstdec_compute_final_cost(dec);
	wtk_debug("please check qtk_kwfstdec_compute_final_cost_?\n");
	best_token = dec->best_token;
	trans = dec->cfg->trans2phn;

	if (best_token)
	{
		if (best_token->pth != NULL)
		{
			for (token = best_token; token;)
			{
				if (token->pth) {
					tran = trans + token->pth->in_label;
					//if(token->pth->in_label >2 )
					//{
						wtk_debug("%d %d\n",tran->phn_id,tran->state_id);
					//}

					if (token->pth->lbest)
					{
						token = token->pth->lbest;
					}
				} else
				{
					token = NULL;
				}
			}
		}
	}
}

void qtk_kwfstdec_get_fa(qtk_kwfstdec_t *dec,wtk_strbuf_t *buf)//out_label
{
	//wtk_debug("=====================get rec result \n");

	if (dec->cur_frame == 0)
	{
		return;
	}
	int cnt;
	wtk_fst_sym_t *sym_out;
	qtk_kwfstdec_token_t *token, *best_token = NULL;
	wtk_string_t *v;
	float best;

	cnt=0;
	best=0.0;

	sym_out = dec->net->cfg->sym_out;
	wtk_strbuf_t *fa,*fa2;
	fa=wtk_strbuf_new(1024,1);
	fa2=wtk_strbuf_new(1024,1);
    int start_frame=-1;
    int end_frame=-1;
    int flagx=0;
    int frame=dec->cur_frame;
	qtk_kwfstdec_compute_final_cost(dec);
	wtk_debug("please check qtk_kwfstdec_compute_final_cost_?\n");
	best_token = dec->best_token;
	if (best_token)
	{
		if (best_token->pth != NULL)
		{
			for (token = best_token; token;)
			{
				if (token->pth) {
					//wtk_debug(" in_lab=%d out_lab=%d\n", token->pth->in_label, token->pth->out_label);
					//wtk_debug("=====================get rec result process best\n");
					if(token->pth->in_label >2 )
					{
						if(!flagx)
						{
							end_frame=frame;
							flagx=1;
						}
					}

					if(token->pth->out_label != 0)
					{
						if (token->pth->out_label > 2)
						{
							start_frame=frame;
							flagx=0;

							if(dec->net->print)
							{
								v=dec->net->print->get_outsym(dec->net->print->ths,token->pth->out_label);
							}else
							{
								v = sym_out->strs[token->pth->out_label];
							}
							if (buf->pos > 0)
							{
								wtk_strbuf_push_front(buf, " ", 1);
							}
		                    wtk_strbuf_push_f(fa,"%f %f %.*s\n",start_frame*0.03,(end_frame+1)*0.03,v->len,v->data);
		                    wtk_strbuf_push_front(fa2, fa->data, fa->pos);
		                    wtk_strbuf_reset(fa);
							wtk_strbuf_push_front(buf, v->data, v->len);

		                    if(token->pth->in_label !=0 && token->pth->in_label !=1 && token->pth->in_label !=2 )
		                    {
		                        best+=token->pth->ac_cost;
		                        cnt++;
		                    }
						}
					}else
					{
		                if(token->pth->in_label!=0 && token->pth->in_label !=1 && token->pth->in_label !=2 )
		                {
		                    best+=token->pth->ac_cost;
		                    cnt++;
		                }
					}

		            if(token->pth->in_label!=0)
		            {
		                    frame--;
		            }

					if (token->pth->lbest)
					{
						token = token->pth->lbest;
					}
				} else
				{
					token = NULL;
				}
			}
			printf("%.*s\n",fa2->pos,fa2->data);
	        wtk_strbuf_delete(fa);
	        wtk_strbuf_delete(fa2);
			dec->conf = best/cnt;
		}
	}

}

void qtk_kwfstdec_best_path_end(qtk_kwfstdec_t *dec)
{
	float cost, final_cost;
	qtk_kwfstdec_token_t *tok;
	qtk_kwfstdec_token_list_t *tlist;
	wtk_queue_node_t *qn;
	final_cost_q_t* fc;

	qn = wtk_queue_peek(&(dec->active_tok), dec->cur_frame + 1);
	//wtk_debug("best pth:%d\n",dec->cur_frame+1);

	tlist = (qtk_kwfstdec_token_list_t*) data_offset(qn,
			qtk_kwfstdec_token_list_t, q_n);
	int count = 0;
	for (tok = tlist->token; tok != NULL; tok = tok->next)
	{
		count++;
		//wtk_debug("count====%p=====:%d\n",tok,count);
		cost = tok->tot_cost;
		final_cost = 0.0;
		fc = (final_cost_q_t*) wtk_queue_find(&(dec->final_cost_q),
				offsetof(final_cost_q_t, q_n),
				(wtk_cmp_handler_t) qtk_kwfstdec_tok_cmp, tok);
		if (fc)
		{
			//wtk_debug("findxxx:%f\n",fc->cost);
			final_cost = fc->cost;
			cost += final_cost;
		} else
		{
			cost = FLT_MAX;
		}
		if (cost < dec->best_weight)
		{
			dec->best_weight = cost;
			dec->best_token = tok;
			dec->best_final_cost = final_cost;
		}
	}
	dec->conf = dec->best_weight / dec->cur_frame;
	//wtk_debug("=====================best weight:%f %f\n",dec->best_weight,dec->best_final_cost);
}

int qtk_kwfstdec_get_best_path(qtk_kwfstdec_t *dec)
{
	qtk_kwfstdec_best_path_end(dec);

	return 0;
}

///////////////////////////////FOR SPEED AND MEM. OPTIMIZATION//////////////////////////////////////////////////
void qtk_kwfstdec_compute_final_cost3(qtk_kwfstdec_t* dec)
{
	qtk_hash_elem_t *elem, *next;
	qtk_kwfstdec_token_t *token;
	final_cost_q_t *fc;
	float best_cost_with_final = FLT_MAX;
	float best_cost = FLT_MAX;
	float final_cost, cost, cost_with_final;
	wtk_fst_state_t *state;
	dec->best_weight = FLT_MAX;
	//wtk_debug("compute final cost\n");
	//elem = qtk_hash_list_clear(dec->tok);
	elem = qtk_hash_list_clear3(dec->tok);
	while (elem != NULL)
	{
		token = elem->token;
		next = elem->tail;
		state = elem->state;//wtk_fst_net_get_load_state(dec->net,elem->key);
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
		if (isFiniteNumber(final_cost))
		{
			fc = (final_cost_q_t*) wtk_heap_malloc(dec->heap,
					sizeof(final_cost_q_t));
			//fc=(final_cost_q_t*)wtk_malloc(sizeof(final_cost_q_t));
			fc->cost = final_cost;
			fc->token = token;
			fc->state = state->id;
			wtk_queue_push(&(dec->final_cost_q), &(fc->q_n));
			cost += final_cost;
			if (cost < dec->best_weight)
			{
				dec->best_weight = cost;
				dec->best_token = token;
				dec->best_final_cost = final_cost;
			}
			//wtk_debug("%p state->id:%d push best:%f\n",token,state->id,final_cost);
		}
		elem = next;
	}

	if (isFiniteNumber(best_cost) && isFiniteNumber(best_cost_with_final))
		dec->final_relative_cost = FLT_MAX;
	else
		dec->final_relative_cost = best_cost_with_final - best_cost;

	if (isFiniteNumber(best_cost_with_final))
		dec->final_best_cost = best_cost_with_final;
	else
		dec->final_best_cost = best_cost;
}

static qtk_kwfstdec_token_t* qtk_kwfstdec_new_token(qtk_kwfstdec_t *dec, float extra_cost)
{
	qtk_kwfstdec_token_t *token;
	token = (qtk_kwfstdec_token_t *)wtk_vpool2_pop(dec->tok_pool);
    if (!token) {
        wtk_debug("ALLOC FAILED\r\n");
        return NULL;
    }
	token->extra_cost= extra_cost;
	token->tot_cost=0.0f;
	token->ac_cost=0.0f;
	token->link=NULL;
	token->pth=NULL;
	token->next=NULL;
	token->hook=token;
	token->state = -1;

    return token;
}
static qtk_kwfstdec_pth_t* qtk_kwfstdec_new_pth(qtk_kwfstdec_t *dec,qtk_kwfstdec_token_t *token)
{
	qtk_kwfstdec_pth_t *pth;

	pth = (qtk_kwfstdec_pth_t *)wtk_vpool2_pop(dec->pth_pool);
    if (!pth) {
        wtk_debug("ALLOC FAILED\r\n");
        return NULL;
    }
    pth->lbest = token;
    pth->lbest = 0;
    pth->used = 0;

    return pth;
}

/**
 * add by dmd
 * Note: consider link between two pth. don't consider token.
 * for no adding men. using pth->lbest for last pth than last token.
 */
static qtk_kwfstdec_token_t* qtk_kwfstdec_find_or_add_token3(qtk_kwfstdec_t* dec,
		wtk_fst_trans_t* trans, int fram_plus_one, float tot_cost, int *changed,
		qtk_kwfstdec_token_t *pre_tok, float ac_cost)
{
	qtk_kwfstdec_token_t *token, *update_tok;
	qtk_kwfstdec_token_list_t*tok_list;
	float extra_cost = 0.0f;
	wtk_fst_state_t* state=trans->to_state;

	if(dec->net->nrbin_states <= 0)
	{
		wtk_fst_net_load_state(dec->net, state);
	}else
	{
		state = wtk_fst_net_get_load_state(dec->net,state->id);
	}
	if (state->frame == dec->cur_frame)
	{
		update_tok = (qtk_kwfstdec_token_t*) state->hook;
	} else
	{
		update_tok = NULL;
	}

	if (!update_tok)
	{
		//wtk_debug("===============find or add token state not found\n");
		tok_list = dec->cur_token_list;
		token = qtk_kwfstdec_new_token(dec, extra_cost);
		//wtk_debug("token= %p pre_tok= %p id=%d\n", token, pre_tok, state->id);
		//token->pth = qtk_hash_list_new_pth(dec->tok, pre_tok);
		token->pth = qtk_kwfstdec_new_pth(dec, pre_tok);
		//token->pth->lbest = pre_tok;
		token->pth->lbest = (qtk_kwfstdec_token_t*)pre_tok->pth;
		if(pre_tok->pth)pre_tok->pth->used++;
		//wtk_debug("pth=%p pre_pth=%p\n", token->pth, pre_tok->pth);
		token->pth->in_label = trans->in_id;
		token->pth->out_label = trans->out_id;
		token->pth->frame = dec->cur_frame;
		token->pth->ac_cost = ac_cost;
		//token->pth->state=state->id;   //for debug
		//token->pth->tot_cost = tot_cost; //for debug
		token->ac_cost = ac_cost;
		token->tot_cost = tot_cost;
		if (tok_list->token != NULL)
			token->next = tok_list->token;
		else
			token->next = NULL;
		dec->num_toks++;
		qtk_hash_list_Insert3(dec->tok, state->id, state, token);
		if (changed)
			*changed = 1;
		//tok_list = qtk_hash_list_new_token_list(token);
		//wtk_queue_push(&(dec->active_tok), &(tok_list->q_n));
		tok_list->token = token;
		state->frame = dec->cur_frame;
		state->hook = token;
	} else
	{
		//wtk_debug("===============find or add token state found\n");
		token = update_tok;
		if (token->tot_cost > tot_cost)
		{
			token->tot_cost = tot_cost;
			token->ac_cost = ac_cost;
			//for mem option
			((qtk_kwfstdec_pth_t*)token->pth->lbest)->used--;
			if(pre_tok->pth)pre_tok->pth->used++;

			//token->pth->lbest = pre_tok;
			token->pth->lbest = (qtk_kwfstdec_token_t*)pre_tok->pth;
			//wtk_debug("pth=%p pre_pth=%p\n", token->pth, pre_tok->pth);
			token->pth->in_label = trans->in_id;
			token->pth->out_label = trans->out_id;
			token->pth->ac_cost = ac_cost;
			//token->pth->tot_cost = tot_cost; //for debug
			if (changed)
				*changed = 1;
		}else
		{
			if (changed)
				*changed = 0;
		}
	}

	return token;
}

float qtk_kwfstdec_process_emitting_state3(qtk_kwfstdec_t* dec,wtk_vector_t *obs)
{
	wtk_fst_state_t *state;
	wtk_fst_trans_t *trans;
	qtk_kwfstdec_token_t *token = NULL, *next_tok;
	qtk_kwfstdec_token_list_t *tok_list;
	qtk_hash_elem_t *final_toks = qtk_hash_list_clear3(dec->tok);
	qtk_hash_elem_t* best_elem = NULL, *elem, *e_tail=NULL;
	int tok_cnt, i, ntrans, *pdf_id_, in_id, use_lat; //,frame, dnn_cnt
	float adaptive_beam = FLT_MAX, next_cutoff = FLT_MAX;
	float cur_cutoff,tot_cost,new_weight,cost_offset,likelihood, ac_cost,acscale; //,lmscale,graph_cost

	cur_cutoff = qtk_kwfstdec_get_cutoff2(dec, final_toks, &tok_cnt,
			&adaptive_beam, &best_elem);
	//wtk_debug("=====================process emitting: cur_cutoff:%f adaptive_beam:%f\n",cur_cutoff,adaptive_beam);
	//exit(0);
	//NEED RESIZE HASH???  tok_cnt

	tok_list = qtk_hash_list_new_token_list(dec, NULL);
	dec->cur_token_list = tok_list;
	pdf_id_ = dec->trans_model->id2pdf_id_;

	//lmscale = dec->cfg->lm_scale;
	acscale = dec->cfg->ac_scale;
	//wtk_debug("acscale=%f\n", acscale);
	cost_offset=0.0f;
	if (best_elem)
	{
		dec->best_elem = best_elem;
		//int state_id=best_elem->key;
		qtk_kwfstdec_token_t *tok = best_elem->token;
		cost_offset = -tok->tot_cost;
		state = best_elem->state;          //wtk_fst_net_get_load_state(dec->net,state_id);
		//for (i = 0, trans = state->v.trans; i < ntrans; ++i, ++trans)
		//wtk_debug("ntrans=%d\n", state->ntrans);
		for (trans = state->v.trans,ntrans=state->ntrans,i = ntrans; i>0; i--, ++trans)
		{
			in_id = trans->in_id;
			if (in_id != 0)
			{
				//dnn_cnt = *(pdf_id + trans->in_id);
				//dnn_cnt= trans->in_id-1;
				//likelihood=(*(obs+dnn_cnt+1));        //10;
				//likelihood = (*(obs + dnn_cnt));		//10;
				//likelihood = *(obs + *(pdf_id_ + in_id));
				//wtk_debug("test: ac_cost=%f %d\n",likelihood,trans->in_id);
				//wtk_debug("test: trans->weight=%f ac_cost=%f %d\n",trans->weight, *(obs + *(pdf_id_ + in_id)),trans->in_id);
				//new_weight = -trans->weight - likelihood;   //-trans->weight + cost_offset - likelihood + tok->tot_cost
				new_weight = -trans->weight - *(obs + *(pdf_id_ + in_id))*acscale;   //-trans->weight + cost_offset - likelihood + tok->tot_cost
				//wtk_debug("next_cutoff=%f %f %f\n", new_weight + adaptive_beam, adaptive_beam,next_cutoff);
				if (new_weight + adaptive_beam < next_cutoff)
				{
					next_cutoff = new_weight + adaptive_beam;
				}

			}
		}
		//exit(0);
	}
	use_lat = dec->cfg->use_lat;
	if(use_lat)
	{
		*(dec->cost_offset + dec->offset_count) = cost_offset;
		dec->offset_count++;
	}
	//wtk_debug("process emitting next cutoff:%f\n",cost_offset);

	//check error
	//qtk_hash_elem_t* el;
	//int cnt;
	//for(cnt=0,el=dec->tok->freed_head;el!=NULL; el = el->tail)cnt++;
	//printf("cnt=%d\n", cnt);
//	for (elem = final_toks; elem != NULL; elem = elem->tail)
//	{
//		for(el=dec->tok->freed_head;el!=NULL; el = el->tail)
//			if(elem == el)
//			{
//				wtk_debug("final_toks=%p elem=%p el=%p\n", final_toks,elem, el);
//				exit(0);
//			}
//
//	}

	//cur frame, for every state.
	//for (elem = final_toks; elem != NULL; elem = e_tail)
	for (elem = final_toks; elem != NULL; elem = elem->tail)
	{
		//wtk_debug("process element state:%d %p %p \n",elem->key,elem->token,elem);
		token = elem->token;
		state = elem->state;	//wtk_fst_net_get_load_state(dec->net,state_id);
		//wtk_debug("state=%d token tot_cost:%f cur_cutoff:%f\n",state->id, token->tot_cost,cur_cutoff);
		if (token->tot_cost <= cur_cutoff)
		{
			//every state, for it's trans to check all state at next frame
			//for (i=0, trans=state->v.trans; i < state->ntrans;++i, ++trans)
			//wtk_debug("state[%d] %p ntrans=%d\n", state->id, state, state->ntrans);
			for (ntrans=state->ntrans, trans=state->v.trans,i=ntrans; i>0; i--, ++trans)
			{
				if (!trans->to_state) continue;    //when state is also  a final state by dmd.
				in_id = trans->in_id;
				//wtk_debug("=======process emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
				if (in_id != 0)
				{
#ifdef DEBUG
					if(trans->in_id < 0)
					{
						wtk_debug("in_id=%d\n", trans->in_id);
					}
#endif
					//dnn_cnt = *(dec->trans_model->id2pdf_id_ + trans->in_id);
					//dnn_cnt=trans->in_id-1;
					//likelihood = *(obs + dnn_cnt);
					likelihood = *(obs + *(pdf_id_ + in_id));
					//wtk_debug("id=%d obs=%p cnt=%d like=%.5f token->tot_cost=%f\n", in_id,obs, *(pdf_id_ + in_id), likelihood,token->tot_cost);
					//ac_cost = token->ac_cost - likelihood;
					ac_cost = cost_offset - likelihood * acscale;
					//graph_cost = -(trans->weight);
					tot_cost = token->tot_cost + ac_cost  + -(trans->weight); // * lmscale;
					//wtk_debug("=======process emitting state:%p state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state,state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
					//tk_debug("state:%d tot_cost:%f cost_offset:%f likehood:%f adaptive_beam:%f ac_cost=%f cur_cost:%f\n",state->id,tot_cost,cost_offset,likelihood,adaptive_beam, ac_cost,token->tot_cost);
					//wtk_debug("tot_cost:%f  next_cutoff:%f\n",tot_cost,next_cutoff);

					if (tot_cost > next_cutoff)
					{
						continue;	//{wtk_debug("continue prune\n");continue;}
					}
					else if (tot_cost + adaptive_beam < next_cutoff)
						next_cutoff = tot_cost + adaptive_beam;
					//wtk_debug("search or add state:%d\n",trans->to_state->id)
					//expand next frame token.
					//wtk_debug("token->pth=%p\n", token->pth);
					next_tok = qtk_kwfstdec_find_or_add_token3(dec,
							trans, 0, tot_cost, NULL, token, likelihood);
					if (use_lat)
						token->link = qtk_hash_list_link_new(dec->tok, next_tok,
								trans->in_id, trans->out_id, -(trans->weight), ac_cost,
								token->link);
				}
			}
		}

		//e_tail = elem->tail;
		//qtk_hash_list_del(dec->tok, elem);
	}
	//free pth
	if (dec->cfg->use_memctl)
	{
		for (elem = final_toks; elem != NULL; elem = elem->tail)
		{
			token = elem->token;
			if(token->pth && token->pth->used==0)
				wtk_vpool2_push(dec->pth_pool, token->pth);
			token->pth = NULL;
			if(!use_lat)
			{
				wtk_vpool2_push(dec->tok_pool, token);
			}
			e_tail=elem;
		}
	}

	//one time free all.
	////if(dec->tok->list_head != final_toks)
	if(final_toks)qtk_hash_list_free(dec->tok, final_toks, e_tail);    //qtk_hash_list_clear:hash->list_head=NULL; or cause err to enter dead loop.
	//exit(0);
	return next_cutoff;
}

int qtk_kwfstdec_process_none_emitting_state3(qtk_kwfstdec_t* dec, float cutoff) {
	qtk_hash_elem_t *elem;//, *elem_end;
	st_q_t *st, *st1;
	qtk_kwfstdec_token_t *token;
	qtk_kwfstdec_token_t *new_token;
	wtk_queue_node_t *qn;
	wtk_fst_state_t *state;
	wtk_fst_trans_t *trans;
	float tot_cost = FLT_MAX, cur_cost = FLT_MAX;
	float graph_cost;//, lmscale = dec->cfg->lm_scale;
	int changed = 0, i, use_lat;//,frame = dec->active_tok.length - 2;

	//wtk_debug("qtk_kwfstdec_process_none_emitting_state3\n");
	wtk_heap_reset(dec->tmp_heap);
	for (elem = dec->tok->list_head; elem != NULL; elem = elem->tail)
	{
		//st = (st_q_t*) wtk_heap_malloc(dec->heap, sizeof(st_q_t));
		st = (st_q_t*) wtk_heap_malloc(dec->tmp_heap, sizeof(st_q_t));
		st->state_id = elem->key;
		st->state = elem->state;
		wtk_queue_push(&(dec->state_q), &(st->q_n));
	}

	use_lat = dec->cfg->use_lat;
	while (1)
//	for (elem = dec->tok->list_head; elem != NULL; elem = elem->tail;)
	{
		qn = wtk_queue_pop_back(&(dec->state_q));
		if (!qn)
		{
			break;
		}
		st = data_offset(qn, st_q_t, q_n);
		state = st->state;	//wtk_fst_net_get_load_state(dec->net,st->state_id);
		//state = elem->state;
		token = (qtk_kwfstdec_token_t*) state->hook;

		cur_cost = token->tot_cost;   //在同步更新的时候会影响．
		//wtk_debug("============process none emitting state:%d cur_cost:%f cutoff:%f\n",st->state_id,cur_cost,cutoff);
		if (cur_cost > cutoff)
			continue;
//		qtk_hash_list_link_delete(token);
		if(use_lat)
			token->link = NULL;
		//state=wtk_fst_net_get_load_state(dec->net,st->state_id);
		for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans)
		{
			if (!trans->to_state) continue;   //when state is also a final state by dmd.
			if (trans->in_id == 0)
			{
				//wtk_debug("============process none emitting state:%d to:%d ilabel:%d olabel:%d weight:%f\n",state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight);
				graph_cost = -(trans->weight);
				tot_cost = cur_cost + graph_cost;//*lmscale;
				//wtk_debug("============process none emitting state:%p state:%d to:%d ilabel:%d olabel:%d weight:%f tot_cost:%f cur_cost=%f\n",state, state->id,trans->to_state->id,trans->in_id,trans->out_id,trans->weight,tot_cost, cur_cost);
				//wtk_debug("tot_cost:%f cutoff:%f\n",tot_cost,cutoff);
				if (tot_cost < cutoff)
				{
					//changed=0;    //no need. following other operation must give value.
					new_token = qtk_kwfstdec_find_or_add_token3(dec,
							trans, 0, tot_cost, &changed,
							token, 0.0);
					if (use_lat)
						token->link = qtk_hash_list_link_new(dec->tok, new_token, 0,
								trans->out_id, graph_cost, 0, token->link);
					if (changed)
					{
//						st1 = (st_q_t*) wtk_heap_malloc(dec->heap,
//								sizeof(st_q_t));
						st1 = (st_q_t*) wtk_heap_malloc(dec->tmp_heap,
														sizeof(st_q_t));
						st1->state_id = trans->to_state->id;
						st1->state = trans->to_state;
						wtk_queue_push(&(dec->state_q), &(st1->q_n));
					}
				}
			}
		}
	}

	return 0;
}

//for outer net
qtk_kwfstdec_t* qtk_kwfstdec_new3(qtk_kwfstdec_cfg_t* cfg, int use_outnet)
{
	qtk_kwfstdec_t *kdec = (qtk_kwfstdec_t*) wtk_malloc(sizeof(qtk_kwfstdec_t));

	kdec->heap = wtk_heap_new(4096);
	kdec->heap2 = wtk_heap_new(4096);
	kdec->bins = (unsigned int*) wtk_malloc(sizeof(unsigned int) * 500);
	memset(kdec->bins, 0, sizeof(unsigned int) * 500);

	kdec->tok_pool = wtk_vpool2_new(sizeof(qtk_kwfstdec_token_t), (int)(cfg->max_active * cfg->pool_scale));
	kdec->pth_pool = wtk_vpool2_new(sizeof(qtk_kwfstdec_pth_t), (int)(cfg->max_active * cfg->pool_scale));
	kdec->tmp_heap = wtk_heap_new(sizeof(qtk_kwfstdec_pth_t) * (int)(cfg->max_active * cfg->pool_scale));

	wtk_queue_init(&(kdec->active_tok));
	wtk_queue_init(&(kdec->hot_words));
	wtk_queue_init(&(kdec->state_q));
	wtk_queue_init(&(kdec->tmp_q));
	wtk_queue_init(&(kdec->final_cost_q));
	//wtk_queue_init(&(kdec->token_map_q));
	wtk_queue_init(&(kdec->topsorted_list_q));

	kdec->cfg = cfg;
	//kdec->trans_model=cfg->trans_model.trans_model;
	if (use_outnet)
		kdec->net = NULL;
	else
		kdec->net = wtk_fst_net_new(&(cfg->net));
	kdec->hot_sym = NULL;
	kdec->lat_support = NULL;
	if (cfg->use_lat)
	{
		//r->use_ntok=0;
		kdec->lat_net = wtk_fst_net3_new(&(cfg->lat_net), &(cfg->net));
	} else
	{
		kdec->lat_net = 0;
	}
	if (cfg->use_rescore)
	{
		kdec->lat_rescore = wtk_rescore_new(&(cfg->rescore), NULL);
	} else
	{
		kdec->lat_rescore = 0;
	}
	kdec->num_toks = 0;
	kdec->cur_frame = -1;
	kdec->tok = qtk_hash_list_new3(cfg->size);
	kdec->best_weight = FLT_MAX;
	kdec->best_elem = NULL;
	kdec->best_token = NULL;
	kdec->best_final_cost = FLT_MAX;
	kdec->cur_token = NULL;
	kdec->final_relative_cost = FLT_MAX;
	kdec->final_best_cost = FLT_MAX;
	if(cfg->use_lat)
		kdec->cost_offset = (float*) wtk_calloc(6000, sizeof(float));
	else
		kdec->cost_offset = NULL;
	kdec->offset_count = 0;

	kdec->input_net = wtk_fst_net2_new(&(cfg->net));
	kdec->conf = 0.0;
	if (cfg->use_hot)
	{
		qtk_kwfstdec_set_hot_words(kdec, kdec->cfg->hot_words);
	}

	return kdec;
}

int qtk_kwfstdec_start3(qtk_kwfstdec_t* dec)
{
	int stateID = 0;
	qtk_kwfstdec_token_t *start_tok;
	qtk_kwfstdec_token_list_t*tok_list;
	wtk_fst_state_t* state;

	dec->num_toks = 1;
	dec->decoding_finalized = 0;
	start_tok = qtk_kwfstdec_new_token(dec, 0.0f);
	//wtk_debug("xxxxxxx:%p\n",start_tok);
	tok_list = qtk_hash_list_new_token_list(dec, start_tok);
	state = wtk_fst_net_get_load_state(dec->net, stateID);
	state->frame = -1;
	state->hook = start_tok;
	qtk_hash_list_Insert3(dec->tok, stateID, state, start_tok);
	dec->cur_token_list = tok_list;
	qtk_kwfstdec_process_none_emitting_state3(dec, dec->cfg->beam);

	return 0;
}

int qtk_kwfstdec_feed3(qtk_kwfstdec_t* dec, wtk_vector_t *v, int index)
{
	float *v2;
	v2 = v;
	if (0 != index%dec->cfg->frame_skip) {
		return 0;
	}
	if (dec->cfg->add_softmax)
	{
		v2 = (float*) wtk_malloc(sizeof(float) * 3045);
		memcpy(v2, v, sizeof(float) * 3045);
		wtk_softmax(v2, 3045);
		wtk_nnet3_log(v2, 3045);
	}
	float cost_cutoff;
	//wtk_debug("==========feed frame %d=============dec->cur_frame=%d\n", index, dec->cur_frame);
	dec->cur_frame++;    //euqal "= index" when no skip.
	if (index % dec->cfg->prune_interval == 0)
		qtk_kwfstdec_prune_active_tokens(dec,dec->cfg->beam * dec->cfg->prune_scale);
	cost_cutoff = qtk_kwfstdec_process_emitting_state3(dec, v2);
	qtk_kwfstdec_process_none_emitting_state3(dec, cost_cutoff);
	if (dec->cfg->add_softmax)
	{
		wtk_free(v2);
	}

	return 0;
}

void qtk_kwfstdec_reset3(qtk_kwfstdec_t* dec)
{
//	wtk_debug("---------reset--------\n");
	memset(dec->bins, 0, sizeof(unsigned int) * 500);

	wtk_heap_reset(dec->heap);
	if(dec->net)
		wtk_fst_net_reset(dec->net);
	wtk_vpool2_reset(dec->tok_pool);
	wtk_vpool2_reset(dec->pth_pool);
	wtk_fst_net2_reset(dec->input_net);
	qtk_hash_list_reset3(dec->tok);
	wtk_queue_init(&(dec->active_tok));
	wtk_queue_init(&(dec->state_q));
	wtk_queue_init(&(dec->tmp_q));
	wtk_queue_init(&(dec->final_cost_q));
	//wtk_queue_init(&(dec->token_map_q));
	wtk_queue_init(&(dec->topsorted_list_q));
	if (dec->cfg->use_lat)
	{
		wtk_fst_net3_reset(dec->lat_net);
		//dec->use_rescore=1;
	}
	if (dec->lat_rescore)
	{
		wtk_rescore_reset(dec->lat_rescore);
	}
	dec->num_toks = 0;
	dec->cur_frame = -1;
	dec->best_token = NULL;
	dec->best_elem = NULL;
	dec->best_weight = FLT_MAX;
	dec->best_final_cost = FLT_MAX;
	dec->cur_token = NULL;
	dec->final_relative_cost = FLT_MAX;
	dec->final_best_cost = FLT_MAX;

	if(dec->lat_support)
	{
		free(dec->lat_support);
		dec->lat_support = NULL;
	}

	if(dec->cfg->use_lat)
	{
		int i;
		float *p = dec->cost_offset;
		for (i = 0; i < 6000; i++)
		{
			*p = 0.0;
			p++;
		}
		dec->offset_count = 0;
	}
	dec->conf = 0.0;
	//dec->trans_model=dec->cfg->trans_model.trans_model;
}

void qtk_kwfstdec_delete3(qtk_kwfstdec_t* dec)
{
	if (dec->lat_rescore)
	{
		wtk_rescore_delete(dec->lat_rescore);
	}
	wtk_free(dec->bins);
	wtk_free(dec->cost_offset);
	wtk_fst_net2_delete(dec->input_net);
	wtk_heap_delete(dec->heap);
	wtk_heap_delete(dec->heap2);
	if (dec->net)
		wtk_fst_net_delete(dec->net);

	wtk_vpool2_delete(dec->tok_pool);
	wtk_vpool2_delete(dec->pth_pool);
	wtk_heap_delete(dec->tmp_heap);

	qtk_hash_list_delete3(dec->tok);
	if (dec->cfg->use_lat)
	{
		wtk_fst_net3_delete(dec->lat_net);
	}
	if (dec->hot_sym)
	{
		wtk_free(dec->hot_sym);
	}
	wtk_free(dec);
}

//////////////////////////test and verify//////////////////
int qtk_kwfstdec_bytes(qtk_kwfstdec_t* dec)
{
	long int bytes;

	bytes = sizeof(*dec);
	bytes = wtk_fst_net_bytes(dec->net);
	bytes += qtk_hash_list_bytes(dec->tok);
	wtk_debug("kwfstdec bytes=%fM\n", qtk_hash_list_bytes(dec->tok)/1024.0/1024.0);
	bytes += wtk_vpool2_bytes(dec->tok_pool);
	wtk_debug("kwfstdec bytes=%fM\n", wtk_vpool2_bytes(dec->tok_pool)/1024.0/1024.0);
	bytes += wtk_vpool2_bytes(dec->pth_pool);
	wtk_debug("kwfstdec bytes=%fM\n", wtk_vpool2_bytes(dec->pth_pool)/1024.0/1024.0);
	bytes += wtk_heap_bytes(dec->heap);
	wtk_debug("kwfstdec heap bytes=%fM\n", wtk_heap_bytes(dec->heap)/1024.0/1024.0);
	bytes += wtk_heap_bytes(dec->heap2);
	wtk_debug("kwfstdec heap2 bytes=%fM\n", wtk_heap_bytes(dec->heap2)/1024.0/1024.0);
	bytes += wtk_heap_bytes(dec->tmp_heap);
	wtk_debug("kwfstdec tmp_heap bytes=%fM\n", wtk_heap_bytes(dec->tmp_heap)/1024.0/1024.0);
	// other...

	return bytes;
}

float qtk_kwfstdec_set_vadindex(qtk_kwfstdec_t* dec,int index){
	int dur_frames = index - dec->valid_index;
	dec->vad_index = index;
	if(dec->idle == 0){
		if(dur_frames > dec->cfg->idle_hint){
			dec->idle = 1;
			dec->recommand_conf = dec->cfg->idle_conf;
		}
		if(dur_frames > dec->cfg->idle_hint * 3/5){
			dec->idle = 2;
		}
		dec->recommand_conf = dec->cfg->norm_conf;
	}else if(dec->idle == 1){
		if(dec->valid_index == 0){
			dec->recommand_conf = dec->cfg->idle_conf;
		}else if(dur_frames > 0 && dur_frames < dec->cfg->idle_hint){
			dec->idle = 0;
			dec->recommand_conf = dec->cfg->norm_conf;
		}else{
			dec->recommand_conf = dec->cfg->idle_conf;
		}
	}else if(dec->idle == 2){
		if(dur_frames > dec->cfg->idle_hint){
			dec->idle = 1;
			dec->recommand_conf = dec->cfg->idle_conf;
		}else if(dur_frames > 0 && dur_frames <= dec->cfg->idle_hint *2/5){
			dec->idle = 0;
		}
		dec->recommand_conf = dec->cfg->norm_conf;
	}
	//wtk_debug("%d %d %f\n",dec->idle,dur_frames,dec->recommand_conf);
	return dec->recommand_conf;
}
