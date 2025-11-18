#ifndef WTK_TTS_PARSER_WTK_TTS_PHN
#define WTK_TTS_PARSER_WTK_TTS_PHN
#include "wtk/core/wtk_type.h"
#include "wtk_tts_phn_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_phn wtk_tts_phn_t;
struct wtk_tts_phn
{
	wtk_tts_phn_cfg_t *cfg;
};

void wtk_tts_phn_init(wtk_tts_phn_t *phn,wtk_tts_phn_cfg_t *cfg);
void wtk_tts_phn_clean(wtk_tts_phn_t *phn);
void wtk_tts_phn_reset(wtk_tts_phn_t *phn);
int wtk_tts_phn_process(wtk_tts_phn_t *pos,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
int wtk_tts_phn_process_snt(wtk_tts_phn_t *phn,wtk_tts_info_t *info,wtk_tts_snt_t *s);
#ifdef __cplusplus
};
#endif
#endif
