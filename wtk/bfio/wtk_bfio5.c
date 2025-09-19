#include "wtk_bfio5.h" 

void wtk_bfio5_on_sp_stft2(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_bfio5_on_stft2(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_bfio5_on_stft2_2(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end);

void wtk_bfio5_on_aec(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end);

void wtk_bfio5_on_wbf(wtk_bfio5_t *bfio5,wtk_complex_t *wbf_out,int idx,int is_end);
void wtk_bfio5_on_gdenoise(wtk_bfio5_t *bfio5, wtk_complex_t *gd_out, int is_end);
void wtk_bfio5_on_wake(wtk_bfio5_wake_t *wake,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len);
void wtk_bfio5_on_wake2(wtk_bfio5_t *bfio5,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len);

static void wtk_bfio5_on_qform(wtk_bfio5_t *bfio5, wtk_complex_t *qf_out,
                              int is_end);
void wtk_bfio5_on_vad(wtk_bfio5_t *bfio5,wtk_vad2_cmd_t cmd,short *data,int len);

void wtk_bfio5_on_ssl2(wtk_bfio5_t *bfio5, wtk_ssl2_extp_t *nbest_extp, int nbest, int ts, int te);
void wtk_bfio5_on_ssl2_2(wtk_bfio5_t *bfio5, wtk_ssl2_extp_t *nbest_extp, int nbest, int ts, int te);

void wtk_bfio5_aec_on_gainnet(wtk_bfio5_edr_t *edr, float *gain, int len, int is_end);

void wtk_bfio5_edr_init(wtk_bfio5_edr_t *edr, wtk_bfio5_cfg_t *cfg)
{
	edr->cfg=cfg;
	edr->nbin=cfg->wins/2+1;

	edr->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));
	edr->bank_sp=wtk_bankfeat_new(&(cfg->bankfeat));

	edr->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	edr->gf=wtk_malloc(sizeof(float)*edr->nbin);

	edr->feature_sp=NULL;
	if(cfg->featm_lm+cfg->featsp_lm>2)
	{
		edr->feature_sp=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_features*(cfg->featm_lm+cfg->featsp_lm-1));
	}

	edr->gainnet2=wtk_gainnet2_new(cfg->gainnet2);
	wtk_gainnet2_set_notify(edr->gainnet2, edr, (wtk_gainnet2_notify_f)wtk_bfio5_aec_on_gainnet);

	edr->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		edr->qmmse=wtk_qmmse_new(&(cfg->qmmse));
	}
}

void wtk_bfio5_edr_clean(wtk_bfio5_edr_t *edr)
{
	wtk_bankfeat_delete(edr->bank_mic);
	wtk_bankfeat_delete(edr->bank_sp);

	if(edr->feature_sp)
	{
		wtk_free(edr->feature_sp);
	}

	wtk_free(edr->g);
	wtk_free(edr->gf);

	if(edr->qmmse)
	{
		wtk_qmmse_delete(edr->qmmse);
	}

	wtk_gainnet2_delete(edr->gainnet2);
}

void wtk_bfio5_edr_reset(wtk_bfio5_edr_t *edr)
{
	wtk_bankfeat_reset(edr->bank_mic);
	wtk_bankfeat_reset(edr->bank_sp);

	if(edr->feature_sp)
	{
		memset(edr->feature_sp, 0, sizeof(float)*edr->bank_sp->cfg->nb_features*(edr->cfg->featm_lm+edr->cfg->featsp_lm-1));
	}

	memset(edr->g, 0, sizeof(float)*edr->cfg->bankfeat.nb_bands);
	memset(edr->gf, 0, sizeof(float)*edr->nbin);

	wtk_gainnet2_reset(edr->gainnet2);

	if(edr->qmmse)
	{
		wtk_qmmse_reset(edr->qmmse);
	}
}

wtk_bfio5_edr_msg_t* wtk_bfio5_edr_msg_new(wtk_bfio5_t *bfio5)
{
	wtk_bfio5_edr_msg_t *msg;

	msg=(wtk_bfio5_edr_msg_t*)wtk_malloc(sizeof(wtk_bfio5_edr_msg_t));
	msg->hook=NULL;
    msg->fft=NULL;
    if(bfio5->cfg->use_stft2_2){
        msg->fft=wtk_complex_new_p2(bfio5->stft2->cfg->channel+bfio5->stft2_2->cfg->channel+bfio5->sp_stft2->cfg->channel, bfio5->stft2->nbin);
    }else{
        msg->fft=wtk_complex_new_p2(bfio5->stft2->cfg->channel+bfio5->sp_stft2->cfg->channel, bfio5->stft2->nbin);
    }
    msg->mask=(float *)wtk_malloc(sizeof(float)*bfio5->stft2->nbin);
	msg->theta=-1;
	msg->theta2=-1;
    return msg;
}

void wtk_bfio5_edr_msg_delete(wtk_bfio5_t *bfio5,wtk_bfio5_edr_msg_t *msg)
{
    if(bfio5->cfg->use_stft2_2){
        wtk_complex_delete_p2(msg->fft,bfio5->stft2->cfg->channel+bfio5->stft2_2->cfg->channel+bfio5->sp_stft2->cfg->channel);
    }else{
        wtk_complex_delete_p2(msg->fft,bfio5->stft2->cfg->channel+bfio5->sp_stft2->cfg->channel);
    }
    wtk_free(msg->mask);
	wtk_free(msg);
}

wtk_bfio5_edr_msg_t* wtk_bfio5_edr_pop_msg(wtk_bfio5_t *bfio5)
{
	return  (wtk_bfio5_edr_msg_t*)wtk_hoard_pop(&(bfio5->msg_hoard));
}

void wtk_bfio5_edr_push_msg(wtk_bfio5_t *bfio5,wtk_bfio5_edr_msg_t *msg)
{
	//wtk_debug("push [%ld]=%p\n",(long)(msg->hook),msg);
	wtk_hoard_push(&(bfio5->msg_hoard),msg);
}

wtk_bfio5_wake_t *wtk_bfio5_wake_new(wtk_bfio5_t *bfio5, wtk_kvadwake_cfg_t *cfg)
{
    wtk_bfio5_wake_t *wake;

    wake=(wtk_bfio5_wake_t *)wtk_malloc(sizeof(wtk_bfio5_wake_t));
    wake->hook=bfio5;

    wake->vwake=wtk_kvadwake_new(cfg);
    wtk_kvadwake_set_notify(wake->vwake, wake, (wtk_kvadwake_notify_f)wtk_bfio5_on_wake);
    wtk_kvadwake_set_notify2(wake->vwake, bfio5, (wtk_kvadwake_notify_f2)wtk_bfio5_on_wake2);

    return wake;
}

void wtk_bfio5_wake_reset(wtk_bfio5_wake_t *wake)
{
    wake->waked=0;
    wake->wake_fs=0;
    wake->wake_fe=0;

    wtk_kvadwake_reset(wake->vwake);
}

void wtk_bfio5_wake_start(wtk_bfio5_wake_t *wake, int theta)
{
    wake->theta=theta;
    wtk_kvadwake_start(wake->vwake);
}

void wtk_bfio5_wake_delete(wtk_bfio5_wake_t *wake)
{
    wtk_kvadwake_delete(wake->vwake);
    wtk_free(wake);
}

wtk_bfio5_t* wtk_bfio5_new(wtk_bfio5_cfg_t *cfg)
{
    wtk_bfio5_t *bfio5;
    int i;
    int wbf_cnt;

    bfio5=(wtk_bfio5_t *)wtk_malloc(sizeof(wtk_bfio5_t));
    bfio5->cfg=cfg;
    bfio5->ths=NULL;
    bfio5->notify=NULL;
    bfio5->ssl2_ths=NULL;
    bfio5->notify_ssl2=NULL;

    bfio5->channel=cfg->stft2.channel;
    bfio5->sp_channel=cfg->sp_stft2.channel;

    wtk_queue_init(&(bfio5->mspstft_q));
    wtk_queue_init(&(bfio5->mstft_2_q));
    wtk_queue_init(&(bfio5->stft2_q));
    wtk_queue_init(&(bfio5->edr_stft_q));

    bfio5->sp_stft2=NULL;
    bfio5->stft2=NULL;
    bfio5->stft2_2=NULL;
    bfio5->aec=NULL;
    bfio5->channel2=0;
    if(cfg->use_aec)
    {
        bfio5->sp_stft2=wtk_stft2_new(&(cfg->sp_stft2));
        wtk_stft2_set_notify(bfio5->sp_stft2,bfio5,(wtk_stft2_notify_f)wtk_bfio5_on_sp_stft2);

        bfio5->stft2=wtk_stft2_new(&(cfg->stft2));
        wtk_stft2_set_notify(bfio5->stft2,bfio5,(wtk_stft2_notify_f)wtk_bfio5_on_stft2);
        bfio5->aec=wtk_aec_new2(&(cfg->aec), bfio5->stft2);
        wtk_aec_set_notify2(bfio5->aec, bfio5, (wtk_aec_notify_f2)wtk_bfio5_on_aec);
    }else
    {
        bfio5->stft2=wtk_stft2_new(&(cfg->stft2));
        wtk_stft2_set_notify(bfio5->stft2,bfio5,(wtk_stft2_notify_f)wtk_bfio5_on_aec);
    }
    if(cfg->use_maskssl2_2){
        bfio5->stft2_2=wtk_stft2_new(&(cfg->stft2_2));
        wtk_stft2_set_notify(bfio5->stft2_2,bfio5,(wtk_stft2_notify_f)wtk_bfio5_on_stft2_2);
        bfio5->channel2=cfg->stft2_2.channel;
    }
    
    bfio5->nbin=bfio5->stft2->nbin;
    bfio5->wbf=NULL;
    bfio5->wbf2=NULL;

    if(cfg->use_wbf2)
    {
        bfio5->wbf2=wtk_wbf2_new2(&(cfg->wbf2), bfio5->stft2);
        wtk_wbf2_set_notify2(bfio5->wbf2, bfio5, (wtk_wbf2_notify_f2)wtk_bfio5_on_wbf);
    }else
    {
        bfio5->wbf=wtk_wbf_new2(&(cfg->wbf), bfio5->stft2);    
        wtk_wbf_set_notify2(bfio5->wbf, bfio5, (wtk_wbf_notify_f2)wtk_bfio5_on_wbf);
    }

    bfio5->gdenoise = NULL;
    if (cfg->use_gdenoise) {
        bfio5->gdenoise = wtk_gainnet_denoise_new2(&(cfg->gdenoise));
        wtk_gainnet_denoise_set_notify2(
            bfio5->gdenoise, bfio5,
            (wtk_gainnet_denoise_notify_f2)wtk_bfio5_on_gdenoise);
    }

    wbf_cnt=bfio5->wbf? bfio5->wbf->cfg->wbf_cnt:bfio5->wbf2->cfg->wbf2_cnt;
    bfio5->wake_cnt = wbf_cnt;
    if (cfg->use_raw_audio) {
        if(cfg->use_all_raw_audio){
            bfio5->wake_cnt += bfio5->channel;
        }else{
            bfio5->wake_cnt += 1;
        }
    }
    if (cfg->use_gdenoise) {
        bfio5->wake_cnt += 1;
    }else if(cfg->use_mic2_raw_audio){
        bfio5->wake_cnt += bfio5->channel2;
    }
    bfio5->wake=NULL;
    if(cfg->use_kvadwake){
        bfio5->wake = (wtk_bfio5_wake_t **)wtk_malloc(sizeof(wtk_bfio5_wake_t *) *
                                                    bfio5->wake_cnt);
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            bfio5->wake[i]=wtk_bfio5_wake_new(bfio5, &(cfg->vwake));
        }
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            if(cfg->use_gdenoise && i == bfio5->wake_cnt-1){
                wtk_kvadwake_set_idx(bfio5->wake[i]->vwake, 1); // 1表示是模型降噪
            }else{
                wtk_kvadwake_set_idx(bfio5->wake[i]->vwake, 0); // 0表示非模型降噪
            }
        }
    }

    bfio5->wake2=NULL;
    if(cfg->use_kvadwake2){
        bfio5->wake2 = (wtk_bfio5_wake_t **)wtk_malloc(sizeof(wtk_bfio5_wake_t *) *
                                                    bfio5->wake_cnt);
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            bfio5->wake2[i]=wtk_bfio5_wake_new(bfio5, &(cfg->vwake2));
        }
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            if(bfio5->cfg->use_gdenoise && i == bfio5->wake_cnt-1){
                wtk_kvadwake_set_idx(bfio5->wake2[i]->vwake, 1); // 1表示是模型降噪
            }else{
                wtk_kvadwake_set_idx(bfio5->wake2[i]->vwake, 0); // 0表示非模型降噪
            }
        }
    }
    bfio5->wake3=NULL;
    bfio5->aec_wake_buf=NULL;
    if(cfg->use_aec_wake){
        bfio5->wake3=wtk_bfio5_wake_new(bfio5, &(cfg->vwake3));
        wtk_kvadwake_set_idx(bfio5->wake3->vwake, 0);
        bfio5->aec_wake_buf=wtk_strbuf_new(1024, 1);
    }


    bfio5->saec_out=NULL;
    bfio5->saec_pad=NULL;
    if(cfg->use_raw_audio)
    {
        if(cfg->use_all_raw_audio){
            bfio5->saec_out=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *)*bfio5->channel);
            bfio5->saec_pad=(float **)wtk_malloc(sizeof(float *)*bfio5->channel);
            for(i=0;i<bfio5->channel;++i){
                bfio5->saec_out[i]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*bfio5->stft2->nbin);
                bfio5->saec_pad[i]=(float *)wtk_malloc(sizeof(float)*bfio5->stft2->cfg->win);
            }
        }else{
            bfio5->saec_out=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *));
            bfio5->saec_pad=(float **)wtk_malloc(sizeof(float *));
            bfio5->saec_out[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*bfio5->stft2->nbin);
            bfio5->saec_pad[0]=(float *)wtk_malloc(sizeof(float)*bfio5->stft2->cfg->win);
        }
    }
    bfio5->mic2_out=NULL;
    bfio5->mic2_pad=NULL;
    if(cfg->use_mic2_raw_audio)
    {
        bfio5->mic2_out=wtk_complex_new_p2(bfio5->channel2,bfio5->stft2_2->nbin);
        bfio5->mic2_pad=wtk_float_new_p2(bfio5->channel2,bfio5->stft2_2->cfg->win);
    }
    bfio5->ssl=wtk_ssl_new2(&(cfg->ssl), bfio5->stft2);
    bfio5->ssl2=NULL;
    if(cfg->use_ssl2){
        bfio5->ssl2=wtk_ssl2_new(&(cfg->ssl2));
        wtk_ssl2_set_notify(bfio5->ssl2, bfio5, (wtk_ssl2_notify_f)wtk_bfio5_on_ssl2);
    }

    bfio5->qform=NULL;
    bfio5->qform2=NULL;
    bfio5->qform3=NULL;
    if(cfg->use_qform2)
    {
        bfio5->qform2=wtk_qform2_new2(&(cfg->qform2),bfio5->stft2);
    }else if(cfg->use_qform3){
        bfio5->qform3=wtk_qform3_new2(&(cfg->qform3),bfio5->stft2);
    }else
    {
        bfio5->qform=wtk_qform9_new2(&(cfg->qform9),bfio5->stft2);
        wtk_qform9_set_notify2(bfio5->qform, bfio5, (wtk_qform9_notify_f2)wtk_bfio5_on_qform);   
    }

    bfio5->input=NULL;
    if(cfg->use_preemph)
    {
        bfio5->memX=(float *)wtk_malloc(sizeof(float)*(bfio5->wake_cnt+1));
        bfio5->input=wtk_strbufs_new(bfio5->cfg->in_channel);
        bfio5->notch_mem=wtk_float_new_p2(bfio5->cfg->in_channel,2);
        bfio5->memD=(float *)wtk_malloc(sizeof(float)*(bfio5->cfg->in_channel));
    }

    bfio5->vad=NULL;
    if(cfg->use_asr && cfg->use_vad)
    {
        bfio5->vad=wtk_vad2_new(&(cfg->vad));
        wtk_vad2_set_notify(bfio5->vad, bfio5, (wtk_vad2_notify_f)wtk_bfio5_on_vad);
        bfio5->vad->use_vad_start=cfg->use_vad_start;
    }
    
    bfio5->decoder = cfg->use_offline_asr ? qtk_decoder_wrapper_new(&cfg->decoder) : NULL;

    bfio5->de_wake_buf=NULL;
    if(bfio5->cfg->use_en_trick){
        if(!cfg->use_kvadwake){
            wtk_debug("error: use_en_trick must use_kvadwake");
            return NULL;
        }
        bfio5->de_wake_buf = wtk_strbufs_new(bfio5->wake_cnt);
    }

    bfio5->edr=NULL;
    bfio5->fftx=NULL;
    bfio5->ffty=NULL;
    if(cfg->aecmdl_fn){
        bfio5->edr=(wtk_bfio5_edr_t *)wtk_malloc(sizeof(wtk_bfio5_edr_t));
        wtk_bfio5_edr_init(bfio5->edr, cfg);
        wtk_hoard_init2(&(bfio5->msg_hoard),offsetof(wtk_bfio5_edr_msg_t,hoard_n),10,
        (wtk_new_handler_t)wtk_bfio5_edr_msg_new,
        (wtk_delete_handler2_t)wtk_bfio5_edr_msg_delete,
        bfio5);
        bfio5->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*bfio5->stft2->nbin);
        bfio5->ffty=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*bfio5->stft2->nbin);
    }
    bfio5->maskssl2=NULL;
    bfio5->maskssl2_2=NULL;
    bfio5->mssl_tmp=NULL;
    bfio5->mssl_2_tmp=NULL;
    if(cfg->use_maskssl2){
		bfio5->maskssl2=wtk_maskssl2_new(&(cfg->maskssl2));
        wtk_maskssl2_set_notify(bfio5->maskssl2, bfio5, (wtk_maskssl2_notify_f)wtk_bfio5_on_ssl2);
        bfio5->mssl_tmp=(int *)wtk_malloc(sizeof(int)*10);
    }
    if(cfg->use_maskssl2_2){
		bfio5->maskssl2_2=wtk_maskssl2_new(&(cfg->maskssl2_2));
        wtk_maskssl2_set_notify(bfio5->maskssl2_2, bfio5, (wtk_maskssl2_notify_f)wtk_bfio5_on_ssl2_2);
        bfio5->mssl_2_tmp=(int *)wtk_malloc(sizeof(int)*10);
    }
    bfio5->erls=NULL;
    if(cfg->use_rls){
		bfio5->erls=wtk_malloc(sizeof(wtk_rls_t)*(bfio5->nbin));
		for(i=0;i<bfio5->nbin;++i)
		{
		    wtk_rls_init(bfio5->erls+i, &(cfg->echo_rls));
		}
    }

    wtk_bfio5_reset(bfio5);

    return bfio5;
}

