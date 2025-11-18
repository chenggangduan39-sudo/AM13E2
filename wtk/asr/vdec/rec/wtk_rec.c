#include "wtk_rec.h"
#include "wtk/asr/wfst/rec/wtk_wfst_dnn_cfg.h"
void wtk_rec_merge_tokenset(wtk_rec_t *rec, wtk_tokenset_t *res,
		wtk_token_t *cmp, wtk_tokenset_t *src);
void wtk_rec_trace2(wtk_rec_t *rec, wtk_tokenset_t *ts);
void wtk_netnode_trace(wtk_netnode_t *node);
void wtk_tokenset_print(wtk_tokenset_t *ts);
float wtk_rec_netnode_wdlike(wtk_rec_t *rec, wtk_netnode_t *node);
void wtk_path_trace(wtk_path_t *path);
//#define wtk_netinst_unlink(inst) {(inst)->next->prev=(inst)->prev;(inst)->prev->next=(inst)->next;}
#define wtk_rec_unlink_list(inst) {(inst)->next->pre=(inst)->pre;(inst)->pre->next=(inst)->next;}
static wtk_token_t null_token =
{ LZERO, 0.0f, NULL, NULL };
wtk_reltoken_t rnull_token =
{ LZERO, 0.0, NULL, NULL };
wtk_reltoken_t max_token =
{ 0, 0, NULL, NULL };


/*
*/
/**
 *   The types LogFloat and LogDouble are used for representing
 *real numbers on a log scale.  LZERO is used for log(0) in log
 *arithmetic, any log real value <= LSMALL is considered to be
 *zero.
 * Return sum x + y on log scale, sum < LSMALL is floored to LZERO;
 */
double wtk_rec_log_add(wtk_rec_t *rec, double x, double y)
{
	double temp, diff, z;

	//log(sum(e^x,e^y))
	//log(e^x+e^y)=x+log(1+e^(y-x))
	if (x < y)
	{
		temp = x;
		x = y;
		y = temp;
	}
	diff = y - x;
	if (diff < rec->min_log_exp)
	{
		return (x < LSMALL) ? LZERO : x;
	}
	else
	{
		z = exp(diff);
		return x + log(1.0 + z);
	}
}

float wtk_rec_calc_mix_prob(wtk_rec_t *rec, wtk_vector_t *obs,
		wtk_mixpdf_t *mix)
{
	wtk_precomp2_t *pre;
	float prob;

	if (mix->index <= rec->hmmlist->num_phy_mix && mix->index > 0)
	{
		pre = rec->mPre + mix->index - 1;
	}
	else
	{
		pre = 0;
	}
	//wtk_debug("calc pre=%p,id=%d,frame=%d\n",pre,pre->id,rec->frame);
	if (pre)
	{
		if (pre->id != rec->frame)
		{
			prob = wtk_mixpdf_calc_dia_prob(mix, obs);
			pre->id = rec->frame;
			pre->outp = prob;
		}
		else
		{
			//wtk_debug("use share mix\n");
			prob = pre->outp;
		}
	}
	else
	{
		prob = wtk_mixpdf_calc_dia_prob(mix, obs);
	}
	//wtk_debug("x=%f\n",prob);
	return prob;
}

void wtk_rec_set_dnn_handler(wtk_rec_t *rec,void *ths,wtk_rec_dnn_get_value_f f)
{
	rec->dnn_ths = ths;
	rec->dnn_get = f;
}

float wtk_rec_calc_prob(wtk_rec_t *rec, wtk_vector_t *obs, wtk_state_t* state)
{
	wtk_precomp2_t *pre;
	wtk_stream_t *stream;
	wtk_mixture_t *mix;
	int streams, i, m;
	float outp = 0;
	float weight, mixprob, streamprob;

	//wtk_debug("%d,%d\n",rec->nsp,state->index);
	if (state->index > 0 && state->index <= rec->nsp)
	{
		pre = rec->sPre + state->index - 1;
	}
	else
	{
		pre = 0;
	}
	if (!pre || pre->id != rec->frame)
	{
		if(rec->cfg->use_dnn)
		{
			wtk_dnn_state_t *s=state->dnn;

			if(rec->dnn_get)
			{
				//wtk_debug("id=%d frame=%d\n",pre->id,rec->frame);
				outp=rec->dnn_get(rec->dnn_ths,rec->f,s->index)+s->gconst;
			}else
			{
				outp=obs[s->index]+s->gconst;
			}

			//outp=obs[s->index]+s->gconst;
			//wtk_debug("v[%d]=%f,%f\n",s->index,obs[s->index],s->gconst);
		}else
		{
			streams = rec->hmmlist->stream_width[0];
			for (i = 1, stream = state->pStream; i <= streams; ++i, ++stream)
			{
				streamprob = LZERO;
				for (m = 1, mix = stream->pmixture; m <= stream->nMixture;
						++m, ++mix)
				{
					//weight=log(mix->fWeight);
					weight = mix->fWeight;
					mixprob = wtk_rec_calc_mix_prob(rec, obs, mix->pdf);
					//log(c*p)=log(c)+log(p)=weight+mixprob
					streamprob = wtk_rec_log_add(rec, streamprob, weight + mixprob);
				}
				if (streams == 1)
				{
					outp = streamprob;
				}
				else
				{
					outp += state->pfStreamWeight[i] * streamprob;
				}
			}
		}
		//wtk_debug("%.*s: %f\n",state->name->len,state->name->data,outp);
		//exit(0);
		if (pre)
		{
			pre->id = rec->frame;
			pre->outp = outp;
		}
	}
	else
	{
		outp = pre->outp;
	}
	//printf("%d:%f\n",++j,outp);//exit(0);
	return outp;
}

int wtk_rec_init_hmm(wtk_rec_t *rec)
{
	wtk_hmmset_t *hl = rec->hmmlist;

	rec->seIndexes = hl->seIndexes;
	rec->nsp = hl->num_states;
	rec->nmp=hl->num_phy_mix;
	rec->sPre = (wtk_precomp2_t*) wtk_calloc(rec->nsp,sizeof(wtk_precomp2_t));
	rec->mPre =
			(wtk_precomp2_t*) wtk_calloc(rec->nmp,sizeof(wtk_precomp2_t));
	return hl->max_hmm_state;
}

wtk_rec_t* wtk_rec_new(wtk_rec_cfg_t *cfg, wtk_hmmset_t *h, wtk_dict_t *d,
		double frame_dur)
{
	wtk_rec_t *r;

	r = (wtk_rec_t*) malloc(sizeof(*r));
	wtk_rec_init(r, cfg, h, d, frame_dur);
	return r;
}

int wtk_rec_delete(wtk_rec_t *rec)
{
	wtk_rec_clean(rec);
	free(rec);
	return 0;
}

