#ifndef WTK_BFIO_ASPEC_WTK_ASPECM_H
#define WTK_BFIO_ASPEC_WTK_ASPECM_H
#include "wtk/bfio/aspec/wtk_aspecm_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/core/wtk_fring.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_aspecm wtk_aspecm_t;

struct wtk_aspecm {
    wtk_aspecm_cfg_t *cfg;

    wtk_qmmse_t *qmmse;
    wtk_aspec_t *aspec;

    wtk_complex_t *cov;
    wtk_complex_t *inv_cov;
    wtk_dcomplex_t *tmp;
    float *cohv;
	wtk_complex_t **gcc_fft;
    wtk_fring_t *q_fring;

    int right_nf;
    float q_spec;
};

wtk_aspecm_t *wtk_aspecm_new(wtk_aspecm_cfg_t *cfg, int nbin, int channel);
void wtk_aspecm_delete(wtk_aspecm_t *aspecm);
void wtk_aspecm_start(wtk_aspecm_t *aspecm);
void wtk_aspecm_reset(wtk_aspecm_t *aspecm);
// fft (channel, nbin) fftx (nbin or NULL)
void wtk_aspecm_feed_aspec(wtk_aspecm_t *aspecm, wtk_complex_t **fft, wtk_complex_t *fftx);
#ifdef __cplusplus
};
#endif
#endif