void wtk_bfio5_delete(wtk_bfio5_t *bfio5)
{
    int i;

    wtk_hoard_clean(&(bfio5->msg_hoard));
    if(bfio5->wake){
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            wtk_bfio5_wake_delete(bfio5->wake[i]);
        }
        wtk_free(bfio5->wake);
    }
    if(bfio5->wake2){
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            wtk_bfio5_wake_delete(bfio5->wake2[i]);
        }
        wtk_free(bfio5->wake2);
    }
    if(bfio5->wake3){
        wtk_bfio5_wake_delete(bfio5->wake3);
    }
    if(bfio5->aec_wake_buf){
        wtk_strbuf_delete(bfio5->aec_wake_buf);
    }

    if(bfio5->wbf)
    {
        wtk_wbf_delete2(bfio5->wbf);
    }else
    {
        wtk_wbf2_delete2(bfio5->wbf2);   
    }
    if (bfio5->gdenoise) {
        wtk_gainnet_denoise_delete2(bfio5->gdenoise);
    }
    if(bfio5->saec_pad)
    {
        if(bfio5->cfg->use_all_raw_audio){
            for(i=0;i<bfio5->channel;++i){
                wtk_free(bfio5->saec_pad[i]);
                wtk_free(bfio5->saec_out[i]);
            }
        }else{
            wtk_free(bfio5->saec_pad[0]);
            wtk_free(bfio5->saec_out[0]);
        }
        wtk_free(bfio5->saec_pad);
        wtk_free(bfio5->saec_out);
    }
    if(bfio5->mic2_pad)
    {
        wtk_complex_delete_p2(bfio5->mic2_out, bfio5->channel2);
        wtk_float_delete_p2(bfio5->mic2_pad, bfio5->channel2);
    }
    wtk_ssl_delete2(bfio5->ssl);
    if(bfio5->ssl2){
        wtk_ssl2_delete(bfio5->ssl2);
    }
    if(bfio5->qform)
    {
        wtk_qform9_delete2(bfio5->qform);
    }else if(bfio5->qform2)
    {
        wtk_qform2_delete2(bfio5->qform2);
    }else if(bfio5->qform3){
        wtk_qform3_delete2(bfio5->qform3);
    }

    if(bfio5->vad)
    {
        wtk_vad2_delete(bfio5->vad);
    }

    if(bfio5->aec)
    {
        wtk_aec_delete2(bfio5->aec);
        wtk_stft2_delete(bfio5->sp_stft2);
    }
    wtk_stft2_delete(bfio5->stft2);
    if(bfio5->stft2_2){
        wtk_stft2_delete(bfio5->stft2_2);
    }

    if(bfio5->input)
    {

        wtk_strbufs_delete(bfio5->input, bfio5->cfg->in_channel);
        wtk_float_delete_p2(bfio5->notch_mem,  bfio5->cfg->in_channel);
        wtk_free(bfio5->memD);
        wtk_free(bfio5->memX);
    }

    if (bfio5->decoder) {
        qtk_decoder_wrapper_delete(bfio5->decoder);
    }

    if(bfio5->de_wake_buf){
        wtk_strbufs_delete(bfio5->de_wake_buf, bfio5->wake_cnt);
    }

    if(bfio5->edr){
        wtk_bfio5_edr_clean(bfio5->edr);
    }
    wtk_free(bfio5->edr);
    if(bfio5->fftx){
        wtk_free(bfio5->fftx);
    }
    if(bfio5->ffty){
        wtk_free(bfio5->ffty);
    }
    if(bfio5->maskssl2){
        wtk_maskssl2_delete(bfio5->maskssl2);
    }
    if(bfio5->maskssl2_2){
        wtk_maskssl2_delete(bfio5->maskssl2_2);
    }
    if(bfio5->mssl_tmp){
        wtk_free(bfio5->mssl_tmp);
    }
    if(bfio5->mssl_2_tmp){
        wtk_free(bfio5->mssl_2_tmp);
    }
	if(bfio5->erls)
	{
		for(i=0;i<bfio5->nbin;++i)
		{
			wtk_rls_clean(bfio5->erls+i);
		}
		wtk_free(bfio5->erls);
	}

    wtk_free(bfio5);
}

void wtk_bfio5_reset(wtk_bfio5_t *bfio5)
{
    int i;
    wtk_queue_t *stft2_q=&(bfio5->stft2_q);
    wtk_queue_t *edr_stft_q=&(bfio5->edr_stft_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *msg2;
    wtk_bfio5_edr_msg_t *msg3;
    bfio5->asr = 0;
    if(bfio5->input)
    {
        memset(bfio5->memX,0,(bfio5->wake_cnt+1)*sizeof(float));
        for(i=0;i<bfio5->cfg->in_channel;++i)
        {
            memset(bfio5->notch_mem[i],0,2*sizeof(float));
        }
        memset(bfio5->memD,0,(bfio5->cfg->in_channel)*sizeof(float));

        if(bfio5->input)
        {
            wtk_strbufs_reset(bfio5->input,  bfio5->cfg->in_channel);
        }
    }

    wtk_queue_init(&(bfio5->mspstft_q));
    wtk_queue_init(&(bfio5->mstft_2_q));

    if(bfio5->aec)
    {
        wtk_aec_reset2(bfio5->aec);
        wtk_stft2_reset(bfio5->sp_stft2);
    }

    while(stft2_q->length>0)
    {
        qn=wtk_queue_pop(stft2_q);
        msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
        wtk_stft2_push_msg(bfio5->stft2, msg2);
    }
    wtk_queue_init(stft2_q);
    wtk_stft2_reset(bfio5->stft2);
    if(bfio5->stft2_2){
        wtk_stft2_reset(bfio5->stft2_2);
    }

    bfio5->end_pos=0;
    bfio5->is_end=0;

    bfio5->waked=0;
    bfio5->wake_theta=0;
    bfio5->wake_fs=bfio5->wake_fe=-1;
    bfio5->wake_extend=0;
    bfio5->wake2_extend=0;
    bfio5->wake_prob=0;
    bfio5->wake2_prob=0;
    bfio5->out_wake_prob=0;
    bfio5->wake_key=0;
    bfio5->sil_ts=0;
    bfio5->speech_ts=0;
    bfio5->sil_end=0;

    bfio5->vad_output=0;
    bfio5->vad_ts=0;
    bfio5->vad_te=0;

    if(bfio5->wake){
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            wtk_bfio5_wake_reset(bfio5->wake[i]);
        }
    }
    if(bfio5->wake2){
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            wtk_bfio5_wake_reset(bfio5->wake2[i]);
        }
    }
    if(bfio5->wake3){
        wtk_bfio5_wake_reset(bfio5->wake3);
    }
    if(bfio5->aec_wake_buf){
        wtk_strbuf_reset(bfio5->aec_wake_buf);
    }

    if(bfio5->wbf)
    {
        wtk_wbf_reset2(bfio5->wbf);
    }else
    {
        wtk_wbf2_reset2(bfio5->wbf2);
    }
    if (bfio5->gdenoise) {
        wtk_gainnet_denoise_reset2(bfio5->gdenoise);
    }
    if(bfio5->saec_pad)
    {
        if(bfio5->cfg->use_all_raw_audio){
            for(i=0;i<bfio5->channel;++i){
                memset(bfio5->saec_out[i], 0, sizeof(wtk_complex_t)*bfio5->stft2->nbin);
                memset(bfio5->saec_pad[i], 0, sizeof(float)*bfio5->stft2->cfg->win);
            }
        }else{
            memset(bfio5->saec_out[0], 0, sizeof(wtk_complex_t)*bfio5->stft2->nbin);
            memset(bfio5->saec_pad[0], 0, sizeof(float)*bfio5->stft2->cfg->win);
        }
    }
    if(bfio5->mic2_pad)
    {
        wtk_complex_zero_p2(bfio5->mic2_out, bfio5->channel2, bfio5->stft2_2->nbin);
        wtk_float_zero_p2(bfio5->mic2_pad, bfio5->channel2, bfio5->stft2_2->cfg->win);
    }
    wtk_ssl_reset2(bfio5->ssl);
    if(bfio5->ssl2){
        wtk_ssl2_reset(bfio5->ssl2);
    }
    if(bfio5->qform)
    {
        wtk_qform9_reset2(bfio5->qform);
    }else if(bfio5->qform2)
    {
        wtk_qform2_reset2(bfio5->qform2);
    }else if(bfio5->qform3){
        wtk_qform3_reset2(bfio5->qform3);
    }
    
    if(bfio5->vad)
    {
        wtk_vad2_reset(bfio5->vad);
    }
    
    if (bfio5->decoder) {
        qtk_decoder_wrapper_reset(bfio5->decoder);
    }

    if(bfio5->de_wake_buf){
        wtk_strbufs_reset(bfio5->de_wake_buf, bfio5->wake_cnt);
    }
    bfio5->de_fs=0;
    bfio5->de_fe=0;
    bfio5->aec_fs=0;
    bfio5->aec_fe=0;

    bfio5->reg_bf=0;
    bfio5->reg_theta=-1;
    bfio5->reg_tms=-1;
    bfio5->reg_end=0;
    bfio5->wake2_ready=0;
    
    bfio5->sum_low=0;
    bfio5->low_cnt=0;

    if(bfio5->edr){
        wtk_bfio5_edr_reset(bfio5->edr);
    }
    if(bfio5->fftx){
        memset(bfio5->fftx, 0, sizeof(wtk_complex_t)*bfio5->stft2->nbin);
    }
    if(bfio5->ffty){
        memset(bfio5->ffty, 0, sizeof(wtk_complex_t)*bfio5->stft2->nbin);
    }
    if(bfio5->maskssl2){
        wtk_maskssl2_reset(bfio5->maskssl2);
    }
    if(bfio5->maskssl2_2){
        wtk_maskssl2_reset(bfio5->maskssl2_2);
    }
    if(bfio5->mssl_tmp){
        memset(bfio5->mssl_tmp, 0, sizeof(int)*10);
    }
    if(bfio5->mssl_2_tmp){
        memset(bfio5->mssl_2_tmp, 0, sizeof(int)*10);
    }
    while(edr_stft_q->length>0)
    {
        qn=wtk_queue_pop(edr_stft_q);
        msg3=(wtk_bfio5_edr_msg_t *)data_offset2(qn, wtk_bfio5_edr_msg_t, q_n);
        wtk_bfio5_edr_push_msg(bfio5, msg3);
    }
    wtk_queue_init(edr_stft_q);
	if(bfio5->erls)
	{
		for(i=0;i<bfio5->nbin;++i)
		{
			wtk_rls_reset(bfio5->erls+i);
		}
	}

	bfio5->sp_silcnt=0;
	bfio5->sp_sil=1;
}

