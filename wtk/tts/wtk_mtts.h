/*
 * wtk_mtts.h
 *
 *  Created on: Dec 17, 2016
 *      Author: dm
 */

#ifndef WTK_TFIRE_KEL_WTK_MTTS_H_
#define WTK_TFIRE_KEL_WTK_MTTS_H_
#include "wtk/tts/wtk_tts.h"
#include "wtk_mtts_cfg.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef struct wtk_mtts wtk_mtts_t;
struct wtk_mtts{
	wtk_mtts_cfg_t *cfg;
	wtk_tts_t *cn;
	wtk_pitch_t *pitch;
	float sshift;
	float svol;
};

wtk_mtts_t *wtk_mtts_new(wtk_mtts_cfg_t *cfg);
int wtk_mtts_process(wtk_mtts_t *tts,char *txt,int txt_bytes);
int wtk_mtts_reset(wtk_mtts_t *mtts);
void wtk_mtts_delete(wtk_mtts_t *mtts);
void wtk_mtts_set_notify(wtk_mtts_t *mtts,void *ths,wtk_tts_notify_f notify);
void wtk_mtts_set_volume_scale(wtk_mtts_t *mtts, float scale);
void wtk_mtts_set_stop_hint(wtk_mtts_t *mtts);
void wtk_mtts_set_speed(wtk_mtts_t *mtts,float speed);
void wtk_mtts_set_pitch(wtk_mtts_t *mtts,float pitch);
void wtk_mtts_notify(wtk_mtts_t *mtts,char *data,int len);
void wtk_mtts_set_minsil_time(wtk_mtts_t* mtts, int time);
#ifdef __cplusplus
};
#endif
#endif /* WTK_TTSMIX_H_ */
