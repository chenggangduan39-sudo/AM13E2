#ifndef WTK_BFIO_ASPEC_WTK_DNMSPEC
#define WTK_BFIO_ASPEC_WTK_DNMSPEC
#include "wtk/core/wtk_complex.h"
#include "wtk/bfio/eig/wtk_eig.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int nbin;
    int channel;
    int ang_num;

    wtk_complex_t ***A;
    wtk_complex_t ***LA;

    wtk_complex_t *ARA;
    wtk_complex_t *tmp;

    wtk_eig_t *eig;
}wtk_dnmspec_t;

wtk_dnmspec_t *wtk_dnmspec_new(int nbin,int channel, int ang_num);
void wtk_dnmspec_reset(wtk_dnmspec_t *dnmspec);
void wtk_dnmspec_delete(wtk_dnmspec_t *dnmspec);
void wtk_dnmspec_start(wtk_dnmspec_t *dnmspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
void wtk_dnmspec_start2(wtk_dnmspec_t *dnmspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx);
// float wtk_dnmspec_flush_spec_k(wtk_dnmspec_t *dnmspec, wtk_complex_t *cov, int k, int ang_idx);
float wtk_dnmspec_flush_spec_k(wtk_dnmspec_t *dnmspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
