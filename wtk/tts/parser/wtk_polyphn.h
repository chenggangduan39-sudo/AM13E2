#ifndef WTK_TTS_PARSER_WTK_POLYPHN
#define WTK_TTS_PARSER_WTK_POLYPHN
#include "wtk/core/wtk_type.h" 
#include "wtk_polyphn_cfg.h"
#include "wtk_tts_def.h"
#include "wtk_defpron.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_polyphn wtk_polyphn_t;
struct wtk_polyphn
{
	wtk_polyphn_cfg_t *cfg;
	wtk_strbuf_t *buf1;
	wtk_strbuf_t *buf2;
	wtk_strbuf_t *buf3;        //for word position add by dmd 20180406
	wtk_defpron_t *defpron;    //user-def pron.
	unsigned use_defpron:1;        // switch for user define pron.
};

wtk_polyphn_t* wtk_polyphn_new(wtk_polyphn_cfg_t *cfg);
void wtk_polyphn_delete(wtk_polyphn_t *p);
void wtk_polyphn_reset(wtk_polyphn_t *p);
int wtk_polyphn_process(wtk_polyphn_t *p,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
int wtk_polyphn_process_snt(wtk_polyphn_t *p,wtk_tts_info_t *info,wtk_tts_snt_t *s);
int wtk_polyphn_bytes(wtk_polyphn_t* polyphn);
#ifdef __cplusplus
};
#endif
#endif
