#ifndef WTK_TTS_PARSER_WTK_TTS_SEGSNT
#define WTK_TTS_PARSER_WTK_TTS_SEGSNT
#include "wtk/core/wtk_type.h" 
#include "wtk_tts_segsnt_cfg.h"
#include "wtk/core/wtk_array.h"
#include "wtk_tts_def.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_segsnt wtk_tts_segsnt_t;

struct wtk_tts_segsnt
{
	wtk_tts_segsnt_cfg_t *cfg;
};

void wtk_tts_segsnt_init(wtk_tts_segsnt_t *snt,wtk_tts_segsnt_cfg_t *cfg);
void wtk_tts_segsnt_clean(wtk_tts_segsnt_t *snt);
void wtk_tts_segsnt_reset(wtk_tts_segsnt_t *snt);
wtk_tts_lab_t* wtk_tts_segsnt_process(wtk_tts_segsnt_t *snt,wtk_tts_info_t *info,char *s,int s_bytes);
wtk_tts_lab_t* wtk_tts_segsnt_processm(wtk_tts_segsnt_t *snt,wtk_tts_info_t *info,char *s,int s_bytes);
#ifdef __cplusplus
};
#endif
#endif
