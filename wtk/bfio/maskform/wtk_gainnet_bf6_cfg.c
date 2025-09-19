#include "wtk_gainnet_bf6_cfg.h"

int wtk_gainnet_bf6_cfg_init(wtk_gainnet_bf6_cfg_t *cfg)
{
	cfg->channel=4;
    cfg->wins=320;

    cfg->ceps_mem=8;
    cfg->nb_delta_ceps=6;

    cfg->nb_bands=0;  

    cfg->use_rbin_res=0;

	wtk_aspec_cfg_init(&(cfg->aspec));
	wtk_covm_cfg_init(&(cfg->covm));
	wtk_bf_cfg_init(&(cfg->bf));

	cfg->use_preemph=1;

	cfg->micnenr=0.5;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->use_ceps=1;


	cfg->rate=16000;

	cfg->agc_a=0.69;
	cfg->agc_b=6.9;

	cfg->lf=6;
    cfg->theta_range=45;
	
	cfg->use_line=0;

	cfg->specsum_fs=0;
	cfg->specsum_fe=8000;

	cfg->clip_s=0;
	cfg->clip_e=8000;

	cfg->thstep=1;

	cfg->use_spec2=0;

	wtk_qmmse_cfg_init(&(cfg->qmmse));
	cfg->use_qmmse=0;

	cfg->mdl_fn=NULL;
    cfg->gainnet=NULL;
	cfg->use_premask=0;

	cfg->use_specmean=0;

	cfg->use_nbands=1;

	cfg->use_masknet=0;
	cfg->masknet=NULL;

	cfg->ls_eye=0.5;
	cfg->use_lsbf=1;

	return 0;
}

int wtk_gainnet_bf6_cfg_clean(wtk_gainnet_bf6_cfg_t *cfg)
{
	if(cfg->gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet2_cfg_delete_bin3(cfg->gainnet);
		}else
		{
			wtk_gainnet2_cfg_delete_bin2(cfg->gainnet);
		}
	}
	if(cfg->masknet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_masknet_cfg_delete_bin3(cfg->masknet);
		}else
		{
			wtk_masknet_cfg_delete_bin2(cfg->masknet);
		}
	}

	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_covm_cfg_clean(&(cfg->covm));
    wtk_aspec_cfg_clean(&(cfg->aspec));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));

	return 0;
}

int wtk_gainnet_bf6_cfg_update_local(wtk_gainnet_bf6_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ceps_mem,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_delta_ceps,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_bands,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micnenr,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_ceps,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,lf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,theta_range,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,specsum_fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,specsum_fe,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,thstep,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_spec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_premask,v);	

	wtk_local_cfg_update_cfg_b(lc,cfg,use_specmean,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_nbands,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_masknet,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ls_eye,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lsbf,v);
	
	lc=wtk_local_cfg_find_lc_s(m,"aspec");
	if(lc)
	{
        ret=wtk_aspec_cfg_update_local(&(cfg->aspec),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
		wtk_bf_cfg_update_local(&(cfg->bf),lc);
	}
	lc=wtk_local_cfg_find_lc_s(m,"covm");
	if(lc)
	{
        ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf6_cfg_update(wtk_gainnet_bf6_cfg_t *cfg)
{
	int ret;

	if(cfg->use_ceps)
	{
		cfg->nb_features_x=cfg->nb_bands+2*cfg->nb_delta_ceps+1;//+cfg->nb_bands;
	}else
	{
		cfg->nb_features_x=cfg->nb_bands;//+cfg->nb_bands;
	}
	cfg->nb_features=cfg->nb_features_x*2+cfg->nb_bands;//+cfg->nb_features_x;
	// cfg->nb_features+=1;
	if(cfg->mdl_fn)
	{
		if(cfg->use_masknet)
		{
			cfg->masknet=wtk_masknet_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->masknet){ret=-1;goto end;}
		}else
		{
			cfg->gainnet=wtk_gainnet2_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet){ret=-1;goto end;}
		}
	}

	cfg->specsum_fs=max(1, cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_fe=min(cfg->wins/2-2, cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));

	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
	cfg->qmmse.step=cfg->wins/2;
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}

	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf6_cfg_update2(wtk_gainnet_bf6_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	if(cfg->use_ceps)
	{
		cfg->nb_features_x=cfg->nb_bands+2*cfg->nb_delta_ceps+1;//+cfg->nb_bands;
	}else
	{
		cfg->nb_features_x=cfg->nb_bands;//+cfg->nb_bands;
	}
	cfg->nb_features=cfg->nb_features_x*2+cfg->nb_bands;//+cfg->nb_features_x;
	// cfg->nb_features+=1;
	
	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		if(cfg->use_masknet)
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->masknet=wtk_masknet_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->masknet){ret=-1;goto end;}
		}else
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet=wtk_gainnet2_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet){ret=-1;goto end;}
		}
	}

	cfg->specsum_fs=max(1, cfg->specsum_fs/(cfg->rate*1.0/cfg->wins));
	cfg->specsum_fe=min(cfg->wins/2-2, cfg->specsum_fe/(cfg->rate*1.0/cfg->wins));

	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_aspec_cfg_update(&(cfg->aspec));
    if(ret!=0){goto end;}
	cfg->qmmse.step=cfg->wins/2;
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}

	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	ret=0;
end:
	return ret;
}

wtk_gainnet_bf6_cfg_t* wtk_gainnet_bf6_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_bf6_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_bf6_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf6_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_bf6_cfg_delete(wtk_gainnet_bf6_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_bf6_cfg_t* wtk_gainnet_bf6_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_bf6_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_bf6_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf6_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_bf6_cfg_delete_bin(wtk_gainnet_bf6_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

