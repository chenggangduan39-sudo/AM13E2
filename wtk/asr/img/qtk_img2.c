#include "qtk_img2.h"
//void qtk_img2_rec_nnet3_notify(qtk_img2_rec_t *ir, wtk_kfeat_t *fe);
void qtk_img2_rec_nnet3_notify(qtk_img2_rec_t *ir,qtk_blas_matrix_t *f1,int end, int plus);
void qtk_img2_rec_nnet3_notify_end(qtk_img2_rec_t *ir);
void qtk_img2_rec_clear_q(qtk_img2_rec_t *ir, wtk_queue_t *q);
#define DEBUG 0

qtk_img2_rec_t *qtk_img2_rec_new(qtk_img2_cfg_t *cfg)
{
    qtk_img2_rec_t *ir = (qtk_img2_rec_t *)wtk_malloc(sizeof(qtk_img2_rec_t));
    ir->cfg = cfg;
    ir->feature_cols = 0;
    ir->kparm =
        (wtk_kparm_t **)wtk_malloc(sizeof(wtk_kparm_t *) * ir->cfg->channels);
    int i;
    for (i = 0; i < ir->cfg->channels; i++) {
        ir->kparm[i] = wtk_kparm_new(&(cfg->kparm));
        switch (ir->kparm[i]->cfg->kfind.bkind) {
        case WTK_FBANK:
            ir->feature_cols += ir->kparm[i]->cfg->melbank.num_bins;
            break;
        case WTK_MFCC:
            ir->feature_cols += ir->kparm[i]->cfg->NUMCEPS;
            break;
        default:
            wtk_debug("error type %d, do not support\n",
                      ir->kparm[0]->cfg->kfind.bkind);
            exit(0);
            break;
        }
    }
    ir->nnet3 = qtk_nnet3_new(&(cfg->nnet3), ir->feature_cols,
                              ir->cfg->kparm.use_ivector);

    ir->pool = wtk_vpool2_new(sizeof(qtk_ifeat2_t), 20);
    wtk_queue_init(&ir->q);
    wtk_queue_init(&ir->sil_q);
    qtk_nnet3_set_notify(
        ir->nnet3, (qtk_nnet3_feature_notify_f)qtk_img2_rec_nnet3_notify, ir);
    // wtk_kxparm_set_notify(ir->parm, ir,
    // (wtk_kxparm_notify_f)qtk_img2_rec_nnet3_notify);
    // wtk_kxparm_set_notify_end(ir->parm, ir,
    // (wtk_kxparm_notify_end_f)qtk_img2_rec_nnet3_notify_end);
    ir->sil_num = 0;
    ir->check_index = 0;
    ir->state = QTK_IMG2_SIL;
    ir->index = 0;
    ir->waked = 0;
    ir->start = 0;
    ir->end = 0;
    ir->prob = 0.0;
    ir->wake_id = 0;
    ir->notify = NULL;
    ir->out_num = 0;

    ir->thresh = cfg->thresh;
    ir->thresh_echo = cfg->thresh_echo;
    ir->sp_sil = 1;

    if (ir->kparm[0]->cfg->use_trick) {
        ir->idle = 1;
    } else {
        ir->idle = 0;
    }

    return ir;
}

void qtk_img2_rec_set_notify(qtk_img2_rec_t *ir, qtk_img2_rec_notify_f notify, void *ths)
{
	ir->notify = notify;
	ir->notify_ths = ths;
}

void qtk_img2_rec_delete(qtk_img2_rec_t *ir)
{
	qtk_img2_rec_clear_q(ir, &(ir->q));
    qtk_img2_rec_clear_q(ir, &(ir->sil_q));
    int i;
    for (i = 0; i < ir->cfg->channels; i++) {
        wtk_kparm_delete(ir->kparm[i]);
    }
    wtk_free(ir->kparm);
    qtk_nnet3_delete(ir->nnet3);
    wtk_vpool2_delete(ir->pool);
	wtk_free(ir);
}

