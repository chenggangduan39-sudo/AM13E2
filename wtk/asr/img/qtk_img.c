#include "qtk_img.h"
//void qtk_img_rec_nnet3_notify(qtk_img_rec_t *ir, wtk_kfeat_t *fe);

void qtk_img_rec_on_kxparm(qtk_img_rec_t *ir, wtk_kfeat_t *feat);
void qtk_img_rec_on_kxparm_end(qtk_img_rec_t *ir);

void qtk_img_rec_nnet3_notify(qtk_img_rec_t *ir,qtk_blas_matrix_t *f1,int end, int plus);
void qtk_img_rec_nnet3_notify_end(qtk_img_rec_t *ir);
void qtk_img_rec_clear_q(qtk_img_rec_t *ir, wtk_queue_t *q);
#define LONG_DEBUG 0
#define SHORT_DEBUG 0

qtk_img_rec_t *qtk_img_rec_new(qtk_img_cfg_t *cfg)
{
    qtk_img_rec_t *ir = (qtk_img_rec_t *)wtk_malloc(sizeof(qtk_img_rec_t));

    ir->cfg = cfg;
    ir->parm = wtk_kxparm_new(&cfg->kxparm);
    ir->pool = wtk_vpool2_new(sizeof(qtk_ifeat_t), 20);
    wtk_queue_init(&ir->q);
    wtk_queue_init(&ir->sil_q);

    ir->nnrt = NULL;
    ir->feat = NULL;
    if (cfg->use_nnrt) {
        ir->num_feats = 0;
        ir->frames = 0;
        ir->nnrt = qtk_nnrt_new(&cfg->nnrt);
        ir->feat = wtk_strbuf_new(1024, 1);

        ir->shape = wtk_malloc(sizeof(int64_t *) * ir->nnrt->num_in);
        ir->cache =
            wtk_malloc(sizeof(qtk_nnrt_value_t *) * (ir->nnrt->num_in - 1));
        ir->cache[0] = NULL;
        int i,j;
        for (i = 0; i < ir->nnrt->num_in; i++) {
            ir->shape[i] = wtk_malloc(sizeof(int64_t) * 5);
            ir->shape[i][0] =
                qtk_nnrt_get_input_shape(ir->nnrt, i, ir->shape[i] + 1, 4);
            for(j = 1; j < ir->shape[i][0]; j++){
                if(ir->shape[i][j] == -1){
                    ir->shape[i][j] = 1;
                }
            }
        }
        ir->ret_len = cfg->chunk * cfg->subsample;
        ir->pad_len = cfg->pad_len;
        ir->chunk_len = ir->ret_len + ir->pad_len;
        ir->tail_len = cfg->pad_len + (2 + cfg->right_context) * cfg->subsample;
        wtk_kxparm_set_notify(ir->parm, ir,
                              (wtk_kxparm_notify_f)qtk_img_rec_on_kxparm);
        wtk_kxparm_set_notify_end(
            ir->parm, ir, (wtk_kxparm_notify_end_f)qtk_img_rec_on_kxparm_end);
    } else {
        qtk_nnet3_set_notify(
            ir->parm->nnet3,
            (qtk_nnet3_feature_notify_f)qtk_img_rec_nnet3_notify, ir);
    }
    // wtk_kxparm_set_notify(ir->parm, ir,
    // (wtk_kxparm_notify_f)qtk_img_rec_nnet3_notify);
    // wtk_kxparm_set_notify_end(ir->parm, ir,
    // (wtk_kxparm_notify_end_f)qtk_img_rec_nnet3_notify_end);
    ir->sil_num = 0;
    ir->check_index = 0;
    ir->state = QTK_IMG_SIL;
    ir->wait_state = QTK_IMG_SIL;
    ir->index = 0;
    ir->waked = 0;
    ir->start = 0;
    ir->start_off = cfg->start_off;
    ir->end = 0;
    ir->end_off = cfg->end_off;
    ir->prob = 0.0;
    ir->wake_id = 0;
    ir->notify = NULL;
    ir->skip = 0;
    ir->print = 0;
    ir->before_av_prob = -1;

    ir->thresh = cfg->thresh;
    ir->thresh_echo = cfg->thresh_echo;

    ir->sp_sil = 1;
    if (ir->parm->parm->cfg->use_trick) {
        ir->idle = 1;
	}
	else
	{
		ir->idle = 0;
	}

	return ir;
}

void qtk_img_rec_set_notify(qtk_img_rec_t *ir, qtk_img_rec_notify_f notify, void *ths)
{
	ir->notify = notify;
	ir->notify_ths = ths;
}

void qtk_img_rec_delete(qtk_img_rec_t *ir)
{
    int i;
    for (i = 0; i < ir->nnrt->num_in; i++) {
        wtk_free(ir->shape[i]);
    }
    wtk_free(ir->shape);
    qtk_img_rec_clear_q(ir, &(ir->q));
    qtk_img_rec_clear_q(ir, &(ir->sil_q));
    if (ir->parm)
        wtk_kxparm_delete(ir->parm);
    if (ir->pool)
        wtk_vpool2_delete(ir->pool);
    if (ir->nnrt) {
        int i;
        if (ir->cache[0]) {
            for (i = 0; i < ir->nnrt->num_in - 1; i++) {
                qtk_nnrt_value_release(ir->nnrt, ir->cache[i]);
            }
        }
        wtk_free(ir->cache);
        wtk_strbuf_delete(ir->feat);
        qtk_nnrt_delete(ir->nnrt);
        }
        wtk_free(ir);
}

