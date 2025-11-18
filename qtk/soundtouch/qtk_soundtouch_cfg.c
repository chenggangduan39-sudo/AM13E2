/*
 * qtk_soundtouch_cfg.c
 *
 *  Created on: Mar 31, 2023
 *      Author: dm
 */
#include "qtk_soundtouch_cfg.h"

int qtk_soundtouch_cfg_init(qtk_soundtouch_cfg_t *cfg)
{
	cfg->sampleRate=16000;
	cfg->channels=1;

    cfg->tempo = 1;
    cfg->pitch = 1;
    cfg->rate = 1;
    cfg->quick = 0;
    cfg->noAntiAlias = 0;
    cfg->goalBPM = 0;
    cfg->speech = 0;
    cfg->detectBPM = 0;

    return 0;
}

int qtk_soundtouch_cfg_clean(qtk_soundtouch_cfg_t *cfg)
{
	return 0;
}

int qtk_soundtouch_cfg_update_local(qtk_soundtouch_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, sampleRate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channels, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, tempo, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pitch, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rate, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, quick, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, noAntiAlias, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, goalBPM, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, speech, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, detectBPM, v);

	return 0;

}

int qtk_soundtouch_cfg_update(qtk_soundtouch_cfg_t *cfg)
{
	return 0;
}


int qtk_soundtouch_cfg_update2(qtk_soundtouch_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}