void qtk_img2_rec_reset(qtk_img2_rec_t *ir)
{
    ir->sil_num = 0;
    ir->waked = 0;
    ir->check_index = 0;
    ir->wake_id = 0;
    ir->prob = 0.0;
    ir->end = 0;
    ir->state = QTK_IMG2_SIL;
	ir->sp_sil = 1;
    // ir->index = 0;
    qtk_img2_rec_clear_q(ir, &(ir->q));
    qtk_img2_rec_clear_q(ir, &(ir->sil_q));
    wtk_vpool2_reset(ir->pool);
    wtk_queue_init(&ir->q);
    wtk_queue_init(&ir->sil_q);
    int i;
    for (i = 0; i < ir->cfg->channels; i++) {
        if (ir->kparm[i]->pcen) {
            wtk_pcen_reset(ir->kparm[i]->pcen);
        }
        wtk_kparm_reset(ir->kparm[i]);
    }
    qtk_nnet3_reset(ir->nnet3);
}

void qtk_img2_rec_reset2(qtk_img2_rec_t *ir)
{
	ir->sil_num = 0;
	ir->check_index = 0;
	ir->wake_id = 0;
	ir->prob = 0.0;
	ir->waked = 0;
	ir->wait_count = 0;
	//ir->end = 0;
	//ir->index=0;
	ir->state = QTK_IMG2_SIL;
	qtk_img2_rec_clear_q(ir, &(ir->q));
	qtk_img2_rec_clear_q(ir, &(ir->sil_q));
	wtk_vpool2_reset(ir->pool);
	wtk_queue_init(&ir->q);
	wtk_queue_init(&ir->sil_q);
}

qtk_ifeat2_t *qtk_img2_rec_feat_new(qtk_img2_rec_t *ir, float *m)
{
    qtk_ifeat2_t *feat = (qtk_ifeat2_t *)wtk_vpool2_pop(ir->pool);
    int i;

    feat->log = (float *)wtk_malloc(sizeof(float) * ir->out_num);
    memcpy(feat->log, m, sizeof(float) * ir->out_num);
    feat->prob = (float *)wtk_malloc(sizeof(float) * ir->out_num);
    for (i = 0; i < ir->out_num; i++)
    {
        feat->prob[i] = exp(m[i] - m[0]);
    }
    feat->speech_prob = 0.0;
    feat->index = ir->index;

    return feat;
}