void qtk_img_rec_reset(qtk_img_rec_t *ir)
{
    ir->sil_num = 0;
    ir->waked = 0;
    ir->check_index = 0;
    ir->wake_id = 0;
    ir->prob = 0.0;
    ir->end = 0;
    ir->print = 0;
    ir->last_av_prob = -1;
	ir->last_maxx = -1;
	ir->last_avx = -1;
	ir->last_max_prob = -1;
	ir->sp_sil = 1;
        ir->before_av_prob = -1;
        ir->wait_flag = 0;

        ir->state = QTK_IMG_SIL;
        ir->wait_state = QTK_IMG_SIL;
        ir->index = 0;
        qtk_img_rec_clear_q(ir, &(ir->q));
        qtk_img_rec_clear_q(ir, &(ir->sil_q));
        wtk_vpool2_reset(ir->pool);
        wtk_queue_init(&ir->q);
        wtk_queue_init(&ir->sil_q);
        if (ir->parm->parm->pcen) {
            wtk_pcen_reset(ir->parm->parm->pcen);
        }
    wtk_kxparm_reset(ir->parm);
    if (ir->nnrt) {
        wtk_strbuf_reset(ir->feat);
        ir->num_feats = 0;
        qtk_nnrt_reset(ir->nnrt);
    }
}

void qtk_img_rec_reset2(qtk_img_rec_t *ir)
{
	ir->sil_num = 0;
	ir->check_index = 0;
	ir->wake_id = 0;
	ir->prob = 0.0;
        ir->before_av_prob = -1;
        ir->waked = 0;
	ir->wait_count = 0;
        ir->wait_flag = 0;
        //ir->end = 0;
	//ir->index=0;
	ir->state = QTK_IMG_SIL;
        ir->wait_state = QTK_IMG_SIL;
        ir->skip = 1;
	qtk_img_rec_clear_q(ir, &(ir->q));
	qtk_img_rec_clear_q(ir, &(ir->sil_q));
	wtk_vpool2_reset(ir->pool);
	wtk_queue_init(&ir->q);
	wtk_queue_init(&ir->sil_q);
}

void qtk_img_rec_show(qtk_img_rec_t *img, int id)
{
	// printf("%f %f ",start*0.08,end*0.08);
	switch (id)
	{
	case 1:
		printf("继续播放 ");
		break;
	case 2:
		printf("暂停播放 ");
		break;
	case 3:
		// printf("下一曲\n");
		break;
	case 4:
		// printf("上一曲\n");
		break;
	case 5:
		// printf("开始运动\n");
		break;
	case 6:
		printf("挂断电话 ");
		break;
	case 7:
		printf("增大音量 ");
		break;
	case 8:
		printf("打开音乐 ");
		break;
	case 9:
		printf("减小音量 ");
		break;
	case 10:
		printf("接听电话 ");
		break;
	default:
		break;
	}
}

qtk_ifeat_t *qtk_img_rec_feat_new(qtk_img_rec_t *ir, float *m)
{
	qtk_ifeat_t *feat = (qtk_ifeat_t *)wtk_vpool2_pop(ir->pool);
	int i;

	feat->log = (float *)wtk_malloc(sizeof(float) * 2);
	feat->prob = (float *)wtk_malloc(sizeof(float) * 2);
	memcpy(feat->log, m, sizeof(float) * 2);
	for (i = 0; i < 2; i++)
	{
		feat->prob[i] = exp(m[i] - m[0]);
	}
	feat->speech_prob = 0.0;
	feat->index = ir->index;

	return feat;
}

/*
 * forward wakeup check, save queue mean value
 *     1.speech prob >= 4
 *     2.send wakeup when av_prob or max_prob is smaller,
 */
int qtk_img_rec_forward_wakeup_check(qtk_img_rec_t *ir, float cul_prob) {
    int ret = 0, i = 1;
    float max = -10.0, av = -10.0, sum = 0.0, val = 0.0;
    qtk_ifeat_t *feat;
    wtk_queue_node_t *qn;
    qtk_img_thresh_cfg_t thresh;

    thresh = ir->sp_sil ? ir->thresh : ir->thresh_echo;

    for (qn = ir->q.pop; qn; qn = qn->next, i++) {
        if (i <= ir->check_index) {
            continue;
        }
        feat = (qtk_ifeat_t *)data_offset(qn, qtk_ifeat_t, q_n);
        val = feat->speech_prob;
        max = (val > max) ? val : max;
        sum += val;
    }

    av = sum / (ir->q.length - ir->check_index);
    if (av < ir->before_av_prob || max > cul_prob) {
        if (!ir->idle) {
            if (av > thresh.av_prob0 && max > thresh.max_prob0 &&
                (ir->q.length - ir->check_index) > thresh.speech_dur) {
                ret = 1;
            }
        } else {
            if (av > thresh.av_prob1 && max > thresh.max_prob1 &&
                (ir->q.length - ir->check_index) > thresh.speech_dur) {
                ret = 1;
            }
        }
    }

    ir->before_av_prob = av;

    return ret;
}

