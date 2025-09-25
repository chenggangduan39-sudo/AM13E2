#ifndef WTK_BFIO_ASPEC_WTK_MASKDSSPEC
#define WTK_BFIO_ASPEC_WTK_MASKDSSPEC
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
}wtk_maskdsspec_t;

wtk_maskdsspec_t *wtk_maskdsspec_new(int nbin,int channel,int ang_num);
void wtk_maskdsspec_reset(wtk_maskdsspec_t *maskdsspec);
void wtk_maskdsspec_delete(wtk_maskdsspec_t *maskdsspec);
void wtk_maskdsspec_start(wtk_maskdsspec_t *maskdsspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
float wtk_maskdsspec_flush_spec_k(wtk_maskdsspec_t *maskdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx);
void wtk_maskdsspec_flush_spec_k2(wtk_maskdsspec_t *maskdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx, float *spec);


#ifdef __cplusplus
};
#endif
#endif