int qtk_img2_rec_speech_check(qtk_img2_rec_t *ir)
{
	int ret = 0, i = 1;
	qtk_ifeat2_t *feat;
	wtk_queue_node_t *qn;
	float max = -10.0, sum = 0.0, val;
	// wtk_debug("%d\n",ir->check_index);
	for (qn = ir->q.pop; qn; qn = qn->next, i++)
	{
		if (i <= ir->check_index)
		{
			continue;
		}
		feat = (qtk_ifeat2_t *)data_offset(qn, qtk_ifeat2_t, q_n);
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

void qtk_img2_rec_clear_q(qtk_img2_rec_t *ir, wtk_queue_t *q)
{
	qtk_ifeat2_t *feat;
	wtk_queue_node_t *qn;
	while (1)
	{
		qn = wtk_queue_pop(q);
		if (!qn)
		{
			break;
		}
		feat = data_offset(qn, qtk_ifeat2_t, q_n);
		wtk_free(feat->log);
		wtk_free(feat->prob);
		wtk_vpool2_push(ir->pool, feat);
	}
}

void qtk_img2_rec_merge_q(qtk_img2_rec_t *ir)
{
	// wtk_debug("merge\n");
	qtk_ifeat2_t *feat;
	wtk_queue_node_t *qn;

	while (1)
	{
		qn = wtk_queue_pop(&(ir->sil_q));
		if (!qn)
		{
			break;
		}
		feat = data_offset(qn, qtk_ifeat2_t, q_n);
		wtk_queue_push(&(ir->q), &(feat->q_n));
	}
}

void qtk_img2_feat_cal(qtk_img2_rec_t *ir, int split)
{
    int start, end, cal_end = ir->q.length;
    qtk_ifeat2_t *feat;
    wtk_queue_node_t *qn;
    qn = ir->q.pop;
    feat = (qtk_ifeat2_t *)data_offset(qn, qtk_ifeat2_t, q_n);
    start = feat->index;
    qn = ir->q.push;
    feat = (qtk_ifeat2_t *)data_offset(qn, qtk_ifeat2_t, q_n);
    end = feat->index;
    qtk_ifeat2_t *show_feat;
    wtk_queue_node_t *show_qn;
    int id = 0;
    int cor = 0;
    int i;
    int cnt_prob = 0;
    float max_avx = 0;
    float val_prob = 0;
    float max_prob = 0;
    float av_prob = 0;
    float sum_prob = 0;
    float *valx;
    float *maxx;
    float *avx;
    float *sumx;
    valx = (float *)wtk_calloc(ir->out_num, sizeof(float)); 
    maxx = (float *)wtk_calloc(ir->out_num, sizeof(float)); 
    avx = (float *)wtk_calloc(ir->out_num, sizeof(float)); 
    sumx = (float *)wtk_calloc(ir->out_num, sizeof(float)); 
    for (i = 0; i < ir->out_num; i++)
    {
        valx[i] = 0;
        maxx[i] = 0;
        avx[i] = 0;
        sumx[i] = 0;
    }

	qtk_img2_thresh_cfg_t *thresh = ir->sp_sil?ir->thresh:ir->thresh_echo;

    if (split == 1)
    {
        cal_end = ir->check_index - ir->sil_num;
    }

    for (show_qn = ir->q.pop; show_qn; show_qn = show_qn->next)
    {
        cnt_prob++;
        show_feat = (qtk_ifeat2_t *)data_offset(show_qn, qtk_ifeat2_t, q_n);
        val_prob = log(1 - exp(show_feat->log[0]) + 1e-12) - log(exp(show_feat->log[0]) + 1e-12);
        max_prob = (val_prob > max_prob) ? val_prob : max_prob;
        sum_prob += val_prob;
        for (i = 1; i < ir->out_num; i++)
        {
            valx[i] = show_feat->log[i] - show_feat->log[0];
            maxx[i] = (valx[i] > maxx[i]) ? valx[i] : maxx[i];
            sumx[i] += valx[i];
            if (split && cnt_prob > cal_end)
            {
                break;
            }
        }
    }

    av_prob = sum_prob / cnt_prob;
    for (i = 1; i < ir->out_num; i++)
    {
        avx[i] = sumx[i] / cnt_prob;
    }
    for (i = 1; i < ir->out_num; i++)
    {
        if (avx[i] > max_avx)
        {
            max_avx = avx[i];
            id = i;
        }
    }

#if 0
    wtk_debug("len = %d start = %f, end = %f, id = %d\n", ir->q.length, start * 0.08, end * 0.08, id);
    if (0 == ir->idle)
    {
        wtk_debug("av_prob = %f(%f) maxx = %.3f(%.3f) avx = %.3f(%.3f) max_prob = %.3f(%.3f) idle = %d\n",av_prob, thresh->av_prob0, maxx[id], thresh->maxx0, avx[id], thresh->avx0, max_prob, thresh->max_prob0, ir->idle);
    }
    else
    {
        wtk_debug("av_prob = %f(%f) maxx = %.3f(%.3f) avx = %.3f(%.3f) max_prob = %.3f(%.3f) idle = %d\n",av_prob, thresh->av_prob1, maxx[id], thresh->maxx1, avx[id], thresh->avx1, max_prob, thresh->max_prob1, ir->idle);
    }
#endif

	if (!ir->idle)
    {
        if (maxx[id] > thresh->maxx0 && avx[id] > thresh->avx0 && av_prob > thresh->av_prob0
                && max_prob > thresh->max_prob0 && ir->q.length > thresh->speech_dur0)
        {
			cor = 1;
			ir->waked = 1;
			ir->start = start;
			ir->end = end;
		}
	}
	else
    {
        if (maxx[id] > thresh->maxx1 && avx[id] > thresh->avx1 && av_prob > thresh->av_prob1
                && max_prob > thresh->max_prob1 && ir->q.length > thresh->speech_dur1)
        {
			cor = 1;
			ir->waked = 1;
			ir->start = start;
			ir->end = end;
		}
	}
   
    if (cor == 1 && ir->notify)
    {
        ir->notify(ir->notify_ths, id, avx[id], start, end);
        qtk_img2_rec_reset2(ir);
        ir->state = QTK_IMG2_WAIT;
        int i;
        for (i = 0; i < ir->cfg->channels; i++) {
            if (ir->kparm[i]->cfg->use_trick) {
                ir->kparm[i]->idle_hint = 0;
                ir->idle = 0;
            }
        }
    }
    else
    {
        qtk_img2_rec_reset2(ir);
    }
    free(valx);
    free(maxx);
    free(avx);
    free(sumx);
}

void qtk_img2_rec_nnet3_notify_end(qtk_img2_rec_t *ir)
{
	if (ir->q.length > 0)
    {
        qtk_img2_feat_cal(ir, 0);
    }
    int i;
    for (i = 0; i < ir->cfg->channels; i++) {
        if (ir->kparm[i]->cfg->use_trick) {
            if (ir->kparm[i]->idle_hint >
                ir->kparm[i]->cfg->idle_trigger_frame) {
                ir->idle = 1;
            }
        }
    }
}

void qtk_img2_rec_nnet3_process(qtk_img2_rec_t *ir, float *f1, int f_dim) {
    int i;
    for (i = 0; i < ir->cfg->channels; i++) {
        if (ir->kparm[i]->cfg->use_trick) {
            if (ir->idle == 0 && ir->kparm[i]->idle_hint >
                                     ir->kparm[i]->cfg->idle_trigger_frame) {
                ir->idle = 1;
            }
        }
    }
    if (ir->waked) {
        return;
    }
    ir->out_num = f_dim;
    float *f = f1;
    float speech_prob = log(1 + 1e-12 - exp(f[0])) - log(exp(f[0]) + 1e-12);
    qtk_img2_cfg_t *cfg = ir->cfg;
    qtk_ifeat2_t *ifeat;
    switch (ir->state) {
    case QTK_IMG2_SIL:
#if DEBUG 
            wtk_debug("SIL:\n");
#endif
            ir->check_index = 0;
            if (speech_prob > cfg->min_vad_thresh)
            {
                ir->state = QTK_IMG2_SPEECH;
                ifeat = qtk_img2_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
            }
            break;
        case QTK_IMG2_SPEECH:
#if DEBUG
            wtk_debug("SPEECH:\n");
#endif
            if (speech_prob > cfg->min_vad_thresh)
            {
                ifeat = qtk_img2_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
#if 0
                if(ir->q.length > 2)
                {
                    qtk_img2_feat_cal(ir, 0);
                }
#endif
            }
            else
            {
                if (qtk_img2_rec_speech_check(ir))
                {
                    ir->state = QTK_IMG2_TBD;
                    ir->sil_num = 1;
                    ifeat = qtk_img2_rec_feat_new(ir, f);
                    wtk_queue_push(&(ir->sil_q), &(ifeat->q_n));
                }
                else
                {
                    // wtk_debug("check fail\n");
                    if (ir->check_index > 0)
                    {
                        qtk_img2_feat_cal(ir, 1);
                    }
                    else
                    {
                        qtk_img2_rec_clear_q(ir, &(ir->q));
                    }
                    ir->state = QTK_IMG2_SIL;
                }
            }
            break;
        case QTK_IMG2_TBD:
#if debug 
            wtk_debug("TBD:\n");
#endif
            if (speech_prob > cfg->min_vad_thresh)
            {
                //qtk_img2_rec_merge_q(ir);
                ir->check_index = ir->q.length;
                ifeat = qtk_img2_rec_feat_new(ir, f);
                ifeat->speech_prob = speech_prob;
                wtk_queue_push(&(ir->q), &(ifeat->q_n));
                ir->state = QTK_IMG2_SPEECH;
            }
            else
            {
                ir->sil_num++;
                if (ir->sil_num > cfg->min_noise_dur)
                {
                    qtk_img2_rec_clear_q(ir, &(ir->sil_q));
                    qtk_img2_feat_cal(ir, 0); // start rec
                    qtk_img2_rec_clear_q(ir, &(ir->q));
                    ir->sil_num = 0;
                }
                else
                {
                    ifeat = qtk_img2_rec_feat_new(ir, f);
                    wtk_queue_push(&(ir->sil_q), &(ifeat->q_n));
                }
            }
            break;
        case QTK_IMG2_WAIT:
#if DEBUG 
            wtk_debug("WAIT:");
#endif
            ir->wait_count++;
            if (ir->wait_count > 20)
            {
                ir->state = QTK_IMG2_SIL;
                ir->wait_count = 0;
            }
#if DEBUG
            wtk_debug("wait===========wait_count = %d\n",ir->wait_count);
#endif
            break;
        default:
            break;
    }
    ir->index++;
}

void qtk_img2_rec_nnet3_notify(qtk_img2_rec_t *ir, qtk_blas_matrix_t *f1,
                               int end, int plus) {
    unsigned int i, row, col;
    float *m;

    if (f1) {
        row = f1->row;
        col = f1->col;
        m = f1->m;
        for (i = 0; i < row; i++) {
            qtk_img2_rec_nnet3_process(ir, m + i * col, col);
        }
    }
    if (end == 1) {
        if (ir->q.length > 0) {
            qtk_img2_feat_cal(ir, 0);
        }

        for (i = 0; i < ir->cfg->channels; i++) {
            if (ir->kparm[i]->cfg->use_trick) {
                if (ir->kparm[i]->idle_hint >
                    ir->kparm[i]->cfg->idle_trigger_frame) {
                    ir->idle = 1;
                }
            }
        }
        return;
    }
}

void qtk_img2_rec_get_sp_sil(qtk_img2_rec_t *ir, int sp_sil)
{
	 ir->sp_sil = sp_sil;
}

void qtk_img2_rec_start(qtk_img2_rec_t *ir) {
    int i;
    for (i = 0; i < ir->cfg->channels; i++) {
        wtk_kparm_start(ir->kparm[i]);
    }
}

int qtk_img2_rec_feed(qtk_img2_rec_t *ir, char *data, int bytes, int is_end) {
    int len = bytes >> 3;
    int queue_len = 0;
    wtk_kfeat_t **feat;
    wtk_queue_node_t **qn;
    feat =
        (wtk_kfeat_t **)wtk_malloc(sizeof(wtk_kfeat_t *) * ir->cfg->channels);
    qn = (wtk_queue_node_t **)wtk_malloc(sizeof(wtk_queue_node_t *) *
                                         ir->cfg->channels);

    if (!is_end) {
        int i,j;
        for (i = 0; i < len; i++) {
            for (j = 0; j < ir->cfg->channels; j++) {
                wtk_kparm_feed(ir->kparm[j],
                               (short *)data + i * ir->cfg->channels + j, 1,
                               is_end);
            }
        }
    } else {
        int i;
        for (i = 0; i < ir->cfg->channels; i++) {
            wtk_kparm_feed(ir->kparm[i], NULL, 0, is_end);
        }
    }
    if (!is_end) {
        queue_len = ir->kparm[0]->cmvn->feat_q.length;
        int i,j;
        for (j = 0; j < queue_len; j++) {
            wtk_kfeat_t *total_feat;
            total_feat = wtk_kfeat_new(ir->feature_cols);
            for (i = 0; i < ir->cfg->channels; i++) {
                qn[i] = wtk_queue_pop(&(ir->kparm[i]->cmvn->feat_q));
                if (!qn) {
                    break;
                }
                feat[i] = data_offset2(qn[i], wtk_kfeat_t, feat_n);
                memcpy(total_feat->v + ir->feature_cols / 4 * i, feat[i]->v,
                       ir->feature_cols / 4 * sizeof(float));
                if (i == ir->cfg->channels - 1) {
                    total_feat->index = feat[i]->index;
                }
            }
#if DEBUG
            for (i = 0; i < ir->feature_cols; i++) {
                printf("%f ", total_feat->v[i]);
            }
            printf("\n");
#endif
            qtk_nnet3_run_kfeat(ir->nnet3, total_feat, is_end);
            for (i = 0; i < ir->cfg->channels; i++) {
                feat[i]->used--;
                wtk_kparm_push_feat(ir->kparm[i], feat[i]);
            }
            wtk_kfeat_delete(total_feat);
        }
    } else {
        qtk_nnet3_run_kfeat(ir->nnet3, NULL, is_end);
    }

    wtk_free(feat);
    wtk_free(qn);
    return ir->waked;
}

int qtk_img2_rec_get_time(qtk_img2_rec_t *ir, float *fs, float *fe)
{
	*fs = ir->start * 0.01 * 4.0 - 16 * 0.01;
	*fe = ir->end * 0.01 * 4.0 - 16 * 0.01;
	// printf("%d %d %f %f\n",ir->start,ir->end,*fs,*fe);
	return 0;
}

float qtk_img2_rec_get_conf(qtk_img2_rec_t *ir)
{
	return ir->prob;
}

void qtk_img2_thresh_set_cfg(qtk_img2_rec_t *ir, qtk_img2_thresh_cfg_t *thresh, int echo)
{
	if(echo){
		ir->thresh_echo = thresh;
	}else{
		ir->thresh = thresh;
	}
}
