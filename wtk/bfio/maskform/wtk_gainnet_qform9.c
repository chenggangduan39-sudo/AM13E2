#include "wtk_gainnet_qform9.h" 

void wtk_gainnet_qform9_on_stft2(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_gainnet_qform9_on_qenvelope(wtk_gainnet_qform9_t *gainnet_qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_gainnet_qform9_on_qenvel1(wtk_gainnet_qform9_t *gainnet_qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_gainnet_qform9_on_qenvel2(wtk_gainnet_qform9_t *gainnet_qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_gainnet_qform9_denoise_on_gainnet(wtk_gainnet_qform9_edra_t *vdr, float *gain, int len, int is_end);

void wtk_gainnet_qform9_edra_init(wtk_gainnet_qform9_edra_t *vdr, wtk_gainnet_qform9_cfg_t *cfg)
{
	vdr->cfg=cfg;
	vdr->nbin=cfg->stft2.win/2+1;

	vdr->bank_mic=wtk_bankfeat_new(&(cfg->bankfeat));

	vdr->g=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->lastg=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_bands);
	vdr->gf=wtk_malloc(sizeof(float)*vdr->nbin);

    vdr->feature_lm=NULL;
    if(cfg->featm_lm>2){
		vdr->feature_lm=wtk_malloc(sizeof(float)*cfg->bankfeat.nb_features*(cfg->featm_lm-1));
    }

	vdr->gainnet2=wtk_gainnet2_new(cfg->gainnet2);
	wtk_gainnet2_set_notify(vdr->gainnet2, vdr, (wtk_gainnet2_notify_f)wtk_gainnet_qform9_denoise_on_gainnet);

	vdr->qmmse=NULL;
	if(cfg->use_qmmse)
	{
		vdr->qmmse=wtk_qmmse_new(&(cfg->mdl_qmmse));
	}
}

void wtk_gainnet_qform9_edra_clean(wtk_gainnet_qform9_edra_t *vdr)
{
	wtk_bankfeat_delete(vdr->bank_mic);


	wtk_free(vdr->g);
	wtk_free(vdr->lastg);
	wtk_free(vdr->gf);

	if(vdr->qmmse)
	{
		wtk_qmmse_delete(vdr->qmmse);
	}

	if(vdr->feature_lm)
	{
		wtk_free(vdr->feature_lm);
	}

	wtk_gainnet2_delete(vdr->gainnet2);
}

void wtk_gainnet_qform9_edra_reset(wtk_gainnet_qform9_edra_t *vdr)
{
	wtk_bankfeat_reset(vdr->bank_mic);

	memset(vdr->g, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->lastg, 0, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
	memset(vdr->gf, 0, sizeof(float)*vdr->nbin);

	if(vdr->feature_lm)
	{
		memset(vdr->feature_lm, 0, sizeof(float)*vdr->cfg->bankfeat.nb_features*(vdr->cfg->featm_lm-1));
	}

	wtk_gainnet2_reset(vdr->gainnet2);

	if(vdr->qmmse)
	{
		wtk_qmmse_reset(vdr->qmmse);
	}
}

wtk_stft2_msg_t* wtk_gainnet_qform9_stft2_msg_new(wtk_gainnet_qform9_t *gainnet_qform9)
{
	wtk_stft2_msg_t *msg;

	msg=(wtk_stft2_msg_t*)wtk_malloc(sizeof(wtk_stft2_msg_t));
	msg->hook=NULL;
	msg->fft=wtk_complex_new_p2(gainnet_qform9->nbin,gainnet_qform9->bf->channel);
	return msg;
}

void wtk_gainnet_qform9_stft2_msg_delete(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,gainnet_qform9->nbin);
	wtk_free(msg);
}

wtk_stft2_msg_t* wtk_gainnet_qform9_pop_stft2_msg(wtk_gainnet_qform9_t *gainnet_qform9)
{
	return  (wtk_stft2_msg_t*)wtk_hoard_pop(&(gainnet_qform9->stft_msg_hoard));
}

void wtk_gainnet_qform9_push_stft2_msg(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *msg)
{
	wtk_hoard_push(&(gainnet_qform9->stft_msg_hoard),msg);
}

wtk_stft2_msg_t* wtk_gainnet_qform9_stft2_msg_copy(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *msg,int channel,int nbin)
{
	wtk_stft2_msg_t *vmsg;

	vmsg=wtk_gainnet_qform9_pop_stft2_msg(gainnet_qform9);
	vmsg->s=msg->s;
	wtk_complex_cpy_p2(vmsg->fft,msg->fft,nbin,channel);
	return vmsg;
}

wtk_gainnet_qform9_envelopemsg_t *wtk_gainnet_qform9_envelope_msg_new(wtk_gainnet_qform9_t *gainnet_qform9)
{
    wtk_gainnet_qform9_envelopemsg_t *msg;

    msg=(wtk_gainnet_qform9_envelopemsg_t *)wtk_malloc(sizeof(wtk_gainnet_qform9_envelopemsg_t));
    msg->smsg=wtk_gainnet_qform9_stft2_msg_new(gainnet_qform9);
    msg->cohv=(float *)wtk_malloc(sizeof(float)*gainnet_qform9->nbin);
    return msg;
}

void wtk_gainnet_qform9_envelope_msg_delete(wtk_gainnet_qform9_t *gainnet_qform9, wtk_gainnet_qform9_envelopemsg_t *qemsg)
{
    wtk_free(qemsg->cohv);
    wtk_gainnet_qform9_stft2_msg_delete(gainnet_qform9,qemsg->smsg);
    wtk_free(qemsg);
}

wtk_gainnet_qform9_envelopemsg_t* wtk_gainnet_qform9_pop_envelope_msg(wtk_gainnet_qform9_t *gainnet_qform9)
{
	return  (wtk_gainnet_qform9_envelopemsg_t*)wtk_hoard_pop(&(gainnet_qform9->qenvel_msg_hoard));
}

void wtk_gainnet_qform9_push_envelope_msg(wtk_gainnet_qform9_t *gainnet_qform9,wtk_gainnet_qform9_envelopemsg_t *msg)
{
	wtk_hoard_push(&(gainnet_qform9->qenvel_msg_hoard),msg);
}

wtk_gainnet_qform9_envelopemsg_t *wtk_gainnet_qform9_envelope_msg_copy(wtk_gainnet_qform9_t *gainnet_qform9, wtk_stft2_msg_t *smsg, float *cohv, int nbin, int channel)
{
    wtk_gainnet_qform9_envelopemsg_t *qemsg;

    qemsg=wtk_gainnet_qform9_pop_envelope_msg(gainnet_qform9);
    qemsg->smsg->hook=NULL;
    qemsg->smsg->s=smsg->s;
    wtk_complex_cpy_p2(qemsg->smsg->fft, smsg->fft, nbin, channel);
    memcpy(qemsg->cohv, cohv, sizeof(float)*nbin);
    return qemsg;
}

wtk_gainnet_qform9_t* wtk_gainnet_qform9_new(wtk_gainnet_qform9_cfg_t *cfg)
{
    wtk_gainnet_qform9_t *gainnet_qform9;
    int i;

    gainnet_qform9=(wtk_gainnet_qform9_t *)wtk_malloc(sizeof(wtk_gainnet_qform9_t));
    gainnet_qform9->cfg=cfg;
    gainnet_qform9->ths=NULL;
    gainnet_qform9->notify=NULL;
    gainnet_qform9->ths_two_channel=NULL;
    gainnet_qform9->notify_two_channel=NULL;
    gainnet_qform9->ths2=NULL;
    gainnet_qform9->notify2=NULL;
	gainnet_qform9->nbin=cfg->stft2.win/2+1;

    gainnet_qform9->input=NULL;
    if(cfg->use_preemph)
    {
        gainnet_qform9->input=wtk_strbufs_new(cfg->bf.nmic);
    }

    gainnet_qform9->stft2=wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(gainnet_qform9->stft2,gainnet_qform9,(wtk_stft2_notify_f)wtk_gainnet_qform9_on_stft2);

    gainnet_qform9->nbin=gainnet_qform9->stft2->nbin;

    gainnet_qform9->covm=NULL;
    gainnet_qform9->covm_class[0]=gainnet_qform9->covm_class[1]=NULL;
    gainnet_qform9->covm=wtk_covm_new(&(cfg->covm), gainnet_qform9->nbin, cfg->stft2.channel);
    if(gainnet_qform9->cfg->use_two_channel){
        gainnet_qform9->covm_class[0]=wtk_covm_new(&(cfg->covm), gainnet_qform9->nbin, cfg->stft2.channel);
        gainnet_qform9->covm_class[1]=wtk_covm_new(&(cfg->covm), gainnet_qform9->nbin, cfg->stft2.channel);
    }
    
    gainnet_qform9->bf_class[0]=gainnet_qform9->bf_class[1]=NULL;
    if(gainnet_qform9->cfg->use_two_channel){
        gainnet_qform9->bf_class[0]=wtk_bf_new(&(cfg->bf),cfg->stft2.win);
        gainnet_qform9->bf_class[1]=wtk_bf_new(&(cfg->bf),cfg->stft2.win);
    }
    gainnet_qform9->bf=NULL;
    gainnet_qform9->bf=wtk_bf_new(&(cfg->bf),cfg->stft2.win);

    gainnet_qform9->aspec=NULL;
    gainnet_qform9->aspec_class[0]=gainnet_qform9->aspec_class[1]=gainnet_qform9->aspec_class[2]=NULL;
    if(cfg->use_two_channel || cfg->use_noise_qenvelope)
    {
        gainnet_qform9->aspec_class[0]=wtk_aspec_new(&(cfg->aspec), gainnet_qform9->stft2->nbin, 3);        
        gainnet_qform9->aspec_class[1]=wtk_aspec_new(&(cfg->aspec), gainnet_qform9->stft2->nbin, 3);
        gainnet_qform9->aspec_class[2]=wtk_aspec_new(&(cfg->aspec), gainnet_qform9->stft2->nbin, 3);        
    }else
    {
        gainnet_qform9->aspec=wtk_aspec_new(&(cfg->aspec), gainnet_qform9->stft2->nbin, 3);        
    }

    gainnet_qform9->fftclass[0]=gainnet_qform9->fftclass[1]=NULL;
    if(gainnet_qform9->aspec_class[0] && !gainnet_qform9->aspec_class[0]->need_cov && gainnet_qform9->cfg->use_two_channel)
    {
        gainnet_qform9->fftclass[0]=wtk_complex_new_p2(gainnet_qform9->nbin,gainnet_qform9->stft2->cfg->channel);
        gainnet_qform9->fftclass[1]=wtk_complex_new_p2(gainnet_qform9->nbin,gainnet_qform9->stft2->cfg->channel);
    }else if(gainnet_qform9->aspec_class[0] && !gainnet_qform9->aspec_class[0]->need_cov)
    {
        gainnet_qform9->fftclass[0]=wtk_complex_new_p2(gainnet_qform9->nbin,gainnet_qform9->aspec_class[0]->cfg->channel);
        gainnet_qform9->fftclass[1]=wtk_complex_new_p2(gainnet_qform9->nbin,gainnet_qform9->aspec_class[1]->cfg->channel);
    }
    
    gainnet_qform9->cov=NULL;
    gainnet_qform9->covclass[0]=gainnet_qform9->covclass[1]=NULL;
    wtk_queue_init(&(gainnet_qform9->stft2_q));
    if((gainnet_qform9->aspec && gainnet_qform9->aspec->need_cov) || (gainnet_qform9->aspec_class[0] && gainnet_qform9->aspec_class[0]->need_cov) )
    {
        gainnet_qform9->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->stft2.channel*cfg->stft2.channel);
        if(cfg->lt<=0)
        {
            gainnet_qform9->wint=wtk_malloc(sizeof(float));
            gainnet_qform9->wint[0]=1;
        }else
        {
            gainnet_qform9->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            gainnet_qform9->winf=wtk_malloc(sizeof(float));
            gainnet_qform9->winf[0]=1;
        }else
        {
            gainnet_qform9->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }

        if(gainnet_qform9->aspec_class[0])
        {
            gainnet_qform9->covclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_qform9->aspec_class[0]->cfg->channel*gainnet_qform9->aspec_class[0]->cfg->channel);
            gainnet_qform9->covclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_qform9->aspec_class[1]->cfg->channel*gainnet_qform9->aspec_class[1]->cfg->channel);
        }
    }
    gainnet_qform9->inv_cov=NULL;
    gainnet_qform9->invcovclass[0]=gainnet_qform9->invcovclass[1]=NULL;
    gainnet_qform9->tmp=NULL;
    if(gainnet_qform9->aspec && gainnet_qform9->aspec->need_inv_cov)
    {
        gainnet_qform9->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*sizeof(wtk_complex_t));
        gainnet_qform9->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*2*sizeof(wtk_dcomplex_t));
    }
    if(gainnet_qform9->aspec_class[0] && gainnet_qform9->aspec_class[0]->need_inv_cov)
    {
        gainnet_qform9->invcovclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_qform9->aspec_class[0]->cfg->channel*gainnet_qform9->aspec_class[0]->cfg->channel);
        gainnet_qform9->invcovclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_qform9->aspec_class[1]->cfg->channel*gainnet_qform9->aspec_class[1]->cfg->channel);
        i=max(gainnet_qform9->aspec_class[0]->cfg->channel,gainnet_qform9->aspec_class[1]->cfg->channel);
        gainnet_qform9->tmp=(wtk_dcomplex_t *)wtk_malloc(i*i*2*sizeof(wtk_dcomplex_t));
    }

    gainnet_qform9->cohv=NULL;
    gainnet_qform9->cohv_class[0]=gainnet_qform9->cohv_class[1]=gainnet_qform9->cohv_class[2]=NULL;
    if(gainnet_qform9->cfg->use_two_channel){
        gainnet_qform9->cohv_class[0]=(float *)wtk_malloc(sizeof(float)*gainnet_qform9->bf_class[0]->nbin);
        gainnet_qform9->cohv_class[1]=(float *)wtk_malloc(sizeof(float)*gainnet_qform9->bf_class[1]->nbin);
    }else if(gainnet_qform9->cfg->use_noise_qenvelope){
        gainnet_qform9->cohv_class[0]=(float *)wtk_malloc(sizeof(float)*gainnet_qform9->bf->nbin);
        gainnet_qform9->cohv_class[1]=(float *)wtk_malloc(sizeof(float)*gainnet_qform9->bf->nbin);
        gainnet_qform9->cohv_class[2]=(float *)wtk_malloc(sizeof(float)*gainnet_qform9->bf->nbin);
    }else{
        gainnet_qform9->cohv=(float *)wtk_malloc(sizeof(float)*gainnet_qform9->bf->nbin);
    }

    gainnet_qform9->qmmse=NULL;
    gainnet_qform9->qmmse_class[0]=gainnet_qform9->qmmse_class[1]=NULL;
    if(cfg->use_post && gainnet_qform9->cfg->use_two_channel)
    {
        gainnet_qform9->qmmse_class[0]=wtk_qmmse_new(&(cfg->qmmse));
        gainnet_qform9->qmmse_class[1]=wtk_qmmse_new(&(cfg->qmmse));
    }else if(cfg->use_post){
        gainnet_qform9->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }
    
    gainnet_qform9->cohv_fn=NULL;
    gainnet_qform9->cohv_fn_class[0]=gainnet_qform9->cohv_fn_class[1]=NULL;
    if(gainnet_qform9->cfg->use_two_channel){
        if(cfg->debug)
        {
            gainnet_qform9->cohv_fn_class[0]=fopen("cohv1.dat","w");
            gainnet_qform9->cohv_fn_class[1]=fopen("cohv2.dat","w");
        }
    }else{
        if(cfg->debug)
        {
            gainnet_qform9->cohv_fn=fopen("cohv.dat","w");
        }
    }

    gainnet_qform9->qenvelope=NULL;
    gainnet_qform9->qenvel[0]=gainnet_qform9->qenvel[1]=NULL;
    gainnet_qform9->q_fring=NULL;
    gainnet_qform9->q_fring_class[0]=gainnet_qform9->q_fring_class[1]=gainnet_qform9->q_fring_class[2]=NULL;

    if(cfg->use_simple_qenvelope){
        if(cfg->debug)
        {
            gainnet_qform9->cohv_fn=fopen("cohv.dat","w");
        }
    }else if(cfg->use_qenvelope){
        wtk_hoard_init2(&(gainnet_qform9->stft_msg_hoard),offsetof(wtk_stft2_msg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_gainnet_qform9_stft2_msg_new,
            (wtk_delete_handler2_t)wtk_gainnet_qform9_stft2_msg_delete,
            gainnet_qform9);

        wtk_hoard_init2(&(gainnet_qform9->qenvel_msg_hoard),offsetof(wtk_gainnet_qform9_envelopemsg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_gainnet_qform9_envelope_msg_new,
            (wtk_delete_handler2_t)wtk_gainnet_qform9_envelope_msg_delete,
            gainnet_qform9);
    }
    if(gainnet_qform9->cfg->use_two_channel){
        if(cfg->use_simple_qenvelope){
            gainnet_qform9->q_fring_class[0]=wtk_fring_new(cfg->qenvl.envelope_nf+1);
            gainnet_qform9->q_fring_class[1]=wtk_fring_new(cfg->qenvl2.envelope_nf+1);
        }else if(cfg->use_qenvelope)
        {
            gainnet_qform9->qenvel[0]=wtk_qenvelope_new(&(cfg->qenvl));
            gainnet_qform9->qenvel[1]=wtk_qenvelope_new(&(cfg->qenvl2));
            wtk_qenvelope_set_notify(gainnet_qform9->qenvel[0], gainnet_qform9, (wtk_qenvelope_notify_f)wtk_gainnet_qform9_on_qenvel1);
            wtk_qenvelope_set_notify(gainnet_qform9->qenvel[1], gainnet_qform9, (wtk_qenvelope_notify_f)wtk_gainnet_qform9_on_qenvel2);
        }
    }else if(cfg->use_noise_qenvelope){
        if(cfg->use_simple_qenvelope){
            gainnet_qform9->q_fring_class[0]=wtk_fring_new(cfg->qenvl.envelope_nf+1);
            gainnet_qform9->q_fring_class[1]=wtk_fring_new(cfg->qenvl2.envelope_nf+1);
            gainnet_qform9->q_fring_class[2]=wtk_fring_new(cfg->qenvl3.envelope_nf+1);
        }else{
            wtk_debug("error use noise_qenvelope");
        }
    }else{
        if(cfg->use_simple_qenvelope){
            gainnet_qform9->q_fring=wtk_fring_new(cfg->qenvl.envelope_nf+1);
        }else if(cfg->use_qenvelope)
        {
            gainnet_qform9->qenvelope=wtk_qenvelope_new(&(cfg->qenvl));
            wtk_qenvelope_set_notify(gainnet_qform9->qenvelope, gainnet_qform9, (wtk_qenvelope_notify_f)wtk_gainnet_qform9_on_qenvelope);
        }
    }

    gainnet_qform9->chn1_buf=NULL;
    gainnet_qform9->chn2_buf=NULL;
    gainnet_qform9->out_buf=NULL;
    if(gainnet_qform9->cfg->use_two_channel){
        gainnet_qform9->chn1_buf=wtk_strbuf_new(1024, 1);
        gainnet_qform9->chn2_buf=wtk_strbuf_new(1024, 1);
        gainnet_qform9->out_buf=wtk_strbuf_new(1024, 1);
    }

    gainnet_qform9->vdr=NULL;
    gainnet_qform9->fftx=NULL;
    if(cfg->gainnet2){
        gainnet_qform9->vdr=(wtk_gainnet_qform9_edra_t *)wtk_malloc(sizeof(wtk_gainnet_qform9_edra_t));
        wtk_gainnet_qform9_edra_init(gainnet_qform9->vdr, cfg);
    	gainnet_qform9->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*gainnet_qform9->nbin);
    }

    wtk_gainnet_qform9_reset(gainnet_qform9);

    return gainnet_qform9;
}