int qtk_img_rec_speech_check(qtk_img_rec_t *ir)
{
	int ret = 0, i = 1;
	qtk_ifeat_t *feat;
	wtk_queue_node_t *qn;
	float max = -10.0, sum = 0.0, val;
	// wtk_debug("%d\n",ir->check_index);
	for (qn = ir->q.pop; qn; qn = qn->next, i++)
	{
		if (i <= ir->check_index)
		{
			continue;
		}
		feat = (qtk_ifeat_t *)data_offset(qn, qtk_ifeat_t, q_n);
		val = feat->speech_prob;
		max = (val > max) ? val : max;
		sum += val;
	}

	sum /= (ir->q.length - ir->check_index);
	if (sum > ir->cfg->avg_vad_thresh && max > ir->cfg->max_vad_thresh)
	{
		ret = 1;
	}
	// wtk_debug("%f %f %d\n",sum,max,ret);
	return ret;
}

void qtk_img_rec_clear_q(qtk_img_rec_t *ir, wtk_queue_t *q)
{
	qtk_ifeat_t *feat;
	wtk_queue_node_t *qn;
	while (1)
	{
		qn = wtk_queue_pop(q);
		if (!qn)
		{
			break;
		}
		feat = data_offset(qn, qtk_ifeat_t, q_n);
		wtk_free(feat->log);
		wtk_free(feat->prob);
		wtk_vpool2_push(ir->pool, feat);
	}
}

void qtk_img_rec_merge_q(qtk_img_rec_t *ir)
{
	//wtk_debug("merge\n");
	qtk_ifeat_t *feat;
	wtk_queue_node_t *qn;

	while (1)
	{
		qn = wtk_queue_pop(&(ir->sil_q));
		if (!qn)
		{
			break;
		}
		feat = data_offset(qn, qtk_ifeat_t, q_n);
		wtk_queue_push(&(ir->q), &(feat->q_n));
	}
}

#if 0
float qtk_img_tree_cal(qtk_img_rec_t *ir, double *feat)
{
	int i, node_id = 0;
	int feat_id = -1;
	double thresh, tprob;
	qtk_mytree_t *tree;
	double prob[5];
	double sum = 0.0, aver;

	for (i = 0; i < 4; i++)
	{
		*(feat + i) = (*(feat + i) - feat_mean[i]) / feat_var[i];
	}
	sum = 0.0;
	for (i = 0; i < 5; i++)
	{
		tree = trees[i];
		node_id = 0;
		while (1)
		{
			feat_id = tree->feat[node_id];
			thresh = tree->thresh[node_id];
			tprob = tree->prob[node_id];

			if (feat[feat_id] > thresh)
			{
				node_id = tree->right[node_id];
			}
			else
			{
				node_id = tree->left[node_id];
			}
			if (node_id < 0)
			{
				prob[i] = tprob;
				break;
			}
		}
		sum += prob[i];
	}
	aver = sum / 2;
	return aver;
}
#endif

#if 0
void qtk_img_feat_cal_bak(qtk_img_rec_t *ir, int split)
{
#if 1
	if (ir->q.length < 5)
	{
		return;
	}
#endif
	int start, end, cal_end = ir->q.length;
	qtk_ifeat_t *feat;
	wtk_queue_node_t *qn;
	// wtk_debug("[1]feat->index=%d\n",feat->index)
	qn = ir->q.pop;
	feat = (qtk_ifeat_t *)data_offset(qn, qtk_ifeat_t, q_n);
	// wtk_debug("[2]feat->index=%d\n",feat->index)
	start = feat->index;
	qn = ir->q.push;
	feat = (qtk_ifeat_t *)data_offset(qn, qtk_ifeat_t, q_n);
	end = feat->index;
	// wtk_debug("[3]feat->index=%d\n",feat->index)

	qtk_ifeat_t *show_feat;
	wtk_queue_node_t *show_qn;
	float val_prob1, val_prob2, max_prob = -100.0, av_prob, sum_prob = 0.0;
	float valx, maxx = -100.0, avx, sumx = 0.0;
	// float rmes=0.0;
	float ee, sumee = 0.0;
	int cnt_prob = 0;

	if (split == 1)
	{
		cal_end = ir->check_index - ir->sil_num;
	}

	for (show_qn = ir->q.pop; show_qn; show_qn = show_qn->next)
	{
		show_feat = (qtk_ifeat_t *)data_offset(show_qn, qtk_ifeat_t, q_n);
		// wtk_debug("%f\n",log(1.0-exp(show_feat->log[0]))-show_feat->log[0]);

		// val_prob2 = log(1.0-exp(show_feat->log[0]))-show_feat->log[0];
		val_prob2 = show_feat->log[1] - show_feat->log[0];
		ee = wtk_pow(val_prob2, 2);
		val_prob1 = show_feat->log[1] - wtk_log(wtk_exp(show_feat->log[0]) + 1e-12);
		valx = show_feat->log[1];
		max_prob = (val_prob1 > max_prob) ? val_prob1 : max_prob;
		maxx = (valx > maxx) ? valx : maxx;
		sum_prob += val_prob2;
		sumx += valx;
		sumee += ee;
		cnt_prob++;
		// wtk_debug("%.3f\n",val_prob1);

		if (split && cnt_prob > cal_end)
		{
			break;
		}
    }
    av_prob = sum_prob / cnt_prob;
    avx = sumx / cnt_prob;
	// rmes = pow((sumee/cnt_prob),0.5);

	// if(ir->q.length < 3)
	// {
	//      return;
	// }

	int cor = 0;
#if 0
	wtk_debug("av_prob = %f maxx = %.3f avx = %.3f max_prob = %.3f \n",av_prob,maxx,avx,max_prob);
	wtk_debug("av_prob = %f maxx = %.3f avx = %.3f max_prob = %.3f \n",
	                                                              ir->cfg->av_prob,ir->cfg->maxx,ir->cfg->avx,ir->cfg->max_prob);
	if (maxx > ir->cfg->maxx && avx > ir->cfg->avx && av_prob > ir->cfg->av_prob && max_prob > ir->cfg->max_prob)
    {
        wtk_debug("%p av_prob = %f maxx = %.3f avx = %.3f max_prob = %.3f \n", ir, ir->av_prob,ir->maxx,ir->avx,ir->max_prob);
    }
#endif
#if DEBUG
    wtk_debug("start = %f, end = %f\n", start * 0.08, end * 0.08);
    if (0 == ir->idle)
    {
        wtk_debug("av_prob = %f(%f) maxx = %.3f(%.3f) avx = %.3f(%.3f) max_prob = %.3f(%.3f) idle = %d\n",av_prob, ir->av_prob0, maxx, ir->maxx0, avx, ir->avx0, max_prob, ir->max_prob0, ir->idle);
    }
    else
    {
        wtk_debug("av_prob = %f(%f) maxx = %.3f(%.3f) avx = %.3f(%.3f) max_prob = %.3f(%.3f) idle = %d\n",av_prob, ir->av_prob1, maxx, ir->maxx1, avx, ir->avx1, max_prob, ir->max_prob1, ir->idle);
    }
#endif
    if (!ir->idle)
    {
        if (maxx > ir->maxx0 && avx > ir->avx0 && av_prob > ir->av_prob0 && max_prob > ir->max_prob0)
        {
            cor = 1;
            ir->waked = 1;
            // ir->wake_id = id;
			// ir->prob = av[id];
			ir->start = start;
			ir->end = end;
            // wtk_debug("m[id]=%f av[id]=%f\n",m[id],av[id]);
		}
	}
	else
	{
		if (maxx > ir->maxx1 && avx > ir->avx1 && av_prob > ir->av_prob1 && max_prob > ir->max_prob1)
		{
			cor = 1;
			ir->waked = 1;
			// ir->wake_id = id;
			// ir->prob = av[id];
			ir->start = start;
			ir->end = end;
			// wtk_debug("m[id]=%f av[id]=%f\n",m[id],av[id]);
		}
	}
	if (cor == 1 && ir->notify)
	{
		ir->notify(ir->notify_ths, 1, exp(show_feat->log[1]), start, end);

		qtk_img_rec_reset2(ir);
		ir->state = QTK_IMG_WAIT;
		// wtk_cmvn_flush(ir->parm->parm->cmvn);
		// wtk_cmvn_reset(ir->parm->parm->cmvn);
		if (ir->parm->parm->cfg->use_trick)
		{
			ir->parm->parm->idle_hint = 0;
			ir->idle = 0;
		}
	}
	else
	{
		qtk_img_rec_reset2(ir);
	}
}
#endif

