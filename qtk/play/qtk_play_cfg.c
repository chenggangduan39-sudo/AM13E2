
#include "qtk_play_cfg.h"

int qtk_play_cfg_init(qtk_play_cfg_t *cfg)
{
	wtk_string_set_s(&cfg->snd_name,"default");
	cfg->channel = 1;
	cfg->sample_rate = 16000;
	cfg->bytes_per_sample = 2;
	cfg->buf_time = 128;
	cfg->period_time=32;
	cfg->start_time=-1;
	cfg->stop_time=-1;
	cfg->silence_time=-1;
	cfg->avail_time=-1;
	cfg->use_uac = 0;
	cfg->use_get_soundcard = 1;
	cfg->volume=50;
	cfg->use_set_volume = 0;

	return 0;
}

int qtk_play_cfg_clean(qtk_play_cfg_t *cfg)
{
	return 0;
}

int qtk_play_cfg_update_local(qtk_play_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_string_t *v;

	wtk_local_cfg_update_cfg_string_v(main, cfg, snd_name, v);
	
	wtk_local_cfg_update_cfg_i(main,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(main,cfg,sample_rate,v);
	wtk_local_cfg_update_cfg_i(main,cfg,bytes_per_sample,v);
	wtk_local_cfg_update_cfg_i(main,cfg,buf_time,v);
	wtk_local_cfg_update_cfg_i(main,cfg,period_time,v);
	wtk_local_cfg_update_cfg_i(main,cfg,start_time,v);
	wtk_local_cfg_update_cfg_i(main,cfg,stop_time,v);
	wtk_local_cfg_update_cfg_i(main,cfg,silence_time,v);
	wtk_local_cfg_update_cfg_i(main,cfg,avail_time,v);
	wtk_local_cfg_update_cfg_i(main,cfg,volume,v);
	wtk_local_cfg_update_cfg_b(main,cfg,use_uac,v);
	wtk_local_cfg_update_cfg_b(main,cfg,use_get_soundcard,v);
	wtk_local_cfg_update_cfg_b(main,cfg,use_set_volume,v);

    return 0;
}

int qtk_play_cfg_update(qtk_play_cfg_t *cfg)
{
#ifdef USE_SLB
	char name[128]={0};

	if(cfg->use_set_volume)
	{
		snprintf(name, 128, "amixer -c 1 cset numid=1,iface=MIXER,name='Master Playback Volume' %d",cfg->volume);

		system(name);
	}

#endif
    return 0;
}
