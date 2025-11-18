#ifndef __QTK_TTS_TAC_CFG_H__
#define __QTK_TTS_TAC_CFG_H__

#include "parse/qtk_tts_parse_cfg.h"
#include "qtk_tts_tac2_lpcnet_cfg.h"
#include "qtk_tts_durian_lpcnet_cfg.h"

//new tac2 cfg
#ifdef __cplusplus
extern "C"{
#endif

typedef struct  qtk_tts_tac_cfg{
    qtk_tts_parse_cfg_t parse_cfg;
    qtk_tts_tac2_lpcnet_cfg_t syn_cfg;
    qtk_tts_durian_lpcnet_cfg_t durian_lpcnet_cfg;
    int use_durian_lpcnet;
}qtk_tts_tac_cfg_t;

int qtk_tts_tac_cfg_init(qtk_tts_tac_cfg_t *cfg);
int qtk_tts_tac_cfg_clean(qtk_tts_tac_cfg_t *cfg);
int qtk_tts_tac_cfg_update_local(qtk_tts_tac_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_tac_cfg_update(qtk_tts_tac_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif
