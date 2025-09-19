#include "wtk_qform9.h" 

void wtk_qform9_on_stft2(wtk_qform9_t *qform9,wtk_stft2_msg_t *msg,int pos,int is_end);
void wtk_qform9_on_qenvelope(wtk_qform9_t *qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_qform9_on_qenvel1(wtk_qform9_t *qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);
void wtk_qform9_on_qenvel2(wtk_qform9_t *qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end);

wtk_stft2_msg_t* wtk_qform9_stft2_msg_new(wtk_qform9_t *qform9)
{
	wtk_stft2_msg_t *msg;

	msg=(wtk_stft2_msg_t*)wtk_malloc(sizeof(wtk_stft2_msg_t));
	msg->hook=NULL;
	msg->fft=wtk_complex_new_p2(qform9->nbin,qform9->bf->channel);
	return msg;
}

void wtk_qform9_stft2_msg_delete(wtk_qform9_t *qform9,wtk_stft2_msg_t *msg)
{
	wtk_complex_delete_p2(msg->fft,qform9->nbin);
	wtk_free(msg);
}

wtk_stft2_msg_t* wtk_qform9_pop_stft2_msg(wtk_qform9_t *qform9)
{
	return  (wtk_stft2_msg_t*)wtk_hoard_pop(&(qform9->stft_msg_hoard));
}

void wtk_qform9_push_stft2_msg(wtk_qform9_t *qform9,wtk_stft2_msg_t *msg)
{
	wtk_hoard_push(&(qform9->stft_msg_hoard),msg);
}

wtk_stft2_msg_t* wtk_qform9_stft2_msg_copy(wtk_qform9_t *qform9,wtk_stft2_msg_t *msg,int channel,int nbin)
{
	wtk_stft2_msg_t *vmsg;

	vmsg=wtk_qform9_pop_stft2_msg(qform9);
	vmsg->s=msg->s;
	wtk_complex_cpy_p2(vmsg->fft,msg->fft,nbin,channel);
	return vmsg;
}

wtk_qform9_envelopemsg_t *wtk_qform9_envelope_msg_new(wtk_qform9_t *qform9)
{
    wtk_qform9_envelopemsg_t *msg;

    msg=(wtk_qform9_envelopemsg_t *)wtk_malloc(sizeof(wtk_qform9_envelopemsg_t));
    msg->smsg=wtk_qform9_stft2_msg_new(qform9);
    msg->cohv=(float *)wtk_malloc(sizeof(float)*qform9->nbin);
    return msg;
}

void wtk_qform9_envelope_msg_delete(wtk_qform9_t *qform9, wtk_qform9_envelopemsg_t *qemsg)
{
    wtk_free(qemsg->cohv);
    wtk_qform9_stft2_msg_delete(qform9,qemsg->smsg);
    wtk_free(qemsg);
}

wtk_qform9_envelopemsg_t* wtk_qform9_pop_envelope_msg(wtk_qform9_t *qform9)
{
	return  (wtk_qform9_envelopemsg_t*)wtk_hoard_pop(&(qform9->qenvel_msg_hoard));
}

void wtk_qform9_push_envelope_msg(wtk_qform9_t *qform9,wtk_qform9_envelopemsg_t *msg)
{
	wtk_hoard_push(&(qform9->qenvel_msg_hoard),msg);
}

wtk_qform9_envelopemsg_t *wtk_qform9_envelope_msg_copy(wtk_qform9_t *qform9, wtk_stft2_msg_t *smsg, float *cohv, int nbin, int channel)
{
    wtk_qform9_envelopemsg_t *qemsg;

    qemsg=wtk_qform9_pop_envelope_msg(qform9);
    qemsg->smsg->hook=NULL;
    qemsg->smsg->s=smsg->s;
    wtk_complex_cpy_p2(qemsg->smsg->fft, smsg->fft, nbin, channel);
    memcpy(qemsg->cohv, cohv, sizeof(float)*nbin);
    return qemsg;
}

wtk_qform9_t* wtk_qform9_new(wtk_qform9_cfg_t *cfg)
{
    wtk_qform9_t *qform9;
    int i;

    qform9=(wtk_qform9_t *)wtk_malloc(sizeof(wtk_qform9_t));
    qform9->cfg=cfg;
    qform9->ths=NULL;
    qform9->notify=NULL;
    qform9->ths_two_channel=NULL;
    qform9->notify_two_channel=NULL;
    qform9->ths2=NULL;
    qform9->notify2=NULL;

    qform9->input=NULL;
    if(cfg->use_preemph)
    {
        qform9->input=wtk_strbufs_new(cfg->bf.nmic);
    }

    qform9->stft2=wtk_stft2_new(&(cfg->stft2));
    wtk_stft2_set_notify(qform9->stft2,qform9,(wtk_stft2_notify_f)wtk_qform9_on_stft2);

    qform9->nbin=qform9->stft2->nbin;

    qform9->covm=NULL;
    qform9->covm_class[0]=qform9->covm_class[1]=NULL;
    qform9->covm=wtk_covm_new(&(cfg->covm), qform9->nbin, cfg->stft2.channel);
    if(qform9->cfg->use_two_channel){
        qform9->covm_class[0]=wtk_covm_new(&(cfg->covm), qform9->nbin, cfg->stft2.channel);
        qform9->covm_class[1]=wtk_covm_new(&(cfg->covm), qform9->nbin, cfg->stft2.channel);
    }
    
    qform9->bf_class[0]=qform9->bf_class[1]=NULL;
    if(qform9->cfg->use_two_channel){
        qform9->bf_class[0]=wtk_bf_new(&(cfg->bf),cfg->stft2.win);
        qform9->bf_class[1]=wtk_bf_new(&(cfg->bf),cfg->stft2.win);
    }
    qform9->bf=NULL;
    qform9->bf=wtk_bf_new(&(cfg->bf),cfg->stft2.win);

    qform9->aspec=NULL;
    qform9->naspec=NULL;
    qform9->aspec_class[0]=qform9->aspec_class[1]=qform9->aspec_class[2]=NULL;
    if(cfg->use_noiseblock)
    {
        qform9->naspec=(wtk_aspec_t **)wtk_malloc(sizeof(wtk_aspec_t *)*cfg->ntheta_num);
        for(i=0; i<cfg->ntheta_num; ++i)
        {
            qform9->naspec[i]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);
        }
    }else if(cfg->use_two_channel || cfg->use_noise_qenvelope)
    {
        qform9->aspec_class[0]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);        
        qform9->aspec_class[1]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);
        qform9->aspec_class[2]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);        
    }else if(cfg->use_two_aspecclass)
    {
        qform9->aspec_class[0]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);        
        qform9->aspec_class[1]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);
        qform9->aspec_class[2]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);        
    }else if(cfg->use_noiseblock2)
    {
        qform9->aspec=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);        
        qform9->naspec=(wtk_aspec_t **)wtk_malloc(sizeof(wtk_aspec_t *)*cfg->ntheta_num);
        for(i=0; i<cfg->ntheta_num; ++i)
        {
            qform9->naspec[i]=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);
        }
    }else
    {
        qform9->aspec=wtk_aspec_new(&(cfg->aspec), qform9->stft2->nbin, 3);        
    }

    qform9->fftclass[0]=qform9->fftclass[1]=NULL;
    if(qform9->aspec_class[0] && !qform9->aspec_class[0]->need_cov && qform9->cfg->use_two_channel)
    {
        qform9->fftclass[0]=wtk_complex_new_p2(qform9->nbin,qform9->stft2->cfg->channel);
        qform9->fftclass[1]=wtk_complex_new_p2(qform9->nbin,qform9->stft2->cfg->channel);
    }else if(qform9->aspec_class[0] && !qform9->aspec_class[0]->need_cov)
    {
        qform9->fftclass[0]=wtk_complex_new_p2(qform9->nbin,qform9->aspec_class[0]->cfg->channel);
        qform9->fftclass[1]=wtk_complex_new_p2(qform9->nbin,qform9->aspec_class[1]->cfg->channel);
    }
    
    qform9->cov=NULL;
    qform9->covclass[0]=qform9->covclass[1]=NULL;
    wtk_queue_init(&(qform9->stft2_q));
    if(cfg->delay_nf>0){
        if(qform9->cfg->use_two_channel){
            wtk_queue_init(&(qform9->delay_q_class[0]));
            wtk_queue_init(&(qform9->delay_q_class[1]));
            wtk_queue_init(&(qform9->delay_q2_class[0]));
            wtk_queue_init(&(qform9->delay_q2_class[1]));
        }else{
            wtk_queue_init(&(qform9->delay_q));
            wtk_queue_init(&(qform9->delay_q2));
        }
    }
    if((qform9->aspec && qform9->aspec->need_cov) || (qform9->naspec && qform9->naspec[0]->need_cov) 
                                                                                || (qform9->aspec_class[0] && qform9->aspec_class[0]->need_cov) )
    {
        qform9->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->stft2.channel*cfg->stft2.channel);
        if(cfg->lt<=0)
        {
            qform9->wint=wtk_malloc(sizeof(float));
            qform9->wint[0]=1;
        }else
        {
            qform9->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            qform9->winf=wtk_malloc(sizeof(float));
            qform9->winf[0]=1;
        }else
        {
            qform9->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }

        if(qform9->aspec_class[0])
        {
            qform9->covclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[0]->cfg->channel*qform9->aspec_class[0]->cfg->channel);
            qform9->covclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[1]->cfg->channel*qform9->aspec_class[1]->cfg->channel);
        }
    }
    qform9->inv_cov=NULL;
    qform9->invcovclass[0]=qform9->invcovclass[1]=NULL;
    qform9->tmp=NULL;
    if((qform9->aspec && qform9->aspec->need_inv_cov) || (qform9->naspec && qform9->naspec[0]->need_inv_cov))
    {
        qform9->inv_cov=(wtk_complex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*sizeof(wtk_complex_t));
        qform9->tmp=(wtk_dcomplex_t *)wtk_malloc(cfg->stft2.channel*cfg->stft2.channel*2*sizeof(wtk_dcomplex_t));
    }
    if(qform9->aspec_class[0] && qform9->aspec_class[0]->need_inv_cov)
    {
        qform9->invcovclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[0]->cfg->channel*qform9->aspec_class[0]->cfg->channel);
        qform9->invcovclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[1]->cfg->channel*qform9->aspec_class[1]->cfg->channel);
        i=max(qform9->aspec_class[0]->cfg->channel,qform9->aspec_class[1]->cfg->channel);
        qform9->tmp=(wtk_dcomplex_t *)wtk_malloc(i*i*2*sizeof(wtk_dcomplex_t));
    }

    qform9->cohv=NULL;
    qform9->cohv_class[0]=qform9->cohv_class[1]=qform9->cohv_class[2]=NULL;
    if(qform9->cfg->use_two_channel){
        qform9->cohv_class[0]=(float *)wtk_malloc(sizeof(float)*qform9->bf_class[0]->nbin);
        qform9->cohv_class[1]=(float *)wtk_malloc(sizeof(float)*qform9->bf_class[1]->nbin);
    }else if(qform9->cfg->use_noise_qenvelope){
        qform9->cohv_class[0]=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
        qform9->cohv_class[1]=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
        qform9->cohv_class[2]=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
    }else{
        qform9->cohv=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
    }

    qform9->qmmse=NULL;
    qform9->qmmse_class[0]=qform9->qmmse_class[1]=NULL;
    if(cfg->use_post && qform9->cfg->use_two_channel)
    {
        qform9->qmmse_class[0]=wtk_qmmse_new(&(cfg->qmmse));
        qform9->qmmse_class[1]=wtk_qmmse_new(&(cfg->qmmse));
    }else if(cfg->use_post){
        qform9->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }
    
    qform9->cohv_fn=NULL;
    qform9->cohv_fn_class[0]=qform9->cohv_fn_class[1]=NULL;
    if(qform9->cfg->use_two_channel){
        if(cfg->debug)
        {
            qform9->cohv_fn_class[0]=fopen("cohv1.dat","w");
            qform9->cohv_fn_class[1]=fopen("cohv2.dat","w");
        }
    }else{
        if(cfg->debug)
        {
            qform9->cohv_fn=fopen("cohv.dat","w");
        }
    }

    qform9->qenvelope=NULL;
    qform9->qenvel[0]=qform9->qenvel[1]=NULL;
    qform9->q_fring=NULL;
    qform9->q_fring_class[0]=qform9->q_fring_class[1]=qform9->q_fring_class[2]=NULL;

    if(cfg->use_simple_qenvelope){
        if(cfg->debug)
        {
            qform9->cohv_fn=fopen("cohv.dat","w");
        }
        wtk_hoard_init2(&(qform9->stft_msg_hoard),offsetof(wtk_stft2_msg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_stft2_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_stft2_msg_delete,
            qform9);
        wtk_hoard_init2(&(qform9->qenvel_msg_hoard),offsetof(wtk_qform9_envelopemsg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_envelope_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_envelope_msg_delete,
            qform9);
    }else if(cfg->use_qenvelope){
        wtk_hoard_init2(&(qform9->stft_msg_hoard),offsetof(wtk_stft2_msg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_stft2_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_stft2_msg_delete,
            qform9);

        wtk_hoard_init2(&(qform9->qenvel_msg_hoard),offsetof(wtk_qform9_envelopemsg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_envelope_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_envelope_msg_delete,
            qform9);
    }
    if(qform9->cfg->use_two_channel){
        if(cfg->use_simple_qenvelope){
            qform9->q_fring_class[0]=wtk_fring_new(cfg->qenvl.envelope_nf+cfg->delay_nf+1);
            qform9->q_fring_class[1]=wtk_fring_new(cfg->qenvl2.envelope_nf+cfg->delay_nf+1);
        }else if(cfg->use_qenvelope)
        {
            qform9->qenvel[0]=wtk_qenvelope_new(&(cfg->qenvl));
            qform9->qenvel[1]=wtk_qenvelope_new(&(cfg->qenvl2));
            wtk_qenvelope_set_notify(qform9->qenvel[0], qform9, (wtk_qenvelope_notify_f)wtk_qform9_on_qenvel1);
            wtk_qenvelope_set_notify(qform9->qenvel[1], qform9, (wtk_qenvelope_notify_f)wtk_qform9_on_qenvel2);
        }
    }else if(cfg->use_noise_qenvelope){
        if(cfg->use_simple_qenvelope){
            qform9->q_fring_class[0]=wtk_fring_new(cfg->qenvl.envelope_nf+cfg->delay_nf+1);
            qform9->q_fring_class[1]=wtk_fring_new(cfg->qenvl2.envelope_nf+cfg->delay_nf+1);
            qform9->q_fring_class[2]=wtk_fring_new(cfg->qenvl3.envelope_nf+cfg->delay_nf+1);
        }else{
            wtk_debug("error use noise_qenvelope");
        }
    }else{
        if(cfg->use_simple_qenvelope){
            qform9->q_fring=wtk_fring_new(cfg->qenvl.envelope_nf+cfg->delay_nf+1);
        }else if(cfg->use_qenvelope)
        {
            qform9->qenvelope=wtk_qenvelope_new(&(cfg->qenvl));
            wtk_qenvelope_set_notify(qform9->qenvelope, qform9, (wtk_qenvelope_notify_f)wtk_qform9_on_qenvelope);
        }
    }

    qform9->chn1_buf=NULL;
    qform9->chn2_buf=NULL;
    qform9->out_buf=NULL;
    if(qform9->cfg->use_two_channel){
        qform9->chn1_buf=wtk_strbuf_new(1024, 1);
        qform9->chn2_buf=wtk_strbuf_new(1024, 1);
        qform9->out_buf=wtk_strbuf_new(1024, 1);
    }

    qform9->howl=NULL;
    if(qform9->cfg->use_howl_suppression){
        qform9->howl = (wtk_qform9_howl_t *)wtk_malloc(sizeof(wtk_qform9_howl_t));
        qform9->howl->correct_count = (int *)wtk_malloc(sizeof(int)*qform9->nbin);
        qform9->howl->freq_howl = (int *)wtk_malloc(sizeof(int)*qform9->nbin);
        qform9->howl->no_howl = (int *)wtk_malloc(sizeof(int)*qform9->nbin);
    }

    qform9->entropy_E=NULL;
    qform9->entropy_Eb=NULL;
    qform9->ncohv_fn=NULL;
    qform9->scohv_fn=NULL;
    qform9->entropy_fn=NULL;
    if(qform9->cfg->use_cohv_cnt){
        qform9->entropy_E=(float *)wtk_malloc(sizeof(float)*qform9->nbin);
        qform9->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->stft2.win);
        if(cfg->debug){
            qform9->ncohv_fn=fopen("ncohv.dat","w");
            qform9->scohv_fn=fopen("scohv.dat","w");
            qform9->entropy_fn=fopen("entropy.dat","w");
        }
    }

	qform9->bs_win=NULL;
	if(cfg->use_bs_win)
	{
		qform9->bs_win=wtk_math_create_hanning_window2(cfg->stft2.win/2);
	}

    wtk_qform9_reset(qform9);

    return qform9;
}

void wtk_qform9_delete(wtk_qform9_t *qform9)
{
    int i;

    if(qform9->input)
    {
        wtk_strbufs_delete(qform9->input, qform9->bf->channel);
    }

    if(qform9->cfg->use_simple_qenvelope){
        wtk_hoard_clean(&(qform9->stft_msg_hoard));
        wtk_hoard_clean(&(qform9->qenvel_msg_hoard));
    }

    if(qform9->qenvelope)
    {
        wtk_hoard_clean(&(qform9->stft_msg_hoard));
        wtk_hoard_clean(&(qform9->qenvel_msg_hoard));
        wtk_qenvelope_delete(qform9->qenvelope);
    }
    if(qform9->qenvel[0]){
        wtk_hoard_clean(&(qform9->stft_msg_hoard));
        wtk_hoard_clean(&(qform9->qenvel_msg_hoard));
        wtk_qenvelope_delete(qform9->qenvel[0]);
        wtk_qenvelope_delete(qform9->qenvel[1]);
    }
    if(qform9->q_fring){
        wtk_fring_delete(qform9->q_fring);
    }
    if(qform9->q_fring_class[0]){
        wtk_fring_delete(qform9->q_fring_class[0]);
        wtk_fring_delete(qform9->q_fring_class[1]);
    }
    if(qform9->q_fring_class[2]){
        wtk_fring_delete(qform9->q_fring_class[2]);
    }
    if(qform9->chn1_buf){
        wtk_strbuf_delete(qform9->chn1_buf);
    }
    if(qform9->chn2_buf){
        wtk_strbuf_delete(qform9->chn2_buf);
    }
    if(qform9->out_buf){
        wtk_strbuf_delete(qform9->out_buf);
    }

    if(qform9->cov)
    {
        wtk_free(qform9->cov);
        wtk_free(qform9->wint);
        wtk_free(qform9->winf);
    }
    if(qform9->inv_cov)
    {
        wtk_free(qform9->inv_cov);
    }
    if(qform9->tmp)
    {
        wtk_free(qform9->tmp);
    }
    if(qform9->covclass[0])
    {
        wtk_free(qform9->covclass[0]);
        wtk_free(qform9->covclass[1]);
    }
    if(qform9->invcovclass[0])
    {
        wtk_free(qform9->invcovclass[0]);
        wtk_free(qform9->invcovclass[1]);
    }
    if(qform9->fftclass[0])
    {
        wtk_complex_delete_p2(qform9->fftclass[0],qform9->nbin);
        wtk_complex_delete_p2(qform9->fftclass[1],qform9->nbin);
    }

    if(qform9->aspec)
    {
        wtk_aspec_delete(qform9->aspec);
    }
    if(qform9->naspec)
    {
        for(i=0; i<qform9->cfg->ntheta_num; ++i)
        {
            wtk_aspec_delete(qform9->naspec[i]);
        }
        wtk_free(qform9->naspec);
    }
    if(qform9->aspec_class[0])
    {
        wtk_aspec_delete(qform9->aspec_class[0]);
        wtk_aspec_delete(qform9->aspec_class[1]);
        wtk_aspec_delete(qform9->aspec_class[2]);
    }

    if(qform9->cohv_fn)
    {
        fclose(qform9->cohv_fn);
    }
    if(qform9->cohv_fn_class[0]){
        fclose(qform9->cohv_fn_class[0]);
        fclose(qform9->cohv_fn_class[1]);
    }
    if(qform9->cohv_class[0]){
        wtk_free(qform9->cohv_class[0]);
        wtk_free(qform9->cohv_class[1]);
    }
    if(qform9->cohv_class[2]){
        wtk_free(qform9->cohv_class[2]);
    }

    if(qform9->cohv){
        wtk_free(qform9->cohv);
    }


    
    if(qform9->qmmse)
    {
        wtk_qmmse_delete(qform9->qmmse);
    }
    if(qform9->qmmse_class[0]){
        wtk_qmmse_delete(qform9->qmmse_class[0]);
        wtk_qmmse_delete(qform9->qmmse_class[1]);
    }

    wtk_stft2_delete(qform9->stft2);
    if(qform9->covm){
        wtk_covm_delete(qform9->covm);
    }
    if(qform9->covm_class[0]){
        wtk_covm_delete(qform9->covm_class[0]);
        wtk_covm_delete(qform9->covm_class[1]);
    }
    if(qform9->bf){
        wtk_bf_delete(qform9->bf);
    }
    if(qform9->bf_class[0]){
        wtk_bf_delete(qform9->bf_class[0]);
        wtk_bf_delete(qform9->bf_class[1]);
    }

    if(qform9->howl){
        wtk_free(qform9->howl->correct_count);
        wtk_free(qform9->howl->freq_howl);
        wtk_free(qform9->howl->no_howl);
        wtk_free(qform9->howl);
    }
    if(qform9->entropy_E){
        wtk_free(qform9->entropy_E);
    }
    if(qform9->entropy_Eb){
        wtk_free(qform9->entropy_Eb);
    }
    if(qform9->ncohv_fn){
        fclose(qform9->ncohv_fn);
    }
    if(qform9->scohv_fn){
        fclose(qform9->scohv_fn);
    }
    if(qform9->entropy_fn){
        fclose(qform9->entropy_fn);
    }
	if(qform9->bs_win)
	{
		wtk_free(qform9->bs_win);
	}

    wtk_free(qform9);
}

void wtk_qform9_reset(wtk_qform9_t *qform9)
{
    int channel=qform9->bf->channel;
    int i;

    qform9->end_pos=0;
    qform9->delay_end_pos=0;
    qform9->delay_end_pos_class[0]=0;
    qform9->delay_end_pos_class[1]=0;
    for(i=0;i<channel;++i)
    {
        memset(qform9->notch_mem[i],0,2*sizeof(float));
    }
	memset(qform9->memD,0,channel*sizeof(float));
    qform9->memX=0;
    qform9->memX_class[0]=0;
    qform9->memX_class[1]=0;

    if(qform9->input)
    {
        wtk_strbufs_reset(qform9->input, qform9->bf->channel);
    }

    if(qform9->qenvelope)
    {
        wtk_qenvelope_reset(qform9->qenvelope);
    }
    if(qform9->qenvel[0]){
        wtk_qenvelope_reset(qform9->qenvel[0]);
        wtk_qenvelope_reset(qform9->qenvel[1]);
    }
    if(qform9->q_fring){
        wtk_fring_reset(qform9->q_fring);
        qform9->q_spec=0;
        qform9->right_nf=0;
    }
    if(qform9->q_fring_class[0]){
        wtk_fring_reset(qform9->q_fring_class[0]);
        wtk_fring_reset(qform9->q_fring_class[1]);
        qform9->q_spec_class[0]=0;
        qform9->q_spec_class[1]=0;
        qform9->right_nf_class[0]=0;
        qform9->right_nf_class[1]=0;
    }
    if(qform9->q_fring_class[2]){
        wtk_fring_reset(qform9->q_fring_class[2]);
        qform9->q_spec_class[2]=0;
        qform9->right_nf_class[2]=0;
    }

    if(qform9->aspec)
    {
        wtk_aspec_reset(qform9->aspec);
    }
    if(qform9->naspec)
    {
        for(i=0; i<qform9->cfg->ntheta_num; ++i)
        {
            wtk_aspec_reset(qform9->naspec[i]);
        }
    }
    if(qform9->aspec_class[0])
    {
        wtk_aspec_reset(qform9->aspec_class[0]);
        wtk_aspec_reset(qform9->aspec_class[1]);
        wtk_aspec_reset(qform9->aspec_class[2]);
    }

    wtk_queue_init(&(qform9->stft2_q));
    if(qform9->cfg->delay_nf>0){
        if(qform9->cfg->use_two_channel){
            wtk_queue_init(&(qform9->delay_q_class[0]));
            wtk_queue_init(&(qform9->delay_q_class[1]));
            wtk_queue_init(&(qform9->delay_q2_class[0]));
            wtk_queue_init(&(qform9->delay_q2_class[1]));
        }else{
            wtk_queue_init(&(qform9->delay_q));
            wtk_queue_init(&(qform9->delay_q2));
        }
    }
    wtk_stft2_reset(qform9->stft2);
    wtk_covm_reset(qform9->covm);
    if(qform9->cfg->use_two_channel){
        wtk_covm_reset(qform9->covm_class[0]);
        wtk_covm_reset(qform9->covm_class[1]);
    }
    wtk_bf_reset(qform9->bf);
    if(qform9->cfg->use_two_channel){
        wtk_bf_reset(qform9->bf_class[0]);
        wtk_bf_reset(qform9->bf_class[1]);
    }

    qform9->nframe=0;

    qform9->theta=qform9->phi=-1;

    if(qform9->cfg->use_two_channel){
        memset(qform9->cohv_class[0],0,sizeof(float)*qform9->bf_class[0]->nbin);
        memset(qform9->cohv_class[1],0,sizeof(float)*qform9->bf_class[1]->nbin);
    }else if(qform9->cfg->use_noise_qenvelope){
        memset(qform9->cohv_class[0],0,sizeof(float)*qform9->bf->nbin);
        memset(qform9->cohv_class[1],0,sizeof(float)*qform9->bf->nbin);
        memset(qform9->cohv_class[2],0,sizeof(float)*qform9->bf->nbin);
    }else{
        memset(qform9->cohv,0,sizeof(float)*qform9->bf->nbin);
    }
    if(qform9->cfg->use_two_channel){
        wtk_strbuf_reset(qform9->chn1_buf);
        wtk_strbuf_reset(qform9->chn2_buf);
        wtk_strbuf_reset(qform9->out_buf);
    }

    if(qform9->qmmse)
    {
        wtk_qmmse_reset(qform9->qmmse);
    }
    if(qform9->qmmse_class[0]){
        wtk_qmmse_reset(qform9->qmmse_class[0]);
        wtk_qmmse_reset(qform9->qmmse_class[1]);
    }
    if(qform9->howl){
        for(i=0;i<qform9->nbin;++i){
            qform9->howl->correct_count[i] = 0;
            qform9->howl->freq_howl[i] = 0;
            qform9->howl->no_howl[i] = 0;
        }
    }
    if(qform9->entropy_E){
        memset(qform9->entropy_E,0,sizeof(float)*qform9->nbin);
    }
    if(qform9->entropy_Eb){
        memset(qform9->entropy_Eb,0,sizeof(float)*qform9->cfg->stft2.win);
    }

    qform9->noise_debug_cnt = 0;
    qform9->noise_debug_cnt_class[0] = 0;
    qform9->noise_debug_cnt_class[1] = 0;
    qform9->delay_nf=qform9->cfg->delay_nf;
    qform9->delay_nf_class[0]=qform9->cfg->delay_nf;
    qform9->delay_nf_class[1]=qform9->cfg->delay_nf;
    qform9->delay_cnt=0;
    qform9->delay_cnt_class[0]=0;
    qform9->delay_cnt_class[1]=0;
    qform9->entropy_in_cnt = 0;
    qform9->entropy_silcnt = 0;
    qform9->entropy_sil = 1;
    qform9->ncohv_sum = 0;
    qform9->ncohv_sum_class[0] = 0;
    qform9->ncohv_sum_class[1] = 0;
    qform9->scohv_sum = 0;
    qform9->scohv_sum_class[0] = 0;
    qform9->scohv_sum_class[1] = 0;
    qform9->ncohv_cnt = 0;
    qform9->ncohv_cnt_class[0] = 0;
    qform9->ncohv_cnt_class[1] = 0;
    qform9->cohv_frame = 0;
    qform9->cohv_frame_class[0] = 0;
    qform9->cohv_frame_class[1] = 0;

	qform9->bs_scale=1.0;
	qform9->bs_last_scale=1.0;
	qform9->bs_real_scale=1.0;
	qform9->bs_max_cnt=0;

    qform9->sil_in_cnt = 0;
    qform9->sil_out_cnt = 0;
    if(qform9->qmmse){
        qform9->sil_max_range = qform9->qmmse->cfg->max_range;
        qform9->sil_noise_suppress = qform9->qmmse->cfg->noise_suppress;
    }
}

void wtk_qform9_set_notify(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_f notify)
{
    qform9->ths=ths;
    qform9->notify=notify;
}

void wtk_qform9_set_notify_two_channel(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_two_channel_f notify)
{
    qform9->ths_two_channel=ths;
    qform9->notify_two_channel=notify;
}

void wtk_qform9_control_bs(wtk_qform9_t *qform9, float *out, int len)
{
	float *bs_win=qform9->bs_win;
	float max_out = qform9->cfg->max_out;
	float out_max;
	int i;

	if(1)
	{
		out_max=wtk_float_abs_max(out, len);
		if(out_max>max_out)
		{
			qform9->bs_scale=max_out/out_max;
			if(qform9->bs_scale<qform9->bs_last_scale)
			{
				qform9->bs_last_scale=qform9->bs_scale;
			}else
			{
				qform9->bs_scale=qform9->bs_last_scale;
			}
			qform9->bs_max_cnt=5;
		}
		if(bs_win){
			for(i=0; i<len/2; ++i)
			{
				out[i]*=qform9->bs_scale * bs_win[i] + qform9->bs_real_scale * (1.0-bs_win[i]);
			}
			for(i=len/2; i<len; ++i){
				out[i]*=qform9->bs_scale;
			}
			qform9->bs_real_scale = qform9->bs_scale;
		}else{
			for(i=0; i<len; ++i){
				out[i]*=qform9->bs_scale;
			}
		}
		if(qform9->bs_max_cnt>0)
		{
			--qform9->bs_max_cnt;
		}
		if(qform9->bs_max_cnt<=0 && qform9->bs_scale<1.0)
		{
			qform9->bs_scale*=1.1f;
			qform9->bs_last_scale=qform9->bs_scale;
			if(qform9->bs_scale>1.0)
			{
				qform9->bs_scale=1.0;
				qform9->bs_last_scale=1.0;
			}
		}
	}else
	{
		qform9->bs_scale=1.0;
		qform9->bs_last_scale=1.0;
		qform9->bs_max_cnt=0;
	}
} 

void wtk_qform9_notify_data(wtk_qform9_t *qform9,float *data,int len, int chn)
{
    short *pv=(short *)data;
    int i;
    wtk_strbuf_t *chn1_buf, *chn2_buf, *out_buf;
    int out_len;

    if(qform9->cfg->use_preemph && qform9->cfg->use_two_channel)
    {
        if(!chn){
            qform9->memX_class[0]=wtk_preemph_asis2(data,len,qform9->memX_class[0]);
        }else{
            qform9->memX_class[1]=wtk_preemph_asis2(data,len,qform9->memX_class[1]);
        }
    }else if(qform9->cfg->use_preemph)
    {
        qform9->memX=wtk_preemph_asis2(data,len,qform9->memX);
    }
    if(qform9->cfg->use_bs){
        for(i=0;i<len;++i){
            data[i] *= 32000.0;
        }
        wtk_qform9_control_bs(qform9, data, len);
        for(i=0;i<len;++i){
            pv[i]=floorf(data[i]+0.5f);
        }
    }else{
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
    }
    if(qform9->cfg->use_two_channel){

        chn1_buf=qform9->chn1_buf;
        chn2_buf=qform9->chn2_buf;
        out_buf=qform9->out_buf;

        if(!chn){
            wtk_strbuf_push(chn1_buf, (char *)pv, len<<1);
        }else{
            wtk_strbuf_push(chn2_buf, (char *)pv, len<<1);
        }

        out_len=min(chn1_buf->pos, chn2_buf->pos);
        if(out_len>0 && qform9->notify_two_channel){  // 二维数组分别输出单通道音频
            short *pv_two[2];

            pv_two[0]=(short *)chn1_buf->data;
            pv_two[1]=(short *)chn2_buf->data;

            qform9->notify_two_channel(qform9->ths_two_channel,pv_two,out_len>>1,0);
            wtk_strbuf_pop(chn1_buf, NULL, out_len);
            wtk_strbuf_pop(chn2_buf, NULL, out_len);
        }else if(out_len>0 && qform9->notify){  // 输出双通道音频
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

            qform9->notify(qform9->ths,(short *)out_buf->data,out_buf->pos>>1,0);
            wtk_strbuf_reset(out_buf);
            wtk_strbuf_pop(chn1_buf, NULL, out_len);
            wtk_strbuf_pop(chn2_buf, NULL, out_len);
        }
    }else{
        if(qform9->notify)
        {
            qform9->notify(qform9->ths,pv,len,0);
        }
    }
}

void wtk_qform9_howl_judge(wtk_qform9_t *qform9, wtk_complex_t *bf_out, int len){
    int i,j;
    float tmp;
    int range = 5;
    int check = 3;
    int correct = 50;
    int no_howl = 100;
    int start, end;

    for(i=0;i<len;++i){
        tmp = sqrtf(bf_out[i].a * bf_out[i].a + bf_out[i].b * bf_out[i].b);
        if(tmp>1){
            ++qform9->howl->freq_howl[i];
            qform9->howl->no_howl[i]=0;
        }else{
            ++qform9->howl->no_howl[i];
        }
        if(qform9->howl->no_howl[i]>no_howl){
            qform9->howl->freq_howl[i] = 0;
            qform9->howl->correct_count[i] = 0;
        }
        if(tmp > 3){
            bf_out[i].a /= expf(tmp/1 * 1.0f);
            bf_out[i].b /= expf(tmp/1 * 1.0f);
        }
    }

    for(i=0;i<len;++i){
        if(qform9->howl->freq_howl[i] > check){
            qform9->howl->correct_count[i] = correct;
            qform9->howl->freq_howl[i] = 0;
        }
    }
    for(i=0;i<len;++i){
        if(qform9->howl->correct_count[i] > 0){
            start = i-range>0?i-range:0;
            end = i+range<len>>1?i+range:len>>1;
            for(j=start;j<end;++j){
                bf_out[j].a /= 30.0;
                bf_out[j].b /= 30.0;
            }
        }
    }
}

void wtk_qform9_feed_cnon(wtk_qform9_t *qform9, wtk_complex_t * fft)
{
	int nbin=qform9->stft2->cfg->win/2+1;
	float sym=qform9->cfg->sym;
	static float fx=2.0f*PI/RAND_MAX;
	int cnon_clip_s=qform9->cfg->cnon_clip_s;
	int cnon_clip_e=qform9->cfg->cnon_clip_e;
	float f,f2;
	int i;

	for(i=max(1, cnon_clip_s);i<min(nbin-1, cnon_clip_e);++i)
	{
		f=rand()*fx;
		f2=0.01f;
		if(f2>0)
		{
			// f2=sqrtf(f2);
			fft[i].a+=sym*cosf(f)*f2;
			fft[i].b+=sym*sinf(f)*f2;
		}
	}
}

void wtk_qform9_flush2(wtk_qform9_t *qform9,wtk_stft2_msg_t *smsg,float *cohv, int chn, int wav_debug, int is_end)
{
    int k;
    wtk_complex_t *bf_out=NULL;
    static int state=0;
    // int nbin=qform9->bf->nbin;
    if(qform9->cfg->use_two_channel){
        if(!chn){
            if(smsg)
            {
                bf_out=wtk_bf_output_fft2_msg2(qform9->bf_class[0],smsg,cohv);
                if(qform9->qmmse_class[0] && wav_debug != -1)
                {
                    wtk_qmmse_feed_cohv(qform9->qmmse_class[0],bf_out,cohv);
                }
                if(wav_debug==1){
                    memset(bf_out, 0, sizeof(wtk_complex_t)*qform9->bf_class[0]->nbin);
                }
            }
            if(qform9->notify || qform9->notify_two_channel)
            {
                if(bf_out)
                {
                    if(qform9->cfg->use_howl_suppression){
                        wtk_qform9_howl_judge(qform9, bf_out, qform9->bf->nbin);
                    }
                    if(qform9->cfg->use_cnon){
                        wtk_qform9_feed_cnon(qform9, bf_out);
                    }
                    if(qform9->cfg->delay_nf>0){
                        k=wtk_stft2_output_ifft(qform9->stft2,bf_out,qform9->stft2->output,qform9->bf_class[0]->pad,qform9->delay_end_pos_class[0],is_end);
                    }else{
                        k=wtk_stft2_output_ifft(qform9->stft2,bf_out,qform9->stft2->output,qform9->bf_class[0]->pad,qform9->end_pos,is_end);
                    }
                    wtk_qform9_notify_data(qform9,qform9->stft2->output,k,0);
                }
                if(is_end)
                {
                    ++state;
                }
            }else if(qform9->notify2)
            {
                qform9->notify2(qform9->ths2, bf_out, is_end);
            }
        }else{
            if(smsg)
            {
                bf_out=wtk_bf_output_fft2_msg2(qform9->bf_class[1],smsg,cohv);
                if(qform9->qmmse_class[1] && wav_debug != -1)
                {
                    wtk_qmmse_feed_cohv(qform9->qmmse_class[1],bf_out,cohv);
                }
                if(wav_debug==1){
                    memset(bf_out, 0, sizeof(wtk_complex_t)*qform9->bf_class[1]->nbin);
                }
            }
            if(qform9->notify || qform9->notify_two_channel)
            {
                if(bf_out)
                {
                    if(qform9->cfg->use_howl_suppression){
                        wtk_qform9_howl_judge(qform9, bf_out, qform9->bf->nbin);
                    }
                    if(qform9->cfg->use_cnon){
                        wtk_qform9_feed_cnon(qform9, bf_out);
                    }
                    if(qform9->cfg->delay_nf>0){
                        k=wtk_stft2_output_ifft(qform9->stft2,bf_out,qform9->stft2->output,qform9->bf_class[1]->pad,qform9->delay_end_pos_class[1],is_end);
                    }else{
                        k=wtk_stft2_output_ifft(qform9->stft2,bf_out,qform9->stft2->output,qform9->bf_class[1]->pad,qform9->end_pos,is_end);
                    }
                    wtk_qform9_notify_data(qform9,qform9->stft2->output,k,1);
                }
                if(is_end)
                {
                    ++state;
                }
            }else if(qform9->notify2)
            {
                qform9->notify2(qform9->ths2, bf_out, is_end);
            }
        }
        if(state==2 && qform9->notify_two_channel){
            qform9->notify_two_channel(qform9->ths_two_channel,NULL,0,1);
        }else if(state==2 && qform9->notify){
            qform9->notify(qform9->ths,NULL,0,1);
        }
    }else{
        if(smsg)
        {
            bf_out=wtk_bf_output_fft2_msg2(qform9->bf,smsg,cohv);
            if(qform9->qmmse && wav_debug != -1)
            {
                wtk_qmmse_feed_cohv(qform9->qmmse,bf_out,cohv);      
            }
            if(wav_debug==1){
                memset(bf_out, 0, sizeof(wtk_complex_t)*qform9->bf->nbin);
            }
        }
        if(qform9->notify)
        {
            if(bf_out)
            {
                if(qform9->cfg->use_howl_suppression){
                    wtk_qform9_howl_judge(qform9, bf_out, qform9->bf->nbin);
                }
                if(qform9->cfg->use_cnon){
                    wtk_qform9_feed_cnon(qform9, bf_out);
                }
                if(qform9->cfg->delay_nf>0){
                    k=wtk_stft2_output_ifft(qform9->stft2,bf_out,qform9->stft2->output,qform9->bf->pad,qform9->delay_end_pos,is_end);
                }else{
                    k=wtk_stft2_output_ifft(qform9->stft2,bf_out,qform9->stft2->output,qform9->bf->pad,qform9->end_pos,is_end);
                }
                wtk_qform9_notify_data(qform9,qform9->stft2->output,k,0);
            }
            if(is_end)
            {
                qform9->notify(qform9->ths,NULL,0,1);
            }
        }else if(qform9->notify2)
        {
            qform9->notify2(qform9->ths2, bf_out, is_end);
        }
    }
 
}

void wtk_qform9_flush(wtk_qform9_t *qform9,wtk_stft2_msg_t *smsg, float *cohv, int chn, int wav_debug, int is_end)
{
    int k;
    int nbin=qform9->bf->nbin;
    int i, channel=qform9->bf->channel;
    int b;
    wtk_covm_t *covm=qform9->covm;
    wtk_covm_t *covm_class[2];
    covm_class[0]=qform9->covm_class[0];
    covm_class[1]=qform9->covm_class[1];

    if(qform9->cfg->use_two_channel){
        if(!chn){
            for(k=1; k<nbin-1; ++k)
            {
                b=0;
                if(cohv[k]<0.0)
                {
                    b=wtk_covm_feed_fft2(covm_class[0], smsg->fft, k, 1);
                    if(b==1)
                    {
                        wtk_bf_update_ncov(qform9->bf_class[0], covm_class[0]->ncov, k);
                    }
                }else
                {
                    if(covm_class[0]->scov)
                    {
                        b=wtk_covm_feed_fft2(covm_class[0], smsg->fft, k, 0);
                        if(b==1)
                        {
                            wtk_bf_update_scov(qform9->bf_class[0], covm_class[0]->scov, k);
                        }
                    }
                }
                if(covm_class[0]->ncnt_sum[k]>5 && (covm_class[0]->scnt_sum==NULL ||  covm_class[0]->scnt_sum[k]>5) && b==1)
                {
                    wtk_bf_update_w(qform9->bf_class[0], k);
                }

                if(qform9->cfg->debug)
                {
                    if(cohv[k]<0)
                    {
                        for(i=0;i<channel;++i)
                        {
                            qform9->bf_class[0]->w[k][i].a=0;
                            qform9->bf_class[0]->w[k][i].b=0;
                        }
                    }else
                    {
                        for(i=0;i<channel;++i)
                        {
                            qform9->bf_class[0]->w[k][i].a=0;
                            qform9->bf_class[0]->w[k][i].b=0;
                            if(i==0)
                            {
                                qform9->bf_class[0]->w[k][i].a=1;
                            }
                        }
                    }
                }else if(wav_debug==-1){
                    if(cohv[k]>0){
                        for(i=0;i<channel;++i)
                        {
                            qform9->bf->w[k][i].a=0;
                            qform9->bf->w[k][i].b=0;
                            if(i==0)
                            {
                                qform9->bf->w[k][i].a=1;
                            }
                        }
                    }
                }
            }
            wtk_qform9_flush2(qform9,smsg,cohv,0,wav_debug,is_end);
        }else{
            for(k=1; k<nbin-1; ++k)
            {
                b=0;
                if(cohv[k]<0.0)
                {
                    b=wtk_covm_feed_fft2(covm_class[1], smsg->fft, k, 1);
                    if(b==1)
                    {
                        wtk_bf_update_ncov(qform9->bf_class[1], covm_class[1]->ncov, k);
                    }
                }else
                {
                    if(covm_class[1]->scov)
                    {
                        b=wtk_covm_feed_fft2(covm_class[1], smsg->fft, k, 0);
                        if(b==1)
                        {
                            wtk_bf_update_scov(qform9->bf_class[1], covm_class[1]->scov, k);
                        }
                    }
                }
                if(covm_class[1]->ncnt_sum[k]>5 && (covm_class[1]->scnt_sum==NULL ||  covm_class[1]->scnt_sum[k]>5) && b==1)
                {
                    wtk_bf_update_w(qform9->bf_class[1], k);
                }

                if(qform9->cfg->debug)
                {
                    if(cohv[k]<0)
                    {
                        for(i=0;i<channel;++i)
                        {
                            qform9->bf_class[1]->w[k][i].a=0;
                            qform9->bf_class[1]->w[k][i].b=0;
                        }
                    }else
                    {
                        for(i=0;i<channel;++i)
                        {
                            qform9->bf_class[1]->w[k][i].a=0;
                            qform9->bf_class[1]->w[k][i].b=0;
                            if(i==0)
                            {
                                qform9->bf_class[1]->w[k][i].a=1;
                            }
                        }
                    }
                }else if(wav_debug==-1){
                    if(cohv[k]>0){
                        for(i=0;i<channel;++i)
                        {
                            qform9->bf->w[k][i].a=0;
                            qform9->bf->w[k][i].b=0;
                            if(i==0)
                            {
                                qform9->bf->w[k][i].a=1;
                            }
                        }
                    }
                }
            }
            wtk_qform9_flush2(qform9,smsg,cohv,1,wav_debug,is_end);
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
                    wtk_bf_update_ncov(qform9->bf, covm->ncov, k);
                }
            }else
            {
                if(covm->scov)
                {
                    b=wtk_covm_feed_fft2(covm, smsg->fft, k, 0);
                    if(b==1)
                    {
                        wtk_bf_update_scov(qform9->bf, covm->scov, k);
                    }
                }
            }
            if(covm->ncnt_sum[k]>5 && (covm->scnt_sum==NULL ||  covm->scnt_sum[k]>5) && b==1)
            {
                wtk_bf_update_w(qform9->bf, k);
            }

            if(qform9->cfg->debug)
            {
                if(cohv[k]<0)
                {
                    for(i=0;i<channel;++i)
                    {
                        qform9->bf->w[k][i].a=0;
                        qform9->bf->w[k][i].b=0;
                    }
                }else
                {
                    for(i=0;i<channel;++i)
                    {
                        qform9->bf->w[k][i].a=0;
                        qform9->bf->w[k][i].b=0;
                        if(i==0)
                        {
                            qform9->bf->w[k][i].a=1;
                        }
                    }
                }
            }else if(wav_debug==-1){
                if(cohv[k]>0){
                    for(i=0;i<channel;++i)
                    {
                        qform9->bf->w[k][i].a=0;
                        qform9->bf->w[k][i].b=0;
                        if(i==0)
                        {
                            qform9->bf->w[k][i].a=1;
                        }
                    }
                }
            }
        }
        wtk_qform9_flush2(qform9,smsg,cohv,0,wav_debug,is_end);
    }
}
void wtk_qform9_on_qenvel1(wtk_qform9_t *qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_qform9_envelopemsg_t *qemsg;
    int k;
    int nbin=qform9->nbin;

    if(msg)
    {
        qemsg=(wtk_qform9_envelopemsg_t *)msg->hook;
        if(state==WTK_QENVELOPE_TROUGH)
        {
            if(qform9->cfg->use_nqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=-1;
                }
            }
        }else if(state==WTK_QENVELOPE_CREST || state==WTK_QENVELOPE_FLAT)
        {
            if(qform9->cfg->use_sqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=1;
                }
            }
        }
        wtk_qform9_flush(qform9, qemsg->smsg, qemsg->cohv, 0, 0, is_end);
        wtk_qform9_push_envelope_msg(qform9, qemsg);
    }else if(is_end)
    {
        wtk_qform9_flush2(qform9, NULL, NULL, 0, 0, 1);
    }
}
void wtk_qform9_on_qenvel2(wtk_qform9_t *qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_qform9_envelopemsg_t *qemsg;
    int k;
    int nbin=qform9->nbin;

    if(msg)
    {
        qemsg=(wtk_qform9_envelopemsg_t *)msg->hook;
        if(state==WTK_QENVELOPE_TROUGH)
        {
            if(qform9->cfg->use_nqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=-1;
                }
            }
        }else if(state==WTK_QENVELOPE_CREST || state==WTK_QENVELOPE_FLAT)
        {
            if(qform9->cfg->use_sqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=1;
                }
            }
        }
        wtk_qform9_flush(qform9, qemsg->smsg, qemsg->cohv, 1, 0, is_end);
        wtk_qform9_push_envelope_msg(qform9, qemsg);
    }else if(is_end)
    {
        wtk_qform9_flush2(qform9, NULL, NULL, 1, 0, 1);
    }
}
void wtk_qform9_on_qenvelope(wtk_qform9_t *qform9,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end)
{
    wtk_qform9_envelopemsg_t *qemsg;
    int k;
    int nbin=qform9->nbin;

    if(msg)
    {
        qemsg=(wtk_qform9_envelopemsg_t *)msg->hook;
        if(state==WTK_QENVELOPE_TROUGH)
        {
            if(qform9->cfg->use_nqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=-1;
                }
            }
        }else if(state==WTK_QENVELOPE_CREST || state==WTK_QENVELOPE_FLAT)
        {
            if(qform9->cfg->use_sqenvelope){
                for(k=1; k<nbin-1; ++k)
                {
                    qemsg->cohv[k]=1;
                }
            }
        }
        wtk_qform9_flush(qform9, qemsg->smsg, qemsg->cohv, 0, 0, is_end);
        wtk_qform9_push_envelope_msg(qform9, qemsg);
    }else if(is_end)
    {
        wtk_qform9_flush2(qform9, NULL, NULL, 0, 0, 1);
    }
}

