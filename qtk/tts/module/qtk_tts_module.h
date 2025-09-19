/*
 * qtk_tts_module.h
 *
 *  Created on: Feb 23, 2023
 *      Author: dm
 */

#ifndef QTK_TTS_MODULE_H_
#define QTK_TTS_MODULE_H_
#include "qtk_tts_module_cfg.h"
#include "qtk/tts/parse/qtk_tts_parse.h"
#include "syn/qtk_tts_syn.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int(*qtk_tts_module_notify_f)(void* ths, short* data, int len, int is_end);

typedef struct{
	float vol;
	float tempo;
	float pitch;
	float rate;
}qtk_tts_param_t;

typedef struct{
	qtk_tts_module_cfg_t* cfg;
    qtk_tts_param_t param;
    qtk_tts_parse_t* parse;
    qtk_tts_syn_t* syn;

    void *soundtouch;
    qtk_tts_module_notify_f notify;
    void* user_data;
    unsigned use_vits:1;
    unsigned use_device:1;
}qtk_tts_module_t;

qtk_tts_module_t* qtk_tts_module_new(qtk_tts_module_cfg_t* cfg);
int qtk_tts_module_feed(qtk_tts_module_t* m, char* data, int len);
int qtk_tts_module_reset(qtk_tts_module_t* m);
void qtk_tts_module_delete(qtk_tts_module_t* m);
void qtk_tts_module_setNotify(qtk_tts_module_t* m, qtk_tts_module_notify_f notify, void* user_data);
void qtk_tts_module_setVol(qtk_tts_module_t* m, float vol);
void qtk_tts_module_setPitch(qtk_tts_module_t* m, float pitch);
void qtk_tts_module_setTempo(qtk_tts_module_t* m, float tempo);
void qtk_tts_module_setVolChanged(qtk_tts_module_t* m, float volDelta);
void qtk_tts_module_setPitchChanged(qtk_tts_module_t* m, float pitchDelta);
void qtk_tts_module_setTempoChanged(qtk_tts_module_t* m, float tempoDelta);
void qtk_tts_module_setRateChanged(qtk_tts_module_t* m, float rateDelta);
#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_API_QTK_TTS_API_H_ */
