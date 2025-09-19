#ifndef __QTK_TTS_PARSE_CFG_H__
#define __QTK_TTS_PARSE_CFG_H__

#include  "wtk/tts/parser/wtk_tts_norm_cfg.h"
#include "wtk/tts/parser/wtk_tts_segsnt_cfg.h"
#include "wtk/tts/parser/wtk_tts_segwrd_cfg.h"
#include "qtk_tts_symbols_id_cfg.h"
#include "qtk_tts_prosody_cfg.h"
#include "qtk_tts_polyphone_cfg.h"
#include "segwrd/qtk_tts_segwrd_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_parse_cfg{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_rbin2_t *rbin;
    wtk_tts_norm_cfg_t norm;
    wtk_tts_segsnt_cfg_t segsnt;
    wtk_tts_segwrd_cfg_t segwrd;
    qtk_tts_symbols_id_cfg_t symbols; 
    qtk_tts_prosody_cfg_t prosody;
    qtk_tts_polyphone_cfg_t ply;
    qtk_tts_segwrd_cfg_t segwrd2;
    wtk_strpool_t *pool;

    unsigned is_en:1;
    unsigned use_segmax:1;
    unsigned debug_norm:1;
    unsigned debug_segsnt:1;
    unsigned debug_segwrd:1;
    unsigned debug_polyphone:1;
    unsigned debug_prosody:1;
    unsigned debug_id:1;
    unsigned int forvits:1;
    unsigned int fordevicetts:1;
}qtk_tts_parse_cfg_t;

int qtk_tts_parse_cfg_init(qtk_tts_parse_cfg_t *cfg);
int qtk_tts_parse_cfg_clean(qtk_tts_parse_cfg_t *cfg);
int qtk_tts_parse_cfg_update_local(qtk_tts_parse_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_tts_parse_cfg_update(qtk_tts_parse_cfg_t *cfg);
int qtk_tts_parse_cfg_update2(qtk_tts_parse_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_tts_parse_cfg_t* qtk_tts_parse_cfg_new_bin(char *cfg_fn,int seek_pos);
void qtk_tts_parse_cfg_delete_bin(qtk_tts_parse_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif
