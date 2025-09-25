#include "wtk_gainnet_bf_stereo_cfg.h"

int wtk_gainnet_bf_stereo_cfg_init(wtk_gainnet_bf_stereo_cfg_t *cfg)
{
	cfg->nmicchannel=0;
	cfg->noutchannel=0;
	cfg->mic_channel=NULL;
	cfg->out_channel=NULL;
	cfg->channel=0;

    cfg->wins=1024;

	wtk_bankfeat_cfg_init(&(cfg->bankfeat));

    cfg->mdl_fn=NULL;
    cfg->gainnet2=NULL;
    cfg->use_rbin_res=0;

	wtk_covm_cfg_init(&(cfg->covm));

	wtk_bf_cfg_init(&(cfg->bf));
	cfg->theta = 180;
	cfg->phi=0;

	wtk_qmmse_cfg_init(&(cfg->qmmse));

	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->rate=16000;

	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_eq=0;

	cfg->use_fftsbf=0;

	cfg->clip_s=0;
	cfg->clip_e=8000;

	cfg->ralpha=0;
	cfg->ralpha2=0;

	cfg->use_qmmse=1;


	cfg->featm_lm=1;

	cfg->g_min=0.05;
	cfg->g_max=0.95;
	cfg->g_a=5.88;
	cfg->g_b=2.94;
	cfg->gbias=0;

	cfg->g_minthresh=1e-6;

    cfg->agcmdl_fn=NULL;
    cfg->agc_gainnet=NULL;
	cfg->g2_min=0.05;
	cfg->g2_max=0.95;
	cfg->agc_a=0.69;
	cfg->agc_b=6.9;
	cfg->agce_thresh=0.1;

	cfg->use_gsigmoid=0;

	cfg->repate_outchannel=0;


	return 0;
}

int wtk_gainnet_bf_stereo_cfg_clean(wtk_gainnet_bf_stereo_cfg_t *cfg)
{
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->out_channel)
	{
		wtk_free(cfg->out_channel);
	}
	if(cfg->gainnet2)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet2_cfg_delete_bin3(cfg->gainnet2);
		}else
		{
			wtk_gainnet2_cfg_delete_bin2(cfg->gainnet2);
		}
	}
	if(cfg->agc_gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet_cfg_delete_bin3(cfg->agc_gainnet);
		}else
		{
			wtk_gainnet_cfg_delete_bin2(cfg->agc_gainnet);
		}
	}

	wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_covm_cfg_clean(&(cfg->covm));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_equalizer_cfg_clean(&(cfg->eq));

	return 0;
}

int wtk_gainnet_bf_stereo_cfg_update_local(wtk_gainnet_bf_stereo_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;
	wtk_array_t *a;
	int i;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);

    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phi,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_fftsbf,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha2,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_str(lc,cfg,agcmdl_fn,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g2_min,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g2_max,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agce_thresh,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,g_min,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g_max,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g_b,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gbias,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,g_minthresh,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,featm_lm,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,repate_outchannel,v);
	
	a=wtk_local_cfg_find_array_s(m,"mic_channel");
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
	a=wtk_local_cfg_find_array_s(m,"out_channel");
	if(a)
	{
		cfg->out_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->noutchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->out_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	lc=wtk_local_cfg_find_lc_s(m,"bankfeat");
	if(lc)
	{
		ret=wtk_bankfeat_cfg_update_local(&(cfg->bankfeat),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"bf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
		if(ret!=0){goto end;}
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
		cfg->qmmse.step=cfg->wins/2;
        if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"eq");
	if(lc)
	{
		ret=wtk_equalizer_cfg_update_local(&(cfg->eq),lc);
		if(ret!=0){goto end;}
	}


	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf_stereo_cfg_update(wtk_gainnet_bf_stereo_cfg_t *cfg)
{
	int ret;

	if(cfg->mdl_fn)
	{
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin2(cfg->mdl_fn);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}
	if(cfg->agcmdl_fn)
	{
		cfg->agc_gainnet=wtk_gainnet_cfg_new_bin2(cfg->agcmdl_fn);
		if(!cfg->agc_gainnet){ret=-1;goto end;}
	}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel)
	{
		cfg->channel=cfg->nmicchannel;
	}
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf_stereo_cfg_update2(wtk_gainnet_bf_stereo_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet2=wtk_gainnet2_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->gainnet2){ret=-1;goto end;}
	}
	if(cfg->agcmdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->agcmdl_fn,strlen(cfg->agcmdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->agc_gainnet=wtk_gainnet_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->agc_gainnet){ret=-1;goto end;}
	}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	if(ret!=0){goto end;}
	if(cfg->channel<cfg->nmicchannel)
	{
		cfg->channel=cfg->nmicchannel;
	}
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	ret=0;
end:
	return ret;
}

wtk_gainnet_bf_stereo_cfg_t* wtk_gainnet_bf_stereo_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_bf_stereo_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_bf_stereo_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf_stereo_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_bf_stereo_cfg_delete(wtk_gainnet_bf_stereo_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_bf_stereo_cfg_t* wtk_gainnet_bf_stereo_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_bf_stereo_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_bf_stereo_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_bf_stereo_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_bf_stereo_cfg_delete_bin(wtk_gainnet_bf_stereo_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

