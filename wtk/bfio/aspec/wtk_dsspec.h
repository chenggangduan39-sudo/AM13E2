#ifndef WTK_BFIO_ASPEC_WTK_DSSPEC
#define WTK_BFIO_ASPEC_WTK_DSSPEC
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
    wtk_complex_t *tmp;
}wtk_dsspec_t;

wtk_dsspec_t *wtk_dsspec_new(int nbin,int channel,int ang_num);
void wtk_dsspec_reset(wtk_dsspec_t *dsspec);
void wtk_dsspec_delete(wtk_dsspec_t *dsspec);
void wtk_dsspec_start(wtk_dsspec_t *dsspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
void wtk_dsspec_start2(wtk_dsspec_t *dsspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx);
// float wtk_dsspec_flush_spec_k(wtk_dsspec_t *dsspec, wtk_complex_t *cov, float cov_travg, int k, int ang_idx);
float wtk_dsspec_flush_spec_k(wtk_dsspec_t *dsspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
