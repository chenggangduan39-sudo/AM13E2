#ifndef WTK_TTS_PARSER_WTK_POLYPHN_CFG
#define WTK_TTS_PARSER_WTK_POLYPHN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk_polyphn_lex.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_polyphn_cfg wtk_polyphn_cfg_t;

struct wtk_polyphn_cfg
{
	char *lex_fn;
	wtk_polyphn_lex_t *polyphn;
	wtk_array_t *soft_wrds_end;     //soft tone words at end of sentence.
	unsigned use_soft_end:1;           // switch for stwrds_ond.
	unsigned use_defpron:1;             // switch for self define pron.
};

int wtk_polyphn_cfg_init(wtk_polyphn_cfg_t *cfg);
int wtk_polyphn_cfg_clean(wtk_polyphn_cfg_t *cfg);
int wtk_polyphn_cfg_update_local(wtk_polyphn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_polyphn_cfg_update(wtk_polyphn_cfg_t *cfg);
int wtk_polyphn_cfg_update2(wtk_polyphn_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
