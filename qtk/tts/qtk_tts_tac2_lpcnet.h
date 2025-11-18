#ifndef __QTK_TTS_TAC2_LPCNET_H__
#define __QTK_TTS_TAC2_LPCNET_H__

#include "qtk_tts_tac2_lpcnet_cfg.h"
#include "acoustic/tac2_syn/qtk_tts_tac2_syn.h"
#include "tts-tac/model/wtk_tac_lpcnet.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_tts_tac2_lpcnet_notify_f)(void *,short *data, int len, int is_end);

typedef struct{
    qtk_tts_tac2_lpcnet_cfg_t *cfg;
    qtk_tts_tac2_syn_t *tac2;

    void *user_data;
    qtk_tts_tac2_lpcnet_notify_f notify;
}qtk_tts_tac2_lpcnet_t;

qtk_tts_tac2_lpcnet_t *qtk_tts_tac2_lpcnet_new(qtk_tts_tac2_lpcnet_cfg_t *cfg);
int qtk_tts_tac2_lpcnet_set_notify(qtk_tts_tac2_lpcnet_t *syn,void *user_data,qtk_tts_tac2_lpcnet_notify_f notify);
int qtk_tts_tac2_lpcnet_process(qtk_tts_tac2_lpcnet_t *syn,wtk_veci_t *vec,int is_end);
int qtk_tts_tac2_lpcnet_delete(qtk_tts_tac2_lpcnet_t *syn);

#ifdef __cplusplus
};
#endif

#endif
