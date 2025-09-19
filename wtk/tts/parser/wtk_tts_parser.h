#ifndef WTK_TTS_PARSER_WTK_TTS_PARSER
#define WTK_TTS_PARSER_WTK_TTS_PARSER
#include "wtk/core/wtk_type.h" 
#include "wtk_tts_parser_cfg.h"
#include "wtk_defpron.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_parser wtk_tts_parser_t;


struct wtk_tts_parser
{
	wtk_tts_parser_cfg_t *cfg;
	wtk_heap_t *heap;
	wtk_strbuf_t *buf;
	wtk_tts_norm_t* norm;
	wtk_tts_segsnt_t segsnt;
	wtk_tts_segwrd_t segwrd;
	wtk_tts_pos_t pos;
	wtk_polyphn_t *polyphn;
	wtk_tts_phn_t phn;
	wtk_tts_lab_t *lab;
};


wtk_tts_parser_t* wtk_tts_parser_new(wtk_tts_parser_cfg_t *cfg,wtk_rbin2_t *rbin);
void wtk_tts_parser_delete(wtk_tts_parser_t *p);
void wtk_tts_parser_reset(wtk_tts_parser_t *p);
wtk_tts_lab_t* wtk_tts_parser_to_snt(wtk_tts_parser_t *p,char *s,int s_bytes);
wtk_tts_lab_t* wtk_tts_parser_to_sntm(wtk_tts_parser_t *p,char *s,int s_bytes,int use_m);
int wtk_tts_parser_process_snt(wtk_tts_parser_t *p,wtk_tts_snt_t *snt);
int wtk_tts_parser_process(wtk_tts_parser_t *p,char *s,int s_bytes);
int wtk_tts_parser_process2(wtk_tts_parser_t *p,char *s,int s_bytes);
void wtk_tts_parser_print(wtk_tts_parser_t *p);
int wtk_tts_parser_bytes(wtk_tts_parser_t *p);
void wtk_tts_parser_defpron_wrd(wtk_tts_parser_t*p, wtk_string_t* k, wtk_string_t* v);
#ifdef __cplusplus
};
#endif
#endif
