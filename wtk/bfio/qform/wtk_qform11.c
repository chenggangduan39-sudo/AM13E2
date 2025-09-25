#include "wtk_qform11.h"

void wtk_qform11_on_stft2(wtk_qform11_t *qform11, wtk_stft2_msg_t *msg, int pos, int is_end);
void wtk_qform11_on_qenvelope(wtk_qform11_t *qform11, wtk_qenvelope_msg_t *msg, wtk_qenvelope_state_t state, int is_end, int idx);

wtk_stft2_msg_t* wtk_qform11_stft2_msg_new(wtk_qform11_t *qform11)
{
	wtk_stft2_msg_t *msg;

	msg=(wtk_stft2_msg_t*)wtk_malloc(sizeof(wtk_stft2_msg_t));
	msg->hook=NULL;
	msg->fft=wtk_complex_new_p2(qform11->nbin,qform11->bf[0]->channel);
	return msg;
}

void wtk_qform11_stft2_msg_delete(wtk_qform11_t *qform11,wtk_stft2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,qform11->nbin);
	wtk_free(msg);
}

wtk_qform11_envelopemsg_t *wtk_qform11_envelope_msg_new(wtk_qform11_t *qform11)
{
    wtk_qform11_envelopemsg_t *msg;

    msg=(wtk_qform11_envelopemsg_t *)wtk_malloc(sizeof(wtk_qform11_envelopemsg_t));
    msg->smsg=wtk_qform11_stft2_msg_new(qform11);
    msg->cohv=(float *)wtk_malloc(sizeof(float)*qform11->nbin);

    return msg;
}

void wtk_qform11_envelope_msg_delete(wtk_qform11_t *qform11, wtk_qform11_envelopemsg_t *qemsg)
{
    wtk_free(qemsg->cohv);
    wtk_qform11_stft2_msg_delete(qform11,qemsg->smsg);
    wtk_free(qemsg);
}

wtk_qform11_envelopemsg_t* wtk_qform11_pop_envelope_msg(wtk_qform11_t *qform11)
{
	return  (wtk_qform11_envelopemsg_t*)wtk_hoard_pop(&(qform11->qenvel_msg_hoard));
}

void wtk_qform11_push_envelope_msg(wtk_qform11_t *qform11,wtk_qform11_envelopemsg_t *msg)
{
	wtk_hoard_push(&(qform11->qenvel_msg_hoard),msg);
}

wtk_qform11_envelopemsg_t *wtk_qform11_envelope_msg_copy(wtk_qform11_t *qform11, wtk_stft2_msg_t *smsg, float *cohv, int nbin, int channel)
{
    wtk_qform11_envelopemsg_t *qemsg;

    qemsg=wtk_qform11_pop_envelope_msg(qform11);
    qemsg->smsg->hook=NULL;
    qemsg->smsg->s=smsg->s;
    wtk_complex_cpy_p2(qemsg->smsg->fft, smsg->fft, nbin, channel);
    memcpy(qemsg->cohv, cohv, sizeof(float)*nbin);
    return qemsg;
}