void wtk_gainnet_qform9_delete(wtk_gainnet_qform9_t *gainnet_qform9)
{
    if(gainnet_qform9->input)
    {
        wtk_strbufs_delete(gainnet_qform9->input, gainnet_qform9->bf->channel);
    }

    if(gainnet_qform9->qenvelope)
    {
        wtk_hoard_clean(&(gainnet_qform9->stft_msg_hoard));
        wtk_hoard_clean(&(gainnet_qform9->qenvel_msg_hoard));
        wtk_qenvelope_delete(gainnet_qform9->qenvelope);
    }
    if(gainnet_qform9->qenvel[0]){
        wtk_hoard_clean(&(gainnet_qform9->stft_msg_hoard));
        wtk_hoard_clean(&(gainnet_qform9->qenvel_msg_hoard));
        wtk_qenvelope_delete(gainnet_qform9->qenvel[0]);
        wtk_qenvelope_delete(gainnet_qform9->qenvel[1]);
    }
    if(gainnet_qform9->q_fring){
        wtk_fring_delete(gainnet_qform9->q_fring);
    }
    if(gainnet_qform9->q_fring_class[0]){
        wtk_fring_delete(gainnet_qform9->q_fring_class[0]);
        wtk_fring_delete(gainnet_qform9->q_fring_class[1]);
    }
    if(gainnet_qform9->q_fring_class[2]){
        wtk_fring_delete(gainnet_qform9->q_fring_class[2]);
    }
    if(gainnet_qform9->chn1_buf){
        wtk_strbuf_delete(gainnet_qform9->chn1_buf);
    }
    if(gainnet_qform9->chn2_buf){
        wtk_strbuf_delete(gainnet_qform9->chn2_buf);
    }
    if(gainnet_qform9->out_buf){
        wtk_strbuf_delete(gainnet_qform9->out_buf);
    }

    if(gainnet_qform9->cov)
    {
        wtk_free(gainnet_qform9->cov);
        wtk_free(gainnet_qform9->wint);
        wtk_free(gainnet_qform9->winf);
    }
    if(gainnet_qform9->inv_cov)
    {
        wtk_free(gainnet_qform9->inv_cov);
    }
    if(gainnet_qform9->tmp)
    {
        wtk_free(gainnet_qform9->tmp);
    }
    if(gainnet_qform9->covclass[0])
    {
        wtk_free(gainnet_qform9->covclass[0]);
        wtk_free(gainnet_qform9->covclass[1]);
    }
    if(gainnet_qform9->invcovclass[0])
    {
        wtk_free(gainnet_qform9->invcovclass[0]);
        wtk_free(gainnet_qform9->invcovclass[1]);
    }
    if(gainnet_qform9->fftclass[0])
    {
        wtk_complex_delete_p2(gainnet_qform9->fftclass[0],gainnet_qform9->nbin);
        wtk_complex_delete_p2(gainnet_qform9->fftclass[1],gainnet_qform9->nbin);
    }

    if(gainnet_qform9->aspec)
    {
        wtk_aspec_delete(gainnet_qform9->aspec);
    }
    if(gainnet_qform9->aspec_class[0])
    {
        wtk_aspec_delete(gainnet_qform9->aspec_class[0]);
        wtk_aspec_delete(gainnet_qform9->aspec_class[1]);
        wtk_aspec_delete(gainnet_qform9->aspec_class[2]);
    }

    if(gainnet_qform9->cohv_fn)
    {
        fclose(gainnet_qform9->cohv_fn);
    }
    if(gainnet_qform9->cohv_fn_class[0]){
        fclose(gainnet_qform9->cohv_fn_class[0]);
        fclose(gainnet_qform9->cohv_fn_class[1]);
    }
    if(gainnet_qform9->cohv_class[0]){
        wtk_free(gainnet_qform9->cohv_class[0]);
        wtk_free(gainnet_qform9->cohv_class[1]);
    }
    if(gainnet_qform9->cohv_class[2]){
        wtk_free(gainnet_qform9->cohv_class[2]);
    }

    if(gainnet_qform9->cohv){
        wtk_free(gainnet_qform9->cohv);
    }
    
    if(gainnet_qform9->qmmse)
    {
        wtk_qmmse_delete(gainnet_qform9->qmmse);
    }
    if(gainnet_qform9->qmmse_class[0]){
        wtk_qmmse_delete(gainnet_qform9->qmmse_class[0]);
        wtk_qmmse_delete(gainnet_qform9->qmmse_class[1]);
    }

    wtk_stft2_delete(gainnet_qform9->stft2);
    if(gainnet_qform9->covm){
        wtk_covm_delete(gainnet_qform9->covm);
    }
    if(gainnet_qform9->covm_class[0]){
        wtk_covm_delete(gainnet_qform9->covm_class[0]);
        wtk_covm_delete(gainnet_qform9->covm_class[1]);
    }
    if(gainnet_qform9->bf){
        wtk_bf_delete(gainnet_qform9->bf);
    }
    if(gainnet_qform9->bf_class[0]){
        wtk_bf_delete(gainnet_qform9->bf_class[0]);
        wtk_bf_delete(gainnet_qform9->bf_class[1]);
    }
    if(gainnet_qform9->vdr){
        wtk_gainnet_qform9_edra_clean(gainnet_qform9->vdr);
    	wtk_free(gainnet_qform9->vdr);
    }
    if(gainnet_qform9->fftx){
    	wtk_free(gainnet_qform9->fftx);
    }
    wtk_free(gainnet_qform9);
}

