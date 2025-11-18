#ifndef EB30729D_E672_4E76_B1FE_037A7CFB8F6B
#define EB30729D_E672_4E76_B1FE_037A7CFB8F6B

#include "qtk/ult/qtk_ult_msc2d_cfg.h"
#include "wtk/bfio/eig/wtk_eig.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/signal/qtk_fft.h"
#include "wtk/signal/wtk_umean.h"

typedef struct qtk_ult_msc2d qtk_ult_msc2d_t;

struct qtk_ult_msc2d {
    qtk_ult_msc2d_cfg_t *cfg;
    wtk_eig_t *eig;
    wtk_complex_t *xcorr;
    wtk_complex_t *xcorr_smoothed;
    wtk_complex_t *neigm_smoothed;
    wtk_umean_t *mean;
    wtk_complex_t *eig_v;
    float *rk_v;
    float *rk_v2;
    float *rk_vi;
    wtk_complex_t *steer;
    int K;
    int L;
    int nframe;

    qtk_cfft_t *fft;
    wtk_complex_t *frame;
};

qtk_ult_msc2d_t *qtk_ult_msc2d_new(qtk_ult_msc2d_cfg_t *cfg);
void qtk_ult_msc2d_delete(qtk_ult_msc2d_t *s);
int qtk_ult_msc2d_feed(qtk_ult_msc2d_t *s, wtk_complex_t *cfr, int range_idx_s,
                       int nrange, int angle_idx_s, int nangle, float *prob);
int qtk_ult_msc2d_update_steer(qtk_ult_msc2d_t *s, float df, float fc,
                               float angle_s, float angle_e, float angle_unit,
                               float range_s, float range_e, float range_unit);

#endif /* EB30729D_E672_4E76_B1FE_037A7CFB8F6B */