wtk_qform11_t *wtk_qform11_new(wtk_qform11_cfg_t *cfg)
{
    wtk_qform11_t *qform11;
    int i;

    qform11 = (wtk_qform11_t *)wtk_malloc(sizeof(wtk_qform11_t));
    qform11->cfg = cfg;
    qform11->ths = NULL;
    qform11->notify = NULL;

    qform11->notch_mem = NULL;
    qform11->memD = NULL;
    qform11->memX = NULL;
    if (cfg->use_preemph)
    {
        qform11->notch_mem = (float **)wtk_malloc(sizeof(float *) * cfg->stft2.channel);
        for (i = 0; i < cfg->stft2.channel; ++i)
        {
            qform11->notch_mem[i] = (float *)wtk_malloc(sizeof(float) * 2);
        }
        qform11->memD = (float *)wtk_malloc(sizeof(float) * cfg->stft2.channel);
        qform11->memX = (float *)wtk_malloc(sizeof(float) * cfg->nmulchannel);
    }

    qform11->input = NULL;
    if (cfg->use_preemph)
    {
        qform11->input = wtk_strbufs_new(cfg->bf.nmic);
    }

    qform11->stft2 = wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(qform11->stft2, qform11, (wtk_stft2_notify_f)wtk_qform11_on_stft2);

    qform11->nbin = qform11->stft2->nbin;

    qform11->covm = NULL;
    qform11->covm = (wtk_covm_t **)wtk_malloc(sizeof(wtk_covm_t *) * cfg->nmulchannel);
    qform11->bf = NULL;
    qform11->bf = (wtk_bf_t **)wtk_malloc(sizeof(wtk_bf_t *) * cfg->nmulchannel);
    for (i = 0; i < cfg->nmulchannel; ++i)
    {
        qform11->covm[i] = wtk_covm_new(&(cfg->covm), qform11->nbin, cfg->stft2.channel);
        qform11->bf[i] = wtk_bf_new(&(cfg->bf), cfg->stft2.win);
    }

    qform11->aspec = NULL;
    qform11->aspec = wtk_aspec_new(&(cfg->aspec), qform11->stft2->nbin, cfg->naspec);
    qform11->aspec_theta_idx=(int *)wtk_malloc(sizeof(int)*cfg->naspec);
    qform11->spec_k=(float *)wtk_malloc(sizeof(float)*cfg->naspec);
    qform11->specsum=(float **)wtk_malloc(sizeof(float*)*cfg->nmulchannel);
    for(i=0;i<cfg->nmulchannel;++i){
        qform11->specsum[i]=(float *)wtk_malloc(sizeof(float)*(cfg->naspec+1));
    }
    qform11->freqsum=(int *)wtk_malloc(sizeof(int)*cfg->nmulchannel);
    qform11->sum_count=(int *)wtk_malloc(sizeof(int)*cfg->nmulchannel);

    qform11->cov = NULL;
    wtk_queue_init(&(qform11->stft2_q));
    if (qform11->aspec && qform11->aspec->need_cov)
    {
        qform11->cov = (wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t) * cfg->stft2.channel * cfg->stft2.channel);
        if (cfg->lt <= 0)
        {
            qform11->wint = wtk_malloc(sizeof(float));
            qform11->wint[0] = 1;
        }
        else
        {
            qform11->wint = wtk_math_create_hanning_window(2 * cfg->lt + 1);
        }

        if (cfg->lf <= 0)
        {
            qform11->winf = wtk_malloc(sizeof(float));
            qform11->winf[0] = 1;
        }
        else
        {
            qform11->winf = wtk_math_create_hanning_window(2 * cfg->lf + 1);
        }
    }
    qform11->inv_cov = NULL;
    qform11->tmp = NULL;
    if (qform11->aspec && qform11->aspec->need_inv_cov)
    {
        qform11->inv_cov = (wtk_complex_t *)wtk_malloc(cfg->stft2.channel * cfg->stft2.channel * sizeof(wtk_complex_t));
        qform11->tmp = (wtk_dcomplex_t *)wtk_malloc(cfg->stft2.channel * cfg->stft2.channel * 2 * sizeof(wtk_dcomplex_t));
    }

    qform11->qmmse = NULL;
    if (cfg->use_post)
    {
        qform11->qmmse = (wtk_qmmse_t **)wtk_malloc(sizeof(wtk_qmmse_t *) * cfg->nmulchannel);
        for (i = 0; i < cfg->nmulchannel; ++i)
        {
            qform11->qmmse[i] = wtk_qmmse_new(&(cfg->qmmse));
        }
    }

    qform11->cohv = NULL;
    qform11->cohv = (float **)wtk_malloc(sizeof(float *) * cfg->nmulchannel);
    for (i = 0; i < cfg->nmulchannel; ++i)
    {
        qform11->cohv[i] = (float *)wtk_malloc(sizeof(float) * qform11->bf[i]->nbin);
    }

    qform11->cohv_fn = NULL;
    if (cfg->debug)
    {
        qform11->cohv_fn = (FILE **)wtk_malloc(sizeof(FILE *) * cfg->nmulchannel);
        char buf[1024];
        for (i = 0; i < cfg->nmulchannel; ++i)
        {
            if(i==0){
                sprintf(buf, "cohv.dat");
            }else{
                sprintf(buf, "cohv%d.dat", i);
            }
            qform11->cohv_fn[i] = fopen(buf, "w");
        }
    }

    qform11->qenvelope = NULL;
    if (cfg->use_qenvelope)
    {
        wtk_hoard_init2(&(qform11->qenvel_msg_hoard),offsetof(wtk_qform11_envelopemsg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform11_envelope_msg_new,
            (wtk_delete_handler2_t)wtk_qform11_envelope_msg_delete,
            qform11);
        qform11->qenvelope = (wtk_qenvelope_t **)wtk_malloc(sizeof(wtk_qenvelope_t *) * cfg->nmulchannel);
        for (i = 0; i < cfg->nmulchannel; ++i)
        {
            wtk_qenvelope_cfg_set_idx(&(cfg->qenvl), i);
            qform11->qenvelope[i] = wtk_qenvelope_new(&(cfg->qenvl));
            wtk_qenvelope_set_notify(qform11->qenvelope[i], qform11, (wtk_qenvelope_notify_f)wtk_qform11_on_qenvelope);
        }
    }

    qform11->out_buf = wtk_strbufs_new(cfg->nmulchannel);
    qform11->output=(short *)wtk_malloc(sizeof(short)*cfg->out_len*cfg->nmulchannel);

    wtk_qform11_reset(qform11);

    return qform11;
}