void wtk_qform9_update_aspec2(wtk_qform9_t *qform9, wtk_aspec_t *aspec, wtk_complex_t *cov, 
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
    if(qform9->cfg->cohv_thresh>0.0 && spec_k[0]<qform9->cfg->cohv_thresh)
    {
        *cohv=-1;
    }
}


void wtk_qform9_update_naspec2(wtk_qform9_t *qform9, wtk_aspec_t *naspec, wtk_complex_t *cov, 
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

float wtk_qform9_entropy(wtk_qform9_t *qform9, wtk_complex_t **fftx)
{
    int rate = qform9->cfg->rate;
    int wins = qform9->cfg->stft2.win;
    int i;
    int fx1 = (250*1.0*wins)/rate;
    int fx2 = (3500*1.0*wins)/rate;
    int km = floor(wins*1.0/8);
    float K = 0.5;
    float *E=qform9->entropy_E;
    float P1;
    float *Eb=qform9->entropy_Eb;
    float sum;
    float prob;
    float Hb;
    float fft_scale=2048.0; // 跟fft缩放有关

    memset(E, 0, sizeof(float) * qform9->nbin);
    memset(Eb, 0, sizeof(float) * wins);
    for(i=fx1;i<fx2;++i){
        E[i] = fftx[i]->a*fft_scale*fftx[i]->a*fft_scale + fftx[i]->b*fft_scale*fftx[i]->b*fft_scale;
        // printf("%f %f %f\n", E[i], fftx[i]->a*32768.0/1024.0 * 64.0, fftx[i]->b*32768.0/1024.0 * 64.0);
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

void wtk_qform9_flush_aspec_lt(wtk_qform9_t *qform9, int index, int is_end)
{
    // float **mic_class=qform9->cfg->mic_class;
    // wtk_complex_t *covclass, *invcovclass;
    wtk_queue_t *stft2_q=&(qform9->stft2_q);
    wtk_queue_t *delay_q=&(qform9->delay_q);
    wtk_queue_t *delay_q2=&(qform9->delay_q2);
    wtk_queue_t *delay_q_class[2];
    wtk_queue_t *delay_q2_class[2];
    int lf=qform9->cfg->lf;
    int lt=qform9->cfg->lt;
    // int i,j,k,k2,tt,ff,c,c2;
    int i,j,k,k2,tt,ff;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg,*smsg_index;
    wtk_stft2_msg_t *smsg_index2;
    wtk_qform9_envelopemsg_t *qemsg_index2=NULL;
    int nbin=qform9->nbin;
    // int channel=qform9->bf->channel, channel2;
    int channel=qform9->bf->channel;
    wtk_complex_t *cov=qform9->cov;
    wtk_complex_t **fft,*fft1,*fft2,*a,*b;
    float *wint=qform9->wint;
    float *winf=qform9->winf;
    float wint2,wintf,winsum;
    wtk_complex_t *inv_cov=qform9->inv_cov;
    wtk_dcomplex_t *tmp=qform9->tmp;
    float cov_travg;
    int ret;
    float *cohv=qform9->cohv;
    float *cohv_class[3];
    float cohvtmp1,cohvtmp2,cohvtmp3;
    float spec_k[3]={0}, specsum;
    float specsum1, specsum2, specsum3;
    int specsum_ns=qform9->cfg->specsum_ns;
    int specsum_ne=qform9->cfg->specsum_ne;
    int ntheta_num=qform9->cfg->ntheta_num;
    float max_spec;
    float qenvel_alpha = qform9->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;
    float entropy=0;
    float entropy_thresh = qform9->cfg->entropy_thresh;
    int entropy_cnt = qform9->cfg->entropy_cnt;
    float cohv_sum=0;
    float cohv_sum_class[2]={0};
    float ncohv_alpha=qform9->cfg->ncohv_alpha;
    float ncohv_alpha_1=1.0-ncohv_alpha;
    float scohv_alpha=qform9->cfg->scohv_alpha;
    float scohv_alpha_1=1.0-scohv_alpha;

    ++qform9->nframe;
    qn=wtk_queue_peek(stft2_q, index);
    smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);

    specsum=0;
    specsum1=specsum2=specsum3=0;
    cohv_class[0]=qform9->cohv_class[0];
    cohv_class[1]=qform9->cohv_class[1];
    if(qform9->cohv_class[2]){
        cohv_class[2]=qform9->cohv_class[2];
    }

    if(qform9->cfg->use_qenvelope && qform9->cfg->use_cohv_cnt && entropy_thresh>0){
        entropy = wtk_qform9_entropy(qform9, smsg_index->fft);
        if(entropy_thresh>0){
            if(entropy<entropy_thresh){
                ++qform9->entropy_in_cnt;
            }else{
                qform9->entropy_in_cnt = 0;
            }
            if(qform9->entropy_in_cnt>=qform9->cfg->entropy_in_cnt){
                qform9->entropy_sil = 0;
                qform9->entropy_silcnt = entropy_cnt;
            }else if(qform9->entropy_sil==0){
                qform9->entropy_silcnt -= 1;
                if(qform9->entropy_silcnt<=0){
                    qform9->entropy_sil = 1;
                }
            }
        }
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

        if(qform9->aspec && qform9->naspec)
        {
            cov_travg=0;
            if(qform9->aspec->need_cov_travg) 
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }

            wtk_qform9_update_aspec2(qform9, qform9->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv+k);

            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }

            if(cohv[k]>0)
            {
                for(i=0; i<ntheta_num; ++i)
                {
                    wtk_qform9_update_naspec2(qform9, qform9->naspec[i], cov, inv_cov, cov_travg, k, spec_k, cohv+k);

                    if(cohv[k]<0)
                    {
                        break;
                    }
                }
            }
        }else if(qform9->aspec)
        {
            cov_travg=0;
            if(qform9->aspec->need_cov_travg) 
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }

            wtk_qform9_update_aspec2(qform9, qform9->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv+k);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
                cohv_sum += 1;
            }
        }else if(qform9->naspec)
        {
            cov_travg=0;
            if(qform9->naspec[0]->need_cov_travg)
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }

            for(i=0; i<ntheta_num; ++i)
            {
                wtk_qform9_update_naspec2(qform9, qform9->naspec[i], cov, inv_cov, cov_travg, k, spec_k, cohv+k);

                if(cohv[k]<0)
                {
                    break;
                }
            }
        }else if(qform9->aspec_class[0] && (qform9->cfg->use_two_channel || qform9->cfg->use_noise_qenvelope))
        {
            cov_travg=0;
            if(qform9->aspec_class[0]->need_cov_travg)
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }
            wtk_qform9_update_aspec2(qform9, qform9->aspec_class[0], cov, inv_cov, cov_travg, k, spec_k, cohv_class[0]+k);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum1+=spec_k[0]*2-spec_k[1]-spec_k[2];
                cohv_sum_class[0] += 1;
            }

            wtk_qform9_update_aspec2(qform9, qform9->aspec_class[1], cov, inv_cov, cov_travg, k, spec_k, cohv_class[1]+k);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum2+=spec_k[0]*2-spec_k[1]-spec_k[2];
                cohv_sum_class[1] += 1;
            }
        }else if(qform9->aspec_class[0])
        {
            cov_travg=0;
            if(qform9->aspec_class[0]->need_cov_travg)
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }
            wtk_qform9_update_aspec2(qform9, qform9->aspec_class[0], cov, inv_cov, cov_travg, k, spec_k, &cohvtmp1);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum1+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
            wtk_qform9_update_aspec2(qform9, qform9->aspec_class[1], cov, inv_cov, cov_travg, k, spec_k, &cohvtmp2);
            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum2+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
            wtk_qform9_update_aspec2(qform9, qform9->aspec_class[2], cov, inv_cov, cov_travg, k, spec_k, &cohvtmp3);
            if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum3+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
            cohv[k] = (cohvtmp1 + cohvtmp2 + cohvtmp3) > -3?1:0;
        }
    }
    if(qform9->cfg->use_simple_qenvelope){
        if(qform9->cfg->use_two_channel){
            if(qform9->entropy_sil==1){
                if(entropy_thresh>0){
                    ++qform9->cohv_frame_class[0];
                    ++qform9->cohv_frame_class[1];
                    if(qform9->cohv_frame_class[0]<=qform9->cfg->cohv_init_frame){
                        qform9->ncohv_sum_class[0] += cohv_sum_class[0]*1.0/qform9->cfg->cohv_init_frame;
                        qform9->ncohv_sum_class[1] += cohv_sum_class[1]*1.0/qform9->cfg->cohv_init_frame;
                    }else{
                        qform9->ncohv_sum_class[0] = qform9->ncohv_sum_class[0] * ncohv_alpha + cohv_sum_class[0] * ncohv_alpha_1;
                        qform9->ncohv_sum_class[1] = qform9->ncohv_sum_class[1] * ncohv_alpha + cohv_sum_class[1] * ncohv_alpha_1;
                    }
                }
            }
            qform9->scohv_sum_class[0] = qform9->scohv_sum_class[0] * scohv_alpha + cohv_sum_class[0] * scohv_alpha_1;
            qform9->scohv_sum_class[1] = qform9->scohv_sum_class[1] * scohv_alpha + cohv_sum_class[1] * scohv_alpha_1;
        }else{
            if(qform9->entropy_sil==1){
                if(entropy_thresh>0){
                    ++qform9->cohv_frame;
                    if(qform9->cohv_frame<=qform9->cfg->cohv_init_frame){
                        qform9->ncohv_sum += cohv_sum*1.0/qform9->cfg->cohv_init_frame;
                    }else{
                        qform9->ncohv_sum = qform9->ncohv_sum * ncohv_alpha + cohv_sum * ncohv_alpha_1;
                    }
                }
            }else{
                if(specsum<qform9->cfg->qenvl.right_min_thresh){
                    // qform9->ncohv_sum = qform9->ncohv_sum * ncohv_alpha + ncohv_sum * ncohv_alpha_1;
                }
            }
            qform9->scohv_sum = qform9->scohv_sum * scohv_alpha + cohv_sum * scohv_alpha_1;
        }
    }
    if(qform9->ncohv_fn){
        fprintf(qform9->ncohv_fn,"%.0f %f\n",qform9->nframe,qform9->ncohv_sum);
    }
    if(qform9->scohv_fn){
        fprintf(qform9->scohv_fn,"%.0f %f\n",qform9->nframe,qform9->scohv_sum);
    }
    if(qform9->entropy_fn){
        fprintf(qform9->entropy_fn,"%.0f %f\n",qform9->nframe,entropy);
    }
    // printf("entropy_fn, qform9->ncohv_sum);
    // printf("%f\n", qform9->scohv_sum);
    // printf("%f\n", ncohv_sum);
    // printf("%d\n", qform9->entropy_sil);
    // printf("%f\n", entropy);
    if(qform9->cfg->use_specsum_bl && specsum<=qform9->cfg->specsum_bl)
    {
        for(k=0;k<nbin;++k)
        {
            cohv[k]=-1;
        }
    }
    if(qform9->cfg->use_two_aspecclass){
        specsum = max(max(specsum1, specsum2), specsum3);
    }
    if(qform9->cfg->use_simple_qenvelope){
        if(qform9->cfg->use_two_channel){
            wtk_fring_push2(qform9->q_fring_class[0], specsum1);
            wtk_fring_push2(qform9->q_fring_class[1], specsum2);
            for(i=0;i<2;++i){
                if(i==0){
                    min_speccrest = qform9->cfg->qenvl.min_speccrest;
                    right_nf = qform9->cfg->qenvl.right_nf;
                    envelope_thresh = qform9->cfg->qenvl.envelope_thresh;
                    right_min_thresh = qform9->cfg->qenvl.right_min_thresh;
                }else if(i==1){
                    min_speccrest = qform9->cfg->qenvl2.min_speccrest;
                    right_nf = qform9->cfg->qenvl2.right_nf;
                    envelope_thresh = qform9->cfg->qenvl2.envelope_thresh;
                    right_min_thresh = qform9->cfg->qenvl2.right_min_thresh;
                }
                if(qform9->q_fring_class[i]->used == qform9->q_fring_class[i]->nslot - 1){
                    max_spec = wtk_fring_max(qform9->q_fring_class[i]);
                    if(max_spec < qform9->q_spec_class[i]){
                        max_spec = qenvel_alpha * qform9->q_spec_class[i] + qenvel_alpha_1 * max_spec; 
                    }
                    // printf("%f\n", max_spec);
                    qform9->q_spec_class[i] = max_spec;
                    if(max_spec > min_speccrest){
                        qform9->right_nf_class[i] = right_nf;
                    }else if(max_spec > envelope_thresh){
                        // --qform9->right_nf;
                    }else if(max_spec > right_min_thresh){
                        --qform9->right_nf_class[i];
                    }else{
                        qform9->right_nf_class[i] = 0;
                    }

                    if(qform9->cfg->use_cohv_cnt){
                        if(qform9->cohv_frame_class[i]>qform9->cfg->cohv_init_frame){
                            if(qform9->ncohv_sum_class[i]<qform9->scohv_sum_class[i]*qform9->cfg->nscohv_scale && qform9->entropy_sil==0 && max_spec > envelope_thresh){
                                qform9->right_nf_class[i] = right_nf;
                            }
                            if(qform9->ncohv_sum_class[i]>qform9->scohv_sum_class[i]*qform9->cfg->nscohv_scale2){
                                qform9->ncohv_cnt_class[i]=qform9->right_nf_class[i];
                            }else if(qform9->ncohv_sum_class[i]>qform9->scohv_sum_class[i]*qform9->cfg->nscohv_scale3){
                            }else if(qform9->ncohv_sum_class[i]>qform9->scohv_sum_class[i]){
                                --qform9->ncohv_cnt_class[i];
                            }else{
                                qform9->ncohv_cnt_class[i]=0;

                            }
                            if(qform9->ncohv_cnt_class[i]>0){
                                qform9->right_nf_class[i]=0;
                            }
                        }
                    }

                    if(qform9->cfg->delay_nf<=0){
                        if(qform9->right_nf_class[i] <= 0){
                            if(qform9->cfg->use_nqenvelope){
                                for(k=1; k<nbin-1; ++k)
                                {
                                    cohv_class[i][k]=-1;
                                }
                            }
                            --qform9->noise_debug_cnt_class[i];
                        }else{
                            if(qform9->cfg->use_sqenvelope){
                                for(k=1; k<nbin-1; ++k)
                                {
                                    cohv_class[i][k]=1;
                                }
                            }
                            qform9->noise_debug_cnt_class[i] = qform9->cfg->noise_debug_cnt;
                        }
                    }
                }

                if(qform9->cfg->delay_nf>0){
                    if(i==0){
                        delay_q_class[0] = &(qform9->delay_q_class[0]);
                        smsg_index2 = wtk_qform9_stft2_msg_copy(qform9, smsg_index, qform9->bf_class[0]->channel, nbin);
                        wtk_queue_push(delay_q_class[0],&(smsg_index2->q_n));
                        delay_q_class[1] = &(qform9->delay_q_class[1]);
                        smsg_index2 = wtk_qform9_stft2_msg_copy(qform9, smsg_index, qform9->bf_class[0]->channel, nbin);
                        wtk_queue_push(delay_q_class[1],&(smsg_index2->q_n));
                    }
                    delay_q2_class[i] = &(qform9->delay_q2_class[i]);
                    qemsg_index2 = wtk_qform9_envelope_msg_copy(qform9, smsg_index, cohv_class[i], nbin, qform9->bf_class[i]->channel);
                    wtk_queue_push(delay_q2_class[i],&(qemsg_index2->q_n));
                    if(qform9->right_nf_class[i] <= 0){
                        qform9->delay_cnt_class[i] = qform9->cfg->delay_nf;
                    }else{
                        --qform9->delay_cnt_class[i];
                    }
                    if(is_end){
                        while(delay_q_class[i]->length>0){
                            if(delay_q_class[i]->length==1){
                                qform9->delay_end_pos_class[i] = qform9->end_pos;
                                is_end = 1;
                            }else{
                                is_end = 0;
                            }
                            qn=wtk_queue_pop(delay_q_class[i]);
                            smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);
                            qn=wtk_queue_pop(delay_q2_class[i]);
                            qemsg_index2=data_offset2(qn,wtk_qform9_envelopemsg_t,q_n);
                            cohv_class[i]=qemsg_index2->cohv;
                            if(qform9->cfg->use_noise_debug){
                                if(qform9->noise_debug_cnt_class[i]<=0){
                                    wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 1, is_end);
                                }else{
                                    wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, is_end);
                                }
                            }else if(qform9->cfg->use_sound_debug){
                                if(qform9->noise_debug_cnt_class[i]<=0){
                                    wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, is_end);
                                }else{
                                    wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, -1, is_end);
                                }
                            }else{
                                wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, is_end);
                            }
                            wtk_qform9_push_stft2_msg(qform9, smsg_index);
                            wtk_qform9_push_envelope_msg(qform9, qemsg_index2);
                        }
                    }else if(qform9->delay_nf_class[i]==0){
                        qn=wtk_queue_pop(delay_q_class[i]);
                        smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);
                        qn=wtk_queue_pop(delay_q2_class[i]);
                        qemsg_index2=data_offset2(qn,wtk_qform9_envelopemsg_t,q_n);
                        cohv_class[i]=qemsg_index2->cohv;
                        if(qform9->right_nf_class[i] <= 0){
                            if(qform9->cfg->use_nqenvelope){
                                for(k=1; k<nbin-1; ++k)
                                {
                                    cohv_class[i][k]=-1;
                                }
                            }
                            --qform9->noise_debug_cnt_class[i];
                        }else{
                            if(qform9->cfg->use_sqenvelope){
                                for(k=1; k<nbin-1; ++k)
                                {
                                    cohv_class[i][k]=1;
                                }
                            }
                            qform9->noise_debug_cnt_class[i] = qform9->cfg->noise_debug_cnt;
                        }
                        if(qform9->cfg->use_noise_debug){
                            if(qform9->noise_debug_cnt_class[i]<=0){
                                wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 1, 0);
                            }else{
                                wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, 0);
                            }
                        }else if(qform9->cfg->use_sound_debug){
                            if(qform9->noise_debug_cnt_class[i]<=0){
                                wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, 0);
                            }else{
                                wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, -1, 0);
                            }
                        }else{
                            wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, 0);
                        }
                        wtk_qform9_push_stft2_msg(qform9, smsg_index);
                        wtk_qform9_push_envelope_msg(qform9, qemsg_index2);
                    }else{
                        --qform9->delay_nf_class[i];
                    }
                }else{
                    if(qform9->cfg->use_noise_debug){
                        if(qform9->noise_debug_cnt_class[i]<=0){
                            wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 1, is_end);
                        }else{
                            wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, is_end);
                        }
                    }else if(qform9->cfg->use_sound_debug){
                        if(qform9->noise_debug_cnt_class[i]<=0){
                            wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, is_end);
                        }else{
                            wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, -1, is_end);
                        }
                    }else{
                        wtk_qform9_flush(qform9, smsg_index, cohv_class[i], i, 0, is_end);
                    }
                }
            }
        }else if(qform9->cfg->use_noise_qenvelope){
            wtk_fring_push2(qform9->q_fring_class[0], specsum1);
            wtk_fring_push2(qform9->q_fring_class[1], specsum2);
            wtk_fring_push2(qform9->q_fring_class[2], specsum1);
            for(i=0;i<3;++i){
                if(i==0){
                    min_speccrest = qform9->cfg->qenvl.min_speccrest;
                    right_nf = qform9->cfg->qenvl.right_nf;
                    envelope_thresh = qform9->cfg->qenvl.envelope_thresh;
                    right_min_thresh = qform9->cfg->qenvl.right_min_thresh;
                }else if(i==1){
                    min_speccrest = qform9->cfg->qenvl2.min_speccrest;
                    right_nf = qform9->cfg->qenvl2.right_nf;
                    envelope_thresh = qform9->cfg->qenvl2.envelope_thresh;
                    right_min_thresh = qform9->cfg->qenvl2.right_min_thresh;
                }else if(i==2){
                    min_speccrest = qform9->cfg->qenvl3.min_speccrest;
                    right_nf = qform9->cfg->qenvl3.right_nf;
                    envelope_thresh = qform9->cfg->qenvl3.envelope_thresh;
                    right_min_thresh = qform9->cfg->qenvl3.right_min_thresh;
                }
                if(qform9->q_fring_class[i]->used == qform9->q_fring_class[i]->nslot - 1){
                    max_spec = wtk_fring_max(qform9->q_fring_class[i]);
                    if(max_spec < qform9->q_spec_class[i]){
                        max_spec = qenvel_alpha * qform9->q_spec_class[i] + qenvel_alpha_1 * max_spec; 
                    }
                    // printf("%f\n", max_spec);
                    qform9->q_spec_class[i] = max_spec;
                    if(max_spec > min_speccrest){
                        qform9->right_nf_class[i] = right_nf;
                    }else if(max_spec > envelope_thresh){
                        // --qform9->right_nf;
                    }else if(max_spec > right_min_thresh){
                        --qform9->right_nf_class[i];
                    }else{
                        qform9->right_nf_class[i] = 0;
                    }
                    if(qform9->right_nf_class[i] <= 0){
                        if(qform9->cfg->use_nqenvelope){
                            for(k=1; k<nbin-1; ++k)
                            {
                                cohv_class[i][k]=-1;
                            }
                        }
                    }else{
                        if(qform9->cfg->use_sqenvelope){
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
            wtk_qform9_flush(qform9, smsg_index, cohv_class[0], 0, 0, is_end);
        }else{
            wtk_fring_push2(qform9->q_fring, specsum);
            min_speccrest = qform9->cfg->qenvl.min_speccrest;
            right_nf = qform9->cfg->qenvl.right_nf;
            envelope_thresh = qform9->cfg->qenvl.envelope_thresh;
            right_min_thresh = qform9->cfg->qenvl.right_min_thresh;
            if(qform9->q_fring->used == qform9->q_fring->nslot - 1){
                max_spec = wtk_fring_max(qform9->q_fring);
                if(max_spec < qform9->q_spec){
                    max_spec = qenvel_alpha * qform9->q_spec + qenvel_alpha_1 * max_spec; 
                }
                if(qform9->cohv_fn)
                {
                    fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,max_spec);
                }
                // printf("%f\n", max_spec);
                qform9->q_spec = max_spec;
                if(max_spec > min_speccrest){
                    qform9->right_nf = right_nf;
                }else if(max_spec > envelope_thresh){
                    // --qform9->right_nf;
                }else if(max_spec > right_min_thresh){
                    --qform9->right_nf;
                }else{
                    qform9->right_nf = 0;
                }
                if(qform9->cfg->use_cohv_cnt){
                    if(qform9->cohv_frame>qform9->cfg->cohv_init_frame){
                        if(qform9->ncohv_sum<qform9->scohv_sum*qform9->cfg->nscohv_scale && qform9->entropy_sil==0 && max_spec > envelope_thresh){
                            qform9->right_nf = right_nf;
                        }
                        if(qform9->ncohv_sum>qform9->scohv_sum*qform9->cfg->nscohv_scale2){
                            qform9->ncohv_cnt=qform9->right_nf;
                        }else if(qform9->ncohv_sum>qform9->scohv_sum*qform9->cfg->nscohv_scale3){
                        }else if(qform9->ncohv_sum>qform9->scohv_sum){
                            --qform9->ncohv_cnt;
                        }else{
                            qform9->ncohv_cnt=0;

                        }
                        if(qform9->ncohv_cnt>0){
                            qform9->right_nf=0;
                        }
                    }
                }
                if(qform9->cfg->delay_nf<=0){
                    if(qform9->right_nf <= 0){
                        if(qform9->cfg->use_nqenvelope){
                            for(k=1; k<nbin-1; ++k)
                            {
                                cohv[k]=-1;
                            }
                        }
                        --qform9->noise_debug_cnt;
                    }else{
                        if(qform9->cfg->use_sqenvelope){
                            for(k=1; k<nbin-1; ++k)
                            {
                                cohv[k]=1;
                            }
                        }
                        qform9->noise_debug_cnt=qform9->cfg->noise_debug_cnt;
                    }
                }
            }else{
                // printf("0\n");
            }

            if(qform9->cfg->delay_nf>0){
                smsg_index2 = wtk_qform9_stft2_msg_copy(qform9, smsg_index, qform9->bf->channel, nbin);
                qemsg_index2 = wtk_qform9_envelope_msg_copy(qform9, smsg_index, cohv, nbin, qform9->bf->channel);
                wtk_queue_push(delay_q,&(smsg_index2->q_n));
                wtk_queue_push(delay_q2,&(qemsg_index2->q_n));
                if(qform9->right_nf<=0){
                    qform9->delay_cnt=qform9->cfg->delay_nf;
                }else{
                    --qform9->delay_cnt;
                }
                if(is_end){
                    while(delay_q->length>0){
                        if(delay_q->length==1){
                            qform9->delay_end_pos = qform9->end_pos;
                            is_end = 1;
                        }else{
                            is_end = 0;
                        }
                        qn=wtk_queue_pop(delay_q);
                        smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);
                        qn=wtk_queue_pop(delay_q2);
                        qemsg_index2=data_offset2(qn,wtk_qform9_envelopemsg_t,q_n);
                        cohv=qemsg_index2->cohv;
                        if(qform9->cfg->use_noise_debug){
                            if(qform9->noise_debug_cnt<=0){
                                wtk_qform9_flush(qform9, smsg_index, cohv, 0, 1, is_end);
                            }else{
                                wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
                            }
                        }else if(qform9->cfg->use_sound_debug){
                            if(qform9->noise_debug_cnt<=0){
                                wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
                            }else{
                                wtk_qform9_flush(qform9, smsg_index, cohv, 0, -1, is_end);
                            }
                        }else{
                            wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
                        }
                        wtk_qform9_push_stft2_msg(qform9, smsg_index);
                        wtk_qform9_push_envelope_msg(qform9, qemsg_index2);
                    }
                }else if(qform9->delay_nf==0){
                    qn=wtk_queue_pop(delay_q);
                    smsg_index=data_offset2(qn,wtk_stft2_msg_t,q_n);
                    qn=wtk_queue_pop(delay_q2);
                    qemsg_index2=data_offset2(qn,wtk_qform9_envelopemsg_t,q_n);
                    cohv=qemsg_index2->cohv;
                    if(qform9->right_nf <= 0){
                        if(qform9->cfg->use_nqenvelope){
                            for(k=1; k<nbin-1; ++k)
                            {
                                cohv[k]=-1;
                            }
                        }
                        --qform9->noise_debug_cnt;
                    }else{
                        if(qform9->cfg->use_sqenvelope){
                            for(k=1; k<nbin-1; ++k)
                            {
                                cohv[k]=1;
                            }
                        }
                        qform9->noise_debug_cnt=qform9->cfg->noise_debug_cnt;
                    }
                    if(qform9->cfg->use_qmmse_param){
                        if(qform9->right_nf <= 0){
                            ++qform9->sil_in_cnt;
                        }else{
                            qform9->sil_in_cnt = 0;
                        }
                        if(qform9->sil_in_cnt>qform9->cfg->sil_in_cnt){
                            qform9->sil_out_cnt = qform9->cfg->sil_out_cnt;
                        }else if(qform9->sil_out_cnt>0){
                            --qform9->sil_out_cnt;
                        }
                        if(qform9->sil_out_cnt>0){
                            qform9->qmmse->cfg->max_range = qform9->cfg->sil_max_range;
                            qform9->qmmse->cfg->noise_suppress = qform9->cfg->sil_noise_suppress;
                        }else{
                            qform9->qmmse->cfg->max_range = qform9->sil_max_range;
                            qform9->qmmse->cfg->noise_suppress = qform9->sil_noise_suppress;
                        }
                    }
                    if(qform9->cfg->use_noise_debug){
                        if(qform9->noise_debug_cnt<=0){
                            wtk_qform9_flush(qform9, smsg_index, cohv, 0, 1, 0);
                        }else{
                            wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, 0);
                        }
                    }else if(qform9->cfg->use_sound_debug){
                        if(qform9->noise_debug_cnt<=0){
                            wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, 0);
                        }else{
                            wtk_qform9_flush(qform9, smsg_index, cohv, 0, -1, 0);
                        }
                    }else{
                        wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, 0);
                    }
                    wtk_qform9_push_stft2_msg(qform9, smsg_index);
                    wtk_qform9_push_envelope_msg(qform9, qemsg_index2);
                }else{
                    --qform9->delay_nf;
                }
            }else{
                if(qform9->cfg->use_noise_debug){
                    if(qform9->noise_debug_cnt<=0){
                        wtk_qform9_flush(qform9, smsg_index, cohv, 0, 1, is_end);
                    }else{
                        wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
                    }
                }else if(qform9->cfg->use_sound_debug){
                    if(qform9->noise_debug_cnt<=0){
                        wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
                    }else{
                        wtk_qform9_flush(qform9, smsg_index, cohv, 0, -1, is_end);
                    }
                }else{
                    wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
                }
            }
        }
    }else if(qform9->cfg->use_qenvelope && qform9->cfg->use_two_channel)
    {
        wtk_qform9_envelopemsg_t *qemsg1;
        wtk_qform9_envelopemsg_t *qemsg2;

        if(!is_end){
            qemsg1=wtk_qform9_envelope_msg_copy(qform9, smsg_index, cohv_class[0], nbin, channel);
            wtk_qenvelope_feed(qform9->qenvel[0], specsum1, (void *)qemsg1, is_end);
            qemsg2=wtk_qform9_envelope_msg_copy(qform9, smsg_index, cohv_class[1], nbin, channel);
            wtk_qenvelope_feed(qform9->qenvel[1], specsum2, (void *)qemsg2, is_end);
        }else{
            wtk_qenvelope_feed(qform9->qenvel[0], 0, NULL, 1);
            wtk_qform9_flush(qform9, smsg_index, cohv_class[0], 0, 0, 1);
            wtk_qenvelope_feed(qform9->qenvel[1], 0, NULL, 1);
            wtk_qform9_flush(qform9, smsg_index, cohv_class[1], 1, 0, 1);
        }
    }else if(qform9->cfg->use_qenvelope)
    {
        wtk_qform9_envelopemsg_t *qemsg;

        if(!is_end){
            qemsg=wtk_qform9_envelope_msg_copy(qform9, smsg_index, cohv, nbin, channel);
            wtk_qenvelope_feed(qform9->qenvelope, specsum, (void *)qemsg, is_end);
        }else{
            wtk_qenvelope_feed(qform9->qenvelope, 0, NULL, 1);
            wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
        }
    }else
    {
        if(qform9->cohv_fn)
        {
            fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,specsum);
        }
        wtk_qform9_flush(qform9, smsg_index, cohv, 0, 0, is_end);
    }
}


