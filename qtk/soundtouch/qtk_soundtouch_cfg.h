/*
 * qtk_soundtouch_cfg.h
 *
 *  Created on: Mar 31, 2023
 *      Author: dm
 */

#ifndef QTK_SOUNDTOUCH_QTK_SOUNDTOUCH_CFG_H_
#define QTK_SOUNDTOUCH_QTK_SOUNDTOUCH_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_soundtouch_cfg{
	int sampleRate;
	int channels;
    float tempo;
    float pitch;
    float rate;
    int   quick;
    int   noAntiAlias;
    float goalBPM;
	int  detectBPM;
    int  speech;
}qtk_soundtouch_cfg_t;
int qtk_soundtouch_cfg_init(qtk_soundtouch_cfg_t *cfg);
int qtk_soundtouch_cfg_clean(qtk_soundtouch_cfg_t *cfg);
int qtk_soundtouch_cfg_update_local(qtk_soundtouch_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_soundtouch_cfg_update(qtk_soundtouch_cfg_t *cfg);
int qtk_soundtouch_cfg_update2(qtk_soundtouch_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
}
#endif
#endif /* QTK_SOUNDTOUCH_QTK_SOUNDTOUCH_CFG_H_ */