void wtk_qform11_delete(wtk_qform11_t *qform11)
{
    int i;
    int nmulchannel = qform11->cfg->nmulchannel;
    int channel = qform11->bf[0]->channel;

    if (qform11->notch_mem)
    {
        for (i = 0; i < channel; ++i)
        {
            wtk_free(qform11->notch_mem[i]);
        }
        wtk_free(qform11->notch_mem);
        wtk_free(qform11->memD);
        wtk_free(qform11->memX);
    }

    if (qform11->input)
    {
        wtk_strbufs_delete(qform11->input, qform11->bf[0]->channel);
    }
    if (qform11->covm)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_covm_delete(qform11->covm[i]);
        }
        wtk_free(qform11->covm);
    }
    if (qform11->bf)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_bf_delete(qform11->bf[i]);
        }
        wtk_free(qform11->bf);
    }
    if (qform11->aspec)
    {
        wtk_aspec_delete(qform11->aspec);
    }
    wtk_free(qform11->aspec_theta_idx);
    wtk_free(qform11->spec_k);
    for (i = 0; i < nmulchannel; ++i)
    {
        wtk_free(qform11->specsum[i]);
    }
    wtk_free(qform11->specsum);
    wtk_free(qform11->freqsum);
    wtk_free(qform11->sum_count);
    if (qform11->cov)
    {
        wtk_free(qform11->cov);
        wtk_free(qform11->wint);
        wtk_free(qform11->winf);
    }
    if (qform11->inv_cov)
    {
        wtk_free(qform11->inv_cov);
    }
    if (qform11->tmp)
    {
        wtk_free(qform11->tmp);
    }
    if (qform11->qmmse)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_qmmse_delete(qform11->qmmse[i]);
        }
        wtk_free(qform11->qmmse);
    }
    if (qform11->cohv)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_free(qform11->cohv[i]);
        }
        wtk_free(qform11->cohv);
    }
    if (qform11->cohv_fn)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            fclose(qform11->cohv_fn[i]);
        }
        wtk_free(qform11->cohv_fn);
    }
    if (qform11->qenvelope)
    {
	    wtk_hoard_clean(&(qform11->qenvel_msg_hoard));
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_qenvelope_delete(qform11->qenvelope[i]);
        }
        wtk_free(qform11->qenvelope);
    }

    wtk_strbufs_delete(qform11->out_buf, nmulchannel);
    wtk_free(qform11->output);

    wtk_stft2_delete(qform11->stft2);
    wtk_free(qform11);
}

void wtk_qform11_reset(wtk_qform11_t *qform11)
{
    int channel = qform11->bf[0]->channel;
    int nmulchannel = qform11->cfg->nmulchannel;
    int naspec=qform11->cfg->naspec;
    int out_len=qform11->cfg->out_len;
    int i;

    qform11->end_pos = 0;

    if (qform11->notch_mem)
    {
        for (i = 0; i < channel; ++i)
        {
            memset(qform11->notch_mem[i], 0, sizeof(float) * 2);
        }
        memset(qform11->memD, 0, sizeof(float) * channel);
        memset(qform11->memX, 0, sizeof(float) * nmulchannel);
    }

    if (qform11->input)
    {
        wtk_strbufs_reset(qform11->input, qform11->bf[0]->channel);
    }

    wtk_stft2_reset(qform11->stft2);

    if (qform11->covm)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_covm_reset(qform11->covm[i]);
        }
    }

    if (qform11->bf)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_bf_reset(qform11->bf[i]);
        }
    }

    if (qform11->aspec)
    {
        wtk_aspec_reset(qform11->aspec);
    }
    memset(qform11->aspec_theta_idx, 0, sizeof(int)*naspec);
    memset(qform11->spec_k, 0, sizeof(float)*naspec);
    for(i=0;i<nmulchannel;++i){
        memset(qform11->specsum[i], 0, sizeof(float)*(naspec+1));
    }
    memset(qform11->freqsum, 0, sizeof(int)*nmulchannel);
    memset(qform11->sum_count, 0, sizeof(int)*nmulchannel);
    wtk_queue_init(&(qform11->stft2_q));

    if (qform11->qmmse)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_qmmse_reset(qform11->qmmse[i]);
        }
    }

    if (qform11->cohv)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            memset(qform11->cohv[i], 0, sizeof(float) * qform11->bf[0]->nbin);
        }
    }

    if (qform11->qenvelope)
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_qenvelope_reset(qform11->qenvelope[i]);
        }
    }
    wtk_strbufs_reset(qform11->out_buf, nmulchannel);
    memset(qform11->output, 0, sizeof(short)*out_len*nmulchannel);

    qform11->nframe = 0;
    qform11->theta = qform11->phi = -1;
}

