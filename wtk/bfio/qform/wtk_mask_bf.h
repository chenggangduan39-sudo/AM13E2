#ifndef WTK_BFIO_QFORM_WTK_MASK_BF_H
#define WTK_BFIO_QFORM_WTK_MASK_BF_H
#include "wtk/bfio/qform/wtk_mask_bf_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mask_bf wtk_mask_bf_t;
typedef void (*wtk_mask_bf_notify_f)(void *ths, wtk_complex_t *output);

struct wtk_mask_bf {
    wtk_mask_bf_cfg_t *cfg;

    float *s_mask;
    float *n_mask;
    float *p_mask;
    float **power;
    float *s_power;
    float *n_power;
    float *eta;
    float *epsi;
    int *update_speech;
    int *update_noise;

    float *entropy_E;
    float *entropy_Eb;

    float scov_alpha;

    wtk_complex_t *fft_tmp;
    wtk_dcomplex_t *fft_tmp2;
    wtk_complex_t **covar;
    wtk_complex_t **Rss;
    wtk_complex_t **Rnn;
    wtk_complex_t **Rss_e;
    wtk_complex_t **Rnn_e;
    wtk_complex_t **Rss_norm;
    wtk_complex_t **Rnn_norm;
    wtk_complex_t *Rnn_;
    wtk_complex_t *Rss_;
    wtk_complex_t **vec;
    wtk_complex_t **w;
    wtk_complex_t *c_temp;
    wtk_complex_t *fftx;
    float *Rss_cnt;
    float *Rnn_cnt;
    float *Rss_cnt_e;
    float *Rnn_cnt_e;
    int *scnt;
    int *ncnt;

    float *s_power_acc;
    float *n_power_acc;
    float *s_power_acc_e;
    float *n_power_acc_e;
    float *speech_power;
    float *noise_power;
    float *snr;
    float snr_acc;
    float snr_acc_e;
    float s_power_cnt;
    float n_power_cnt;
    float s_power_cnt_e;
    float n_power_cnt_e;
    int ref_channel;

    int nframe;
    int update_w;

    void *ths;
    wtk_mask_bf_notify_f notify;
};

wtk_mask_bf_t *wtk_mask_bf_new(wtk_mask_bf_cfg_t *cfg);
void wtk_mask_bf_delete(wtk_mask_bf_t *mask_bf);
void wtk_mask_bf_start(wtk_mask_bf_t *mask_bf);
void wtk_mask_bf_reset(wtk_mask_bf_t *mask_bf);
void wtk_mask_bf_feed(wtk_mask_bf_t *mask_bf, wtk_complex_t **fft, float *speech_mask,
                  float *noise_mask, float *post_mask, int sp_sil);
void wtk_mask_bf_set_notify(wtk_mask_bf_t *mask_bf, void *ths, wtk_mask_bf_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
