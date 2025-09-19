#include "qtk_mgainnet_mod_cfg.h"

int qtk_mgainnet_mod_cfg_init(qtk_mgainnet_mod_cfg_t *cfg)
{
    qtk_recorder_cfg_init(&cfg->rcd);
	qtk_player_cfg_init(&cfg->usbaudio);
	qtk_player_cfg_init(&cfg->lineout);
#ifdef USE_FOR_DEV
	qtk_led_cfg_init(&cfg->led);
#endif
    wtk_string_set(&cfg->cache_path, 0, 0);
    wtk_string_set(&cfg->uart_fn, 0, 0);
    cfg->engine_params = NULL;
    cfg->swap_buf = NULL;

    cfg->echo_shift = 1.0;
	cfg->use_lineout = 0;
	cfg->use_usbaudio = 0;
    cfg->use_uart = 0;
	cfg->debug = 0;
	cfg->use_log = 0;
    cfg->use_log_wav=0;
    cfg->use_resample=0;
    return 0;
}

int qtk_mgainnet_mod_cfg_clean(qtk_mgainnet_mod_cfg_t *cfg)
{
    if(cfg->swap_buf)
    {
        wtk_strbuf_delete(cfg->swap_buf);
    }
    qtk_recorder_cfg_clean(&cfg->rcd);
	qtk_player_cfg_clean(&cfg->usbaudio);
	qtk_player_cfg_clean(&cfg->lineout);

#ifdef USE_FOR_DEV
	qtk_led_cfg_clean(&cfg->led);
#endif
    return 0;
}

int qtk_mgainnet_mod_cfg_update_local(qtk_mgainnet_mod_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_str(main, cfg, engine_params, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, cache_path, v);
    wtk_local_cfg_update_cfg_string_v(main, cfg, uart_fn, v);
    wtk_local_cfg_update_cfg_f(main, cfg, echo_shift, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_lineout, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_usbaudio, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_uart, v);
    wtk_local_cfg_update_cfg_b(main, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_log_wav, v);
    wtk_local_cfg_update_cfg_b(main, cfg, use_resample, v);

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
#ifdef USE_FOR_DEV
    lc = wtk_local_cfg_find_lc_s(main, "led");
    if(lc){
        qtk_led_cfg_update_local(&cfg->led, lc);
    }
#endif

	if(cfg->engine_params)
	{
		v=wtk_local_cfg_find_string_s(main,"pwd");
		cfg->swap_buf = wtk_strbuf_new(1024,1);
		qtk_module_replace_pwd(cfg->swap_buf,cfg->engine_params,strlen(cfg->engine_params),v);
		cfg->engine_params=cfg->swap_buf->data;
	}

    return 0;
}

int qtk_mgainnet_mod_cfg_update(qtk_mgainnet_mod_cfg_t *cfg)
{
#ifndef __ANDROID__
    qtk_recorder_cfg_update(&cfg->rcd);
	qtk_player_cfg_update(&cfg->usbaudio);
	qtk_player_cfg_update(&cfg->lineout);
#endif
#ifdef USE_FOR_DEV
	qtk_led_cfg_update(&cfg->led);
#endif
    return 0;
}