void wtk_qform9_update_aspec(wtk_qform9_t *qform9, wtk_aspec_t *aspec, wtk_complex_t **fft, float fftabs2, int k, float *spec_k, float *cohv)
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
    if(qform9->cfg->cohv_thresh>0.0 && spec_k[0]<qform9->cfg->cohv_thresh)
    {
        *cohv=-1;
    }
}


void wtk_qform9_update_naspec(wtk_qform9_t *qform9, wtk_aspec_t *naspec, wtk_complex_t **fft, float fftabs2, int k, float *spec_k, float *cohv)
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

void wtk_qform9_flush_aspec(wtk_qform9_t *qform9, wtk_stft2_msg_t *msg, int is_end)
{
    int k,i,c;
    int nbin=qform9->nbin;
    int channel=qform9->bf->channel, ch1,ch2;
    wtk_complex_t **fft, *fft2, **fftclass, **fftclass2;
    float **mic_class=qform9->cfg->mic_class;
    float fftabs2;
    float spec_k[3]={0}, specsum;
    float *cohv=qform9->cohv;
    float cohvtmp1,cohvtmp2;
    int specsum_ns=qform9->cfg->specsum_ns;
    int specsum_ne=qform9->cfg->specsum_ne;
    int ntheta_num=qform9->cfg->ntheta_num;
    float max_spec;
    float qenvel_alpha = qform9->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;

    ++qform9->nframe;

    fft=msg->fft;

    if(qform9->aspec_class[0])
    {
        fftclass=qform9->fftclass[0];
        fftclass2=qform9->fftclass[1];
        ch1=qform9->cfg->aspec_class1.channel;
        ch2=qform9->cfg->aspec_class2.channel;

        for(k=1; k<nbin-1; ++k)
        {
            for(i=0;i<ch1;++i)
            {
                c=mic_class[0][i];
                fftclass[k][i]=fft[k][c];
            }
            for(i=0;i<ch2;++i)
            {
                c=mic_class[1][i];
                fftclass2[k][i]=fft[k][c];
            }
        }   
    }

    specsum=0;
    for(k=1; k<nbin-1; ++k)
    {
        if(qform9->aspec && qform9->naspec)
        {
            fftabs2=0;
            fft2=fft[k];
            for(i=0; i<channel; ++i,++fft2)
            {
                fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
            }

            wtk_qform9_update_aspec(qform9,qform9->aspec,fft,fftabs2,k,spec_k,cohv+k);

            if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }

            if(cohv[k]>0)
            {
                for(i=0; i<ntheta_num; ++i)
                {
                    wtk_qform9_update_naspec(qform9,qform9->naspec[i],fft,fftabs2,k,spec_k,cohv+k);

                    if(cohv[k]<0)
                    {
                        break;
                    }
                }
            }
        }else if(qform9->aspec)
        {
            fftabs2=0;
            fft2=fft[k];
            for(i=0; i<channel; ++i,++fft2)
            {
                fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
            }

            wtk_qform9_update_aspec(qform9,qform9->aspec,fft,fftabs2,k,spec_k,cohv+k);

            if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
        }else if(qform9->naspec)
        {
            fftabs2=0;
            fft2=fft[k];
            for(i=0; i<channel; ++i,++fft2)
            {
                fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
            }

            for(i=0; i<ntheta_num; ++i)
            {
                wtk_qform9_update_naspec(qform9,qform9->naspec[i],fft,fftabs2,k,spec_k,cohv+k);

                if(cohv[k]<0)
                {
                    break;
                }
            }
        }else if(qform9->aspec_class[0])
        {
            fftclass=qform9->fftclass[0];
            ch1=qform9->cfg->aspec_class1.channel;

            fftabs2=0;
            fft2=fftclass[k];
            for(i=0; i<ch1; ++i, ++fft2)
            {
                fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
            }
            wtk_qform9_update_aspec(qform9,qform9->aspec_class[0],fftclass,fftabs2,k,spec_k,&cohvtmp1);

            // if(k==64)
            // {
            //     printf("%f %f %f %.1f\n",spec_k[0],spec_k[1],spec_k[2],cohvtmp1);
            // }

            fftclass2=qform9->fftclass[1];
            ch2=qform9->cfg->aspec_class2.channel;

            fftabs2=0;
            fft2=fftclass2[k];
            for(i=0; i<ch1; ++i, ++fft2)
            {
                fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
            }
            wtk_qform9_update_aspec(qform9,qform9->aspec_class[1],fftclass2,fftabs2,k,spec_k,&cohvtmp2);

            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
            if(qform9->cfg->use_twojoin)
            {
                if(cohvtmp1>0 || cohvtmp2>0)
                {
                    cohv[k]=1;
                }else
                {
                    cohv[k]=-1;
                }
            }else
            {
                if(cohvtmp1>0 && cohvtmp2>0)
                {
                    cohv[k]=1;
                }else
                {
                    cohv[k]=-1;
                }
            }
        }
    }
    if(qform9->cfg->use_specsum_bl && specsum<=qform9->cfg->specsum_bl)
    {
        for(k=0;k<nbin;++k)
        {
            cohv[k]=-1;
        }
    }
    if(qform9->cfg->use_simple_qenvelope){
        wtk_fring_push2(qform9->q_fring, specsum);
        min_speccrest = qform9->cfg->qenvl.min_speccrest;
        right_nf = qform9->cfg->qenvl.right_nf;
        envelope_thresh = qform9->cfg->qenvl.envelope_thresh;
        right_min_thresh = qform9->cfg->qenvl.right_min_thresh;
        if(qform9->q_fring->used == qform9->q_fring->nslot - 1){
            max_spec = wtk_fring_max(qform9->q_fring);
            if(max_spec < qform9->q_spec){
                max_spec = qenvel_alpha * qform9->q_spec + qenvel_alpha_1 * max_spec; 
            }
            if(qform9->cohv_fn)
            {
                fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,max_spec);
            }
            // printf("%f\n", max_spec);
            qform9->q_spec = max_spec;
            if(max_spec > min_speccrest){
                qform9->right_nf = right_nf;
            }else if(max_spec > envelope_thresh){
                // --qform9->right_nf;
            }else if(max_spec > right_min_thresh){
                --qform9->right_nf;
            }else{
                qform9->right_nf = 0;
            }
            if(qform9->right_nf <= 0){
                if(qform9->cfg->use_nqenvelope){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=-1;
                    }
                }
            }else{
                if(qform9->cfg->use_sqenvelope){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=1;
                    }
                }
            }
        }
        wtk_qform9_flush(qform9, msg, cohv, 0, 0, is_end);
    }else if(qform9->cfg->use_qenvelope)
    {
        wtk_qform9_envelopemsg_t *qemsg;

        if(!is_end){
            qemsg=wtk_qform9_envelope_msg_copy(qform9, msg, cohv, nbin, channel);
            wtk_qenvelope_feed(qform9->qenvelope, specsum, (void *)qemsg, is_end);
        }else{
            wtk_qenvelope_feed(qform9->qenvelope, 0, NULL, 1);
            wtk_qform9_flush(qform9, msg, cohv, 0, 0, is_end);
        }
    }else
    {
        if(qform9->cohv_fn)
        {
            fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,specsum);
        }
        wtk_qform9_flush(qform9, msg, cohv, 0, 0, is_end);
    }
}

