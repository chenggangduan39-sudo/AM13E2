#ifndef WTK_BFIO_ASPEC_WTK_MASKZDSSPEC
#define WTK_BFIO_ASPEC_WTK_MASKZDSSPEC
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    int nbin;
    int channel;
    int ang_num;

    int max_theta;

    float **tdoa;
    float *t_nbins;

	wtk_complex_t ***ovec;
	wtk_complex_t ***a_ovec;
    wtk_complex_t *tmp;
}wtk_maskzdsspec_t;

wtk_maskzdsspec_t *wtk_maskzdsspec_new(int nbin,int channel,int ang_num);
void wtk_maskzdsspec_reset(wtk_maskzdsspec_t *maskzdsspec);
void wtk_maskzdsspec_delete(wtk_maskzdsspec_t *maskzdsspec);
void wtk_maskzdsspec_start(wtk_maskzdsspec_t *maskzdsspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye, int th_step);

wtk_maskzdsspec_t *wtk_maskzdsspec_new2(int nbin,int channel, int ang_num, float **mic_pos,  float sv, int rate, int use_line);
void wtk_maskzdsspec_reset2(wtk_maskzdsspec_t *maskzdsspec);
void wtk_maskzdsspec_delete2(wtk_maskzdsspec_t *maskzdsspec);
void wtk_maskzdsspec_start2(wtk_maskzdsspec_t *maskzdsspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx, int use_line, float ls_eye, int th_step);

float wtk_maskzdsspec_flush_spec_k(wtk_maskzdsspec_t *maskzdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx);
void wtk_maskzdsspec_flush_spec_k2(wtk_maskzdsspec_t *maskzdsspec, wtk_complex_t *scov,wtk_complex_t *ncov, float prob, int k, int ang_idx, float *spec);


#ifdef __cplusplus
};
#endif
#endif