void wtk_gainnet_qform9_reset(wtk_gainnet_qform9_t *gainnet_qform9)
{
    int channel=gainnet_qform9->bf->channel;
    int i;

    gainnet_qform9->end_pos=0;
    for(i=0;i<channel;++i)
    {
        memset(gainnet_qform9->notch_mem[i],0,2*sizeof(float));
    }
	memset(gainnet_qform9->memD,0,channel*sizeof(float));
    gainnet_qform9->memX=0;
    gainnet_qform9->memX_class[0]=0;
    gainnet_qform9->memX_class[1]=0;

    if(gainnet_qform9->input)
    {
        wtk_strbufs_reset(gainnet_qform9->input, gainnet_qform9->bf->channel);
    }

    if(gainnet_qform9->qenvelope)
    {
        wtk_qenvelope_reset(gainnet_qform9->qenvelope);
    }
    if(gainnet_qform9->qenvel[0]){
        wtk_qenvelope_reset(gainnet_qform9->qenvel[0]);
        wtk_qenvelope_reset(gainnet_qform9->qenvel[1]);
    }
    if(gainnet_qform9->q_fring){
        wtk_fring_reset(gainnet_qform9->q_fring);
        gainnet_qform9->q_spec=0;
        gainnet_qform9->right_nf=0;
    }
    if(gainnet_qform9->q_fring_class[0]){
        wtk_fring_reset(gainnet_qform9->q_fring_class[0]);
        wtk_fring_reset(gainnet_qform9->q_fring_class[1]);
        gainnet_qform9->q_spec_class[0]=0;
        gainnet_qform9->q_spec_class[1]=0;
        gainnet_qform9->right_nf_class[0]=0;
        gainnet_qform9->right_nf_class[1]=0;
    }
    if(gainnet_qform9->q_fring_class[2]){
        wtk_fring_reset(gainnet_qform9->q_fring_class[2]);
        gainnet_qform9->q_spec_class[2]=0;
        gainnet_qform9->right_nf_class[2]=0;
    }

    if(gainnet_qform9->aspec)
    {
        wtk_aspec_reset(gainnet_qform9->aspec);
    }

    if(gainnet_qform9->aspec_class[0])
    {
        wtk_aspec_reset(gainnet_qform9->aspec_class[0]);
        wtk_aspec_reset(gainnet_qform9->aspec_class[1]);
        wtk_aspec_reset(gainnet_qform9->aspec_class[2]);
    }

    wtk_queue_init(&(gainnet_qform9->stft2_q));
    wtk_stft2_reset(gainnet_qform9->stft2);
    wtk_covm_reset(gainnet_qform9->covm);
    if(gainnet_qform9->cfg->use_two_channel){
        wtk_covm_reset(gainnet_qform9->covm_class[0]);
        wtk_covm_reset(gainnet_qform9->covm_class[1]);
    }
    wtk_bf_reset(gainnet_qform9->bf);
    if(gainnet_qform9->cfg->use_two_channel){
        wtk_bf_reset(gainnet_qform9->bf_class[0]);
        wtk_bf_reset(gainnet_qform9->bf_class[1]);
    }

    gainnet_qform9->nframe=0;

    gainnet_qform9->theta=gainnet_qform9->phi=-1;

    if(gainnet_qform9->cfg->use_two_channel){
        memset(gainnet_qform9->cohv_class[0],0,sizeof(float)*gainnet_qform9->bf_class[0]->nbin);
        memset(gainnet_qform9->cohv_class[1],0,sizeof(float)*gainnet_qform9->bf_class[1]->nbin);
    }else if(gainnet_qform9->cfg->use_noise_qenvelope){
        memset(gainnet_qform9->cohv_class[0],0,sizeof(float)*gainnet_qform9->bf->nbin);
        memset(gainnet_qform9->cohv_class[1],0,sizeof(float)*gainnet_qform9->bf->nbin);
        memset(gainnet_qform9->cohv_class[2],0,sizeof(float)*gainnet_qform9->bf->nbin);
    }else{
        memset(gainnet_qform9->cohv,0,sizeof(float)*gainnet_qform9->bf->nbin);
    }
    if(gainnet_qform9->cfg->use_two_channel){
        wtk_strbuf_reset(gainnet_qform9->chn1_buf);
        wtk_strbuf_reset(gainnet_qform9->chn2_buf);
        wtk_strbuf_reset(gainnet_qform9->out_buf);
    }

    if(gainnet_qform9->qmmse)
    {
        wtk_qmmse_reset(gainnet_qform9->qmmse);
    }
    if(gainnet_qform9->qmmse_class[0]){
        wtk_qmmse_reset(gainnet_qform9->qmmse_class[0]);
        wtk_qmmse_reset(gainnet_qform9->qmmse_class[1]);
    }
    if(gainnet_qform9->vdr){
    	wtk_gainnet_qform9_edra_reset(gainnet_qform9->vdr);
    }
    if(gainnet_qform9->fftx){
    	memset(gainnet_qform9->fftx, 0, sizeof(wtk_complex_t)*(gainnet_qform9->nbin));
    }
}

void wtk_gainnet_qform9_set_notify(wtk_gainnet_qform9_t *gainnet_qform9,void *ths,wtk_gainnet_qform9_notify_f notify)
{
    gainnet_qform9->ths=ths;
    gainnet_qform9->notify=notify;
}

void wtk_gainnet_qform9_set_notify_two_channel(wtk_gainnet_qform9_t *gainnet_qform9,void *ths,wtk_gainnet_qform9_notify_two_channel_f notify)
{
    gainnet_qform9->ths_two_channel=ths;
    gainnet_qform9->notify_two_channel=notify;
}

void wtk_gainnet_qform9_denoise_on_gainnet(wtk_gainnet_qform9_edra_t *vdr, float *gain, int len, int is_end)
{
	memcpy(vdr->g, gain, sizeof(float)*vdr->cfg->bankfeat.nb_bands);
}

