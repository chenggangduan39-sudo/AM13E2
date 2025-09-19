#ifndef WTK_FST_WTK_WFSTENV_CFG_H_
#define WTK_FST_WTK_WFSTENV_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfstenv_cfg wtk_wfstenv_cfg_t;

struct wtk_wfstdec_cfg;

struct wtk_wfstenv_cfg
{
	struct wtk_wfstdec_cfg *cfg;
	wtk_string_t sep;
	wtk_string_t custom;
	unsigned use_vad:1;
	unsigned use_usrec:1;
	unsigned use_ebnfdec:1;
	unsigned use_hint:1;
	unsigned use_dec2:1;
	unsigned use_timestamp:1;
	unsigned use_laststate:1;
	unsigned use_pp:1;
};

int wtk_wfstenv_cfg_init(wtk_wfstenv_cfg_t *cfg,struct wtk_wfstdec_cfg *dec_cfg);
void wtk_wfstenv_cfg_init2(wtk_wfstenv_cfg_t *cfg);
int wtk_wfstenv_cfg_update_local(wtk_wfstenv_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wfstenv_cfg_update_local2(wtk_wfstenv_cfg_t *cfg,wtk_local_cfg_t *lc,int ebnf2);
#ifdef __cplusplus
};
#endif
#endif
