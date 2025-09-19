#include "wtk_gainnet_denoise_cfg.h" 

int wtk_gainnet_denoise_cfg_init(wtk_gainnet_denoise_cfg_t *cfg)
{  
    cfg->wins=1024;
	cfg->rate=16000;

	wtk_bankfeat_cfg_init(&(cfg->bankfeat));

    cfg->mdl_fn=NULL;
    cfg->gainnet=NULL;
	cfg->gainnet3=NULL;
    cfg->use_rbin_res=0;

	cfg->agc_a=0.69;
	cfg->agc_b=6.9;

	wtk_qmmse_cfg_init(&(cfg->qmmse));
	cfg->use_qmmse=0;

	cfg->use_gainnet3=0;

	cfg->gbias=0;
	cfg->ralpha=0.5;

	return 0;
}

int wtk_gainnet_denoise_cfg_clean(wtk_gainnet_denoise_cfg_t *cfg)
{
	if(cfg->gainnet)
	{
		if(cfg->use_rbin_res)
		{
			wtk_gainnet7_cfg_delete_bin3(cfg->gainnet);
		}else
		{
			wtk_gainnet7_cfg_delete_bin2(cfg->gainnet);
		}
        } else if (cfg->gainnet3) {
            if (cfg->use_rbin_res) {
                wtk_gainnet3_cfg_delete_bin3(cfg->gainnet3);
            } else {
                wtk_gainnet3_cfg_delete_bin2(cfg->gainnet3);
            }
        }
        wtk_bankfeat_cfg_clean(&(cfg->bankfeat));
	wtk_qmmse_cfg_clean(&(cfg->qmmse));

	return 0;
}

int wtk_gainnet_denoise_cfg_update_local(wtk_gainnet_denoise_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_local_cfg_t *m;
	int ret;

	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

    wtk_local_cfg_update_cfg_str(lc,cfg,mdl_fn,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,agc_a,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,agc_b,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_qmmse,v);

	wtk_local_cfg_update_cfg_b(lc,cfg,use_gainnet3,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,gbias,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,ralpha,v);

	m=wtk_local_cfg_find_lc_s(lc,"bankfeat");
	if(m)
	{
        ret=wtk_bankfeat_cfg_update_local(&(cfg->bankfeat),m);
        if(ret!=0){goto end;}
    }
	m=wtk_local_cfg_find_lc_s(lc,"qmmse");
	if(m)
	{
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),m);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
	return ret;
}

int wtk_gainnet_denoise_cfg_update(wtk_gainnet_denoise_cfg_t *cfg)
{
	int ret;

	if(cfg->mdl_fn)
	{
		if(cfg->use_gainnet3)
		{
			cfg->gainnet3=wtk_gainnet3_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet3){ret=-1;goto end;}
		}else
		{
			cfg->gainnet=wtk_gainnet7_cfg_new_bin2(cfg->mdl_fn);
			if(!cfg->gainnet){ret=-1;goto end;}
		}
	}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
	cfg->qmmse.step=cfg->wins/2;
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

int wtk_gainnet_denoise_cfg_update2(wtk_gainnet_denoise_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->use_rbin_res=1;
	if(cfg->mdl_fn)
	{
		if(cfg->use_gainnet3)
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet3=wtk_gainnet3_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet3){ret=-1;goto end;}
		}else
		{
			item=wtk_rbin2_get(rbin,cfg->mdl_fn,strlen(cfg->mdl_fn));
			if(!item){ret=-1;goto end;}
			cfg->gainnet=wtk_gainnet7_cfg_new_bin3(rbin->fn,item->pos);
			if(!cfg->gainnet){ret=-1;goto end;}
		}
	}
	ret=wtk_bankfeat_cfg_update(&(cfg->bankfeat));
    if(ret!=0){goto end;}
	cfg->qmmse.step=cfg->wins/2;
	ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}



wtk_gainnet_denoise_cfg_t* wtk_gainnet_denoise_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_gainnet_denoise_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_gainnet_denoise_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_denoise_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_gainnet_denoise_cfg_delete(wtk_gainnet_denoise_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_gainnet_denoise_cfg_t* wtk_gainnet_denoise_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_gainnet_denoise_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_gainnet_denoise_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_gainnet_denoise_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_gainnet_denoise_cfg_delete_bin(wtk_gainnet_denoise_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
