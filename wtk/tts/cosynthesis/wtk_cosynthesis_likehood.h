#ifndef WTK_COSYNTHESIS_LIKEHOOD_H
#define WTK_COSYNTHESIS_LIKEHOOD_H
#ifdef __cplusplus
extern "C" {
#endif

#include "wtk/tts/cosynthesis/wtk_cosynthesis_lexicon_cfg.h"
#include "wtk/tts/cosynthesis/wtk_cosynthesis.h"

int wtk_cosynthesis_get_likehood(wtk_cosynthesis_t *cs, float *tar_spec_model_mean,float *tar_spec_model_var, int tscol,
                                float *tar_picth_model_mean,float *tar_pitch_model_var,float *tar_pitch_model_w, int tpcol,
                                float *tar_dur_model_mean, float *tar_dur_model_var, int tdcol,
                                float *conca_spec_model_mean,float *conca_spec_model_var, int csrow, int cscol,
                                float *conca_pitch_model_mean,float *conca_pitch_model_var,float *conca_pitch_model_w,int cprow,int cpcol,
                                short *candi_uid, int culen,wtk_unit_t *data, 
                                short *candi_uid_pre, int cuplen, wtk_unit_t *data_pre,int nphone,
                                float **glb_select_cost,float ***glb_conca_cost,float spec_w,float dur_w,
                                float pitch_w,float conca_spec_w,float conca_pitch_w);

#ifdef __cplusplus
};
#endif
#endif
