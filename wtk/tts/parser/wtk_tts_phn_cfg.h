#ifndef WTK_TTS_PARSER_WTK_TTS_PHN_CFG
#define WTK_TTS_PARSER_WTK_TTS_PHN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/wtk_kdict.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_tts_def.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_phn_cfg wtk_tts_phn_cfg_t;
struct wtk_tts_phn_cfg
{
	char *dict_fn;
	wtk_kdict_t *dict;
	int dict_hint;
};

int wtk_tts_phn_cfg_init(wtk_tts_phn_cfg_t *cfg);
int wtk_tts_phn_cfg_clean(wtk_tts_phn_cfg_t *cfg);
int wtk_tts_phn_cfg_update_local(wtk_tts_phn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_tts_phn_cfg_update(wtk_tts_phn_cfg_t *cfg,wtk_strpool_t *pool);
int wtk_tts_phn_cfg_update2(wtk_tts_phn_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
int wtk_tts_phn_cfg_bytes(wtk_tts_phn_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