int wtk_rec_init(wtk_rec_t *rec, wtk_rec_cfg_t *cfg, wtk_hmmset_t *h,
		wtk_dict_t *d, double frame_dur)
{
	wtk_reltoken_t *rtoks;
	int i, ntok;
	int min, max;
	float rate;
	wtk_heap_t *heap;

	memset(rec, 0, sizeof(*rec));
	//wtk_rec_cfg_init(&(rec->cfg));
	rec->cfg = cfg;
	if(cfg->use_prune)
	{
		rec->prune=wtk_prune_new(&(cfg->prune));
	}else
	{
		rec->prune=NULL;
	}
	rec->frame_dur = frame_dur;
	ntok = rec->cfg->ntok;
	rec->hmmlist = h;
	rec->dict = d;
	rec->lat = 0;
	//rec->state=0;
	rec->local_heap = wtk_heap_new(4096);
	heap = rec->global_heap = wtk_heap_new(4096);
	wtk_queue_init(&(rec->inst_queue));
	wtk_queue_init(&(rec->path_no_queue));
	wtk_queue_init(&(rec->path_yes_queue));
	wtk_queue_init(&(rec->align_no_queue));
	wtk_queue_init(&(rec->align_yes_queue));
	rec->min_log_exp = -log(-LZERO);
	rec->max_state = wtk_rec_init_hmm(rec);
	rec->sBuf = (wtk_tokenset_t*) wtk_heap_malloc(heap,
			rec->max_state * sizeof(wtk_tokenset_t));
	rec->sBuf -= 1;
	rec->node_array = wtk_array_new_h(heap, rec->cfg->ntok,
			sizeof(wtk_netnode_t*));
	min = cfg->bit_heap_min;
	max = cfg->bit_heap_max;
	rate = cfg->bit_heap_growf;
	if (ntok > 0)
	{
		rec->rtoks_inst2 = (wtk_reltoken_t*) wtk_heap_malloc(heap,
				sizeof(wtk_reltoken_t) * ntok);
		rec->rtoks_merge = (wtk_reltoken_t*) wtk_heap_malloc(heap,
				sizeof(wtk_reltoken_t) * ntok);
		rtoks = (wtk_reltoken_t*) wtk_heap_malloc(heap,
				rec->max_state * sizeof(wtk_reltoken_t) * ntok);
		rec->rths = (wtk_nxtpath_t*) wtk_heap_malloc(heap,
				sizeof(wtk_nxtpath_t) * ntok);
		rec->rel_tok_heap = wtk_bit_heap_new(sizeof(wtk_reltoken_t) * ntok, min,
				max, rate);
		rec->nxt_path_heap = wtk_bit_heap_new(sizeof(wtk_nxtpath_t), min, max,
				rate);
	}
	else
	{
		rtoks = 0;
		//rec->rths=0;rec->rtoks=0;
	}
	for (i = 0; i < rec->max_state; ++i)
	{
		rec->sBuf[i + 1].set = rtoks;
		if (rtoks)
		{
			rtoks += ntok;
		}
		rec->sBuf[i + 1].tok = null_token;
		rec->sBuf[i + 1].n = 0;
		if (rtoks)
		{
			rec->sBuf[i + 1].set[0] = max_token;
		}
	}
	rec->path_heap = wtk_bit_heap_new(sizeof(wtk_path_t), min, max, rate);
	rec->align_heap = wtk_bit_heap_new(sizeof(wtk_align_t), min, max, rate);
	rec->tokenset_heap_maxi = h->max_hmm_state;
	rec->tokenset_heap =
			(wtk_bit_heap_t**) wtk_calloc(rec->tokenset_heap_maxi,sizeof(wtk_bit_heap_t*));
	rec->tokenset_heap[1] = wtk_bit_heap_new(sizeof(wtk_tokenset_t), min, max,
			rate);
	rec->inst_heap = wtk_bit_heap_new(sizeof(wtk_netinst_t), min, max, rate);
	rec->word_thresh = rec->gen_thresh = rec->n_thresh = LSMALL;
	if(rec->cfg->hlda_matrix)
	{
		rec->hlda_vector=wtk_vector_new(wtk_matrix_rows(rec->cfg->hlda_matrix));
	}else
	{
		rec->hlda_vector=0;
	}
	return 0;
}

int wtk_rec_clean(wtk_rec_t *rec)
{
	int i;

	if(rec->prune)
	{
		wtk_prune_delete(rec->prune);
	}
	for (i = 1; i < rec->tokenset_heap_maxi; ++i)
	{
		if (rec->tokenset_heap[i])
		{
			wtk_bit_heap_delete(rec->tokenset_heap[i]);
		}
	}wtk_free(rec->tokenset_heap);
	wtk_bit_heap_delete(rec->inst_heap);
	if (rec->mPre)
	{
		wtk_free(rec->mPre);
	}
	if (rec->sPre)
	{
		wtk_free(rec->sPre);
	}
	if (rec->rel_tok_heap)
	{
		wtk_bit_heap_delete(rec->rel_tok_heap);
	}
	if (rec->nxt_path_heap)
	{
		wtk_bit_heap_delete(rec->nxt_path_heap);
	}
	wtk_bit_heap_delete(rec->path_heap);
	wtk_bit_heap_delete(rec->align_heap);
	wtk_heap_delete(rec->global_heap);
	wtk_heap_delete(rec->local_heap);
	if(rec->hlda_vector)
	{
		wtk_vector_delete(rec->hlda_vector);
	}
	return 0;
}

int wtk_rec_reset(wtk_rec_t *rec)
{
	int i;

	if(rec->prune)
	{
		wtk_prune_reset(rec->prune);
	}
	wtk_bit_heap_reset(rec->inst_heap);
	for (i = 1; i < rec->tokenset_heap_maxi; ++i)
	{
		if (rec->tokenset_heap[i])
		{
			wtk_bit_heap_reset(rec->tokenset_heap[i]);
		}
	}
	rec->best_emit_score=0;
	rec->word_thresh = rec->gen_thresh = rec->n_thresh = LSMALL;
	wtk_queue_init(&(rec->inst_queue));
	wtk_queue_init(&(rec->path_no_queue));
	wtk_queue_init(&(rec->path_yes_queue));
	wtk_queue_init(&(rec->align_no_queue));
	wtk_queue_init(&(rec->align_yes_queue));
	if (rec->lat)
	{
		wtk_lat_reset(rec->lat);
		rec->lat = 0;
	}
	if (rec->rel_tok_heap)
	{
		wtk_bit_heap_reset(rec->rel_tok_heap);
	}
	if (rec->nxt_path_heap)
	{
		wtk_bit_heap_reset(rec->nxt_path_heap);
	}
	wtk_bit_heap_reset(rec->path_heap);
	wtk_bit_heap_reset(rec->align_heap);
	wtk_heap_reset(rec->local_heap);
	return 0;
}

void wtk_rec_init_tokenset(wtk_rec_t *rec, wtk_tokenset_t *ts)
{
	int ntok = rec->cfg->ntok;

	ts->tok = null_token;
	if (ntok > 1)
	{
		ts->set = (wtk_reltoken_t*) wtk_bit_heap_malloc(rec->rel_tok_heap);
		ts->set[0] = max_token;
		ts->n = 1;
	}
	else
	{
		ts->n = 0;
	}
}

void wtk_rec_clean_tokenset(wtk_rec_t *rec, wtk_tokenset_t *ts)
{
	int ntok = rec->cfg->ntok;

	if (ntok > 1)
	{
		wtk_bit_heap_free(rec->rel_tok_heap, ts->set);
	}
}

float wtk_rec_netnode_wdlike(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_netlink_t *dest, *end;
	wtk_netnode_t *t;
	wtk_hmm_t *hmm;
	float like, best, scale;
	int N;

	best = LZERO;
	scale = rec->cfg->lmscale;
	end = node->links + node->nlinks;
	for (dest = node->links; dest < end; ++dest)
	{
		t = dest->node;
		if (!wtk_netnode_tr0(t))
		{
			break;
		}
		like = dest->like * scale;
		if (like <= best)
		{
			continue;
		}
		if (wtk_netnode_word(t))
		{
			best = like;
		}
		else if (wtk_netnode_wd0(t))
		{
			hmm = t->info.hmm;
			N = hmm->num_state;
			like += hmm->transP[1][N];
			like += wtk_rec_netnode_wdlike(rec, t);
			if (like > best)
			{
				best = like;
			}
		}
	}
	return best;
}

int wtk_rec_move_to_recent(wtk_rec_t *rec, wtk_netinst_t *inst)
{
	//wtk_debug("###################### move inst ######################\n");
	//wtk_netnode_print3(inst->node);
	if (!inst->node)
	{
		return 0;
	}
	//wtk_netnode_print3(inst->node);
	if (inst == rec->nxt_inst)
	{
		rec->nxt_inst = wtk_queue_node_data(inst->q_n.next,wtk_netinst_t,q_n);
	}
	//wtk_debug("###################### touch %s \n",wtk_netnode_name2(inst->node));
	wtk_queue_touch_node(&(rec->inst_queue), &(inst->q_n));
	inst->pxd = 0;
	inst->ooo = 1;
	return 1;
}

/**
 * @brief move word node to the font of the inst queue,when wtk_rec_step_inst1 is
 * called, the word will be recalced.
 *
 *  if word->word ==> there will be loop, push word to the front.
 */
int wtk_rec_reorder_inst(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_netlink_t *dest, *end;
	wtk_netinst_t *inst;
	int ret = 0;

	//wtk_debug("###################### reorder inst ######################\n");
	if (!node->net_inst && !node->net_inst->ooo)
	{
		return ret;
	}
	//wtk_netnode_print(node);
	inst = node->net_inst;
	inst->ooo = 0;
	end = node->links + node->nlinks;
	for (dest = node->links; dest < end; ++dest)
	{
		if (!wtk_netnode_tr0(dest->node))
		{
			break;
		}
		if (dest->node->net_inst)
		{
			ret += wtk_rec_move_to_recent(rec, dest->node->net_inst);
		}
	}
	for (dest = node->links; dest < end; ++dest)
	{
		if (!wtk_netnode_tr0(dest->node))
		{
			break;
		}
		if (dest->node->net_inst)
		{
			ret += wtk_rec_reorder_inst(rec, dest->node);
		}
	}
	return ret;
}

