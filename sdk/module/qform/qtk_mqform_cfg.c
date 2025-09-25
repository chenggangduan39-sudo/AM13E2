#include "qtk_mqform_cfg.h"

int qtk_mqform_cfg_init(qtk_mqform_cfg_t *cfg)
{
	qtk_recorder_cfg_init(&cfg->recorder_cfg);
	qtk_player_cfg_init(&cfg->player_cfg);
	cfg->sqform = NULL;
	cfg->swakeup_buf = NULL;
	cfg->debug = 0;
	cfg->use_recorder = 1;
	cfg->use_player = 1;
	cfg->use_sample = 1;
	cfg->echo_shift = 1.0f;
	cfg->use_dsp = 0;
	cfg->resample_rate = 16000;
	cfg->max_output_length = 30;
	cfg->out_channel = 1;
	cfg->use_log_wav = 0;
	return 0;
}
int qtk_mqform_cfg_clean(qtk_mqform_cfg_t *cfg)
{
	if(cfg->swakeup_buf){
		wtk_strbuf_delete(cfg->swakeup_buf);
	}
	qtk_recorder_cfg_clean(&cfg->recorder_cfg);
	qtk_player_cfg_clean(&cfg->player_cfg);
	return 0;
}
int qtk_mqform_cfg_update_local(qtk_mqform_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc = main;
	wtk_local_cfg_update_cfg_str(lc, cfg, sqform, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, debug, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_dsp, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_recorder, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_player, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_sample, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_log_wav, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shift, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, resample_rate, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, max_output_length, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, out_channel, v);

    lc = wtk_local_cfg_find_lc_s(main,"recorder");
    if(lc)
    {
        ret = qtk_recorder_cfg_update_local(&(cfg->recorder_cfg),lc);
        if(ret != 0) 
            return -1;
    }else{
        return -1;
    }
   	
	lc = wtk_local_cfg_find_lc_s(main,"player");
	if(lc){
		ret = qtk_player_cfg_update_local(&(cfg->player_cfg),lc);
		if(ret != 0)
			return -1;
	}

	if(cfg->sqform)
	{
		v=wtk_local_cfg_find_string_s(main,"pwd");
		cfg->swakeup_buf = wtk_strbuf_new(1024,1);
		qtk_module_replace_pwd(cfg->swakeup_buf,cfg->sqform,strlen(cfg->sqform),v);
		cfg->sqform=cfg->swakeup_buf->data;
	}
	return 0;
}
int qtk_mqform_cfg_update(qtk_mqform_cfg_t *cfg)
{
#ifndef OFFLINE_TEST
    qtk_recorder_cfg_update(&cfg->recorder_cfg);
    qtk_player_cfg_update(&cfg->player_cfg);
#endif

	return 0;
}
