#include "wtk_bfio.h" 

void wtk_bfio_on_sp_stft2(wtk_bfio_t *bfio,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_bfio_on_stft2(wtk_bfio_t *bfio,wtk_stft2_msg_t *msg,int pos,int is_end);

void wtk_bfio_on_aec(wtk_bfio_t *bfio,wtk_stft2_msg_t *msg,int pos,int is_end);

void wtk_bfio_on_wbf(wtk_bfio_t *bfio,wtk_complex_t *wbf_out,int idx,int is_end);
void wtk_bfio_on_gdenoise(wtk_bfio_t *bfio, wtk_complex_t *gd_out, int is_end);
void wtk_bfio_on_wake(wtk_bfio_wake_t *wake,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len);
void wtk_bfio_on_wake2(wtk_bfio_t *bfio,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len);

static void wtk_bfio_on_qform(wtk_bfio_t *bfio, wtk_complex_t *qf_out,
                              int is_end);
void wtk_bfio_on_vad(wtk_bfio_t *bfio,wtk_vad2_cmd_t cmd,short *data,int len);

void wtk_bfio_on_ssl2(wtk_bfio_t *bfio, wtk_ssl2_extp_t *nbest_extp, int nbest, int ts, int te);

wtk_bfio_wake_t *wtk_bfio_wake_new(wtk_bfio_t *bfio, wtk_kvadwake_cfg_t *cfg)
{
    wtk_bfio_wake_t *wake;

    wake=(wtk_bfio_wake_t *)wtk_malloc(sizeof(wtk_bfio_wake_t));
    wake->hook=bfio;

    wake->vwake=wtk_kvadwake_new(cfg);
    wtk_kvadwake_set_notify(wake->vwake, wake, (wtk_kvadwake_notify_f)wtk_bfio_on_wake);
    wtk_kvadwake_set_notify2(wake->vwake, bfio, (wtk_kvadwake_notify_f2)wtk_bfio_on_wake2);

    return wake;
}

void wtk_bfio_wake_reset(wtk_bfio_wake_t *wake)
{
    wake->waked=0;
    wake->wake_fs=0;
    wake->wake_fe=0;

    wtk_kvadwake_reset(wake->vwake);
}

void wtk_bfio_wake_start(wtk_bfio_wake_t *wake, int theta)
{
    wake->theta=theta;
    wtk_kvadwake_start(wake->vwake);
}

void wtk_bfio_wake_delete(wtk_bfio_wake_t *wake)
{
    wtk_kvadwake_delete(wake->vwake);
    wtk_free(wake);
}

wtk_bfio_t* wtk_bfio_new(wtk_bfio_cfg_t *cfg)
{
    wtk_bfio_t *bfio;
    int i;
    int wbf_cnt;

    bfio=(wtk_bfio_t *)wtk_malloc(sizeof(wtk_bfio_t));
    bfio->cfg=cfg;
    bfio->ths=NULL;
    bfio->notify=NULL;
    bfio->ssl2_ths=NULL;
    bfio->notify_ssl2=NULL;

    bfio->channel=cfg->stft2.channel;
    bfio->sp_channel=cfg->sp_stft2.channel;

    wtk_queue_init(&(bfio->mspstft_q));
    wtk_queue_init(&(bfio->stft2_q));

    bfio->sp_stft2=NULL;
    bfio->stft2=NULL;
    bfio->aec=NULL;
    if(cfg->use_aec)
    {
        bfio->sp_stft2=wtk_stft2_new(&(cfg->sp_stft2));
        wtk_stft2_set_notify(bfio->sp_stft2,bfio,(wtk_stft2_notify_f)wtk_bfio_on_sp_stft2);

        bfio->stft2=wtk_stft2_new(&(cfg->stft2));
        wtk_stft2_set_notify(bfio->stft2,bfio,(wtk_stft2_notify_f)wtk_bfio_on_stft2);
        bfio->aec=wtk_aec_new2(&(cfg->aec), bfio->stft2);
        wtk_aec_set_notify2(bfio->aec, bfio, (wtk_aec_notify_f2)wtk_bfio_on_aec);
    }else
    {
        bfio->stft2=wtk_stft2_new(&(cfg->stft2));
        wtk_stft2_set_notify(bfio->stft2,bfio,(wtk_stft2_notify_f)wtk_bfio_on_aec);
    }
    
    bfio->wbf=NULL;
    bfio->wbf2=NULL;

    if(cfg->use_wbf2)
    {
        bfio->wbf2=wtk_wbf2_new2(&(cfg->wbf2), bfio->stft2);
        wtk_wbf2_set_notify2(bfio->wbf2, bfio, (wtk_wbf2_notify_f2)wtk_bfio_on_wbf);
    }else
    {
        bfio->wbf=wtk_wbf_new2(&(cfg->wbf), bfio->stft2);    
        wtk_wbf_set_notify2(bfio->wbf, bfio, (wtk_wbf_notify_f2)wtk_bfio_on_wbf);
    }

    bfio->gdenoise = NULL;
    if (cfg->use_gdenoise) {
        bfio->gdenoise = wtk_gainnet_denoise_new2(&(cfg->gdenoise));
        wtk_gainnet_denoise_set_notify2(
            bfio->gdenoise, bfio,
            (wtk_gainnet_denoise_notify_f2)wtk_bfio_on_gdenoise);
    }

    wbf_cnt=bfio->wbf? bfio->wbf->cfg->wbf_cnt:bfio->wbf2->cfg->wbf2_cnt;
    bfio->wake_cnt = wbf_cnt;
    if (cfg->use_raw_audio) {
        if(cfg->use_all_raw_audio){
            bfio->wake_cnt += bfio->channel;
        }else{
            bfio->wake_cnt += 1;
        }
    }
    if (cfg->use_gdenoise) {
        bfio->wake_cnt += 1;
    }
    bfio->wake=NULL;
    if(cfg->use_kvadwake){
        bfio->wake = (wtk_bfio_wake_t **)wtk_malloc(sizeof(wtk_bfio_wake_t *) *
                                                    bfio->wake_cnt);
        for (i = 0; i < bfio->wake_cnt; ++i) {
            bfio->wake[i]=wtk_bfio_wake_new(bfio, &(cfg->vwake));
        }
        for (i = 0; i < bfio->wake_cnt; ++i) {
            if(cfg->use_gdenoise && i == bfio->wake_cnt-1){
                wtk_kvadwake_set_idx(bfio->wake[i]->vwake, 1); // 1表示是模型降噪
            }else{
                wtk_kvadwake_set_idx(bfio->wake[i]->vwake, 0); // 0表示非模型降噪
            }
        }
    }

    bfio->wake2=NULL;
    if(cfg->use_kvadwake2){
        bfio->wake2 = (wtk_bfio_wake_t **)wtk_malloc(sizeof(wtk_bfio_wake_t *) *
                                                    bfio->wake_cnt);
        for (i = 0; i < bfio->wake_cnt; ++i) {
            bfio->wake2[i]=wtk_bfio_wake_new(bfio, &(cfg->vwake2));
        }
        for (i = 0; i < bfio->wake_cnt; ++i) {
            if(bfio->cfg->use_gdenoise && i == bfio->wake_cnt-1){
                wtk_kvadwake_set_idx(bfio->wake2[i]->vwake, 1); // 1表示是模型降噪
            }else{
                wtk_kvadwake_set_idx(bfio->wake2[i]->vwake, 0); // 0表示非模型降噪
            }
        }
    }
    bfio->wake3=NULL;
    bfio->aec_wake_buf=NULL;
    if(cfg->use_aec_wake){
        bfio->wake3=wtk_bfio_wake_new(bfio, &(cfg->vwake3));
        wtk_kvadwake_set_idx(bfio->wake3->vwake, 0);
        bfio->aec_wake_buf=wtk_strbuf_new(1024, 1);
    }


    bfio->saec_out=NULL;
     bfio->saec_pad=NULL;
    if(cfg->use_raw_audio)
    {
        if(cfg->use_all_raw_audio){
            bfio->saec_out=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *)*bfio->channel);
            bfio->saec_pad=(float **)wtk_malloc(sizeof(float *)*bfio->channel);
            for(i=0;i<bfio->channel;++i){
                bfio->saec_out[i]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*bfio->stft2->nbin);
                bfio->saec_pad[i]=(float *)wtk_malloc(sizeof(float)*bfio->stft2->cfg->win);
            }
        }else{
            bfio->saec_out=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *));
            bfio->saec_pad=(float **)wtk_malloc(sizeof(float *));
            bfio->saec_out[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*bfio->stft2->nbin);
            bfio->saec_pad[0]=(float *)wtk_malloc(sizeof(float)*bfio->stft2->cfg->win);
        }
    }
    bfio->ssl=wtk_ssl_new2(&(cfg->ssl), bfio->stft2);
    bfio->ssl2=NULL;
    if(cfg->use_ssl2){
        bfio->ssl2=wtk_ssl2_new(&(cfg->ssl2));
        wtk_ssl2_set_notify(bfio->ssl2, bfio, (wtk_ssl2_notify_f)wtk_bfio_on_ssl2);
    }

    bfio->qform=NULL;
    bfio->qform2=NULL;
    bfio->qform3=NULL;
    if(cfg->use_qform2)
    {
        bfio->qform2=wtk_qform2_new2(&(cfg->qform2),bfio->stft2);
    }else if(cfg->use_qform3){
        bfio->qform3=wtk_qform3_new2(&(cfg->qform3),bfio->stft2);
    }else
    {
        bfio->qform=wtk_qform9_new2(&(cfg->qform9),bfio->stft2);
        wtk_qform9_set_notify2(bfio->qform, bfio, (wtk_qform9_notify_f2)wtk_bfio_on_qform);   
    }

    bfio->input=NULL;
    if(cfg->use_preemph)
    {
        bfio->memX=(float *)wtk_malloc(sizeof(float)*(bfio->wake_cnt+1));
        if(cfg->use_aec)
        {
            bfio->input=wtk_strbufs_new(bfio->channel+bfio->sp_channel);
            bfio->notch_mem=wtk_float_new_p2(bfio->channel+bfio->sp_channel,2);
            bfio->memD=(float *)wtk_malloc(sizeof(float)*(bfio->channel+bfio->sp_channel));
        }else
        {
            bfio->input=wtk_strbufs_new(bfio->channel);
            bfio->notch_mem=wtk_float_new_p2(bfio->channel,2);
            bfio->memD=(float *)wtk_malloc(sizeof(float)*(bfio->channel));
        }
    }

    bfio->vad=NULL;
    if(cfg->use_asr && cfg->use_vad)
    {
        bfio->vad=wtk_vad2_new(&(cfg->vad));
        wtk_vad2_set_notify(bfio->vad, bfio, (wtk_vad2_notify_f)wtk_bfio_on_vad);
        bfio->vad->use_vad_start=cfg->use_vad_start;
    }
    
    bfio->decoder = cfg->use_offline_asr ? qtk_decoder_wrapper_new(&cfg->decoder) : NULL;

    bfio->de_wake_buf=NULL;
    if(bfio->cfg->use_en_trick){
        if(!cfg->use_kvadwake){
            wtk_debug("error: use_en_trick must use_kvadwake");
            return NULL;
        }
        bfio->de_wake_buf = wtk_strbufs_new(bfio->wake_cnt);
    }

    wtk_bfio_reset(bfio);

    return bfio;
}

