#include "qtk_player_cfg.h" 

int qtk_player_cfg_init(qtk_player_cfg_t *cfg)
{
	cfg->buf_time = 8;
	cfg->period_time = 8;
	cfg->snd_name = "default";
	cfg->use_for_bfio = 1;
	cfg->use_uac = 0;
	cfg->channel = 2;
	cfg->sample_rate = 64000;
	cfg->bytes_per_sample = 4;
	cfg->start_time=-1;
	cfg->stop_time=-1;
	cfg->silence_time=-1;
	cfg->avail_time=-1;

	cfg->max_write_fail_times = 3;

	cfg->use_set_volume=0;
	cfg->volume=50;
	cfg->device_number =  0;

	return 0;
}

int qtk_player_cfg_clean(qtk_player_cfg_t *cfg)
{
	return 0;
}

int qtk_player_cfg_update_local(qtk_player_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,snd_name,v);
	
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sample_rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,bytes_per_sample,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,buf_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,period_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,volume,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,start_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,stop_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,silence_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,avail_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,device_number,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_write_fail_times,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_for_bfio,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_uac,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_set_volume,v);
	return 0;
}

int qtk_player_cfg_update(qtk_player_cfg_t *cfg)
{
#ifdef USE_SLB
	char name[128]={0};

	if(cfg->use_set_volume)
	{
		snprintf(name, 128, "amixer -c 1 cset numid=1,iface=MIXER,name='Master Playback Volume' %d",cfg->volume);
		wtk_debug("%s\n",name);
		system(name);
	}

#endif
	return 0;
}
