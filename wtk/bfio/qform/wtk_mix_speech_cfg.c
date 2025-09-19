#include "wtk_mix_speech_cfg.h"

int wtk_mix_speech_cfg_init(wtk_mix_speech_cfg_t *cfg)
{
	cfg->channel=0;
	cfg->nmicchannel=0;
	cfg->mic_channel=NULL;
    cfg->wins=320;
	cfg->rate=16000;

	cfg->main_cfg=NULL;
	cfg->mbin_cfg=NULL;

	cfg->mix_type=0;

	cfg->max_1 = 32000;
	cfg->min_1 = -32000;
	cfg->max_2 = 30000;
	cfg->min_2 = -30000;
	cfg->f_win = 128;
	cfg->mic_scale = NULL;
	cfg->scale_cnt = 20;
	cfg->scale_step = NULL;
	cfg->micenr_thresh=300;
	cfg->micenr_cnt=10;

	return 0;
}

int wtk_mix_speech_cfg_clean(wtk_mix_speech_cfg_t *cfg)
{
	if(cfg->mic_channel)
	{
		wtk_free(cfg->mic_channel);
	}
	if(cfg->mic_scale){
		wtk_free(cfg->mic_scale);
	}
	if(cfg->scale_step){
		wtk_free(cfg->scale_step);
	}
	return 0;
}

int wtk_mix_speech_cfg_update_local(wtk_mix_speech_cfg_t *cfg,wtk_local_cfg_t *m)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	wtk_array_t *a;
	int i;
	int ret;
	float scale_sum;
	float scale;

	lc=m;
	wtk_local_cfg_update_cfg_i(lc,cfg,wins,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,max_1,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_1,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,min_2,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,f_win,v);

	wtk_local_cfg_update_cfg_i(lc,cfg,mix_type,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,scale_cnt,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,micenr_thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,micenr_cnt,v);

	a=wtk_local_cfg_find_array_s(lc,"mic_channel");
	if(a)
	{
		cfg->mic_channel=(int*)wtk_malloc(sizeof(int)*a->nslot);
		cfg->nmicchannel=a->nslot;
		for(i=0;i<a->nslot;++i)
		{
			v=((wtk_string_t**)a->slot)[i];
			cfg->mic_channel[i]=wtk_str_atoi(v->data,v->len);
		}
	}

	a=wtk_local_cfg_find_array_s(lc,"mic_scale");
	if(a)
	{
		cfg->mic_scale = (float *)wtk_malloc(sizeof(float)*cfg->channel);
		cfg->scale_step = (float *)wtk_malloc(sizeof(float)*cfg->channel);
		for(i=0;i<cfg->channel;++i){
			cfg->mic_scale[i] = 1.0;
		}
		cfg->nmic_scale=a->nslot;
		for(i = 0; i < min(cfg->channel, cfg->nmic_scale); ++i)
		{
			v = ((wtk_string_t**)a->slot)[i];
			cfg->mic_scale[i] = wtk_str_atof(v->data,v->len);
		}
		scale_sum = 0;
		for(i=0;i<cfg->channel;++i){
			scale_sum += cfg->mic_scale[i];
		}
		scale = cfg->channel * 1.0/scale_sum;
		for(i=0;i<cfg->channel;++i){
			cfg->mic_scale[i] *= scale;
			cfg->scale_step[i] = fabs(cfg->mic_scale[i] - 1.0) * 1.0/ cfg->scale_cnt;
			// printf("%f\n", cfg->scale_step[i]);
		}
	}else{
		cfg->mic_scale = (float *)wtk_malloc(sizeof(float)*cfg->channel);
		cfg->scale_step = (float *)wtk_malloc(sizeof(float)*cfg->channel);
		for(i=0;i<cfg->channel;++i){
			cfg->mic_scale[i] = 1.0;
			cfg->scale_step[i] = 0.0;
		}
		goto end;
	}
	// exit(0);
	ret=0;
end:
	return ret;
}

int wtk_mix_speech_cfg_update(wtk_mix_speech_cfg_t *cfg)
{
	int ret;

	if(cfg->channel < cfg->nmicchannel){
		cfg->channel = cfg->nmicchannel;
	}
	ret=0;
	return ret;
}

int wtk_mix_speech_cfg_update2(wtk_mix_speech_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return wtk_mix_speech_cfg_update(cfg);
}

wtk_mix_speech_cfg_t* wtk_mix_speech_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_mix_speech_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_mix_speech_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(wtk_mix_speech_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_mix_speech_cfg_delete(wtk_mix_speech_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_mix_speech_cfg_t* wtk_mix_speech_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_mix_speech_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_mix_speech_cfg,fn,"./cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(wtk_mix_speech_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void wtk_mix_speech_cfg_delete_bin(wtk_mix_speech_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

// channel 总数量  main_scale 主通道增益,无增益则为1
void wtk_mix_speech_cfg_set_channel(wtk_mix_speech_cfg_t *cfg, int channel, float main_scale)
{
	int i;
	float scale_sum;
	float scale;

	wtk_mix_speech_cfg_clean(cfg);
	cfg->channel = channel;
	cfg->nmicchannel = channel;
	cfg->mic_channel = (int *)wtk_malloc(sizeof(int)*channel);
	cfg->mic_scale = (float *)wtk_malloc(sizeof(float)*channel);
	cfg->scale_step = (float *)wtk_malloc(sizeof(float)*channel);

	for(i=0;i<channel;++i){
		cfg->mic_channel[i] = i;
		cfg->mic_scale[i] = 1.0;
	}
	cfg->mic_scale[0] = main_scale;
	scale_sum = 0;
	for(i=0;i<channel;++i){
		scale_sum += cfg->mic_scale[i];
	}
	scale = channel * 1.0/scale_sum;
	for(i=0;i<channel;++i){
		cfg->mic_scale[i] *= scale;
		cfg->scale_step[i] = fabs(cfg->mic_scale[i] - 1.0) * 1.0/ cfg->scale_cnt;
	}
	// for(i=0;i<channel;++i){
	// 	printf("%d %f %f\n", cfg->mic_channel[i], cfg->mic_scale[i], cfg->scale_step[i]);
	// }
}