void wtk_qform11_set_notify(wtk_qform11_t *qform11, void *ths, wtk_qform11_notify_f notify)
{
    qform11->ths = ths;
    qform11->notify = notify;
}

void wtk_qform11_notify_data(wtk_qform11_t *qform11, float *data, int len, int chn)
{
    short *pv = (short *)data;
    short *output = qform11->output;
    int i,j;
    wtk_strbuf_t **out_buf=qform11->out_buf;
    int nmulchannel=qform11->cfg->nmulchannel;
    int fsize=qform11->cfg->out_len;
    int length;

    if (qform11->cfg->use_preemph)
    {
        qform11->memX[chn] = wtk_preemph_asis2(data, len, qform11->memX[chn]);
    }
    for (i = 0; i < len; ++i)
    {
        if (fabs(data[i]) < 1.0)
        {
            pv[i] = data[i] * 32000;
        }
        else
        {
            if (data[i] > 0)
            {
                pv[i] = 32000;
            }
            else
            {
                pv[i] = -32000;
            }
        }
    }
    wtk_strbuf_push(out_buf[chn], (char *)pv, len<<1);
    length = out_buf[0]->pos>>1;
    for(i=1;i<nmulchannel;++i){
        length = min(length, out_buf[i]->pos>>1);
    }
    while (length >= fsize)
    {
        for(i=0;i<fsize;++i){
            for(j=0;j<nmulchannel;++j){
                pv = (short *)out_buf[j]->data;
                output[i*nmulchannel+j]=pv[i];
            }
        }
        if(qform11->notify){
            qform11->notify(qform11->ths, output, fsize * nmulchannel, 0);
        }
        wtk_strbufs_pop(out_buf, nmulchannel, fsize*(sizeof(short)));
        length = out_buf[0]->pos>>1;
        for(i=1;i<nmulchannel;++i){
            length = min(length, out_buf[i]->pos>>1);
        }
    }
}

void wtk_qform11_flush2(wtk_qform11_t *qform11, wtk_stft2_msg_t *smsg, float *cohv, int chn, int is_end)
{
    int k;
    wtk_complex_t *bf_out = NULL;
    static int state=0;
    int nmulchannel=qform11->cfg->nmulchannel;
    // int nbin=qform11->bf->nbin;
    if (smsg)
    {
        bf_out = wtk_bf_output_fft2_msg2(qform11->bf[chn], smsg, cohv);
        if (qform11->qmmse[chn])
        {
            wtk_qmmse_feed_cohv(qform11->qmmse[chn], bf_out, cohv);
        }
    }
    if (qform11->notify)
    {
        if (bf_out)
        {
            k = wtk_stft2_output_ifft(qform11->stft2, bf_out, qform11->stft2->output, qform11->bf[chn]->pad, qform11->end_pos, is_end);
            wtk_qform11_notify_data(qform11, qform11->stft2->output, k, chn);
        }
        if (is_end)
        {
            ++state;
        }
        if(state == nmulchannel){
            qform11->notify(qform11->ths, NULL, 0, 1);
        }
    }
}