void wtk_rec_detach_inst(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_netinst_t *inst;
	int i, n;

	//wtk_debug("###################### detach inst ######################\n");
	//wtk_netnode_print(node);
	inst = node->net_inst;
	node->net_inst = 0;
	wtk_queue_remove(&(rec->inst_queue), &(inst->q_n));
	wtk_rec_clean_tokenset(rec, inst->exit);
	wtk_bit_heap_free(rec->tokenset_heap[1], inst->exit);
	if (wtk_netnode_hmm(node))
	{
		n = node->info.hmm->num_state - 1;
	}
	else
	{
		n = 1;
	}
	for (i = 0; i < n; ++i)
	{
		wtk_rec_clean_tokenset(rec, inst->state + i);
	}
	wtk_bit_heap_free(rec->tokenset_heap[n], inst->state);
	wtk_bit_heap_free(rec->inst_heap, inst);
}

void wtk_rec_attach_inst(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_bit_heap_t *bit_heap;
	wtk_tokenset_t *cur;
	wtk_tokenset_t *end;
	wtk_netinst_t *inst;
	int n;

	//wtk_debug("###################### attach inst ######################\n");
	//wtk_netnode_print(node);
	inst = (wtk_netinst_t*) wtk_bit_heap_malloc(rec->inst_heap);
	if (wtk_netnode_hmm(node))
	{
		n = node->info.hmm->num_state - 1;
	}
	else
	{
		n = 1;
	}
	inst->node = node;
	node->net_inst = inst;
	bit_heap = rec->tokenset_heap[n];
	if (!bit_heap)
	{
		bit_heap = rec->tokenset_heap[n] = wtk_bit_heap_new(
				sizeof(wtk_tokenset_t) * n, rec->cfg->bit_heap_min,
				rec->cfg->bit_heap_max, rec->cfg->bit_heap_growf);
	}
	inst->state = (wtk_tokenset_t*) wtk_bit_heap_malloc(bit_heap);
	inst->exit = (wtk_tokenset_t*) wtk_bit_heap_malloc(rec->tokenset_heap[1]);
	wtk_rec_init_tokenset(rec, inst->exit);
	end = inst->state + n;
	for (cur = inst->state; cur < end; ++cur)
	{
		wtk_rec_init_tokenset(rec, cur);
	}
	inst->max = LZERO;
	if (wtk_netnode_wd0(node))
	{
		inst->wdlk = wtk_rec_netnode_wdlike(rec, node);
	}
	else
	{
		inst->wdlk = LZERO;
	}
	inst->ooo = 1;
	wtk_queue_push(&(rec->inst_queue), &(inst->q_n));
	//wtk_debug("==========================attach start\n");
	//wtk_rec_trace_inst(rec);
	wtk_rec_reorder_inst(rec, node);
	//wtk_rec_trace_inst(rec);
	//wtk_debug("==========================attach end\n");
}

void wtk_tokenset_cpy(wtk_tokenset_t *dst, wtk_tokenset_t *src)
{
	int i;

	dst->tok = src->tok;
	dst->n = src->n;
	for (i = 0; i < dst->n; ++i)
	{
		dst->set[i] = src->set[i];
	}
}

wtk_path_t *wtk_rec_new_path(wtk_rec_t *rec)
{
	wtk_path_t *path;

	path = (wtk_path_t*) wtk_bit_heap_malloc(rec->path_heap);
	wtk_queue_push_front(&(rec->path_no_queue), &(path->q_n));
	path->queue = &(rec->path_no_queue);
	path->chain = 0;
	path->used = 0;
	path->usage = 0;
	return path;
}

void wtk_rec_ref_align(wtk_rec_t *rec, wtk_align_t *align)
{
	if (align->usage == 0)
	{
		wtk_queue_remove(align->queue, &(align->q_n));
		align->queue = &(rec->align_yes_queue);
		wtk_queue_push_front(align->queue, &(align->q_n));
	}
	++align->usage;
}

wtk_align_t* wtk_rec_new_align(wtk_rec_t *rec, wtk_netnode_t *node, int state,
		double like, int frame, wtk_align_t *prev)
{
	wtk_align_t *align;

	align = (wtk_align_t*) wtk_bit_heap_malloc(rec->align_heap);
	align->usage = 0;
	align->used = 0;
	align->node = node;
	align->state = state;
	align->like = like;
	align->frame = frame;
	align->prev = prev;
	if (prev)
	{
		wtk_rec_ref_align(rec, prev);
	}
	align->queue = &(rec->align_no_queue);
	wtk_queue_push_front(align->queue, &(align->q_n));
	return align;
}

void wtk_rec_ref_path(wtk_rec_t *rec, wtk_path_t* path)
{
	if (path->usage == 0)
	{
		wtk_queue_remove(path->queue, &(path->q_n));
		path->queue = &(rec->path_yes_queue);
		wtk_queue_push_front(path->queue, &(path->q_n));
	}
	++path->usage;
}

wtk_nxtpath_t* wtk_rec_new_nxtpath(wtk_rec_t *rec, wtk_path_t *newpath,
		wtk_reltoken_t *cur)
{
	wtk_nxtpath_t *nxtpath;

	nxtpath = (wtk_nxtpath_t*) wtk_bit_heap_malloc(rec->nxt_path_heap);
	if (!newpath->chain)
	{
		newpath->chain = nxtpath;
	}
	nxtpath->chain = 0;
	nxtpath->like = newpath->like + cur->like;
	nxtpath->lm = cur->lm;
	nxtpath->prev = cur->path;
	if (nxtpath->prev)
	{
		wtk_rec_ref_path(rec, nxtpath->prev);
	}
	nxtpath->align = cur->align;
	if (nxtpath->align)
	{
		wtk_rec_ref_align(rec, nxtpath->align);
	}
	return nxtpath;
}

void wtk_rec_move_path_yes_ref(wtk_rec_t *rec, wtk_path_t *path)
{
	wtk_queue_remove(path->queue, &(path->q_n));
	path->queue = &(rec->path_yes_queue);
	wtk_queue_push_front(path->queue, &(path->q_n));
}

void wtk_rec_move_align_yes_ref(wtk_rec_t *rec, wtk_align_t *align)
{
	wtk_queue_remove(align->queue, &(align->q_n));
	align->queue = &(rec->align_yes_queue);
	wtk_queue_push_front(align->queue, &(align->q_n));
}

void wtk_rec_collect_tokenset_path(wtk_rec_t* rec, wtk_tokenset_t *cur)
{
	wtk_path_t *path;
	wtk_align_t *align;
	int i;

	path = cur->tok.path;
	if (path && !path->used)
	{
		if (path->usage != 0)
		{
			wtk_rec_move_path_yes_ref(rec, path);
		}
		path->used = 1;
	}
	for (i = 1; i < cur->n; ++i)
	{
		path = cur->set[i].path;
		if (path && !path->used)
		{
			if (path->usage != 0)
			{
				wtk_rec_move_path_yes_ref(rec, path);
			}
			path->used = 1;
		}
		align = cur->set[i].align;
		if (align && !align->used)
		{
			if (align->usage != 0)
			{
				wtk_rec_move_align_yes_ref(rec, align);
			}
			align->used = 1;
		}
	}
	align = cur->tok.align;
	if (align && !align->used)
	{
		if (align->usage != 0)
		{
			wtk_rec_move_align_yes_ref(rec, align);
		}
		align->used = 1;
	}
}

void wtk_rec_de_ref_align(wtk_rec_t *rec, wtk_align_t *align)
{
	--align->usage;
	if (align->usage == 0)
	{
		wtk_queue_remove(align->queue, &(align->q_n));
		align->queue = &(rec->align_no_queue);
		wtk_queue_push(&(rec->align_no_queue), &(align->q_n));
	}
}

