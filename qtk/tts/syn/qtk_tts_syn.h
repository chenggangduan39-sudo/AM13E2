/*
 * qtk_tts_syn.h
 *
 *  Created on: Apr 26, 2023
 *      Author: dm
 */

#ifndef QTK_TTS_SYN_QTK_TTS_SYN_H_
#define QTK_TTS_SYN_QTK_TTS_SYN_H_
#include "qtk_tts_syn_cfg.h"
#include "qtk/tts/parse/qtk_tts_parse.h"
#include "cosynthesis/wtk_wsola.h"
#ifdef USE_VITS
#include "qtk/tts/vits/qtk_vits.h"
#endif
#ifdef USE_DEVICE
#include "qtk/tts/acoustic/devicetts/qtk_devicetts.h"
#endif
#ifdef USE_WORLD
#include "wtk/tts/tts-mer/syn/wtk_mer_wav.h"
#endif
#ifdef USE_LPCNET
#include "tts-tac/model/wtk_tac_lpcnet.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef int(*qtk_tts_syn_notify_f)(void* ths, short* data, int len, int is_end);

typedef struct{
	qtk_tts_syn_cfg_t* cfg;
#ifdef USE_VITS
    qtk_vits_t *vits;
#endif
#ifdef USE_DEVICE
    qtk_devicetts_t *device;
#endif

#ifdef USE_WORLD
    wtk_mer_wav_param_t *wparam;
#endif
    wtk_wsola_t* wsola;
    qtk_tts_syn_notify_f notify;
    void* user_data;
}qtk_tts_syn_t;

qtk_tts_syn_t* qtk_tts_syn_new(qtk_tts_syn_cfg_t* cfg);
int qtk_tts_syn_feed(qtk_tts_syn_t* syn, wtk_veci_t **id_vec, int nid);
int qtk_tts_syn_reset(qtk_tts_syn_t* syn);
void qtk_tts_syn_delete(qtk_tts_syn_t* syn);
void qtk_tts_syn_set_notify(qtk_tts_syn_t* syn, qtk_tts_syn_notify_f notify, void* user_data);
#ifdef __cplusplus
};
#endif
#endif /* QTK_TTS_SYN_QTK_TTS_SYN_H_ */
