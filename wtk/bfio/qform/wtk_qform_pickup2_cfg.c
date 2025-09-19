#include "wtk_qform_pickup2_cfg.h"

int wtk_qform_pickup2_cfg_init(wtk_qform_pickup2_cfg_t *cfg)
{
    wtk_stft_cfg_init(&(cfg->stft));
    wtk_qmmse_cfg_init(&(cfg->qmmse));

    cfg->rate=16000;

    cfg->notch_radius=0;
	cfg->preemph=0;

    return 0;
}


int wtk_qform_pickup2_cfg_clean(wtk_qform_pickup2_cfg_t *cfg)
{
    wtk_stft_cfg_clean(&(cfg->stft));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));

    return 0;
}

int wtk_qform_pickup2_cfg_update_local(wtk_qform_pickup2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;

	wtk_local_cfg_update_cfg_f(lc,cfg,notch_radius,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,preemph,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);
    m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"stft");
	if(lc)
	{
        ret=wtk_stft_cfg_update_local(&(cfg->stft),lc);
        if(ret!=0){goto end;}
    }

    lc=wtk_local_cfg_find_lc_s(m,"qmmse");
	if(lc)
	{
        cfg->qmmse.step=cfg->stft.win/2;
        ret=wtk_qmmse_cfg_update_local(&(cfg->qmmse),lc);
        if(ret!=0){goto end;}
    }

end:
    return ret;
}

int wtk_qform_pickup2_cfg_update(wtk_qform_pickup2_cfg_t *cfg)
{
    int ret;

    ret=wtk_stft_cfg_update(&(cfg->stft));
    if(ret!=0){goto end;}
    ret=wtk_qmmse_cfg_update(&(cfg->qmmse));
    if(ret!=0){goto end;}
    cfg->notch_radius_den=cfg->notch_radius*cfg->notch_radius+0.7*(1-cfg->notch_radius)*(1-cfg->notch_radius);

    ret=0;
end:
    return ret;
}

int wtk_qform_pickup2_cfg_update2(wtk_qform_pickup2_cfg_t *cfg,wtk_source_loader_t *sl)
{
    return wtk_qform_pickup2_cfg_update(cfg);
}



wtk_qform_pickup2_cfg_t* wtk_qform_pickup2_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_qform_pickup2_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_qform_pickup2_cfg,cfg_fn);
	cfg=(wtk_qform_pickup2_cfg_t*)main_cfg->cfg;
	cfg->main_cfg=main_cfg;
	return cfg;
}

void wtk_qform_pickup2_cfg_delete(wtk_qform_pickup2_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_qform_pickup2_cfg_t* wtk_qform_pickup2_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_qform_pickup2_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_qform_pickup2_cfg,bin_fn,"./cfg");
	cfg=(wtk_qform_pickup2_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_qform_pickup2_cfg_delete_bin(wtk_qform_pickup2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

void wtk_qform_pickup2_cfg_set_noise_suppress(wtk_qform_pickup2_cfg_t *cfg,float noise_suppress)
{
    if(fabs(noise_suppress)>0.0)
    {
        wtk_qmmse_cfg_set_noise_suppress(&(cfg->qmmse),noise_suppress);
    }
}