void wtk_bfio5_set_notify(wtk_bfio5_t *bfio5,void *ths,wtk_bfio5_notify_f notify)
{
    bfio5->ths=ths;
    bfio5->notify=notify;
}

void wtk_bfio5_set_ssl2_notify(wtk_bfio5_t *bfio5, void *ths, wtk_bfio5_notify_ssl2_f notify)
{
    bfio5->ssl2_ths=ths;
    bfio5->notify_ssl2=notify;
}

void wtk_bfio5_on_ssl2(wtk_bfio5_t *bfio5, wtk_ssl2_extp_t *nbest_extp, int nbest, int ts, int te)
{
    if(nbest>0){
        ts = ts * (bfio5->cfg->rate / 1000);
        te = te * (bfio5->cfg->rate / 1000);
        wtk_queue_t *edr_stft_q=&(bfio5->edr_stft_q);
        wtk_queue_node_t *qn;
        wtk_bfio5_edr_msg_t *edr_msg;
        for(qn=edr_stft_q->pop; qn; qn=qn->next)
        {
            edr_msg=(wtk_bfio5_edr_msg_t *)data_offset2(qn, wtk_bfio5_edr_msg_t, q_n);
            if(edr_msg->s >= ts)
            {
                edr_msg->theta=nbest_extp[0].theta;
                // wtk_debug("%f %d %d\n", edr_msg->s, ts, nbest_extp[0].theta);
            }
            if(edr_msg->s >= te)
            {
                break;
            }
        }
        // wtk_debug("%d %d %d\n", ts, te, nbest_extp[0].theta);
    }
    if(bfio5->notify_ssl2){
        bfio5->notify_ssl2(bfio5->ssl2_ths, nbest_extp, nbest);
    }
}

void wtk_bfio5_on_ssl2_2(wtk_bfio5_t *bfio5, wtk_ssl2_extp_t *nbest_extp, int nbest, int ts, int te)
{
    if(nbest>0){
        ts = ts * (bfio5->cfg->rate / 1000);
        te = te * (bfio5->cfg->rate / 1000);
        wtk_queue_t *edr_stft_q=&(bfio5->edr_stft_q);
        wtk_queue_node_t *qn;
        wtk_bfio5_edr_msg_t *edr_msg;
        for(qn=edr_stft_q->pop; qn; qn=qn->next)
        {
            edr_msg=(wtk_bfio5_edr_msg_t *)data_offset2(qn, wtk_bfio5_edr_msg_t, q_n);
            if(edr_msg->s >= ts)
            {
                edr_msg->theta2=nbest_extp[0].theta;
                // wtk_debug("%f %d %d\n", edr_msg->s, ts, nbest_extp[0].theta);
            }
            if(edr_msg->s >= te)
            {
                break;
            }
        }
        // wtk_debug("%d %d %d\n", ts, te, nbest_extp[0].theta);
    }
}

void wtk_bfio5_start(wtk_bfio5_t *bfio5)
{
    int i, wbf_cnt;
    int *theta;

    if (bfio5->wbf) {
        wbf_cnt = bfio5->wbf->cfg->wbf_cnt;
        theta = bfio5->wbf->theta;
    } else {
        wbf_cnt = bfio5->wbf2->cfg->wbf2_cnt;
        theta = bfio5->wbf2->cfg->theta;
    }

    if(bfio5->wbf)
    {
        wtk_wbf_start2(bfio5->wbf);
    }else
    {
        wtk_wbf2_start(bfio5->wbf2);
    }
    if(bfio5->wake){
        for(i=0; i<wbf_cnt; ++i)
        {
            wtk_bfio5_wake_start(bfio5->wake[i], theta[i]);
        }
        if (bfio5->cfg->use_raw_audio) {
            if(bfio5->cfg->use_all_raw_audio){
                for(i=0; i<bfio5->channel; ++i){
                    wtk_bfio5_wake_start(bfio5->wake[i+wbf_cnt], -1);
                }
            }else{
                wtk_bfio5_wake_start(bfio5->wake[wbf_cnt], -1);
            }
        }
        if(bfio5->cfg->use_mic2_raw_audio){
            for(i=bfio5->wake_cnt-bfio5->channel2; i<bfio5->wake_cnt; ++i){
                wtk_bfio5_wake_start(bfio5->wake[i], -1);
            }
        }
    }
    if(bfio5->wake2){
        for(i=0; i<wbf_cnt; ++i)
        {
            wtk_bfio5_wake_start(bfio5->wake2[i], theta[i]);
        }
        if (bfio5->cfg->use_raw_audio) {
            if(bfio5->cfg->use_all_raw_audio){
                for(i=0; i<bfio5->channel; ++i){
                    wtk_bfio5_wake_start(bfio5->wake2[i+wbf_cnt], -1);
                }
            }else{
                wtk_bfio5_wake_start(bfio5->wake2[wbf_cnt], -1);
            }
        }
    }
    if(bfio5->wake3){
        wtk_bfio5_wake_start(bfio5->wake3, -1);
    }
    if(bfio5->qform)
    {
        wtk_qform9_start2(bfio5->qform, 90, 0);
    }else if(bfio5->qform2)
    {
        wtk_qform2_start2(bfio5->qform2, 90, 0);
    }else if(bfio5->qform3){
        wtk_qform3_start2(bfio5->qform3, 90, 0);
    }
    
}

void wtk_bfio5_aec_on_gainnet(wtk_bfio5_edr_t *edr, float *gain, int len, int is_end)
{
	memcpy(edr->g, gain, sizeof(float)*edr->cfg->bankfeat.nb_bands);
}

void wtk_bfio5_set_waked(wtk_bfio5_t *bfio5, float wake_fs, float wake_fe, int bftheta, float wake_prob, int need_notify, int need_ssl, wtk_bfio5_wake_t *wake, int is_end, int is_wake2)
{
    float ns, ne;
    int rate=bfio5->cfg->rate;
    // int i, 
    int cancel;
    int min_idx;
    // float min_thsub, fp;
    wtk_queue_t *stft2_q=&(bfio5->stft2_q);
    wtk_queue_t *edr_stft_q=&(bfio5->edr_stft_q);
    wtk_stft2_msg_t *msg;
    wtk_bfio5_edr_msg_t *msg2;
    wtk_queue_node_t *qn;
    float mssl2_theta=-1;
    float mssl2_2_theta=-1;
    int num_theta=0;
    // int theta_range=bfio5->wbf?bfio5->wbf->theta_range:bfio5->wbf2->theta_range;

    if(is_wake2){
        bfio5->wake_fs=wake_fs+bfio5->wake2_extend;
        bfio5->wake_fe=wake_fe+bfio5->wake2_extend;
        bfio5->wake2_prob=wake_prob;
        bfio5->wake_key=1;
    }else{
        bfio5->wake_fs=wake_fs+bfio5->wake_extend;
        bfio5->wake_fe=wake_fe+bfio5->wake_extend;
        bfio5->wake_prob=wake_prob;
        bfio5->wake_key=0;
    }
    bfio5->out_wake_prob=wake_prob;

    if(need_notify && bfio5->notify)
    {
        if(bfio5->cfg->use_asr && !bfio5->cfg->use_vad){
            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_END, NULL, 0);
        }
        bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE, NULL ,0);
        if(bfio5->wake_key==0){
            bfio5->wake2_ready=1;
        }
        if(bfio5->cfg->use_asr && !bfio5->cfg->use_vad){
            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_START, NULL, 0);
        }
    }

    ns=(bfio5->wake_fs+bfio5->cfg->wake_ssl_fs)*rate;
    ne=(bfio5->wake_fe+bfio5->cfg->wake_ssl_fe)*rate;
    // wtk_debug("%f %f\n",bfio5->cfg->wake_ssl_fs,bfio5->cfg->wake_ssl_fe);
    // wtk_debug("%f %f\n", ns/rate, ne/rate);
    msg=(wtk_stft2_msg_t *)data_offset2(stft2_q->pop, wtk_stft2_msg_t, q_n);
    if (bfio5->cfg->debug) {
        wtk_debug("smsg_s =%.0f wake_ssl_ns=%.0f wake_ssl_ne=%.0f [%f %f]\n",
                  msg->s, ns, ne, wake_fs + bfio5->cfg->wake_ssl_fs,
                  wake_fe + bfio5->cfg->wake_ssl_fe);
    }
    
    if(need_ssl)
    {
        if(bfio5->maskssl2){
            // wtk_debug("%d\n", edr_stft_q->length);
            int *mssl_tmp=bfio5->mssl_tmp;
            mssl2_theta = 0;
            num_theta = 0;
            for(qn=edr_stft_q->pop; qn; qn=qn->next)
            {
                msg2=(wtk_bfio5_edr_msg_t *)data_offset2(qn, wtk_bfio5_edr_msg_t, q_n);
                // wtk_debug("=====%f %f %f\n", msg2->s, ns, ne);
                if(msg2->s >= ns)
                {
                    if(msg2->theta != -1 && num_theta < 10){
                        mssl_tmp[num_theta] = msg2->theta;
                        ++num_theta;
                        // wtk_debug("%f %f %d %d\n", msg2->s, ns, num_theta, msg2->theta);
                    }
                }
                if(msg->s >= ne)
                {
                    break;
                }
            }
            if(num_theta > 0){
                mssl2_theta = wtk_int_mode(mssl_tmp, num_theta);
            }
            if(mssl2_theta >= 70 && mssl2_theta <= 110){
                if(bfio5->maskssl2_2){
                    int *mssl_2_tmp=bfio5->mssl_2_tmp;
                    mssl2_2_theta = 0;
                    num_theta = 0;
                    for(qn=edr_stft_q->pop; qn; qn=qn->next)
                    {
                        msg2=(wtk_bfio5_edr_msg_t *)data_offset2(qn, wtk_bfio5_edr_msg_t, q_n);
                        // wtk_debug("=====%f %f %f\n", msg2->s, ns, ne);
                        if(msg2->s >= ns)
                        {
                            if(msg2->theta != -1 && num_theta < 10){
                                mssl_2_tmp[num_theta] = msg2->theta2;
                                ++num_theta;
                                // wtk_debug("%f %f %d %d\n", msg2->s, ns, num_theta, msg2->theta2);
                            }
                        }
                        if(msg->s >= ne)
                        {
                            break;
                        }
                    }
                    if(num_theta > 0){
                        mssl2_2_theta = wtk_int_mode(mssl_2_tmp, num_theta);
                    }
                    if(mssl2_2_theta < 85){
                        mssl2_theta = 80;
                    }else if(mssl2_2_theta > 95){
                        mssl2_theta = 100;
                    }
                }
            }
        }else{
            wtk_ssl_reset2(bfio5->ssl);
            for(qn=stft2_q->pop; qn; qn=qn->next)
            {
                msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                if(msg->s >= ns)
                {
                    // printf("%f %f %f\n",msg->s,ns,ne);
                    wtk_ssl_feed_stft2msg(bfio5->ssl, msg, 0);
                }
                if(msg->s >= ne)
                {
                    break;
                }
            }
            wtk_ssl_feed_stft2msg(bfio5->ssl, NULL, 1);
            // wtk_ssl_print(bfio5->ssl);
        }
    }
    min_idx=-1;

    if(bfio5->maskssl2){
        if(num_theta > 0){
            min_idx = 0;
        }
    }else{
        if (bfio5->ssl->nbest >= 1) {
            min_idx = 0;
        }
    }

    if (bfio5->cfg->use_asr && need_ssl) {
        if(bfio5->cfg->use_vad){
            if (bfio5->waked == 1) {
                wtk_vad2_feed(bfio5->vad, NULL, 0, 1);
                wtk_vad2_reset(bfio5->vad);
            }
            wtk_vad_set_margin(bfio5->vad->vad, bfio5->cfg->vad_left_margin,
                            bfio5->cfg->vad_right_margin);
            wtk_vad2_start(bfio5->vad);
        }
        bfio5->waked = 1;
    }

    if(min_idx>=0)
    {
        if(bfio5->maskssl2){
            if (abs(bfio5->wake_theta - mssl2_theta) <=
                bfio5->cfg->ressl_range) {  // 与上次定位角度小于等于15度不更新qform
                bfio5->wake_theta=mssl2_theta;
                if (bfio5->cfg->debug) {
                    wtk_debug("wake_theta=bfio5_theta=%d started\n",
                            bfio5->wake_theta);
                }
                bfio5->waked=1;
                if(need_notify && bfio5->notify)
                {
                    if(wake->vwake->asr_res)
                    {
                        //printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                        bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos>>1);
                    }
                    bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_SSL, NULL ,0);
                }

                if (bfio5->cfg->use_asr) {
                    if (bfio5->qform) {
                        wtk_qform9_feed_smsg(bfio5->qform, NULL, 0, 1);
                    } else if(bfio5->qform2){
                        wtk_qform2_feed_smsg(bfio5->qform2, NULL, 0, 1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                    }else if(bfio5->qform3){
                        wtk_qform3_feed_smsg(bfio5->qform3, NULL, 0, 1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                    }
                }
                need_ssl = 0;
            } else {
                bfio5->wake_theta=mssl2_theta;
                if (bfio5->cfg->debug) {
                    wtk_debug("wake_theta=%d\n", bfio5->wake_theta);
                }
                if(need_notify && bfio5->notify)
                {
                    if(wake->vwake->asr_res)
                    {
                        //printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                        bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos>>1);
                    }
                    bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_SSL, NULL ,0);
                }

                if(bfio5->cfg->use_asr)
                {
                    if (bfio5->qform) {
                        wtk_qform9_feed_smsg(bfio5->qform,NULL,0,1);
                        wtk_qform9_reset2(bfio5->qform);
                    } else if(bfio5->qform2){
                        wtk_qform2_feed_smsg(bfio5->qform2,NULL,0,1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                        wtk_qform2_reset2(bfio5->qform2);
                    }else if(bfio5->qform3){
                        wtk_qform3_feed_smsg(bfio5->qform3,NULL,0,1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                        wtk_qform3_reset2(bfio5->qform3);
                    }

                    if(bfio5->qform)
                    {
                        wtk_qform9_start2(bfio5->qform,bfio5->wake_theta,0);
                    }else if(bfio5->qform2)
                    {
                        wtk_qform2_start2(bfio5->qform2,bfio5->wake_theta,0);
                    }else if(bfio5->qform3){
                        wtk_qform3_start2(bfio5->qform3,bfio5->wake_theta,0);
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
                            cancel = bfio5->vad_output - msg->s;
                            if (bfio5->cfg->debug) {
                                wtk_debug("out %.0f, msg->s %.0f cancel %d\n",
                                        bfio5->vad_output, msg->s, cancel);
                            }
                            if (cancel > 0) {
                                if (bfio5->notify) {
                                    bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_CANCEL,
                                                NULL, cancel);
                                }
                                bfio5->vad_output -= cancel;
                            }

                        }
                    }
                }
            }
        }else{
            if (abs(bfio5->wake_theta - bfio5->ssl->nbest_extp[min_idx].theta) <=
                bfio5->cfg->ressl_range) {  // 与上次定位角度小于等于15度不更新qform
                bfio5->wake_theta=bfio5->ssl->nbest_extp[min_idx].theta;
                if (bfio5->cfg->debug) {
                    wtk_debug("wake_theta=bfio5_theta=%d started\n",
                            bfio5->wake_theta);
                }
                bfio5->waked=1;
                if(need_notify && bfio5->notify)
                {
                    if(wake->vwake->asr_res)
                    {
                        //printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                        bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos>>1);
                    }
                    bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_SSL, NULL ,0);
                }

                if (bfio5->cfg->use_asr) {
                    if (bfio5->qform) {
                        wtk_qform9_feed_smsg(bfio5->qform, NULL, 0, 1);
                    } else if(bfio5->qform2){
                        wtk_qform2_feed_smsg(bfio5->qform2, NULL, 0, 1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                    }else if(bfio5->qform3){
                        wtk_qform3_feed_smsg(bfio5->qform3, NULL, 0, 1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                    }
                }
                need_ssl = 0;
            } else {
                bfio5->wake_theta=bfio5->ssl->nbest_extp[min_idx].theta;
                if (bfio5->cfg->debug) {
                    wtk_debug("wake_theta=%d\n", bfio5->wake_theta);
                }
                if(need_notify && bfio5->notify)
                {
                    if(wake->vwake->asr_res)
                    {
                        //printf("%.*s\n",wake->vwake->asr_res->pos,wake->vwake->asr_res->data);
                        bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_RES, (short *)(wake->vwake->asr_res->data), wake->vwake->asr_res->pos>>1);
                    }
                    bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE_SSL, NULL ,0);
                }

                if(bfio5->cfg->use_asr)
                {
                    if (bfio5->qform) {
                        wtk_qform9_feed_smsg(bfio5->qform,NULL,0,1);
                        wtk_qform9_reset2(bfio5->qform);
                    } else if(bfio5->qform2){
                        wtk_qform2_feed_smsg(bfio5->qform2,NULL,0,1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                        wtk_qform2_reset2(bfio5->qform2);
                    }else if(bfio5->qform3){
                        wtk_qform3_feed_smsg(bfio5->qform3,NULL,0,1);
                        wtk_bfio5_on_qform(bfio5, NULL, 1);
                        wtk_qform3_reset2(bfio5->qform3);
                    }

                    if(bfio5->qform)
                    {
                        wtk_qform9_start2(bfio5->qform,bfio5->wake_theta,0);
                    }else if(bfio5->qform2)
                    {
                        wtk_qform2_start2(bfio5->qform2,bfio5->wake_theta,0);
                    }else if(bfio5->qform3){
                        wtk_qform3_start2(bfio5->qform3,bfio5->wake_theta,0);
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
                            cancel = bfio5->vad_output - msg->s;
                            if (bfio5->cfg->debug) {
                                wtk_debug("out %.0f, msg->s %.0f cancel %d\n",
                                        bfio5->vad_output, msg->s, cancel);
                            }
                            if (cancel > 0) {
                                if (bfio5->notify) {
                                    bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_CANCEL,
                                                NULL, cancel);
                                }
                                bfio5->vad_output -= cancel;
                            }

                        }
                    }
                }
            }
        }
    }else
    {
        if (bfio5->cfg->debug) {
            wtk_debug("[%f %f] waked ssl nbest has no theta == bftheta\n",
                      wake_fs, wake_fe);
        }
    }

    if (bfio5->cfg->use_asr && need_ssl && !is_end) {

        // ne = wake_fe * rate;
        for (qn = stft2_q->pop; qn; qn = qn->next) {
            msg = (wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
            if (msg->s >= ne) {
                break;
            }
        }

        if (msg->s >= ne) {
            cancel = bfio5->vad_output - msg->s;
            if (bfio5->cfg->debug) {
                wtk_debug("out %.0f, msg->s %.0f cancel %d\n", bfio5->vad_output,
                          msg->s, cancel);
            }
            if (cancel > 0) {
                if (bfio5->notify) {
                    bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_CANCEL, NULL, cancel);
                }
                bfio5->vad_output -= cancel;
            }

            for (; qn; qn = qn->next) {
                msg = (wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                if (bfio5->qform) {
                    wtk_qform9_feed_smsg(bfio5->qform, msg, 0, 0);
                } else if(bfio5->qform2){
                    wtk_qform2_feed_smsg(bfio5->qform2, msg, 0, 0);
                    wtk_bfio5_on_qform(bfio5, bfio5->qform2->out, 0);
                }else if(bfio5->qform3){
                    wtk_qform3_feed_smsg(bfio5->qform3, msg, 0, 0);
                    wtk_bfio5_on_qform(bfio5, bfio5->qform3->out, 0);
                }
            }
        }
    }
}

