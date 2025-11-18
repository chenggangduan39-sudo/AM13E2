#ifndef __QTK_TTS_TAC2_LPCNET_CFG_H__
#define __QTK_TTS_TAC2_LPCNET_CFG_H__

#include "acoustic/tac2_syn/qtk_tts_tac2_syn_cfg.h"
#include "tts-tac/cfg/wtk_tac_cfg_syn_lpcnet.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    qtk_tts_tac2_syn_cfg_t tac2_cfg;
    wtk_tac_cfg_syn_lpcnet_t lpcnet_cfg;
    int sample_rate;
}qtk_tts_tac2_lpcnet_cfg_t;

int qtk_tts_tac2_lpcnet_cfg_init(qtk_tts_tac2_lpcnet_cfg_t *cfg);
int qtk_tts_tac2_lpcnet_cfg_clean(qtk_tts_tac2_lpcnet_cfg_t *cfg);
int qtk_tts_tac2_lpcnet_cfg_update_local(qtk_tts_tac2_lpcnet_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_tac2_lpcnet_cfg_update(qtk_tts_tac2_lpcnet_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif
