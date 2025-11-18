#ifndef WTK_BFIO_ASPEC_WTK_MVDRWSPEC
#define WTK_BFIO_ASPEC_WTK_MVDRWSPEC
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
    float **B;
    wtk_complex_t *tmp;
}wtk_mvdrwspec_t;

wtk_mvdrwspec_t *wtk_mvdrwspec_new(int nbin,int channel,int ang_num);
void wtk_mvdrwspec_reset(wtk_mvdrwspec_t *mvdrwspec);
void wtk_mvdrwspec_delete(wtk_mvdrwspec_t *mvdrwspec);
void wtk_mvdrwspec_start(wtk_mvdrwspec_t *mvdrwspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
void wtk_mvdrwspec_start2(wtk_mvdrwspec_t *mvdrwspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx);
// float wtk_mvdrwspec_flush_spec_k(wtk_mvdrwspec_t *mvdrwspec, wtk_complex_t *inv_cov, float cov_travg, int k, int ang_idx);
float wtk_mvdrwspec_flush_spec_k(wtk_mvdrwspec_t *mvdrwspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);
#ifdef __cplusplus
};
#endif
#endif
