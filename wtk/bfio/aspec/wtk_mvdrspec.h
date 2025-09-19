#ifndef WTK_BFIO_ASPEC_WTK_MVDRSPEC
#define WTK_BFIO_ASPEC_WTK_MVDRSPEC
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
}wtk_mvdrspec_t;

wtk_mvdrspec_t *wtk_mvdrspec_new(int nbin,int channel,int ang_num);
void wtk_mvdrspec_reset(wtk_mvdrspec_t *mvdrspec);
void wtk_mvdrspec_delete(wtk_mvdrspec_t *mvdrspec);
void wtk_mvdrspec_start(wtk_mvdrspec_t *mvdrspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
void wtk_mvdrspec_start2(wtk_mvdrspec_t *mvdrspec,  float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx);
// float wtk_mvdrspec_flush_spec_k(wtk_mvdrspec_t *mvdrspec, wtk_complex_t *inv_cov, float cov_travg, int k, int ang_idx);
float wtk_mvdrspec_flush_spec_k(wtk_mvdrspec_t *mvdrspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);


#ifdef __cplusplus
};
#endif
#endif
