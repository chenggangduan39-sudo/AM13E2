#ifndef WTK_ASR_PARM_WTK_LDA_CFG
#define WTK_ASR_PARM_WTK_LDA_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/math/wtk_mat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lda_cfg wtk_lda_cfg_t;
struct wtk_lda_cfg
{
	wtk_matf_t *lda;
	int win;
	char *lda_fn;
};

int wtk_lda_cfg_init(wtk_lda_cfg_t *cfg);
int wtk_lda_cfg_clean(wtk_lda_cfg_t *cfg);
int wtk_lda_cfg_update_local(wtk_lda_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_lda_cfg_update(wtk_lda_cfg_t *cfg);
int wtk_lda_cfg_update2(wtk_lda_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
