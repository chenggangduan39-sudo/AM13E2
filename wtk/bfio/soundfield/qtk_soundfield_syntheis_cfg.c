#include "qtk_soundfield_syntheis_cfg.h" 

int qtk_soundfield_syntheis_cfg_init(qtk_soundfield_syntheis_cfg_t *cfg){
	cfg->hop_size = 256;
	cfg->fs = 16000;
	cfg->window = NULL;
	cfg->win_gain = NULL;
	cfg->N = 12;
	cfg->pw_angle = -25;
	cfg->array_spacing = 0.07;
	cfg->center[0] = 0;
	cfg->center[1] = 2;
	cfg->center[2] = 0;
	cfg->scale = 5;
	return 0;
}

int qtk_soundfield_syntheis_cfg_clean(qtk_soundfield_syntheis_cfg_t *cfg){
	if(cfg->window) {
        wtk_free(cfg->window);
    }
	if(cfg->win_gain){
		wtk_free(cfg->win_gain);
	}
	return 0;
}

int qtk_soundfield_syntheis_cfg_update_local(qtk_soundfield_syntheis_cfg_t *cfg,wtk_local_cfg_t *lc){
	wtk_string_t *v;
	int i;

	wtk_local_cfg_update_cfg_i(lc,cfg,hop_size,v)
	wtk_local_cfg_update_cfg_i(lc,cfg,fs,v)
	wtk_local_cfg_update_cfg_i(lc,cfg,N,v)
	wtk_local_cfg_update_cfg_f(lc,cfg,pw_angle,v)
	wtk_local_cfg_update_cfg_f(lc,cfg,array_spacing,v)
	wtk_local_cfg_update_cfg_f(lc,cfg,scale,v)

	wtk_array_t *a;
	a = wtk_local_cfg_find_array_s(lc,"center");
	if(a)
	{
		if(a->nslot != 3){
			return -1;
		}
		for(i = 0;i < 3;++i)
		{
			v = ((wtk_string_t**)(a->slot))[i];
			cfg->center[i]=wtk_str_atof(v->data,v->len);
		}
	}

	return 0;
}

int qtk_soundfield_syntheis_cfg_update(qtk_soundfield_syntheis_cfg_t *cfg){
	int i;
	int window_sz = cfg->hop_size * 2;
	cfg->window = wtk_malloc(sizeof(float) * window_sz);
    for (i = 0; i < window_sz; i++) {
        cfg->window[i] = 0.5 * (1 - cos(WTK_TPI * i / (window_sz - 1)));
    }
	cfg->win_gain = wtk_malloc(sizeof(float) * cfg->hop_size);
	for (i = 0; i < cfg->hop_size; i++) {
		cfg->win_gain[i] = cfg->window[i] * cfg->window[i] + cfg->window[i + cfg->hop_size] * cfg->window[i + cfg->hop_size];
	}
	return 0;
}
