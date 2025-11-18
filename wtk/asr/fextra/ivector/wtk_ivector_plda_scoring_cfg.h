#ifndef _WTK_IVECTOR_PLDA_SCORING_CFG_H_
#define _WTK_IVECTOR_PLDA_SCORING_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_ivector_plda_scoring_cfg wtk_ivector_plda_scoring_cfg_t;

struct wtk_ivector_plda_scoring_cfg
{
    wtk_vecf_t *mean;
    wtk_matf_t *transform;
    wtk_vecf_t *psi;
    wtk_vecf_t *offset;
    char *plda_fn;
};


int wtk_ivector_plda_scoring_cfg_init(wtk_ivector_plda_scoring_cfg_t *cfg);
int wtk_ivector_plda_scoring_cfg_clean(wtk_ivector_plda_scoring_cfg_t *cfg);
int wtk_ivector_plda_scoring_cfg_update_local(wtk_ivector_plda_scoring_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_ivector_plda_scoring_cfg_update(wtk_ivector_plda_scoring_cfg_t *cfg);
int wtk_ivector_plda_scoring_cfg_update2(wtk_ivector_plda_scoring_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif