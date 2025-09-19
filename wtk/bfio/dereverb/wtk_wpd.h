#ifndef WTK_BFIO_DEREVERB_WTK_WPD
#define WTK_BFIO_DEREVERB_WTK_WPD
#include "wtk_wpd_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_wpd wtk_wpd_t;
struct wtk_wpd
{
    wtk_wpd_cfg_t *cfg;

    int nbin;
    int channel;
    int nl;
    float nframe;

    wtk_complex_t **norig;
    wtk_complex_t **norigs;
    float **masks;
    float **maskn;

    wtk_covm2_t *covm;

    wtk_complex_t *tmp_cov;
	wtk_dcomplex_t *tmp;

    wtk_complex_t *bfw;
};

wtk_wpd_t* wtk_wpd_new(wtk_wpd_cfg_t *cfg, int nbin);
void wtk_wpd_reset(wtk_wpd_t *wpd);
void wtk_wpd_start(wtk_wpd_t *wpd, int theta, int phi);
void wtk_wpd_delete(wtk_wpd_t *wpd);
void wtk_wpd_feed_k(wtk_wpd_t *wpd,wtk_complex_t *fft, wtk_complex_t *ffts, float *mask, wtk_complex_t *outfft, int k);
#ifdef __cplusplus
};
#endif
#endif