void wtk_rec_de_ref_path_prev(wtk_rec_t *rec, wtk_path_t *path)
{
	wtk_path_t *pth;
	wtk_nxtpath_t tmp, *cur;

	tmp.prev = path->prev;
	tmp.chain = path->chain;
	for (cur = &tmp; cur; cur = cur->chain)
	{
		pth = cur->prev;
		if (pth)
		{
			--pth->usage;
			if (pth->usage == 0)
			{
				wtk_queue_remove(pth->queue, &(pth->q_n));
				pth->queue = &(rec->path_no_queue);
				wtk_queue_push_front(pth->queue, &(pth->q_n));
			}
		}
	}
}

void wtk_rec_unlink_path(wtk_rec_t* rec, wtk_path_t *path)
{
	wtk_nxtpath_t *pth, *nth;

	wtk_queue_remove(path->queue, &(path->q_n));
	for (pth = path->chain; pth; pth = nth)
	{
		nth = pth->chain;
		//release npath.
		wtk_bit_heap_free(rec->nxt_path_heap, pth);
	}
	path->prev = 0;
	path->usage = 0;
	path->chain = 0;
	path->frame = -1;
	wtk_bit_heap_free(rec->path_heap, path);
}

void wtk_rec_unlink_align(wtk_rec_t *rec, wtk_align_t *align)
{
	wtk_queue_remove(align->queue, &(align->q_n));
	align->prev = 0;
	align->usage = 0;
	wtk_bit_heap_free(rec->align_heap, align);
}

void wtk_rec_collect_path(wtk_rec_t *rec)
{
	wtk_netinst_t *inst;
	wtk_tokenset_t *cur;
	wtk_queue_node_t *n, *p;
	wtk_path_t *path;
	wtk_align_t *align;
	int i, j;

	for (n = rec->inst_queue.pop; n; n = n->next)
	{
		inst = data_offset(n,wtk_netinst_t,q_n);
		if (!inst->node)
		{
			continue;
		}
		j = wtk_netnode_hmm(inst->node) ?
				(inst->node->info.hmm->num_state - 1) : 1;
		for (i = 1, cur = inst->state; i <= j; ++i, ++cur)
		{
			wtk_rec_collect_tokenset_path(rec, cur);
		}
		wtk_rec_collect_tokenset_path(rec, inst->exit);
	}
	for (n = rec->path_no_queue.pop; n; n = p)
	{
		p = n->next;
		path = data_offset(n,wtk_path_t,q_n);
		if (!path->used)
		{
			if (path->align)
			{
				wtk_rec_de_ref_align(rec, path->align);
			}
			wtk_rec_de_ref_path_prev(rec, path);
			wtk_rec_unlink_path(rec, path);
		}
		else
		{
			path->used = 0;
		}
	}
	for (n = rec->path_yes_queue.pop; n; n = n->next)
	{
		path = data_offset(n,wtk_path_t,q_n);
		if (!path->used)
		{
			break;
		}
		path->used = 0;
	}
	rec->cpath = rec->path_yes_queue.length + rec->path_no_queue.length;
	for (n = rec->align_no_queue.pop; n; n = p)
	{
		p = n->next;
		align = data_offset(n,wtk_align_t,q_n);
		if (!align->used)
		{
			if (align->prev)
			{
				wtk_rec_de_ref_align(rec, align->prev);
			}
			wtk_rec_unlink_align(rec, align);
		}
		else
		{
			align->used = 0;
		}
	}
	for (n = rec->align_yes_queue.pop; n; n = n->next)
	{
		align = data_offset(n,wtk_align_t,q_n);
		if (!align->used)
		{
			break;
		}
		align->used = 0;
	}
	rec->calign = rec->align_yes_queue.length + rec->align_no_queue.length;
}

void wtk_rec_step_word2(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_netinst_t *inst = node->net_inst;
	wtk_path_t *newpath;
	wtk_reltoken_t *cur, *end;
	wtk_nxtpath_t *nxtpath;

	if (!node->info.pron)
	{
		wtk_tokenset_cpy(inst->exit, inst->state);
	}
	else
	{
		//only one state for word node.
		inst->exit->tok = inst->state->tok;
		inst->exit->tok.like += rec->cfg->wordpen
				+ node->info.pron->prob * rec->cfg->pscale;
		newpath = wtk_rec_new_path(rec);
		newpath->node = node;
		newpath->usage = 0;
		newpath->frame = rec->frame;
		newpath->like = inst->exit->tok.like;
		newpath->lm = inst->exit->tok.lm;
		newpath->align = inst->exit->tok.align;
		if (newpath->align)
		{
			wtk_rec_ref_align(rec, newpath->align);
		}
		inst->exit->tok.path = newpath;
		inst->exit->tok.lm = 0;
		inst->exit->tok.align = 0;
		newpath->prev = inst->state->tok.path;
		if (newpath->prev)
		{
			wtk_rec_ref_path(rec, newpath->prev);
		}
		if (rec->cfg->ntok > 1)
		{
			inst->exit->n = 1;
			inst->exit->set[0].path = newpath;

			//wtk_tokenset_print(inst->state);
			if (inst->state->n > 1)
			{
				cur = inst->state->set + 1;
				nxtpath = wtk_rec_new_nxtpath(rec, newpath, cur);
				end = inst->state->set + inst->state->n;
				for (++cur; cur < end; ++cur)
				{
					nxtpath->chain = wtk_rec_new_nxtpath(rec, newpath, cur);
					nxtpath = nxtpath->chain;
				}
			}
		}
		else
		{
			inst->exit->n = 0;
			newpath->chain = 0;
		}
	}
}

wtk_path_t* wtk_path_recent_pron_path(wtk_path_t *path)
{
	while (path && !path->node->info.pron)
	{
		path = path->prev;
	}
	return path;
}

void wtk_rec_merge_tokenset(wtk_rec_t *rec, wtk_tokenset_t *res,
		wtk_token_t *cmp, wtk_tokenset_t *src)
{
	float n_thresh = rec->n_thresh;
	wtk_tokenset_t tmp;
	wtk_reltoken_t *cur, *mch;
	wtk_path_t *path;
	wtk_reltoken_t *rtoks = rec->rtoks_merge;
	wtk_netnode_t *n;
	wtk_netnode_t **pnode;
	wtk_array_t *node_array = rec->node_array;
	float like, diff, limit;
	int i, null, aux, j;

	if (cmp->like >= res->tok.like)
	{
		if (cmp->like > n_thresh)
		{
			if (res->tok.like > n_thresh)
			{
				//save resource to tmp;
				tmp.tok = res->tok;
				for (i = 0; i < res->n; ++i)
				{
					rtoks[i] = res->set[i];
				}
				tmp.n = res->n;
				tmp.set = rtoks;
				//copy src to resource
				res->tok = *cmp;
				for (i = 0; i < src->n; ++i)
				{
					res->set[i] = src->set[i];
				}
				res->n = src->n;
			}
			else
			{
				//res is tool small, just copy src;
				res->tok = *cmp;
				for (i = 0; i < src->n; ++i)
				{
					res->set[i] = src->set[i];
				}
				res->n = src->n;
				return;
			}
		}
		else
		{
			//no need merge for too small.
			return;
		}
	}
	else
	{
		if (cmp->like > n_thresh)
		{
			tmp.tok = *cmp;
			tmp.set = src->set;
			tmp.n = src->n;
		}
		else
		{
			//no need merge for too small.
			return;
		}
	}
	wtk_array_reset(node_array);
	for (i = null = 0, cur = res->set; i < res->n; ++i, ++cur)
	{
		path = wtk_path_recent_pron_path(cur->path);
		if (path)
		{
			n = path->node;
			n->aux = i + 1;
			pnode = (wtk_netnode_t**) wtk_array_push(node_array);
			pnode[0] = n;
		}
		else
		{
			null = i + 1;
		}
	}
	limit = n_thresh - tmp.tok.like;
	diff = res->tok.like - tmp.tok.like;
	for (i = 0, cur = tmp.set; i < tmp.n; ++i, ++cur)
	{
		if (cur->like < limit)
		{
			break;
		}
		path = wtk_path_recent_pron_path(cur->path);
		if (path)
		{
			n = path->node;
			aux = n->aux;
		}
		else
		{
			n = 0;
			aux = null;
		}
		like = cur->like - diff;
		mch = 0;
		//find match tok/path if one exists.
		if (aux != 0)
		{
			for (j = aux - 1; j < res->n; ++j)
			{
				path = wtk_path_recent_pron_path(res->set[j].path);
				if (!path)
				{
					if (!n || !n->info.pron->word)
					{
						mch = res->set + j;
						break;
					}
				}
				else if (n && n->info.pron->word && path->node == n)
				{
					mch = res->set + j;
					break;
				}
			}
		}
		if (!mch)
		{
			if (res->n < rec->cfg->ntok)
			{
				mch = res->set + res->n;
				++res->n;
				*mch = rnull_token;
			}
			else
			{
				mch = res->set + res->n - 1;
			}
		}
		if (like > mch->like)
		{
			for (--mch; like > mch->like; --mch)
			{
				mch[1] = mch[0];
			}
			++mch;
			mch->path = cur->path;
			mch->lm = cur->lm;
			mch->align = cur->align;
			mch->like = like;
		}
	}
	pnode = (wtk_netnode_t**) node_array->slot;
	for (i = 0; i < node_array->nslot; ++i)
	{
		pnode[i]->aux = 0;
	}
}

