#include "qtk_audio_cfg.h" 

int qtk_audio_cfg_init(qtk_audio_cfg_t *cfg)
{
	cfg->use_audio=0;
	cfg->use_daemon=0;
	cfg->use_sys_play=0;
	cfg->use_sys_record=0;
	cfg->debug=0;
	cfg->use_dc = 0;
	cfg->use_zero=0;
	cfg->zero_wait_time = 200;
	cfg->daemon_wait_time = 2000;
	qtk_usb_cfg_init(&(cfg->usb));
	qtk_player_cfg_init(&(cfg->player));
	qtk_recorder_cfg_init(&(cfg->recorder));
	qtk_auout_cfg_init(&(cfg->auout));
	qtk_auin_cfg_init(&(cfg->auin));
	qtk_audio_daemon_cfg_init(&(cfg->daemon));
	return 0;
}

int qtk_audio_cfg_clean(qtk_audio_cfg_t *cfg)
{
	qtk_usb_cfg_clean(&(cfg->usb));
	qtk_player_cfg_clean(&(cfg->player));
	qtk_recorder_cfg_clean(&(cfg->recorder));
	qtk_auout_cfg_clean(&(cfg->auout));
	qtk_auin_cfg_clean(&(cfg->auin));
	qtk_audio_daemon_cfg_clean(&(cfg->daemon));
	return 0;
}

int qtk_audio_cfg_update_local(qtk_audio_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	lc=main;
	wtk_local_cfg_update_cfg_i(lc,cfg,zero_wait_time,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_audio,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_daemon,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,daemon_wait_time,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sys_play,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sys_record,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_zero,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dc,v);
	if(cfg->use_audio)
	{
		lc=wtk_local_cfg_find_lc_s(main,"player");
		if(lc)
		{
			qtk_player_cfg_update_local(&(cfg->player),lc);
		}
		lc=wtk_local_cfg_find_lc_s(main,"recorder");
		if(lc)
		{
			qtk_recorder_cfg_update_local(&(cfg->recorder),lc);
		}
	} else {
		lc=wtk_local_cfg_find_lc_s(main,"usb");
		if(lc){
			qtk_usb_cfg_update_local(&(cfg->usb),lc);
		}
	}
	lc=wtk_local_cfg_find_lc_s(main,"auin");
	if(lc)
	{
		qtk_auin_cfg_update_local(&(cfg->auin),lc);
	}
	lc=wtk_local_cfg_find_lc_s(main,"auout");
	if(lc)
	{
		qtk_auout_cfg_update_local(&(cfg->auout),lc);
	}

	lc=wtk_local_cfg_find_lc_s(main,"daemon");
	if(lc)
	{
		qtk_audio_daemon_cfg_update_local(&(cfg->daemon),lc);
	}
	return 0;
}

int qtk_audio_cfg_update(qtk_audio_cfg_t *cfg)
{
	if(cfg->use_audio) {
		qtk_player_cfg_update(&(cfg->player));
		qtk_recorder_cfg_update(&(cfg->recorder));
	} else {
		qtk_usb_cfg_update(&(cfg->usb));
	}
	qtk_auout_cfg_update(&(cfg->auout));
	qtk_auin_cfg_update(&(cfg->auin));
	qtk_audio_daemon_cfg_update(&(cfg->daemon));
	return 0;
}
