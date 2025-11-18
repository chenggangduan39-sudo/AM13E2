#ifndef WTK_BFIO_ASPEC_WTK_MASKGCCSPEC
#define WTK_BFIO_ASPEC_WTK_MASKGCCSPEC
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
}wtk_maskgccspec_t;

wtk_maskgccspec_t *wtk_maskgccspec_new(int nbin,int channel,int ang_num);
void wtk_maskgccspec_reset(wtk_maskgccspec_t *maskgccspec);
void wtk_maskgccspec_delete(wtk_maskgccspec_t *maskgccspec);
void wtk_maskgccspec_start(wtk_maskgccspec_t *maskgccspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
float wtk_maskgccspec_flush_spec_k(wtk_maskgccspec_t *maskgccspec, wtk_complex_t *fft, float prob, int k, int ang_idx);
void wtk_maskgccspec_flush_spec_k2(wtk_maskgccspec_t *maskgccspec, wtk_complex_t *fft, float prob, int k, int ang_idx, float *spec);


#ifdef __cplusplus
};
#endif
#endif