void wtk_rec_step_hmm2(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_hmm_t *hmm = node->info.hmm;
	wtk_netinst_t *inst = node->net_inst;
	wtk_tokenset_t cmp, *res, *cur;
	wtk_align_t *align;
	int n, N;
	double like;

	N = hmm->num_state;
	cur = inst->state;
	res = inst->exit;
	cmp.tok = cur->tok;
	cmp.tok.like += hmm->transP[1][N];
	if (res->n == 0)
	{
		if (cmp.tok.like > res->tok.like)
		{
			res->tok = cmp.tok;
		}
	}
	else
	{
		wtk_rec_merge_tokenset(rec, res, &(cmp.tok), cur);
	}
	if (rec->cfg->model)
	{
		like = res->tok.like - res->tok.lm * rec->cfg->lmscale;
		align = wtk_rec_new_align(rec, node, -1, like, rec->frame,
				res->tok.align);
		res->tok.align = align;
		if (rec->cfg->ntok > 1)
		{
			res->set[0].align = align;
		}
		for (n = 1; n < res->n; ++n)
		{
			align = wtk_rec_new_align(rec, node, -1, like, rec->frame,
					res->set[n].align);
			res->set[n].align = align;
		}
	}
}

void wtk_rec_set_entry_state(wtk_rec_t *rec, wtk_netnode_t *node,
		wtk_tokenset_t *src)
{
	wtk_netinst_t *inst;
	wtk_tokenset_t *res;

	if (!node->net_inst)
	{
		wtk_rec_attach_inst(rec, node);
	}
	inst = node->net_inst;
	res = inst->state;
	if (res->n == 0)
	{
		if (src->tok.like > res->tok.like)
		{
			res->tok = src->tok;
		}
	}
	else
	{
		wtk_rec_merge_tokenset(rec, res, &src->tok, src);
	}
	if (res->tok.like > inst->max)
	{
		inst->max = res->tok.like;
	}
	if (node->type == n_word
			&& (!rec->p_word_max_node || !rec->p_word_max_node->net_inst
					|| (res->tok.like > rec->p_word_max_node->net_inst->max)))
	{
		rec->p_word_max_node = node;
	}
}

void wtk_rec_step_inst2(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_token_t tok;
	wtk_tokenset_t xtok;
	wtk_netinst_t *inst = node->net_inst;
	wtk_netlink_t *dest;
	float lm;
	int i, j;

	if (wtk_netnode_word(node))
	{
		wtk_rec_step_word2(rec, node);
	}
	else if (wtk_netnode_tr0(node))
	{
		wtk_rec_step_hmm2(rec, node);
	}
	tok = inst->exit->tok;
	xtok.set = rec->rtoks_inst2;
	wtk_tokenset_cpy(&xtok, inst->exit);
	if (wtk_netnode_word(node))
	{
		if (tok.like < rec->word_thresh)
		{
			tok = null_token;
		}
	}
	if (tok.like > rec->gen_thresh)
	{
		for (i = 0, dest = node->links; i < node->nlinks; ++i, ++dest)
		{
			lm = dest->like;
			//wtk_debug("toklike=%f,lm=%f,ls=%f\n",tok.like,lm,rec->cfg->lmscale);
			xtok.tok.like = tok.like + lm * rec->cfg->lmscale;
			if (xtok.tok.like > rec->gen_thresh)
			{
				xtok.tok.lm = tok.lm + lm;
				for (j = 0; j < xtok.n; ++j)
				{
					xtok.set[j].lm = inst->exit->set[j].lm + lm;
				}
				wtk_rec_set_entry_state(rec, dest->node, &xtok);
			}
		}
	}
	inst->pxd = 1;
}

void wtk_rec_step_hmm1_state_merge(wtk_rec_t *rec, wtk_netinst_t *inst,
		wtk_tokenset_t *res, wtk_hmm_t *hmm, int j)
{
	wtk_tokenset_t *cur;
	wtk_token_t cmp;
	int i, endi;
	short **seIndex;

	seIndex = rec->seIndexes[hmm->tIdx];
	i = seIndex[j][0];
	endi = seIndex[j][1];
	cur = inst->state + i - 1;
	wtk_tokenset_cpy(res, cur);
	res->tok.like += hmm->transP[i][j];
	for (++i, ++cur; i <= endi; ++i, ++cur)
	{
		cmp = cur->tok;
		cmp.like += hmm->transP[i][j];
		if (res->n == 0)
		{
			if (cmp.like > res->tok.like)
			{
				res->tok = cmp;
			}
		}
		else
		{
			wtk_rec_merge_tokenset(rec, res, &cmp, cur);
		}
	}
}

void wtk_rec_step_hmm1_state_align(wtk_rec_t *rec, wtk_netinst_t *inst,
		wtk_tokenset_t *res, wtk_hmm_t *hmm, int j, wtk_token_t *max)
{
	wtk_netnode_t *node = inst->node;
	wtk_align_t *align;
	wtk_reltoken_t *set;
	float outp, like;
	int n;

	if (res->tok.like > rec->gen_thresh)
	{
		outp = wtk_rec_calc_prob(rec, rec->obs, hmm->pState[j]);
		like = res->tok.like - res->tok.lm * rec->cfg->lmscale;
		res->tok.like += outp;
		if(rec->prune)
		{
			//wtk_debug("score=%f\n",score);
			//wtk_debug("like=%f/%f\n",res->tok.like,rec->best_emit_score);
			wtk_prune_add(rec->prune,res->tok.like-rec->best_emit_score);
			//wtk_histogram_add_score(r->histogram,score,LZERO);
		}
		if (res->tok.like > max->like)
		{
			*max = res->tok;
		}
		if (rec->cfg->state)
		{
			//empty align or state not equal in the same hmm node or in different hmm node.
			if (!res->tok.align || res->tok.align->state != j
					|| res->tok.align->node != node)
			{
				align = wtk_rec_new_align(rec, node, j, like, rec->frame - 1,
						res->tok.align);
				res->tok.align = align;
				if (rec->cfg->ntok > 1)
				{
					res->set[0].align = align;
				}
			}
			for (n = 1; n < res->n; ++n)
			{
				set = res->set + n;
				if (!set->align || set->align->state != j
						|| set->align->node != node)
				{
					align = wtk_rec_new_align(rec, node, j, like,
							rec->frame - 1, set->align);
					set->align = align;
				}
			}
		}
	}
	else
	{
		res->tok = null_token;
		res->n = (rec->cfg->ntok > 1) ? 1 : 0;
	}
}

void wtk_rec_step_hmm1_exit_align(wtk_rec_t *rec, wtk_netinst_t *inst,
		wtk_tokenset_t* res, wtk_hmm_t *hmm)
{
	wtk_netnode_t *node = inst->node;
	wtk_align_t *align;
	wtk_token_t tok =
	{ 0, 0, 0, 0 };
	float like;
	int n;

	if (res->tok.like > LSMALL)
	{
		tok.like = res->tok.like + inst->wdlk;
		//wtk_netnode_print(inst->node);
		//wtk_debug("wdlk=%f,tok->like=%f\n",inst->wdlk,tok.like);
		if (tok.like > rec->p_word_max_tok.like)
		{
			rec->p_word_max_tok = tok;
			rec->p_word_max_node = node;
		}
		if (rec->cfg->model && !wtk_netnode_tr0(node))
		{
			like = res->tok.like - res->tok.lm * rec->cfg->lmscale;
			align = wtk_rec_new_align(rec, node, -1, like, rec->frame,
					res->tok.align);
			res->tok.align = align;
			if (rec->cfg->ntok > 1)
			{
				res->set[0].align = align;
			}
			for (n = 1; n < res->n; ++n)
			{
				align = wtk_rec_new_align(rec, node, -1, like, rec->frame,
						res->set[n].align);
				res->set[n].align = align;
			}
		}
	}
	else
	{
		inst->exit->tok = null_token;
		inst->exit->n = (rec->cfg->ntok > 1) ? 1 : 0;
	}
}

