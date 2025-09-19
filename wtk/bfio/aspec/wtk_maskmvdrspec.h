#ifndef WTK_BFIO_ASPEC_WTK_MASKMVDRSPEC
#define WTK_BFIO_ASPEC_WTK_MASKMVDRSPEC
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
}wtk_maskmvdrspec_t;

wtk_maskmvdrspec_t *wtk_maskmvdrspec_new(int nbin,int channel,int ang_num);
void wtk_maskmvdrspec_reset(wtk_maskmvdrspec_t *maskmvdrspec);
void wtk_maskmvdrspec_delete(wtk_maskmvdrspec_t *maskmvdrspec);
void wtk_maskmvdrspec_start(wtk_maskmvdrspec_t *maskmvdrspec,  float **mic_pos, float sv, int rate, float theta, float phi, int ang_idx);
float wtk_maskmvdrspec_flush_spec_k(wtk_maskmvdrspec_t *maskmvdrspec, wtk_complex_t *scov,wtk_complex_t *inv_cov, float prob, int k, int ang_idx);
void wtk_maskmvdrspec_flush_spec_k2(wtk_maskmvdrspec_t *maskmvdrspec, wtk_complex_t *scov,wtk_complex_t *inv_cov, float prob, int k, int ang_idx, float *spec);


#ifdef __cplusplus
};
#endif
#endif