void qtk_img_feat_cal(qtk_img_rec_t *ir, int split, int res) {
    int start, end, cal_end = ir->q.length;
    qtk_ifeat_t *feat;
    wtk_queue_node_t *qn;
    // wtk_debug("[1]feat->index=%d\n",feat->index)
    qn = ir->q.pop;
    feat = (qtk_ifeat_t *)data_offset(qn, qtk_ifeat_t, q_n);
    // wtk_debug("[2]feat->index=%d\n",feat->index)
    start = feat->index + ir->start_off + 1;
    qn = ir->q.push;
    feat = (qtk_ifeat_t *)data_offset(qn, qtk_ifeat_t, q_n);
    end = feat->index + 1;
    // wtk_debug("[3]feat->index=%d\n",feat->index)

    qtk_ifeat_t *show_feat;
    wtk_queue_node_t *show_qn;
    qtk_img_thresh_cfg_t thresh = ir->sp_sil ? ir->thresh : ir->thresh_echo;
    float val_prob, max_prob = -100.0, av_prob, sum_prob = 0.0;
    float valx, maxx = -100.0, avx, sumx = 0.0;
    int cnt_prob = 0;

    if (split == 1) {
        cal_end = ir->check_index - ir->sil_num;
    }

    for (show_qn = ir->q.pop; show_qn; show_qn = show_qn->next) {
        show_feat = (qtk_ifeat_t *)data_offset(show_qn, qtk_ifeat_t, q_n);
        val_prob = wtk_log(1 + 1e-12 - exp(show_feat->log[0])) -
                   wtk_log(wtk_exp(show_feat->log[0]) + 1e-12);
        valx = show_feat->log[1] - show_feat->log[0];
        if (val_prob > max_prob) {
            max_prob = val_prob;
            end = show_feat->index + 1;
        }
        maxx = (valx > maxx) ? valx : maxx;
        sum_prob += val_prob;
        sumx += valx;
        cnt_prob++;

        if (split && cnt_prob > cal_end) {
            break;
        }
    }
    end += ir->end_off;
    av_prob = sum_prob / cnt_prob;
    avx = sumx / cnt_prob;

#if SHORT_DEBUG
    if (av_prob > ir->last_av_prob)
    {
        ir->print = 1;
        ir->last_av_prob = av_prob;
        ir->last_max_prob = max_prob;
        ir->last_avx = avx;
        ir->last_maxx = maxx;
    }
#endif

    int cor = 0;
#if LONG_DEBUG
    wtk_debug("len = %d start = %f %d, end = %f %d\n", ir->q.length,
              start * 0.08, start, end * 0.08, end);
    if (0 == ir->idle)
    {
        wtk_debug("av_prob = %f(%f) maxx = %.3f(%.3f) avx = %.3f(%.3f) max_prob = %.3f(%.3f) idle = %d\n",av_prob, thresh.av_prob0, maxx, thresh.maxx0, avx, thresh.avx0, max_prob, thresh.max_prob0, ir->idle);
    }
    else
    {
        wtk_debug("av_prob = %f(%f) maxx = %.3f(%.3f) avx = %.3f(%.3f) max_prob = %.3f(%.3f) idle = %d\n",av_prob, thresh.av_prob1, maxx, thresh.maxx1, avx, thresh.avx1, max_prob, thresh.max_prob1, ir->idle);
    }
#endif

	if (!ir->idle)
	{
		if (maxx > thresh.maxx0 && avx > thresh.avx0 && av_prob > thresh.av_prob0 && max_prob > thresh.max_prob0 && ir->q.length > thresh.speech_dur)
		{
			cor = 1;
			ir->waked = 1;
			ir->start = start;
			ir->end = end;
		}
	}
	else
	{
		if (maxx > thresh.maxx1 && avx > thresh.avx1 && av_prob > thresh.av_prob1 && max_prob > thresh.max_prob1 && ir->q.length > thresh.speech_dur)
		{
			cor = 1;
			ir->waked = 1;
			ir->start = start;
			ir->end = end;
		}
	}
        if (cor == 1 && ir->notify) {
            ir->prob = av_prob;
            if (start == end) {
                end += 1;
            }
            if (0 == ir->cfg->notify_bias) {
                ir->notify(ir->notify_ths, res, start, end);
            }
            qtk_img_rec_reset2(ir);
            ir->state = QTK_IMG_WAIT;

            if (ir->parm->parm->cfg->use_trick) {
                ir->parm->parm->idle_hint = 0;
                ir->idle = 0;
            }
        } else {
            qtk_img_rec_reset2(ir);
        }
}

void qtk_img_rec_wait_process(qtk_img_rec_t *ir, float *f) {
    qtk_img_cfg_t *cfg = ir->cfg;
    qtk_ifeat_t *ifeat;
    float speech_prob = 0.0;

    speech_prob = f[1] - log(exp(f[0]) + 1e-12);
    if (speech_prob < cfg->min_vad_thresh) {
        ir->wait_flag = 1;
    }

    if (ir->wait_flag) {
        switch (ir->wait_state) {
        case QTK_IMG_SIL:
#if LONG_DEBUG
            wtk_debug("SIL:");
            wtk_debug("speech_prob = %f, min_vad_thresh = %f\n", speech_prob,
                      cfg->min_vad_thresh);
#endif
            ir->check_index = 0;
            if (speech_prob > cfg->min_vad_thresh) {
                ir->wait_state = QTK_IMG_SPEECH;
                ifeat = qtk_img_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
            }
            break;
        case QTK_IMG_SPEECH:
#if LONG_DEBUG
            wtk_debug("SPEECH:");
            wtk_debug("speech_prob = %f, min_vad_thresh = %f\n", speech_prob,
                      cfg->min_vad_thresh);
#endif
            if (speech_prob > cfg->min_vad_thresh) {
                ifeat = qtk_img_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
                if (qtk_img_rec_forward_wakeup_check(ir, speech_prob)) {
                    qtk_img_feat_cal(ir, 0, 0);
                }
            } else {
                if (qtk_img_rec_speech_check(ir)) {
                    ir->wait_state = QTK_IMG_TBD;
                    ir->sil_num = 1;
                    ifeat = qtk_img_rec_feat_new(ir, f);
                    wtk_queue_push(&(ir->sil_q), &(ifeat->q_n));
                } else {
                    if (ir->check_index > 0) {
                        qtk_img_feat_cal(ir, 1, 0);
                    } else {
                        qtk_img_rec_clear_q(ir, &(ir->q));
                    }
                    ir->wait_state = QTK_IMG_SIL;
                }
            }
            break;
        case QTK_IMG_TBD:
#if LONG_DEBUG
            wtk_debug("TBD:\n");
#endif
            if (speech_prob > cfg->min_vad_thresh) {
                ir->check_index = ir->q.length;
                ifeat = qtk_img_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
                ir->wait_state = QTK_IMG_SPEECH;
            } else {
                ir->sil_num++;
                if (ir->sil_num > cfg->min_noise_dur) {
                    qtk_img_rec_clear_q(ir, &(ir->sil_q));
                    qtk_img_feat_cal(ir, 0, 0);
                    qtk_img_rec_clear_q(ir, &(ir->q));
                    ir->sil_num = 0;
                } else {
                    ifeat = qtk_img_rec_feat_new(ir, f);
                    wtk_queue_push(&(ir->sil_q), &(ifeat->q_n));
                }
            }
            break;
        default:
            break;
        }
    }
}