void wtk_gainnet_qform9_notify_data(wtk_gainnet_qform9_t *gainnet_qform9,float *data,int len, int chn)
{
    short *pv=(short *)data;
    int i;
    wtk_strbuf_t *chn1_buf, *chn2_buf, *out_buf;
    int out_len;

    if(gainnet_qform9->cfg->use_preemph && gainnet_qform9->cfg->use_two_channel)
    {
        if(!chn){
            gainnet_qform9->memX_class[0]=wtk_preemph_asis2(data,len,gainnet_qform9->memX_class[0]);
        }else{
            gainnet_qform9->memX_class[1]=wtk_preemph_asis2(data,len,gainnet_qform9->memX_class[1]);
        }
    }else if(gainnet_qform9->cfg->use_preemph)
    {
        gainnet_qform9->memX=wtk_preemph_asis2(data,len,gainnet_qform9->memX);
    }
    for(i=0;i<len;++i)
    {
        if(fabs(data[i])<1.0)
        {
            pv[i]=data[i]*32000;
        }else
        {
            if(data[i]>0)
            {
                pv[i]=32000;
            }else
            {
                pv[i]=-32000;
            }
        }
    }
    if(gainnet_qform9->cfg->use_two_channel){

        chn1_buf=gainnet_qform9->chn1_buf;
        chn2_buf=gainnet_qform9->chn2_buf;
        out_buf=gainnet_qform9->out_buf;

        if(!chn){
            wtk_strbuf_push(chn1_buf, (char *)pv, len<<1);
        }else{
            wtk_strbuf_push(chn2_buf, (char *)pv, len<<1);
        }

        out_len=min(chn1_buf->pos, chn2_buf->pos);
        if(out_len>0 && gainnet_qform9->notify_two_channel){  // 二维数组分别输出单通道音频
            short *pv_two[2];

            pv_two[0]=(short *)chn1_buf->data;
            pv_two[1]=(short *)chn2_buf->data;

            gainnet_qform9->notify_two_channel(gainnet_qform9->ths_two_channel,pv_two,out_len>>1,0);
            wtk_strbuf_pop(chn1_buf, NULL, out_len);
            wtk_strbuf_pop(chn2_buf, NULL, out_len);
        }else if(out_len>0 && gainnet_qform9->notify){  // 输出双通道音频
            short *pv1, *pv2;
            int i;
            int out_length;

            pv1=(short *)chn1_buf->data;
            pv2=(short *)chn2_buf->data;
            out_length=out_len>>1;
            for(i=0;i<out_length;++i){
                wtk_strbuf_push(out_buf, (char *)(pv1+i), sizeof(short));
                wtk_strbuf_push(out_buf, (char *)(pv2+i), sizeof(short));
            }

            gainnet_qform9->notify(gainnet_qform9->ths,(short *)out_buf->data,out_buf->pos>>1,0);
            wtk_strbuf_reset(out_buf);
            wtk_strbuf_pop(chn1_buf, NULL, out_len);
            wtk_strbuf_pop(chn2_buf, NULL, out_len);
        }
    }else{
        if(gainnet_qform9->notify)
        {
            gainnet_qform9->notify(gainnet_qform9->ths,pv,len,0);
        }
    }
}

void wtk_gainnet_qform9_edra_feed(wtk_gainnet_qform9_t *gainnet_qform9, wtk_stft2_msg_t *smsg, float gbias)
{
	int i;
    wtk_gainnet_qform9_edra_t *vdr=gainnet_qform9->vdr;
	int nbin=vdr->nbin;
	float *g=vdr->g, *gf=vdr->gf;
	float *lastg=vdr->lastg;
    float ralpha=vdr->cfg->ralpha;
	wtk_gainnet2_t *gainnet2=vdr->gainnet2;
	wtk_bankfeat_t *bank_mic=vdr->bank_mic;
	int nb_bands=bank_mic->cfg->nb_bands;
	int nb_features=bank_mic->cfg->nb_features;
	wtk_qmmse_t *qmmse=vdr->qmmse;
	int featm_lm=vdr->cfg->featm_lm;
	float *feature_lm=vdr->feature_lm;
    wtk_complex_t **s_fft=smsg->fft;
    wtk_complex_t *fftx=gainnet_qform9->fftx;
    float scale = 32768.0/gainnet_qform9->stft2->cfg->win;
	int channel=gainnet_qform9->stft2->cfg->channel;
	int clip_s=gainnet_qform9->cfg->clip_s;
	int clip_e=gainnet_qform9->cfg->clip_e;
    int k;
	float gf_k;
    
    for(i=0;i<nbin;++i){
        fftx[i].a = s_fft[i][0].a*scale;
        fftx[i].b = s_fft[i][0].b*scale;
    }

	wtk_bankfeat_flush_frame_features(bank_mic, fftx);
	if(qmmse)
	{
		wtk_qmmse_flush_denoise_mask(qmmse, fftx);
	}
    // wtk_gainnet2_feed(gainnet2, bank_mic->features, nb_features, nb_features, 0);  
    wtk_gainnet2_feed2(gainnet2, bank_mic->features, nb_features, feature_lm, nb_features*(featm_lm-1), 0);  
    for(i=0; i<nb_bands; ++i)
    {
        g[i]=max(g[i],lastg[i]*ralpha);
        lastg[i]=g[i];
    }

    wtk_bankfeat_interp_band_gain(bank_mic, nbin, gf, g);
    if(gbias>0)
    {
        for (i=1; i<nbin-1; ++i)
        {
            gf[i]=min(gf[i]+gbias,1);
        }
    }
	if(feature_lm && featm_lm>1)
	{
		memmove(feature_lm+nb_features,feature_lm,sizeof(float)*nb_features*(featm_lm-2));
		memcpy(feature_lm,bank_mic->features,sizeof(float)*nb_features);
	}

    for(k=clip_s+1; k<clip_e; ++k)
    {
        gf_k=vdr->gf[k];
        for(i=0; i<channel; ++i)
        {
            smsg->fft[k][i].a*=gf_k;
            smsg->fft[k][i].b*=gf_k;
        }
    }
	for(k=0; k<=clip_s; ++k)
	{
        memset(smsg->fft[k], 0, sizeof(wtk_complex_t)*channel);
	}
	for(k=clip_e; k<nbin; ++k)
	{
        memset(smsg->fft[k], 0, sizeof(wtk_complex_t)*channel);
	}
}

void wtk_gainnet_qform9_flush2(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *smsg,float *cohv, int chn, int is_end)
{
    int k;
    wtk_complex_t *bf_out=NULL;
    static int state=0;
    // int nbin=gainnet_qform9->bf->nbin;
    if(gainnet_qform9->cfg->use_two_channel){
        if(!chn){
            if(smsg)
            {
                bf_out=wtk_bf_output_fft2_msg2(gainnet_qform9->bf_class[0],smsg,cohv);
                if(gainnet_qform9->qmmse_class[0])
                {
                    wtk_qmmse_feed_cohv(gainnet_qform9->qmmse_class[0],bf_out,cohv);
                }
            }
            if(gainnet_qform9->notify || gainnet_qform9->notify_two_channel)
            {
                if(bf_out)
                {
                    k=wtk_stft2_output_ifft(gainnet_qform9->stft2,bf_out,gainnet_qform9->stft2->output,gainnet_qform9->bf_class[0]->pad,gainnet_qform9->end_pos,is_end);
                    wtk_gainnet_qform9_notify_data(gainnet_qform9,gainnet_qform9->stft2->output,k,0);
                }
                if(is_end)
                {
                    ++state;
                }
            }else if(gainnet_qform9->notify2)
            {
                gainnet_qform9->notify2(gainnet_qform9->ths2, bf_out, is_end);
            }
        }else{
            if(smsg)
            {
                bf_out=wtk_bf_output_fft2_msg2(gainnet_qform9->bf_class[1],smsg,cohv);
                if(gainnet_qform9->qmmse_class[1])
                {
                    wtk_qmmse_feed_cohv(gainnet_qform9->qmmse_class[1],bf_out,cohv);
                }
            }
            if(gainnet_qform9->notify || gainnet_qform9->notify_two_channel)
            {
                if(bf_out)
                {
                    k=wtk_stft2_output_ifft(gainnet_qform9->stft2,bf_out,gainnet_qform9->stft2->output,gainnet_qform9->bf_class[1]->pad,gainnet_qform9->end_pos,is_end);
                    wtk_gainnet_qform9_notify_data(gainnet_qform9,gainnet_qform9->stft2->output,k,1);
                }
                if(is_end)
                {
                    ++state;
                }
            }else if(gainnet_qform9->notify2)
            {
                gainnet_qform9->notify2(gainnet_qform9->ths2, bf_out, is_end);
            }
        }
        if(state==2 && gainnet_qform9->notify_two_channel){
            gainnet_qform9->notify_two_channel(gainnet_qform9->ths_two_channel,NULL,0,1);
        }else if(state==2 && gainnet_qform9->notify){
            gainnet_qform9->notify(gainnet_qform9->ths,NULL,0,1);
        }
    }else{
        if(smsg)
        {
            bf_out=wtk_bf_output_fft2_msg2(gainnet_qform9->bf,smsg,cohv);
            if(gainnet_qform9->qmmse)
            {
                wtk_qmmse_feed_cohv(gainnet_qform9->qmmse,bf_out,cohv);      
            }
        }
        if(gainnet_qform9->notify)
        {
            if(bf_out)
            {
                k=wtk_stft2_output_ifft(gainnet_qform9->stft2,bf_out,gainnet_qform9->stft2->output,gainnet_qform9->bf->pad,gainnet_qform9->end_pos,is_end);
                wtk_gainnet_qform9_notify_data(gainnet_qform9,gainnet_qform9->stft2->output,k,0);
            }
            if(is_end)
            {
                gainnet_qform9->notify(gainnet_qform9->ths,NULL,0,1);
            }
        }else if(gainnet_qform9->notify2)
        {
            gainnet_qform9->notify2(gainnet_qform9->ths2, bf_out, is_end);
        }
    }
 
}

