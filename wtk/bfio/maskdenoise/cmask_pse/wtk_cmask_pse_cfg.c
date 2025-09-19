#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse_cfg.h"

int wtk_cmask_pse_cfg_init(wtk_cmask_pse_cfg_t *cfg) {

	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;
	cfg->nbfchannel=0;

    cfg->wins=1024;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;
	cfg->sv_thresh = 0.33;
	cfg->sv_thresh_mid = 0.25;
	cfg->sv_thresh_low = 0.2;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->emb));
    qtk_onnxruntime_cfg_init(&(cfg->pse));
#endif
	cfg->num_frame = 1;
	wtk_fbank_cfg_init(&(cfg->fbank));
	cfg->use_rls3=1;
	wtk_rls3_cfg_init(&(cfg->echo_rls3));
	cfg->use_qmmse=0;
	wtk_qmmse_cfg_init(&(cfg->qmmse));
	cfg->use_eq=0;
	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_bf=0;
	wtk_covm_cfg_init(&(cfg->covm));
	wtk_covm_cfg_init(&(cfg->echo_covm));
	wtk_bf_cfg_init(&(cfg->bf));
	wtk_cmask_sv_cfg_init(&(cfg->sv));
	cfg->bfmu=1;
	cfg->bfmu2=1;
	cfg->echo_bfmu=1;
	cfg->echo_bfmu2=1;
	cfg->bf_clip_s=0;
	cfg->bf_clip_e=8000;
	cfg->use_cnon=0;
	cfg->sym=1e-2;
	cfg->use_echo_bf=0;
	cfg->use_echocovm=1;
	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;
	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;
	cfg->entropy_thresh=-1;
	cfg->entropy_sp_thresh=-1;
	cfg->entropy_in_cnt=2;
	cfg->entropy_cnt=20;

	cfg->raw_alpha1=1.0;
	cfg->raw_alpha2=1.0;
	cfg->raw_alpha3=1.0;
	cfg->raw_alpha4=1.0;
	cfg->pow_scale=0.25;
	cfg->alpha1 = 1.0;
	cfg->alpha2 = 1.0;
	cfg->alpha3 = 1.0;
	cfg->alpha4 = 1.0;
	cfg->scale1=1.0;
	cfg->scale2=1.0;
	cfg->scale3=1.0;
	cfg->scale4=1.0;
	cfg->init_low_freq=0;
	cfg->init_high_freq=0;

	cfg->clip_s=0;
	cfg->clip_e=8000;
	cfg->de_clip_s = 0;
	cfg->de_clip_e = 8000;
	cfg->de_thresh = 1e4;
	cfg->de_alpha = 1.0;
	cfg->de_pow_ratio = 2.0;
	cfg->de_mul_ratio = 2.0;

	cfg->eng_scale = 1.0;
	cfg->eng1_thresh = 10.0;
	cfg->eng1_thresh2 = 10.0;
	cfg->eng2_thresh = 2000;
	cfg->eng2_thresh2 = 2000;
	cfg->eng_freq = 200;
	cfg->eng_cnt = 80;

	cfg->sp_max_smooth_gain=-1;
	cfg->sp_min_smooth_gain=-1;
	cfg->mic_min_smooth_gain=-1;
	cfg->mic_max_smooth_gain=-1;

	cfg->use_freq_preemph=0;
	cfg->pre_pow_ratio=2.0;
	cfg->pre_mul_ratio=2.0;
	cfg->pre_clip_s=4000;
	cfg->pre_clip_e=8000;
	cfg->entropy_ratio=4.0;
	cfg->entropy_min_scale=0.05;

	cfg->max_bs_out=32000;

    cfg->emb0_len = 0;
    cfg->emb1_len = 0;
    cfg->emb2_len = 0;
    cfg->emb3_len = 0;
    cfg->gamma_len = 0;
    cfg->beta_len = 0;
	cfg->gamma1_len = 0;
	cfg->beta1_len = 0;
	cfg->gamma2_len = 0;
	cfg->beta2_len = 0;
	cfg->emb_mask_len = 0;
	cfg->emb_mask_type = 0;

	cfg->use_onnx=1;
	cfg->use_ccm=0;
	cfg->use_aec_model=0;
	cfg->use_trick=0;
	cfg->use_sv_check = 0;
    return 0;
}

