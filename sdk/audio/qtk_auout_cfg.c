#include "qtk_auout_cfg.h" 

int qtk_auout_cfg_init(qtk_auout_cfg_t *cfg)
{
	// wtk_mp3dec_cfg_init(&cfg->mp3dec);
	// cfg->use_mp3dec = 0;

	wtk_pitch_cfg_init(&cfg->pitch);
	cfg->pitch_shift = 1.0f;
	cfg->pitch_shift_min = 0.2f;
	cfg->pitch_shift_max = 4.0f;
	cfg->pitch_shift_step = 0.2f;
	cfg->volume_shift = 1.0f;
	cfg->volume_shift_min = 0.2f;
	cfg->volume_shift_max = 4.0f;
	cfg->volume_shift_step = 0.2f;
	cfg->wait_time = 0;
	cfg->play_wait_end_time = 200;
	cfg->debug = 0;

	cfg->err_exit = 0;

	return 0;
}

int qtk_auout_cfg_clean(qtk_auout_cfg_t *cfg)
{
	// wtk_mp3dec_cfg_clean(&cfg->mp3dec);
	wtk_pitch_cfg_clean(&(cfg->pitch));
	return 0;
}

int qtk_auout_cfg_update_local(qtk_auout_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;

	lc = main;
	wtk_local_cfg_update_cfg_f(lc,cfg,pitch_shift,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pitch_shift_min,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pitch_shift_max,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,pitch_shift_step,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,volume_shift,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,volume_shift_min,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,volume_shift_max,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,volume_shift_step,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,wait_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,play_wait_end_time,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,err_exit,v);
	// wtk_local_cfg_update_cfg_b(lc,cfg,use_mp3dec,v);

	lc = wtk_local_cfg_find_lc_s(main,"pitch");
	if(lc) {
		wtk_pitch_cfg_update_local(&cfg->pitch,lc);
	}

	// if(cfg->use_mp3dec) {
	// 	lc = wtk_local_cfg_find_lc_s(main,"mp3dec");
	// 	if(lc) {
	// 		wtk_mp3dec_cfg_update_local(&cfg->mp3dec,lc);
	// 	}
	// }

	return 0;
}

int qtk_auout_cfg_update(qtk_auout_cfg_t *cfg)
{
	// wtk_mp3dec_cfg_update(&cfg->mp3dec);
	wtk_pitch_cfg_update(&cfg->pitch);
	return 0;
}
