#ifndef WTK_FST_LM_GEN_WTK_LMGEN_REC_CFG
#define WTK_FST_LM_GEN_WTK_LMGEN_REC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/wfst/rec/rescore/lm/nglm/wtk_nglm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmgen_rec_cfg wtk_lmgen_rec_cfg_t;
struct wtk_lmgen_rec_cfg
{
	wtk_nglm_cfg_t lm;
	float hit_scale;
	float has_hit_scale;
	float hit_scale2;
	float step_ppl_beam;
	float end_ppl_beam;
	float end_thresh;
	float stop_scale;
	int ntok;
	int max_tok;
	int max_depth;
	int nbest;
};

int wtk_lmgen_rec_cfg_init(wtk_lmgen_rec_cfg_t *cfg);
int wtk_lmgen_rec_cfg_clean(wtk_lmgen_rec_cfg_t *cfg);
int wtk_lmgen_rec_cfg_update_local(wtk_lmgen_rec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_lmgen_rec_cfg_update(wtk_lmgen_rec_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
