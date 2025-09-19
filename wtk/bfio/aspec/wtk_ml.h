#ifndef WTK_BFIO_ASPEC_WTK_ML
#define WTK_BFIO_ASPEC_WTK_ML
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int nbin;
    int channel;
    int ang_num;
	wtk_complex_t ***ovec;
}wtk_ml_t;

wtk_ml_t *wtk_ml_new(int nbin,int channel,int ang_num);
void wtk_ml_reset(wtk_ml_t *ml);
void wtk_ml_delete(wtk_ml_t *ml);
void wtk_ml_start(wtk_ml_t *ml, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
void wtk_ml_start2(wtk_ml_t *ml, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx);
float wtk_ml_flush_spec_k(wtk_ml_t *ml, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);
float wtk_ml_flush_spec_k2(wtk_ml_t *ml, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);


#ifdef __cplusplus
};
#endif
#endif