static float wtk_bfio5_de_energy(short *p,int n)
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

float wtk_bfio5_entropy(wtk_bfio5_t *bfio5, wtk_complex_t **fftx, int chn)
{
    int rate = bfio5->cfg->rate;
    int wins = bfio5->stft2->cfg->win;
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

int wtk_bfio5_feed_low_trick(wtk_bfio5_t *bfio5, wtk_stft2_msg_t *msg, int chn, int is_end)
{
    int low_fs_idx=bfio5->cfg->low_fs_idx;
    int low_fe_idx=bfio5->cfg->low_fe_idx;
    float low_thresh=bfio5->cfg->low_thresh;
    int i;
    int nbin=bfio5->stft2->nbin;
    float low=0;

    if(is_end){
        low = bfio5->sum_low/bfio5->low_cnt;
        bfio5->sum_low=0;
        bfio5->low_cnt=0;
        // printf("%f\n", low);
        if(low>low_thresh){
            return 1;
        }
        return 0;
    }
    if(msg){
        wtk_complex_t **fft=msg->fft;
        float entropy=wtk_bfio5_entropy(bfio5, fft, chn);
        for(i=0;i<nbin;++i){
            if(i>=low_fs_idx && i<=low_fe_idx){
                low+=fft[i][chn].a * fft[i][chn].a + fft[i][chn].b * fft[i][chn].b;
            }
        }
        if(entropy<3){
            bfio5->sum_low+=low;
            ++bfio5->low_cnt;
            // printf("%f\n", low);
        }
    }
    return 0;
}
static void _bfio5_waked_post(wtk_bfio5_t *bfio5, int is_end) {
    float max_wake_prob = -50;
    int max_idx = -1;
    wtk_bfio5_wake_t **wake = bfio5->wake;
    wtk_bfio5_wake_t **wake2 = bfio5->wake2;
    float wake_prob = bfio5->wake_prob;
    int is_wake2=0;
    int i;
    float dup_time;
    int offset;
    int energy_len;
    float energy=0;
    float energy_conf=bfio5->cfg->energy_conf;
    short *de;

    if(bfio5->cfg->use_kvadwake){
        for (i = 0; i < bfio5->wake_cnt; ++i) {
            if (wake[i]->waked == 1) {
                if (bfio5->cfg->debug) {
                    wtk_debug("wake[%d] bftheta=%d wake_prob=%f wake_ts=[%f %f]\n",
                            i, wake[i]->theta, wake[i]->wake_prob,
                            wake[i]->wake_fs, wake[i]->wake_fe);
                }
                if(bfio5->cfg->use_en_trick){
                    offset=max(0,floor(wake[i]->wake_fs*16000-bfio5->de_fs));
                    // printf("%d %d %d\n", offset, energy_len, bfio5->de_wake_buf[i]->pos);
                    energy_len=floor((wake[i]->wake_fe-wake[i]->wake_fs)*16000);
                    de=(short *)bfio5->de_wake_buf[i]->data;
                    if(bfio5->de_wake_buf[i]->pos>0){
                        energy=wtk_bfio5_de_energy(de+offset, max(min(energy_len, bfio5->de_wake_buf[i]->pos/2-offset), 0));
                        // printf("idx=%d energy=%.12f %f %f conf=%f\n", i, energy, wake[i]->wake_fs, wake[i]->wake_fe, energy_conf);
                        if(energy<energy_conf){
                            continue;
                        }
                    }
                }
                if(bfio5->cfg->use_low_trick){
                    wtk_queue_t *stft2_q=&(bfio5->stft2_q);
                    wtk_stft2_msg_t *msg;
                    wtk_queue_node_t *qn;
                    int rate=bfio5->cfg->rate;
                    float ns;
                    float ne;
                    int ret=0;
                    // ns=(wake[i]->wake_fs+bfio5->cfg->wake_ssl_fs)*rate;
                    // ne=(wake[i]->wake_fe+bfio5->cfg->wake_ssl_fe)*rate;
                    ns=wake[i]->wake_fs*rate;
                    ne=wake[i]->wake_fe*rate;
                    for(qn=stft2_q->pop; qn; qn=qn->next)
                    {
                        msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                        if(msg->s >= ns)
                        {
                            // printf("%f %f %f\n",msg->s,ns,ne);
                            wtk_bfio5_feed_low_trick(bfio5, msg, i, 0);
                        }
                        if(msg->s >= ne)
                        {
                            break;
                        }
                    }
                    ret = wtk_bfio5_feed_low_trick(bfio5, NULL, i, 1);
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
        if(bfio5->cfg->use_aec_wake && max_idx > -1){
            short null_buf[16000]={0};
            // wtk_debug("%f %f\n", bfio5->aec_fs, bfio5->aec_fe);
            offset=max(0,floor((wake[max_idx]->wake_fs+bfio5->cfg->aec_wake_fs)*16000-bfio5->aec_fs));
            // printf("%d %d %d\n", offset, energy_len, bfio5->de_wake_buf[i]->pos);
            energy_len=max(min(floor((wake[max_idx]->wake_fe-wake[max_idx]->wake_fs-bfio5->cfg->aec_wake_fs+bfio5->cfg->aec_wake_fe)*16000), bfio5->aec_wake_buf->pos/2-offset), 0);
            de=(short *)bfio5->aec_wake_buf->data;
            if(bfio5->aec_wake_buf->pos>0){
                energy=wtk_bfio5_de_energy(de+offset, energy_len);
                if(energy>energy_conf){
                    bfio5->wake3->waked = 0;
                    wtk_kvadwake_start(bfio5->wake3->vwake);
                    wtk_kvadwake_feed(bfio5->wake3->vwake, null_buf, 16000, is_end);
                    wtk_kvadwake_feed(bfio5->wake3->vwake, de+offset, energy_len, is_end);
                    wtk_kvadwake_feed(bfio5->wake3->vwake, NULL, 0, 1);
                    wtk_kvadwake_reset(bfio5->wake3->vwake);
                    if(bfio5->wake3->waked){
                        max_idx = -1;
                        max_wake_prob = -50;
                    }
                }
            }
        }
    }

    if(bfio5->cfg->use_kvadwake2){
        if(max_idx == -1){
            for (i = 0; i < bfio5->wake_cnt; ++i) {
                if (wake2[i]->waked == 1) {
                    if (bfio5->cfg->debug) {
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
        if (bfio5->cfg->use_gdenoise && max_idx == bfio5->wake_cnt - 1) {
            dup_time = bfio5->cfg->gd_dup_time;
        } else {
            dup_time = bfio5->cfg->dup_time;
        }
        if(is_wake2){
            wake=bfio5->wake2;
            wake_prob=bfio5->wake2_prob;
        }
        if (fabs(wake[max_idx]->wake_fe - bfio5->wake_fe) < dup_time ||
            fabs(wake[max_idx]->wake_fs - bfio5->wake_fs) < dup_time) {
            if (wake[max_idx]->wake_prob > wake_prob) {
                if (bfio5->cfg->debug) {
                    wtk_debug("change waked [%f %f] to [%f %f ], wake[%d] "
                              "bftheta=%d wake_prob=%f  lst_wakeprob=%f\n",
                              bfio5->wake_fs, bfio5->wake_fe,
                              wake[max_idx]->wake_fs, wake[max_idx]->wake_fe,
                              max_idx, wake[max_idx]->theta,
                              wake[max_idx]->wake_prob, wake_prob);
                }
                wtk_bfio5_set_waked(bfio5, wake[max_idx]->wake_fs,
                                   wake[max_idx]->wake_fe, wake[max_idx]->theta,
                                   wake[max_idx]->wake_prob, 0, 0,
                                   wake[max_idx], is_end, is_wake2);
            } else {
                if (bfio5->cfg->debug) {
                    wtk_debug("waked over [%f %f] pop [%f %f]\n", bfio5->wake_fs,
                              bfio5->wake_fe, wake[max_idx]->wake_fs,
                              wake[max_idx]->wake_fe);
                }
            }
        } else {
            wtk_bfio5_set_waked(bfio5, wake[max_idx]->wake_fs,
                               wake[max_idx]->wake_fe, wake[max_idx]->theta,
                               wake[max_idx]->wake_prob, 1, 1, wake[max_idx], is_end, is_wake2);
        }
    }
}

void wtk_bfio5_on_wbf(wtk_bfio5_t *bfio5,wtk_complex_t *wbf_out,int idx,int is_end)
{
    float *data=bfio5->stft2->output;
    short *pv=NULL;
    int k=0;
    int i;
    float scale=bfio5->cfg->wake_scale;

    if(wbf_out)
    {
        k=wtk_stft2_output_ifft(bfio5->stft2,wbf_out,data,bfio5->wbf?bfio5->wbf->bf[idx]->pad:(bfio5->wbf2->qform3?bfio5->wbf2->qform3[idx]->pad:bfio5->wbf2->qform2[idx]->pad),bfio5->end_pos,bfio5->is_end);
        if(bfio5->cfg->use_preemph)
        {
            bfio5->memX[idx]=wtk_preemph_asis2(data,k,bfio5->memX[idx]);
        }

        pv=(short *)data;
        for(i=0;i<k;++i)
        {
            pv[i] = QTK_SSAT16f(data[i]*scale);
        }
    }
#ifdef USE_LOG
	static wtk_wavfile_t *log = NULL;

    if(bfio5->wake[idx]->theta==90)
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

#ifdef USE_LOG_BFIO5

    int wbf_cnt=bfio5->wbf? bfio5->wbf->cfg->wbf_cnt:bfio5->wbf2->cfg->wbf2_cnt;

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

    if(bfio5->cfg->use_kvadwake){
        if(bfio5->cfg->use_en_trick){
            wtk_strbuf_t *de_wake_buf=bfio5->de_wake_buf[idx];
            int de_len;
            int de_wake_len=bfio5->cfg->de_wake_len;

            wtk_strbuf_push(de_wake_buf, (char *)pv, k*sizeof(short));

            de_len=de_wake_buf->pos/sizeof(short);
            
            if(de_len>de_wake_len){
                wtk_strbuf_pop(de_wake_buf, NULL, (de_len-de_wake_len)*sizeof(short));
            }
            if(idx==0){
                bfio5->de_fe+=k;
                bfio5->de_fs=max(0,bfio5->de_fe-de_wake_len);
            }
            // wtk_debug("%f %f\n", bfio5->de_fs, bfio5->de_fe);
        }
        wtk_kvadwake_feed(bfio5->wake[idx]->vwake, pv, k, is_end);
    }
    // if(bfio5->cfg->use_kvadwake2){
    //     wtk_kvadwake_feed(bfio5->wake2[idx]->vwake, pv, k, is_end);
    // }
}

void wtk_bfio5_on_gdenoise(wtk_bfio5_t *bfio5, wtk_complex_t *gd_out, int is_end) {
    int wbf_cnt =
        bfio5->wbf ? bfio5->wbf->cfg->wbf_cnt : bfio5->wbf2->cfg->wbf2_cnt;
    float *data = bfio5->stft2->output;
    float *pad = bfio5->gdenoise->pad;
    short *pv = NULL;
    int k = 0;
    int i;
    float scale=bfio5->cfg->wake_scale;

    if (gd_out) {
        k = wtk_stft2_output_ifft(bfio5->stft2, gd_out, data, pad, bfio5->end_pos,
                                  bfio5->is_end);
        if (bfio5->cfg->use_preemph) {
            if(bfio5->cfg->use_all_raw_audio){
                bfio5->memX[wbf_cnt + bfio5->channel] =
                    wtk_preemph_asis2(data, k, bfio5->memX[wbf_cnt + bfio5->channel]);
            }else{
                bfio5->memX[wbf_cnt + 1] =
                    wtk_preemph_asis2(data, k, bfio5->memX[wbf_cnt + 1]);
            }
        }

        pv = (short *)data;
        for (i = 0; i < k; ++i) {
            pv[i] = QTK_SSAT16f(data[i]*scale);
        }
    }
#ifdef USE_LOG_BFIO5
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
    if(bfio5->cfg->use_all_raw_audio){
        if(bfio5->cfg->use_kvadwake){
            wtk_kvadwake_feed(bfio5->wake[wbf_cnt + bfio5->channel]->vwake, pv, k, is_end);
        }
        if(bfio5->cfg->use_kvadwake2){
            wtk_kvadwake_feed(bfio5->wake2[wbf_cnt + bfio5->channel]->vwake, pv, k, is_end);
        }
    }else{
        if(bfio5->cfg->use_kvadwake){
            wtk_kvadwake_feed(bfio5->wake[wbf_cnt + 1]->vwake, pv, k, is_end);
        }
        if(bfio5->cfg->use_kvadwake2){
            wtk_kvadwake_feed(bfio5->wake2[wbf_cnt + 1]->vwake, pv, k, is_end);
        }
    }
}

void wtk_bfio5_on_wake(wtk_bfio5_wake_t *wake,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len)
{
    if(cmd==WTK_KVADWAKE_WAKE)
    {
        wake->waked=1;
        wake->wake_fs=fs;
        wake->wake_fe=fe;
        wake->wake_prob=wtk_kvadwake_get_conf(wake->vwake);
    }
}

void wtk_bfio5_on_wake2(wtk_bfio5_t *bfio5,wtk_kvadwake_cmd_t cmd,float fs,float fe,short *data, int len)
{
    if(cmd==WTK_KVADWAKE_WAKE2){
        if(bfio5->notify && bfio5->wake2_ready){
            bfio5->notify(bfio5->ths, WTK_BFIO5_WAKE2, NULL, 0);
            bfio5->wake2_ready=0;
        }
    }
}

static void wtk_bfio5_on_qform(wtk_bfio5_t *bfio5, wtk_complex_t *qf_out,
                              int is_end) {
    float *data=bfio5->stft2->output;
    short *pv=NULL;
    int k=0;
    int i;
    float *pad = NULL;
    int ifft_is_end=bfio5->is_end;
    float scale=bfio5->cfg->wake_scale;

    if(bfio5->qform){
        pad = bfio5->qform->bf->pad;
        if(bfio5->qform->cfg->use_qenvelope){
            ifft_is_end = is_end;
        }
    }else if(bfio5->qform2){
        pad = bfio5->qform2->pad;
    }else if(bfio5->qform3){
        pad = bfio5->qform3->pad;
    }

    if(qf_out)
    {
        k = wtk_stft2_output_ifft(bfio5->stft2, qf_out, data, pad, bfio5->end_pos,
                                  ifft_is_end);
        if(bfio5->cfg->use_preemph)
        {
            bfio5->memX[bfio5->wake_cnt]=wtk_preemph_asis2(data,k,bfio5->memX[bfio5->wake_cnt]);
        }

        pv=(short *)data;
        for(i=0;i<k;++i)
        {
            pv[i] = QTK_SSAT16f(data[i]*scale);
        }
    }
    if(bfio5->reg_bf){
        if(bfio5->notify)
        {
            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_DATA, pv, k);
        }
        if(is_end){
            wtk_bfio5_reset(bfio5);
        }
    }else{
        if(bfio5->waked==1){
            bfio5->sil_end=0;
        }
        if(bfio5->cfg->use_one_shot){
            bfio5->sil_ts=bfio5->vad_output/bfio5->cfg->rate-bfio5->wake_fe;
            if(bfio5->sil_ts >= bfio5->cfg->sil_delay*0.001 && bfio5->waked==1 && bfio5->vad_ts <= bfio5->wake_fe){
                if(bfio5->cfg->use_vad){
                    bfio5->notify(bfio5->ths, WTK_BFIO5_SIL_END, NULL, bfio5->vad_output);
                    // wtk_debug("sil_ts=[%f], vad_ts/te=[%f/%f] wake_fs/fe=[%f/%f] val=[%f]\n", bfio5->sil_ts, bfio5->vad_ts, bfio5->vad_te, bfio5->wake_fs, bfio5->wake_fe, bfio5->vad_output/bfio5->cfg->rate);
                    bfio5->waked=0;
                    bfio5->sil_end=1;
                    wtk_vad2_feed(bfio5->vad, NULL, 0, 1);
                    wtk_vad2_reset(bfio5->vad);
                    wtk_vad2_start(bfio5->vad);
                }
            }
            bfio5->speech_ts=bfio5->vad_output/bfio5->cfg->rate-bfio5->wake_fe;
            if(bfio5->speech_ts >= bfio5->cfg->speech_delay*0.001 && (bfio5->speech_ts <= bfio5->cfg->speech_delay*0.001 + k*1.0/bfio5->cfg->rate) && bfio5->sil_end==0){
                bfio5->notify(bfio5->ths, WTK_BFIO5_SPEECH_END, NULL, bfio5->vad_output);
                // wtk_debug("speech_ts=[%f], vad_ts/te=[%f/%f] wake_fs/fe=[%f/%f] val=[%f]\n", bfio5->sil_ts, bfio5->vad_ts, bfio5->vad_te, bfio5->wake_fs, bfio5->wake_fe, bfio5->vad_output/bfio5->cfg->rate);
                if(bfio5->waked==1){
                    if(bfio5->cfg->use_vad){
                        bfio5->waked=0;
                        wtk_vad2_feed(bfio5->vad, NULL, 0, 1);
                        wtk_vad2_reset(bfio5->vad);
                        wtk_vad2_start(bfio5->vad);
                    }else{
                        if(bfio5->notify){
                            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_END, NULL, 0);
                        }
                    }
                }
                bfio5->sil_end=1;
            }
        }

        if(bfio5->waked==1 && !is_end && bfio5->cfg->use_vad)
        {
            // wtk_debug("vad=[%f]  wake_fs/fe=[%f/%f]\n", bfio5->vad_output/bfio5->cfg->rate,  bfio5->wake_fs, bfio5->wake_fe);
            wtk_vad2_feed(bfio5->vad, (char *)pv, k<<1, is_end);
        }else
        {
            bfio5->vad_output+=k;
            if(bfio5->notify)
            {
                bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_DATA, pv, k);
            }
        }
        if (is_end) {
            if(bfio5->cfg->use_vad){
                wtk_vad2_feed(bfio5->vad, NULL, 0, 1);
                wtk_vad2_reset(bfio5->vad);
                wtk_vad2_start(bfio5->vad);
            }
        }
    }
}

void wtk_bfio5_on_vad(wtk_bfio5_t *bfio5,wtk_vad2_cmd_t cmd,short *data,int len)
{
    float pos=0;
    float ne;
    if (bfio5->cfg->use_offline_asr) {
        wtk_string_t rec_res;
        switch(cmd) {
        case WTK_VAD2_START:
            bfio5->vad_ts = bfio5->vad_output / bfio5->cfg->rate;
            qtk_decoder_wrapper_start(bfio5->decoder);
            bfio5->asr = 1;
            break;
        case WTK_VAD2_DATA:
            bfio5->vad_output += len;
            if (bfio5->asr == 1) {
                qtk_decoder_wrapper_feed(bfio5->decoder, (char *)data, len << 1, 0);
            }
            if (bfio5->notify) {
                bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_DATA, data, len);
            }
            break;
        case WTK_VAD2_END:
            bfio5->vad_te = bfio5->vad_output / bfio5->cfg->rate;
            qtk_decoder_wrapper_feed(bfio5->decoder, NULL, 0, 1);
            qtk_decoder_wrapper_get_result(bfio5->decoder, &rec_res);
            if(bfio5->notify) {
                bfio5->notify(bfio5->ths, WTK_BFIO5_ASR_RES, (short *)(rec_res.data), rec_res.len);
            }
            qtk_decoder_wrapper_reset(bfio5->decoder);
            bfio5->asr = 0;
            break;
        case WTK_VAD2_CANCEL:
            bfio5->vad_output -= len;
            break;
        }
        return ;
    }

	switch(cmd)
    {
    case WTK_VAD2_START:
        bfio5->vad_ts=bfio5->vad_output/bfio5->cfg->rate;
        if(bfio5->notify)
        {
            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_START, NULL, 0);
        }
        // sil=0;
        break;
    case WTK_VAD2_DATA:
        bfio5->vad_output+=len;
        // if(sil==0)
        // {
        pos = bfio5->vad_output/bfio5->cfg->rate - bfio5->wake_fe - bfio5->cfg->speech_delay*0.001;
        if(pos>0 && bfio5->cfg->use_one_shot){  // 有语音时长强制截断
            // wtk_debug("vad=[%f]  wake_fs/fe=[%f/%f]\n", bfio5->vad_output/bfio5->cfg->rate,  bfio5->wake_fs, bfio5->wake_fe);
            // wtk_debug("===========pos=%f\n", pos/bfio5->cfg->rate);
            // bfio5->notify(bfio5->ths, WTK_BFIO5_SPEECH_END, NULL, len);
            if(bfio5->notify)
            {
                bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_END, NULL, len);
            }
            // wtk_debug("%d\n", bfio5->waked);
            bfio5->waked = 0;
        }
        if (bfio5->notify) {
            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_DATA, data, len);
        }
        // }
        break;
    case WTK_VAD2_END:
        bfio5->vad_te=bfio5->vad_output/bfio5->cfg->rate;
        // wtk_debug("vad_ts/te=[%f/%f] wake_fs/fe=[%f/%f] pos=%f\n", bfio5->vad_ts, bfio5->vad_te, bfio5->wake_fs, bfio5->wake_fe, pos);
        ne=(bfio5->wake_fe+bfio5->cfg->wake_ssl_fe);
        if(bfio5->notify)
        {
            if(bfio5->vad_ts <= ne){ // 删除唤醒词
                pos = (ne-bfio5->vad_ts)*bfio5->cfg->rate;
                // bfio5->notify(bfio5->ths, WTK_BFIO5_ASR_CANCEL, NULL, pos);
            }
            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_END, NULL, len);
            // wtk_debug("%d\n", bfio5->waked);
        }
        // sil=1;
        break;
    case WTK_VAD2_CANCEL:
        bfio5->vad_output-=len;
        if(bfio5->notify)
        {
            bfio5->notify(bfio5->ths, WTK_BFIO5_VAD_CANCEL, NULL, len);
        }
        break;
    }
}

