#include "qtk_mod_tt2t_cfg.h"

int qtk_mod_tt2t_cfg_init(qtk_mod_tt2t_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_init(&cfg->rcd);
    qtk_play_cfg_init(&cfg->lineout);
#endif
    wtk_string_set(&cfg->tt2t_fn, 0, 0);
    wtk_string_set(&cfg->dett2t_fn, 0, 0);
	wtk_string_set(&cfg->cache_path, 0, 0);
    wtk_string_set(&cfg->uart_fn, 0, 0);
    cfg->audio_type = 1;
    cfg->use_uart = 0;
    cfg->use_lineout = 0;
	cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_log_wav = 0;
    cfg->max_output_length = 30;
    cfg->restart_time = 3000.0;
    cfg->sleep_time = 3600*6;

    cfg->mic_shift = 1.0;
    cfg->skip_head_tm = 200;
    cfg->use_bin = 0;
    cfg->record_time = 5000;
    
    return 0;
}

int qtk_mod_tt2t_cfg_clean(qtk_mod_tt2t_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_clean(&cfg->rcd);
    qtk_play_cfg_clean(&cfg->lineout);
#endif
    return 0;
}

int qtk_mod_tt2t_cfg_update_local(qtk_mod_tt2t_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_string_v(main, cfg, tt2t_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, dett2t_fn, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, uart_fn, v);
    wtk_local_cfg_update_cfg_f(main, cfg, mic_shift, v);
    wtk_local_cfg_update_cfg_i(main, cfg, audio_type, v);
    wtk_local_cfg_update_cfg_i(main, cfg, max_output_length, v);
    wtk_local_cfg_update_cfg_i(main, cfg, sleep_time, v);
    wtk_local_cfg_update_cfg_i(main, cfg, skip_head_tm, v);
    wtk_local_cfg_update_cfg_i(main, cfg, record_time, v);
    wtk_local_cfg_update_cfg_f(main, cfg, restart_time, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_uart, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_bin, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_lineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log_wav, v);

#ifndef __ANDROID__
    lc = wtk_local_cfg_find_lc_s(main, "rcd");
    if(lc){
        qtk_record_cfg_update_local(&cfg->rcd, lc);
    }
    lc = wtk_local_cfg_find_lc_s(main, "lineout");
    if(lc){
        qtk_play_cfg_update_local(&cfg->lineout, lc);
    }
#endif
    return 0;
}

int qtk_mod_tt2t_cfg_update(qtk_mod_tt2t_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_record_cfg_update(&cfg->rcd);
    qtk_play_cfg_update(&cfg->lineout);
#endif
    return 0;
}
