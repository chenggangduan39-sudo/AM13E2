#ifndef WTK_BFIO_ASPEC_WTK_DNMSPEC2
#define WTK_BFIO_ASPEC_WTK_DNMSPEC2
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
    
    float ***invA1;
    float ***invA2;
    wtk_complex_t ***invAx;
    wtk_complex_t ***invAx2;
    float ***invL1;
    float ***invL2;
}wtk_dnmspec2_t;

wtk_dnmspec2_t *wtk_dnmspec2_new(int nbin,int channel, int npairs, int ang_num);
void wtk_dnmspec2_reset(wtk_dnmspec2_t *dnmspec2);
void wtk_dnmspec2_delete(wtk_dnmspec2_t *dnmspec2);
void wtk_dnmspec2_start(wtk_dnmspec2_t *dnmspec2, float **mic_pos, float sv, int rate, float theta, float phi ,float **pairs_m, int ang_idx);
void wtk_dnmspec2_start2(wtk_dnmspec2_t *dnmspec2, float **mic_pos, float sv, int rate, float x, float y, float z, float **pairs_m, int ang_idx);
// float wtk_dnmspec2_flush_spec_k(wtk_dnmspec2_t *dnmspec2, float **pairs_m, wtk_complex_t *cov, int k, int ang_idx);
float wtk_dnmspec2_flush_spec_k(wtk_dnmspec2_t *dnmspec2, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