void wtk_gainnet_qform9_flush(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *smsg, float *cohv, int chn, int is_end)
{
    int k;
    int nbin=gainnet_qform9->bf->nbin;
    int i, channel=gainnet_qform9->bf->channel;
    int b;
    wtk_covm_t *covm=gainnet_qform9->covm;
    wtk_covm_t *covm_class[2];
    covm_class[0]=gainnet_qform9->covm_class[0];
    covm_class[1]=gainnet_qform9->covm_class[1];

    if(gainnet_qform9->cfg->use_two_channel){
        if(!chn){
            for(k=1; k<nbin-1; ++k)
            {
                b=0;
                if(cohv[k]<0.0)
                {
                    b=wtk_covm_feed_fft2(covm_class[0], smsg->fft, k, 1);
                    if(b==1)
                    {
                        wtk_bf_update_ncov(gainnet_qform9->bf_class[0], covm_class[0]->ncov, k);
                    }
                }else
                {
                    if(covm_class[0]->scov)
                    {
                        b=wtk_covm_feed_fft2(covm_class[0], smsg->fft, k, 0);
                        if(b==1)
                        {
                            wtk_bf_update_scov(gainnet_qform9->bf_class[0], covm_class[0]->scov, k);
                        }
                    }
                }
                if(covm_class[0]->ncnt_sum[k]>5 && (covm_class[0]->scnt_sum==NULL ||  covm_class[0]->scnt_sum[k]>5) && b==1)
                {
                    wtk_bf_update_w(gainnet_qform9->bf_class[0], k);
                }

                if(gainnet_qform9->cfg->debug)
                {
                    if(cohv[k]<0)
                    {
                        for(i=0;i<channel;++i)
                        {
                            gainnet_qform9->bf_class[0]->w[k][i].a=0;
                            gainnet_qform9->bf_class[0]->w[k][i].b=0;
                        }
                    }else
                    {
                        for(i=0;i<channel;++i)
                        {
                            gainnet_qform9->bf_class[0]->w[k][i].a=0;
                            gainnet_qform9->bf_class[0]->w[k][i].b=0;
                            if(i==0)
                            {
                                gainnet_qform9->bf_class[0]->w[k][i].a=1;
                            }
                        }
                    }
                }
            }
            wtk_gainnet_qform9_flush2(gainnet_qform9,smsg,cohv,0,is_end);
        }else{
            for(k=1; k<nbin-1; ++k)
            {
                b=0;
                if(cohv[k]<0.0)
                {
                    b=wtk_covm_feed_fft2(covm_class[1], smsg->fft, k, 1);
                    if(b==1)
                    {
                        wtk_bf_update_ncov(gainnet_qform9->bf_class[1], covm_class[1]->ncov, k);
                    }
                }else
                {
                    if(covm_class[1]->scov)
                    {
                        b=wtk_covm_feed_fft2(covm_class[1], smsg->fft, k, 0);
                        if(b==1)
                        {
                            wtk_bf_update_scov(gainnet_qform9->bf_class[1], covm_class[1]->scov, k);
                        }
                    }
                }
                if(covm_class[1]->ncnt_sum[k]>5 && (covm_class[1]->scnt_sum==NULL ||  covm_class[1]->scnt_sum[k]>5) && b==1)
                {
                    wtk_bf_update_w(gainnet_qform9->bf_class[1], k);
                }

                if(gainnet_qform9->cfg->debug)
                {
                    if(cohv[k]<0)
                    {
                        for(i=0;i<channel;++i)
                        {
                            gainnet_qform9->bf_class[1]->w[k][i].a=0;
                            gainnet_qform9->bf_class[1]->w[k][i].b=0;
                        }
                    }else
                    {
                        for(i=0;i<channel;++i)
                        {
                            gainnet_qform9->bf_class[1]->w[k][i].a=0;
                            gainnet_qform9->bf_class[1]->w[k][i].b=0;
                            if(i==0)
                            {
                                gainnet_qform9->bf_class[1]->w[k][i].a=1;
                            }
                        }
                    }
                }
            }
            wtk_gainnet_qform9_flush2(gainnet_qform9,smsg,cohv,1,is_end);
        }
    }else{
        for(k=1; k<nbin-1; ++k)
        {
            b=0;
            if(cohv[k]<0.0)
            {
                b=wtk_covm_feed_fft2(covm, smsg->fft, k, 1);
                if(b==1)
                {
                    wtk_bf_update_ncov(gainnet_qform9->bf, covm->ncov, k);
                }
            }else
            {
                if(covm->scov)
                {
                    b=wtk_covm_feed_fft2(covm, smsg->fft, k, 0);
                    if(b==1)
                    {
                        wtk_bf_update_scov(gainnet_qform9->bf, covm->scov, k);
                    }
                }
            }
            if(covm->ncnt_sum[k]>5 && (covm->scnt_sum==NULL ||  covm->scnt_sum[k]>5) && b==1)
            {
                wtk_bf_update_w(gainnet_qform9->bf, k);
            }

            if(gainnet_qform9->cfg->debug)
            {
                if(cohv[k]<0)
                {
                    for(i=0;i<channel;++i)
                    {
                        gainnet_qform9->bf->w[k][i].a=0;
                        gainnet_qform9->bf->w[k][i].b=0;
                    }
                }else
                {
                    for(i=0;i<channel;++i)
                    {
                        gainnet_qform9->bf->w[k][i].a=0;
                        gainnet_qform9->bf->w[k][i].b=0;
                        if(i==0)
                        {
                            gainnet_qform9->bf->w[k][i].a=1;
                        }
                    }
                }
            }
        }
        if(gainnet_qform9->vdr){
            wtk_gainnet_qform9_edra_feed(gainnet_qform9, smsg, gainnet_qform9->cfg->gbias);
        }
        wtk_gainnet_qform9_flush2(gainnet_qform9,smsg,cohv,0,is_end);
    }
}
void wtk_gainnet_qform9_on_qenvel1(wtk_gainnet_qform9_t *gainnet_qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_gainnet_qform9_envelopemsg_t *qemsg;
    int k;
    int nbin=gainnet_qform9->nbin;

    if(msg)
    {
        qemsg=(wtk_gainnet_qform9_envelopemsg_t *)msg->hook;
        if(state==WTK_QENVELOPE_TROUGH)
        {
            for(k=1; k<nbin-1; ++k)
            {
                qemsg->cohv[k]=-1;
            }
        }else if(state==WTK_QENVELOPE_CREST || state==WTK_QENVELOPE_FLAT)
        {
            if(gainnet_qform9->cfg->use_sqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=1;
                }
            }
        }
        wtk_gainnet_qform9_flush(gainnet_qform9, qemsg->smsg, qemsg->cohv, 0, is_end);
        wtk_gainnet_qform9_push_envelope_msg(gainnet_qform9, qemsg);
    }else if(is_end)
    {
        wtk_gainnet_qform9_flush2(gainnet_qform9, NULL, NULL, 0, 1);
    }
}
void wtk_gainnet_qform9_on_qenvel2(wtk_gainnet_qform9_t *gainnet_qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_gainnet_qform9_envelopemsg_t *qemsg;
    int k;
    int nbin=gainnet_qform9->nbin;

    if(msg)
    {
        qemsg=(wtk_gainnet_qform9_envelopemsg_t *)msg->hook;
        if(state==WTK_QENVELOPE_TROUGH)
        {
            for(k=1; k<nbin-1; ++k)
            {
                qemsg->cohv[k]=-1;
            }
        }else if(state==WTK_QENVELOPE_CREST || state==WTK_QENVELOPE_FLAT)
        {
            if(gainnet_qform9->cfg->use_sqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=1;
                }
            }
        }
        wtk_gainnet_qform9_flush(gainnet_qform9, qemsg->smsg, qemsg->cohv, 1, is_end);
        wtk_gainnet_qform9_push_envelope_msg(gainnet_qform9, qemsg);
    }else if(is_end)
    {
        wtk_gainnet_qform9_flush2(gainnet_qform9, NULL, NULL, 1, 1);
    }
}
void wtk_gainnet_qform9_on_qenvelope(wtk_gainnet_qform9_t *gainnet_qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_gainnet_qform9_envelopemsg_t *qemsg;
    int k;
    int nbin=gainnet_qform9->nbin;

    if(msg)
    {
        qemsg=(wtk_gainnet_qform9_envelopemsg_t *)msg->hook;
        if(state==WTK_QENVELOPE_TROUGH)
        {
            for(k=1; k<nbin-1; ++k)
            {
                qemsg->cohv[k]=-1;
            }
        }else if(state==WTK_QENVELOPE_CREST || state==WTK_QENVELOPE_FLAT)
        {
            if(gainnet_qform9->cfg->use_sqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=1;
                }
            }
        }
        wtk_gainnet_qform9_flush(gainnet_qform9, qemsg->smsg, qemsg->cohv, 0, is_end);
        wtk_gainnet_qform9_push_envelope_msg(gainnet_qform9, qemsg);
    }else if(is_end)
    {
        wtk_gainnet_qform9_flush2(gainnet_qform9, NULL, NULL, 0, 1);
    }
}

void wtk_gainnet_qform9_update_aspec2(wtk_gainnet_qform9_t *gainnet_qform9, wtk_aspec_t *aspec, wtk_complex_t *cov, 
                                                                                    wtk_complex_t *inv_cov, float cov_travg, int k, float *spec_k, float *cohv)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
    }

    *cohv=1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            *cohv=-1;
        }
    }
    if(gainnet_qform9->cfg->cohv_thresh>0.0 && spec_k[0]<gainnet_qform9->cfg->cohv_thresh)
    {
        *cohv=-1;
    }
}


void wtk_gainnet_qform9_update_naspec2(wtk_gainnet_qform9_t *gainnet_qform9, wtk_aspec_t *naspec, wtk_complex_t *cov, 
                                                                                    wtk_complex_t *inv_cov, float cov_travg, int k, float *spec_k, float *cohv)
{
    int sang_num=naspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(naspec, NULL, 0, cov_travg, cov, inv_cov, k ,n);
    }

    *cohv=-1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            *cohv=1;
        }
    }
}

