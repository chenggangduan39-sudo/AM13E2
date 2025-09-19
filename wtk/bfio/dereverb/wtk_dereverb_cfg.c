#include "wtk_dereverb_cfg.h"

int wtk_dereverb_cfg_init(wtk_dereverb_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft));
	wtk_bf_cfg_init(&(cfg->bf));
	wtk_admm_cfg_init(&(cfg->admm));
	wtk_covm_cfg_init(&(cfg->covm));

	cfg->mbin_cfg=NULL;
	cfg->main_cfg=NULL;

	cfg->theta = 180;
	cfg->phi=0;
	cfg->use_fixtheta=1;
	cfg->use_admm=1;

	cfg->use_exlsty=0;
	cfg->fft_scale=1;

	cfg->theta2=0;

	return 0;
}

int wtk_dereverb_cfg_clean(wtk_dereverb_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft));
	wtk_covm_cfg_clean(&(cfg->covm));
	wtk_bf_cfg_clean(&(cfg->bf));
	wtk_admm_cfg_clean(&(cfg->admm));

	return 0;
}

int wtk_dereverb_cfg_update_local(wtk_dereverb_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_f(lc,cfg,theta,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,theta2,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,phi,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,fft_scale,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixtheta,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_admm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_exlsty,v);

	lc=wtk_local_cfg_find_lc_s(main,"stft2");
	if(lc)
	{
		ret=wtk_stft2_cfg_update_local(&(cfg->stft),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"covm");
	if(lc)
	{
        ret=wtk_covm_cfg_update_local(&(cfg->covm),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(main,"bf");
	if(lc)
	{
		ret=wtk_bf_cfg_update_local(&(cfg->bf),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"admm");
	if(lc)
	{
		ret=wtk_admm_cfg_update_local(&(cfg->admm),lc);
		if(ret!=0){goto end;}
	}

	ret=0;
end:
	return ret;
}

int wtk_dereverb_cfg_update(wtk_dereverb_cfg_t *cfg)
{
	int ret;

	ret=wtk_stft2_cfg_update(&(cfg->stft));
	if(ret!=0){goto end;}
	ret=wtk_covm_cfg_update(&(cfg->covm));
    if(ret!=0){goto end;}
	ret=wtk_bf_cfg_update(&(cfg->bf));
	if(ret!=0){goto end;}
	ret=wtk_admm_cfg_update(&(cfg->admm));
	if(ret!=0){goto end;}
	ret=0;
end:
	return ret;
}

int wtk_dereverb_cfg_update2(wtk_dereverb_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_dereverb_cfg_update(cfg);
}

wtk_dereverb_cfg_t* wtk_dereverb_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_dereverb_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_dereverb_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_dereverb_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_dereverb_cfg_delete(wtk_dereverb_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_dereverb_cfg_t* wtk_dereverb_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_dereverb_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_dereverb_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_dereverb_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_dereverb_cfg_delete_bin(wtk_dereverb_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