void wtk_bfio_delete(wtk_bfio_t *bfio)
{
    int i;

    if(bfio->wake){
        for (i = 0; i < bfio->wake_cnt; ++i) {
            wtk_bfio_wake_delete(bfio->wake[i]);
        }
        wtk_free(bfio->wake);
    }
    if(bfio->wake2){
        for (i = 0; i < bfio->wake_cnt; ++i) {
            wtk_bfio_wake_delete(bfio->wake2[i]);
        }
        wtk_free(bfio->wake2);
    }
    if(bfio->wake3){
        wtk_bfio_wake_delete(bfio->wake3);
    }
    if(bfio->aec_wake_buf){
        wtk_strbuf_delete(bfio->aec_wake_buf);
    }

    if(bfio->wbf)
    {
        wtk_wbf_delete2(bfio->wbf);
    }else
    {
        wtk_wbf2_delete2(bfio->wbf2);   
    }
    if (bfio->gdenoise) {
        wtk_gainnet_denoise_delete2(bfio->gdenoise);
    }
    if(bfio->saec_pad)
    {
        if(bfio->cfg->use_all_raw_audio){
            for(i=0;i<bfio->channel;++i){
                wtk_free(bfio->saec_pad[i]);
                wtk_free(bfio->saec_out[i]);
            }
        }else{
            wtk_free(bfio->saec_pad[0]);
            wtk_free(bfio->saec_out[0]);
        }
        wtk_free(bfio->saec_pad);
        wtk_free(bfio->saec_out);
    }
    wtk_ssl_delete2(bfio->ssl);
    if(bfio->ssl2){
        wtk_ssl2_delete(bfio->ssl2);
    }
    if(bfio->qform)
    {
        wtk_qform9_delete2(bfio->qform);
    }else if(bfio->qform2)
    {
        wtk_qform2_delete2(bfio->qform2);
    }else if(bfio->qform3){
        wtk_qform3_delete2(bfio->qform3);
    }

    if(bfio->vad)
    {
        wtk_vad2_delete(bfio->vad);
    }

    if(bfio->aec)
    {
        wtk_aec_delete2(bfio->aec);
        wtk_stft2_delete(bfio->sp_stft2);
    }
    wtk_stft2_delete(bfio->stft2);

    if(bfio->input)
    {
        if(bfio->cfg->use_aec)
        {
            wtk_strbufs_delete(bfio->input, bfio->channel+bfio->sp_channel);
            wtk_float_delete_p2(bfio->notch_mem,  bfio->channel+bfio->sp_channel);
        }else
        {
            wtk_strbufs_delete(bfio->input, bfio->channel);
            wtk_float_delete_p2(bfio->notch_mem,  bfio->channel);
        }
        wtk_free(bfio->memD);
        wtk_free(bfio->memX);
    }

    if (bfio->decoder) {
        qtk_decoder_wrapper_delete(bfio->decoder);
    }

    if(bfio->de_wake_buf){
        wtk_strbufs_delete(bfio->de_wake_buf, bfio->wake_cnt);
    }

    wtk_free(bfio);
}