void qtk_img_rec_nnet3_notify_end(qtk_img_rec_t *ir)
{
    if (ir->q.length > 0)
    {
        qtk_img_feat_cal(ir, 0, 1);
    }
#if SHORT_DEBUG
    if (0 == ir->print) 
    {
        printf("-1000.00 -1000.00 -1000.00 -1000.00\n");
    }
    else
    {
        printf("%.3f %.3f %.3f %.3f\n" ,ir->last_av_prob, ir->last_maxx, ir->last_avx, ir->last_max_prob);
    }
#endif
	if (ir->parm->parm->cfg->use_trick)
	{
		if (ir->parm->parm->idle_hint > ir->parm->parm->cfg->idle_trigger_frame)
		{
			ir->idle = 1;
		}
	}
}

// float kresult[]=
// {
// #include "kresult"
// };
// int xx=0;
// float likehood;
// void qtk_img_rec_nnet3_notify(qtk_img_rec_t *ir, wtk_kfeat_t *fe)
void qtk_img_rec_nnet3_process(qtk_img_rec_t *ir, float *f1) {
    // wtk_debug("index = %d\n", ir->index);
    if (ir->parm->parm->cfg->use_trick) {
        if (ir->idle == 0 && ir->parm->parm->idle_hint >
                                 ir->parm->parm->cfg->idle_trigger_frame) {
            ir->idle = 1;
        }
    }
    // int xxx;
    // float *kf = kresult + xx*2;
    // for(xxx=0;xxx<2;xxx++){
    //      fe->v[xxx] = kf[xxx];
    // }
    // xx++;
    // wtk_debug("ir->waked=%d\n",ir->waked)
    if (ir->waked) {
        return;
    }
    //      if(end)
    //      {
    //              if(ir->q.length > 0)
    //              {
    //                      qtk_img_feat_cal(ir,0);
    //              }
    //              return;
    //      }
    float *f = f1;
    //      if(ir->skip > 0)
    // {
    //      //wtk_debug("=========skip===========\n");
    //      ir->skip--;
    //      return;
    // }
    // if(f[0] <= 0.0 ){
    //      f[0] = 0.001;
    // }
    /*float textxx = exp(-26.02668);
    double xx = exp(f[0]);
    double x = xx + 1e-12;
    double xxx = log(x);
    float testx = log(4.974580850614913e-12+1e-12);
    wtk_debug("%f %f %.15f %.15f %f\n",testx,textxx,x,xxx,f[0]);*/
    // float speech_prob = (log(1.0 - exp(f[0]) +1e-12 ) - f[0]);
    float speech_prob = f[1] - log(exp(f[0]) + 1e-12);
    // printf("%f %f\n",f[0],f[1]);
    //  likehood += (f[1]-f[0]);
#if 0
	wtk_debug("f[0] = %f, f[1] = %f\n",exp(f[0]),exp(f[1]));
	wtk_debug("f[0] = %f, f[1] = %f\n",f[0],f[1]);
#endif
	// float speech_prob = log(1-exp(f[0]))-f[0];
	//  *f = log(*f);
	//  speech_prob = f[1]-f[0];
	//wtk_debug("%f %d\n",speech_prob,ir->index);
	if (ir->skip == 1)
	{
		if (speech_prob > 0) // cfg->min_vad_thresh
		{
			// fake_prob1 = 0.9;fake_prob2=0.1;
			// printf("%.1f %.1f\n",fake_prob1,fake_prob2);
			// wtk_debug("=========skip===========\n");
			// return;
		}
		else
		{
			ir->skip = 0;
			// wtk_debug("****************skipEnd*************\n");
		}
	}

	qtk_img_cfg_t *cfg = ir->cfg;
	qtk_ifeat_t *ifeat;
	//wtk_debug("%f %d\n",speech_prob,ir->index);
	switch (ir->state)
    {
        case QTK_IMG_SIL:
#if LONG_DEBUG 
            wtk_debug("SIL:");
            wtk_debug("speech_prob = %f, min_vad_thresh = %f\n", speech_prob, cfg->min_vad_thresh);
#endif
            ir->check_index = 0;
            if (speech_prob > cfg->min_vad_thresh)
            {
                ir->state = QTK_IMG_SPEECH;
                ifeat = qtk_img_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
            }
            break;
        case QTK_IMG_SPEECH:
#if LONG_DEBUG
            wtk_debug("SPEECH:");
            wtk_debug("speech_prob = %f, min_vad_thresh = %f\n", speech_prob, cfg->min_vad_thresh);
#endif
            if (speech_prob > cfg->min_vad_thresh)
            {
                ifeat = qtk_img_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
                if (qtk_img_rec_forward_wakeup_check(ir, speech_prob)) {
                    //wtk_debug("len = %d\n", ir->q.length);
                    qtk_img_feat_cal(ir, 0, 1);
                }
            }
            else
            {
#if 0
                qtk_img_feat_cal(ir, 0);
                qtk_img_rec_clear_q(ir, &(ir->q));
                ir->state = QTK_IMG_SIL;
#endif
                if (qtk_img_rec_speech_check(ir))
                {
                    ir->state = QTK_IMG_TBD;
                    ir->sil_num = 1;
                    ifeat = qtk_img_rec_feat_new(ir, f);
                    wtk_queue_push(&(ir->sil_q), &(ifeat->q_n));
                }
                else
                {
                    //wtk_debug("check fail\n");
#if 0
                    qtk_img_rec_clear_q(ir, &(ir->q));
                    ir->state = QTK_IMG_SIL;
#endif
                    if (ir->check_index > 0)
                    {
                        qtk_img_feat_cal(ir, 1, 1);
                    }
                    else
                    {
                        qtk_img_rec_clear_q(ir, &(ir->q));
                    }
                    ir->state = QTK_IMG_SIL;
                }
            }
            break;
        case QTK_IMG_TBD:
#if LONG_DEBUG
            wtk_debug("TBD:\n");
#endif
            if (speech_prob > cfg->min_vad_thresh)
            {
                //qtk_img_rec_merge_q(ir);
                ir->check_index = ir->q.length;
                ifeat = qtk_img_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
                ir->state = QTK_IMG_SPEECH;
            }
            else
            {
                ir->sil_num++;
                if (ir->sil_num > cfg->min_noise_dur)
                {
                    qtk_img_rec_clear_q(ir, &(ir->sil_q));
#if LONG_DEBUG
                    wtk_debug("rec start\n");
                    wtk_debug("rec_speech_prob=%f\n",speech_prob);
                    wtk_debug("--------------->len = %d\n", ir->q.length);
#endif
                    qtk_img_feat_cal(ir, 0, 1); // start rec
                    qtk_img_rec_clear_q(ir, &(ir->q));
                    //ir->state = QTK_IMG_SIL;
                    ir->sil_num = 0;
                }
                else
                {
                    ifeat = qtk_img_rec_feat_new(ir, f);
                    wtk_queue_push(&(ir->sil_q), &(ifeat->q_n));
                }
            }
            break;
        case QTK_IMG_WAIT:
#if LONG_DEBUG
            wtk_debug("WAIT:");
#endif
            //		if(speech_prob < 1){
            ir->wait_count++;
            if (ir->cfg->use_waitwake) {
                qtk_img_rec_wait_process(ir, f);
                if (ir->wait_count > 13) {
                    if (ir->q.length > 0) {
                        qtk_img_feat_cal(ir, 0, 0);
                    }
                    qtk_img_rec_reset2(ir);
                    ir->state = QTK_IMG_SIL;
                    ir->wait_count = 0;
                }
            } else {
                if (ir->cfg->notify_bias == ir->wait_count) {
                    if (ir->start == ir->end) {
                        ir->end += 1;
                    }
                    ir->notify(ir->notify_ths, 0, ir->start, ir->end);
                }
                if (ir->wait_count > 10) {
                    ir->state = QTK_IMG_SIL;
                    ir->wait_count = 0;
                }
            }
#if 0 
            wtk_debug("wait===========wait_count = %d\n",ir->wait_count);
#endif
            //		}
            //		else{
            //			ir->state = QTK_IMG_WAIT;
            //		}
            break;
        default:
            break;
    }
    ir->index++;
}

