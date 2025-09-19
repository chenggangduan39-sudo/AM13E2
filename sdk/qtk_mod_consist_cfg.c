#include "qtk_mod_consist_cfg.h"

int qtk_mod_consist_cfg_init(qtk_mod_consist_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_init(&cfg->rcd);
    qtk_play_cfg_init(&cfg->play);
#endif

	wtk_string_set(&cfg->cache_path, 0, 0);
    wtk_string_set(&cfg->consist_fn, 0, 0);
    
    cfg->consist_cfg = NULL;
	cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_consist_bin = 0;
    cfg->use_xcorr = 0;
    cfg->use_bfio = 0;
    cfg->mode=0;

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

    cfg->skip_head_tm = 200;

    return 0;
}

int qtk_mod_consist_cfg_clean(qtk_mod_consist_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_clean(&cfg->rcd);
    qtk_play_cfg_clean(&cfg->play);
#endif

    if(cfg->consist_cfg)
    {
        cfg->use_consist_bin ? wtk_consist_cfg_delete_bin(cfg->consist_cfg) : wtk_consist_cfg_delete(cfg->consist_cfg);
    }
    return 0;
}

int qtk_mod_consist_cfg_update_local(qtk_mod_consist_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, consist_fn, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_consist_bin, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_xcorr, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_bfio, v);
    wtk_local_cfg_update_cfg_i(main, cfg, mode, v);

    wtk_local_cfg_update_cfg_i(main, cfg, skip_head_tm, v);

	wtk_local_cfg_update_cfg_f(main, cfg, eq_offset, v);
	wtk_local_cfg_update_cfg_f(main, cfg, nil_er, v);

	wtk_local_cfg_update_cfg_f(main, cfg, mic_corr_er, v);
	wtk_local_cfg_update_cfg_f(main, cfg, mic_corr_aver, v);
	wtk_local_cfg_update_cfg_f(main, cfg, mic_energy_er, v);
	wtk_local_cfg_update_cfg_f(main, cfg, mic_energy_aver, v);

	wtk_local_cfg_update_cfg_f(main, cfg, spk_corr_er, v);
	wtk_local_cfg_update_cfg_f(main, cfg, spk_corr_aver, v);
	wtk_local_cfg_update_cfg_f(main, cfg, spk_energy_er, v);
	wtk_local_cfg_update_cfg_f(main, cfg, spk_energy_aver, v);

#ifndef __ANDROID__
    lc = wtk_local_cfg_find_lc_s(main, "rcd");
    if(lc){
        qtk_record_cfg_update_local(&cfg->rcd, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "play");
    if(lc){
        qtk_play_cfg_update_local(&cfg->play, lc);
    }
#endif
    return 0;
}

int qtk_mod_consist_cfg_update(qtk_mod_consist_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_update(&cfg->rcd);
    qtk_play_cfg_update(&cfg->play);
#endif
	
    cfg->consist_cfg = cfg->use_consist_bin ? wtk_consist_cfg_new_bin(cfg->consist_fn.data) : wtk_consist_cfg_new(cfg->consist_fn.data);
    if(cfg->consist_cfg)
    {
        cfg->consist_cfg->use_xcorr=cfg->use_xcorr;

        cfg->consist_cfg->eq_offset=cfg->eq_offset;
        cfg->consist_cfg->nil_er=cfg->nil_er;
        cfg->consist_cfg->mic_corr_er=cfg->mic_corr_er;
        cfg->consist_cfg->mic_corr_aver=cfg->mic_corr_aver;
        cfg->consist_cfg->mic_energy_er=cfg->mic_energy_er;
        cfg->consist_cfg->mic_energy_aver=cfg->mic_energy_aver;

        cfg->consist_cfg->spk_corr_er=cfg->spk_corr_er;
        cfg->consist_cfg->spk_corr_aver=cfg->spk_corr_aver;
        cfg->consist_cfg->spk_energy_er=cfg->spk_energy_er;
        cfg->consist_cfg->spk_energy_aver=cfg->spk_energy_aver;
    }

    printf("consist arg:\n");
	printf("eq_offset       = %f\n",cfg->consist_cfg->eq_offset);
	printf("nil_er          = %f\n",cfg->consist_cfg->nil_er);
	printf("mic_corr_er     = %f\n",cfg->consist_cfg->mic_corr_er);
	printf("mic_corr_aver   = %f\n",cfg->consist_cfg->mic_corr_aver);
	printf("mic_energy_er   = %f\n",cfg->consist_cfg->mic_energy_er);
	printf("mic_energy_aver = %f\n",cfg->consist_cfg->mic_energy_aver);
	printf("spk_corr_er     = %f\n",cfg->consist_cfg->spk_corr_er);
	printf("spk_corr_aver   = %f\n",cfg->consist_cfg->spk_corr_aver);
	printf("spk_energy_er   = %f\n",cfg->consist_cfg->spk_energy_er);
	printf("spk_energy_aver = %f\n",cfg->consist_cfg->spk_energy_aver);

    return 0;
}
