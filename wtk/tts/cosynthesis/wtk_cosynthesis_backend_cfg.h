#ifndef WTK_COSYN_BACKEND_CFG
#define WTK_COSYN_BACKEND_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_cosynthesis_dtree.h"
#include "wtk_cosynthesis_hmm.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cosynthesis_backend_cfg wtk_cosynthesis_backend_cfg_t;

struct wtk_cosynthesis_backend_cfg
{
	wtk_cosynthesis_dtree_cfg_t tree_cfg;
	wtk_cosynthesis_hmm_cfg_t hmm_cfg;
	wtk_cosynthesis_dtree_t *tree;
	wtk_cosynthesis_hmm_t *hmm;
    float spec_weight;
    float dur_weight;
    float pitch_weight;
    float conca_spec_weight;
    float conca_pitch_weight;
};

int wtk_cosynthesis_backend_cfg_init(wtk_cosynthesis_backend_cfg_t *cfg);
int wtk_cosynthesis_backend_cfg_clean(wtk_cosynthesis_backend_cfg_t *cfg);
int wtk_cosynthesis_backend_cfg_update_local(wtk_cosynthesis_backend_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_cosynthesis_backend_cfg_update2(wtk_cosynthesis_backend_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);

#ifdef __cplusplus
};
#endif
#endif