void wtk_qform9_on_stft2(wtk_qform9_t *qform9,wtk_stft2_msg_t *msg,int pos,int is_end)
{
    wtk_queue_t *stft2_q=&(qform9->stft2_q);
    int lt=qform9->cfg->lt;
    wtk_queue_node_t *qn;
    wtk_stft2_msg_t *smsg;
    int i;

    if(is_end)
    {
        qform9->end_pos=pos;
    }
    if(qform9->cov)
    {
        if(msg)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
        }
        if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
        {
            wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-lt-1, 0);
        }else if(stft2_q->length==2*lt+1)
        {
            wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-lt-1, (is_end && lt==0)?1: 0);
            qn=wtk_queue_pop(stft2_q);
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            wtk_stft2_push_msg(qform9->stft2,smsg);
        }else if(is_end && stft2_q->length==0)
        {
            wtk_qform9_flush2(qform9, NULL, NULL, 0, 0, 1);
        }
        if(is_end)
        {
            if(stft2_q->length>0)
            {
                if(stft2_q->length<lt+1)
                {
                    for(i=0; i<stft2_q->length-1; ++i)
                    {
                        wtk_qform9_flush_aspec_lt(qform9, i, 0);
                    }
                    wtk_qform9_flush_aspec_lt(qform9, stft2_q->length-1, 1);
                }else
                {
                    for(i=0; i<lt-1; ++i)
                    {
                        wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-lt+i, 0);   
                    }
                    wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-1, 1);
                }
            }
            while(qform9->stft2_q.length>0)
            {
                qn=wtk_queue_pop(&(qform9->stft2_q));
                if(!qn){break;}
                smsg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
                wtk_stft2_push_msg(qform9->stft2,smsg);
            }
        }
    }else
    {
        if(msg)
        {
            wtk_qform9_flush_aspec(qform9,msg,is_end);
            wtk_stft2_push_msg(qform9->stft2, msg); 
        }else if(is_end && !msg)
        {
            wtk_qform9_flush2(qform9, NULL, NULL, 0, 0, 1);
        }
    }   
}

