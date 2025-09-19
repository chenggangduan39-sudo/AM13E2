#include "wtk_gainnet_aec5_cfg.h"

int wtk_gainnet_aec5_cfg_init(wtk_gainnet_aec5_cfg_t *cfg)
{
	cfg->nmicchannel=1;
	cfg->nspchannel=1;

	cfg->rate=16000;
    cfg->wins=1024;

	wtk_bankfeat_cfg_init(&(cfg->bankfeat));

    cfg->mdl_fn=NULL;
    cfg->gainnet=NULL;
	cfg->gainnet2=NULL;
	cfg->gainnet6=NULL;
	cfg->use_gainnet2=0;
	cfg->use_gainnet6=0;
    cfg->use_rbin_res=0;

	wtk_rls_cfg_init(&(cfg->echo_rls));
	cfg->use_nlms=0;
	cfg->use_rls=1;
	cfg->use_kalman=0;
	wtk_nlms_cfg_init(&(cfg->echo_nlms));
	qtk_ahs_kalman_cfg_init(&(cfg->echo_kalman));

	cfg->use_preemph=1;

	cfg->spenr_thresh=100;
	cfg->spenr_cnt=10;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->use_epostsingle=0;

	cfg->use_qmmse=0;
	wtk_qmmse_cfg_init(&(cfg->qmmse));

	cfg->use_maxleak=0;

	cfg->agc_a=0.69;
	cfg->agc_b=6.9;

	cfg->gbias=0;

	cfg->ralpha=0.5;

	cfg->use_nlmsdelay=0;
	cfg->M=4;
	cfg->power_alpha=0.1;
	cfg->mufb=0.1;
	cfg->nlms_ms=8000;

	return 0;
}

int wtk_gainnet_aec5_cfg_clean(wtk_gainnet_aec5_cfg_t *cfg)
{
	if(cfg->gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet5_cfg_delete_bin3(cfg->gainnet);
		}else
		{
			wtk_gainnet5_cfg_delete_bin2(cfg->gainnet);
		}
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
	if(cfg->gainnet6)
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
	wtk_nlms_cfg_clean(&(cfg->echo_nlms));
	qtk_ahs_kalman_cfg_clean(&(cfg->echo_kalman));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	
	return 0;
}

int wtk_gainnet_aec5_cfg_update_local(wtk_gainnet_aec5_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_preemph,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,spenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,spenr_cnt,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_epostsingle,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,nmicchannel,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,nspchannel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nmicchannel,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_gainnet2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_gainnet6,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_maxleak,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,gbias,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,M,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nlms_ms,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,power_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,mufb,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_nlmsdelay,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_nlms,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rls,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_kalman,v);

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
	lc=wtk_local_cfg_find_lc_s(m,"echo_nlms");
	if(lc)
	{
		ret=wtk_nlms_cfg_update_local(&(cfg->echo_nlms),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"echo_kalman");
	if(lc)
	{
		ret=qtk_ahs_kalman_cfg_update_local(&(cfg->echo_kalman),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
		ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_gainnet_aec5_cfg_update(wtk_gainnet_aec5_cfg_t *cfg)
{
	int ret;

	if(cfg->use_gainnet2)
	{
		if(cfg->mdl_fn)
		{
			cfg->gainnet2=wtk_gainnet2_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet2){ret=-1;goto end;}
		}
	}else if(cfg->use_gainnet6)
	{
		if(cfg->mdl_fn)
		{
			cfg->gainnet6=wtk_gainnet6_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet6){ret=-1;goto end;}
		}
	}else
	{
		if(cfg->mdl_fn)
		{
			cfg->gainnet=wtk_gainnet5_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet){ret=-1;goto end;}
		}
	}

	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}

	cfg->echo_rls.channel=cfg->nmicchannel;
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));
	if(ret!=0){goto end;}
	cfg->echo_nlms.channel=cfg->nmicchannel;
	cfg->echo_nlms.N=cfg->nspchannel;
	ret=wtk_nlms_cfg_update(&(cfg->echo_nlms));
	if(ret!=0){goto end;}
	ret=qtk_ahs_kalman_cfg_update(&(cfg->echo_kalman));
	if(ret!=0){goto end;}
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}

	cfg->nlms_nframe=cfg->nlms_ms*cfg->rate*0.002f/cfg->wins;

	ret=0;
end:
	return ret;
}

int wtk_gainnet_aec5_cfg_update2(wtk_gainnet_aec5_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;
	
	cfg->use_rbin_res=1;
	if(cfg->use_gainnet2)
	{
		if(cfg->mdl_fn)
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet2=wtk_gainnet2_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet2){ret=-1;goto end;}
		}
	}else if(cfg->use_gainnet6)
	{
		if(cfg->mdl_fn)
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet6=wtk_gainnet6_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet6){ret=-1;goto end;}
		}
	}else
	{
		if(cfg->mdl_fn)
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet=wtk_gainnet5_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet){ret=-1;goto end;}
		}
	}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}

	cfg->echo_rls.channel=cfg->nmicchannel;
	cfg->echo_rls.N=cfg->nspchannel;
	ret=wtk_rls_cfg_update(&(cfg->echo_rls));
	if(ret!=0){goto end;}
	cfg->echo_nlms.channel=cfg->nmicchannel;
	cfg->echo_nlms.N=cfg->nspchannel;
	ret=wtk_nlms_cfg_update(&(cfg->echo_nlms));
	if(ret!=0){goto end;}

	ret=qtk_ahs_kalman_cfg_update(&(cfg->echo_kalman));
	if(ret!=0){goto end;}

	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}

	cfg->nlms_nframe=cfg->nlms_ms*cfg->rate*2.0/cfg->wins;

	ret=0;
end:
	return ret;
}

wtk_gainnet_aec5_cfg_t* wtk_gainnet_aec5_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_aec5_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_aec5_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_aec5_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_aec5_cfg_delete(wtk_gainnet_aec5_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_aec5_cfg_t* wtk_gainnet_aec5_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_aec5_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_aec5_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_aec5_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_aec5_cfg_delete_bin(wtk_gainnet_aec5_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