void wtk_rec_step_hmm1(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_netinst_t *inst;
	wtk_token_t max;
	wtk_hmm_t *hmm;
	wtk_tokenset_t *res, *cur;
	int i, j, N;

	inst = node->net_inst;
	max = null_token;
	hmm = node->info.hmm;
	N = hmm->num_state;
	for (j = 2, res = rec->sBuf + 2; j < N; ++j, ++res)
	{
		wtk_rec_step_hmm1_state_merge(rec, inst, res, hmm, j);
		wtk_rec_step_hmm1_state_align(rec, inst, res, hmm, j, &max);
	}
	for (i = 1, res = rec->sBuf + 1, cur = inst->state; i < N;
			++i, ++res, ++cur)
	{
		wtk_tokenset_cpy(cur, res);
	}
	if (max.like > rec->p_gen_max_tok.like)
	{
		rec->p_gen_max_tok = max;
		rec->p_gen_max_node = node;
	}
	inst->max = max.like;
	res = inst->exit;
	wtk_rec_step_hmm1_state_merge(rec, inst, res, hmm, N);
	wtk_rec_step_hmm1_exit_align(rec, inst, res, hmm);
}

void wtk_rec_step_word1(wtk_rec_t *rec, wtk_netnode_t *node)
{
	wtk_netinst_t *inst;

	inst = node->net_inst;
	inst->exit->tok = inst->state->tok = null_token;
	inst->state->n = (rec->cfg->ntok > 1) ? 1 : 0;
	inst->max = LZERO;
}

void wtk_rec_step_inst1(wtk_rec_t *rec, wtk_netnode_t *node)
{
	//wtk_netnode_trace(node);
	if (wtk_netnode_hmm(node))
	{
		wtk_rec_step_hmm1(rec, node);
	}
	else
	{
		//set likelihood of word to LZERO.
		wtk_rec_step_word1(rec, node);
	}
	node->net_inst->pxd = 0;
}

void wtk_rec_step_word(wtk_rec_t *rec)
{
	wtk_netinst_t *inst;
	wtk_queue_node_t *n1, *n2 = 0;

	//wtk_debug("%d\n",rec->inst_queue.length);
	for (n1 = rec->inst_queue.pop; n1; n1 = n2)
	{
		inst = wtk_queue_node_data(n1,wtk_netinst_t,q_n);
		//wtk_netnode_print3(inst->node);
		//wtk_debug("max=%f/%f\n",inst->max,rec->gen_thresh);
		if (inst->max < rec->gen_thresh)
		{
			n2 = n1->next;
			wtk_rec_detach_inst(rec, inst->node);
		}
		else
		{
			rec->nxt_inst = inst;
			wtk_rec_step_inst2(rec, inst->node);
			n2 = rec->nxt_inst->q_n.next;
		}
	}
	//wtk_debug("%d\n",rec->inst_queue.length);
	//exit(0);
}

void wtk_rec_feed(wtk_rec_t *rec, wtk_vector_t* obs)
{
	static float half_small = LSMALL / 2;
	wtk_rec_cfg_t *cfg = (rec->cfg);
	wtk_netinst_t *inst;
	wtk_queue_node_t *n;

#ifdef DEBUG_NET
	wtk_vector_print(obs);
#endif
	++rec->frame;
	//wtk_debug("v[%d]=%d\n",rec->frame,rec->inst_queue.length);
	rec->sBuf[1].n = (rec->cfg->ntok > 1) ? 1 : 0;
	if(rec->hlda_vector)
	{
		wtk_matrix_multiply_vector(rec->hlda_vector,rec->cfg->hlda_matrix,obs);
		rec->obs=rec->hlda_vector;
	}else
	{
		rec->obs = obs;
	}
	rec->p_gen_max_node = rec->p_word_max_node = 0;
	rec->p_gen_max_tok = rec->p_word_max_tok = null_token;
	//wtk_rec_trace_like(rec);
	if(rec->prune)
	{
		wtk_prune_reset(rec->prune);
	}
	for (n = rec->inst_queue.pop; n; n = n->next)
	{
		inst = wtk_queue_node_data(n,wtk_netinst_t,q_n);
		if (inst->node)
		{
			wtk_rec_step_inst1(rec, inst->node);
		}
	}
	//wtk_rec_trace_like(rec);
	rec->word_thresh = rec->p_word_max_tok.like - cfg->word_beam;
	if (rec->word_thresh < LSMALL)
	{
		rec->word_thresh = LSMALL;
	}
	if(rec->prune)
	{
		rec->gen_thresh=wtk_prune_get_thresh(rec->prune)+rec->best_emit_score;
		//wtk_debug("v[%d]:%f/%f inst=%d\n",rec->frame,rec->gen_thresh,rec->p_gen_max_tok.like - cfg->gen_beam,rec->inst_queue.length);
		rec->best_emit_score=rec->prune->max+rec->best_emit_score;
	}else
	{
		rec->gen_thresh = rec->p_gen_max_tok.like - cfg->gen_beam;
	}
	if (rec->gen_thresh < LSMALL)
	{
		rec->gen_thresh = LSMALL;
	}
	if (cfg->ntok > 1)
	{
		rec->n_thresh = rec->p_gen_max_tok.like - cfg->n_beam;
		if (rec->n_thresh < half_small)
		{
			rec->n_thresh = half_small;
		}
	}
	wtk_rec_step_word(rec);
	if (((rec->path_yes_queue.length + rec->path_no_queue.length - rec->cpath)
			> cfg->path_coll_thresh)
			|| (rec->align_no_queue.length + rec->align_yes_queue.length
					- rec->calign) > cfg->align_coll_thresh)
	{
		wtk_rec_collect_path(rec);
	}
	//wtk_rec_trace_inst(rec);
}

void wtk_rec_feed2(wtk_rec_t *r,wtk_feat_t *f)
{
	if(r->dnn_get)
	{
		//wtk_debug("frame=%d f=%p app=%p index=%d cnt=%d\n",r->frame,f,f->app_hook,f->index,r->dnn_cnt);
		if(!f->app_hook)
		{
			r->f=f;
			//++r->cache_frame;
		}else
		{
			//++r->frame;
			f=(wtk_feat_t*)(f->app_hook);
		}
		wtk_rec_feed(r,f->rv);
	}else
	{
		if(f->app_hook)
		{
			f=(wtk_feat_t*)(f->app_hook);
			wtk_rec_feed(r,f->rv);
		}else
		{
			r->f=f;
			wtk_rec_feed(r,f->rv);
		}
	}
}

int wtk_rec_start(wtk_rec_t *rec, wtk_lat_t *lat)
{
	wtk_netinst_t *inst;
	int i;

	if (rec->sPre)
	{
		for (i = 0; i < rec->nsp; ++i)
		{
			rec->sPre[i].id = -1;
		}
	}
	if (rec->mPre)
	{
		for (i = 0; i < rec->nmp; ++i)
		{
			rec->mPre[i].id = -1;
		}
	}
	rec->calign = rec->cpath = 0;
	rec->lat = lat;
	rec->frame = 0;
	lat->initial.net_inst = lat->final.net_inst = 0;
	wtk_rec_attach_inst(rec, &(lat->initial));
	inst = lat->initial.net_inst;
	inst->state->tok.lm = inst->state->tok.like = inst->max = 0;
	inst->state->tok.path = 0;
	inst->state->n = rec->cfg->ntok > 1 ? 1 : 0;
	rec->p_gen_max_node = rec->p_word_max_node = 0;
	rec->p_gen_max_tok = rec->p_word_max_tok = null_token;
	rec->best_emit_score=0;
	wtk_rec_step_word(rec);
	//wtk_debug("state: %d\n",rec->cfg->state);
	return 0;
}

