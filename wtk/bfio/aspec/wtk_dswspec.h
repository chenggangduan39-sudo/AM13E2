#ifndef WTK_BFIO_ASPEC_WTK_DSWSPEC
#define WTK_BFIO_ASPEC_WTK_DSWSPEC
#include "wtk/core/wtk_complex.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int nbin;
    int channel;
    int ang_num;

	wtk_complex_t ***ovec;
    wtk_complex_t *tmp;
    float **B;
}wtk_dswspec_t;

wtk_dswspec_t *wtk_dswspec_new(int nbin,int channel,int ang_num);
void wtk_dswspec_reset(wtk_dswspec_t *dswspec);
void wtk_dswspec_delete(wtk_dswspec_t *dswspec);
void wtk_dswspec_start(wtk_dswspec_t *dswspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
void wtk_dswspec_start2(wtk_dswspec_t *dswspec,  float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx);
// float wtk_dswspec_flush_spec_k(wtk_dswspec_t *dswspec, wtk_complex_t *cov, float cov_travg, int k, int ang_idx);
float wtk_dswspec_flush_spec_k(wtk_dswspec_t *dswspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

void wtk_dswspec_flush_spec_k2(wtk_dswspec_t *dswspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec);


#ifdef __cplusplus
};
#endif
#endif
