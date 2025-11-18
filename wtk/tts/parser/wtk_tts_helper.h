#ifndef WTK_TTS_PARSER_WTK_TTS_HELPER
#define WTK_TTS_PARSER_WTK_TTS_HELPER
#include "wtk_tts_parser_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_phn_repalce wtk_tts_phn_repalce_t;

struct wtk_tts_phn_repalce
{
	wtk_string_t *phns;
	int tone;
	int index;
};

#ifdef __cplusplus
};
#endif
#endif