static void qtk_img_rec_feed_nnrt(qtk_img_rec_t *ir, float *input) {
    float *fake_cache[30], *out;
    fake_cache[0] = NULL;
    int8_t data2[128] = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    int i;
    qtk_nnrt_t *nnrt = ir->nnrt;
    qtk_nnrt_value_t cache[30], input_val;
    qtk_nnrt_value_t output;
    int shape_len;
    int64_t shapex[4];
    int ncache = nnrt->num_in - 1;
    if (ir->cache[0]) {
        for (i = 0; i < ncache; i++) {
            cache[i] = ir->cache[i];
        }
    } else {
        int64_t *cache_shape;
        for (i = 0; i < nnrt->num_in - 1; i++) {
            cache_shape = ir->shape[i + 1] + 1;
            shape_len = ir->shape[i + 1][0];
            if (shape_len == 2) {
                fake_cache[i] =
                    wtk_calloc(cache_shape[1] * cache_shape[0], sizeof(float));
            } else if (shape_len == 3) {
                fake_cache[i] =
                    wtk_calloc(cache_shape[2] * cache_shape[1] * cache_shape[0],
                               sizeof(float));
            } else {
                fake_cache[i] = wtk_calloc(cache_shape[3] * cache_shape[2] *
                                               cache_shape[1] * cache_shape[0],
                                           sizeof(float));
            }
            if (i == nnrt->num_in - 3) {
                cache[i] = qtk_nnrt_value_create_external2(
                    ir->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, shape_len,
                    data2, cache_shape[1]);
            } else {
                cache[i] = qtk_nnrt_value_create_external(
                    ir->nnrt, QTK_NNRT_VALUE_ELEM_F32, cache_shape, shape_len,
                    fake_cache[i]);
            }
        }
    }
    input_val = qtk_nnrt_value_create_external(nnrt, QTK_NNRT_VALUE_ELEM_F32,
                                               ir->shape[0] + 1, 3, input);

    qtk_nnrt_feed(nnrt, input_val, 0);

    for (i = 0; i < ncache; i++) {
        qtk_nnrt_feed(ir->nnrt, cache[i], i + 1);
    }
    qtk_nnrt_run(ir->nnrt);

    qtk_nnrt_get_output(ir->nnrt, &output, 0);

    out = qtk_nnrt_value_get_data(nnrt, output);
    // print_float(out, 16);
    for (i = 0; i < ncache; i++) {
        qtk_nnrt_get_output(ir->nnrt, &ir->cache[i], i + 1);
    }
    qtk_nnrt_value_get_shape(nnrt, output, shapex, 3);

    for (i = 0; i < shapex[1]; i++) {
        if (ir->cfg->blank_penalty != 0) {
            *(out + i * shapex[2]) -= ir->cfg->blank_penalty;
            wtk_softmax(out + i * shapex[2], shapex[2]);
            wtk_add_log(out + i * shapex[2], shapex[2]);
        }
        qtk_img_rec_nnet3_process(ir, out + i * shapex[2]);
    }

    qtk_nnrt_value_release(ir->nnrt, input_val);
    for (i = 0; i < ncache; i++) {
        qtk_nnrt_value_release(ir->nnrt, cache[i]);
    }
    qtk_nnrt_value_release(ir->nnrt, output);
    if (fake_cache[0]) {
        for (i = 0; i < ncache; i++) {
            wtk_free(fake_cache[i]);
        }
    }
}

