#ifndef WTK_BFIO_ASPEC_WTK_ZDSWSPEC
#define WTK_BFIO_ASPEC_WTK_ZDSWSPEC
#include "wtk/core/wtk_complex.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif
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
    float **B;
    float **LD;
}wtk_zdswspec_t;

wtk_zdswspec_t *wtk_zdswspec_new(int nbin,int channel,int ang_num);
void wtk_zdswspec_reset(wtk_zdswspec_t *zdswspec);
void wtk_zdswspec_delete(wtk_zdswspec_t *zdswspec);
void wtk_zdswspec_start(wtk_zdswspec_t *zdswspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye);
void wtk_zdswspec_start2(wtk_zdswspec_t *zdswspec,  float **mic_pos, float sv, int rate, float x, float y, float z, int ang_idx, int use_line, float ls_eye);
// float wtk_zdswspec_flush_spec_k(wtk_zdswspec_t *zdswspec, wtk_complex_t *cov, float cov_travg, int k, int ang_idx);
float wtk_zdswspec_flush_spec_k(wtk_zdswspec_t *zdswspec, float **pairs_m, wtk_complex_t **fft, float fftabs2, float cov_travg, 
                                                                                                wtk_complex_t *cov, wtk_complex_t *inv_cov, int k, int ang_idx);

#ifdef __cplusplus
};
#endif
#endif
