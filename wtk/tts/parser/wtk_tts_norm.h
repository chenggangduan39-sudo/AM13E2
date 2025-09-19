#ifndef WTK_TTS_PARSER_WTK_TTS_NORM
#define WTK_TTS_PARSER_WTK_TTS_NORM
#include "wtk/core/wtk_type.h" 
#include "wtk_tts_norm_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_norm wtk_tts_norm_t;

struct wtk_tts_norm
{
	wtk_tts_norm_cfg_t *cfg;
	wtk_lex_t *lex;
};

wtk_tts_norm_t* wtk_tts_norm_new(wtk_tts_norm_cfg_t *cfg,wtk_rbin2_t *rbin);
void wtk_tts_norm_delete(wtk_tts_norm_t *n);
void wtk_tts_norm_reset(wtk_tts_norm_t *n);
wtk_string_t wtk_tts_norm_process(wtk_tts_norm_t *n,char *data,int bytes);
int wtk_tts_norm_bytes(wtk_tts_norm_t* n);
#ifdef __cplusplus
};
#endif
#endif