void qtk_img_rec_on_kxparm(qtk_img_rec_t *ir, wtk_kfeat_t *feat) {
    wtk_strbuf_push_float(ir->feat, feat->v, ir->shape[0][3]);
    ir->num_feats++;
    if ((ir->num_feats >= ir->shape[0][2]) &&
        ((ir->num_feats - ir->pad_len) % ir->ret_len == 0)) {
        // wtk_debug("%d\n",ir->num_feats);
        qtk_img_rec_feed_nnrt(ir, (float *)ir->feat->data);
        wtk_strbuf_pop(ir->feat, NULL,
                       ir->shape[0][3] * ir->ret_len * sizeof(float));
        ir->frames += ir->ret_len;
    }
}

static float eps_feat[] = {
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850, -23.025850, -23.025850, -23.025850, -23.025850,
    -23.025850, -23.025850};

void qtk_img_rec_on_kxparm_end(qtk_img_rec_t *ir) {
    int i;
    if (ir->num_feats - ir->frames > ir->chunk_len / 2) {
        for (i = 0; i <= ir->shape[0][3] - (ir->num_feats - ir->frames); i++) {
            wtk_strbuf_push_float(ir->feat, eps_feat, ir->shape[0][3]);
        }
        qtk_img_rec_feed_nnrt(ir, (float *)ir->feat->data);
    }
}

void qtk_img_rec_nnet3_notify(qtk_img_rec_t *ir, qtk_blas_matrix_t *f1, int end,
                              int plus) {
    unsigned int i, row, col;
    float *m;

    if (f1) {
        row = f1->row;
        col = f1->col;
        m = f1->m;
        for (i = 0; i < row; i++) {
            qtk_img_rec_nnet3_process(ir, m + i * col);
        }
    }
    if (end == 1) {
        if (ir->q.length > 0) {
            qtk_img_feat_cal(ir, 0, 1);
        }
#if SHORT_DEBUG
        if (0 == ir->print) {
            printf("-1000.00 -1000.00 -1000.00 -1000.00\n");
        } else {
            printf("%.3f %.3f %.3f %.3f\n", ir->last_av_prob, ir->last_maxx,
                   ir->last_avx, ir->last_max_prob);
        }
#endif
        if (ir->parm->parm->cfg->use_trick) {
            if (ir->parm->parm->idle_hint >
                ir->parm->parm->cfg->idle_trigger_frame) {
                ir->idle = 1;
            }
        }
        return;
    }
}

void qtk_img_rec_start(qtk_img_rec_t *ir)
{
	wtk_kxparm_start(ir->parm);
}

int qtk_img_rec_feed(qtk_img_rec_t *ir, char *data, int bytes, int is_end)
{
	//wtk_debug("%p %p\n", ir, ir->parm);
	wtk_kxparm_feed(ir->parm, (short *)data, bytes >> 1, is_end);

	return ir->waked;
}

int qtk_img_rec_get_time(qtk_img_rec_t *ir, float *fs, float *fe)
{
    *fs = ir->start * ir->cfg->f_dur - 4 * ir->cfg->f_dur;
    *fe = ir->end * ir->cfg->f_dur - 4 * ir->cfg->f_dur;
    // printf("%d %d %f %f\n",ir->start,ir->end,*fs,*fe);
    return 0;
}

float qtk_img_rec_get_conf(qtk_img_rec_t *ir)
{
	return ir->prob;
}

void qtk_img_rec_get_sp_sil(qtk_img_rec_t *ir, int sp_sil)
{
	 ir->sp_sil = sp_sil;
}

int qtk_img_set_prob(qtk_img_rec_t *ir, float prob, int echo) {
    if (echo) {
        ir->thresh_echo.av_prob0 = prob;
    } else {
        ir->thresh.av_prob0 = prob;
    }
    return 0;
}

// int qtk_img_thresh_set_cfg(qtk_img_cfg_t *cfg, float av_prob, float maxx, float avx, float max_prob)
// {
// 	qtk_img_cfg_t *thresh_cfg = cfg;

// 	thresh_cfg->av_prob = av_prob;
// 	thresh_cfg->maxx = maxx;
// 	thresh_cfg->avx = avx;
// 	thresh_cfg->max_prob = max_prob;
// }
void qtk_img_thresh_set_cfg(qtk_img_rec_t *ir, qtk_img_thresh_cfg_t thresh, int echo)
{
	if(echo){
		ir->thresh_echo = thresh;
	}else{
		ir->thresh = thresh;
	}
}