void wtk_bfio5_reg_bf(wtk_bfio5_t *bfio5, wtk_stft2_msg_t *msg, int pos, int is_end)
{
    wtk_queue_node_t *qn;
    wtk_queue_t *stft2_q=&(bfio5->stft2_q);
    wtk_stft2_msg_t *msg2;
    wtk_stft2_msg_t *msg3;
    int max_theta=bfio5->cfg->use_line?180:359;
    int theta=0;


    if(bfio5->reg_theta >= 0 && bfio5->reg_theta <= max_theta){
        if(is_end){
            if (bfio5->qform) {
                wtk_qform9_feed_smsg(bfio5->qform,NULL,0,1);
                wtk_qform9_reset2(bfio5->qform);
            } else if(bfio5->qform2){
                wtk_qform2_feed_smsg(bfio5->qform2,NULL,0,1);
                wtk_bfio5_on_qform(bfio5, NULL, 1);
                wtk_qform2_reset2(bfio5->qform2);
            }else if(bfio5->qform3){
                wtk_qform3_feed_smsg(bfio5->qform3,NULL,0,1);
                wtk_bfio5_on_qform(bfio5, NULL, 1);
                wtk_qform3_reset2(bfio5->qform3);
            }
        }else{
            if(bfio5->qform)
            {
                wtk_qform9_feed_smsg(bfio5->qform, msg, pos, is_end);
            }else if(bfio5->qform2)
            {
                wtk_qform2_feed_smsg(bfio5->qform2, msg, pos ,is_end);
                wtk_bfio5_on_qform(bfio5, msg ? bfio5->qform2->out : NULL, is_end);
            }else if(bfio5->qform3){
                wtk_qform3_feed_smsg(bfio5->qform3, msg, pos ,is_end);
                wtk_bfio5_on_qform(bfio5, msg ? bfio5->qform3->out : NULL, is_end);
            }
        }
    }else if(bfio5->reg_tms>0){
        if(is_end){
            if (bfio5->qform) {
                wtk_qform9_feed_smsg(bfio5->qform,NULL,0,1);
                wtk_qform9_reset2(bfio5->qform);
            } else if(bfio5->qform2){
                wtk_qform2_feed_smsg(bfio5->qform2,NULL,0,1);
                wtk_bfio5_on_qform(bfio5, NULL, 1);
                wtk_qform2_reset2(bfio5->qform2);
            }else if(bfio5->qform3){
                wtk_qform3_feed_smsg(bfio5->qform3,NULL,0,1);
                wtk_bfio5_on_qform(bfio5, NULL, 1);
                wtk_qform3_reset2(bfio5->qform3);
            }
        }else{
            if(bfio5->reg_end){
                if(bfio5->qform)
                {
                    wtk_qform9_feed_smsg(bfio5->qform, msg, pos, is_end);
                }else if(bfio5->qform2)
                {
                    wtk_qform2_feed_smsg(bfio5->qform2, msg, pos ,is_end);
                    wtk_bfio5_on_qform(bfio5, msg ? bfio5->qform2->out : NULL, is_end);
                }else if(bfio5->qform3){
                    wtk_qform3_feed_smsg(bfio5->qform3, msg, pos ,is_end);
                    wtk_bfio5_on_qform(bfio5, msg ? bfio5->qform3->out : NULL, is_end);
                }
            }else{
                if(msg)
                {
                    wtk_queue_push(stft2_q, &(msg->q_n));
                }
                if(msg->s>bfio5->reg_tms){
                    wtk_ssl_feed_stft2msg(bfio5->ssl, NULL, 1);
                    wtk_ssl_print(bfio5->ssl);
                    theta=bfio5->ssl->nbest_extp[0].theta;
                    // wtk_debug("==========%f %d\n", msg->s, bfio5->reg_tms);
                    wtk_ssl_reset2(bfio5->ssl);
                    bfio5->reg_end=1;
                    if(bfio5->qform)
                    {
                        wtk_qform9_start2(bfio5->qform,theta,0);
                    }else if(bfio5->qform2)
                    {
                        wtk_qform2_start2(bfio5->qform2,theta,0);
                    }else if(bfio5->qform3){
                        wtk_qform3_start2(bfio5->qform3,theta,0);
                    }
                    for(qn=stft2_q->pop; qn; qn=qn->next)
                    {
                        msg3=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                        if(bfio5->qform)
                        {
                            wtk_qform9_feed_smsg(bfio5->qform, msg3, pos, 0);
                        }else if(bfio5->qform2)
                        {
                            wtk_qform2_feed_smsg(bfio5->qform2, msg3, pos ,0);
                            wtk_bfio5_on_qform(bfio5, bfio5->qform2->out, 0);
                        }else if(bfio5->qform3){
                            wtk_qform3_feed_smsg(bfio5->qform3, msg3, pos ,0);
                            wtk_bfio5_on_qform(bfio5, bfio5->qform3->out, 0);
                        }
                    }
                    
                    while(stft2_q->length>0)
                    {
                        qn=wtk_queue_pop(stft2_q);
                        msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                        wtk_stft2_push_msg(bfio5->stft2, msg2);
                    }
                }else{
                    wtk_ssl_feed_stft2msg(bfio5->ssl, msg, 0);
                }
            }
        }

    }else{
        if(is_end){
            wtk_ssl_feed_stft2msg(bfio5->ssl, NULL, 1);
            wtk_ssl_print(bfio5->ssl);
            theta=bfio5->ssl->nbest_extp[0].theta;
            wtk_ssl_reset2(bfio5->ssl);

            if(bfio5->qform)
            {
                wtk_qform9_start2(bfio5->qform,theta,0);
            }else if(bfio5->qform2)
            {
                wtk_qform2_start2(bfio5->qform2,theta,0);
            }else if(bfio5->qform3){
                wtk_qform3_start2(bfio5->qform3,theta,0);
            }
            
            for(qn=stft2_q->pop; qn; qn=qn->next)
            {
                msg3=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                if(bfio5->qform)
                {
                    wtk_qform9_feed_smsg(bfio5->qform, msg3, pos, 0);
                }else if(bfio5->qform2)
                {
                    wtk_qform2_feed_smsg(bfio5->qform2, msg3, pos ,0);
                    wtk_bfio5_on_qform(bfio5, bfio5->qform2->out, 0);
                }else if(bfio5->qform3){
                    wtk_qform3_feed_smsg(bfio5->qform3, msg3, pos ,0);
                    wtk_bfio5_on_qform(bfio5, bfio5->qform3->out, 0);
                }
            }
            
            while(stft2_q->length>0)
            {
                qn=wtk_queue_pop(stft2_q);
                msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                wtk_stft2_push_msg(bfio5->stft2, msg2);
            }

            if (bfio5->qform) {
                wtk_qform9_feed_smsg(bfio5->qform,NULL,0,1);
                wtk_qform9_reset2(bfio5->qform);
            } else if(bfio5->qform2){
                wtk_qform2_feed_smsg(bfio5->qform2,NULL,0,1);
                wtk_bfio5_on_qform(bfio5, NULL, 1);
                wtk_qform2_reset2(bfio5->qform2);
            }else if(bfio5->qform3){
                wtk_qform3_feed_smsg(bfio5->qform3,NULL,0,1);
                wtk_bfio5_on_qform(bfio5, NULL, 1);
                wtk_qform3_reset2(bfio5->qform3);
            }
        }else{
            if(msg)
            {
                wtk_queue_push(stft2_q, &(msg->q_n));
            }
            wtk_ssl_feed_stft2msg(bfio5->ssl, msg, 0);
        }
    }
}

void wtk_bfio5_edr_feed_wake(wtk_bfio5_t *bfio5, wtk_complex_t **stft2_out, int is_end) {
    float *data = bfio5->stft2_2->output;
    short *pv = NULL;
    int k = 0;
    int i, n;
    int channel2=bfio5->channel2;
    float **pad = bfio5->mic2_pad;
    int wake_cnt = bfio5->wake_cnt;
    float scale=bfio5->cfg->wake_scale;

    for(i=0;i<channel2;++i){
        bfio5->wake[wake_cnt-channel2+i]->waked=0;

        k = wtk_stft2_output_ifft(bfio5->stft2_2, stft2_out[i], data, pad[i], bfio5->end_pos,bfio5->is_end);
        bfio5->memX[wake_cnt-channel2+i] = wtk_preemph_asis2(data, k, bfio5->memX[wake_cnt-channel2+i]);
        pv = (short *)data;
        for (n = 0; n < k; ++n) {
            pv[n] = QTK_SSAT16f(data[n]*scale);
        }
#ifdef USE_LOG_BFIO5
        int j;
        static wtk_wavfile_t *mic2_log[8];
        for(j=0;j < bfio5->channel2;++j){
            if(i==j)
            {
                if(!mic2_log[j])
                {
                    mic2_log[j]=wtk_wavfile_new(16000);
                    wtk_wavfile_open2(mic2_log[j],"wbf/wbf.mic2.");
                    mic2_log[j]->max_pend=0;
                }
                if(k>0)
                {
                    wtk_wavfile_write(mic2_log[j],(char *)pv,k<<1);
                }
                if(is_end)
                {
                    wtk_debug("============ close ============\n");
                    wtk_wavfile_close(mic2_log[j]);
                    wtk_wavfile_delete(mic2_log[j]);
                    mic2_log[j]=NULL;
                }
            }
        }
#endif
        if(bfio5->cfg->use_kvadwake){
            wtk_kvadwake_feed(bfio5->wake[wake_cnt-channel2+i]->vwake, pv, k, is_end);
        }
        if(bfio5->cfg->use_kvadwake2){
            wtk_kvadwake_feed(bfio5->wake2[wake_cnt-channel2+i]->vwake, pv, k, is_end);
        }
    }
}

void wtk_bfio5_feed_edr_model(wtk_bfio5_t *bfio5, wtk_bfio5_edr_msg_t *edr_msg, wtk_complex_t *fftx, wtk_complex_t *ffty, int usecohv)
{
    wtk_bfio5_edr_t *edr=bfio5->edr;
	int i;
	int nbin=edr->nbin;
	float *g=edr->g, *gf=edr->gf;
	wtk_gainnet2_t *gainnet2=edr->gainnet2;
	wtk_bankfeat_t *bank_mic=edr->bank_mic;
	wtk_bankfeat_t *bank_sp=edr->bank_sp;
	int featsp_lm=edr->cfg->featsp_lm;
	int featm_lm=edr->cfg->featm_lm;
	float *feature_sp=edr->feature_sp;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=edr->qmmse;
	float *qmmse_gain;
	wtk_complex_t *fftytmp, sed, *fftxtmp;
	float ef,yf;
	float leak;

	wtk_bankfeat_flush_frame_features(bank_mic, fftx);
	if(usecohv)
	{
		fftytmp=ffty;
		fftxtmp=fftx;
		for(i=0;i<nbin;++i,++fftxtmp,++fftytmp)
		{
			ef=fftxtmp->a*fftxtmp->a+fftxtmp->b*fftxtmp->b;
			yf=fftytmp->a*fftytmp->a+fftytmp->b*fftytmp->b;
			sed.a=fftytmp->a*fftxtmp->a+fftytmp->b*fftxtmp->b;
			sed.b=-fftytmp->a*fftxtmp->b+fftytmp->b*fftxtmp->a;
			leak=(sed.a*sed.a+sed.b*sed.b)/(max(ef,yf)*yf+1e-9);
			leak=sqrtf(leak);
			fftytmp->a*=leak;
			fftytmp->b*=leak;
			leak=(sed.a*sed.a+sed.b*sed.b)/(ef*yf+1e-9);
			gf[i]=leak*yf;
		}
	}
    wtk_bankfeat_flush_frame_features(bank_sp, ffty);
	if(feature_sp && featsp_lm>1)
	{
		memmove(feature_sp+nb_features*featm_lm,feature_sp+nb_features*(featm_lm-1),sizeof(float)*nb_features*(featsp_lm-1));
		memcpy(feature_sp+nb_features*(featm_lm-1),bank_sp->features,sizeof(float)*nb_features);
	}

	if(qmmse && usecohv)
	{
		wtk_qmmse_flush_mask(qmmse, fftx, gf);
	}
    if(feature_sp)
    {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features, feature_sp, nb_features*(featm_lm+featsp_lm-1), 0);  
    }else
    {
        wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features, bank_sp->features, nb_features, 0);  
    }
    wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
    if(qmmse && usecohv)
    {	
        qmmse_gain=qmmse->gain;
        for (i=1; i<nbin-1; ++i)
        {
            if(gf[i]>qmmse_gain[i] )
            {
                gf[i]=qmmse_gain[i];
            }
        }
    }
    for(i=0;i<nbin;++i){
        edr_msg->mask[i] = gf[i];
    }
    // for(i=0;i<channel2;++i){
    //     for(k=0;k<nbin;++k){
    //         bfio5->mic2_out[i][k].a = edr_msg->fft[i+channel][k].a;
    //         bfio5->mic2_out[i][k].b = edr_msg->fft[i+channel][k].b;
    //     }
    // }
    if(bfio5->cfg->use_mic2_raw_audio){
        int channel=bfio5->channel;
        int channel2=bfio5->channel2;
        float scale=sqrtf(bfio5->stft2_2->cfg->win);
        int k;
        for(i=0;i<channel2;++i){
            for(k=0;k<nbin;++k){
                bfio5->mic2_out[i][k].a = edr_msg->fft[i+channel][k].a*scale;
                bfio5->mic2_out[i][k].b = edr_msg->fft[i+channel][k].b*scale;
            }
        }
    }
    // printf("%f\n", wtk_float_abs_mean(gf, nbin));
    // wtk_debug("====\n");
    // getchar();
    wtk_maskssl2_feed_fft2(bfio5->maskssl2, edr_msg->fft, edr_msg->mask, 0);
    if(bfio5->maskssl2_2){
        wtk_maskssl2_feed_fft2(bfio5->maskssl2_2, edr_msg->fft+bfio5->stft2->cfg->channel, edr_msg->mask, 0);
    }

	if(feature_sp && featm_lm>1)
	{
		memmove(feature_sp+nb_features,feature_sp,sizeof(float)*nb_features*(featm_lm-2));
		memcpy(feature_sp,bank_mic->features,sizeof(float)*nb_features);
	}
}

