#ifndef WTK_TTS_PARSER_WTK_TTS_PARSER_CFG
#define WTK_TTS_PARSER_WTK_TTS_PARSER_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk_tts_segsnt.h"
#include "wtk_tts_segwrd.h"
#include "wtk/tts/parser/pos/wtk_tts_pos.h"
#include "wtk_tts_phn.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_tts_norm.h"
#include "wtk_polyphn.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_parser_cfg wtk_tts_parser_cfg_t;

struct wtk_tts_parser_cfg
{
	wtk_tts_norm_cfg_t norm;
	wtk_tts_segsnt_cfg_t segsnt;
	wtk_tts_segwrd_cfg_t segwrd;
	wtk_tts_pos_cfg_t pos;
	wtk_polyphn_cfg_t polyphn;
	wtk_tts_phn_cfg_t phn;
	wtk_strpool_t *pool;
	unsigned use_pos:1;
	unsigned use_hi:1;
};

int wtk_tts_parser_cfg_init(wtk_tts_parser_cfg_t *cfg);
int wtk_tts_parser_cfg_clean(wtk_tts_parser_cfg_t *cfg);
int wtk_tts_parser_cfg_update_local(wtk_tts_parser_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_tts_parser_cfg_update(wtk_tts_parser_cfg_t *cfg);
int wtk_tts_parser_cfg_update2(wtk_tts_parser_cfg_t *cfg,wtk_strpool_t *pool);

int wtk_tts_parser_cfg_update3(wtk_tts_parser_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);

int wtk_tts_parser_cfg_bytes(wtk_tts_parser_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