void wtk_qform11_flush(wtk_qform11_t *qform11, wtk_stft2_msg_t *smsg, float *cohv, int chn, int is_end)
{
    int k;
    int nbin = qform11->bf[0]->nbin;
    int i, channel = qform11->bf[0]->channel;
    int b;
    wtk_covm_t **covm = qform11->covm;

    for (k = 1; k < nbin - 1; ++k)
    {
        b = 0;
        if (cohv[k] < 0.0)
        {
            b = wtk_covm_feed_fft2(covm[chn], smsg->fft, k, 1);
            if (b == 1)
            {
                wtk_bf_update_ncov(qform11->bf[chn], covm[chn]->ncov, k);
            }
        }
        else
        {
            if (covm[chn]->scov)
            {
                b = wtk_covm_feed_fft2(covm[chn], smsg->fft, k, 0);
                if (b == 1)
                {
                    wtk_bf_update_scov(qform11->bf[chn], covm[chn]->scov, k);
                }
            }
        }
        if (covm[chn]->ncnt_sum[k] > 5 && (covm[chn]->scnt_sum == NULL || covm[chn]->scnt_sum[k] > 5) && b == 1)
        {
            wtk_bf_update_w(qform11->bf[chn], k);
        }

        if (qform11->cfg->debug)
        {
            if (cohv[k] < 0)
            {
                for (i = 0; i < channel; ++i)
                {
                    qform11->bf[chn]->w[k][i].a = 0;
                    qform11->bf[chn]->w[k][i].b = 0;
                }
            }
            else
            {
                for (i = 0; i < channel; ++i)
                {
                    qform11->bf[chn]->w[k][i].a = 0;
                    qform11->bf[chn]->w[k][i].b = 0;
                    if (i == 0)
                    {
                        qform11->bf[chn]->w[k][i].a = 1;
                    }
                }
            }
        }
    }
    wtk_qform11_flush2(qform11, smsg, cohv, chn, is_end);
}

void wtk_qform11_on_qenvelope(wtk_qform11_t *qform11, wtk_qenvelope_msg_t *msg, wtk_qenvelope_state_t state, int is_end, int idx)
{
    wtk_qform11_envelopemsg_t *qemsg;
    int k;
    int nbin = qform11->nbin;

    if (msg)
    {
        qemsg = (wtk_qform11_envelopemsg_t *)msg->hook;
        if (state == WTK_QENVELOPE_TROUGH)
        {
            for (k = 1; k < nbin - 1; ++k)
            {
                qemsg->cohv[k] = -1;
            }
        }
        else if (state == WTK_QENVELOPE_CREST || state == WTK_QENVELOPE_FLAT)
        {
            if (qform11->cfg->use_sqenvelope)
            {
                for (k = 1; k < nbin - 1; ++k)
                {
                    qemsg->cohv[k] = 1;
                }
            }
        }
        wtk_qform11_flush(qform11, qemsg->smsg, qemsg->cohv, idx, is_end);
        wtk_qform11_push_envelope_msg(qform11, qemsg);
    }
    else if (is_end)
    {
        wtk_qform11_flush2(qform11, NULL, NULL, idx, 1);
    }
}

void wtk_qform11_update_aspec(wtk_qform11_t *qform11, wtk_aspec_t *aspec, wtk_complex_t *cov,
                              wtk_complex_t *inv_cov, float cov_travg, int k, float *spec_k, 
                              float **cohv, float **specsum, int *freqsum, int use_sum)
{
    int naspec=qform11->cfg->naspec;
    int nmulchannel=qform11->cfg->nmulchannel;
    float **mul_theta_range=qform11->cfg->mul_theta_range;
    int *aspec_theta_idx=qform11->aspec_theta_idx;
    int min_idx, max_idx;
    int i,j;

    for(i=0;i<naspec;++i){
        if(aspec_theta_idx[i]==1){
            spec_k[i] = wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k, i);
            // wtk_debug("i=[%d] theta=[%f]\n", i, qform11->cfg->aspec_theta[i]);
        }
    }
    if(use_sum){
        for(i=0;i<nmulchannel;++i){
            cohv[i][k]=-1;
            min_idx=mul_theta_range[i][4];
            max_idx=mul_theta_range[i][5];
            for(j=max(min_idx-2,0)+2;j<=min(max_idx+2,naspec)-2;++j){
                // wtk_debug("i=[%d] j=[%d] min=[%d] max=[%d] theta=[%f]\n", i, j, min_idx, max_idx, qform11->cfg->aspec_theta[j]);
                if(spec_k[j] > spec_k[j-2] && spec_k[j] > spec_k[j+2]){
                    specsum[i][j] += 2 * spec_k[j] - spec_k[j-2] - spec_k[j+2];
                    freqsum[i] += 1;
                    cohv[i][k]=1;
                }
            }
        }
    }else{
        for(i=0;i<nmulchannel;++i){
            cohv[i][k]=-1;
            min_idx=mul_theta_range[i][4];
            max_idx=mul_theta_range[i][5];
            for(j=max(min_idx-2,0)+2;j<min(max_idx+2,naspec)-2;++j){
                if(spec_k[j] > spec_k[j-2] && spec_k[j] > spec_k[j+2]){
                    cohv[i][k]=1;
                }
            }
        }
    }
}

