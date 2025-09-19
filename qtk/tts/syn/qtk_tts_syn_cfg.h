/*
 * qtk_tts_syn_cfg.h
 *
 *  Created on: Apr 26, 2023
 *      Author: dm
 */

#ifndef QTK_TTS_SYN_QTK_TTS_SYN_CFG_H_
#define QTK_TTS_SYN_QTK_TTS_SYN_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "cosynthesis/wtk_wsola_cfg.h"
#ifdef USE_VITS
#include "vits/qtk_vits_cfg.h"
#endif
#ifdef USE_DEVICE
#include "acoustic/devicetts/qtk_devicetts_cfg.h"
#endif
#ifdef USE_LPCNET
#include "tts-tac/cfg/wtk_tac_cfg_syn_lpcnet.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct{
#ifdef USE_VITS
    qtk_vits_cfg_t vits;
#endif

#ifdef USE_DEVICE
    qtk_devicetts_cfg_t device;
#endif
#ifdef USE_LPCNET
    wtk_tac_cfg_syn_lpcnet_t lpcnet;
#endif
    wtk_wsola_cfg_t wsola;
    int flow_step;

    unsigned use_vits:1;   //encode
    unsigned use_device:1; //encode
    unsigned use_lpcnet:1; //decode
    unsigned use_world:1;  //decode
    unsigned use_smooth:1;

    unsigned use_sample_float:1; //wav sample size, integer or float
    unsigned int use_flow:1;
}qtk_tts_syn_cfg_t;

int qtk_tts_syn_cfg_init(qtk_tts_syn_cfg_t *cfg);
int qtk_tts_syn_cfg_clean(qtk_tts_syn_cfg_t *cfg);
int qtk_tts_syn_cfg_update_local(qtk_tts_syn_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_syn_cfg_update(qtk_tts_syn_cfg_t *cfg);
int qtk_tts_syn_cfg_update2(qtk_tts_syn_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif /* QTK_TTS_SYN_QTK_TTS_SYN_CFG_H_ */