void wtk_gainnet_qform9_flush_aspec_lt(wtk_gainnet_qform9_t *gainnet_qform9, int index, int is_end)
{
    // float **mic_class=gainnet_qform9->cfg->mic_class;
    // wtk_complex_t *covclass, *invcovclass;
    wtk_queue_t *stft2_q=&(gainnet_qform9->stft2_q);
    int lf=gainnet_qform9->cfg->lf;
    int lt=gainnet_qform9->cfg->lt;
    // int i,j,k,k2,tt,ff,c,c2;
    int i,j,k,k2,tt,ff;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg,*smsg_index;
    int nbin=gainnet_qform9->nbin;
    // int channel=gainnet_qform9->bf->channel, channel2;
    int channel=gainnet_qform9->bf->channel;
    wtk_complex_t *cov=gainnet_qform9->cov;
    wtk_complex_t **fft,*fft1,*fft2,*a,*b;
    float *wint=gainnet_qform9->wint;
    float *winf=gainnet_qform9->winf;
    float wint2,wintf,winsum;
    wtk_complex_t *inv_cov=gainnet_qform9->inv_cov;
    wtk_dcomplex_t *tmp=gainnet_qform9->tmp;
    float cov_travg;
    int ret;
    float *cohv=gainnet_qform9->cohv;
    float *cohv_class[3];
    float cohvtmp1,cohvtmp2,cohvtmp3;
    float spec_k[3]={0}, specsum;
    float specsum1, specsum2, specsum3;
    int specsum_ns=gainnet_qform9->cfg->specsum_ns;
    int specsum_ne=gainnet_qform9->cfg->specsum_ne;
    float max_spec;
    float qenvel_alpha = gainnet_qform9->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;

    ++gainnet_qform9->nframe;
    qn=wtk_queue_peek(stft2_q, index);
    smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);

    specsum=0;
    specsum1=specsum2=specsum3=0;
    cohv_class[0]=gainnet_qform9->cohv_class[0];
    cohv_class[1]=gainnet_qform9->cohv_class[1];
    if(gainnet_qform9->cohv_class[2]){
        cohv_class[2]=gainnet_qform9->cohv_class[2];
    }
    for(k=1;k<nbin-1;++k)
    {
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        winsum=0;
        for(qn=stft2_q->pop,tt=2*lt+1-stft2_q->length;qn;qn=qn->next,++tt)
        {
            wint2=wint[tt];
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            fft=smsg->fft;
            for(k2=max(1,k-lf),ff=k2-(k-lf);k2<min(nbin-1,k+lf+1);++k2,++ff)
            {
                wintf=wint2*winf[ff];
                winsum+=wintf;

                fft1=fft2=fft[k2];
                for(i=0;i<channel;++i,++fft1)
                {
                    fft2=fft1;
                    for(j=i;j<channel;++j,++fft2)
                    {
                        a=cov+i*channel+j;
                        if(i!=j)
                        {
                            a->a+=(fft1->a*fft2->a+fft1->b*fft2->b)*wintf;
                            a->b+=(-fft1->a*fft2->b+fft1->b*fft2->a)*wintf;
                        }else
                        {
                            a->a+=(fft1->a*fft2->a+fft1->b*fft2->b)*wintf;
                            a->b+=0;
                        }
                    }
                }
            }
        }
        winsum=1.0/winsum;
        for(i=0;i<channel;++i)
        {
            for(j=i;j<channel;++j)
            {
                a=cov+i*channel+j;
                a->a*=winsum;
                a->b*=winsum;

                if(i!=j)
                {
                    b=cov+j*channel+i;
                    b->a=a->a;
                    b->b=-a->b;
                }
            }
        }
        if(inv_cov)
        {
            ret=wtk_complex_invx4(cov,tmp,channel,inv_cov,1);            
            if(ret!=0)
            {
                j=0;
                for(i=0;i<channel;++i)
                {
                    cov[j].a+=0.01;
                    j+=channel+1;
                }
                wtk_complex_invx4(cov,tmp,channel,inv_cov,1);
            }
        }

        if(gainnet_qform9->aspec)
        {
            cov_travg=0;
            if(gainnet_qform9->aspec->need_cov_travg) 
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }

            wtk_gainnet_qform9_update_aspec2(gainnet_qform9, gainnet_qform9->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv+k);

            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
        }else if(gainnet_qform9->aspec_class[0] && (gainnet_qform9->cfg->use_two_channel || gainnet_qform9->cfg->use_noise_qenvelope))
        {
            cov_travg=0;
            if(gainnet_qform9->aspec_class[0]->need_cov_travg)
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }
            wtk_gainnet_qform9_update_aspec2(gainnet_qform9, gainnet_qform9->aspec_class[0], cov, inv_cov, cov_travg, k, spec_k, cohv_class[0]+k);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum1+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }

            wtk_gainnet_qform9_update_aspec2(gainnet_qform9, gainnet_qform9->aspec_class[1], cov, inv_cov, cov_travg, k, spec_k, cohv_class[1]+k);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum2+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
        }else if(gainnet_qform9->aspec_class[0])
        {
            cov_travg=0;
            if(gainnet_qform9->aspec_class[0]->need_cov_travg)
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }
            wtk_gainnet_qform9_update_aspec2(gainnet_qform9, gainnet_qform9->aspec_class[0], cov, inv_cov, cov_travg, k, spec_k, &cohvtmp1);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum1+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
            wtk_gainnet_qform9_update_aspec2(gainnet_qform9, gainnet_qform9->aspec_class[1], cov, inv_cov, cov_travg, k, spec_k, &cohvtmp2);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum2+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
            wtk_gainnet_qform9_update_aspec2(gainnet_qform9, gainnet_qform9->aspec_class[2], cov, inv_cov, cov_travg, k, spec_k, &cohvtmp3);
            if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum3+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
            cohv[k] = (cohvtmp1 + cohvtmp2 + cohvtmp3) > -3?1:0;
        }
    }
    if(gainnet_qform9->cfg->use_simple_qenvelope){
        if(gainnet_qform9->cfg->use_two_channel){
            wtk_fring_push2(gainnet_qform9->q_fring_class[0], specsum1);
            wtk_fring_push2(gainnet_qform9->q_fring_class[1], specsum2);
            for(i=0;i<2;++i){
                if(i==0){
                    min_speccrest = gainnet_qform9->cfg->qenvl.min_speccrest;
                    right_nf = gainnet_qform9->cfg->qenvl.right_nf;
                    envelope_thresh = gainnet_qform9->cfg->qenvl.envelope_thresh;
                    right_min_thresh = gainnet_qform9->cfg->qenvl.right_min_thresh;
                }else if(i==1){
                    min_speccrest = gainnet_qform9->cfg->qenvl2.min_speccrest;
                    right_nf = gainnet_qform9->cfg->qenvl2.right_nf;
                    envelope_thresh = gainnet_qform9->cfg->qenvl2.envelope_thresh;
                    right_min_thresh = gainnet_qform9->cfg->qenvl2.right_min_thresh;
                }
                if(gainnet_qform9->q_fring_class[0]->used == gainnet_qform9->q_fring_class[i]->nslot - 1){
                    max_spec = wtk_fring_max(gainnet_qform9->q_fring_class[i]);
                    if(max_spec < gainnet_qform9->q_spec_class[i]){
                        max_spec = qenvel_alpha * gainnet_qform9->q_spec_class[i] + qenvel_alpha_1 * max_spec; 
                    }
                    // printf("%f\n", max_spec);
                    gainnet_qform9->q_spec_class[i] = max_spec;
                    if(max_spec > min_speccrest){
                        gainnet_qform9->right_nf_class[i] = right_nf;
                    }else if(max_spec > envelope_thresh){
                        // --gainnet_qform9->right_nf;
                    }else if(max_spec > right_min_thresh){
                        --gainnet_qform9->right_nf_class[i];
                    }else{
                        gainnet_qform9->right_nf_class[i] = 0;
                    }
                    if(gainnet_qform9->right_nf_class[i] <= 0){
                        for(k=1; k<nbin-1; ++k)
                        {
                            cohv_class[i][k]=-1;
                        }
                    }else{
                        if(gainnet_qform9->cfg->use_sqenvelope){
                            for(k=1; k<nbin-1; ++k)
                            {
                                cohv_class[i][k]=1;
                            }
                        }
                    }
                }
                wtk_gainnet_qform9_flush(gainnet_qform9, smsg_index, cohv_class[i], i, is_end);
            }
        }else if(gainnet_qform9->cfg->use_noise_qenvelope){
            wtk_fring_push2(gainnet_qform9->q_fring_class[0], specsum1);
            wtk_fring_push2(gainnet_qform9->q_fring_class[1], specsum2);
            wtk_fring_push2(gainnet_qform9->q_fring_class[2], specsum1);
            for(i=0;i<3;++i){
                if(i==0){
                    min_speccrest = gainnet_qform9->cfg->qenvl.min_speccrest;
                    right_nf = gainnet_qform9->cfg->qenvl.right_nf;
                    envelope_thresh = gainnet_qform9->cfg->qenvl.envelope_thresh;
                    right_min_thresh = gainnet_qform9->cfg->qenvl.right_min_thresh;
                }else if(i==1){
                    min_speccrest = gainnet_qform9->cfg->qenvl2.min_speccrest;
                    right_nf = gainnet_qform9->cfg->qenvl2.right_nf;
                    envelope_thresh = gainnet_qform9->cfg->qenvl2.envelope_thresh;
                    right_min_thresh = gainnet_qform9->cfg->qenvl2.right_min_thresh;
                }else if(i==2){
                    min_speccrest = gainnet_qform9->cfg->qenvl3.min_speccrest;
                    right_nf = gainnet_qform9->cfg->qenvl3.right_nf;
                    envelope_thresh = gainnet_qform9->cfg->qenvl3.envelope_thresh;
                    right_min_thresh = gainnet_qform9->cfg->qenvl3.right_min_thresh;
                }
                if(gainnet_qform9->q_fring_class[i]->used == gainnet_qform9->q_fring_class[i]->nslot - 1){
                    max_spec = wtk_fring_max(gainnet_qform9->q_fring_class[i]);
                    if(max_spec < gainnet_qform9->q_spec_class[i]){
                        max_spec = qenvel_alpha * gainnet_qform9->q_spec_class[i] + qenvel_alpha_1 * max_spec; 
                    }
                    // printf("%f\n", max_spec);
                    gainnet_qform9->q_spec_class[i] = max_spec;
                    if(max_spec > min_speccrest){
                        gainnet_qform9->right_nf_class[i] = right_nf;
                    }else if(max_spec > envelope_thresh){
                        // --gainnet_qform9->right_nf;
                    }else if(max_spec > right_min_thresh){
                        --gainnet_qform9->right_nf_class[i];
                    }else{
                        gainnet_qform9->right_nf_class[i] = 0;
                    }
                    if(gainnet_qform9->right_nf_class[i] <= 0){
                        for(k=1; k<nbin-1; ++k)
                        {
                            cohv_class[i][k]=-1;
                        }
                    }else{
                        if(gainnet_qform9->cfg->use_sqenvelope){
                            for(k=1; k<nbin-1; ++k)
                            {
                                cohv_class[i][k]=1;
                            }
                        }
                    }
                }
            }
            for(k=1; k<nbin-1; ++k)
            {
                if(cohv_class[1][k]==1 && cohv_class[2][k]==-1){
                    cohv_class[0][k]=-1;
                }
            }
            wtk_gainnet_qform9_flush(gainnet_qform9, smsg_index, cohv_class[0], 0, is_end);
        }else{
            wtk_fring_push2(gainnet_qform9->q_fring, specsum);
            min_speccrest = gainnet_qform9->cfg->qenvl.min_speccrest;
            right_nf = gainnet_qform9->cfg->qenvl.right_nf;
            envelope_thresh = gainnet_qform9->cfg->qenvl.envelope_thresh;
            right_min_thresh = gainnet_qform9->cfg->qenvl.right_min_thresh;
            if(gainnet_qform9->q_fring->used == gainnet_qform9->q_fring->nslot - 1){
                max_spec = wtk_fring_max(gainnet_qform9->q_fring);
                if(max_spec < gainnet_qform9->q_spec){
                    max_spec = qenvel_alpha * gainnet_qform9->q_spec + qenvel_alpha_1 * max_spec; 
                }
                if(gainnet_qform9->cohv_fn)
                {
                    fprintf(gainnet_qform9->cohv_fn,"%.0f %f\n",gainnet_qform9->nframe,max_spec);
                }
                // printf("%f\n", max_spec);
                gainnet_qform9->q_spec = max_spec;
                if(max_spec > min_speccrest){
                    gainnet_qform9->right_nf = right_nf;
                }else if(max_spec > envelope_thresh){
                    // --gainnet_qform9->right_nf;
                }else if(max_spec > right_min_thresh){
                    --gainnet_qform9->right_nf;
                }else{
                    gainnet_qform9->right_nf = 0;
                }
                if(gainnet_qform9->right_nf <= 0){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=-1;
                    }
                }else{
                    if(gainnet_qform9->cfg->use_sqenvelope){
                        for(k=1; k<nbin-1; ++k)
                        {
                            cohv[k]=1;
                        }
                    }
                }
            }
            wtk_gainnet_qform9_flush(gainnet_qform9, smsg_index, cohv, 0, is_end);
        }
    }else if(gainnet_qform9->cfg->use_qenvelope && gainnet_qform9->cfg->use_two_channel)
    {
        wtk_gainnet_qform9_envelopemsg_t *qemsg1;
        wtk_gainnet_qform9_envelopemsg_t *qemsg2;

        if(!is_end){
            qemsg1=wtk_gainnet_qform9_envelope_msg_copy(gainnet_qform9, smsg_index, cohv_class[0], nbin, channel);
            wtk_qenvelope_feed(gainnet_qform9->qenvel[0], specsum1, (void *)qemsg1, is_end);
            qemsg2=wtk_gainnet_qform9_envelope_msg_copy(gainnet_qform9, smsg_index, cohv_class[1], nbin, channel);
            wtk_qenvelope_feed(gainnet_qform9->qenvel[1], specsum2, (void *)qemsg2, is_end);
        }else{
            wtk_qenvelope_feed(gainnet_qform9->qenvel[0], 0, NULL, 1);
            wtk_gainnet_qform9_flush(gainnet_qform9, smsg_index, cohv_class[0], 0, 1);
            wtk_qenvelope_feed(gainnet_qform9->qenvel[1], 0, NULL, 1);
            wtk_gainnet_qform9_flush(gainnet_qform9, smsg_index, cohv_class[1], 1, 1);
        }
    }else if(gainnet_qform9->cfg->use_qenvelope)
    {
        wtk_gainnet_qform9_envelopemsg_t *qemsg;

        if(!is_end){
            qemsg=wtk_gainnet_qform9_envelope_msg_copy(gainnet_qform9, smsg_index, cohv, nbin, channel);
            wtk_qenvelope_feed(gainnet_qform9->qenvelope, specsum, (void *)qemsg, is_end);
        }else{
            wtk_qenvelope_feed(gainnet_qform9->qenvelope, 0, NULL, 1);
            wtk_gainnet_qform9_flush(gainnet_qform9, smsg_index, cohv, 0, is_end);
        }
    }else
    {
        if(gainnet_qform9->cohv_fn)
        {
            fprintf(gainnet_qform9->cohv_fn,"%.0f %f\n",gainnet_qform9->nframe,specsum);
        }
        wtk_gainnet_qform9_flush(gainnet_qform9, smsg_index, cohv, 0, is_end);
    }
}