void wtk_nxtpath_from_path(wtk_nxtpath_t* nxt_path, wtk_path_t* path)
{
	nxt_path->prev = path->prev;
	nxt_path->like = path->like;
	nxt_path->lm = path->lm;
	nxt_path->align = path->align;
	nxt_path->chain = path->chain;
}

void wtk_rec_lat_from_path(wtk_rec_t *rec, wtk_lat_t *lat, wtk_path_t *path,
		int *ln)
{
	wtk_dict_word_t *null_word;
	wtk_nxtpath_t tmp, *nxt_path;
	wtk_lnode_t *ne, *ns;
	wtk_align_t *align, *al, *pr;
	wtk_larc_t *la;
	wtk_rec_cfg_t *cfg = (rec->cfg);
	wtk_heap_t *heap = rec->local_heap;
	wtk_string_t *labpr = 0, *labid;
	//wtk_label_t *label=rec->hmmlist->label;
	double prlike, wp, like, dur;
	float frame_dur = rec->frame_dur;
	int i, frame, n;
	char buf[64];
	wtk_hmm_t *hmm;

	null_word = rec->dict->null_word;
	wtk_nxtpath_from_path(&tmp, path);
	ne = lat->lnodes - path->usage;
	ne->time = path->frame * rec->frame_dur;
	ne->info.word =
			path->node->info.pron ? path->node->info.pron->word : null_word;
	if(path->node->info.pron)
	{
		ne->v=path->node->info.pron->pnum;
	}else
	{
		ne->v=1;
	}
	ne->score = path->like;
	align = path->align;
	for (nxt_path = &tmp; nxt_path; nxt_path = nxt_path->chain)
	{
		la = lat->larcs + (++(*ln));
		if (nxt_path->prev)
		{
			ns = lat->lnodes - nxt_path->prev->usage;
			prlike = nxt_path->prev->like;
		}
		else
		{
			ns = lat->lnodes;
			prlike = 0;
		}
		la->start = ns;
		la->end = ne;
		wp = (!ne->info.word || ne->info.word == null_word) ? 0 : cfg->wordpen;
		//acoustic like.
		la->aclike = nxt_path->like - prlike - nxt_path->lm * cfg->lmscale - wp;
		if (path->node->info.pron)
		{
			la->prlike = path->node->info.pron->prob;
			la->aclike -= la->prlike * cfg->pscale;
		}
		else
		{
			la->prlike = 0;
		}
		la->lmlike = nxt_path->lm;
		la->score = nxt_path->like;
		la->farc = ns->foll;
		la->parc = ne->pred;
		ns->foll = ne->pred = la;
		if (nxt_path->prev && !ns->info.word)
		{
			//if start word not bind,bind start word.
			wtk_rec_lat_from_path(rec, lat, nxt_path->prev, ln);
		}
		align = nxt_path->align;
		if (align)
		{
			for (i = 0, al = align; al; al = al->prev, ++i)
				;
			la->nalign = i;
			la->lalign = (wtk_lalign_t*) wtk_heap_malloc(heap,
					sizeof(wtk_lalign_t) * i);
			frame = path->frame;
			//Allow for wp diff between path and align
			like = nxt_path->like - nxt_path->lm * cfg->lmscale - wp;
			for (pr = 0, al = align; al; al = al->prev)
			{
				hmm = al->node->info.hmm;
				//wtk_debug("hmm: %.*s\n",hmm->name->len,hmm->name->data);
				//wtk_debug("pr=%p,like=%f\n",pr,pr?pr->like:-1);
				if (al->state < 0)
				{
					if (!pr)
					{
						pr = al;
						labpr = hmm->name;
						//wtk_netnode_print(pr->node);
						continue;
					}
					dur = (pr->frame - al->frame) * frame_dur; //lat->frame_dur;
					like = pr->like - al->like;
					pr = al;
					labid = labpr;
					labpr = hmm->name;
				}
				else
				{
					n = sprintf(buf, "s%d", al->state);
					labid = wtk_heap_dup_string(heap, buf, n);
					//labid=wtk_label_find(label,buf,n,1)->name;
					dur = (frame - al->frame) * frame_dur;
					like = like - al->like;
					frame = al->frame;
				}
				--i;
				la->lalign[i].state = al->state;
				la->lalign[i].name = labid;
				la->lalign[i].dur = dur;
				la->lalign[i].like = like;
				like = al->like;
			}
			if (pr)
			{
				if (nxt_path->prev)
				{
					dur = (pr->frame - nxt_path->prev->frame) * frame_dur;
					like = pr->like - nxt_path->prev->like;
				}
				else
				{
					dur = pr->frame * frame_dur;
					like = pr->like;
				}
				--i;
				la->lalign[i].state = -1;
				la->lalign[i].name = labpr;
				la->lalign[i].dur = dur;
				la->lalign[i].like = like;
			}
		}
	}
}

void wtk_path_mark(wtk_path_t *path, int *nn, int *nl)
{
	wtk_nxtpath_t *nxt_path;

	if (path->usage >= 0)
	{
		path->usage = -(*nn)++;
		++(*nl);
		if (path->prev)
		{
			wtk_path_mark(path->prev, nn, nl);
		}
		//wtk_debug("nxt_path=%p.\n",path->chain);
		for (nxt_path = path->chain; nxt_path; nxt_path = nxt_path->chain)
		{
			++*(nl);
			if (nxt_path->prev)
			{
				wtk_path_mark(nxt_path->prev, nn, nl);
			}
		}
	}
}

wtk_lat_t* wtk_rec_create_lat(wtk_rec_t* rec, wtk_tokenset_t *res)
{
	wtk_lat_t* lat;
	wtk_netnode_t node;
	wtk_path_t path;
	wtk_nxtpath_t *rth;
	wtk_reltoken_t *cur;
	int i, nn, nl, ln;
	wtk_rec_cfg_t *cfg = (rec->cfg);

	node.type = n_word;
	node.nlinks = 0;
	node.info.pron = 0;
	path.node = &node;
	path.like = res->tok.like;
	path.lm = res->tok.lm;
	path.usage = 0;
	path.align = res->tok.align;
	path.frame = rec->frame;
	path.prev = res->tok.path;
	if (res->n > 1)
	{
		rth = rec->rths;
		path.chain = rth + 1;
		for (i = 1, cur = res->set + 1; i < res->n; ++i, ++cur)
		{
			rth[i].like = res->tok.like + cur->like;
			rth[i].lm = cur->lm;
			rth[i].prev = cur->path;
			rth[i].align = cur->align;
			rth[i].chain = 0;
			rth[i - 1].chain = rth + i;
		}
	}
	else
	{
		path.chain = 0;
	}
	nn = 1;
	nl = 0;
	wtk_path_mark(&path, &nn, &nl);
	lat = wtk_lat_new_h(rec->local_heap);
	wtk_lat_create(lat, nn, nl);
	lat->lmscale = cfg->lmscale;
	lat->wdpenalty = cfg->wordpen;
	lat->prscale = cfg->pscale;

	lat->lnodes[0].time = 0;
	lat->lnodes[0].info.word = 0;
	lat->lnodes[0].score = 0;
	ln = -1;
	wtk_rec_lat_from_path(rec, lat, &path, &ln);
	//wtk_lat_print3(lat,stdout);
	return lat;
}

wtk_transcription_t* wtk_rec_finish(wtk_rec_t *rec)
{
	wtk_transcription_t *trans = 0;
	wtk_lat_t *lat;

	lat = wtk_rec_finish2(rec);
	if (!lat)
	{
		goto end;
	}
	//wtk_lat_print(lat);
	trans = wtk_rec_transcription_from_lat(rec, lat);
	wtk_lat_clean(lat);
end:
	return trans;
}

