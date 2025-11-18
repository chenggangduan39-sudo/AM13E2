#ifndef WTK_COSYNTHESIS_CAL
#define WTK_COSYNTHESIS_CAL
#include <stdint.h>
#include "wtk/core/math/wtk_math.h"
#include "wtk_cosynthesis_lexicon_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

float kld_preselect(float*candi_dur_mean,float *candi_dur_var,float* candi_lf0_mean,float *candi_lf0_var,
float* candi_lf0_weight,float *candi_mcep_mean,float *candi_mcep_var,float *dur_mean,float *dur_var,
float*lf0_mean,float *lf0_var,float* lf0_weight,float *mcep_mean,float *mcep_var,int nphone,float *constant,
int dur_len,int lf0_len,int mcep_len);

#ifdef __cplusplus
};
#endif
#endif
