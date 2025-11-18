#include "qtk_mqform2_mod_cfg.h"

int qtk_mqform2_mod_cfg_init(qtk_mqform2_mod_cfg_t *cfg)
{
    qtk_recorder_cfg_init(&cfg->rcd);
#ifndef __ANDROID__
	qtk_player_cfg_init(&cfg->usbaudio);
	qtk_player_cfg_init(&cfg->lineout);
#endif
#ifdef USE_FOR_DEV
	qtk_led_cfg_init(&cfg->led);
#endif
    wtk_string_set(&cfg->soundscreen_fn, 0, 0);
    wtk_string_set(&cfg->eqform_fn, 0, 0);
    wtk_string_set(&cfg->aec_fn, 0, 0);
    wtk_string_set(&cfg->beamnet_fn, 0, 0);
    wtk_string_set(&cfg->beamnet2_fn, 0, 0);
	wtk_string_set(&cfg->cache_path, 0, 0);
    wtk_string_set(&cfg->uart_fn, 0, 0);
    cfg->echo_shift = 1.0;
    cfg->audio_type = 1;
    cfg->audio2_type = -1;
	cfg->use_lineout = 0;
	cfg->use_usbaudio = 0;
    cfg->use_uart = 0;
    cfg->use_aec = 0;
	cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_olddevice = 1;
    cfg->max_output_length=50;
    cfg->out_source_channel = 0;
    return 0;
}

int qtk_mqform2_mod_cfg_clean(qtk_mqform2_mod_cfg_t *cfg)
{
    qtk_recorder_cfg_clean(&cfg->rcd);
#ifndef __ANDROID__
	qtk_player_cfg_clean(&cfg->usbaudio);
	qtk_player_cfg_clean(&cfg->lineout);
#endif
#ifdef USE_FOR_DEV
	qtk_led_cfg_clean(&cfg->led);
#endif
    return 0;
}

int qtk_mqform2_mod_cfg_update_local(qtk_mqform2_mod_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_string_v(main, cfg, soundscreen_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, eqform_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, beamnet_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, beamnet2_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, aec_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, uart_fn, v);
    wtk_local_cfg_update_cfg_f(main, cfg, echo_shift, v);
    wtk_local_cfg_update_cfg_i(main, cfg, audio_type, v);
    wtk_local_cfg_update_cfg_i(main, cfg, audio2_type, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_output_length, v);
    wtk_local_cfg_update_cfg_i(main, cfg, out_source_channel, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_lineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_usbaudio, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_uart, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_aec, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_olddevice, v);
    lc = wtk_local_cfg_find_lc_s(main, "rcd");
    if(lc){
        qtk_recorder_cfg_update_local(&cfg->rcd, lc);
    }
#ifndef __ANDROID__
    lc = wtk_local_cfg_find_lc_s(main, "usbaudio");
    if(lc){
        qtk_player_cfg_update_local(&cfg->usbaudio, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "lineout");
    if(lc){
        qtk_player_cfg_update_local(&cfg->lineout, lc);
    }
#endif
#ifdef USE_FOR_DEV
    lc = wtk_local_cfg_find_lc_s(main, "led");
    if(lc){
        qtk_led_cfg_update_local(&cfg->led, lc);
    }
#endif
    return 0;
}

int qtk_mqform2_mod_cfg_update(qtk_mqform2_mod_cfg_t *cfg)
{
    qtk_recorder_cfg_update(&cfg->rcd);
#ifndef __ANDROID__
	qtk_player_cfg_update(&cfg->usbaudio);
	qtk_player_cfg_update(&cfg->lineout);
#endif
#ifdef USE_FOR_DEV
	qtk_led_cfg_update(&cfg->led);
#endif
	// if(cfg->use_lineout == 0 && cfg->use_usbaudio == 0){
	// 	cfg->use_usbaudio = 1;
	// }
    return 0;
}