void wtk_bfio_reset(wtk_bfio_t *bfio)
{
    int i;
    wtk_queue_t *stft2_q=&(bfio->stft2_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *msg2;
    bfio->asr = 0;
    if(bfio->input)
    {
        memset(bfio->memX,0,(bfio->wake_cnt+1)*sizeof(float));
        if(bfio->aec)
        {
            for(i=0;i<bfio->channel+bfio->sp_channel;++i)
            {
                memset(bfio->notch_mem[i],0,2*sizeof(float));
            }
            memset(bfio->memD,0,(bfio->channel+bfio->sp_channel)*sizeof(float));

            if(bfio->input)
            {
                wtk_strbufs_reset(bfio->input,  bfio->channel+bfio->sp_channel);
            }
        }else
        {
            for(i=0;i<bfio->channel;++i)
            {
                memset(bfio->notch_mem[i],0,2*sizeof(float));
            }
            memset(bfio->memD,0,bfio->channel*sizeof(float));

            if(bfio->input)
            {
                wtk_strbufs_reset(bfio->input,  bfio->channel);
            }
        }   
    }

    wtk_queue_init(&(bfio->mspstft_q));

    if(bfio->aec)
    {
        wtk_aec_reset2(bfio->aec);
        wtk_stft2_reset(bfio->sp_stft2);
    }

    while(stft2_q->length>0)
    {
        qn=wtk_queue_pop(stft2_q);
        msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
        wtk_stft2_push_msg(bfio->stft2, msg2);
    }
    wtk_queue_init(stft2_q);
    wtk_stft2_reset(bfio->stft2);

    bfio->end_pos=0;
    bfio->is_end=0;

    bfio->waked=0;
    bfio->wake_theta=0;
    bfio->wake_fs=bfio->wake_fe=-1;
    bfio->wake_extend=0;
    bfio->wake2_extend=0;
    bfio->wake_prob=0;
    bfio->wake2_prob=0;
    bfio->out_wake_prob=0;
    bfio->wake_key=0;
    bfio->sil_ts=0;
    bfio->speech_ts=0;
    bfio->sil_end=0;

    bfio->vad_output=0;
    bfio->vad_ts=0;
    bfio->vad_te=0;

    if(bfio->wake){
        for (i = 0; i < bfio->wake_cnt; ++i) {
            wtk_bfio_wake_reset(bfio->wake[i]);
        }
    }
    if(bfio->wake2){
        for (i = 0; i < bfio->wake_cnt; ++i) {
            wtk_bfio_wake_reset(bfio->wake2[i]);
        }
    }
    if(bfio->wake3){
        wtk_bfio_wake_reset(bfio->wake3);
    }
    if(bfio->aec_wake_buf){
        wtk_strbuf_reset(bfio->aec_wake_buf);
    }

    if(bfio->wbf)
    {
        wtk_wbf_reset2(bfio->wbf);
    }else
    {
        wtk_wbf2_reset2(bfio->wbf2);
    }
    if (bfio->gdenoise) {
        wtk_gainnet_denoise_reset2(bfio->gdenoise);
    }
    if(bfio->saec_pad)
    {
        if(bfio->cfg->use_all_raw_audio){
            for(i=0;i<bfio->channel;++i){
                memset(bfio->saec_out[i], 0, sizeof(wtk_complex_t)*bfio->stft2->nbin);
                memset(bfio->saec_pad[i], 0, sizeof(float)*bfio->stft2->cfg->win);
            }
        }else{
            memset(bfio->saec_out[0], 0, sizeof(wtk_complex_t)*bfio->stft2->nbin);
            memset(bfio->saec_pad[0], 0, sizeof(float)*bfio->stft2->cfg->win);
        }
    }
    wtk_ssl_reset2(bfio->ssl);
    if(bfio->ssl2){
        wtk_ssl2_reset(bfio->ssl2);
    }
    if(bfio->qform)
    {
        wtk_qform9_reset2(bfio->qform);
    }else if(bfio->qform2)
    {
        wtk_qform2_reset2(bfio->qform2);
    }else if(bfio->qform3){
        wtk_qform3_reset2(bfio->qform3);
    }
    
    if(bfio->vad)
    {
        wtk_vad2_reset(bfio->vad);
    }
    
    if (bfio->decoder) {
        qtk_decoder_wrapper_reset(bfio->decoder);
    }

    if(bfio->de_wake_buf){
        wtk_strbufs_reset(bfio->de_wake_buf, bfio->wake_cnt);
    }
    bfio->de_fs=0;
    bfio->de_fe=0;
    bfio->aec_fs=0;
    bfio->aec_fe=0;

    bfio->reg_bf=0;
    bfio->reg_theta=-1;
    bfio->reg_tms=-1;
    bfio->reg_end=0;
    bfio->wake2_ready=0;
    
    bfio->sum_low=0;
    bfio->low_cnt=0;
}

void wtk_bfio_set_notify(wtk_bfio_t *bfio,void *ths,wtk_bfio_notify_f notify)
{
    bfio->ths=ths;
    bfio->notify=notify;
}

void wtk_bfio_set_ssl2_notify(wtk_bfio_t *bfio, void *ths, wtk_bfio_notify_ssl2_f notify)
{
    bfio->ssl2_ths=ths;
    bfio->notify_ssl2=notify;
}

void wtk_bfio_on_ssl2(wtk_bfio_t *bfio, wtk_ssl2_extp_t *nbest_extp, int nbest, int ts, int te)
{
    if(bfio->notify_ssl2){
        bfio->notify_ssl2(bfio->ssl2_ths, nbest_extp, nbest);
    }
}

void wtk_bfio_start(wtk_bfio_t *bfio)
{
    int i, wbf_cnt;
    int *theta;

    if (bfio->wbf) {
        wbf_cnt = bfio->wbf->cfg->wbf_cnt;
        theta = bfio->wbf->theta;
    } else {
        wbf_cnt = bfio->wbf2->cfg->wbf2_cnt;
        theta = bfio->wbf2->cfg->theta;
    }

    if(bfio->wbf)
    {
        wtk_wbf_start2(bfio->wbf);
    }else
    {
        wtk_wbf2_start(bfio->wbf2);
    }
    if(bfio->wake){
        for(i=0; i<wbf_cnt; ++i)
        {
            wtk_bfio_wake_start(bfio->wake[i], theta[i]);
        }
        if (bfio->cfg->use_raw_audio) {
            if(bfio->cfg->use_all_raw_audio){
                for(i=0; i<bfio->channel; ++i){
                    wtk_bfio_wake_start(bfio->wake[i+wbf_cnt], -1);
                }
            }else{
                wtk_bfio_wake_start(bfio->wake[wbf_cnt], -1);
            }
        }
    }
    if(bfio->wake2){
        for(i=0; i<wbf_cnt; ++i)
        {
            wtk_bfio_wake_start(bfio->wake2[i], theta[i]);
        }
        if (bfio->cfg->use_raw_audio) {
            if(bfio->cfg->use_all_raw_audio){
                for(i=0; i<bfio->channel; ++i){
                    wtk_bfio_wake_start(bfio->wake2[i+wbf_cnt], -1);
                }
            }else{
                wtk_bfio_wake_start(bfio->wake2[wbf_cnt], -1);
            }
        }
    }
    if(bfio->wake3){
        wtk_bfio_wake_start(bfio->wake3, -1);
    }
    if(bfio->qform)
    {
        wtk_qform9_start2(bfio->qform, 0, 0);
    }else if(bfio->qform2)
    {
        wtk_qform2_start2(bfio->qform2, 0, 0);
    }else if(bfio->qform3){
        wtk_qform3_start2(bfio->qform3, 0, 0);
    }
    
}

void wtk_bfio_set_waked(wtk_bfio_t *bfio, float wake_fs, float wake_fe, int bftheta, float wake_prob, int need_notify, int need_ssl, wtk_bfio_wake_t *wake, int is_end, int is_wake2)
{
    float ns, ne;
    int rate=bfio->cfg->rate;
    // int i, 
    int cancel;
    int min_idx;
    // float min_thsub, fp;
    wtk_queue_t *stft2_q=&(bfio->stft2_q);
    wtk_stft2_msg_t *msg;
    wtk_queue_node_t *qn;
    // int theta_range=bfio->wbf?bfio->wbf->theta_range:bfio->wbf2->theta_range;


    if(is_wake2){
        bfio->wake_fs=wake_fs+bfio->wake2_extend;
        bfio->wake_fe=wake_fe+bfio->wake2_extend;
        bfio->wake2_prob=wake_prob;
        bfio->wake_key=1;
    }else{
        bfio->wake_fs=wake_fs+bfio->wake_extend;
        bfio->wake_fe=wake_fe+bfio->wake_extend;
        bfio->wake_prob=wake_prob;
        bfio->wake_key=0;
    }
    bfio->out_wake_prob=wake_prob;

    if(need_notify && bfio->notify)
    {
        if(bfio->cfg->use_asr && !bfio->cfg->use_vad){
            bfio->notify(bfio->ths, WTK_BFIO_VAD_END, NULL, 0);
        }
        bfio->notify(bfio->ths, WTK_BFIO_WAKE, NULL ,0);
        if(bfio->wake_key==0){
            bfio->wake2_ready=1;
        }
        if(bfio->cfg->use_asr && !bfio->cfg->use_vad){
            bfio->notify(bfio->ths, WTK_BFIO_VAD_START, NULL, 0);
        }
    }

    ns=(bfio->wake_fs+bfio->cfg->wake_ssl_fs)*rate;
    ne=(bfio->wake_fe+bfio->cfg->wake_ssl_fe)*rate;
    // wtk_debug("%f %f\n",bfio->cfg->wake_ssl_fs,bfio->cfg->wake_ssl_fe);
    // wtk_debug("%f %f\n", ns/rate, ne/rate);
    msg=(wtk_stft2_msg_t *)data_offset2(stft2_q->pop, wtk_stft2_msg_t, q_n);
    if (bfio->cfg->debug) {
        wtk_debug("smsg_s =%.0f wake_ssl_ns=%.0f wake_ssl_ne=%.0f [%f %f]\n",
                  msg->s, ns, ne, wake_fs + bfio->cfg->wake_ssl_fs,
                  wake_fe + bfio->cfg->wake_ssl_fe);
    }
    
    if(need_ssl)
    {
        wtk_ssl_reset2(bfio->ssl);
        for(qn=stft2_q->pop; qn; qn=qn->next)
        {
            msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
            if(msg->s >= ns)
            {
                // printf("%f %f %f\n",msg->s,ns,ne);
                wtk_ssl_feed_stft2msg(bfio->ssl, msg, 0);
            }
            if(msg->s >= ne)
            {
                break;
            }
        }
        wtk_ssl_feed_stft2msg(bfio->ssl, NULL, 1);
        // wtk_ssl_print(bfio->ssl);
    }
    min_idx=-1;

    if (bfio->ssl->nbest >= 1) {
        min_idx = 0;
    }

    if (bfio->cfg->use_asr && need_ssl) {
        if(bfio->cfg->use_vad){
            if (bfio->waked == 1) {
                wtk_vad2_feed(bfio->vad, NULL, 0, 1);
                wtk_vad2_reset(bfio->vad);
            }
            wtk_vad_set_margin(bfio->vad->vad, bfio->cfg->vad_left_margin,
                            bfio->cfg->vad_right_margin);
            wtk_vad2_start(bfio->vad);
        }
        bfio->waked = 1;
    }

    if(min_idx>=0)
    {
        if (abs(bfio->wake_theta - bfio->ssl->nbest_extp[min_idx].theta) <=
            bfio->cfg->ressl_range) {  // 与上次定位角度小于等于15度不更新qform
            bfio->wake_theta=bfio->ssl->nbest_extp[min_idx].theta;
            if (bfio->cfg->debug) {
                wtk_debug("wake_theta=bfio_theta=%d started\n",
                          bfio->wake_theta);
            }
            bfio->waked=1;
            if(need_notify && bfio->notify)
            {
            	if(wake->vwake->asr_res)
            	{
            		//printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                    bfio->notify(bfio->ths, WTK_BFIO_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos>>1);
            	}
                bfio->notify(bfio->ths, WTK_BFIO_WAKE_SSL, NULL ,0);
            }

            if (bfio->cfg->use_asr) {
                if (bfio->qform) {
                    wtk_qform9_feed_smsg(bfio->qform, NULL, 0, 1);
                } else if(bfio->qform2){
                    wtk_qform2_feed_smsg(bfio->qform2, NULL, 0, 1);
                    wtk_bfio_on_qform(bfio, NULL, 1);
                }else if(bfio->qform3){
                    wtk_qform3_feed_smsg(bfio->qform3, NULL, 0, 1);
                    wtk_bfio_on_qform(bfio, NULL, 1);
                }
            }
            need_ssl = 0;
        } else {
            bfio->wake_theta=bfio->ssl->nbest_extp[min_idx].theta;
            if (bfio->cfg->debug) {
                wtk_debug("wake_theta=%d\n", bfio->wake_theta);
            }
            if(need_notify && bfio->notify)
            {
            	if(wake->vwake->asr_res)
            	{
            		//printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                    bfio->notify(bfio->ths, WTK_BFIO_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos>>1);
            	}
                bfio->notify(bfio->ths, WTK_BFIO_WAKE_SSL, NULL ,0);
            }

            if(bfio->cfg->use_asr)
            {
                if (bfio->qform) {
                    wtk_qform9_feed_smsg(bfio->qform,NULL,0,1);
                    wtk_qform9_reset2(bfio->qform);
                } else if(bfio->qform2){
                    wtk_qform2_feed_smsg(bfio->qform2,NULL,0,1);
                    wtk_bfio_on_qform(bfio, NULL, 1);
                    wtk_qform2_reset2(bfio->qform2);
                }else if(bfio->qform3){
                    wtk_qform3_feed_smsg(bfio->qform3,NULL,0,1);
                    wtk_bfio_on_qform(bfio, NULL, 1);
                    wtk_qform3_reset2(bfio->qform3);
                }

                if(bfio->qform)
                {
                    wtk_qform9_start2(bfio->qform,bfio->wake_theta,0);
                }else if(bfio->qform2)
                {
                    wtk_qform2_start2(bfio->qform2,bfio->wake_theta,0);
                }else if(bfio->qform3){
                    wtk_qform3_start2(bfio->qform3,bfio->wake_theta,0);
                }

                if (need_notify) {
                    // ne = wake_fe * rate;
                    for (qn = stft2_q->pop; qn; qn = qn->next) {
                        msg = (wtk_stft2_msg_t *)data_offset2(
                            qn, wtk_stft2_msg_t, q_n);
                        if (msg->s >= ne) {
                            break;
                        }
                    }

                    if (msg->s >= ne) {
                        cancel = bfio->vad_output - msg->s;
                        if (bfio->cfg->debug) {
                            wtk_debug("out %.0f, msg->s %.0f cancel %d\n",
                                      bfio->vad_output, msg->s, cancel);
                        }
                        if (cancel > 0) {
                            if (bfio->notify) {
                                bfio->notify(bfio->ths, WTK_BFIO_VAD_CANCEL,
                                             NULL, cancel);
                            }
                            bfio->vad_output -= cancel;
                        }

                    }
                }
            }
        }
    }else
    {
        if (bfio->cfg->debug) {
            wtk_debug("[%f %f] waked ssl nbest has no theta == bftheta\n",
                      wake_fs, wake_fe);
        }
    }

    if (bfio->cfg->use_asr && need_ssl && !is_end) {

        // ne = wake_fe * rate;
        for (qn = stft2_q->pop; qn; qn = qn->next) {
            msg = (wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
            if (msg->s >= ne) {
                break;
            }
        }

        if (msg->s >= ne) {
            cancel = bfio->vad_output - msg->s;
            if (bfio->cfg->debug) {
                wtk_debug("out %.0f, msg->s %.0f cancel %d\n", bfio->vad_output,
                          msg->s, cancel);
            }
            if (cancel > 0) {
                if (bfio->notify) {
                    bfio->notify(bfio->ths, WTK_BFIO_VAD_CANCEL, NULL, cancel);
                }
                bfio->vad_output -= cancel;
            }

            for (; qn; qn = qn->next) {
                msg = (wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                if (bfio->qform) {
                    wtk_qform9_feed_smsg(bfio->qform, msg, 0, 0);
                } else if(bfio->qform2){
                    wtk_qform2_feed_smsg(bfio->qform2, msg, 0, 0);
                    wtk_bfio_on_qform(bfio, bfio->qform2->out, 0);
                }else if(bfio->qform3){
                    wtk_qform3_feed_smsg(bfio->qform3, msg, 0, 0);
                    wtk_bfio_on_qform(bfio, bfio->qform3->out, 0);
                }
            }
        }
    }
}

static float wtk_bfio_de_energy(short *p,int n)
{
    float tmp;
	float f;
	int i;

	f=0;
	for(i=0;i<n;++i)
	{
        tmp=p[i]*1.0/32867.0;
		f+=tmp*tmp;
	}
	f/=n;

	return f;
}

float wtk_bfio_entropy(wtk_bfio_t *bfio, wtk_complex_t **fftx, int chn)
{
    int rate = bfio->cfg->rate;
    int wins = bfio->stft2->cfg->win;
    int i;
    int fx1 = (250*1.0*wins)/rate;
    int fx2 = (2000*1.0*wins)/rate;
    int km = floor(wins*1.0/8);
    float K = 0.5;
    float E[1024]={0};
    float P1;
    float Eb[1024]={0};
    float sum;
    float prob;
    float Hb;

    for(i=fx1;i<fx2;++i)
    {
        E[i] = fftx[i][chn].a * fftx[i][chn].a + fftx[i][chn].b * fftx[i][chn].b;
    }
    sum = 1e-10;
    for(i=fx1;i<fx2;++i)
    {
        sum += E[i];
    }
    for(i=fx1;i<fx2;++i)
    {
        P1 = E[i]/sum;
        if(P1>=0.9){
            E[i] = 0;
        }
    }
    sum = 0;
    for(i=0;i<km;++i)
    {
        Eb[i] = K;
        Eb[i] += E[i*4]+E[i*4+1]+E[i*4+2]+E[i*4+3];
        sum += Eb[i];
    }
    Hb = 0;
    for(i=0;i<wins;++i)
    {
        prob = Eb[i]/sum;
        Hb += -prob*logf(prob+1e-10);
    }
    // printf("%f\n", Hb);

    return Hb;
}

int wtk_bfio_feed_low_trick(wtk_bfio_t *bfio, wtk_stft2_msg_t *msg, int chn, int is_end)
{
    int low_fs_idx=bfio->cfg->low_fs_idx;
    int low_fe_idx=bfio->cfg->low_fe_idx;
    float low_thresh=bfio->cfg->low_thresh;
    int i;
    int nbin=bfio->stft2->nbin;
    float low=0;

    if(is_end){
        low = bfio->sum_low/bfio->low_cnt;
        bfio->sum_low=0;
        bfio->low_cnt=0;
        // printf("%f\n", low);
        if(low>low_thresh){
            return 1;
        }
        return 0;
    }
    if(msg){
        wtk_complex_t **fft=msg->fft;
        float entropy=wtk_bfio_entropy(bfio, fft, chn);
        for(i=0;i<nbin;++i){
            if(i>=low_fs_idx && i<=low_fe_idx){
                low+=fft[i][chn].a * fft[i][chn].a + fft[i][chn].b * fft[i][chn].b;
            }
        }
        if(entropy<3){
            bfio->sum_low+=low;
            ++bfio->low_cnt;
            // printf("%f\n", low);
        }
    }
    return 0;
}
static void _bfio_waked_post(wtk_bfio_t *bfio, int is_end) {
    float max_wake_prob = -50;
    int max_idx = -1;
    wtk_bfio_wake_t **wake = bfio->wake;
    wtk_bfio_wake_t **wake2 = bfio->wake2;
    float wake_prob = bfio->wake_prob;
    int is_wake2=0;
    int i;
    float dup_time;
    int offset;
    int energy_len;
    float energy=0;
    float energy_conf=bfio->cfg->energy_conf;
    short *de;

    if(bfio->cfg->use_kvadwake){
        for (i = 0; i < bfio->wake_cnt; ++i) {
            if (wake[i]->waked == 1) {
                if (bfio->cfg->debug) {
                    wtk_debug("wake[%d] bftheta=%d wake_prob=%f wake_ts=[%f %f]\n",
                            i, wake[i]->theta, wake[i]->wake_prob,
                            wake[i]->wake_fs, wake[i]->wake_fe);
                }
                if(bfio->cfg->use_en_trick){
                    offset=max(0,floor(wake[i]->wake_fs*16000-bfio->de_fs));
                    // printf("%d %d %d\n", offset, energy_len, bfio->de_wake_buf[i]->pos);
                    energy_len=floor((wake[i]->wake_fe-wake[i]->wake_fs)*16000);
                    de=(short *)bfio->de_wake_buf[i]->data;
                    if(bfio->de_wake_buf[i]->pos>0){
                        energy=wtk_bfio_de_energy(de+offset, max(min(energy_len, bfio->de_wake_buf[i]->pos/2-offset), 0));
                        // printf("idx=%d energy=%.12f %f %f conf=%f\n", i, energy, wake[i]->wake_fs, wake[i]->wake_fe, energy_conf);
                        if(energy<energy_conf){
                            continue;
                        }
                    }
                }
                if(bfio->cfg->use_low_trick){
                    wtk_queue_t *stft2_q=&(bfio->stft2_q);
                    wtk_stft2_msg_t *msg;
                    wtk_queue_node_t *qn;
                    int rate=bfio->cfg->rate;
                    float ns;
                    float ne;
                    int ret=0;
                    // ns=(wake[i]->wake_fs+bfio->cfg->wake_ssl_fs)*rate;
                    // ne=(wake[i]->wake_fe+bfio->cfg->wake_ssl_fe)*rate;
                    ns=wake[i]->wake_fs*rate;
                    ne=wake[i]->wake_fe*rate;
                    for(qn=stft2_q->pop; qn; qn=qn->next)
                    {
                        msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                        if(msg->s >= ns)
                        {
                            // printf("%f %f %f\n",msg->s,ns,ne);
                            wtk_bfio_feed_low_trick(bfio, msg, i, 0);
                        }
                        if(msg->s >= ne)
                        {
                            break;
                        }
                    }
                    ret = wtk_bfio_feed_low_trick(bfio, NULL, i, 1);
                    // printf("%d\n", ret);
                    if(ret==0){
                        continue;
                    }
                }
                if (wake[i]->wake_prob > max_wake_prob) {
                    max_idx = i;
                    max_wake_prob = wake[i]->wake_prob;
                }
            }
        }
        if(bfio->cfg->use_aec_wake && max_idx > -1){
            short null_buf[16000]={0};
            // wtk_debug("%f %f\n", bfio->aec_fs, bfio->aec_fe);
            offset=max(0,floor((wake[max_idx]->wake_fs+bfio->cfg->aec_wake_fs)*16000-bfio->aec_fs));
            // printf("%d %d %d\n", offset, energy_len, bfio->de_wake_buf[i]->pos);
            energy_len=max(min(floor((wake[max_idx]->wake_fe-wake[max_idx]->wake_fs-bfio->cfg->aec_wake_fs+bfio->cfg->aec_wake_fe)*16000), bfio->aec_wake_buf->pos/2-offset), 0);
            de=(short *)bfio->aec_wake_buf->data;
            if(bfio->aec_wake_buf->pos>0){
                energy=wtk_bfio_de_energy(de+offset, energy_len);
                if(energy>energy_conf){
                    bfio->wake3->waked = 0;
                    wtk_kvadwake_start(bfio->wake3->vwake);
                    wtk_kvadwake_feed(bfio->wake3->vwake, null_buf, 16000, is_end);
                    wtk_kvadwake_feed(bfio->wake3->vwake, de+offset, energy_len, is_end);
                    wtk_kvadwake_feed(bfio->wake3->vwake, NULL, 0, 1);
                    wtk_kvadwake_reset(bfio->wake3->vwake);
                    if(bfio->wake3->waked){
                        max_idx = -1;
                        max_wake_prob = -50;
                    }
                }
            }
        }
    }

    if(bfio->cfg->use_kvadwake2){
        if(max_idx == -1){
            for (i = 0; i < bfio->wake_cnt; ++i) {
                if (wake2[i]->waked == 1) {
                    if (bfio->cfg->debug) {
                        wtk_debug("wake[%d] bftheta=%d wake_prob=%f wake_ts=[%f %f]\n",
                                i, wake2[i]->theta, wake2[i]->wake_prob,
                                wake2[i]->wake_fs, wake2[i]->wake_fe);
                    }
                    if (wake2[i]->wake_prob > max_wake_prob) {
                        max_idx = i;
                        max_wake_prob = wake2[i]->wake_prob;
                    }
                }
            }
            is_wake2 = 1;
        }
    }

    if (max_idx >= 0) {
        if (bfio->cfg->use_gdenoise && max_idx == bfio->wake_cnt - 1) {
            dup_time = 2.5;
        } else {
            dup_time = 1.2;
        }
        if(is_wake2){
            wake=bfio->wake2;
            wake_prob=bfio->wake2_prob;
        }
        if (fabs(wake[max_idx]->wake_fe - bfio->wake_fe) < dup_time ||
            fabs(wake[max_idx]->wake_fs - bfio->wake_fs) < dup_time) {
            if (wake[max_idx]->wake_prob > wake_prob) {
                if (bfio->cfg->debug) {
                    wtk_debug("change waked [%f %f] to [%f %f ], wake[%d] "
                              "bftheta=%d wake_prob=%f  lst_wakeprob=%f\n",
                              bfio->wake_fs, bfio->wake_fe,
                              wake[max_idx]->wake_fs, wake[max_idx]->wake_fe,
                              max_idx, wake[max_idx]->theta,
                              wake[max_idx]->wake_prob, wake_prob);
                }
                wtk_bfio_set_waked(bfio, wake[max_idx]->wake_fs,
                                   wake[max_idx]->wake_fe, wake[max_idx]->theta,
                                   wake[max_idx]->wake_prob, 0, 0,
                                   wake[max_idx], is_end, is_wake2);
            } else {
                if (bfio->cfg->debug) {
                    wtk_debug("waked over [%f %f] pop [%f %f]\n", bfio->wake_fs,
                              bfio->wake_fe, wake[max_idx]->wake_fs,
                              wake[max_idx]->wake_fe);
                }
            }
        } else {
            wtk_bfio_set_waked(bfio, wake[max_idx]->wake_fs,
                               wake[max_idx]->wake_fe, wake[max_idx]->theta,
                               wake[max_idx]->wake_prob, 1, 1, wake[max_idx], is_end, is_wake2);
        }
    }
}

void wtk_bfio_on_wbf(wtk_bfio_t *bfio,wtk_complex_t *wbf_out,int idx,int is_end)
{
    float *data=bfio->stft2->output;
    short *pv=NULL;
    int k=0;
    int i;

    if(wbf_out)
    {
        k=wtk_stft2_output_ifft(bfio->stft2,wbf_out,data,bfio->wbf?bfio->wbf->bf[idx]->pad:(bfio->wbf2->qform3?bfio->wbf2->qform3[idx]->pad:bfio->wbf2->qform2[idx]->pad),bfio->end_pos,bfio->is_end);
        if(bfio->cfg->use_preemph)
        {
            bfio->memX[idx]=wtk_preemph_asis2(data,k,bfio->memX[idx]);
        }

        pv=(short *)data;
        for(i=0;i<k;++i)
        {
            pv[i] = QTK_SSAT16f(data[i]);
        }
    }
#ifdef USE_LOG
	static wtk_wavfile_t *log = NULL;

    if(bfio->wake[idx]->theta==90)
	{
		if(!log)
		{
			log=wtk_wavfile_new(16000);
			wtk_wavfile_open2(log,"wbf");
			log->max_pend=0;
		}
        if(k>0)
        {
		    wtk_wavfile_write(log,(char *)pv,k<<1);
        }
		if(is_end)
		{
			wtk_debug("============ close ============\n");
			wtk_wavfile_close(log);
			wtk_wavfile_delete(log);
			log=NULL;
		}
	}
#endif

#ifdef USE_LOG_BFIO

    int wbf_cnt=bfio->wbf? bfio->wbf->cfg->wbf_cnt:bfio->wbf2->cfg->wbf2_cnt;

    int j;
	static wtk_wavfile_t *log[8];
    for(j=0;j < wbf_cnt;++j){
        if(idx==j)
        {
        	if(!log[j])
        	{
        		log[j]=wtk_wavfile_new(16000);
        		wtk_wavfile_open2(log[j],"wbf/wbf");
        		log[j]->max_pend=0;
        	}
            if(k>0)
            {
        	    wtk_wavfile_write(log[j],(char *)pv,k<<1);
            }
        	if(is_end)
        	{
        		wtk_debug("============ close ============\n");
        		wtk_wavfile_close(log[j]);
        		wtk_wavfile_delete(log[j]);
        		log[j]=NULL;
        	}
        }
    }

#endif

    if(bfio->cfg->use_kvadwake){
        if(bfio->cfg->use_en_trick){
            wtk_strbuf_t *de_wake_buf=bfio->de_wake_buf[idx];
            int de_len;
            int de_wake_len=bfio->cfg->de_wake_len;

            wtk_strbuf_push(de_wake_buf, (char *)pv, k*sizeof(short));

            de_len=de_wake_buf->pos/sizeof(short);
            
            if(de_len>de_wake_len){
                wtk_strbuf_pop(de_wake_buf, NULL, (de_len-de_wake_len)*sizeof(short));
            }
            if(idx==0){
                bfio->de_fe+=k;
                bfio->de_fs=max(0,bfio->de_fe-de_wake_len);
            }
            // wtk_debug("%f %f\n", bfio->de_fs, bfio->de_fe);
        }
        wtk_kvadwake_feed(bfio->wake[idx]->vwake, pv, k, is_end);
    }
    // if(bfio->cfg->use_kvadwake2){
    //     wtk_kvadwake_feed(bfio->wake2[idx]->vwake, pv, k, is_end);
    // }
}

void wtk_bfio_on_gdenoise(wtk_bfio_t *bfio, wtk_complex_t *gd_out, int is_end) {
    int wbf_cnt =
        bfio->wbf ? bfio->wbf->cfg->wbf_cnt : bfio->wbf2->cfg->wbf2_cnt;
    float *data = bfio->stft2->output;
    float *pad = bfio->gdenoise->pad;
    short *pv = NULL;
    int k = 0;
    int i;

    if (gd_out) {
        k = wtk_stft2_output_ifft(bfio->stft2, gd_out, data, pad, bfio->end_pos,
                                  bfio->is_end);
        if (bfio->cfg->use_preemph) {
            if(bfio->cfg->use_all_raw_audio){
                bfio->memX[wbf_cnt + bfio->channel] =
                    wtk_preemph_asis2(data, k, bfio->memX[wbf_cnt + bfio->channel]);
            }else{
                bfio->memX[wbf_cnt + 1] =
                    wtk_preemph_asis2(data, k, bfio->memX[wbf_cnt + 1]);
            }
        }

        pv = (short *)data;
        for (i = 0; i < k; ++i) {
            pv[i] = QTK_SSAT16f(data[i]);
        }
    }
#ifdef USE_LOG_BFIO
    static wtk_wavfile_t *log = NULL;
    if (!log) {
        log = wtk_wavfile_new(16000);
        wtk_wavfile_open2(log, "wbf/gdenoise");
        log->max_pend = 0;
    }
    if (k > 0) {
        wtk_wavfile_write(log, (char *)pv, k << 1);
    }
    if (is_end) {
        wtk_debug("============ close ============\n");
        wtk_wavfile_close(log);
        wtk_wavfile_delete(log);
        log = NULL;
    }
#endif
    if(bfio->cfg->use_all_raw_audio){
        if(bfio->cfg->use_kvadwake){
            wtk_kvadwake_feed(bfio->wake[wbf_cnt + bfio->channel]->vwake, pv, k, is_end);
        }
        if(bfio->cfg->use_kvadwake2){
            wtk_kvadwake_feed(bfio->wake2[wbf_cnt + bfio->channel]->vwake, pv, k, is_end);
        }
    }else{
        if(bfio->cfg->use_kvadwake){
            wtk_kvadwake_feed(bfio->wake[wbf_cnt + 1]->vwake, pv, k, is_end);
        }
        if(bfio->cfg->use_kvadwake2){
            wtk_kvadwake_feed(bfio->wake2[wbf_cnt + 1]->vwake, pv, k, is_end);
        }
    }
}

void wtk_bfio_on_wake(wtk_bfio_wake_t *wake,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len)
{
    if(cmd==WTK_KVADWAKE_WAKE)
    {
        wake->waked=1;
        wake->wake_fs=fs;
        wake->wake_fe=fe;
        wake->wake_prob=wtk_kvadwake_get_conf(wake->vwake);
    }
}

void wtk_bfio_on_wake2(wtk_bfio_t *bfio,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len)
{
    if(cmd==WTK_KVADWAKE_WAKE2){
        if(bfio->notify && bfio->wake2_ready){
            bfio->notify(bfio->ths, WTK_BFIO_WAKE2, NULL, 0);
            bfio->wake2_ready=0;
        }
    }
}

static void wtk_bfio_on_qform(wtk_bfio_t *bfio, wtk_complex_t *qf_out,
                              int is_end) {
    float *data=bfio->stft2->output;
    short *pv=NULL;
    int k=0;
    int i;
    float *pad = NULL;
    int ifft_is_end=bfio->is_end;

    if(bfio->qform){
        pad = bfio->qform->bf->pad;
        if(bfio->qform->cfg->use_qenvelope){
            ifft_is_end = is_end;
        }
    }else if(bfio->qform2){
        pad = bfio->qform2->pad;
    }else if(bfio->qform3){
        pad = bfio->qform3->pad;
    }

    if(qf_out)
    {
        k = wtk_stft2_output_ifft(bfio->stft2, qf_out, data, pad, bfio->end_pos,
                                  ifft_is_end);
        if(bfio->cfg->use_preemph)
        {
            bfio->memX[bfio->wake_cnt]=wtk_preemph_asis2(data,k,bfio->memX[bfio->wake_cnt]);
        }

        pv=(short *)data;
        for(i=0;i<k;++i)
        {
            pv[i] = QTK_SSAT16f(data[i]);
        }
    }
    if(bfio->reg_bf){
        if(bfio->notify)
        {
            bfio->notify(bfio->ths, WTK_BFIO_VAD_DATA, pv, k);
        }
        if(is_end){
            wtk_bfio_reset(bfio);
        }
    }else{
        if(bfio->waked==1){
            bfio->sil_end=0;
        }
        if(bfio->cfg->use_one_shot){
            bfio->sil_ts=bfio->vad_output/bfio->cfg->rate-bfio->wake_fe;
            if(bfio->sil_ts >= bfio->cfg->sil_delay*0.001 && bfio->waked==1 && bfio->vad_ts <= bfio->wake_fe){
                if(bfio->cfg->use_vad){
                    bfio->notify(bfio->ths, WTK_BFIO_SIL_END, NULL, bfio->vad_output);
                    // wtk_debug("sil_ts=[%f], vad_ts/te=[%f/%f] wake_fs/fe=[%f/%f] val=[%f]\n", bfio->sil_ts, bfio->vad_ts, bfio->vad_te, bfio->wake_fs, bfio->wake_fe, bfio->vad_output/bfio->cfg->rate);
                    bfio->waked=0;
                    bfio->sil_end=1;
                    wtk_vad2_feed(bfio->vad, NULL, 0, 1);
                    wtk_vad2_reset(bfio->vad);
                    wtk_vad2_start(bfio->vad);
                }
            }
            bfio->speech_ts=bfio->vad_output/bfio->cfg->rate-bfio->wake_fe;
            if(bfio->speech_ts >= bfio->cfg->speech_delay*0.001 && (bfio->speech_ts <= bfio->cfg->speech_delay*0.001 + k*1.0/bfio->cfg->rate) && bfio->sil_end==0){
                bfio->notify(bfio->ths, WTK_BFIO_SPEECH_END, NULL, bfio->vad_output);
                // wtk_debug("speech_ts=[%f], vad_ts/te=[%f/%f] wake_fs/fe=[%f/%f] val=[%f]\n", bfio->sil_ts, bfio->vad_ts, bfio->vad_te, bfio->wake_fs, bfio->wake_fe, bfio->vad_output/bfio->cfg->rate);
                if(bfio->waked==1){
                    if(bfio->cfg->use_vad){
                        bfio->waked=0;
                        wtk_vad2_feed(bfio->vad, NULL, 0, 1);
                        wtk_vad2_reset(bfio->vad);
                        wtk_vad2_start(bfio->vad);
                    }else{
                        if(bfio->notify){
                            bfio->notify(bfio->ths, WTK_BFIO_VAD_END, NULL, 0);
                        }
                    }
                }
                bfio->sil_end=1;
            }
        }

        if(bfio->waked==1 && !is_end && bfio->cfg->use_vad)
        {
            // wtk_debug("vad=[%f]  wake_fs/fe=[%f/%f]\n", bfio->vad_output/bfio->cfg->rate,  bfio->wake_fs, bfio->wake_fe);
            wtk_vad2_feed(bfio->vad, (char *)pv, k<<1, is_end);
        }else
        {
            bfio->vad_output+=k;
            if(bfio->notify)
            {
                bfio->notify(bfio->ths, WTK_BFIO_VAD_DATA, pv, k);
            }
        }
        if (is_end) {
            if(bfio->cfg->use_vad){
                wtk_vad2_feed(bfio->vad, NULL, 0, 1);
                wtk_vad2_reset(bfio->vad);
                wtk_vad2_start(bfio->vad);
            }
        }
    }
}

void wtk_bfio_on_vad(wtk_bfio_t *bfio,wtk_vad2_cmd_t cmd,short *data,int len)
{
    float pos=0;
    float ne;
    if (bfio->cfg->use_offline_asr) {
        wtk_string_t rec_res;
        switch(cmd) {
        case WTK_VAD2_START:
            bfio->vad_ts = bfio->vad_output / bfio->cfg->rate;
            qtk_decoder_wrapper_start(bfio->decoder);
            bfio->asr = 1;
            break;
        case WTK_VAD2_DATA:
            bfio->vad_output += len;
            if (bfio->asr == 1) {
                qtk_decoder_wrapper_feed(bfio->decoder, (char *)data, len << 1, 0);
            }
            if (bfio->notify) {
                bfio->notify(bfio->ths, WTK_BFIO_VAD_DATA, data, len);
            }
            break;
        case WTK_VAD2_END:
            bfio->vad_te = bfio->vad_output / bfio->cfg->rate;
            qtk_decoder_wrapper_feed(bfio->decoder, NULL, 0, 1);
            qtk_decoder_wrapper_get_result(bfio->decoder, &rec_res);
            if(bfio->notify) {
                bfio->notify(bfio->ths, WTK_BFIO_ASR_RES, (short *)(rec_res.data), rec_res.len);
            }
            qtk_decoder_wrapper_reset(bfio->decoder);
            bfio->asr = 0;
            break;
        case WTK_VAD2_CANCEL:
            bfio->vad_output -= len;
            break;
        }
        return ;
    }

	switch(cmd)
    {
    case WTK_VAD2_START:
        bfio->vad_ts=bfio->vad_output/bfio->cfg->rate;
        if(bfio->notify)
        {
            bfio->notify(bfio->ths, WTK_BFIO_VAD_START, NULL, 0);
        }
        // sil=0;
        break;
    case WTK_VAD2_DATA:
        bfio->vad_output+=len;
        // if(sil==0)
        // {
        pos = bfio->vad_output/bfio->cfg->rate - bfio->wake_fe - bfio->cfg->speech_delay*0.001;
        if(pos>0 && bfio->cfg->use_one_shot){  // 有语音时长强制截断
            // wtk_debug("vad=[%f]  wake_fs/fe=[%f/%f]\n", bfio->vad_output/bfio->cfg->rate,  bfio->wake_fs, bfio->wake_fe);
            // wtk_debug("===========pos=%f\n", pos/bfio->cfg->rate);
            // bfio->notify(bfio->ths, WTK_BFIO_SPEECH_END, NULL, len);
            if(bfio->notify)
            {
                bfio->notify(bfio->ths, WTK_BFIO_VAD_END, NULL, len);
            }
            // wtk_debug("%d\n", bfio->waked);
            bfio->waked = 0;
        }
        if (bfio->notify) {
            bfio->notify(bfio->ths, WTK_BFIO_VAD_DATA, data, len);
        }
        // }
        break;
    case WTK_VAD2_END:
        bfio->vad_te=bfio->vad_output/bfio->cfg->rate;
        // wtk_debug("vad_ts/te=[%f/%f] wake_fs/fe=[%f/%f] pos=%f\n", bfio->vad_ts, bfio->vad_te, bfio->wake_fs, bfio->wake_fe, pos);
        ne=(bfio->wake_fe+bfio->cfg->wake_ssl_fe);
        if(bfio->notify)
        {
            if(bfio->vad_ts <= ne){ // 删除唤醒词
                pos = (ne-bfio->vad_ts)*bfio->cfg->rate;
                // bfio->notify(bfio->ths, WTK_BFIO_ASR_CANCEL, NULL, pos);
            }
            bfio->notify(bfio->ths, WTK_BFIO_VAD_END, NULL, len);
            // wtk_debug("%d\n", bfio->waked);
        }
        // sil=1;
        break;
    case WTK_VAD2_CANCEL:
        bfio->vad_output-=len;
        if(bfio->notify)
        {
            bfio->notify(bfio->ths, WTK_BFIO_VAD_CANCEL, NULL, len);
        }
        break;
    }
}

void wtk_bfio_reg_bf(wtk_bfio_t *bfio, wtk_stft2_msg_t *msg, int pos, int is_end)
{
    wtk_queue_node_t *qn;
    wtk_queue_t *stft2_q=&(bfio->stft2_q);
    wtk_stft2_msg_t *msg2;
    wtk_stft2_msg_t *msg3;
    int max_theta=bfio->cfg->use_line?180:359;
    int theta=0;


    if(bfio->reg_theta >= 0 && bfio->reg_theta <= max_theta){
        if(is_end){
            if (bfio->qform) {
                wtk_qform9_feed_smsg(bfio->qform,NULL,0,1);
                wtk_qform9_reset2(bfio->qform);
            } else if(bfio->qform2){
                wtk_qform2_feed_smsg(bfio->qform2,NULL,0,1);
                wtk_bfio_on_qform(bfio, NULL, 1);
                wtk_qform2_reset2(bfio->qform2);
            }else if(bfio->qform3){
                wtk_qform3_feed_smsg(bfio->qform3,NULL,0,1);
                wtk_bfio_on_qform(bfio, NULL, 1);
                wtk_qform3_reset2(bfio->qform3);
            }
        }else{
            if(bfio->qform)
            {
                wtk_qform9_feed_smsg(bfio->qform, msg, pos, is_end);
            }else if(bfio->qform2)
            {
                wtk_qform2_feed_smsg(bfio->qform2, msg, pos ,is_end);
                wtk_bfio_on_qform(bfio, msg ? bfio->qform2->out : NULL, is_end);
            }else if(bfio->qform3){
                wtk_qform3_feed_smsg(bfio->qform3, msg, pos ,is_end);
                wtk_bfio_on_qform(bfio, msg ? bfio->qform3->out : NULL, is_end);
            }
        }
    }else if(bfio->reg_tms>0){
        if(is_end){
            if (bfio->qform) {
                wtk_qform9_feed_smsg(bfio->qform,NULL,0,1);
                wtk_qform9_reset2(bfio->qform);
            } else if(bfio->qform2){
                wtk_qform2_feed_smsg(bfio->qform2,NULL,0,1);
                wtk_bfio_on_qform(bfio, NULL, 1);
                wtk_qform2_reset2(bfio->qform2);
            }else if(bfio->qform3){
                wtk_qform3_feed_smsg(bfio->qform3,NULL,0,1);
                wtk_bfio_on_qform(bfio, NULL, 1);
                wtk_qform3_reset2(bfio->qform3);
            }
        }else{
            if(bfio->reg_end){
                if(bfio->qform)
                {
                    wtk_qform9_feed_smsg(bfio->qform, msg, pos, is_end);
                }else if(bfio->qform2)
                {
                    wtk_qform2_feed_smsg(bfio->qform2, msg, pos ,is_end);
                    wtk_bfio_on_qform(bfio, msg ? bfio->qform2->out : NULL, is_end);
                }else if(bfio->qform3){
                    wtk_qform3_feed_smsg(bfio->qform3, msg, pos ,is_end);
                    wtk_bfio_on_qform(bfio, msg ? bfio->qform3->out : NULL, is_end);
                }
            }else{
                if(msg)
                {
                    wtk_queue_push(stft2_q, &(msg->q_n));
                }
                if(msg->s>bfio->reg_tms){
                    wtk_ssl_feed_stft2msg(bfio->ssl, NULL, 1);
                    wtk_ssl_print(bfio->ssl);
                    theta=bfio->ssl->nbest_extp[0].theta;
                    // wtk_debug("==========%f %d\n", msg->s, bfio->reg_tms);
                    wtk_ssl_reset2(bfio->ssl);
                    bfio->reg_end=1;
                    if(bfio->qform)
                    {
                        wtk_qform9_start2(bfio->qform,theta,0);
                    }else if(bfio->qform2)
                    {
                        wtk_qform2_start2(bfio->qform2,theta,0);
                    }else if(bfio->qform3){
                        wtk_qform3_start2(bfio->qform3,theta,0);
                    }
                    for(qn=stft2_q->pop; qn; qn=qn->next)
                    {
                        msg3=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                        if(bfio->qform)
                        {
                            wtk_qform9_feed_smsg(bfio->qform, msg3, pos, 0);
                        }else if(bfio->qform2)
                        {
                            wtk_qform2_feed_smsg(bfio->qform2, msg3, pos ,0);
                            wtk_bfio_on_qform(bfio, bfio->qform2->out, 0);
                        }else if(bfio->qform3){
                            wtk_qform3_feed_smsg(bfio->qform3, msg3, pos ,0);
                            wtk_bfio_on_qform(bfio, bfio->qform3->out, 0);
                        }
                    }
                    
                    while(stft2_q->length>0)
                    {
                        qn=wtk_queue_pop(stft2_q);
                        msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                        wtk_stft2_push_msg(bfio->stft2, msg2);
                    }
                }else{
                    wtk_ssl_feed_stft2msg(bfio->ssl, msg, 0);
                }
            }
        }

    }else{
        if(is_end){
            wtk_ssl_feed_stft2msg(bfio->ssl, NULL, 1);
            wtk_ssl_print(bfio->ssl);
            theta=bfio->ssl->nbest_extp[0].theta;
            wtk_ssl_reset2(bfio->ssl);

            if(bfio->qform)
            {
                wtk_qform9_start2(bfio->qform,theta,0);
            }else if(bfio->qform2)
            {
                wtk_qform2_start2(bfio->qform2,theta,0);
            }else if(bfio->qform3){
                wtk_qform3_start2(bfio->qform3,theta,0);
            }
            
            for(qn=stft2_q->pop; qn; qn=qn->next)
            {
                msg3=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                if(bfio->qform)
                {
                    wtk_qform9_feed_smsg(bfio->qform, msg3, pos, 0);
                }else if(bfio->qform2)
                {
                    wtk_qform2_feed_smsg(bfio->qform2, msg3, pos ,0);
                    wtk_bfio_on_qform(bfio, bfio->qform2->out, 0);
                }else if(bfio->qform3){
                    wtk_qform3_feed_smsg(bfio->qform3, msg3, pos ,0);
                    wtk_bfio_on_qform(bfio, bfio->qform3->out, 0);
                }
            }
            
            while(stft2_q->length>0)
            {
                qn=wtk_queue_pop(stft2_q);
                msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                wtk_stft2_push_msg(bfio->stft2, msg2);
            }

            if (bfio->qform) {
                wtk_qform9_feed_smsg(bfio->qform,NULL,0,1);
                wtk_qform9_reset2(bfio->qform);
            } else if(bfio->qform2){
                wtk_qform2_feed_smsg(bfio->qform2,NULL,0,1);
                wtk_bfio_on_qform(bfio, NULL, 1);
                wtk_qform2_reset2(bfio->qform2);
            }else if(bfio->qform3){
                wtk_qform3_feed_smsg(bfio->qform3,NULL,0,1);
                wtk_bfio_on_qform(bfio, NULL, 1);
                wtk_qform3_reset2(bfio->qform3);
            }
        }else{
            if(msg)
            {
                wtk_queue_push(stft2_q, &(msg->q_n));
            }
            wtk_ssl_feed_stft2msg(bfio->ssl, msg, 0);
        }
    }
}

void wtk_bfio_on_aec(wtk_bfio_t *bfio,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int i, nbin=bfio->stft2->nbin;
    wtk_bfio_wake_t **wake=bfio->wake;
    wtk_bfio_wake_t **wake2=bfio->wake2;
    wtk_queue_t *stft2_q=&(bfio->stft2_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *msg2;
    int wbf_cnt=bfio->wbf? bfio->wbf->cfg->wbf_cnt:bfio->wbf2->cfg->wbf2_cnt;
    float *data=bfio->stft2->output;
    float **saec_pad=bfio->saec_pad;
    wtk_complex_t **saec_out=bfio->saec_out;
    short *pv=NULL;
    int k=0;
    int n;

    if(bfio->reg_bf){
        wtk_bfio_reg_bf(bfio, msg, pos, is_end);
    }else{
        if(is_end)
        {
            bfio->end_pos=pos;
            bfio->is_end=1;
        }

        if(msg)
        {
            wtk_queue_push(stft2_q, &(msg->q_n));
            if(stft2_q->length > bfio->cfg->stft2_hist)
            {
                qn=wtk_queue_pop(stft2_q);
                msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                wtk_stft2_push_msg(bfio->stft2, msg2);
            }
        }
        if(bfio->cfg->use_asr)
        {
            if(bfio->qform)
            {
                wtk_qform9_feed_smsg(bfio->qform, msg, pos, is_end);
            }else if(bfio->qform2)
            {
                wtk_qform2_feed_smsg(bfio->qform2, msg, pos ,is_end);
                wtk_bfio_on_qform(bfio, msg ? bfio->qform2->out : NULL, is_end);
            }else if(bfio->qform3){
                wtk_qform3_feed_smsg(bfio->qform3, msg, pos ,is_end);
                wtk_bfio_on_qform(bfio, msg ? bfio->qform3->out : NULL, is_end);
            }
        }
        if(bfio->ssl2){
            wtk_ssl2_feed_fft2(bfio->ssl2, msg->fft, is_end);
        }
        if(bfio->cfg->use_kvadwake){
            for(i=0; i<wbf_cnt; ++i)
            {
                wake[i]->waked=0;
            }
            if(bfio->cfg->use_aec){
                for (i = 0; i < bfio->wake_cnt; ++i) {
                    wtk_kvadwake_get_sp_sil(bfio->wake[i]->vwake, bfio->aec->sp_sil);
                }
            }
        }
        if(bfio->cfg->use_kvadwake2){
            for(i=0; i<wbf_cnt; ++i)
            {
                wake2[i]->waked=0;
            }
            if(bfio->cfg->use_aec){
                for (i = 0; i < bfio->wake_cnt; ++i) {
                    wtk_kvadwake_get_sp_sil(bfio->wake2[i]->vwake, bfio->aec->sp_sil);
                }
            }
        }
        if(bfio->wbf)
        {
            wtk_wbf_feed_smsg(bfio->wbf, msg, pos, is_end);
        }else
        {
            wtk_wbf2_feed_smsg(bfio->wbf2, msg, pos, is_end);
        }
        if (bfio->cfg->use_raw_audio)
        {
            if(bfio->cfg->use_all_raw_audio){
                for(n=0;n<bfio->channel;++n){
                    if(bfio->cfg->use_kvadwake){
                        wake[n+wbf_cnt]->waked=0;
                    }
                    if(bfio->cfg->use_kvadwake2){
                        wake2[n+wbf_cnt]->waked=0;
                    }
                    if (is_end) 
                    {
                        if(bfio->cfg->use_kvadwake){
                            wtk_kvadwake_feed(wake[n+wbf_cnt]->vwake, NULL, 0, 1);
                        }
                        if(bfio->cfg->use_kvadwake2){
                            wtk_kvadwake_feed(wake2[n+wbf_cnt]->vwake, NULL, 0, 1);
                        }
                    } else 
                    {   
                        for(i=0;i<nbin;++i)
                        {
                            saec_out[n][i]=msg->fft[i][n];
                        }
                        k=wtk_stft2_output_ifft(bfio->stft2,saec_out[n],data,saec_pad[n],bfio->end_pos,is_end);
                        if(bfio->cfg->use_preemph)
                        {
                            bfio->memX[n+wbf_cnt]=wtk_preemph_asis2(data,k,bfio->memX[n+wbf_cnt]);
                        }
                        pv=(short *)data;
                        for(i=0;i<k;++i)
                        {
                            pv[i] = QTK_SSAT16f(data[i]);
                        }
    
                        if(bfio->cfg->use_kvadwake){
                            wtk_kvadwake_feed(wake[n+wbf_cnt]->vwake, pv, k, 0);
                        }else{
                            if(n==0){
                                bfio->wake_extend+=k*1.0/bfio->cfg->rate;
                            }
                        }
                        if(bfio->cfg->use_kvadwake2){
                            wtk_kvadwake_feed(wake2[n+wbf_cnt]->vwake, pv, k, 0);
                        }else{
                            if(n==0){
                                bfio->wake2_extend+=k*1.0/bfio->cfg->rate;

                            }
                        }
                    }
                }
            }else{
                if(bfio->cfg->use_kvadwake){
                    wake[wbf_cnt]->waked=0;
                }
                if(bfio->cfg->use_kvadwake2){
                    wake2[wbf_cnt]->waked=0;
                }
                if (is_end) 
                {
                    if(bfio->cfg->use_kvadwake){
                        wtk_kvadwake_feed(wake[wbf_cnt]->vwake, NULL, 0, 1);
                    }
                    if(bfio->cfg->use_kvadwake2){
                        wtk_kvadwake_feed(wake2[wbf_cnt]->vwake, NULL, 0, 1);
                    }
                } else 
                {   
                    for(i=0;i<nbin;++i)
                    {
                        saec_out[0][i]=msg->fft[i][0];
                    }
                    k=wtk_stft2_output_ifft(bfio->stft2,saec_out[0],data,saec_pad[0],bfio->end_pos,is_end);
                    if(bfio->cfg->use_preemph)
                    {
                        bfio->memX[wbf_cnt]=wtk_preemph_asis2(data,k,bfio->memX[wbf_cnt]);
                    }
                    pv=(short *)data;
                    for(i=0;i<k;++i)
                    {
                        pv[i] = QTK_SSAT16f(data[i]);
                    }

                    if(bfio->cfg->use_kvadwake){
                        wtk_kvadwake_feed(wake[wbf_cnt]->vwake, pv, k, 0);
                    }else{
                        bfio->wake_extend+=k*1.0/bfio->cfg->rate;
                    }
                    if(bfio->cfg->use_kvadwake2){
                        wtk_kvadwake_feed(wake2[wbf_cnt]->vwake, pv, k, 0);
                    }else{
                        bfio->wake2_extend+=k*1.0/bfio->cfg->rate;
                    }
        #ifdef USE_LOG_BFIO

                    static wtk_wavfile_t *raw_log;
                    if(!raw_log)
                    {
                        raw_log=wtk_wavfile_new(16000);
                        wtk_debug(" ==========> open wbf/wbf.0.wav\n");
                        wtk_wavfile_open(raw_log,"wbf/wbf.0.wav");
                        raw_log->max_pend=0;
                    }
                    if(k>0)
                    {
                        wtk_wavfile_write(raw_log,(char *)pv,k<<1);
                    }
                    if(is_end)
                    {
                        wtk_debug("============ close ============\n");
                        wtk_wavfile_close(raw_log);
                        wtk_wavfile_delete(raw_log);
                        raw_log=NULL;
                    }
        #endif
                }
            }
        }
        if (bfio->cfg->use_gdenoise) {
            if(bfio->cfg->use_all_raw_audio){
                if(bfio->cfg->use_kvadwake){
                    wake[wbf_cnt + bfio->channel]->waked = 0;
                }
                if(bfio->cfg->use_kvadwake2){
                    wake2[wbf_cnt + bfio->channel]->waked=0;
                }
            }else{
                if(bfio->cfg->use_kvadwake){
                    wake[wbf_cnt + 1]->waked = 0;
                }
                if(bfio->cfg->use_kvadwake2){
                    wake2[wbf_cnt + 1]->waked=0;
                }
            }
            for (i = 0; i < nbin; ++i) {
                saec_out[0][i] = msg->fft[i][0];
            }
            wtk_gainnet_denoise_feed2(bfio->gdenoise, saec_out[0], is_end);
        }
        _bfio_waked_post(bfio, is_end);
    }
}


void wtk_bfio_on_stft2(wtk_bfio_t *bfio,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *mspstft_q=&(bfio->mspstft_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *sp_msg;

    qn=wtk_queue_pop(mspstft_q);
    if(qn)
    {
        sp_msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
    }else
    {
        sp_msg=NULL;
    }

    wtk_aec_feed_stftmsg(bfio->aec, msg, sp_msg, pos, is_end);

    if(sp_msg)
    {
        wtk_stft2_push_msg(bfio->sp_stft2, sp_msg);
    }
}

void wtk_bfio_on_sp_stft2(wtk_bfio_t *bfio,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    if(msg)
    {
        wtk_queue_push(&(bfio->mspstft_q),  &(msg->q_n));
    }
}

void wtk_bfio_feed_dc(wtk_bfio_t *bfio,short **data,int len,int is_end)
{   
    int i,j;
    int channel=bfio->channel;
    int sp_channel=bfio->sp_channel;
    float fv;
    float *fp[512],*fp2[512];
    wtk_strbuf_t **input=bfio->input;

    for(i=0;i<channel;++i)
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j];
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        
        wtk_preemph_dc(fp[i], bfio->notch_mem[i], len);
        bfio->memD[i]=wtk_preemph_asis(fp[i], len, bfio->memD[i]);
    }

    if(bfio->aec)
    {
        for(i=channel;i<channel+sp_channel;++i){
            wtk_strbuf_reset(input[i]);
            for(j=0;j<len;++j)
            {
                fv=data[i][j];
                wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
            }
            fp2[i-channel]=(float *)(input[i]->data);
            
            wtk_preemph_dc(fp2[i-channel], bfio->notch_mem[i], len);
            bfio->memD[i]=wtk_preemph_asis(fp2[i-channel], len, bfio->memD[i]);
        }
        wtk_stft2_feed_float(bfio->sp_stft2, fp2, len, is_end);
    }
    wtk_stft2_feed_float(bfio->stft2, fp, len, is_end);
}


void wtk_bfio_feed(wtk_bfio_t *bfio,short **data,int len,int is_end)
{
    short *pv[16];
    int i;
    int channel=bfio->channel;
    int spchannel=bfio->sp_channel;

    if(bfio->input)
    {
        wtk_bfio_feed_dc(bfio,data,len,is_end);
    }else
    {
        if(bfio->aec)
        {
			if(data)
			{
                for(i=0;i<spchannel;++i)
                {
				    pv[i]=data[channel+i];
                }
				wtk_stft2_feed2(bfio->sp_stft2, pv, len, is_end);
			}else
			{
				wtk_stft2_feed2(bfio->sp_stft2, NULL, 0, is_end);
			}
        }
        wtk_stft2_feed2(bfio->stft2, data, len, is_end);
    }
    if(bfio->aec && bfio->aec_wake_buf && !is_end){
        wtk_strbuf_t *aec_wake_buf=bfio->aec_wake_buf;
        int aec_len;
        int aec_wake_len=bfio->cfg->aec_wake_len;

        wtk_strbuf_push(aec_wake_buf, (char *)data[channel], len*sizeof(short));

        aec_len=aec_wake_buf->pos/sizeof(short);
        
        if(aec_len>aec_wake_len){
            wtk_strbuf_pop(aec_wake_buf, NULL, (aec_len-aec_wake_len)*sizeof(short));
        }
        bfio->aec_fe+=len;
        bfio->aec_fs=max(0,bfio->de_fe-aec_wake_len);
        // wtk_debug("%f %f\n", bfio->aec_fs, bfio->aec_fe);
    }
}

void wtk_bfio_set_one_shot(wtk_bfio_t *bfio, int on) {
    bfio->cfg->use_one_shot = on;
    if (!on) {
        bfio->waked = 1;
    }else{
        bfio->waked = 0;
    }
}

void wtk_bfio_set_offline_qform(wtk_bfio_t *bfio, int tms, int theta)
{
    int max_theta=bfio->cfg->use_line?180:359;

    bfio->reg_bf=1;
    bfio->reg_theta=theta;
    bfio->reg_tms=tms;
    bfio->reg_end=0;
    if(theta >= 0 && theta <= max_theta){
        if(bfio->qform)
        {
            wtk_qform9_start2(bfio->qform,theta,0);
        }else if(bfio->qform2)
        {
            wtk_qform2_start2(bfio->qform2,theta,0);
        }else if(bfio->qform3){
            wtk_qform3_start2(bfio->qform3,theta,0);
        }
    }
}

int wtk_bfio_set_wake_words(wtk_bfio_t *bfio,char *words,int len)
{
    int ret = -1;
    int wake_cnt=bfio->wake_cnt;
    int i;

    if(bfio->wake2){
        bfio->cfg->use_kvadwake2=0;
        if(bfio->wake2[0]->vwake->cfg->use_kwdec2){
            for(i=0;i<wake_cnt;++i){
                wtk_kwdec2_feed2(bfio->wake2[i]->vwake->kwdec2, NULL, 0, 1);
            }
            for(i=0;i<wake_cnt;++i){
                ret = wtk_kwdec2_set_words(bfio->wake2[i]->vwake->kwdec2,words,strlen(words));
                if(ret!=0){
                    goto end;
                }
            }
            wtk_kwdec2_set_words_cfg(bfio->wake2[0]->vwake->kwdec2,words,strlen(words));
            for(i=0;i<wake_cnt;++i){
                wtk_kwdec2_decwords_set(bfio->wake2[i]->vwake->kwdec2);
            }
            for(i=0;i<wake_cnt;++i){
                wtk_kwdec2_reset(bfio->wake2[i]->vwake->kwdec2);
                wtk_kwdec2_start(bfio->wake2[i]->vwake->kwdec2);
            }
        }else if(bfio->wake2[0]->vwake->cfg->use_wdec){
            wtk_wdec_set_words(bfio->wake2[wake_cnt-1]->vwake->wdec,words,strlen(words));
            wtk_wdec_feed(bfio->wake2[wake_cnt-1]->vwake->wdec,NULL,0,1);
            wtk_wdec_reset(bfio->wake2[wake_cnt-1]->vwake->wdec);
            wtk_wdec_start(bfio->wake2[wake_cnt-1]->vwake->wdec, NULL);
        }
        bfio->cfg->use_kvadwake2=1;
    }
end:
    return ret;
}
