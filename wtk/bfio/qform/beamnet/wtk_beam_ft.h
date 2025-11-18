#ifndef WTK_BFIO_QFORM_BEAMNET_WTK_BEAMFT_H
#define WTK_BFIO_QFORM_BEAMNET_WTK_BEAMFT_H
#include "wtk/bfio/qform/beamnet/wtk_beam_ft_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_beam_ft wtk_beam_ft_t;

struct wtk_beam_ft {
    wtk_beam_ft_cfg_t *cfg;

    float *time_delay;
    wtk_complex_t *mean_mat;
    wtk_complex_t **input_norm;
    wtk_complex_t **mic_covar;
    wtk_complex_t ***ideal_phase_covar;
    wtk_complex_t **freq_covar;
    wtk_complex_t **ideal_phase_shift;
    float **freq_covar_sum_a;
    float **freq_covar_sum_b;
    float *x_mag;
    float *csa;
    float *csb;
    float *csna;
    float *csnb;

    float theta;
    float theta2;
    int ntheta;
    int *region_mask;
};

wtk_beam_ft_t *wtk_beam_ft_new(wtk_beam_ft_cfg_t *cfg);
void wtk_beam_ft_delete(wtk_beam_ft_t *beam_ft);
void wtk_beam_ft_reset(wtk_beam_ft_t *beam_ft);
void wtk_beam_ft_start(wtk_beam_ft_t *beam_ft, float theta, float theta2,
                       float phi);
void wtk_beam_ft_feed(wtk_beam_ft_t *beam_ft, wtk_complex_t **fft, int channel,
                      int nbin);

#ifdef __cplusplus
};
#endif
#endif
