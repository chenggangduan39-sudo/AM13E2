#ifndef _WTK_IVECTOR_PLDA_SCORING_H_
#define _WTK_IVECTOR_PLDA_SCORING_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk_ivector_plda_scoring_cfg.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_ivector_plda_scoring wtk_ivector_plda_scoring_t;

struct wtk_ivector_plda_scoring
{
    wtk_ivector_plda_scoring_cfg_t *cfg;
    wtk_nnet3_xvector_compute_t *x;
    wtk_vecf_t *tmp;
    wtk_vecf_t *tmp2;
    wtk_vecf_t *tmp3;
    wtk_vecf_t *ivector;
};

wtk_ivector_plda_scoring_t* wtk_ivector_plda_scoring_new(wtk_ivector_plda_scoring_cfg_t *cfg);
void wtk_ivector_plda_scoring_delete(wtk_ivector_plda_scoring_t *s);
wtk_vecf_t* wtk_ivector_plda_scoring_transfrom_ivector(wtk_ivector_plda_scoring_t *s,wtk_vecf_t *xvector);
float wtk_ivector_plda_scoring_get_normalization_factor(wtk_ivector_plda_scoring_t *s,int cnt);
wtk_vecf_t* wtk_ivector_plda_scoring_ivector_normalization(wtk_ivector_plda_scoring_t *s,int cnt);
float wtk_ivector_plda_scoring_loglikelihood(wtk_ivector_plda_scoring_t *s,wtk_vecf_t *feat1,int cnt,wtk_vecf_t *feat2);
#ifdef __cplusplus
};
#endif
#endif