#include "wtk_vboxebf4_cfg.h"

int wtk_vboxebf4_cfg_init(wtk_vboxebf4_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
	cfg->nspchannel=0;
	cfg->sp_channel=NULL;
	cfg->nbfchannel=0;

    cfg->wins=1024;

    cfg->mdl_fn=NULL;
    cfg->gainnet=NULL;
	cfg->gainnet6=NULL;
	cfg->use_gainnet6=0;
    cfg->use_rbin_res=0;

	wtk_covm_cfg_init(&(cfg->covm));
	wtk_covm_cfg_init(&(cfg->echo_covm));

	cfg->bfmu=1;
	cfg->echo_bfmu=1;
	wtk_bf_cfg_init(&(cfg->bf));
	cfg->theta = 180;
	cfg->phi=0;

	wtk_rls_cfg_init(&(cfg->echo_rls));

	wtk_bankfeat_cfg_init(&(cfg->bankfeat));

	wtk_qmmse_cfg_init(&(cfg->qmmse));
	cfg->use_qmmse=0;

	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;

	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->use_epostsingle=0;

	cfg->use_fixtheta=0;

	cfg->rate=16000;

	wtk_equalizer_cfg_init(&(cfg->eq));
	cfg->use_eq=0;

	cfg->agc_a=0.69;
	cfg->agc_a2=1.0;
	cfg->agc_b=6.9;

	cfg->use_ssl=0;
	wtk_ssl2_cfg_init(&(cfg->ssl2));
	cfg->use_maskssl=0;
	wtk_maskssl_cfg_init(&(cfg->maskssl));
	cfg->use_maskssl2=0;
	wtk_maskssl2_cfg_init(&(cfg->maskssl2));
	
	cfg->pframe_fs=200;
	cfg->pframe_fe=8000;
	cfg->pframe_alpha=1;
	cfg->pframe_thresh=-1;

	cfg->use_fftsbf=0;
	cfg->use_efftsbf=1;

	cfg->use_phaseo=0;

	cfg->clip_s=0;
	cfg->clip_e=8000;

	cfg->gbias=0;

	cfg->ralpha=0.6;

	cfg->use_aecmmse=1;
	cfg->use_demmse=1;

	cfg->use_echocovm=0;

	cfg->use_cnon=0;
	cfg->sym=1e-2;
	cfg->use_ssl_delay=0;

	cfg->de_clip_s = 0;
	cfg->de_clip_e = 8000;
	cfg->de_thresh = 1e4;
	cfg->de_alpha = 1.0;
	return 0;
}

int wtk_vboxebf4_cfg_clean(wtk_vboxebf4_cfg_t *cfg)
{
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->sp_channel)
	{
		wtk_free(cfg->sp_channel);
	}
	if(cfg->gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet5_cfg_delete_bin3(cfg->gainnet);
		}else
		{
			wtk_gainnet5_cfg_delete_bin2(cfg->gainnet);
		}
	}else
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet6_cfg_delete_bin3(cfg->gainnet6);
		}else
		{
			wtk_gainnet6_cfg_delete_bin2(cfg->gainnet6);
		}
	}
	wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
	wtk_rls_cfg_clean(&(cfg->echo_rls));
	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_covm_cfg_clean(&(cfg->covm));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_equalizer_cfg_clean(&(cfg->eq));
	wtk_ssl2_cfg_clean(&(cfg->ssl2));
	wtk_maskssl_cfg_clean(&(cfg->maskssl));
	wtk_maskssl2_cfg_clean(&(cfg->maskssl2));

	return 0;
}