void wtk_qform9_start_aspec1(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
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

void wtk_qform9_start_aspec2(wtk_aspec_t *aspec, float theta, float phi, float theta_range)
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

void wtk_qform9_start(wtk_qform9_t *qform9, float theta, float phi)
{
    int i;

    qform9->theta=theta;
    qform9->phi=phi;
    if(qform9->cfg->use_two_channel){
        wtk_bf_update_ovec(qform9->bf_class[0],qform9->cfg->theta_center_class1,0);
        wtk_bf_update_ovec(qform9->bf_class[1],qform9->cfg->theta_center_class2,0);
        wtk_bf_init_w(qform9->bf_class[0]);
        wtk_bf_init_w(qform9->bf_class[1]);
    }else{
        wtk_bf_update_ovec(qform9->bf,theta,phi);
        wtk_bf_init_w(qform9->bf);
    }

    if(qform9->cfg->use_two_channel || qform9->cfg->use_noise_qenvelope)
    {
        if(qform9->cfg->use_cline1)
        {
            wtk_qform9_start_aspec2(qform9->aspec_class[0], qform9->cfg->theta_center_class1, 0, qform9->cfg->theta_range_class1);
        }else
        {
            wtk_qform9_start_aspec1(qform9->aspec_class[0], qform9->cfg->theta_center_class1, 0, qform9->cfg->theta_range_class1);
        }

        if(qform9->cfg->use_cline2)
        {
            wtk_qform9_start_aspec2(qform9->aspec_class[1], qform9->cfg->theta_center_class2, 0, qform9->cfg->theta_range_class2);
        }else
        {      
            wtk_qform9_start_aspec1(qform9->aspec_class[1], qform9->cfg->theta_center_class2, 0, qform9->cfg->theta_range_class2);
        }
    }else if(qform9->cfg->use_two_aspecclass)
    {
        int theta2,theta3;
        if(qform9->cfg->use_line){
            if(qform9->cfg->theta_range <= 20){
                qform9->cfg->theta_center_class1=qform9->cfg->theta_center_class2=qform9->cfg->theta_center_class3=qform9->theta;
                qform9->cfg->theta_range_class=qform9->cfg->theta_range;
            }else if(qform9->cfg->theta_range <= 60){
                theta2=qform9->theta-(qform9->cfg->theta_range-20);
                theta3=qform9->theta+(qform9->cfg->theta_range-20);
                qform9->cfg->theta_center_class1=max(0, theta2);
                qform9->cfg->theta_center_class2=qform9->theta;
                qform9->cfg->theta_center_class3=min(180, theta3);
                qform9->cfg->theta_range_class=20;
            }else{
                theta2=qform9->theta-qform9->cfg->theta_range/3*2;
                theta3=qform9->theta+qform9->cfg->theta_range/3*2;
                qform9->cfg->theta_center_class1=max(0, theta2);
                qform9->cfg->theta_center_class2=qform9->theta;
                qform9->cfg->theta_center_class3=min(180, theta3);   
                qform9->cfg->theta_range_class=qform9->cfg->theta_range/3;
            }
            wtk_qform9_start_aspec2(qform9->aspec_class[0], qform9->cfg->theta_center_class1, 0, qform9->cfg->theta_range_class);
            wtk_qform9_start_aspec2(qform9->aspec_class[1], qform9->cfg->theta_center_class2, 0, qform9->cfg->theta_range_class);
            wtk_qform9_start_aspec2(qform9->aspec_class[2], qform9->cfg->theta_center_class3, 0, qform9->cfg->theta_range_class);
        }else{
            if(qform9->cfg->theta_range <= 20){
                qform9->cfg->theta_center_class1=qform9->cfg->theta_center_class2=qform9->cfg->theta_center_class3=qform9->theta;
                qform9->cfg->theta_range_class=qform9->cfg->theta_range;
            }else if(qform9->cfg->theta_range <= 60){
                theta2=qform9->theta-(qform9->cfg->theta_range-20);
                theta3=qform9->theta+(qform9->cfg->theta_range-20);
                qform9->cfg->theta_center_class1=max(0, theta2);
                qform9->cfg->theta_center_class2=qform9->theta;
                qform9->cfg->theta_center_class3=min(360, theta3);
                qform9->cfg->theta_range_class=20;
            }else{
                theta2=qform9->theta-qform9->cfg->theta_range/3*2;
                theta3=qform9->theta+qform9->cfg->theta_range/3*2;
                qform9->cfg->theta_center_class1=theta2<0?360-theta2:theta2;
                qform9->cfg->theta_center_class2=qform9->theta;
                qform9->cfg->theta_center_class3=theta3>360?theta3-360:theta3;
                qform9->cfg->theta_range_class=qform9->cfg->theta_range/3;
            }
            wtk_qform9_start_aspec1(qform9->aspec_class[0], qform9->cfg->theta_center_class1, 0, qform9->cfg->theta_range_class);
            wtk_qform9_start_aspec1(qform9->aspec_class[1], qform9->cfg->theta_center_class2, 0, qform9->cfg->theta_range_class);
            wtk_qform9_start_aspec1(qform9->aspec_class[2], qform9->cfg->theta_center_class3, 0, qform9->cfg->theta_range_class);
        }
    }else
    {
        if(qform9->cfg->use_line)
        {
            if(qform9->cfg->use_noiseblock || qform9->cfg->use_noiseblock2)
            {
                for(i=0; i<qform9->cfg->ntheta_num; ++i)
                {
                    wtk_qform9_start_aspec2(qform9->naspec[i], qform9->cfg->ntheta_center[i], phi, qform9->cfg->ntheta_range);
                }
            }

            if(!qform9->cfg->use_noiseblock || qform9->cfg->use_noiseblock2)
            {
                wtk_qform9_start_aspec2(qform9->aspec, theta, phi, qform9->cfg->theta_range);
            }
        }else
        {
            if(qform9->cfg->use_noiseblock || qform9->cfg->use_noiseblock2)
            {
                for(i=0; i<qform9->cfg->ntheta_num; ++i)
                {
                    wtk_qform9_start_aspec1(qform9->naspec[i], qform9->cfg->ntheta_center[i], phi, qform9->cfg->ntheta_range);
                }
            }

            if(!qform9->cfg->use_noiseblock || qform9->cfg->use_noiseblock2)
            {         
                wtk_qform9_start_aspec1(qform9->aspec, theta, phi, qform9->cfg->theta_range);
            }
        }
    }

    if(qform9->cfg->use_t_r_qenvelope){
        float **t_r_qenvl=qform9->cfg->t_r_qenvl;
        int idx=0;
        int idx2=0;
        float min_dis=360;
        float min_dis2=360;
        float dis;
        float dis2;
        if(qform9->cfg->use_two_channel){
            for(i=0;i<qform9->cfg->t_r_number;++i){
                if(t_r_qenvl[i][2] == 0){
                    if(t_r_qenvl[i][0] == -1){
                        dis = fabs(qform9->cfg->theta_range-t_r_qenvl[i][1]);
                        if(dis < min_dis){
                            min_dis = dis;
                            idx = i;
                        }
                    }else{
                        if(theta == t_r_qenvl[i][0]){
                            dis = fabs(qform9->cfg->theta_range-t_r_qenvl[i][1]);
                            if(dis < min_dis){
                                min_dis = dis;
                                idx = i;
                            }
                        }
                    }
                }else{
                    if(t_r_qenvl[i][0] == -1){
                        dis2 = fabs(qform9->cfg->theta_range-t_r_qenvl[i][1]);
                        if(dis2 < min_dis2){
                            min_dis2 = dis2;
                            idx2 = i;
                        }
                    }else{
                        if(theta == t_r_qenvl[i][0]){
                            dis2 = fabs(qform9->cfg->theta_range-t_r_qenvl[i][1]);
                            if(dis2 < min_dis2){
                                min_dis2 = dis2;
                                idx2 = i;
                            }
                        }
                    }
                }
            }
            qform9->cfg->qenvl.envelope_thresh = t_r_qenvl[idx][3];
            qform9->cfg->qenvl.min_speccrest = t_r_qenvl[idx][4];
            qform9->cfg->qenvl.right_min_thresh = t_r_qenvl[idx][5];
            qform9->cfg->qenvl2.envelope_thresh = t_r_qenvl[idx2][3];
            qform9->cfg->qenvl2.min_speccrest = t_r_qenvl[idx2][4];
            qform9->cfg->qenvl2.right_min_thresh = t_r_qenvl[idx2][5];
        }else{
            for(i=0;i<qform9->cfg->t_r_number;++i){
                if(t_r_qenvl[i][0] == -1){
                    dis = fabs(qform9->cfg->theta_range-t_r_qenvl[i][1]);
                    if(dis < min_dis){
                        min_dis = dis;
                        idx = i;
                    }
                }else{
                    if(theta == t_r_qenvl[i][0]){
                        dis = fabs(qform9->cfg->theta_range-t_r_qenvl[i][1]);
                        if(dis < min_dis){
                            min_dis = dis;
                            idx = i;
                        }
                    }
                }
            }
            qform9->cfg->qenvl.envelope_thresh = t_r_qenvl[idx][3];
            qform9->cfg->qenvl.min_speccrest = t_r_qenvl[idx][4];
            qform9->cfg->qenvl.right_min_thresh = t_r_qenvl[idx][5];
        }
    }
}

void wtk_qform9_feed2(wtk_qform9_t *qform,short **data,int len,int is_end)
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

void wtk_qform9_feed(wtk_qform9_t *qform9,short **data,int len,int is_end)
{
#ifdef DEBUG_WAV
	static wtk_wavfile_t *mic_log=NULL;

	if(!mic_log)
	{
		mic_log=wtk_wavfile_new(16000);
		wtk_wavfile_set_channel(mic_log,qform9->bf->channel);
		wtk_wavfile_open2(mic_log,"qform9");
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
    if(qform9->cfg->use_preemph)
    {
        wtk_qform9_feed2(qform9,data,len,is_end);
    }else
    {
        wtk_stft2_feed(qform9->stft2,data,len,is_end);
    }
}



wtk_qform9_t* wtk_qform9_new2(wtk_qform9_cfg_t *cfg, wtk_stft2_t *stft2)
{
    wtk_qform9_t *qform9;
    int i;

    qform9=(wtk_qform9_t *)wtk_malloc(sizeof(wtk_qform9_t));
    qform9->cfg=cfg;
    qform9->ths=NULL;
    qform9->notify=NULL;
    qform9->ths2=NULL;
    qform9->notify2=NULL;

    qform9->input=NULL;

    qform9->stft2=stft2;
    qform9->nbin=stft2->nbin;

    qform9->covm=wtk_covm_new(&(cfg->covm),qform9->nbin, stft2->cfg->channel);
    qform9->bf=wtk_bf_new(&(cfg->bf), stft2->cfg->win);

    qform9->aspec=NULL;
    qform9->naspec=NULL;
    qform9->aspec_class[0]=qform9->aspec_class[1]=NULL;
    if(cfg->use_noiseblock)
    {
        qform9->naspec=(wtk_aspec_t **)wtk_malloc(sizeof(wtk_aspec_t *)*cfg->ntheta_num);
        for(i=0; i<cfg->ntheta_num; ++i)
        {
            qform9->naspec[i]=wtk_aspec_new(&(cfg->aspec), qform9->nbin, 3);
        }
    }else if(cfg->use_two_aspecclass)
    {
        qform9->aspec_class[0]=wtk_aspec_new(&(cfg->aspec_class1), qform9->nbin, 3);        
        qform9->aspec_class[1]=wtk_aspec_new(&(cfg->aspec_class2), qform9->nbin, 3);        
    }else if(cfg->use_noiseblock2)
    {
        qform9->aspec=wtk_aspec_new(&(cfg->aspec), qform9->nbin, 3);        
        qform9->naspec=(wtk_aspec_t **)wtk_malloc(sizeof(wtk_aspec_t *)*cfg->ntheta_num);
        for(i=0; i<cfg->ntheta_num; ++i)
        {
            qform9->naspec[i]=wtk_aspec_new(&(cfg->aspec), qform9->nbin, 3);
        }
    }else
    {
        qform9->aspec=wtk_aspec_new(&(cfg->aspec), qform9->nbin, 3);        
    }

    qform9->fftclass[0]=qform9->fftclass[1]=NULL;
    if(qform9->aspec_class[0] && !qform9->aspec_class[0]->need_cov)
    {
        qform9->fftclass[0]=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *)*qform9->aspec_class[0]->cfg->channel);
        qform9->fftclass[1]=(wtk_complex_t **)wtk_malloc(sizeof(wtk_complex_t *)*qform9->aspec_class[1]->cfg->channel);
    }
    
    qform9->cov=NULL;
    qform9->covclass[0]=qform9->covclass[1]=NULL;
    wtk_queue_init(&(qform9->stft2_q));
    if((qform9->aspec && qform9->aspec->need_cov) || (qform9->naspec && qform9->naspec[0]->need_cov) 
                                                                                || (qform9->aspec_class[0] && qform9->aspec_class[0]->need_cov) )
    {
        qform9->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->bf->channel*qform9->bf->channel);
        if(cfg->lt<=0)
        {
            qform9->wint=wtk_malloc(sizeof(float));
            qform9->wint[0]=1;
        }else
        {
            qform9->wint=wtk_math_create_hanning_window(2*cfg->lt+1);
        }

        if(cfg->lf<=0)
        {
            qform9->winf=wtk_malloc(sizeof(float));
            qform9->winf[0]=1;
        }else
        {
            qform9->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }

        if(qform9->aspec_class[0])
        {
            qform9->covclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[0]->cfg->channel*qform9->aspec_class[0]->cfg->channel);
            qform9->covclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[1]->cfg->channel*qform9->aspec_class[1]->cfg->channel);
        }
    }
    qform9->inv_cov=NULL;
    qform9->invcovclass[0]=qform9->invcovclass[1]=NULL;
    qform9->tmp=NULL;
    if((qform9->aspec && qform9->aspec->need_inv_cov) || (qform9->naspec && qform9->naspec[0]->need_inv_cov))
    {
        qform9->inv_cov=(wtk_complex_t *)wtk_malloc(qform9->bf->channel*qform9->bf->channel*sizeof(wtk_complex_t));
        qform9->tmp=(wtk_dcomplex_t *)wtk_malloc(qform9->bf->channel*qform9->bf->channel*2*sizeof(wtk_dcomplex_t));
    }
    if(qform9->aspec_class[0] && qform9->aspec_class[0]->need_inv_cov)
    {
        qform9->invcovclass[0]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[0]->cfg->channel*qform9->aspec_class[0]->cfg->channel);
        qform9->invcovclass[1]=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->aspec_class[1]->cfg->channel*qform9->aspec_class[1]->cfg->channel);
        i=max(qform9->aspec_class[0]->cfg->channel,qform9->aspec_class[1]->cfg->channel);
        qform9->tmp=(wtk_dcomplex_t *)wtk_malloc(i*i*2*sizeof(wtk_dcomplex_t));
    }

    qform9->cohv=NULL;
    qform9->cohv_class[0]=qform9->cohv_class[1]=qform9->cohv_class[2]=NULL;
    if(qform9->cfg->use_two_channel){
        qform9->cohv_class[0]=(float *)wtk_malloc(sizeof(float)*qform9->bf_class[0]->nbin);
        qform9->cohv_class[1]=(float *)wtk_malloc(sizeof(float)*qform9->bf_class[1]->nbin);
    }else if(qform9->cfg->use_noise_qenvelope){
        qform9->cohv_class[0]=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
        qform9->cohv_class[1]=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
        qform9->cohv_class[2]=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
    }else{
        qform9->cohv=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);
    }

    qform9->entropy_E=NULL;
    qform9->entropy_Eb=NULL;
    qform9->ncohv_fn=NULL;
    qform9->scohv_fn=NULL;
    qform9->entropy_fn=NULL;
    if(qform9->cfg->use_cohv_cnt){
        qform9->entropy_E=(float *)wtk_malloc(sizeof(float)*qform9->nbin);
        qform9->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->stft2.win);
        if(cfg->debug){
            qform9->ncohv_fn=fopen("ncohv.dat","w");
            qform9->scohv_fn=fopen("scohv.dat","w");
            qform9->entropy_fn=fopen("entropy.dat","w");
        }
    }

    qform9->cohv_fn=NULL;
    qform9->cohv_fn_class[0]=qform9->cohv_fn_class[1]=NULL;
    if(qform9->cfg->use_two_channel){
        if(cfg->debug)
        {
            qform9->cohv_fn_class[0]=fopen("cohv1.dat","w");
            qform9->cohv_fn_class[1]=fopen("cohv2.dat","w");
        }
    }else{
        if(cfg->debug)
        {
            qform9->cohv_fn=fopen("cohv.dat","w");
        }
    }

    qform9->qmmse=NULL;
    if(cfg->use_post)
    {
        qform9->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }
    
    qform9->cohv_fn=NULL;
    if(cfg->debug)
    {
        qform9->cohv_fn=fopen("cohv.dat","w");
    }

    qform9->qenvelope=NULL;
    qform9->qenvel[0]=qform9->qenvel[1]=NULL;
    qform9->q_fring=NULL;
    qform9->q_fring_class[0]=qform9->q_fring_class[1]=qform9->q_fring_class[2]=NULL;

    if(cfg->use_simple_qenvelope){
        if(cfg->debug)
        {
            qform9->cohv_fn=fopen("cohv.dat","w");
        }
        wtk_hoard_init2(&(qform9->stft_msg_hoard),offsetof(wtk_stft2_msg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_stft2_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_stft2_msg_delete,
            qform9);
        wtk_hoard_init2(&(qform9->qenvel_msg_hoard),offsetof(wtk_qform9_envelopemsg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_envelope_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_envelope_msg_delete,
            qform9);
    }else if(cfg->use_qenvelope){
        wtk_hoard_init2(&(qform9->stft_msg_hoard),offsetof(wtk_stft2_msg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_stft2_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_stft2_msg_delete,
            qform9);

        wtk_hoard_init2(&(qform9->qenvel_msg_hoard),offsetof(wtk_qform9_envelopemsg_t,hoard_n),10,
            (wtk_new_handler_t)wtk_qform9_envelope_msg_new,
            (wtk_delete_handler2_t)wtk_qform9_envelope_msg_delete,
            qform9);
    }
    if(qform9->cfg->use_two_channel){
        if(cfg->use_simple_qenvelope){
            qform9->q_fring_class[0]=wtk_fring_new(cfg->qenvl.envelope_nf+cfg->delay_nf+1);
            qform9->q_fring_class[1]=wtk_fring_new(cfg->qenvl2.envelope_nf+cfg->delay_nf+1);
        }else if(cfg->use_qenvelope)
        {
            qform9->qenvel[0]=wtk_qenvelope_new(&(cfg->qenvl));
            qform9->qenvel[1]=wtk_qenvelope_new(&(cfg->qenvl2));
            wtk_qenvelope_set_notify(qform9->qenvel[0], qform9, (wtk_qenvelope_notify_f)wtk_qform9_on_qenvel1);
            wtk_qenvelope_set_notify(qform9->qenvel[1], qform9, (wtk_qenvelope_notify_f)wtk_qform9_on_qenvel2);
        }
    }else if(cfg->use_noise_qenvelope){
        if(cfg->use_simple_qenvelope){
            qform9->q_fring_class[0]=wtk_fring_new(cfg->qenvl.envelope_nf+cfg->delay_nf+1);
            qform9->q_fring_class[1]=wtk_fring_new(cfg->qenvl2.envelope_nf+cfg->delay_nf+1);
            qform9->q_fring_class[2]=wtk_fring_new(cfg->qenvl3.envelope_nf+cfg->delay_nf+1);
        }else{
            wtk_debug("error use noise_qenvelope");
        }
    }else{
        if(cfg->use_simple_qenvelope){
            qform9->q_fring=wtk_fring_new(cfg->qenvl.envelope_nf+cfg->delay_nf+1);
        }else if(cfg->use_qenvelope)
        {
            qform9->qenvelope=wtk_qenvelope_new(&(cfg->qenvl));
            wtk_qenvelope_set_notify(qform9->qenvelope, qform9, (wtk_qenvelope_notify_f)wtk_qform9_on_qenvelope);
        }
    }

    wtk_qform9_reset2(qform9);

    return qform9;
}

void wtk_qform9_delete2(wtk_qform9_t *qform9)
{
    int i;

	wtk_hoard_clean(&(qform9->stft_msg_hoard));

    if(qform9->cfg->use_simple_qenvelope){
        wtk_hoard_clean(&(qform9->stft_msg_hoard));
        wtk_hoard_clean(&(qform9->qenvel_msg_hoard));
    }
    if(qform9->qenvelope)
    {
	    wtk_hoard_clean(&(qform9->qenvel_msg_hoard));
        wtk_qenvelope_delete(qform9->qenvelope);
    }
    if(qform9->qenvel[0]){
        wtk_hoard_clean(&(qform9->stft_msg_hoard));
        wtk_hoard_clean(&(qform9->qenvel_msg_hoard));
        wtk_qenvelope_delete(qform9->qenvel[0]);
        wtk_qenvelope_delete(qform9->qenvel[1]);
    }
    if(qform9->q_fring){
        wtk_fring_delete(qform9->q_fring);
    }
    if(qform9->q_fring_class[0]){
        wtk_fring_delete(qform9->q_fring_class[0]);
        wtk_fring_delete(qform9->q_fring_class[1]);
    }
    if(qform9->q_fring_class[2]){
        wtk_fring_delete(qform9->q_fring_class[2]);
    }

    if(qform9->cov)
    {
        wtk_free(qform9->cov);
        wtk_free(qform9->wint);
        wtk_free(qform9->winf);
    }
    if(qform9->inv_cov)
    {
        wtk_free(qform9->inv_cov);
    }
    if(qform9->tmp)
    {
        wtk_free(qform9->tmp);
    }
    if(qform9->covclass[0])
    {
        wtk_free(qform9->covclass[0]);
        wtk_free(qform9->covclass[1]);
    }
    if(qform9->invcovclass[0])
    {
        wtk_free(qform9->invcovclass[0]);
        wtk_free(qform9->invcovclass[1]);
    }
    if(qform9->fftclass[0])
    {
        wtk_free(qform9->fftclass[0]);
        wtk_free(qform9->fftclass[1]);
    }

    if(qform9->aspec)
    {
        wtk_aspec_delete(qform9->aspec);
    }
    if(qform9->naspec)
    {
        for(i=0; i<qform9->cfg->ntheta_num; ++i)
        {
            wtk_aspec_delete(qform9->naspec[i]);
        }
        wtk_free(qform9->naspec);
    }
    if(qform9->aspec_class[0])
    {
        wtk_aspec_delete(qform9->aspec_class[0]);
        wtk_aspec_delete(qform9->aspec_class[1]);
    }

    if(qform9->cohv_fn)
    {
        fclose(qform9->cohv_fn);
    }
    if(qform9->cohv_fn_class[0]){
        fclose(qform9->cohv_fn_class[0]);
        fclose(qform9->cohv_fn_class[1]);
    }
    if(qform9->cohv_class[0]){
        wtk_free(qform9->cohv_class[0]);
        wtk_free(qform9->cohv_class[1]);
    }
    if(qform9->cohv_class[2]){
        wtk_free(qform9->cohv_class[2]);
    }

    if(qform9->cohv){
        wtk_free(qform9->cohv);
    }
    
    if(qform9->entropy_E){
        wtk_free(qform9->entropy_E);
    }
    if(qform9->entropy_Eb){
        wtk_free(qform9->entropy_Eb);
    }
    if(qform9->ncohv_fn){
        fclose(qform9->ncohv_fn);
    }
    if(qform9->scohv_fn){
        fclose(qform9->scohv_fn);
    }
    if(qform9->entropy_fn){
        fclose(qform9->entropy_fn);
    }

    if(qform9->qmmse)
    {
        wtk_qmmse_delete(qform9->qmmse);
    }

    wtk_covm_delete(qform9->covm);
    wtk_bf_delete(qform9->bf);
    wtk_free(qform9);
}

void wtk_qform9_reset2(wtk_qform9_t *qform9)
{
    int i;

    qform9->end_pos=0;
    qform9->delay_end_pos=0;
    qform9->delay_end_pos_class[0]=0;
    qform9->delay_end_pos_class[1]=0;

    if(qform9->qenvelope)
    {
        wtk_qenvelope_reset(qform9->qenvelope);
    }
    if(qform9->qenvel[0]){
        wtk_qenvelope_reset(qform9->qenvel[0]);
        wtk_qenvelope_reset(qform9->qenvel[1]);
    }
    if(qform9->q_fring){
        wtk_fring_reset(qform9->q_fring);
        qform9->q_spec=0;
        qform9->right_nf=0;
    }
    if(qform9->q_fring_class[0]){
        wtk_fring_reset(qform9->q_fring_class[0]);
        wtk_fring_reset(qform9->q_fring_class[1]);
        qform9->q_spec_class[0]=0;
        qform9->q_spec_class[1]=0;
        qform9->right_nf_class[0]=0;
        qform9->right_nf_class[1]=0;
    }
    if(qform9->q_fring_class[2]){
        wtk_fring_reset(qform9->q_fring_class[2]);
        qform9->q_spec_class[2]=0;
        qform9->right_nf_class[2]=0;
    }

    if(qform9->aspec)
    {
        wtk_aspec_reset(qform9->aspec);
    }
    if(qform9->naspec)
    {
        for(i=0; i<qform9->cfg->ntheta_num; ++i)
        {
            wtk_aspec_reset(qform9->naspec[i]);
        }
    }
    if(qform9->aspec_class[0])
    {
        wtk_aspec_reset(qform9->aspec_class[0]);
        wtk_aspec_reset(qform9->aspec_class[1]);
    }

    wtk_queue_init(&(qform9->stft2_q));

    wtk_covm_reset(qform9->covm);
    wtk_bf_reset(qform9->bf);

    qform9->nframe=0;

    qform9->theta=qform9->phi=-1;

    if(qform9->cfg->use_two_channel){
        memset(qform9->cohv_class[0],0,sizeof(float)*qform9->bf_class[0]->nbin);
        memset(qform9->cohv_class[1],0,sizeof(float)*qform9->bf_class[1]->nbin);
    }else if(qform9->cfg->use_noise_qenvelope){
        memset(qform9->cohv_class[0],0,sizeof(float)*qform9->bf->nbin);
        memset(qform9->cohv_class[1],0,sizeof(float)*qform9->bf->nbin);
        memset(qform9->cohv_class[2],0,sizeof(float)*qform9->bf->nbin);
    }else{
        memset(qform9->cohv,0,sizeof(float)*qform9->bf->nbin);
    }

    if(qform9->entropy_E){
        memset(qform9->entropy_E,0,sizeof(float)*qform9->nbin);
    }
    if(qform9->entropy_Eb){
        memset(qform9->entropy_Eb,0,sizeof(float)*qform9->cfg->stft2.win);
    }

    if(qform9->qmmse)
    {
        wtk_qmmse_reset(qform9->qmmse);
    }

    qform9->noise_debug_cnt = 0;
    qform9->noise_debug_cnt_class[0] = 0;
    qform9->noise_debug_cnt_class[1] = 0;
    qform9->delay_nf=qform9->cfg->delay_nf;
    qform9->delay_nf_class[0]=qform9->cfg->delay_nf;
    qform9->delay_nf_class[1]=qform9->cfg->delay_nf;
    qform9->delay_cnt=0;
    qform9->delay_cnt_class[0]=0;
    qform9->delay_cnt_class[1]=0;
    qform9->entropy_in_cnt = 0;
    qform9->entropy_silcnt = 0;
    qform9->entropy_sil = 1;
    qform9->ncohv_sum = 0;
    qform9->ncohv_sum_class[0] = 0;
    qform9->ncohv_sum_class[1] = 0;
    qform9->scohv_sum = 0;
    qform9->scohv_sum_class[0] = 0;
    qform9->scohv_sum_class[1] = 0;
    qform9->ncohv_cnt = 0;
    qform9->ncohv_cnt_class[0] = 0;
    qform9->ncohv_cnt_class[1] = 0;
    qform9->cohv_frame = 0;
    qform9->cohv_frame_class[0] = 0;
    qform9->cohv_frame_class[1] = 0;

	qform9->bs_scale=1.0;
	qform9->bs_last_scale=1.0;
	qform9->bs_real_scale=1.0;
	qform9->bs_max_cnt=0;

    qform9->sil_in_cnt = 0;
    qform9->sil_out_cnt = 0;
    if(qform9->qmmse){
        qform9->sil_max_range = qform9->qmmse->cfg->max_range;
        qform9->sil_noise_suppress = qform9->qmmse->cfg->noise_suppress;
    }
}

void wtk_qform9_start2(wtk_qform9_t *qform9, float theta, float phi)
{
    wtk_qform9_start(qform9, theta, phi);
}

void wtk_qform9_set_notify2(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_f2 notify)
{
    qform9->ths2=ths;
    qform9->notify2=notify;
}

void wtk_qform9_feed_smsg(wtk_qform9_t *qform9,wtk_stft2_msg_t *smsg2,int pos,int is_end)
{
    wtk_queue_t *stft2_q=&(qform9->stft2_q);
    int lt=qform9->cfg->lt;
    wtk_queue_node_t *qn;
    int i;
    wtk_stft2_msg_t *msg, *smsg;

    if(is_end)
    {
        qform9->end_pos=pos;
    }
    msg=NULL;
    if(smsg2)
    {
        msg = wtk_qform9_stft2_msg_copy(qform9, smsg2, qform9->bf->channel, qform9->nbin);
    }

    if(qform9->cov)
    {
        if(msg)
        {
            wtk_queue_push(stft2_q,&(msg->q_n));
        }
        if(stft2_q->length>=lt+1 && stft2_q->length<2*lt+1)
        {
            wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-lt-1, 0);
        }else if(stft2_q->length==2*lt+1)
        {
            wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-lt-1, (is_end && lt==0)?1: 0);
            qn=wtk_queue_pop(stft2_q);
            smsg=data_offset2(qn,wtk_stft2_msg_t,q_n);
            wtk_qform9_push_stft2_msg(qform9, smsg);
        }else if(is_end && stft2_q->length==0)
        {
            wtk_qform9_flush2(qform9, NULL, NULL, 0, 0, 1);
        }
        if(is_end)
        {
            if(stft2_q->length>0)
            {
                if(stft2_q->length<lt+1)
                {
                    for(i=0; i<stft2_q->length-1; ++i)
                    {
                        wtk_qform9_flush_aspec_lt(qform9, i, 0);
                    }
                    wtk_qform9_flush_aspec_lt(qform9, stft2_q->length-1, 1);
                }else
                {
                    for(i=0; i<lt-1; ++i)
                    {
                        wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-lt+i, 0);   
                    }
                    wtk_qform9_flush_aspec_lt(qform9,stft2_q->length-1, 1);
                }
            }
            while(qform9->stft2_q.length>0)
            {
                qn=wtk_queue_pop(&(qform9->stft2_q));
                if(!qn){break;}
                smsg=(wtk_stft2_msg_t *)data_offset(qn,wtk_stft2_msg_t,q_n);
                wtk_qform9_push_stft2_msg(qform9, smsg);
            }
        }
    }else
    {
        if(msg)
        {
            wtk_qform9_flush_aspec(qform9,msg,is_end);
            wtk_qform9_push_stft2_msg(qform9, msg);
        }else if(is_end && !msg)
        {
            wtk_qform9_flush2(qform9, NULL, NULL, 0, 0, 1);
        }
    }
}



