#ifndef __QTK_TTS_TAC2_SYN_CFG_H__
#define __QTK_TTS_TAC2_SYN_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_array.h"
#include "qtk_tts_tac2_syn_enc_cfg.h"
#include "qtk_tts_tac2_syn_dec_cfg.h"
#include "qtk_tts_tac2_syn_postnet_cfg.h"
#include "tts/dfsmn/qtk_tts_dfsmn_cfg.h"
#include "qtk_tts_tac2_syn_gmmdec_cfg.h"
#include "tts-tac/cfg/wtk_tac_cfg_syn_lpcnet.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct  qtk_tts_tac2_syn_cfg{
    wtk_array_t *embedding_dim;
    char *embedding_fn;
    qtk_tts_tac2_syn_enc_cfg_t enc_cfg;
    qtk_tts_dfsmn_cfg_t dfsmn_enc_cfg;
    qtk_tts_tac2_syn_dec_cfg_t dec_cfg;
    qtk_tts_tac2_syn_gmmdec_cfg_t gmmdec_cfg;
    qtk_tts_tac2_syn_postnet_cfg_t postnet;

    unsigned int use_postnet:1;
    unsigned int use_dfsmn:1;
    unsigned int use_gmmdec:1;
}qtk_tts_tac2_syn_cfg_t;


int qtk_tts_tac2_syn_cfg_init(qtk_tts_tac2_syn_cfg_t *cfg);
int qtk_tts_tac2_syn_cfg_clean(qtk_tts_tac2_syn_cfg_t *cfg);
int qtk_tts_tac2_syn_cfg_update_local(qtk_tts_tac2_syn_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_tac2_syn_cfg_update(qtk_tts_tac2_syn_cfg_t *cfg);


#ifdef __cplusplus
};
#endif

#endif