void wtk_qform11_flush_aspec_lt(wtk_qform11_t *qform11, int index, int is_end)
{
    wtk_queue_t *stft2_q = &(qform11->stft2_q);
    int lf = qform11->cfg->lf;
    int lt = qform11->cfg->lt;
    int i, j, k, k2, tt, ff;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg, *smsg_index;
    int nbin = qform11->nbin;
    int channel = qform11->bf[0]->channel;
    int nmulchannel=qform11->cfg->nmulchannel;
    wtk_complex_t *cov = qform11->cov;
    wtk_complex_t **fft, *fft1, *fft2, *a, *b;
    float *wint = qform11->wint;
    float *winf = qform11->winf;
    float wint2, wintf, winsum;
    wtk_complex_t *inv_cov = qform11->inv_cov;
    wtk_dcomplex_t *tmp = qform11->tmp;
    float cov_travg;
    int ret;
    float **cohv = qform11->cohv;
    float *spec_k = qform11->spec_k;
    float **specsum = qform11->specsum;
    int *freqsum = qform11->freqsum;
    // int *sum_count = qform11->sum_count;
    int specsum_ns = qform11->cfg->specsum_ns;
    int specsum_ne = qform11->cfg->specsum_ne;
    int naspec=qform11->cfg->naspec;

    ++qform11->nframe;
    qn = wtk_queue_peek(stft2_q, index);
    smsg_index = data_offset2(qn, wtk_stft2_msg_t, q_n);

    for (k = 1; k < nbin - 1; ++k)
    {
        memset(cov, 0, sizeof(wtk_complex_t) * channel * channel);
        winsum = 0;
        for (qn = stft2_q->pop, tt = 2 * lt + 1 - stft2_q->length; qn; qn = qn->next, ++tt)
        {
            wint2 = wint[tt];
            smsg = data_offset2(qn, wtk_stft2_msg_t, q_n);
            fft = smsg->fft;
            for (k2 = max(1, k - lf), ff = k2 - (k - lf); k2 < min(nbin - 1, k + lf + 1); ++k2, ++ff)
            {
                wintf = wint2 * winf[ff];
                winsum += wintf;

                fft1 = fft2 = fft[k2];
                for (i = 0; i < channel; ++i, ++fft1)
                {
                    fft2 = fft1;
                    for (j = i; j < channel; ++j, ++fft2)
                    {
                        a = cov + i * channel + j;
                        if (i != j)
                        {
                            a->a += (fft1->a * fft2->a + fft1->b * fft2->b) * wintf;
                            a->b += (-fft1->a * fft2->b + fft1->b * fft2->a) * wintf;
                        }
                        else
                        {
                            a->a += (fft1->a * fft2->a + fft1->b * fft2->b) * wintf;
                            a->b += 0;
                        }
                    }
                }
            }
        }
        winsum = 1.0 / winsum;
        for (i = 0; i < channel; ++i)
        {
            for (j = i; j < channel; ++j)
            {
                a = cov + i * channel + j;
                a->a *= winsum;
                a->b *= winsum;

                if (i != j)
                {
                    b = cov + j * channel + i;
                    b->a = a->a;
                    b->b = -a->b;
                }
            }
        }
        if (inv_cov)
        {
            ret = wtk_complex_invx4(cov, tmp, channel, inv_cov, 1);
            if (ret != 0)
            {
                j = 0;
                for (i = 0; i < channel; ++i)
                {
                    cov[j].a += 0.01;
                    j += channel + 1;
                }
                wtk_complex_invx4(cov, tmp, channel, inv_cov, 1);
            }
        }
        if (qform11->aspec)
        {
            cov_travg = 0;
            if (qform11->aspec->need_cov_travg)
            {
                for (i = 0; i < channel; ++i)
                {
                    cov_travg += cov[i * channel + i].a;
                }
                cov_travg /= channel;
            }
            if (k >= specsum_ns && k <= specsum_ne)
            {
                wtk_qform11_update_aspec(qform11, qform11->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv, specsum, freqsum, 1);
            }else{
                wtk_qform11_update_aspec(qform11, qform11->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv, specsum, freqsum, 0);
            }
        }
    }
    for(i=0;i<nmulchannel;++i){
        specsum[i][naspec]=0;
        for(j=0;j<naspec;++j){
            if(specsum[i][j]>specsum[i][naspec]){
                specsum[i][naspec]=specsum[i][j];
            }
        }
    }

    if (qform11->cfg->use_qenvelope)
    {
        wtk_qform11_envelopemsg_t *qemsg;

        for(i=0;i<nmulchannel;++i){
            qemsg=wtk_qform11_envelope_msg_copy(qform11, smsg_index, cohv[i], nbin, channel);
            // wtk_qenvelope_feed(qform11->qenvelope[i], specsum[i]*1.0/sum_count[i], (void *)qemsg, is_end);
            wtk_qenvelope_feed(qform11->qenvelope[i], specsum[i][naspec], (void *)qemsg, is_end);
            // wtk_qenvelope_feed(qform11->qenvelope[i], freqsum[i], (void *)qemsg, is_end);
            // wtk_qenvelope_feed(qform11->qenvelope[i], freqsum[i]*specsum[i]*1.0/sum_count[i], (void *)qemsg, is_end);
        }
    }
    else
    {
        for(i=0;i<nmulchannel;++i){
            if (qform11->cohv_fn)
            {
                fprintf(qform11->cohv_fn[i], "%.0f %f\n", qform11->nframe, specsum[i][naspec]);
            }
            wtk_qform11_flush(qform11, smsg_index, cohv[i], i, is_end);
        }
    }
    for(i=0;i<nmulchannel;++i){
        memset(specsum[i], 0, sizeof(float)*naspec);
    }
    memset(freqsum, 0, sizeof(int)*nmulchannel);
}

