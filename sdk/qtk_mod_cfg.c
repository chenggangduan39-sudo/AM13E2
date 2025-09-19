#include "qtk_mod_cfg.h"

int qtk_mod_cfg_init(qtk_mod_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_init(&cfg->rcd);
	qtk_play_cfg_init(&cfg->usbaudio);
	qtk_play_cfg_init(&cfg->lineout);
#endif
#ifdef USE_LED
	qtk_led_cfg_init(&cfg->led);
#endif
    wtk_string_set(&cfg->vboxebf_fn, 0, 0);
    wtk_string_set(&cfg->qform_fn, 0, 0);
    wtk_string_set(&cfg->soundscreen_fn, 0, 0);
    wtk_string_set(&cfg->kws_fn, 0, 0);
    wtk_string_set(&cfg->enroll_fn, 0, 0);
	wtk_string_set(&cfg->cache_path, 0, 0);
    wtk_string_set(&cfg->uart_fn, 0, 0);
    cfg->audio_type = 1;
    cfg->audio2_type = -1;
    cfg->audio3_type = -1;
	cfg->use_lineout = 0;
	cfg->use_usbaudio = 0;
    cfg->use_uart = 0;
	cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_log_wav = 0;
    cfg->max_add_count = 160;
    cfg->max_play_count = 100;
    cfg->max_output_length = 30;
    cfg->restart_time = 3000.0;
    cfg->sleep_time = 3600*6;
    cfg->use_resample = 0;
    cfg->resample_rate = 16000;

    cfg->mic_shift = 1.0;
    cfg->spk_shift = 1.0;
    cfg->echo_shift = 1.0;
    cfg->lineout_shift = 1.0;
    cfg->spknum = 2;
    cfg->use_in_resample=0;

    cfg->skip_head_tm = 200;

    return 0;
}

int qtk_mod_cfg_clean(qtk_mod_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_clean(&cfg->rcd);
	qtk_play_cfg_clean(&cfg->usbaudio);
	qtk_play_cfg_clean(&cfg->lineout);
#endif
#ifdef USE_LED
	qtk_led_cfg_clean(&cfg->led);
#endif
    return 0;
}

int qtk_mod_cfg_update_local(qtk_mod_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_string_v(main, cfg, vboxebf_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, qform_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, soundscreen_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, kws_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, enroll_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, uart_fn, v);
    wtk_local_cfg_update_cfg_f(main, cfg, mic_shift, v);
    wtk_local_cfg_update_cfg_f(main, cfg, spk_shift, v);
    wtk_local_cfg_update_cfg_f(main, cfg, echo_shift, v);
    wtk_local_cfg_update_cfg_f(main, cfg, lineout_shift, v);
    wtk_local_cfg_update_cfg_f(main, cfg, restart_time, v);
    wtk_local_cfg_update_cfg_i(main, cfg, audio_type, v);
    wtk_local_cfg_update_cfg_i(main, cfg, audio2_type, v);
    wtk_local_cfg_update_cfg_i(main, cfg, audio3_type, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_add_count, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_play_count, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_output_length, v);
    wtk_local_cfg_update_cfg_i(main, cfg, sleep_time, v);
    wtk_local_cfg_update_cfg_i(main, cfg, resample_rate, v);
    wtk_local_cfg_update_cfg_i(main, cfg, spknum, v);
    wtk_local_cfg_update_cfg_i(main, cfg, skip_head_tm, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_lineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_usbaudio, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_uart, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log_wav, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_resample, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_in_resample, v);

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
#ifdef USE_LED
    lc = wtk_local_cfg_find_lc_s(main, "led");
    if(lc){
        qtk_led_cfg_update_local(&cfg->led, lc);
    }
#endif
    return 0;
}

int qtk_mod_cfg_update(qtk_mod_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_update(&cfg->rcd);
	qtk_play_cfg_update(&cfg->usbaudio);
	qtk_play_cfg_update(&cfg->lineout);
#endif
#ifdef USE_LED
	qtk_led_cfg_update(&cfg->led);
#endif
	// if(cfg->use_lineout == 0 && cfg->use_usbaudio == 0){
	// 	cfg->use_usbaudio = 1;
	// }
    return 0;
}
