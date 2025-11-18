#ifndef WTK_BFIO_ASPEC_WTK_NGCCSPEC2
#define WTK_BFIO_ASPEC_WTK_NGCCSPEC2
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int nbin;
    int channel;
    int npairs;
    int ang_num;

    float **alpha_meth;
    wtk_complex_t ***alpha;

    unsigned meth_init:1;
}wtk_ngccspec2_t;

wtk_ngccspec2_t *wtk_ngccspec2_new(int nbin,int channel,int npairs,int ang_num);
void wtk_ngccspec2_reset(wtk_ngccspec2_t *ngccspec2);
void wtk_ngccspec2_delete(wtk_ngccspec2_t *ngccspec2);
void wtk_ngccspec2_start(wtk_ngccspec2_t *ngccspec2,  float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m,int ang_idx);
void wtk_ngccspec2_start2(wtk_ngccspec2_t *ngccspec2,  float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m,int ang_idx);
// float wtk_ngccspec2_flush_spec_k(wtk_ngccspec2_t *ngccspec2, float **pairs_m, wtk_complex_t **fft, int k, int ang_idx);

float wtk_ngccspec2_flush_spec_k(wtk_ngccspec2_t *ngccspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

float wtk_ngccspec2_flush_spec_k2(wtk_ngccspec2_t *ngccspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
