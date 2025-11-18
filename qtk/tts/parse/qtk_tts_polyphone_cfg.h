#ifndef  __QTK_TTS_POLYPHONE_CFG_H__
#define __QTK_TTS_POLYPHONE_CFG_H__

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_kdict.h"
#include "dispoly/qtk_tts_dispoly_cfg.h"
#include "wtk/tts/parser/wtk_tts_def.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_polyphone_cfg{
    qtk_tts_dispoly_cfg_t disply;

    char *pron_fn;
    char *ply_fn;
    char *unstressed_fn;
    char *char_fn;
    char *pp_fn;
    char *poly_id_fn;
    int pron_hint;
    int ply_hint;
    int unstressed_hint;
    int char_hint;
    int pp_hint;
    int poly_id_hint;
    wtk_kdict_t *pron_dict;
    wtk_kdict_t *ply_dict;
    wtk_kdict_t *unstressed_dict;
    wtk_kdict_t *char_dict;
    wtk_kdict_t *pp_dict;
    wtk_kdict_t *poly_id_dict;
    unsigned is_en:1;
    unsigned use_disply:1;
}qtk_tts_polyphone_cfg_t;

typedef struct{
    wtk_string_t *chart;
    wtk_tts_wrd_pron_t *pron;
}qtk_tts_polyphone_char_t;

int qtk_tts_polyphone_cfg_init(qtk_tts_polyphone_cfg_t *cfg);
int qtk_tts_polyphone_cfg_clean(qtk_tts_polyphone_cfg_t *cfg);
int qtk_tts_polyphone_cfg_update_local(qtk_tts_polyphone_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_polyphone_cfg_update(qtk_tts_polyphone_cfg_t *cfg);
int qtk_tts_polyphone_cfg_update2(qtk_tts_polyphone_cfg_t *cfg, wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif

#endif
