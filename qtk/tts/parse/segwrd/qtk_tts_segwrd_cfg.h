#ifndef __QTK_TTS_SEGWRD_CFG_H__
#define __QTK_TTS_SEGWRD_CFG_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_kdict.h"

typedef struct{
    int wordmax;
    int wordmin;
    int feature_idx_hint;
    int unigram_hint;
    int post_cut_hint;
    int n_tag;
    int n_feature;
    wtk_array_t *norm_num;
    wtk_array_t *norm_letter;
    char *feature_idx_fn;
    char *unigram_fn;
    char *post_cut_fn;
    char *modelw_fn;
    wtk_kdict_t *feature_idx_dict;
    wtk_kdict_t *unigram_dict;
    wtk_kdict_t *post_cut_dict;
    float scalar;

    unsigned int use_pre:1;
    unsigned int use_post:1;
}qtk_tts_segwrd_cfg_t;

int qtk_tts_segwrd_cfg_init(qtk_tts_segwrd_cfg_t *cfg);
int qtk_tts_segwrd_cfg_clean(qtk_tts_segwrd_cfg_t *cfg);
int qtk_tts_segwrd_cfg_update_local(qtk_tts_segwrd_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_tts_segwrd_cfg_update(qtk_tts_segwrd_cfg_t *cfg);
int qtk_tts_segwrd_cfg_update2(qtk_tts_segwrd_cfg_t *cfg, wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif

#endif
