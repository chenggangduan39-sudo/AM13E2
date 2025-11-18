#ifndef WTK_BFIO_ASPEC_WTK_ZDSSPEC
#define WTK_BFIO_ASPEC_WTK_ZDSSPEC
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
}wtk_zdsspec_t;

wtk_zdsspec_t *wtk_zdsspec_new(int nbin,int channel,int ang_num);
void wtk_zdsspec_reset(wtk_zdsspec_t *zdsspec);
void wtk_zdsspec_delete(wtk_zdsspec_t *zdsspec);
void wtk_zdsspec_start(wtk_zdsspec_t *zdsspec, float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye);
void wtk_zdsspec_start2(wtk_zdsspec_t *zdsspec, float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx,int use_line, float ls_eye);
float wtk_zdsspec_flush_spec_k(wtk_zdsspec_t *zdsspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
