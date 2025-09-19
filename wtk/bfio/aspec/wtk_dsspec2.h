#ifndef WTK_BFIO_ASPEC_WTK_DSSPEC2
#define WTK_BFIO_ASPEC_WTK_DSSPEC2
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
}wtk_dsspec2_t;

wtk_dsspec2_t *wtk_dsspec2_new(int nbin,int channel, int npairs, int ang_num);
void wtk_dsspec2_reset(wtk_dsspec2_t *dsspec2);
void wtk_dsspec2_delete(wtk_dsspec2_t *dsspec2);
void wtk_dsspec2_start(wtk_dsspec2_t *dsspec2,  float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx);
void wtk_dsspec2_start2(wtk_dsspec2_t *dsspec2,  float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx);
// float wtk_dsspec2_flush_spec_k(wtk_dsspec2_t *dsspec2, float **pairs_m, wtk_complex_t *cov, int k, int ang_idx);
float wtk_dsspec2_flush_spec_k(wtk_dsspec2_t *dsspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