wtk_qform9_t* wtk_qform9_new3(wtk_qform9_cfg_t *cfg, int win, int channel)
{
    wtk_qform9_t *qform9;

    qform9=(wtk_qform9_t *)wtk_malloc(sizeof(wtk_qform9_t));
    qform9->cfg=cfg;
    qform9->ths=NULL;
    qform9->notify=NULL;
    qform9->ths3=NULL;
    qform9->notify3=NULL;

    qform9->input=NULL;

    qform9->nbin=win/2+1;

    qform9->covm=wtk_covm_new(&(cfg->covm),qform9->nbin, channel);
    qform9->bf=wtk_bf_new(&(cfg->bf), win);

    qform9->aspec=NULL;
    qform9->aspec=wtk_aspec_new(&(cfg->aspec), qform9->nbin, 3);        

    qform9->cov=NULL;
    if((qform9->aspec && qform9->aspec->need_cov))
    {
        qform9->cov=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*qform9->bf->channel*qform9->bf->channel);
        if(cfg->lf<=0)
        {
            qform9->winf=wtk_malloc(sizeof(float));
            qform9->winf[0]=1;
        }else
        {
            qform9->winf=wtk_math_create_hanning_window(2*cfg->lf+1);
        }
    }
    qform9->inv_cov=NULL;
    qform9->tmp=NULL;
    if((qform9->aspec && qform9->aspec->need_inv_cov))
    {
        qform9->inv_cov=(wtk_complex_t *)wtk_malloc(qform9->bf->channel*qform9->bf->channel*sizeof(wtk_complex_t));
        qform9->tmp=(wtk_dcomplex_t *)wtk_malloc(qform9->bf->channel*qform9->bf->channel*2*sizeof(wtk_dcomplex_t));
    }

    qform9->cohv=(float *)wtk_malloc(sizeof(float)*qform9->bf->nbin);

    qform9->qmmse=NULL;
    if(cfg->use_post)
    {
        qform9->qmmse=wtk_qmmse_new(&(cfg->qmmse));
    }
    
    qform9->cohv_fn=NULL;
    if(cfg->debug)
    {
        qform9->cohv_fn=fopen("cohv.dat","w");
    }

    qform9->qenvelope=NULL;
    qform9->q_fring=NULL;
    if(cfg->use_simple_qenvelope){
        qform9->q_fring=wtk_fring_new(cfg->qenvl.envelope_nf+1);
    }

    qform9->entropy_E=NULL;
    qform9->entropy_Eb=NULL;
    qform9->ncohv_fn=NULL;
    qform9->scohv_fn=NULL;
    qform9->entropy_fn=NULL;
    if(qform9->cfg->use_cohv_cnt){
        qform9->entropy_E=(float *)wtk_malloc(sizeof(float)*qform9->nbin);
        qform9->entropy_Eb=(float *)wtk_malloc(sizeof(float)*cfg->stft2.win);
        if(cfg->debug){
            qform9->ncohv_fn=fopen("ncohv.dat","w");
            qform9->scohv_fn=fopen("scohv.dat","w");
            qform9->entropy_fn=fopen("entropy.dat","w");
        }
    }

    wtk_qform9_reset3(qform9);

    return qform9;
}

