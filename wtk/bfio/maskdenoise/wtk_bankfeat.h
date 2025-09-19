#ifndef WTK_BFIO_MASKDENOISE_WTK_BANKFEAT
#define WTK_BFIO_MASKDENOISE_WTK_BANKFEAT
#include "wtk_bankfeat_cfg.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_bankfeat wtk_bankfeat_t;

struct wtk_bankfeat
{
	wtk_bankfeat_cfg_t *cfg;

    float  *Ex;
    float *dct_table;
    float **cepstral_mem;
    int memid;

    float *features;

    unsigned silence:1;
};

wtk_bankfeat_t *wtk_bankfeat_new(wtk_bankfeat_cfg_t *cfg);
void wtk_bankfeat_delete(wtk_bankfeat_t *bankfeat);
void wtk_bankfeat_reset(wtk_bankfeat_t *bankfeat);
void wtk_bankfeat_flush_frame_features(wtk_bankfeat_t *bankfeat, wtk_complex_t *fftx);

void wtk_bankfeat_interp_band_gain(wtk_bankfeat_t *bankfeat, int nbin, float *g, const float *bandE);
void wtk_bankfeat_compute_band_energy(wtk_bankfeat_t *bankfeat, float *bandE, wtk_complex_t *fft);
void wtk_bankfeat_compute_band_energy2(wtk_bankfeat_t *bankfeat, float *bandE, wtk_complex_t *fft, float *mask);
#ifdef __cplusplus
};
#endif
#endif