#include "wtk_cmask_bfse_cfg.h"

int wtk_cmask_bfse_cfg_init(wtk_cmask_bfse_cfg_t *cfg) {

	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;
	cfg->nbfchannel=0;
	cfg->out_channels=0;
	cfg->nmic=0;
	cfg->mic_pos=NULL;
	cfg->sv=343;
	cfg->feat_len=20;
	cfg->theta=NULL;
	cfg->ntheta=2;
    cfg->wins=1024;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->sep));
    qtk_onnxruntime_cfg_init(&(cfg->bfse));
#endif
	cfg->use_qmmse=0;
	wtk_qmmse_cfg_init(&(cfg->qmmse));
	wtk_qmmse_cfg_init(&(cfg->qmmse2));
	cfg->use_eq=0;
	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_bf=0;
	wtk_covm_cfg_init(&(cfg->covm));
	wtk_bf_cfg_init(&(cfg->bf));
	wtk_aspec_cfg_init(&(cfg->aspec));
	cfg->bfmu=1;
	cfg->bfmu2=1;
	cfg->bf_clip_s=0;
	cfg->bf_clip_e=8000;
	cfg->use_cnon=0;
	cfg->sym=1e-2;
	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;
	cfg->entropy_thresh=-1;
	cfg->entropy_in_cnt=2;
	cfg->entropy_cnt=20;

	cfg->clip_s=0;
	cfg->clip_e=8000;
	cfg->de_clip_s = 0;
	cfg->de_clip_e = 8000;
	cfg->de_thresh = 1e4;
	cfg->de_alpha = 1.0;
	cfg->de_pow_ratio = 2.0;
	cfg->de_mul_ratio = 2.0;

	cfg->use_freq_preemph=0;
	cfg->pre_pow_ratio=2.0;
	cfg->pre_mul_ratio=2.0;
	cfg->pre_clip_s=4000;
	cfg->pre_clip_e=8000;
	cfg->entropy_ratio=4.0;
	cfg->entropy_min_scale=0.05;

	cfg->max_bs_out=32000;

	cfg->use_onnx=1;
	cfg->use_trick=0;
	cfg->use_norm=1;

    cfg->specsum_fs=0;
    cfg->specsum_fe=8000;

	cfg->theta_range=15;
	cfg->q_nf=5;
	cfg->right_nf=14;
	cfg->min_speccrest=500;
	cfg->envelope_thresh=200;
	cfg->right_min_thresh=10;
	cfg->q_alpha=0.8;
	cfg->use_sqenvelope=0;
	cfg->use_line=0;
    return 0;
}

int wtk_cmask_bfse_cfg_clean(wtk_cmask_bfse_cfg_t *cfg) {
	int i;
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
	if(cfg->mic_pos)
	{
		for(i=0;i<cfg->nmic;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
	if(cfg->theta){
		wtk_free(cfg->theta);
	}
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_clean(&(cfg->sep));
    qtk_onnxruntime_cfg_clean(&(cfg->bfse));
#endif
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_qmmse_cfg_clean(&(cfg->qmmse2));
	wtk_covm_cfg_clean(&(cfg->covm));
    wtk_bf_cfg_clean(&cfg->bf);
	wtk_equalizer_cfg_clean(&(cfg->eq));
    wtk_aspec_cfg_clean(&(cfg->aspec));

    return 0;
}

int wtk_cmask_bfse_cfg_update(wtk_cmask_bfse_cfg_t *cfg) {
	int ret;

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse2));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	cfg->out_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
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
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ne=min(cfg->wins/2-1, cfg->specsum_ne);

#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update(&(cfg->sep));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
		ret = qtk_onnxruntime_cfg_update(&(cfg->bfse));
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif

	ret=0;
end:
	return ret;
}

int wtk_cmask_bfse_cfg_update2(wtk_cmask_bfse_cfg_t *cfg, wtk_source_loader_t *sl) {
	int ret;

	ret=wtk_qmmse_cfg_update2(&(cfg->qmmse), sl->hook);
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update2(&(cfg->qmmse2), sl->hook);
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update2(&(cfg->covm), sl->hook);
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	cfg->out_channels = cfg->nmicchannel * (cfg->nmicchannel - 1) / 2;
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
    cfg->specsum_ns=floor(cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ns=max(1, cfg->specsum_ns);
    cfg->specsum_ne=floor(cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_ne=min(cfg->wins/2-1, cfg->specsum_ne);

#ifdef ONNX_DEC
	if(cfg->use_onnx){
		ret = qtk_onnxruntime_cfg_update2(&(cfg->sep), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
		ret = qtk_onnxruntime_cfg_update2(&(cfg->bfse), sl->hook);
		if (ret != 0) {
			wtk_debug("update onnx failed\n");
			goto end;
		}
	}
#endif
	ret=0;
end:
	return ret;
}

int wtk_cmask_bfse_cfg_update_local(wtk_cmask_bfse_cfg_t *cfg, wtk_local_cfg_t *m) {
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sv,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,feat_len,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbfchannel,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_onnx,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cnon,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bf,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bf_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sym,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_in_cnt,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,entropy_cnt,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_pow_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_mul_ratio,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_freq_preemph,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pre_pow_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pre_mul_ratio,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pre_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pre_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_ratio,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,entropy_min_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,max_bs_out,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_trick,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_norm,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,theta_range,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fs,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,specsum_fe,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,q_nf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_nf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,min_speccrest,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,envelope_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,right_min_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,q_alpha,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sqenvelope,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_aspec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

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
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"qmmse2");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse2),lc);
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
	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
		if(ret!=0){goto end;}
	}
    lc=wtk_local_cfg_find_lc_s(m,"aspec");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"mic");
	if(lc)
	{
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->mic_pos=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nmic=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[cfg->nmic]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[cfg->nmic][i]=wtk_str_atof(v->data,v->len);
				// wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
			++cfg->nmic;
		}
		if(cfg->nmic!=cfg->nmicchannel)
		{
			wtk_debug("error: nmic=%d!=nmicchannel=%d\n",cfg->nmic,cfg->nmicchannel);
			exit(1);
		}
	}
    a = wtk_local_cfg_find_array_s(m, "theta");
    if (a) {
		cfg->ntheta = a->nslot;
        cfg->theta = wtk_malloc(sizeof(float) * a->nslot);
        for (i = 0; i < a->nslot; i++) {
            v = cast(wtk_string_t **, a->slot)[i];
            cfg->theta[i] = wtk_str_atof(v->data, v->len);
			// wtk_debug("theta[%d]=%f\n", i, cfg->theta[i]);
        }
    }
#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "sep");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->sep), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
	lc = wtk_local_cfg_find_lc_s(m, "bfse");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->bfse), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
    ret = 0;
end:
    return ret;
}

wtk_cmask_bfse_cfg_t* wtk_cmask_bfse_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_cmask_bfse_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_cmask_bfse_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_cmask_bfse_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_cmask_bfse_cfg_delete(wtk_cmask_bfse_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_cmask_bfse_cfg_t* wtk_cmask_bfse_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_cmask_bfse_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_cmask_bfse_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_cmask_bfse_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_cmask_bfse_cfg_delete_bin(wtk_cmask_bfse_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