void wtk_qform9_delete3(wtk_qform9_t *qform9)
{
    if(qform9->q_fring){
        wtk_fring_delete(qform9->q_fring);
    }
    if(qform9->cov)
    {
        wtk_free(qform9->cov);
        wtk_free(qform9->winf);
    }
    if(qform9->inv_cov)
    {
        wtk_free(qform9->inv_cov);
    }
    if(qform9->tmp)
    {
        wtk_free(qform9->tmp);
    }

    if(qform9->aspec)
    {
        wtk_aspec_delete(qform9->aspec);
    }

    if(qform9->cohv_fn)
    {
        fclose(qform9->cohv_fn);
    }

    wtk_free(qform9->cohv);
    
    if(qform9->qmmse)
    {
        wtk_qmmse_delete(qform9->qmmse);
    }

    wtk_covm_delete(qform9->covm);
    wtk_bf_delete(qform9->bf);

    if(qform9->entropy_E)
    {
        wtk_free(qform9->entropy_E);
        wtk_free(qform9->entropy_Eb);
    }
    if(qform9->ncohv_fn)
    {
        fclose(qform9->ncohv_fn);
    }
    if(qform9->scohv_fn)
    {
        fclose(qform9->scohv_fn);
    }
    if(qform9->entropy_fn)
    {
        fclose(qform9->entropy_fn);
    }
    wtk_free(qform9);
}

