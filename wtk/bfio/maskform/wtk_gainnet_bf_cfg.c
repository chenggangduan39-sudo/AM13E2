#include "wtk_gainnet_bf_cfg.h"

int wtk_gainnet_bf_cfg_init(wtk_gainnet_bf_cfg_t *cfg)
{
	cfg->channel=4;
    cfg->wins=320;
	cfg->rate=16000;

    cfg->pitch_min_period=32;
    cfg->pitch_max_period=256;
    cfg->pitch_frame_size=320;

    cfg->ceps_mem=8;
    cfg->nb_delta_ceps=6;

    cfg->nb_bands=0;  
	cfg->eband=NULL;

    cfg->mdl_fn=NULL;
    cfg->gainnet=NULL;
    cfg->use_rbin_res=0;
	cfg->use_pitchpost=1;

	cfg->use_denoise_single=0;

	cfg->use_ssl=0;
	wtk_qenvelope_cfg_init(&(cfg->qenvl));
	wtk_ssl2_cfg_init(&(cfg->ssl2));
	cfg->use_qmmse=0;
	wtk_qmmse_cfg_init(&(cfg->qmmse));

	wtk_covm_cfg_init(&(cfg->covm));
    wtk_bf_cfg_init(&(cfg->bf));

	cfg->theta=90;
	cfg->phi=0;
	
    cfg->use_agc=0;
    cfg->agc_mdl_fn=NULL;
    cfg->agc_gainnet=NULL;
	cfg->agc_a=0.69;
	cfg->agc_b=6.9;

	cfg->pframe_alpha=0.2;
	cfg->pframe_thresh=0.1;

	return 0;
}

int wtk_gainnet_bf_cfg_clean(wtk_gainnet_bf_cfg_t *cfg)
{
    if(cfg->eband)
    {
        wtk_free(cfg->eband);
    }
	if(cfg->gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet_cfg_delete_bin3(cfg->gainnet);
		}else
		{
			wtk_gainnet_cfg_delete_bin2(cfg->gainnet);
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

	wtk_qenvelope_cfg_clean(&(cfg->qenvl));
	wtk_ssl2_cfg_clean(&(cfg->ssl2));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));
	wtk_covm_cfg_clean(&(cfg->covm));
    wtk_bf_cfg_clean(&(cfg->bf));

	return 0;
}

int wtk_gainnet_bf_cfg_update_local(wtk_gainnet_bf_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	wtk_array_t *a;
	int i, ret;

	lc=m;
    wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,pitch_min_period,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pitch_max_period,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pitch_frame_size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,ceps_mem,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nb_delta_ceps,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_pitchpost,v);

    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_denoise_single,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_ssl,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phi,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_agc,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,agc_mdl_fn,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,pframe_alpha,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pframe_thresh,v);

	a=wtk_local_cfg_find_int_array_s(lc,"eband");
    if(a)
    {
        cfg->nb_bands=a->nslot;
        cfg->eband=(int *)wtk_malloc(cfg->nb_bands*sizeof(int));
        for(i=0;i<cfg->nb_bands;++i)
        {
            cfg->eband[i]=((int *)(a->slot))[i];
        }
    }
	lc=wtk_local_cfg_find_lc_s(m,"qenvelope");
	if(lc)
	{
		ret=wtk_qenvelope_cfg_update_local(&(cfg->qenvl),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(m,"ssl2");
	if(lc)
	{
		ret=wtk_ssl2_cfg_update_local(&(cfg->ssl2),lc);
		cfg->ssl2.wins=cfg->wins;
		if(ret!=0){goto end;}
	}	
	lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
		ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
		cfg->qmmse.step=cfg->wins/2;
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

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf_cfg_update(wtk_gainnet_bf_cfg_t *cfg)
{
	int ret;

	cfg->pitch_buf_size=cfg->pitch_max_period+cfg->pitch_frame_size;
    cfg->nb_features=cfg->nb_bands+3*cfg->nb_delta_ceps+2;

	if(cfg->mdl_fn)
	{
		cfg->gainnet=wtk_gainnet_cfg_new_bin2(cfg->mdl_fn);
		if(!cfg->gainnet){ret=-1;goto end;}
	}
	if(cfg->agc_mdl_fn)
	{
		cfg->agc_gainnet=wtk_gainnet_cfg_new_bin2(cfg->agc_mdl_fn);
		if(!cfg->agc_gainnet){ret=-1;goto end;}
	}
	cfg->agc_nb_features=cfg->nb_bands+2*cfg->nb_delta_ceps+1;

	if(cfg->use_qmmse)
	{
		ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
		if(ret!=0){goto end;}
	}
	if(cfg->use_ssl)
	{
		ret=wtk_qenvelope_cfg_update(&(cfg->qenvl));
		if(ret!=0){goto end;}
		ret=wtk_ssl2_cfg_update(&(cfg->ssl2));
		if(ret!=0){goto end;}
	}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

int wtk_gainnet_bf_cfg_update2(wtk_gainnet_bf_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->gainnet=wtk_gainnet_cfg_new_bin3(rbin->fn,item->pos);
        if(!cfg->gainnet){ret=-1;goto end;}
	}
	if(cfg->agc_mdl_fn)
	{
		item=wtk_rbin2_get(rbin,cfg->agc_mdl_fn,strlen(cfg->agc_mdl_fn));
		if(!item){ret=-1;goto end;}
		cfg->agc_gainnet=wtk_gainnet_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->agc_gainnet){ret=-1;goto end;}
	}
	cfg->agc_nb_features=cfg->nb_bands+2*cfg->nb_delta_ceps+1;

    cfg->pitch_buf_size=cfg->pitch_max_period+cfg->pitch_frame_size;
    cfg->nb_features=cfg->nb_bands+3*cfg->nb_delta_ceps+2;
	if(cfg->use_ssl)
	{
		ret=wtk_qenvelope_cfg_update(&(cfg->qenvl));
		if(ret!=0){goto end;}
		ret=wtk_ssl2_cfg_update(&(cfg->ssl2));
		if(ret!=0){goto end;}
	}
	if(cfg->use_qmmse)
	{
		ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
		if(ret!=0){goto end;}
	}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
    if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}