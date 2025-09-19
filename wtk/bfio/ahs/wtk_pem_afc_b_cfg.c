#include "wtk_pem_afc_b_cfg.h"

int wtk_pem_afc_b_cfg_init(wtk_pem_afc_b_cfg_t *cfg)
{
    cfg->channel = 0;
    cfg->nmicchannel = 1;
    cfg->nspchannel = 1;
    cfg->mic_channel = NULL;
    cfg->sp_channel = NULL;

    cfg->rate = 16000;
    cfg->wins = 256;
    cfg->hop_size = 128;
    cfg->DAC_delay = 96;
    cfg->N_afc = 512;
    cfg->N_ar = 20;
    cfg->mu1 = 0.1;
    cfg->mu2 = 0.8;
    cfg->delta = 1e-6;

    cfg->power_alpha = 0.5;

    cfg->in_scale = 1.0;
    cfg->out_scale = 1.0;

	cfg->clip_s = 0;
	cfg->clip_e = 8000;
	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_eq=0;

	cfg->use_filter = 1;

	cfg->use_onnx = 0;
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_init(&(cfg->onnx));
#endif
	cfg->compress_factor = 0.3;

    qtk_nnrt_cfg_init(&cfg->rt);
	cfg->num_in = 6;
	cfg->num_out = 2;

	qtk_rir_estimate_cfg_init(&(cfg->r_est));
	cfg->use_refcompensation = 0;
    return 0;
}
int wtk_pem_afc_b_cfg_clean(wtk_pem_afc_b_cfg_t *cfg)
{
    if(cfg->mic_channel){
        wtk_free(cfg->mic_channel);
    }
    if(cfg->sp_channel){
        wtk_free(cfg->sp_channel);
    }
	wtk_equalizer_cfg_clean(&(cfg->eq));
    qtk_nnrt_cfg_clean(&cfg->rt);
	if(cfg->use_refcompensation){
		qtk_rir_estimate_cfg_clean(&cfg->r_est);
	}
    return 0;
}
int wtk_pem_afc_b_cfg_update_local(wtk_pem_afc_b_cfg_t *cfg, wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,hop_size,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,DAC_delay,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,N_afc,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,N_ar,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,mu1,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mu2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,delta,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,power_alpha,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,in_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,out_scale,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_filter,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_onnx,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ncnn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,compress_factor,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_refcompensation,v);
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
    if(cfg->nmicchannel!=1){
        wtk_debug("mic_channel array size is not 1");
        exit(0);
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
	}else{
	    wtk_local_cfg_update_cfg_i(lc,cfg,nspchannel,v);
    }
    if(cfg->nspchannel!=1){
        wtk_debug("sp_channel array size is not 1");
        exit(0);
    }
	lc=wtk_local_cfg_find_lc_s(m,"eq");
	if(lc)
	{
		ret=wtk_equalizer_cfg_update_local(&(cfg->eq),lc);
		if(ret!=0){goto end;}
	}
#ifdef ONNX_DEC
    lc = wtk_local_cfg_find_lc_s(m, "onnx");
    if (lc) {
        ret = qtk_onnxruntime_cfg_update_local(&(cfg->onnx), lc);
        if (ret != 0) {
            wtk_debug("update local onnx failed\n");
            goto end;
        }
    }
#endif
    lc = wtk_local_cfg_find_lc_s(m, "rt");
    if (lc) {
        qtk_nnrt_cfg_update_local(&cfg->rt, lc);
    }


	if(cfg->use_refcompensation){
		lc = wtk_local_cfg_find_lc_s(m, "r_est");
		if (lc) {
			qtk_rir_estimate_cfg_update_local(&cfg->r_est, lc);
		}
	}
    ret = 0;
end:
    return ret;
}


int wtk_pem_afc_b_cfg_update(wtk_pem_afc_b_cfg_t *cfg)
{
	int ret;
    cfg->Nframe_DAC_delay = floor(cfg->DAC_delay * cfg->rate * 1.0 / 1000);//(int)(cfg->DAC_delay / cfg->hop_size);
    cfg->N_block = cfg->N_afc / cfg->hop_size;
	if(cfg->channel<cfg->nmicchannel)
	{
		cfg->channel=cfg->nmicchannel;
	}
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
#ifdef ONNX_DEC
    ret = qtk_onnxruntime_cfg_update(&(cfg->onnx));
    if (ret != 0) {
        wtk_debug("update onnx failed\n");
        goto end;
    }
#endif
    qtk_nnrt_cfg_update(&cfg->rt);

	if(cfg->use_refcompensation){
		ret = qtk_rir_estimate_cfg_update(&(cfg->r_est));
	}

	ret=0;
end:
	return ret;
}
int wtk_pem_afc_b_cfg_update2(wtk_pem_afc_b_cfg_t *cfg, wtk_source_loader_t *sl)
{
	int ret;

    ret = wtk_pem_afc_b_cfg_update(cfg);
    if(ret != 0){
        goto end;
    }
#ifdef ONNX_DEC
    ret = qtk_onnxruntime_cfg_update2(&(cfg->onnx), sl->hook);
    if (ret != 0) {
        wtk_debug("update onnx failed\n");
        goto end;
    }
#endif
	ret=0;
end:
	return ret;
}

wtk_pem_afc_b_cfg_t* wtk_pem_afc_b_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_pem_afc_b_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_pem_afc_b_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_pem_afc_b_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_pem_afc_b_cfg_delete(wtk_pem_afc_b_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_pem_afc_b_cfg_t* wtk_pem_afc_b_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_pem_afc_b_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_pem_afc_b_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_pem_afc_b_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_pem_afc_b_cfg_delete_bin(wtk_pem_afc_b_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

void wtk_pem_afc_b_cfg_set(wtk_pem_afc_b_cfg_t *cfg, float out_scale,float delay){
	cfg->out_scale = out_scale;
	if(delay > 0)
	{
		cfg->Nframe_DAC_delay = floor(cfg->DAC_delay * cfg->rate * 1.0 / 1000);
	}
}