float wtk_bfio5_energy(wtk_complex_t *fft, float *p,int n, int min_idx, int max_idx)
{
    float f,f2;
    int i;

    if(fft){
        f=0;
        for(i=min_idx;i<=max_idx;++i)
        {
            f+=fft[i].a*fft[i].a+fft[i].b*fft[i].b;
        }
        f/=n;

        f2=0;
        for(i=min_idx;i<=max_idx;++i)
        {
            f2+=(fft[i].a*fft[i].a+fft[i].b*fft[i].b-f)*(fft[i].a*fft[i].a+fft[i].b*fft[i].b-f);
        }
        f2/=n;
    }else if(p){
        f=0;
        for(i=min_idx;i<=max_idx;++i)
        {
            f+=p[i];
        }
        f/=n;

        f2=0;
        for(i=min_idx;i<=max_idx;++i)
        {
            f2+=(p[i]-f)*(p[i]-f);
        }
        f2/=n;
    }

    return f2;
}

void wtk_bfio5_feed_edr(wtk_bfio5_t *bfio5, wtk_bfio5_edr_msg_t *edr_msg)
{
	int k;
	int nbin=bfio5->nbin;
	wtk_rls_t *erls=bfio5->erls, *erlstmp;
	wtk_complex_t ffttmp[64]={0};
	wtk_complex_t fftsp2[10]={0};
	int usecohv=1;
    wtk_complex_t **fft;
    int nmicchannel=bfio5->stft2->cfg->channel;
	float spenr;
	float spenr_thresh=bfio5->cfg->spenr_thresh;
	int spenr_cnt=bfio5->cfg->spenr_cnt;
    wtk_complex_t *fftx=bfio5->fftx;
    wtk_complex_t *ffty=bfio5->ffty;
    if(bfio5->stft2_2){
        nmicchannel+=bfio5->stft2_2->cfg->channel;
    }

    if(edr_msg){
        fft=edr_msg->fft;

        spenr=wtk_bfio5_energy(fft[nmicchannel], NULL, nbin, 1, nbin-1);
        if(spenr>spenr_thresh)
		{
			bfio5->sp_sil=0;
			bfio5->sp_silcnt=spenr_cnt;
		}else if(bfio5->sp_sil==0)
		{
			bfio5->sp_silcnt-=1;
			if(bfio5->sp_silcnt<=0)
			{
				bfio5->sp_sil=1;
			}
		}

        if(erls)
        {
            erlstmp=erls;
            for(k=0; k<nbin; ++k, ++erlstmp)
            {

                ffttmp[0]=fft[0][k];
                fftsp2[0]=fft[nmicchannel][k];
                wtk_rls_feed3(erlstmp, ffttmp, fftsp2, bfio5->sp_sil==0);
                if(bfio5->sp_sil==0)
                {
                    fftx[k]=erlstmp->out[0];
                    ffty[k]=erlstmp->lsty[0];
                }else{
                    fftx[k]=ffttmp[0];
                    ffty[k]=fftsp2[0];
                }
            }
        }else
        {
            usecohv=0;
            memcpy(fftx,fft[0],sizeof(wtk_complex_t)*nbin);
            memcpy(ffty,fft[nmicchannel],sizeof(wtk_complex_t)*nbin);
        }
    }
    wtk_bfio5_feed_edr_model(bfio5, edr_msg, fftx, ffty, usecohv);
}

