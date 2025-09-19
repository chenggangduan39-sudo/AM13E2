#ifndef WTK_FST_LM_GEN_WTK_LMGEN2_CFG
#define WTK_FST_LM_GEN_WTK_LMGEN2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_nglm.h"
#include "wtk/core/segmenter/wtk_prune.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmgen2_cfg wtk_lmgen2_cfg_t;
struct wtk_lmgen2_cfg
{
	wtk_lm_dict_cfg_t dict;
	wtk_nglm_cfg_t lm;
	wtk_prune_cfg_t prune;
	wtk_prune_cfg_t predict_prune;
	int depth;
	float step_ppl_thresh;
	float first_ppl_thresh;
	float step_ppl_thresh2;
	float skip_pen;
	float bow_pen;
	int nbest;
	int predict_nwrd;
	int predict_nbest;
	float predict_step_prob;
};

int wtk_lmgen2_cfg_init(wtk_lmgen2_cfg_t *cfg);
int wtk_lmgen2_cfg_clean(wtk_lmgen2_cfg_t *cfg);
int wtk_lmgen2_cfg_update_local(wtk_lmgen2_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_lmgen2_cfg_update(wtk_lmgen2_cfg_t *cfg);
int wtk_lmgen2_cfg_update2(wtk_lmgen2_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