void wtk_gainnet_qform9_update_aspec(wtk_gainnet_qform9_t *gainnet_qform9, wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, int k, float *spec_k, float *cohv)
{
    int sang_num=aspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(aspec, fft, fftabs2, 0, NULL, NULL, k ,n);
    }

    *cohv=1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            *cohv=-1;
        }
    }
    if(gainnet_qform9->cfg->cohv_thresh>0.0 && spec_k[0]<gainnet_qform9->cfg->cohv_thresh)
    {
        *cohv=-1;
    }
}


void wtk_gainnet_qform9_update_naspec(wtk_gainnet_qform9_t *gainnet_qform9, wtk_aspec_t *naspec, wtk_complex_t **fft, float fftabs2, int k, float *spec_k, float *cohv)
{
    int sang_num=naspec->start_ang_num;
    int n;

    for(n=0; n<sang_num; ++n)
    {
        spec_k[n]=wtk_aspec_flush_spec_k(naspec, fft, fftabs2, 0, NULL, NULL, k ,n);
    }

    *cohv=-1;
    for(n=1; n<sang_num; ++n)
    {
        if(spec_k[0]<=spec_k[n])
        {
            *cohv=1;
        }
    }
}

void wtk_gainnet_qform9_flush_aspec(wtk_gainnet_qform9_t *gainnet_qform9, wtk_stft2_msg_t *msg, int is_end)
{
    int k,i;
    int nbin=gainnet_qform9->nbin;
    int channel=gainnet_qform9->bf->channel;
    wtk_complex_t **fft, *fft2;
    float fftabs2;
    float spec_k[3]={0}, specsum;
    float *cohv=gainnet_qform9->cohv;
    int specsum_ns=gainnet_qform9->cfg->specsum_ns;
    int specsum_ne=gainnet_qform9->cfg->specsum_ne;
    float max_spec;
    float qenvel_alpha = gainnet_qform9->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;

    ++gainnet_qform9->nframe;

    fft=msg->fft;

    specsum=0;
    for(k=1; k<nbin-1; ++k)
    {
        if(gainnet_qform9->aspec)
        {
            fftabs2=0;
            fft2=fft[k];
            for(i=0; i<channel; ++i,++fft2)
            {
                fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
            }

            wtk_gainnet_qform9_update_aspec(gainnet_qform9,gainnet_qform9->aspec,fft,fftabs2,k,spec_k,cohv+k);

            if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
        }
    }

    if(gainnet_qform9->cfg->use_simple_qenvelope){
        wtk_fring_push2(gainnet_qform9->q_fring, specsum);
        min_speccrest = gainnet_qform9->cfg->qenvl.min_speccrest;
        right_nf = gainnet_qform9->cfg->qenvl.right_nf;
        envelope_thresh = gainnet_qform9->cfg->qenvl.envelope_thresh;
        right_min_thresh = gainnet_qform9->cfg->qenvl.right_min_thresh;
        if(gainnet_qform9->q_fring->used == gainnet_qform9->q_fring->nslot - 1){
            max_spec = wtk_fring_max(gainnet_qform9->q_fring);
            if(max_spec < gainnet_qform9->q_spec){
                max_spec = qenvel_alpha * gainnet_qform9->q_spec + qenvel_alpha_1 * max_spec; 
            }
            if(gainnet_qform9->cohv_fn)
            {
                fprintf(gainnet_qform9->cohv_fn,"%.0f %f\n",gainnet_qform9->nframe,max_spec);
            }
            // printf("%f\n", max_spec);
            gainnet_qform9->q_spec = max_spec;
            if(max_spec > min_speccrest){
                gainnet_qform9->right_nf = right_nf;
            }else if(max_spec > envelope_thresh){
                // --gainnet_qform9->right_nf;
            }else if(max_spec > right_min_thresh){
                --gainnet_qform9->right_nf;
            }else{
                gainnet_qform9->right_nf = 0;
            }
            if(gainnet_qform9->right_nf <= 0){
                for(k=1; k<nbin-1; ++k)
                {
                    cohv[k]=-1;
                }
            }else{
                if(gainnet_qform9->cfg->use_sqenvelope){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=1;
                    }
                }
            }
        }
        wtk_gainnet_qform9_flush(gainnet_qform9, msg, cohv, 0, is_end);
    }else if(gainnet_qform9->cfg->use_qenvelope)
    {
        wtk_gainnet_qform9_envelopemsg_t *qemsg;

        if(!is_end){
            qemsg=wtk_gainnet_qform9_envelope_msg_copy(gainnet_qform9, msg, cohv, nbin, channel);
            wtk_qenvelope_feed(gainnet_qform9->qenvelope, specsum, (void *)qemsg, is_end);
        }else{
            wtk_qenvelope_feed(gainnet_qform9->qenvelope, 0, NULL, 1);
            wtk_gainnet_qform9_flush(gainnet_qform9, msg, cohv, 0, is_end);
        }
    }else
    {
        if(gainnet_qform9->cohv_fn)
        {
            fprintf(gainnet_qform9->cohv_fn,"%.0f %f\n",gainnet_qform9->nframe,specsum);
        }
        wtk_gainnet_qform9_flush(gainnet_qform9, msg, cohv, 0, is_end);
    }
}

