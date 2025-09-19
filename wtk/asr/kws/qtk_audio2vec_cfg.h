#ifndef __WTK_ASR_KWS_QTK_AUDIO2VEC_CFG_H__
#define __WTK_ASR_KWS_QTK_AUDIO2VEC_CFG_H__
#pragma once
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#include "wtk_svprint.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_audio2vec_cfg qtk_audio2vec_cfg_t;

struct qtk_audio2vec_cfg {
    wtk_svprint_cfg_t svprint;
    wtk_kvad_cfg_t vad;
    unsigned int use_vad : 1;
    unsigned int use_window_pad : 1;
};

int qtk_audio2vec_cfg_init(qtk_audio2vec_cfg_t *cfg);
int qtk_audio2vec_cfg_clean(qtk_audio2vec_cfg_t *cfg);
int qtk_audio2vec_cfg_update(qtk_audio2vec_cfg_t *cfg);
int qtk_audio2vec_cfg_update2(qtk_audio2vec_cfg_t *cfg,
                              wtk_source_loader_t *sl);
int qtk_audio2vec_cfg_update_local(qtk_audio2vec_cfg_t *cfg, wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif
