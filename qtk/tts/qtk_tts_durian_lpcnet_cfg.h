#ifndef __QTK_TTS_DURIAN_LPCNET_CFG_H__
#define __QTK_TTS_DURIAN_LPCNET_CFG_H__

#include "acoustic/durian/qtk_durian_cfg.h"
#include "tts-tac/cfg/wtk_tac_cfg_syn_lpcnet.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct {
    qtk_durian_cfg_t durian_cfg;
    wtk_tac_cfg_syn_lpcnet_t lpcnet_cfg;
}qtk_tts_durian_lpcnet_cfg_t;

int qtk_tts_durian_lpcnet_cfg_init(qtk_tts_durian_lpcnet_cfg_t *cfg);
int qtk_tts_durian_lpcnet_cfg_clean(qtk_tts_durian_lpcnet_cfg_t *cfg);
int qtk_tts_durian_lpcnet_cfg_update_local(qtk_tts_durian_lpcnet_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_durian_lpcnet_cfg_update(qtk_tts_durian_lpcnet_cfg_t *cfg);

#ifdef __cplusplus
};
#endif


#endif
