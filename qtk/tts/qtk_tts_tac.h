#ifndef __QTK_TTS_TAC_H__
#define __QTK_TTS_TAC_H__

#include "qtk_tts_tac_cfg.h"
#include "parse/qtk_tts_parse.h"
#include "qtk_tts_tac2_lpcnet.h"
#include "qtk_tts_durian_lpcnet.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*qtk_tac_notify_f)(void *user_data,short *data,int len,int is_end);

typedef struct qtk_tts_tac{
    qtk_tts_tac_cfg_t *cfg;
    
    qtk_tts_parse_t *parse;
    qtk_tts_tac2_lpcnet_t *tac2_lpcnet;
    qtk_tts_durian_lpcnet_t *durian_lpcnet;

    qtk_tac_notify_f notify;
    void *user_data;
    wtk_mer_wav_stream_t *wav;
}qtk_tts_tac_t;

qtk_tts_tac_t *qtk_tts_tac_new(qtk_tts_tac_cfg_t *cfg);
int qtk_tts_tac_delete(qtk_tts_tac_t *tac);
int qtk_tts_tac_reset(qtk_tts_tac_t *tac);
int qtk_tts_tac_process(qtk_tts_tac_t *tac,char *txt,int len);
void qtk_tts_tac_set_notify(qtk_tts_tac_t *tac,void *user_data,qtk_tac_notify_f);

#ifdef __cplusplus
};
#endif

#endif
