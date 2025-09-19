#ifndef WTK_FST_EBNFDEC_WTK_EBNFDEC2_CFG
#define WTK_FST_EBNFDEC_WTK_EBNFDEC2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ebnfdec2_cfg wtk_ebnfdec2_cfg_t;
struct wtk_ebnfdec2_cfg
{
	wtk_mbin_cfg_t *egram_bin_cfg;
	wtk_main_cfg_t *egram_cfg;
	wtk_main_cfg_t *dec_cfg;
	struct wtk_wfstdec_cfg *dec_bin_cfg;
	char *dec_fn;
	char *compile_fn;
	char *ebnf_fn;
	char *usr_bin;
	unsigned use_bin:1;
};

int wtk_ebnfdec2_cfg_init(wtk_ebnfdec2_cfg_t *cfg);
int wtk_ebnfdec2_cfg_clean(wtk_ebnfdec2_cfg_t *cfg);
int wtk_ebnfdec2_cfg_update_local(wtk_ebnfdec2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_ebnfdec2_cfg_update(wtk_ebnfdec2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
