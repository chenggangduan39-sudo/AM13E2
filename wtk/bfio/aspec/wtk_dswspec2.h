#ifndef WTK_BFIO_ASPEC_WTK_DSWSPEC2
#define WTK_BFIO_ASPEC_WTK_DSWSPEC2
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

    wtk_complex_t ***alpha;
    float **sinc;

    unsigned sinc_init:1;
}wtk_dswspec2_t;

wtk_dswspec2_t *wtk_dswspec2_new(int nbin,int channel, int npairs, int ang_num);
void wtk_dswspec2_reset(wtk_dswspec2_t *dswspec2);
void wtk_dswspec2_delete(wtk_dswspec2_t *dswspec2);
void wtk_dswspec2_start(wtk_dswspec2_t *dswspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx);
void wtk_dswspec2_start2(wtk_dswspec2_t *dswspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx);
// float wtk_dswspec2_flush_spec_k(wtk_dswspec2_t *dswspec2, float **pairs_m, wtk_complex_t *cov, int k, int ang_idx);
float wtk_dswspec2_flush_spec_k(wtk_dswspec2_t *dswspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
