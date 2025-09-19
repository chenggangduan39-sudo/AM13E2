#ifndef WTK_FST_EBNFDEC_WTK_EBNFDEC_CFG
#define WTK_FST_EBNFDEC_WTK_EBNFDEC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/wfst/wtk_wfstdec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ebnfdec_cfg wtk_ebnfdec_cfg_t;
struct wtk_ebnfdec_cfg
{
	wtk_mbin_cfg_t *egram_bin_cfg;
	wtk_main_cfg_t *egram_cfg;
	wtk_main_cfg_t *dec_cfg;
	wtk_wfstdec_cfg_t *dec_bin_cfg;
	wtk_vad_cfg_t *vad_cfg;
	char *dec_fn;
	char *compile_fn;
	char *vad_fn;
	unsigned use_bin:1;
};

int wtk_ebnfdec_cfg_init(wtk_ebnfdec_cfg_t *cfg);
int wtk_ebnfdec_cfg_clean(wtk_ebnfdec_cfg_t *cfg);
int wtk_ebnfdec_cfg_update_local(wtk_ebnfdec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_ebnfdec_cfg_update(wtk_ebnfdec_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
