#include "qtk_sweetspot_cfg.h" 
#include "wtk/core/math/wtk_math.h"

int qtk_sweetspot_cfg_init(qtk_sweetspot_cfg_t *cfg){
	cfg->speaker_distance = 5.0f;
	cfg->gain = 1.0f;
	cfg->hop_size = 256;
	cfg->max_N_delay = 100;
	cfg->window = NULL;
	cfg->win_gain = NULL;
	return 0;
}

int qtk_sweetspot_cfg_clean(qtk_sweetspot_cfg_t *cfg){
	if(cfg->window) {
        wtk_free(cfg->window);
    }
	if(cfg->win_gain){
		wtk_free(cfg->win_gain);
	}
	return 0;
}

int qtk_sweetspot_cfg_update_local(qtk_sweetspot_cfg_t *cfg,wtk_local_cfg_t *lc){
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_f(lc,cfg,speaker_distance,v)
	wtk_local_cfg_update_cfg_f(lc,cfg,gain,v)
	return 0;
}

int qtk_sweetspot_cfg_update(qtk_sweetspot_cfg_t *cfg){
	int i;
	int window_sz = cfg->hop_size * 2;
	cfg->window = wtk_malloc(sizeof(float) * window_sz);
    for (i = 0; i < window_sz; i++) {
        cfg->window[i] = 0.5 * (1 - cos(WTK_TPI * i / (window_sz - 1)));
    }

	cfg->win_gain = wtk_malloc(sizeof(float) * cfg->hop_size);
	for (i = 0; i < cfg->hop_size; i++) {
		cfg->win_gain[i] = cfg->window[i] + cfg->window[i + cfg->hop_size];
	}
	return 0;
}
