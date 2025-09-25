#include "qtk_mult_mod_cfg.h"

int qtk_mult_mod_cfg_init(qtk_mult_mod_cfg_t *cfg)
{
    qtk_recorder_cfg_init(&cfg->rcd);
	qtk_player_cfg_init(&cfg->usbaudio);
	qtk_player_cfg_init(&cfg->lineout);

    wtk_string_set(&cfg->vboxebf_fn, 0, 0);
    wtk_string_set(&cfg->cache_path, 0, 0);

    cfg->echo_shift = 1.0;
    cfg->audio_type = 1;
	cfg->use_lineout = 0;
	cfg->use_usbaudio = 0;
	cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_log_wav = 0;
    cfg->use_resample = 0;
    cfg->resample_rate = 16000;
    cfg->mic_shift = 1.0;
    cfg->spk_shift = 1.0;
    cfg->spknum = 2;
    cfg->use_in_resample=0;
    cfg->skip_head_tm = 200;

    cfg->max_output_length = 30;
    cfg->max_add_count = 20;

    return 0;
}

int qtk_mult_mod_cfg_clean(qtk_mult_mod_cfg_t *cfg)
{
    qtk_recorder_cfg_clean(&cfg->rcd);
	qtk_player_cfg_clean(&cfg->usbaudio);
	qtk_player_cfg_clean(&cfg->lineout);

    return 0;
}

int qtk_mult_mod_cfg_update_local(qtk_mult_mod_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_string_v(main, cfg, vboxebf_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_f(main, cfg, mic_shift, v);
    wtk_local_cfg_update_cfg_f(main, cfg, spk_shift, v);
    wtk_local_cfg_update_cfg_f(main, cfg, echo_shift, v);
    wtk_local_cfg_update_cfg_i(main, cfg, audio_type, v);
    wtk_local_cfg_update_cfg_i(main, cfg, resample_rate, v);
    wtk_local_cfg_update_cfg_i(main, cfg, spknum, v);
    wtk_local_cfg_update_cfg_i(main, cfg, skip_head_tm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_output_length, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_add_count, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_lineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_usbaudio, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log_wav, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_resample, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_in_resample, v);

    lc = wtk_local_cfg_find_lc_s(main, "rcd");
    if(lc){
        qtk_recorder_cfg_update_local(&cfg->rcd, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "usbaudio");
    if(lc){
        qtk_player_cfg_update_local(&cfg->usbaudio, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "lineout");
    if(lc){
        qtk_player_cfg_update_local(&cfg->lineout, lc);
    }

    return 0;
}

int qtk_mult_mod_cfg_update(qtk_mult_mod_cfg_t *cfg)
{
    qtk_recorder_cfg_update(&cfg->rcd);
	qtk_player_cfg_update(&cfg->usbaudio);
	qtk_player_cfg_update(&cfg->lineout);

    return 0;
}