void wtk_bfio5_on_aec(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    int i, nbin=bfio5->stft2->nbin;
    wtk_bfio5_wake_t **wake=bfio5->wake;
    wtk_bfio5_wake_t **wake2=bfio5->wake2;
    wtk_queue_t *stft2_q=&(bfio5->stft2_q);
    wtk_queue_t *edr_stft_q=&(bfio5->edr_stft_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *msg2;
    wtk_bfio5_edr_msg_t *edr_msg;
    int wbf_cnt=bfio5->wbf? bfio5->wbf->cfg->wbf_cnt:bfio5->wbf2->cfg->wbf2_cnt;
    float *data=bfio5->stft2->output;
    float **saec_pad=bfio5->saec_pad;
    wtk_complex_t **saec_out=bfio5->saec_out;
    short *pv=NULL;
    int k=0;
    int n;
    float scale=bfio5->cfg->wake_scale;

    if(bfio5->reg_bf){
        wtk_bfio5_reg_bf(bfio5, msg, pos, is_end);
    }else{
        if(is_end)
        {
            bfio5->end_pos=pos;
            bfio5->is_end=1;
        }

        if(msg)
        {
            wtk_queue_push(stft2_q, &(msg->q_n));
            if(stft2_q->length > bfio5->cfg->stft2_hist)
            {
                qn=wtk_queue_pop(stft2_q);
                msg2=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
                wtk_stft2_push_msg(bfio5->stft2, msg2);
            }
        }
        if(bfio5->edr){
            if(edr_stft_q->length>bfio5->cfg->stft2_hist){
                qn=wtk_queue_pop(edr_stft_q);
                edr_msg=(wtk_bfio5_edr_msg_t *)data_offset2(qn, wtk_bfio5_edr_msg_t, q_n);
                wtk_bfio5_edr_push_msg(bfio5, edr_msg);
            }
        }
        if(bfio5->cfg->use_asr)
        {
            if(bfio5->qform)
            {
                wtk_qform9_feed_smsg(bfio5->qform, msg, pos, is_end);
            }else if(bfio5->qform2)
            {
                wtk_qform2_feed_smsg(bfio5->qform2, msg, pos ,is_end);
                wtk_bfio5_on_qform(bfio5, msg ? bfio5->qform2->out : NULL, is_end);
            }else if(bfio5->qform3){
                wtk_qform3_feed_smsg(bfio5->qform3, msg, pos ,is_end);
                wtk_bfio5_on_qform(bfio5, msg ? bfio5->qform3->out : NULL, is_end);
            }
        }
        if(bfio5->ssl2){
            wtk_ssl2_feed_fft2(bfio5->ssl2, msg->fft, is_end);
        }
        if(bfio5->cfg->use_kvadwake){
            for(i=0; i<wbf_cnt; ++i)
            {
                wake[i]->waked=0;
            }
            if(bfio5->cfg->use_aec){
                for (i = 0; i < bfio5->wake_cnt; ++i) {
                    wtk_kvadwake_get_sp_sil(bfio5->wake[i]->vwake, bfio5->aec->sp_sil);
                }
            }
        }
        if(bfio5->cfg->use_kvadwake2){
            for(i=0; i<wbf_cnt; ++i)
            {
                wake2[i]->waked=0;
            }
            if(bfio5->cfg->use_aec){
                for (i = 0; i < bfio5->wake_cnt; ++i) {
                    wtk_kvadwake_get_sp_sil(bfio5->wake2[i]->vwake, bfio5->aec->sp_sil);
                }
            }
        }
        if(bfio5->wbf)
        {
            wtk_wbf_feed_smsg(bfio5->wbf, msg, pos, is_end);
        }else
        {
            wtk_wbf2_feed_smsg(bfio5->wbf2, msg, pos, is_end);
        }
        if (bfio5->cfg->use_raw_audio)
        {
            if(bfio5->cfg->use_all_raw_audio){
                for(n=0;n<bfio5->channel;++n){
                    if(bfio5->cfg->use_kvadwake){
                        wake[n+wbf_cnt]->waked=0;
                    }
                    if(bfio5->cfg->use_kvadwake2){
                        wake2[n+wbf_cnt]->waked=0;
                    }
                    if (is_end) 
                    {
                        if(bfio5->cfg->use_kvadwake){
                            wtk_kvadwake_feed(wake[n+wbf_cnt]->vwake, NULL, 0, 1);
                        }
                        if(bfio5->cfg->use_kvadwake2){
                            wtk_kvadwake_feed(wake2[n+wbf_cnt]->vwake, NULL, 0, 1);
                        }
                    } else 
                    {   
                        for(i=0;i<nbin;++i)
                        {
                            saec_out[n][i]=msg->fft[i][n];
                        }
                        k=wtk_stft2_output_ifft(bfio5->stft2,saec_out[n],data,saec_pad[n],bfio5->end_pos,is_end);
                        if(bfio5->cfg->use_preemph)
                        {
                            bfio5->memX[n+wbf_cnt]=wtk_preemph_asis2(data,k,bfio5->memX[n+wbf_cnt]);
                        }
                        pv=(short *)data;
                        for(i=0;i<k;++i)
                        {
                            pv[i] = QTK_SSAT16f(data[i]*scale);
                        }
    
                        if(bfio5->cfg->use_kvadwake){
                            wtk_kvadwake_feed(wake[n+wbf_cnt]->vwake, pv, k, 0);
                        }else{
                            if(n==0){
                                bfio5->wake_extend+=k*1.0/bfio5->cfg->rate;
                            }
                        }
                        if(bfio5->cfg->use_kvadwake2){
                            wtk_kvadwake_feed(wake2[n+wbf_cnt]->vwake, pv, k, 0);
                        }else{
                            if(n==0){
                                bfio5->wake2_extend+=k*1.0/bfio5->cfg->rate;

                            }
                        }
#ifdef USE_LOG_BFIO5
                        int j;
                        static wtk_wavfile_t *raw_log[8];
                        for(j=0;j < bfio5->channel;++j){
                            if(n==j)
                            {
                                if(!raw_log[j])
                                {
                                    raw_log[j]=wtk_wavfile_new(16000);
                                    wtk_wavfile_open2(raw_log[j],"wbf/wbf.0");
                                    raw_log[j]->max_pend=0;
                                }
                                if(k>0)
                                {
                                    wtk_wavfile_write(raw_log[j],(char *)pv,k<<1);
                                }
                                if(is_end)
                                {
                                    wtk_debug("============ close ============\n");
                                    wtk_wavfile_close(raw_log[j]);
                                    wtk_wavfile_delete(raw_log[j]);
                                    raw_log[j]=NULL;
                                }
                            }
                        }
#endif
                    }
                }
            }else{
                if(bfio5->cfg->use_kvadwake){
                    wake[wbf_cnt]->waked=0;
                }
                if(bfio5->cfg->use_kvadwake2){
                    wake2[wbf_cnt]->waked=0;
                }
                if (is_end) 
                {
                    if(bfio5->cfg->use_kvadwake){
                        wtk_kvadwake_feed(wake[wbf_cnt]->vwake, NULL, 0, 1);
                    }
                    if(bfio5->cfg->use_kvadwake2){
                        wtk_kvadwake_feed(wake2[wbf_cnt]->vwake, NULL, 0, 1);
                    }
                } else 
                {   
                    for(i=0;i<nbin;++i)
                    {
                        saec_out[0][i]=msg->fft[i][0];
                    }
                    k=wtk_stft2_output_ifft(bfio5->stft2,saec_out[0],data,saec_pad[0],bfio5->end_pos,is_end);
                    if(bfio5->cfg->use_preemph)
                    {
                        bfio5->memX[wbf_cnt]=wtk_preemph_asis2(data,k,bfio5->memX[wbf_cnt]);
                    }
                    pv=(short *)data;
                    for(i=0;i<k;++i)
                    {
                        pv[i] = QTK_SSAT16f(data[i]*scale);
                    }

                    if(bfio5->cfg->use_kvadwake){
                        wtk_kvadwake_feed(wake[wbf_cnt]->vwake, pv, k, 0);
                    }else{
                        bfio5->wake_extend+=k*1.0/bfio5->cfg->rate;
                    }
                    if(bfio5->cfg->use_kvadwake2){
                        wtk_kvadwake_feed(wake2[wbf_cnt]->vwake, pv, k, 0);
                    }else{
                        bfio5->wake2_extend+=k*1.0/bfio5->cfg->rate;
                    }
        #ifdef USE_LOG_BFIO5

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
        if (bfio5->cfg->use_gdenoise) {
            if(bfio5->cfg->use_all_raw_audio){
                if(bfio5->cfg->use_kvadwake){
                    wake[wbf_cnt + bfio5->channel]->waked = 0;
                }
                if(bfio5->cfg->use_kvadwake2){
                    wake2[wbf_cnt + bfio5->channel]->waked=0;
                }
            }else{
                if(bfio5->cfg->use_kvadwake){
                    wake[wbf_cnt + 1]->waked = 0;
                }
                if(bfio5->cfg->use_kvadwake2){
                    wake2[wbf_cnt + 1]->waked=0;
                }
            }
            for (i = 0; i < nbin; ++i) {
                saec_out[0][i] = msg->fft[i][0];
            }
            wtk_gainnet_denoise_feed2(bfio5->gdenoise, saec_out[0], is_end);
        }
        if(bfio5->cfg->use_mic2_raw_audio){
            wtk_bfio5_edr_feed_wake(bfio5, bfio5->mic2_out, is_end);
        }
        _bfio5_waked_post(bfio5, is_end);
    }
}


void wtk_bfio5_on_stft2(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *mspstft_q=&(bfio5->mspstft_q);
    wtk_queue_t *mstft_2_q=&(bfio5->mstft_2_q);
    wtk_queue_t *edr_stft_q=&(bfio5->edr_stft_q);
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *sp_msg;
    wtk_stft2_msg_t *stft2_2_msg;
    wtk_bfio5_edr_msg_t *edr_msg;

    qn=wtk_queue_pop(mspstft_q);
    if(qn)
    {
        sp_msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
    }else
    {
        sp_msg=NULL;
    }
    qn=wtk_queue_pop(mstft_2_q);
    if(qn)
    {
        stft2_2_msg=(wtk_stft2_msg_t *)data_offset2(qn, wtk_stft2_msg_t, q_n);
    }else
    {
        stft2_2_msg=NULL;
    }

    wtk_aec_feed_stftmsg(bfio5->aec, msg, sp_msg, pos, is_end);
    if(bfio5->edr)
    {
        int i, j;
        int channel=bfio5->channel;
        int channel2=bfio5->channel2;
        int sp_channel=bfio5->sp_channel;
        edr_msg = wtk_bfio5_edr_pop_msg(bfio5);
        for(i=0;i<bfio5->stft2->nbin;++i){
            for(j=0;j<channel;++j){
                edr_msg->fft[j][i].a=msg->fft[i][j].a*1.0/sqrt(bfio5->cfg->wins);
                edr_msg->fft[j][i].b=msg->fft[i][j].b*1.0/sqrt(bfio5->cfg->wins);
            }
            for(j=channel;j<channel+channel2;++j){
                edr_msg->fft[j][i].a=stft2_2_msg->fft[i][j-channel].a*1.0/sqrt(bfio5->cfg->wins);
                edr_msg->fft[j][i].b=stft2_2_msg->fft[i][j-channel].b*1.0/sqrt(bfio5->cfg->wins);
            }
            for(j=channel+channel2;j<channel+channel2+sp_channel;++j){
                edr_msg->fft[j][i].a=sp_msg->fft[i][j-channel-channel2].a*1.0/sqrt(bfio5->cfg->wins);
                edr_msg->fft[j][i].b=sp_msg->fft[i][j-channel-channel2].b*1.0/sqrt(bfio5->cfg->wins);
            }
        }
        edr_msg->s = msg->s;
        edr_msg->theta = -1;
        edr_msg->theta2 = -1;
        // wtk_debug("%f\n", edr_msg->s);
        wtk_bfio5_feed_edr(bfio5, edr_msg);
        wtk_queue_push(edr_stft_q, &(edr_msg->q_n));
    }

    if(sp_msg)
    {
        wtk_stft2_push_msg(bfio5->sp_stft2, sp_msg);
    }
    if(stft2_2_msg){
        wtk_stft2_push_msg(bfio5->stft2_2, stft2_2_msg);
    }
}

void wtk_bfio5_on_sp_stft2(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    if(msg)
    {
        wtk_queue_push(&(bfio5->mspstft_q),  &(msg->q_n));
    }
}

void wtk_bfio5_on_stft2_2(wtk_bfio5_t *bfio5,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    if(msg)
    {
        wtk_queue_push(&(bfio5->mstft_2_q),  &(msg->q_n));
    }
}

void wtk_bfio5_feed_dc(wtk_bfio5_t *bfio5,short **data,int len,int is_end)
{   
    int i,j;
    int channel=bfio5->channel;
    int channel2=bfio5->channel2;
    int sp_channel=bfio5->sp_channel;
    float fv;
    float *fp[512],*fp2[512], *fp3[512];
    wtk_strbuf_t **input=bfio5->input;

    if(bfio5->cfg->use_nmicchannel){
        int nmicchannel=bfio5->cfg->nmicchannel;
        int *mic_channel=bfio5->cfg->mic_channel;
        int nmicchannel2=bfio5->cfg->nmicchannel2;
        int *mic_channel2=bfio5->cfg->mic_channel2;
        int nspchannel=bfio5->cfg->nspchannel;
        int *sp_channel=bfio5->cfg->sp_channel;

        for(i=0;i<nmicchannel2;++i){
            wtk_strbuf_reset(input[mic_channel2[i]]);
            for(j=0;j<len;++j)
            {
                fv=data[mic_channel2[i]][j];
                wtk_strbuf_push(input[mic_channel2[i]],(char *)(&fv),sizeof(float));
            }
            fp3[i]=(float *)(input[mic_channel2[i]]->data);
            wtk_preemph_dc(fp3[i], bfio5->notch_mem[mic_channel2[i]], len);
            bfio5->memD[mic_channel2[i]]=wtk_preemph_asis(fp3[i], len, bfio5->memD[mic_channel2[i]]);
        }
        if(bfio5->stft2_2){
            wtk_stft2_feed_float(bfio5->stft2_2, fp3, len, is_end);
        }

        for(i=0;i<nmicchannel;++i)
        {
            wtk_strbuf_reset(input[mic_channel[i]]);
            for(j=0;j<len;++j)
            {
                fv=data[mic_channel[i]][j];
                wtk_strbuf_push(input[mic_channel[i]],(char *)(&fv),sizeof(float));
            }
            fp[i]=(float *)(input[mic_channel[i]]->data);
            wtk_preemph_dc(fp[i], bfio5->notch_mem[mic_channel[i]], len);
            bfio5->memD[mic_channel[i]]=wtk_preemph_asis(fp[i], len, bfio5->memD[mic_channel[i]]);
        }
        if(bfio5->aec)
        {
            for(i=0;i<nspchannel;++i){
                wtk_strbuf_reset(input[sp_channel[i]]);
                for(j=0;j<len;++j)
                {
                    fv=data[sp_channel[i]][j];
                    wtk_strbuf_push(input[sp_channel[i]],(char *)(&fv),sizeof(float));
                }
                fp2[i]=(float *)(input[sp_channel[i]]->data);
                wtk_preemph_dc(fp2[i], bfio5->notch_mem[sp_channel[i]], len);
                bfio5->memD[sp_channel[i]]=wtk_preemph_asis(fp2[i], len, bfio5->memD[sp_channel[i]]);
            }
            wtk_stft2_feed_float(bfio5->sp_stft2, fp2, len, is_end);
        }
        wtk_stft2_feed_float(bfio5->stft2, fp, len, is_end);
    }else{
        for(i=0;i<channel2;++i){
            wtk_strbuf_reset(input[i]);
            for(j=0;j<len;++j)
            {
                fv=data[i][j];
                wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
            }
            fp3[i]=(float *)(input[i]->data);
            wtk_preemph_dc(fp3[i], bfio5->notch_mem[i], len);
            bfio5->memD[i]=wtk_preemph_asis(fp3[i], len, bfio5->memD[i]);
        }
        if(bfio5->stft2_2){
            wtk_stft2_feed_float(bfio5->stft2_2, fp3, len, is_end);
        }

        for(i=channel2;i<channel+channel2;++i)
        {
            wtk_strbuf_reset(input[i]);
            for(j=0;j<len;++j)
            {
                fv=data[i][j];
                wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
            }
            fp[i-channel2]=(float *)(input[i]->data);
            
            wtk_preemph_dc(fp[i-channel2], bfio5->notch_mem[i], len);
            bfio5->memD[i]=wtk_preemph_asis(fp[i-channel2], len, bfio5->memD[i]);
        }

        if(bfio5->aec)
        {
            for(i=channel+channel2;i<channel+channel2+sp_channel;++i){
                wtk_strbuf_reset(input[i]);
                for(j=0;j<len;++j)
                {
                    fv=data[i][j];
                    wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
                }
                fp2[i-channel-channel2]=(float *)(input[i]->data);
                
                wtk_preemph_dc(fp2[i-channel-channel2], bfio5->notch_mem[i], len);
                bfio5->memD[i]=wtk_preemph_asis(fp2[i-channel-channel2], len, bfio5->memD[i]);
            }
            wtk_stft2_feed_float(bfio5->sp_stft2, fp2, len, is_end);
        }
        wtk_stft2_feed_float(bfio5->stft2, fp, len, is_end);
    }
}


void wtk_bfio5_feed(wtk_bfio5_t *bfio5,short **data,int len,int is_end)
{
    short *pv[16];
    int i;
    int channel=bfio5->channel;
    int channel2=bfio5->channel2;
    int spchannel=bfio5->sp_channel;

    if(bfio5->cfg->use_nmicchannel){
        int nmicchannel=bfio5->cfg->nmicchannel;
        int *mic_channel=bfio5->cfg->mic_channel;
        int nmicchannel2=bfio5->cfg->nmicchannel2;
        int *mic_channel2=bfio5->cfg->mic_channel2;
        int nspchannel=bfio5->cfg->nspchannel;
        int *sp_channel=bfio5->cfg->sp_channel;
        if(bfio5->input)
        {
            wtk_bfio5_feed_dc(bfio5,data,len,is_end);
        }else
        {
            if(bfio5->stft2_2){
                for(i=0;i<nmicchannel2;++i){
                    pv[i]=data[mic_channel2[i]];
                }
                wtk_stft2_feed2(bfio5->stft2_2, pv, len, is_end);
            }
            if(bfio5->aec)
            {
                if(data)
                {
                    for(i=0;i<nspchannel;++i)
                    {
                        pv[i]=data[sp_channel[i]];
                    }
                    wtk_stft2_feed2(bfio5->sp_stft2, pv, len, is_end);
                }else
                {
                    wtk_stft2_feed2(bfio5->sp_stft2, NULL, 0, is_end);
                }
            }
            if(data)
            {
                for(i=0;i<nmicchannel;++i)
                {
                    pv[i]=data[mic_channel[i]];
                }
                wtk_stft2_feed2(bfio5->stft2, pv, len, is_end);
            }
        }
    }else{
        if(bfio5->input)
        {
            wtk_bfio5_feed_dc(bfio5,data,len,is_end);
        }else
        {
            if(bfio5->stft2_2){
                wtk_stft2_feed2(bfio5->stft2_2, data, len, is_end);
            }
            if(bfio5->aec)
            {
                if(data)
                {
                    for(i=0;i<spchannel;++i)
                    {
                        pv[i]=data[channel2+channel+i];
                    }
                    wtk_stft2_feed2(bfio5->sp_stft2, pv, len, is_end);
                }else
                {
                    wtk_stft2_feed2(bfio5->sp_stft2, NULL, 0, is_end);
                }
            }
            if(data)
            {
                for(i=0;i<channel;++i)
                {
                    pv[i]=data[channel2+i];
                }
                wtk_stft2_feed2(bfio5->stft2, pv, len, is_end);
            }
        }
    }
    if(bfio5->aec && bfio5->aec_wake_buf && !is_end){
        wtk_strbuf_t *aec_wake_buf=bfio5->aec_wake_buf;
        int aec_len;
        int aec_wake_len=bfio5->cfg->aec_wake_len;

        wtk_strbuf_push(aec_wake_buf, (char *)data[channel2+channel], len*sizeof(short));

        aec_len=aec_wake_buf->pos/sizeof(short);
        
        if(aec_len>aec_wake_len){
            wtk_strbuf_pop(aec_wake_buf, NULL, (aec_len-aec_wake_len)*sizeof(short));
        }
        bfio5->aec_fe+=len;
        bfio5->aec_fs=max(0,bfio5->de_fe-aec_wake_len);
        // wtk_debug("%f %f\n", bfio5->aec_fs, bfio5->aec_fe);
    }
}

void wtk_bfio5_set_one_shot(wtk_bfio5_t *bfio5, int on) {
    bfio5->cfg->use_one_shot = on;
    if (!on) {
        bfio5->waked = 1;
    }else{
        bfio5->waked = 0;
    }
}

void wtk_bfio5_set_offline_qform(wtk_bfio5_t *bfio5, int tms, int theta)
{
    int max_theta=bfio5->cfg->use_line?180:359;

    bfio5->reg_bf=1;
    bfio5->reg_theta=theta;
    bfio5->reg_tms=tms;
    bfio5->reg_end=0;
    if(theta >= 0 && theta <= max_theta){
        if(bfio5->qform)
        {
            wtk_qform9_start2(bfio5->qform,theta,0);
        }else if(bfio5->qform2)
        {
            wtk_qform2_start2(bfio5->qform2,theta,0);
        }else if(bfio5->qform3){
            wtk_qform3_start2(bfio5->qform3,theta,0);
        }
    }
}

int wtk_bfio5_set_wake_words(wtk_bfio5_t *bfio5,char *words,int len)
{
    int ret = -1;
    int wake_cnt=bfio5->wake_cnt;
    int i;

    if(bfio5->wake2){
        bfio5->cfg->use_kvadwake2=0;
        if(bfio5->wake2[0]->vwake->cfg->use_kwdec2){
            for(i=0;i<wake_cnt;++i){
                wtk_kwdec2_feed2(bfio5->wake2[i]->vwake->kwdec2, NULL, 0, 1);
            }
            for(i=0;i<wake_cnt;++i){
                ret = wtk_kwdec2_set_words(bfio5->wake2[i]->vwake->kwdec2,words,strlen(words));
                if(ret!=0){
                    goto end;
                }
            }
            wtk_kwdec2_set_words_cfg(bfio5->wake2[0]->vwake->kwdec2,words,strlen(words));
            for(i=0;i<wake_cnt;++i){
                wtk_kwdec2_decwords_set(bfio5->wake2[i]->vwake->kwdec2);
            }
            for(i=0;i<wake_cnt;++i){
                wtk_kwdec2_reset(bfio5->wake2[i]->vwake->kwdec2);
                wtk_kwdec2_start(bfio5->wake2[i]->vwake->kwdec2);
            }
        }else if(bfio5->wake2[0]->vwake->cfg->use_wdec){
            wtk_wdec_set_words(bfio5->wake2[wake_cnt-1]->vwake->wdec,words,strlen(words));
            wtk_wdec_feed(bfio5->wake2[wake_cnt-1]->vwake->wdec,NULL,0,1);
            wtk_wdec_reset(bfio5->wake2[wake_cnt-1]->vwake->wdec);
            wtk_wdec_start(bfio5->wake2[wake_cnt-1]->vwake->wdec, NULL);
        }
        bfio5->cfg->use_kvadwake2=1;
    }
end:
    return ret;
}