int wtk_vboxebf4_cfg_update_local(wtk_vboxebf4_cfg_t *cfg,wtk_local_cfg_t *m)
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

	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_epostsingle,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixtheta,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_ssl,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskssl,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_maskssl2,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_gainnet6,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,pframe_fs,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pframe_fe,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pframe_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pframe_thresh,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_fftsbf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_efftsbf,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,clip_e,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_phaseo,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,gbias,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,bfmu,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,echo_bfmu,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_aecmmse,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_demmse,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_echocovm,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbfchannel,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,use_cnon,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,sym,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ssl_delay,v);
	
	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_s,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,de_clip_e,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,de_alpha,v);
	
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

	lc=wtk_local_cfg_find_lc_s(m,"bankfeat");
	if(lc)
	{
		ret=wtk_bankfeat_cfg_update_local(&(cfg->bankfeat),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_rls");
	if(lc)
	{
		ret=wtk_rls_cfg_update_local(&(cfg->echo_rls),lc);
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
	lc=wtk_local_cfg_find_lc_s(m,"echo_covm");
	if(lc)
	{
        ret=wtk_covm_cfg_update_local(&(cfg->echo_covm),lc);
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

	lc=wtk_local_cfg_find_lc_s(m,"ssl2");
	if(lc)
	{
		ret=wtk_ssl2_cfg_update_local(&(cfg->ssl2),lc);
		cfg->ssl2.wins=cfg->wins;
		if(ret!=0){goto end;}
	}	
	lc=wtk_local_cfg_find_lc_s(m,"maskssl");
	if(lc)
	{
		ret=wtk_maskssl_cfg_update_local(&(cfg->maskssl),lc);
		cfg->maskssl.wins=cfg->wins;
		if(ret!=0){goto end;}
	}		
	lc=wtk_local_cfg_find_lc_s(m,"maskssl2");
	if(lc)
	{
		ret=wtk_maskssl2_cfg_update_local(&(cfg->maskssl2),lc);
		cfg->maskssl2.wins=cfg->wins;
		if(ret!=0){goto end;}
	}	
	ret=0;
end:
	return ret;
}

int wtk_vboxebf4_cfg_update(wtk_vboxebf4_cfg_t *cfg)
{
	int ret;

	if(cfg->mdl_fn)
	{
		if(cfg->use_gainnet6==0)
		{
			cfg->gainnet=wtk_gainnet5_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet){ret=-1;goto end;}
		}else
		{
			cfg->gainnet6=wtk_gainnet6_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet6){ret=-1;goto end;}
		}
	}

	cfg->pframe_fs=max(1, cfg->pframe_fs/(cfg->rate*1.0/cfg->wins));
	cfg->pframe_fe=min(cfg->wins/2-2, cfg->pframe_fe/(cfg->rate*1.0/cfg->wins));

	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->echo_covm));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}

	cfg->echo_rls.channel=cfg->nmicchannel;
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	if(cfg->use_ssl)
	{
		ret=wtk_ssl2_cfg_update(&(cfg->ssl2));
		if(ret!=0){goto end;}
	}
	if(cfg->use_maskssl)
	{
		ret=wtk_maskssl_cfg_update(&(cfg->maskssl));
		if(ret!=0){goto end;}
	}	
	if(cfg->use_maskssl2)
	{
		ret=wtk_maskssl2_cfg_update(&(cfg->maskssl2));
		if(ret!=0){goto end;}
	}
	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
	}
	cfg->bf.nmic = cfg->nbfchannel;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;

	cfg->de_clip_s = (cfg->de_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->de_clip_e = (cfg->de_clip_e * 1.0 * cfg->wins) / cfg->rate;

	ret=0;
end:
	return ret;
}

int wtk_vboxebf4_cfg_update2(wtk_vboxebf4_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		if(cfg->use_gainnet6==0)
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet=wtk_gainnet5_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet){ret=-1;goto end;}
		}else
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet6=wtk_gainnet6_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet6){ret=-1;goto end;}
		}
	}
	
	cfg->pframe_fs=max(1, cfg->pframe_fs/(cfg->rate*1.0/cfg->wins));
	cfg->pframe_fe=min(cfg->wins/2-2, cfg->pframe_fe/(cfg->rate*1.0/cfg->wins));

	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->echo_covm));
	if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}

	cfg->echo_rls.channel=cfg->nmicchannel;
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));	
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}
	ret=wtk_equalizer_cfg_update(&(cfg->eq));
	if(ret!=0){goto end;}
	if(cfg->use_ssl)
	{
		ret=wtk_ssl2_cfg_update(&(cfg->ssl2));
		if(ret!=0){goto end;}
	}
	if(cfg->use_maskssl)
	{
		ret=wtk_maskssl_cfg_update2(&(cfg->maskssl),sl);
		if(ret!=0){goto end;}
	}
	if(cfg->use_maskssl2)
	{
		ret=wtk_maskssl2_cfg_update2(&(cfg->maskssl2),sl);
		if(ret!=0){goto end;}
	}

	if(cfg->channel<cfg->nmicchannel+cfg->nspchannel)
	{
		cfg->channel=cfg->nmicchannel+cfg->nspchannel;
	}
	if(cfg->nbfchannel==0){
		cfg->nbfchannel=cfg->nmicchannel;
	}
	cfg->bf.nmic = cfg->nbfchannel;
	cfg->clip_s=(cfg->clip_s*1.0*cfg->wins)/cfg->rate;
	cfg->clip_e=(cfg->clip_e*1.0*cfg->wins)/cfg->rate;
	
	cfg->de_clip_s = (cfg->de_clip_s * 1.0 * cfg->wins) / cfg->rate;
	cfg->de_clip_e = (cfg->de_clip_e * 1.0 * cfg->wins) / cfg->rate;

	ret=0;
end:
	return ret;
}

wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_vboxebf4_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_vboxebf4_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_vboxebf4_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_vboxebf4_cfg_delete(wtk_vboxebf4_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_vboxebf4_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_vboxebf4_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_vboxebf4_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_vboxebf4_cfg_delete_bin(wtk_vboxebf4_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new2(char *fn, char *fn2)
{
	wtk_main_cfg_t *main_cfg;
	wtk_vboxebf4_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type5(wtk_vboxebf4_cfg,fn,fn2);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_vboxebf4_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_vboxebf4_cfg_delete2(wtk_vboxebf4_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new_bin2(char *fn, char *fn2)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_vboxebf4_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type4(wtk_vboxebf4_cfg,fn,"./cfg",fn2);
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_vboxebf4_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_vboxebf4_cfg_delete_bin2(wtk_vboxebf4_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}