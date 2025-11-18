#ifndef WTK_BFIO_ASPEC_WTK_GCCSPEC
#define WTK_BFIO_ASPEC_WTK_GCCSPEC
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
}wtk_gccspec_t;

wtk_gccspec_t *wtk_gccspec_new(int nbin,int channel,int ang_num);
void wtk_gccspec_reset(wtk_gccspec_t *gccspec);
void wtk_gccspec_delete(wtk_gccspec_t *gccspec);
void wtk_gccspec_start(wtk_gccspec_t *gccspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
void wtk_gccspec_start2(wtk_gccspec_t *gccspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx);
// float wtk_gccspec_flush_spec_k(wtk_gccspec_t *gccspec, wtk_complex_t **fft, float fftabs2, int k, int ang_idx);
float wtk_gccspec_flush_spec_k(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);
float wtk_gccspec_flush_spec_k2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);


float wtk_gccspec_flush_spec_k3(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);
float wtk_gccspec_flush_spec_k4(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);


void wtk_gccspec_flush_spec_k_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec);
void wtk_gccspec_flush_spec_k2_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec);


void wtk_gccspec_flush_spec_k3_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec);
void wtk_gccspec_flush_spec_k4_2(wtk_gccspec_t *gccspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx, float *spec);

#ifdef __cplusplus
};
#endif
#endif
