#ifndef WTK_ASR_WFST_WTK_WFSTDEC_OUTPUT_CFG
#define WTK_ASR_WFST_WTK_WFSTDEC_OUTPUT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_version_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfstdec_output_cfg wtk_wfstdec_output_cfg_t;
struct wtk_wfstdec_output_cfg
{
	wtk_version_cfg_t version;
	wtk_string_t res;
	wtk_string_t sep;
};

int wtk_wfstdec_output_cfg_init(wtk_wfstdec_output_cfg_t *cfg);
int wtk_wfstdec_output_cfg_clean(wtk_wfstdec_output_cfg_t *cfg);
int wtk_wfstdec_output_cfg_update_local(wtk_wfstdec_output_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_wfstdec_output_cfg_update(wtk_wfstdec_output_cfg_t *cfg);
int wtk_wfstdec_output_cfg_update2(wtk_wfstdec_output_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