void wtk_gainnet_qform9_on_stft2(wtk_gainnet_qform9_t *gainnet_qform9,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *stft2_q=&(gainnet_qform9->stft2_q);
    int lt=gainnet_qform9->cfg->lt;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg;
    int i;

    if(is_end)
    {
        gainnet_qform9->end_pos=pos;
    }
    if(gainnet_qform9->cov)
    {
        if(msg)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
        }
        if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
        {
            wtk_gainnet_qform9_flush_aspec_lt(gainnet_qform9,stft2_q->length-lt-1, 0);
        }else if(stft2_q->length==2*lt+1)
        {
            wtk_gainnet_qform9_flush_aspec_lt(gainnet_qform9,stft2_q->length-lt-1, (is_end && lt==0)?1: 0);
            qn=wtk_queue_pop(stft2_q);
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            wtk_stft2_push_msg(gainnet_qform9->stft2,smsg);
        }else if(is_end && stft2_q->length==0)
        {
            wtk_gainnet_qform9_flush2(gainnet_qform9, NULL, NULL, 0, 1);
        }
        if(is_end)
        {
            if(stft2_q->length>0)
            {
                if(stft2_q->length<lt+1)
                {
                    for(i=0; i<stft2_q->length-1; ++i)
                    {
                        wtk_gainnet_qform9_flush_aspec_lt(gainnet_qform9, i, 0);
                    }
                    wtk_gainnet_qform9_flush_aspec_lt(gainnet_qform9, stft2_q->length-1, 1);
                }else
                {
                    for(i=0; i<lt-1; ++i)
                    {
                        wtk_gainnet_qform9_flush_aspec_lt(gainnet_qform9,stft2_q->length-lt+i, 0);   
                    }
                    wtk_gainnet_qform9_flush_aspec_lt(gainnet_qform9,stft2_q->length-1, 1);
                }
            }
            while(gainnet_qform9->stft2_q.length>0)
            {
                qn=wtk_queue_pop(&(gainnet_qform9->stft2_q));
                if(!qn){break;}
                smsg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
                wtk_stft2_push_msg(gainnet_qform9->stft2,smsg);
            }
        }
    }else
    {
        if(msg)
        {
            wtk_gainnet_qform9_flush_aspec(gainnet_qform9,msg,is_end);
            wtk_stft2_push_msg(gainnet_qform9->stft2, msg); 
        }else if(is_end && !msg)
        {
            wtk_gainnet_qform9_flush2(gainnet_qform9, NULL, NULL, 0, 1);
        }
    }   
}

void wtk_gainnet_qform9_start_aspec1(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
{
    float theta2, theta3;

    theta2=theta+2*theta_range;
    if(theta2>=360)
    {
        theta2-=360;
    }
    theta3=theta-2*theta_range;
    if(theta3<0)
    {
        theta3+=360;
    }
    aspec->start_ang_num=3;
    wtk_aspec_start(aspec, theta, 0, 0);
    wtk_aspec_start(aspec, theta2, 0, 1);
    wtk_aspec_start(aspec, theta3, 0, 2);   
}

void wtk_gainnet_qform9_start_aspec2(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
{
    float theta2, theta3;

    if(theta==0 || theta==180)
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            theta2=360-theta2;
        }

        aspec->start_ang_num=2;
        wtk_aspec_start(aspec, theta, 0, 0);
        wtk_aspec_start(aspec, theta2, 0, 1);
    }else
    {
        theta2=theta+2*theta_range;
        if(theta2>180)
        {
            if(theta+theta_range>=180)
            {
                theta2=-1;
            }else
            {
                theta2=180;   
            }
        }
        theta3=theta-2*theta_range;
        if(theta3<0)
        {
            if(theta-theta_range<=0)
            {
                theta3=-1;
            }else
            {
                theta3=0;
            }
        }
        if(theta2==-1 || theta3==-1)
        {
            aspec->start_ang_num=2;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2==-1?theta3:theta2, 0, 1);
        }else
        {
            aspec->start_ang_num=3;
            wtk_aspec_start(aspec, theta, 0, 0);
            wtk_aspec_start(aspec, theta2, 0, 1);
            wtk_aspec_start(aspec, theta3, 0, 2);   
        }
    }
}

void wtk_gainnet_qform9_start(wtk_gainnet_qform9_t *gainnet_qform9, float theta, float phi)
{
    int i;

    gainnet_qform9->theta=theta;
    gainnet_qform9->phi=phi;
    if(gainnet_qform9->cfg->use_two_channel){
        wtk_bf_update_ovec(gainnet_qform9->bf_class[0],gainnet_qform9->cfg->theta_center_class1,0);
        wtk_bf_update_ovec(gainnet_qform9->bf_class[1],gainnet_qform9->cfg->theta_center_class2,0);
        wtk_bf_init_w(gainnet_qform9->bf_class[0]);
        wtk_bf_init_w(gainnet_qform9->bf_class[1]);
    }else{
        wtk_bf_update_ovec(gainnet_qform9->bf,theta,phi);
        wtk_bf_init_w(gainnet_qform9->bf);
    }

    if(gainnet_qform9->cfg->use_two_channel || gainnet_qform9->cfg->use_noise_qenvelope)
    {
        if(gainnet_qform9->cfg->use_cline1)
        {
            wtk_gainnet_qform9_start_aspec2(gainnet_qform9->aspec_class[0], gainnet_qform9->cfg->theta_center_class1, 0, gainnet_qform9->cfg->theta_range_class1);
        }else
        {
            wtk_gainnet_qform9_start_aspec1(gainnet_qform9->aspec_class[0], gainnet_qform9->cfg->theta_center_class1, 0, gainnet_qform9->cfg->theta_range_class1);
        }

        if(gainnet_qform9->cfg->use_cline2)
        {
            wtk_gainnet_qform9_start_aspec2(gainnet_qform9->aspec_class[1], gainnet_qform9->cfg->theta_center_class2, 0, gainnet_qform9->cfg->theta_range_class2);
        }else
        {      
            wtk_gainnet_qform9_start_aspec1(gainnet_qform9->aspec_class[1], gainnet_qform9->cfg->theta_center_class2, 0, gainnet_qform9->cfg->theta_range_class2);
        }
    }else
    {
        if(gainnet_qform9->cfg->use_line)
        {
            wtk_gainnet_qform9_start_aspec2(gainnet_qform9->aspec, theta, phi, gainnet_qform9->cfg->theta_range);
        }else
        {
            wtk_gainnet_qform9_start_aspec1(gainnet_qform9->aspec, theta, phi, gainnet_qform9->cfg->theta_range);
        }
    }

    if(gainnet_qform9->cfg->use_t_r_qenvelope){
        float **t_r_qenvl=gainnet_qform9->cfg->t_r_qenvl;
        int idx=0;
        int idx2=0;
        float min_dis=360;
        float min_dis2=360;
        float dis;
        float dis2;
        if(gainnet_qform9->cfg->use_two_channel){
            for(i=0;i<gainnet_qform9->cfg->t_r_number;++i){
                if(t_r_qenvl[i][2] == 0){
                    if(t_r_qenvl[i][0] == -1){
                        dis = fabs(gainnet_qform9->cfg->theta_range-t_r_qenvl[i][1]);
                        if(dis < min_dis){
                            min_dis = dis;
                            idx = i;
                        }
                    }else{
                        if(theta == t_r_qenvl[i][0]){
                            dis = fabs(gainnet_qform9->cfg->theta_range-t_r_qenvl[i][1]);
                            if(dis < min_dis){
                                min_dis = dis;
                                idx = i;
                            }
                        }
                    }
                }else{
                    if(t_r_qenvl[i][0] == -1){
                        dis2 = fabs(gainnet_qform9->cfg->theta_range-t_r_qenvl[i][1]);
                        if(dis2 < min_dis2){
                            min_dis2 = dis2;
                            idx2 = i;
                        }
                    }else{
                        if(theta == t_r_qenvl[i][0]){
                            dis2 = fabs(gainnet_qform9->cfg->theta_range-t_r_qenvl[i][1]);
                            if(dis2 < min_dis2){
                                min_dis2 = dis2;
                                idx2 = i;
                            }
                        }
                    }
                }
            }
            gainnet_qform9->cfg->qenvl.envelope_thresh = t_r_qenvl[idx][3];
            gainnet_qform9->cfg->qenvl.min_speccrest = t_r_qenvl[idx][4];
            gainnet_qform9->cfg->qenvl.right_min_thresh = t_r_qenvl[idx][5];
            gainnet_qform9->cfg->qenvl2.envelope_thresh = t_r_qenvl[idx2][3];
            gainnet_qform9->cfg->qenvl2.min_speccrest = t_r_qenvl[idx2][4];
            gainnet_qform9->cfg->qenvl2.right_min_thresh = t_r_qenvl[idx2][5];
        }else{
            for(i=0;i<gainnet_qform9->cfg->t_r_number;++i){
                if(t_r_qenvl[i][0] == -1){
                    dis = fabs(gainnet_qform9->cfg->theta_range-t_r_qenvl[i][1]);
                    if(dis < min_dis){
                        min_dis = dis;
                        idx = i;
                    }
                }else{
                    if(theta == t_r_qenvl[i][0]){
                        dis = fabs(gainnet_qform9->cfg->theta_range-t_r_qenvl[i][1]);
                        if(dis < min_dis){
                            min_dis = dis;
                            idx = i;
                        }
                    }
                }
            }
            gainnet_qform9->cfg->qenvl.envelope_thresh = t_r_qenvl[idx][3];
            gainnet_qform9->cfg->qenvl.min_speccrest = t_r_qenvl[idx][4];
            gainnet_qform9->cfg->qenvl.right_min_thresh = t_r_qenvl[idx][5];
        }
    }
}

void wtk_gainnet_qform9_feed2(wtk_gainnet_qform9_t *qform,short **data,int len,int is_end)
{   
    int i,j;
    int channel=qform->bf->channel;
    float fv;
    float *fp[10];
    wtk_strbuf_t **input=qform->input;

    for(i=0;i<channel;++i)
    {
        wtk_strbuf_reset(input[i]);
        for(j=0;j<len;++j)
        {
            fv=data[i][j]/32768.0;
            wtk_strbuf_push(input[i],(char *)(&fv),sizeof(float));
        }
        fp[i]=(float *)(input[i]->data);
        wtk_preemph_dc(fp[i],qform->notch_mem[i],len);
        qform->memD[i]=wtk_preemph_asis(fp[i],len,qform->memD[i]);
    }
    wtk_stft2_feed_float(qform->stft2,fp,len,is_end);
}

void wtk_gainnet_qform9_feed(wtk_gainnet_qform9_t *gainnet_qform9,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,gainnet_qform9->bf->channel);
		wtk_wavfile_open2(mic_log,"gainnet_qform9");
	}
	if(len>0)
	{
		wtk_wavfile_write_mc(mic_log,data,len);
	}
	if(is_end && mic_log)
	{
		wtk_wavfile_close(mic_log);
		wtk_wavfile_delete(mic_log);
		mic_log=NULL;
	}
#endif
    if(gainnet_qform9->cfg->use_preemph)
    {
        wtk_gainnet_qform9_feed2(gainnet_qform9,data,len,is_end);
    }else
    {
        wtk_stft2_feed(gainnet_qform9->stft2,data,len,is_end);
    }
}