int wtk_cmask_pse_cfg_clean(wtk_cmask_pse_cfg_t *cfg) {
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->emb));
    qtk_onnxruntime_cfg_clean(&(cfg->pse));
#endif
	wtk_fbank_cfg_clean(&(cfg->fbank));
	wtk_rls3_cfg_clean(&(cfg->echo_rls3));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_covm_cfg_clean(&(cfg->covm));
	wtk_covm_cfg_clean(&(cfg->echo_covm));
    wtk_bf_cfg_clean(&cfg->bf);
	wtk_equalizer_cfg_clean(&(cfg->eq));
	wtk_cmask_sv_cfg_clean(&(cfg->sv));

    return 0;
}

int wtk_cmask_pse_cfg_update(wtk_cmask_pse_cfg_t *cfg) {
	int ret;

	ret=wtk_fbank_cfg_update(&(cfg->fbank));
	if(ret!=0){goto end;}
	if(cfg->use_rls3){
		cfg->echo_rls3.channel=1;
		cfg->echo_rls3.N=cfg->nspchannel;
		ret=wtk_rls3_cfg_update(&(cfg->echo_rls3));
		if(ret!=0){goto end;}
	}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->echo_covm));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	ret=wtk_cmask_sv_cfg_update(&(cfg->sv));
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
	}
	cfg->bf.nmic=cfg->nbfchannel;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_s=(cfg->bf_clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_e=(cfg->bf_clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->de_clip_s = (cfg->de_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->de_clip_e = (cfg->de_clip_e * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_s = (cfg->pre_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_e = (cfg->pre_clip_e * 1.0 * cfg->wins) / cfg->rate;

#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update(&(cfg->emb));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
		ret = qtk_onnxruntime_cfg_update(&(cfg->pse));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
	if(cfg->emb_mask_type==0){
		cfg->emb_mask_len = cfg->emb_mask_len * cfg->num_frame * cfg->num_frame;
	}else if(cfg->emb_mask_type==1){
		cfg->emb_mask_len = (cfg->emb_mask_len+1) * cfg->num_frame * cfg->num_frame;
	}else if(cfg->emb_mask_type==2){
		cfg->emb_mask_len = cfg->emb_mask_len * cfg->num_frame;
	}else if(cfg->emb_mask_type==3){
		cfg->emb_mask_len = cfg->emb_mask_len * cfg->num_frame * cfg->num_frame;
	}
	wtk_fbank_cfg_update(&(cfg->fbank));
	ret=0;
end:
	return ret;
}

int wtk_cmask_pse_cfg_update2(wtk_cmask_pse_cfg_t *cfg, wtk_source_loader_t *sl) {
	int ret;

	ret=wtk_fbank_cfg_update2(&(cfg->fbank), sl->hook);
	if(ret!=0){goto end;}
	if(cfg->use_rls3){
		cfg->echo_rls3.channel=1;
		cfg->echo_rls3.N=cfg->nspchannel;
		ret=wtk_rls3_cfg_update(&(cfg->echo_rls3));
		if(ret!=0){goto end;}
	}
	ret=wtk_qmmse_cfg_update2(&(cfg->qmmse), sl->hook);
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update2(&(cfg->covm), sl->hook);
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update2(&(cfg->echo_covm), sl->hook);
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	ret=wtk_cmask_sv_cfg_update2(&(cfg->sv),sl);
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
	}
	cfg->bf.nmic=cfg->nbfchannel;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_s=(cfg->bf_clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->bf_clip_e=(cfg->bf_clip_e*1.0*cfg->wins)/cfg->rate;
	cfg->de_clip_s = (cfg->de_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->de_clip_e = (cfg->de_clip_e * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_s = (cfg->pre_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->pre_clip_e = (cfg->pre_clip_e * 1.0 * cfg->wins) / cfg->rate;

#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update2(&(cfg->emb), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
		ret = qtk_onnxruntime_cfg_update2(&(cfg->pse), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
	if(cfg->emb_mask_type==0){
		cfg->emb_mask_len = cfg->emb_mask_len * cfg->num_frame * cfg->num_frame;
	}else if(cfg->emb_mask_type==1){
		cfg->emb_mask_len = (cfg->emb_mask_len+1) * cfg->num_frame * cfg->num_frame;
	}else if(cfg->emb_mask_type==2){
		cfg->emb_mask_len = cfg->emb_mask_len * cfg->num_frame;
	}else if(cfg->emb_mask_type==3){
		cfg->emb_mask_len = cfg->emb_mask_len * cfg->num_frame * cfg->num_frame;
	}
	wtk_fbank_cfg_update2(&(cfg->fbank), sl->hook);
	ret=0;
end:
	return ret;
}

int wtk_cmask_pse_cfg_update_local(wtk_cmask_pse_cfg_t *cfg, wtk_local_cfg_t *m) {
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_onnx,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ccm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rls3,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cnon,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bf,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,emb0_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,emb1_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,emb2_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,emb3_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,gamma_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,beta_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,gamma1_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,beta1_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,gamma2_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,beta2_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,emb_mask_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,emb_mask_type,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sv_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sv_thresh_low,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_bfmu,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_bfmu2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sym,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_echo_bf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_echocovm,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_sp_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_in_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha3,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,raw_alpha4,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pow_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha3,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,alpha4,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale3,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,scale4,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,init_low_freq,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,init_high_freq,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_pow_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_mul_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng1_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng2_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng2_thresh2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,eng_freq,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,eng_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,sp_max_smooth_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sp_min_smooth_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mic_min_smooth_gain,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mic_max_smooth_gain,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_freq_preemph,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pre_pow_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pre_mul_ratio,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pre_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pre_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_min_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_bs_out,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_aec_model,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_trick,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sv_check,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,num_frame,v);

	a=wtk_local_cfg_find_array_s(lc,"mic_channel");
	if(a)
	{
		cfg->mic_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	a=wtk_local_cfg_find_array_s(lc,"sp_channel");
	if(a)
	{
		cfg->sp_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nspchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->sp_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_rls3");
	if(lc)
	{
		ret=wtk_rls3_cfg_update_local(&(cfg->echo_rls3),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"eq");
	if(lc)
	{
		ret=wtk_equalizer_cfg_update_local(&(cfg->eq),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"covm");
	if(lc)
	{
		ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_covm");
	if(lc)
	{
		ret=wtk_covm_cfg_update_local(&(cfg->echo_covm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
		if(ret!=0){goto end;}
	}
#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "emb");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->emb), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
	lc = wtk_local_cfg_find_lc_s(m, "pse");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->pse), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
	lc=wtk_local_cfg_find_lc_s(m,"fbank");
	if(lc)
	{
		ret = wtk_fbank_cfg_update_local(&(cfg->fbank),lc);
		if (ret != 0) {
			wtk_debug("update local fbank failed\n");
			goto end;
		}
	}

	lc=wtk_local_cfg_find_lc_s(m,"sv");
	if(lc)
	{
		ret = wtk_cmask_sv_cfg_update_local(&(cfg->sv),lc);
		if (ret != 0) {
			wtk_debug("update local sv failed\n");
			goto end;
		}
	}
    ret = 0;
end:
    return ret;
}

wtk_cmask_pse_cfg_t* wtk_cmask_pse_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_cmask_pse_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_cmask_pse_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_cmask_pse_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_cmask_pse_cfg_delete(wtk_cmask_pse_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_cmask_pse_cfg_t* wtk_cmask_pse_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_cmask_pse_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_cmask_pse_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_cmask_pse_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_cmask_pse_cfg_delete_bin(wtk_cmask_pse_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