void wtk_qform11_on_stft2(wtk_qform11_t *qform11, wtk_stft2_msg_t *msg, int pos, int is_end)
{
    wtk_queue_t *stft2_q = &(qform11->stft2_q);
    int lt = qform11->cfg->lt;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg;
    int i;

    if (is_end)
    {
        qform11->end_pos = pos;
    }
    if (qform11->cov)
    {
        if (msg)
        {
            wtk_queue_push(stft2_q, &(msg->q_n));
        }
        if (stft2_q->length >= lt + 1 && stft2_q->length < 2 * lt + 1)
        {
            wtk_qform11_flush_aspec_lt(qform11, stft2_q->length - lt - 1, 0);
        }
        else if (stft2_q->length == 2 * lt + 1)
        {
            wtk_qform11_flush_aspec_lt(qform11, stft2_q->length - lt - 1, (is_end && lt == 0) ? 1 : 0);
            qn = wtk_queue_pop(stft2_q);
            smsg = data_offset2(qn, wtk_stft2_msg_t, q_n);
            wtk_stft2_push_msg(qform11->stft2, smsg);
        }
        else if (is_end && stft2_q->length == 0)
        {
            wtk_qform11_flush2(qform11, NULL, NULL, 0, 1);
        }
        if (is_end)
        {
            if (stft2_q->length > 0)
            {
                if (stft2_q->length < lt + 1)
                {
                    for (i = 0; i < stft2_q->length - 1; ++i)
                    {
                        wtk_qform11_flush_aspec_lt(qform11, i, 0);
                    }
                    wtk_qform11_flush_aspec_lt(qform11, stft2_q->length - 1, 1);
                }
                else
                {
                    for (i = 0; i < lt - 1; ++i)
                    {
                        wtk_qform11_flush_aspec_lt(qform11, stft2_q->length - lt + i, 0);
                    }
                    wtk_qform11_flush_aspec_lt(qform11, stft2_q->length - 1, 1);
                }
            }
            while (qform11->stft2_q.length > 0)
            {
                qn = wtk_queue_pop(&(qform11->stft2_q));
                if (!qn)
                {
                    break;
                }
                smsg = (wtk_stft2_msg_t *)data_offset(qn, wtk_stft2_msg_t, q_n);
                wtk_stft2_push_msg(qform11->stft2, smsg);
            }
        }
    }
}

void wtk_qform11_start_aspec(wtk_qform11_t *qform11)
{
    int naspec = qform11->cfg->naspec;
    float *aspec_theta = qform11->cfg->aspec_theta;
    wtk_aspec_t *aspec = qform11->aspec;
    int i;

    aspec->start_ang_num = naspec;
    for (i = 0; i < naspec; ++i)
    {
        wtk_aspec_start(aspec, aspec_theta[i], 0, i);
    }
}

