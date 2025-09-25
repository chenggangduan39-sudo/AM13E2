#ifndef WTK_TTS_PARSER_WTK_TTS_NORM_CFG
#define WTK_TTS_PARSER_WTK_TTS_NORM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/lex/wtk_lex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_norm_cfg wtk_tts_norm_cfg_t;
struct wtk_tts_norm_cfg
{
	wtk_lex_cfg_t lex;
	char *lex_fn;
};

int wtk_tts_norm_cfg_init(wtk_tts_norm_cfg_t *cfg);
int wtk_tts_norm_cfg_clean(wtk_tts_norm_cfg_t *cfg);
int wtk_tts_norm_cfg_update_local(wtk_tts_norm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_tts_norm_cfg_update(wtk_tts_norm_cfg_t *cfg);
int wtk_tts_norm_cfg_update2(wtk_tts_norm_cfg_t *cfg, wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
