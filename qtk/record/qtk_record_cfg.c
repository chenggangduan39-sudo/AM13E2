#include "qtk_record_cfg.h"

qtk_record_cfg_t* qtk_record_cfg_new()
{
	qtk_record_cfg_t*cfg;

	cfg = wtk_malloc(sizeof(*cfg));
	qtk_record_cfg_init(cfg);
	return cfg;
}

void qtk_record_cfg_delete(qtk_record_cfg_t *cfg)
{
	qtk_record_cfg_clean(cfg);
	wtk_free(cfg);
}

int qtk_record_cfg_init(qtk_record_cfg_t *cfg)
{
    cfg->snd_name = "default";
	cfg->channel = 10;
	cfg->skip_channels = NULL;
	cfg->nskip = 0;
	cfg->sample_rate = 16000;
	cfg->bytes_per_sample = 2;
	cfg->buf_time = 20;
#if defined(USE_802A) || defined(USE_BMC)
	cfg->mic_gain = 8;
    cfg->cb_gain = 8;
#else
	cfg->mic_gain = 196;
    cfg->cb_gain = 15;
#endif
    cfg->use_gain_set = 0;
	cfg->use_log_ori_audio = 1;
	cfg->use_get_card = 0;

	return 0;
}

int qtk_record_cfg_clean(qtk_record_cfg_t *cfg)
{
	if(cfg->skip_channels) {
		wtk_free(cfg->skip_channels);
	}
	return 0;
}

int qtk_record_cfg_update_local(qtk_record_cfg_t *cfg,wtk_local_cfg_t *main)
{
    wtk_string_t *v;
	wtk_array_t *a;
	int i;

	a = wtk_local_cfg_find_array_s(main,"skip_channels");
	if(a) {
		cfg->skip_channels = (int*)wtk_malloc(sizeof(int) * a->nslot);
		cfg->nskip = a->nslot;

		for(i=0;i < a->nslot; ++i) {
			v=((wtk_string_t**)a->slot)[i];
			cfg->skip_channels[i] = wtk_str_atoi(v->data,v->len);
		}
	}
	wtk_local_cfg_update_cfg_str(main,cfg,snd_name,v);
	wtk_local_cfg_update_cfg_i(main,cfg,channel,v);
	wtk_local_cfg_update_cfg_i(main,cfg,sample_rate,v);
	wtk_local_cfg_update_cfg_i(main,cfg,bytes_per_sample,v);
	wtk_local_cfg_update_cfg_i(main,cfg,buf_time,v);
	wtk_local_cfg_update_cfg_i(main,cfg,mic_gain,v);
    wtk_local_cfg_update_cfg_i(main,cfg,cb_gain,v);
    wtk_local_cfg_update_cfg_b(main,cfg,use_gain_set,v);
	wtk_local_cfg_update_cfg_b(main,cfg,use_get_card,v);
    return 0;
}

int qtk_record_cfg_update(qtk_record_cfg_t *cfg)
{
    return 0;
}