void wtk_qform9_reset3(wtk_qform9_t *qform9)
{
    if(qform9->q_fring){
        wtk_fring_reset(qform9->q_fring);
        qform9->q_spec=0;
        qform9->right_nf=0;
    }

    if(qform9->qenvelope)
    {
        wtk_qenvelope_reset(qform9->qenvelope);
    }

    if(qform9->aspec)
    {
        wtk_aspec_reset(qform9->aspec);
    }

    wtk_covm_reset(qform9->covm);
    wtk_bf_reset(qform9->bf);

    qform9->nframe=0;

    qform9->theta=qform9->phi=-1;

    memset(qform9->cohv,0,sizeof(float)*qform9->bf->nbin);

    if(qform9->qmmse)
    {
        wtk_qmmse_reset(qform9->qmmse);
    }
    if(qform9->entropy_E){
        memset(qform9->entropy_E,0,sizeof(float)*qform9->nbin);
    }
    if(qform9->entropy_Eb){
        memset(qform9->entropy_Eb,0,sizeof(float)*(qform9->nbin-1)*2);
    }
    qform9->noise_debug_cnt = 0;
    qform9->entropy_silcnt = 0;
    qform9->entropy_sil = 1;
    qform9->sil_in_cnt = 0;
    qform9->sil_out_cnt = 0;
    if(qform9->qmmse){
        qform9->sil_max_range = qform9->qmmse->cfg->max_range;
        qform9->sil_noise_suppress = qform9->qmmse->cfg->noise_suppress;
    }
}

void wtk_qform9_start3(wtk_qform9_t *qform9, float theta, float phi)
{
    wtk_qform9_start(qform9, theta, phi);
}

void wtk_qform9_set_notify3(wtk_qform9_t *qform9,void *ths,wtk_qform9_notify_f3 notify)
{
    qform9->ths3=ths;
    qform9->notify3=notify;
}


void wtk_qform9_flush2_fft(wtk_qform9_t *qform9,wtk_complex_t **fft,float *cohv, int chn, int wav_debug, int is_end)
{
    wtk_complex_t *bf_out=NULL;
    // int nbin=qform9->bf->nbin;
    bf_out=wtk_bf_output_fft2(qform9->bf,fft,cohv);
    if(qform9->qmmse && wav_debug != -1)
    {
        wtk_qmmse_feed_cohv(qform9->qmmse,bf_out,cohv);      
    }
    if(wav_debug==1){
        memset(bf_out, 0, sizeof(wtk_complex_t)*qform9->bf->nbin);
    }
    if(qform9->notify3)
    {
        qform9->notify3(qform9->ths3, bf_out, is_end);
    }
}

void wtk_qform9_flush_fft(wtk_qform9_t *qform9,wtk_complex_t **fft, float *cohv, int chn, int wav_debug, int is_end)
{
    int k;
    int nbin=qform9->bf->nbin;
    int i, channel=qform9->bf->channel;
    int b;
    wtk_covm_t *covm=qform9->covm;

    for(k=1; k<nbin-1; ++k)
    {
        b=0;
        if(cohv[k]<0.0)
        {
            b=wtk_covm_feed_fft2(covm, fft, k, 1);
            if(b==1)
            {
                wtk_bf_update_ncov(qform9->bf, covm->ncov, k);
            }
        }else
        {
            if(covm->scov)
            {
                b=wtk_covm_feed_fft2(covm, fft, k, 0);
                if(b==1)
                {
                    wtk_bf_update_scov(qform9->bf, covm->scov, k);
                }
            }
        }
        if(covm->ncnt_sum[k]>5 && (covm->scnt_sum==NULL ||  covm->scnt_sum[k]>5) && b==1)
        {
            wtk_bf_update_w(qform9->bf, k);
        }

        if(qform9->cfg->debug)
        {
            if(cohv[k]<0)
            {
                for(i=0;i<channel;++i)
                {
                    qform9->bf->w[k][i].a=0;
                    qform9->bf->w[k][i].b=0;
                }
            }else
            {
                for(i=0;i<channel;++i)
                {
                    qform9->bf->w[k][i].a=0;
                    qform9->bf->w[k][i].b=0;
                    if(i==0)
                    {
                        qform9->bf->w[k][i].a=1;
                    }
                }
            }
        }else if(wav_debug==-1){
            if(cohv[k]>0){
                for(i=0;i<channel;++i)
                {
                    qform9->bf->w[k][i].a=0;
                    qform9->bf->w[k][i].b=0;
                    if(i==0)
                    {
                        qform9->bf->w[k][i].a=1;
                    }
                }
            }
        }
    }
    wtk_qform9_flush2_fft(qform9,fft,cohv,0,wav_debug,is_end);
}

void wtk_qform9_flush_aspec_lt_fft(wtk_qform9_t *qform9, wtk_complex_t **fft, int is_end)
{
    int lf=qform9->cfg->lf;
    int i,j,k,k2,ff;
    int nbin=qform9->nbin;
    int channel=qform9->bf->channel;
    wtk_complex_t *cov=qform9->cov;
    wtk_complex_t *fft1,*fft2,*a,*b;
    float *winf=qform9->winf;
    float wintf,winsum;
    wtk_complex_t *inv_cov=qform9->inv_cov;
    wtk_dcomplex_t *tmp=qform9->tmp;
    float cov_travg;
    int ret;
    float *cohv=qform9->cohv;
    float spec_k[3]={0}, specsum;
    int specsum_ns=qform9->cfg->specsum_ns;
    int specsum_ne=qform9->cfg->specsum_ne;
    float max_spec;
    float qenvel_alpha = qform9->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;
    float entropy=0;
    float entropy_thresh = qform9->cfg->entropy_thresh;
    int entropy_cnt = qform9->cfg->entropy_cnt;
    float cohv_sum=0;
    float ncohv_alpha=qform9->cfg->ncohv_alpha;
    float ncohv_alpha_1=1.0-ncohv_alpha;
    float scohv_alpha=qform9->cfg->scohv_alpha;
    float scohv_alpha_1=1.0-scohv_alpha;

    ++qform9->nframe;

    specsum=0;

    if(qform9->cfg->use_qenvelope && qform9->cfg->use_cohv_cnt && entropy_thresh>0){
        entropy = wtk_qform9_entropy(qform9, fft);
        if(entropy_thresh>0){
            if(entropy<entropy_thresh){
                ++qform9->entropy_in_cnt;
            }else{
                qform9->entropy_in_cnt = 0;
            }
            if(qform9->entropy_in_cnt>=qform9->cfg->entropy_in_cnt){
                qform9->entropy_sil = 0;
                qform9->entropy_silcnt = entropy_cnt;
            }else if(qform9->entropy_sil==0){
                qform9->entropy_silcnt -= 1;
                if(qform9->entropy_silcnt<=0){
                    qform9->entropy_sil = 1;
                }
            }
        }
    }

    for(k=1;k<nbin-1;++k)
    {
        memset(cov,0,sizeof(wtk_complex_t)*channel*channel);
        winsum=0;
        for(k2=max(1,k-lf),ff=k2-(k-lf);k2<min(nbin-1,k+lf+1);++k2,++ff)
        {
            wintf=winf[ff];
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

       if(qform9->aspec)
        {
            cov_travg=0;
            if(qform9->aspec->need_cov_travg) 
            {
                for(i=0;i<channel;++i)
                {
                    cov_travg+=cov[i*channel+i].a;
                }
                cov_travg/=channel;
            }

            wtk_qform9_update_aspec2(qform9, qform9->aspec, cov, inv_cov, cov_travg, k, spec_k, cohv+k);

            if(k>=specsum_ns && k<=specsum_ne  && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
                cohv_sum += 1;
            }
        }
    }
    if(qform9->cfg->use_simple_qenvelope){
        if(qform9->entropy_sil==1){
            if(entropy_thresh>0){
                ++qform9->cohv_frame;
                if(qform9->cohv_frame<=qform9->cfg->cohv_init_frame){
                    qform9->ncohv_sum += cohv_sum*1.0/qform9->cfg->cohv_init_frame;
                }else{
                    qform9->ncohv_sum = qform9->ncohv_sum * ncohv_alpha + cohv_sum * ncohv_alpha_1;
                }
            }
        }else{
            if(specsum<qform9->cfg->qenvl.right_min_thresh){
                // qform9->ncohv_sum = qform9->ncohv_sum * ncohv_alpha + ncohv_sum * ncohv_alpha_1;
            }
        }
        qform9->scohv_sum = qform9->scohv_sum * scohv_alpha + cohv_sum * scohv_alpha_1;
    }
    if(qform9->ncohv_fn){
        fprintf(qform9->ncohv_fn,"%.0f %f\n",qform9->nframe,qform9->ncohv_sum);
    }
    if(qform9->scohv_fn){
        fprintf(qform9->scohv_fn,"%.0f %f\n",qform9->nframe,qform9->scohv_sum);
    }
    if(qform9->entropy_fn){
        fprintf(qform9->entropy_fn,"%.0f %f\n",qform9->nframe,entropy);
    }
    // printf("entropy_fn, qform9->ncohv_sum);
    // printf("%f\n", qform9->scohv_sum);
    // printf("%f\n", ncohv_sum);
    // printf("%d\n", qform9->entropy_sil);
    // printf("%f\n", entropy);
    if(qform9->cfg->use_specsum_bl && specsum<=qform9->cfg->specsum_bl)
    {
        for(k=0;k<nbin;++k)
        {
            cohv[k]=-1;
        }
    }
    if(qform9->cfg->use_simple_qenvelope){
        wtk_fring_push2(qform9->q_fring, specsum);
        min_speccrest = qform9->cfg->qenvl.min_speccrest;
        right_nf = qform9->cfg->qenvl.right_nf;
        envelope_thresh = qform9->cfg->qenvl.envelope_thresh;
        right_min_thresh = qform9->cfg->qenvl.right_min_thresh;
        if(qform9->q_fring->used == qform9->q_fring->nslot - 1){
            max_spec = wtk_fring_max(qform9->q_fring);
            if(max_spec < qform9->q_spec){
                max_spec = qenvel_alpha * qform9->q_spec + qenvel_alpha_1 * max_spec; 
            }
            if(qform9->cohv_fn)
            {
                fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,max_spec);
            }
            // printf("%f\n", max_spec);
            qform9->q_spec = max_spec;
            if(max_spec > min_speccrest){
                qform9->right_nf = right_nf;
            }else if(max_spec > envelope_thresh){
                // --qform9->right_nf;
            }else if(max_spec > right_min_thresh){
                --qform9->right_nf;
            }else{
                qform9->right_nf = 0;
            }
            if(qform9->cfg->use_cohv_cnt){
                if(qform9->cohv_frame>qform9->cfg->cohv_init_frame){
                    if(qform9->ncohv_sum<qform9->scohv_sum*qform9->cfg->nscohv_scale && qform9->entropy_sil==0 && max_spec > envelope_thresh){
                        qform9->right_nf = right_nf;
                    }
                    if(qform9->ncohv_sum>qform9->scohv_sum*qform9->cfg->nscohv_scale2){
                        qform9->ncohv_cnt=qform9->right_nf;
                    }else if(qform9->ncohv_sum>qform9->scohv_sum*qform9->cfg->nscohv_scale3){
                    }else if(qform9->ncohv_sum>qform9->scohv_sum){
                        --qform9->ncohv_cnt;
                    }else{
                        qform9->ncohv_cnt=0;

                    }
                    if(qform9->ncohv_cnt>0){
                        qform9->right_nf=0;
                    }
                }
            }
            if(qform9->right_nf <= 0){
                if(qform9->cfg->use_nqenvelope){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=-1;
                    }
                    --qform9->noise_debug_cnt;
                }
            }else{
                if(qform9->cfg->use_sqenvelope){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=1;
                    }
                }
                qform9->noise_debug_cnt=qform9->cfg->noise_debug_cnt;
            }
            if(qform9->cfg->use_qmmse_param){
                if(qform9->right_nf <= 0){
                    ++qform9->sil_in_cnt;
                }else{
                    qform9->sil_in_cnt = 0;
                }
                if(qform9->sil_in_cnt>qform9->cfg->sil_in_cnt){
                    qform9->sil_out_cnt = qform9->cfg->sil_out_cnt;
                }else if(qform9->sil_out_cnt>0){
                    --qform9->sil_out_cnt;
                }
                if(qform9->sil_out_cnt>0){
                    qform9->qmmse->cfg->max_range = qform9->cfg->sil_max_range;
                    qform9->qmmse->cfg->noise_suppress = qform9->cfg->sil_noise_suppress;
                }else{
                    qform9->qmmse->cfg->max_range = qform9->sil_max_range;
                    qform9->qmmse->cfg->noise_suppress = qform9->sil_noise_suppress;
                }
            }

            if(qform9->cfg->use_noise_debug){
                if(qform9->noise_debug_cnt<=0){
                    wtk_qform9_flush_fft(qform9, fft, cohv, 0, 1, is_end);
                }else{
                    wtk_qform9_flush_fft(qform9, fft, cohv, 0, 0, is_end);
                }
            }else if(qform9->cfg->use_sound_debug){
                if(qform9->noise_debug_cnt<=0){
                    wtk_qform9_flush_fft(qform9, fft, cohv, 0, 0, is_end);
                }else{
                    wtk_qform9_flush_fft(qform9, fft, cohv, 0, -1, is_end);
                }
            }else{
                wtk_qform9_flush_fft(qform9, fft, cohv, 0, 0, is_end);
            }
        }
    }else
    {
        if(qform9->cohv_fn)
        {
            fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,specsum);
        }
        wtk_qform9_flush_fft(qform9, fft, cohv, 0, 0, is_end);
    }
}

void wtk_qform9_flush_aspec_fft(wtk_qform9_t *qform9, wtk_complex_t **fft, int is_end)
{
    int k,i;
    int nbin=qform9->nbin;
    int channel=qform9->bf->channel;
    wtk_complex_t *fft2;
    float fftabs2;
    float spec_k[3]={0}, specsum;
    float *cohv=qform9->cohv;
    int specsum_ns=qform9->cfg->specsum_ns;
    int specsum_ne=qform9->cfg->specsum_ne;
    float max_spec;
    float qenvel_alpha = qform9->cfg->qenvel_alpha;
    float qenvel_alpha_1 = 1.0 - qenvel_alpha;
    float min_speccrest;
    int right_nf;
    float envelope_thresh;
    float right_min_thresh;

    ++qform9->nframe;

    specsum=0;
    for(k=1; k<nbin-1; ++k)
    {
        if(qform9->aspec)
        {
            fftabs2=0;
            fft2=fft[k];
            for(i=0; i<channel; ++i,++fft2)
            {
                fftabs2+=fft2->a*fft2->a+fft2->b*fft2->b;
            }

            wtk_qform9_update_aspec(qform9,qform9->aspec,fft,fftabs2,k,spec_k,cohv+k);

            if(k>=specsum_ns && k<=specsum_ne && spec_k[0]>spec_k[1] && spec_k[0]>spec_k[2])
            {
                specsum+=spec_k[0]*2-spec_k[1]-spec_k[2];
            }
        }
    }
    if(qform9->cfg->use_specsum_bl && specsum<=qform9->cfg->specsum_bl)
    {
        for(k=0;k<nbin;++k)
        {
            cohv[k]=-1;
        }
    }
    if(qform9->cfg->use_simple_qenvelope){
        wtk_fring_push2(qform9->q_fring, specsum);
        min_speccrest = qform9->cfg->qenvl.min_speccrest;
        right_nf = qform9->cfg->qenvl.right_nf;
        envelope_thresh = qform9->cfg->qenvl.envelope_thresh;
        right_min_thresh = qform9->cfg->qenvl.right_min_thresh;
        if(qform9->q_fring->used == qform9->q_fring->nslot - 1){
            max_spec = wtk_fring_max(qform9->q_fring);
            if(max_spec < qform9->q_spec){
                max_spec = qenvel_alpha * qform9->q_spec + qenvel_alpha_1 * max_spec; 
            }
            if(qform9->cohv_fn)
            {
                fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,max_spec);
            }
            // printf("%f\n", max_spec);
            qform9->q_spec = max_spec;
            if(max_spec > min_speccrest){
                qform9->right_nf = right_nf;
            }else if(max_spec > envelope_thresh){
                // --qform9->right_nf;
            }else if(max_spec > right_min_thresh){
                --qform9->right_nf;
            }else{
                qform9->right_nf = 0;
            }
            if(qform9->right_nf <= 0){
                if(qform9->cfg->use_nqenvelope){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=-1;
                    }
                }
            }else{
                if(qform9->cfg->use_sqenvelope){
                    for(k=1; k<nbin-1; ++k)
                    {
                        cohv[k]=1;
                    }
                }
            }
        }
        wtk_qform9_flush_fft(qform9, fft, cohv, 0, 0, is_end);
    }else
    {
        if(qform9->cohv_fn)
        {
            fprintf(qform9->cohv_fn,"%.0f %f\n",qform9->nframe,specsum);
        }
        wtk_qform9_flush_fft(qform9, fft, cohv, 0, 0, is_end);
    }
}

void wtk_qform9_feed_fft(wtk_qform9_t *qform9,wtk_complex_t **fft,int is_end)
{
    if(qform9->cov)
    {
        if(is_end)
        {
            wtk_qform9_flush2_fft(qform9, NULL, NULL, 0, 0, 1);
        }else{
            wtk_qform9_flush_aspec_lt_fft(qform9, fft, is_end);
        }
    }else
    {
        if(is_end)
        {
            wtk_qform9_flush2_fft(qform9, NULL, NULL, 0, 0, 1);
        }else{
            wtk_qform9_flush_aspec_fft(qform9, fft, is_end);
        }
    }
}
