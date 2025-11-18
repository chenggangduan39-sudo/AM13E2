#include "qtk_mod_rp_cfg.h"

int qtk_mod_rp_cfg_init(qtk_mod_rp_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_init(&cfg->rcd);
	qtk_play_cfg_init(&cfg->usbaudio);
	qtk_play_cfg_init(&cfg->lineout);
#endif
	wtk_string_set(&cfg->cache_path, 0, 0);
    cfg->use_lineout = 0;
	cfg->use_usbaudio = 0;
    cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_log_wav = 0;
    cfg->use_resample = 0;

    return 0;
}

int qtk_mod_rp_cfg_clean(qtk_mod_rp_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_clean(&cfg->rcd);
	qtk_play_cfg_clean(&cfg->usbaudio);
	qtk_play_cfg_clean(&cfg->lineout);
#endif

    return 0;
}

int qtk_mod_rp_cfg_update_local(qtk_mod_rp_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_lineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_usbaudio, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_resample, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log_wav, v);

#ifndef __ANDROID__
    lc = wtk_local_cfg_find_lc_s(main, "rcd");
    if(lc){
        qtk_record_cfg_update_local(&cfg->rcd, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "usbaudio");
    if(lc){
        qtk_play_cfg_update_local(&cfg->usbaudio, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "lineout");
    if(lc){
        qtk_play_cfg_update_local(&cfg->lineout, lc);
    }
#endif

    return 0;
}

int qtk_mod_rp_cfg_update(qtk_mod_rp_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_update(&cfg->rcd);
	qtk_play_cfg_update(&cfg->usbaudio);
	qtk_play_cfg_update(&cfg->lineout);
#endif

	// if(cfg->use_lineout == 0 && cfg->use_usbaudio == 0){
	// 	cfg->use_usbaudio = 1;
	// }
    return 0;
}
