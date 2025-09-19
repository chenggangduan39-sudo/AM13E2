#include "wtk_consist_cfg.h"


int wtk_consist_cfg_init(wtk_consist_cfg_t *cfg)
{
    // wtk_ssl2_cfg_init(&(cfg->ssl));
	cfg->playfn=NULL;
	cfg->channel = 8;
	cfg->spchannel = 2;
	cfg->use_xcorr = 1;
	cfg->eq_offset=35000.0f;
	cfg->mic_corr_er=200.0f;
	cfg->mic_corr_aver = 180.0f;
	cfg->mic_energy_er = 800.0f;
	cfg->mic_energy_aver = 700.0f;

	cfg->spk_corr_er=200.0f;
	cfg->spk_corr_aver = 180.0f;
	cfg->spk_energy_er = 600.0f;
	cfg->spk_energy_aver = 500.0f;

	cfg->nil_er = 100.0f;

	cfg->use_equal = 1;

	cfg->mic_energy_min=800.0f;

    return 0;
}

int wtk_consist_cfg_clean(wtk_consist_cfg_t *cfg)
{
    // wtk_ssl2_cfg_clean(&(cfg->ssl));
    return 0;
}
int wtk_consist_cfg_update_local(wtk_consist_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_str(lc, cfg, playfn, v);

	wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);

	wtk_local_cfg_update_cfg_i(lc, cfg, spchannel, v);

	wtk_local_cfg_update_cfg_b(lc, cfg, use_xcorr, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_equal, v);

	wtk_local_cfg_update_cfg_f(lc, cfg, eq_offset, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, nil_er, v);

	wtk_local_cfg_update_cfg_f(lc, cfg, mic_corr_er, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_corr_aver, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_energy_er, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_energy_aver, v);

	wtk_local_cfg_update_cfg_f(lc, cfg, mic_energy_min, v);

	wtk_local_cfg_update_cfg_f(lc, cfg, spk_corr_er, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, spk_corr_aver, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, spk_energy_er, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, spk_energy_aver, v);

	// lc=wtk_local_cfg_find_lc_s(main,"ssl");
	// if(lc)
	// {
	// 	ret=wtk_ssl2_cfg_update_local(&(cfg->ssl),lc);
	// 	if(ret!=0){goto end;}
	// }

	ret=0;
//end:
    return ret;
}

int wtk_consist_cfg_update(wtk_consist_cfg_t *cfg)
{
    int ret;

    // ret=wtk_ssl2_cfg_update(&(cfg->ssl));
    // if(ret!=0){goto end;}

    ret=0;
//end:
    return ret;
}

int wtk_consist_cfg_update2(wtk_consist_cfg_t *cfg, wtk_source_loader_t *lc)
{
    return wtk_consist_cfg_update(cfg);
}

wtk_consist_cfg_t* wtk_consist_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_consist_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_consist_cfg,cfg_fn);
	cfg=(wtk_consist_cfg_t*)main_cfg->cfg;
	cfg->hook=main_cfg;
	return cfg;
}

void wtk_consist_cfg_delete(wtk_consist_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t *)cfg->hook);
}

wtk_consist_cfg_t* wtk_consist_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_consist_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_consist_cfg,bin_fn,"./cfg");
	cfg=(wtk_consist_cfg_t*)mbin_cfg->cfg;
	cfg->hook=mbin_cfg;
	return cfg;
}

void wtk_consist_cfg_delete_bin(wtk_consist_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t *)cfg->hook);
}
