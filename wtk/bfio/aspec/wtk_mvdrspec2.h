#ifndef WTK_BFIO_ASPEC_WTK_MVDRSPEC2
#define WTK_BFIO_ASPEC_WTK_MVDRSPEC2
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
}wtk_mvdrspec2_t;

wtk_mvdrspec2_t *wtk_mvdrspec2_new(int nbin,int channel, int npairs, int ang_num);
void wtk_mvdrspec2_reset(wtk_mvdrspec2_t *mvdrspec2);
void wtk_mvdrspec2_delete(wtk_mvdrspec2_t *mvdrspec2);
void wtk_mvdrspec2_start(wtk_mvdrspec2_t *mvdrspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx);
void wtk_mvdrspec2_start2(wtk_mvdrspec2_t *mvdrspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx);
// float wtk_mvdrspec2_flush_spec_k(wtk_mvdrspec2_t *mvdrspec2, float **pairs_m, wtk_complex_t *cov, int k, int ang_idx);
float wtk_mvdrspec2_flush_spec_k(wtk_mvdrspec2_t *mvdrspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