void wtk_qform11_update_aspec_theta(wtk_qform11_t *qform11)
{
    float *aspec_theta = qform11->cfg->aspec_theta;
    int *aspec_theta_idx = qform11->aspec_theta_idx;
    float **mul_theta_range = qform11->cfg->mul_theta_range;
    int *sum_count = qform11->sum_count;
    int nmulchannel = qform11->cfg->nmulchannel;
    int naspec = qform11->cfg->naspec;
    // int tstep = qform11->cfg->tstep;
    float theta1, theta2;
    int i, j;

    memset(aspec_theta_idx, 0, sizeof(int) * naspec);
    for (i = 0; i < nmulchannel; ++i)
    {
        theta1 = mul_theta_range[i][2];
        theta2 = mul_theta_range[i][3];
        for (j = 0; j < naspec; ++j)
        {
            // if (aspec_theta[j] >= theta1 && aspec_theta[j] <= theta2 && !(aspec_theta[j] == theta1+tstep || aspec_theta[j] == theta2-tstep))
            if (aspec_theta[j] >= theta1 && aspec_theta[j] <= theta2)
            {
                aspec_theta_idx[j] = 1;
            }
        }
        sum_count[i] = (min(mul_theta_range[i][5]+2,naspec)-2) - (max(mul_theta_range[i][4]-2,0)+2) + 1;
    }
    // for(i=0;i<naspec;++i){
    //     if(aspec_theta_idx[i]==1){
    //         wtk_debug("%f\n", aspec_theta[i]);
    //     }
    // }
}

void wtk_qform11_set_theta(wtk_qform11_t *qform11, float theta, float phi, float range, int idx)
{
    int nmulchannel = qform11->cfg->nmulchannel;

    if (idx >= nmulchannel)
    {
        wtk_debug("error idx=[%d] for nmulchannel=[%d]\n", idx, nmulchannel);
        return;
    }
    qform11->cfg->mul_theta_range[idx][0] = theta;
    qform11->cfg->mul_theta_range[idx][1] = range;
    wtk_bf_update_ovec(qform11->bf[idx], theta, 0);
    wtk_bf_init_w(qform11->bf[idx]); // 更换角度是否可以不重置
    wtk_qform11_update_aspec_theta(qform11);
}

void wtk_qform11_start(wtk_qform11_t *qform11, float theta, float phi)
{
    int i;
    int nmulchannel = qform11->cfg->nmulchannel;

    qform11->theta = theta;
    qform11->phi = phi;
    if (nmulchannel == 1)
    {
        wtk_bf_update_ovec(qform11->bf[0], theta, phi);
        wtk_bf_init_w(qform11->bf[0]);
    }
    else
    {
        for (i = 0; i < nmulchannel; ++i)
        {
            wtk_bf_update_ovec(qform11->bf[i], qform11->cfg->mul_theta_range[i][0], 0);
            wtk_bf_init_w(qform11->bf[i]);
        }
    }
    wtk_qform11_start_aspec(qform11);
    wtk_qform11_update_aspec_theta(qform11);
}

void wtk_qform11_feed2(wtk_qform11_t *qform, short **data, int len, int is_end)
{
    int i, j;
    int channel = qform->bf[0]->channel;
    float fv;
    float *fp[10];
    wtk_strbuf_t **input = qform->input;

    for (i = 0; i < channel; ++i)
    {
        wtk_strbuf_reset(input[i]);
        for (j = 0; j < len; ++j)
        {
            fv = data[i][j] / 32768.0;
            wtk_strbuf_push(input[i], (char *)(&fv), sizeof(float));
        }
        fp[i] = (float *)(input[i]->data);
        wtk_preemph_dc(fp[i], qform->notch_mem[i], len);
        qform->memD[i] = wtk_preemph_asis(fp[i], len, qform->memD[i]);
    }
    wtk_stft2_feed_float(qform->stft2, fp, len, is_end);
}

void wtk_qform11_feed(wtk_qform11_t *qform11, short **data, int len, int is_end)
{
#ifdef DEBUG_WAV
    static wtk_wavfile_t *mic_log = NULL;

    if (!mic_log)
    {
        mic_log = wtk_wavfile_new(16000);
        wtk_wavfile_set_channel(mic_log, qform11->bf->channel);
        wtk_wavfile_open2(mic_log, "qform11");
    }
    if (len > 0)
    {
        wtk_wavfile_write_mc(mic_log, data, len);
    }
    if (is_end && mic_log)
    {
        wtk_wavfile_close(mic_log);
        wtk_wavfile_delete(mic_log);
        mic_log = NULL;
    }
#endif
    if (qform11->cfg->use_preemph)
    {
        wtk_qform11_feed2(qform11, data, len, is_end);
    }
    else
    {
        wtk_stft2_feed(qform11->stft2, data, len, is_end);
    }
}