wtk_lat_t* wtk_rec_finish2(wtk_rec_t *rec)
{
	wtk_lat_t *lat = rec->lat;
	wtk_netinst_t *inst = lat->final.net_inst;
	wtk_lat_t *new_lat = 0;
	wtk_tokenset_t dummy;
	wtk_reltoken_t rtok;

	//wtk_netnode_print(rec->p_gen_max_tok.align->node);
	rec->is_forceout = inst ? 0 : 1;
	//wtk_debug("forceout=%d\n",rec->is_forceout);
	if (inst)
	{
		//wtk_tokenset_print(inst->exit);
		//wtk_path_print(inst->exit->tok.path);
		new_lat = wtk_rec_create_lat(rec, inst->exit);
	}
	else if (rec->p_gen_max_tok.path)
	{
		//wtk_path_print(rec->p_gen_max_tok.path);
		dummy.n = (rec->cfg->ntok > 1) ? 1 : 0;
		dummy.tok = rec->p_gen_max_tok;
		dummy.set = &(rtok);
		dummy.set[0].like = 0.0;
		dummy.set[0].path = dummy.tok.path;
		dummy.set[0].lm = dummy.tok.lm;
		new_lat = wtk_rec_create_lat(rec, &dummy);
	}
	else
	{
		goto end;
	}
end:
	return new_lat;
}

double wtk_rec_time(wtk_rec_t *rec)
{
	return rec->frame * rec->frame_dur;
}

int wtk_token_path_index(wtk_token_t *tok)
{
	int index;
	wtk_path_t *p;

	for (index = 0, p = tok->path; p; p = p->prev, ++index)
	{
	}
	return index;
}

void wtk_token_print(wtk_token_t* t)
{
	wtk_netnode_t *n;
	//wtk_string_t *v;

	wtk_debug("===================== \n");
	if (t->path && t->path->node)
	{
		n = t->path->node;
		if (n->type == n_word)
		{
			//v=n->info.pron->outsym;
			wtk_debug("word:\n");
		}
		else if (n->type == n_hmm)
		{
			wtk_debug("hmm:\n");
		}
	}
}

double wtk_rec_sil(wtk_rec_t *rec)
{
	wtk_netinst_t *inst = rec->lat->final.net_inst;
	double t = 0;
	wtk_path_t *path;
	wtk_align_t *a;
	wtk_string_t *name;

	if (!inst || !inst->exit || !inst->exit)
	{
		goto end;
	}
	path = inst->exit->tok.path;
	if (!path || !path->node)
	{
		goto end;
	}
	a = path->align;
	name = path->node->info.pron->outsym;
	printf("%f,%d,%d,dur=%f,like=%f,%*.*s\n", rec->frame_dur, rec->frame,
			a->frame, (rec->frame - a->frame) * rec->frame_dur, path->like,
			name->len, name->len, name->data);
	end:
	wtk_debug("time=%f\n", t);
	return 0;
}

int wtk_rec_max_is_sil(wtk_rec_t *rec)
{
	wtk_netnode_t *node = rec->p_gen_max_node;
	//wtk_netnode_t *wn;
	wtk_string_t *str;
	//char *str;
	int ret = 0;

	if (!node)
	{
		goto end;
	}
	//str=wtk_netnode_name2(node);
	//wtk_debug("%d: %s\n",node->type,str);
	if (node->type & n_hmm)
	{
		str = node->info.hmm->name;
		if (wtk_string_cmp_s(str,"sp") == 0 || wtk_string_cmp_s(str,"sil") == 0)
		{
			ret = 1;
		}
	}
	/*
	 wn=wtk_netnode_wn(node);
	 if(!wn){goto end;}
	 str=wn->info.pron->outsym;
	 wtk_debug("%.*s\n",str->len,str->data);
	 if(wtk_string_cmp_s(str,"sil")==0)
	 {
	 ret=1;
	 }
	 */
	end: return ret;
}

void wtk_path_print(wtk_path_t *path)
{
	wtk_netnode_print(path->node);
	wtk_debug("align=%p\n", path->align);
}

void wtk_path_trace(wtk_path_t *path)
{
	if (path->prev)
	{
		wtk_path_trace(path->prev);
	}
	wtk_netnode_print(path->node);
}

void wtk_rec_trace(wtk_rec_t *rec)
{
	wtk_netinst_t *inst;
	wtk_queue_node_t *n1;

	for (n1 = rec->inst_queue.pop; n1; n1 = n1->next)
	{
		inst = wtk_queue_node_data(n1,wtk_netinst_t,q_n);
		//wtk_debug("inst=%p,node=%p\n",inst,inst->node);
		if (!inst->node)
		{
			break;
		}wtk_debug("inst=%p,path=%p\n", inst, inst->exit->tok.path);
		if (inst->exit->tok.path)
		{
			printf("++++++++++++++++++++++++++++++\n");
			wtk_netnode_print(inst->node);
			printf("-------------------------------\n");
			wtk_path_trace(inst->exit->tok.path);
			printf("++++++++++++++++++++++++++++++\n");
			//getchar();
		}
	}wtk_debug("trace rec: %d...\n", rec->inst_queue.length);
	//getchar();
}

void wtk_rec_trace_like(wtk_rec_t *rec)
{
	wtk_queue_node_t *n;
	wtk_netinst_t *inst;

	printf("-----------------------------------\n");
	for (n = rec->inst_queue.pop; n; n = n->next)
	{
		inst = wtk_queue_node_data(n,wtk_netinst_t,q_n);
		wtk_netnode_trace(inst->node);
	}
}

void wtk_tokenset_print(wtk_tokenset_t *ts)
{
	char *name;
	int i;
	wtk_path_t *path;

	wtk_debug("========== tokenset %p============\n", ts);
	printf("tokelike: %f\n", ts->tok.like);
	printf("lm: %f\n", ts->tok.lm);
	if (ts->tok.align)
	{
		printf("align: %f\n", ts->tok.align->like);
	}
	for (i = 0; i < ts->n; ++i)
	{
		printf("[%d/%d]: like=%f,lm=%f\n", i, ts->n, ts->set[i].like,
				ts->set[i].lm);
		path = (ts->set + i)->path;
		if (path)
		{
			name = wtk_netnode_name2(path->node);
			printf("[%d/%d]: %s(%p),like=%f,lm=%f\n", i, ts->n, name, path,
					path->like, path->lm);
		}
		else
		{
			name = "NULL PATH";
			printf("[%d/%d]: %s(%p)\n", i, ts->n, name, path);
		}
	}wtk_debug("=======================================\n");
}

void wtk_rec_trace2(wtk_rec_t *rec, wtk_tokenset_t *ts)
{
	wtk_path_t *p;
	int i = 0;

	wtk_debug(
			"align=%p,like=%f\n", ts->tok.align, ts->tok.align?ts->tok.align->like:-1);
	p = ts->tok.path;
	while (p)
	{
		wtk_debug("%d: %p=%f\n", ++i, p->align, p->align->like);
		p = p->prev;
	}
}

void wtk_rec_trace_best_path(wtk_rec_t *rec)
{
	wtk_netnode_t *wn;
	wtk_path_t *path;
	wtk_netnode_t *node;
	char *p;

	node = rec->p_gen_max_node;
	if (!node)
	{
		return;
	}
	wn = wtk_netnode_wn(node);
	p = wtk_netnode_name2(wn);
	printf("%s[1] ", p);
	path = rec->p_gen_max_tok.path;
	if (path)
	{
		while (path)
		{
			p = wtk_netnode_name2(path->node);
			printf("%s ", p);
			path = path->prev;
		}
	}
	printf("\n");
}

void wtk_netnode_trace(wtk_netnode_t *node)
{
	wtk_string_t *str;
	wtk_netinst_t *inst = (wtk_netinst_t*) node->net_inst;

	str = wtk_netnode_name(node);
	if (str) //wtk_string_cmp_s(str,"f")==0)
	{
		printf("%*.*s,%f,%f ", str->len, str->len, str->data,
				inst->exit->tok.like,
				inst->exit->tok.path ? inst->exit->tok.path->align->like : -1);
		if (wtk_netnode_hmm(node))
		{
			printf("hmm \n");
		}
		else
		{
			printf("word\n");
		}
	}
}

void wtk_rec_trace_inst(wtk_rec_t *rec)
{
	wtk_queue_node_t *n;
	wtk_netinst_t *inst;
	int i;

	wtk_debug("============== inst queue ==================\n");
	for (i = 0, n = rec->inst_queue.pop; n; n = n->next, ++i)
	{
		inst = data_offset(n,wtk_netinst_t,q_n);
		printf("%d: %s(%s,%p)\n", i, wtk_netnode_name2(inst->node),
				inst->node->type & n_hmm ? "hmm" : "wrd",inst);
	}
}
