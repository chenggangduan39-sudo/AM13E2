#ifndef WTK_BFIO_VBOX_WTK_VBOXBF3
#define WTK_BFIO_VBOX_WTK_VBOXBF3
#include "wtk/bfio/ahs/qtk_kalmanv3.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk_vboxebf3_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vboxebf3 wtk_vboxebf3_t;
typedef void (*wtk_vboxebf3_notify_f)(void *ths, short *output, int len);
typedef void (*wtk_vboxebf3_notify_ssl_f)(void *ths, float ts, float te,
                                          wtk_ssl2_extp_t *nbest_extp,
                                          int nbest);
typedef void (*wtk_vboxebf3_notify_eng_f)(void *ths, float energy, float snr);

typedef struct {
    wtk_vboxebf3_cfg_t *cfg;
    int nbin;

    wtk_bankfeat_t *bank_mic;
    wtk_bankfeat_t *bank_sp;

    float *feature_sp;

    float *g;
    float *lastg;
    float *gf;
    float *ssl_gf;

    float *g2;
    float *lastg2;
    float *gf2;

    wtk_gainnet2_t *gainnet2;

    wtk_gainnet7_t *gainnet7;

    wtk_gainnet_t *agc_gainnet;

    wtk_qmmse_t *qmmse;
} wtk_vboxebf3_edra_t;

struct wtk_vboxebf3 {
    wtk_vboxebf3_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **mic2;
    wtk_strbuf_t **sp;

    float *analysis_window;
    float *synthesis_window;
    wtk_drft_t *rfft;
    float *rfft_in;
    int nbin;
    float **analysis_mem;
    float **analysis_mem_sp;
    float *synthesis_mem;
    float **mul_synthesis_mem;

    wtk_complex_t **fft;
    wtk_complex_t **fft_sp;

    wtk_rls_t *erls;
    wtk_nlms_t *enlms;
    qtk_ahs_kalman_t *ekalman;

    wtk_complex_t *fftx;
    wtk_complex_t *ffty;

    wtk_vboxebf3_edra_t *vdr;

    wtk_covm_t *covm;
    wtk_covm_t *echo_covm;
    wtk_bf_t *bf;

    int bfflushnf;

    float *sim_scov;
    float *sim_ncov;
    float *sim_echo_scov;
    float *sim_echo_ncov;
    int *sim_cnt_sum;
    int *sim_echo_cnt_sum;

    float **mul_scov;
    float **mul_ncov;
    float **mul_echo_scov;
    float **mul_echo_ncov;
    int *mul_cnt_sum;
    int *mul_echo_cnt_sum;

    float *mask_mu;
    float *mask_mu2;

    float *entropy_E;
    float *entropy_Eb;
    int mu_entropy_cnt;

    wtk_qmmse_t *qmmse2;
    qtk_ahs_gain_controller_t *gc;

    wtk_maskssl_t *maskssl;
    wtk_maskssl2_t *maskssl2;

    float *mul_out;
    float *out;

    wtk_equalizer_t *eq;
    wtk_limiter_t *limiter;

    void *ths;
    wtk_vboxebf3_notify_f notify;

    void *ssl_ths;
    wtk_vboxebf3_notify_ssl_f notify_ssl;

    void *eng_ths;
    wtk_vboxebf3_notify_eng_f notify_eng;

    int sp_silcnt;
    int mic_silcnt;

    float inmic_scale;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    int gc_cnt;

    int eng_cnt;

    int gf_mask_cnt;

    float mic_delay;
    float sp_delay;
    int mic_delay_samples;
    int sp_delay_samples;

    int agc_level;
    int ans_level;
    int aec_level;

    unsigned sp_sil : 1;
    unsigned mic_sil : 1;
    unsigned agc_on : 1;

    unsigned agc_enable : 1;
    unsigned echo_enable : 1;
    unsigned denoise_enable : 1;
    unsigned eq_enable : 1;
    unsigned ssl_enable : 1;
};

wtk_vboxebf3_t *wtk_vboxebf3_new(wtk_vboxebf3_cfg_t *cfg);
void wtk_vboxebf3_delete(wtk_vboxebf3_t *vboxebf3);
void wtk_vboxebf3_start(wtk_vboxebf3_t *vboxebf3);
void wtk_vboxebf3_reset(wtk_vboxebf3_t *vboxebf3);
void wtk_vboxebf3_set_notify(wtk_vboxebf3_t *vboxebf3, void *ths,
                             wtk_vboxebf3_notify_f notify);
void wtk_vboxebf3_set_ssl_notify(wtk_vboxebf3_t *vboxebf3, void *ths,
                                 wtk_vboxebf3_notify_ssl_f notify);
void wtk_vboxebf3_set_eng_notify(wtk_vboxebf3_t *vboxebf3, void *ths,
                                 wtk_vboxebf3_notify_eng_f notify);
/**
 * len=mic array samples
 */
void wtk_vboxebf3_feed(wtk_vboxebf3_t *vboxebf3, short *data, int len,
                       int is_end);
void wtk_vboxebf3_feed_mul(wtk_vboxebf3_t *vboxebf3, short *data, int len,
                           int is_end);

void wtk_vboxebf3_set_micvolume(wtk_vboxebf3_t *vboxebf3, float fscale);
void wtk_vboxebf3_set_agcenable(wtk_vboxebf3_t *vboxebf3, int enable);
void wtk_vboxebf3_set_agclevel(wtk_vboxebf3_t *vboxebf3, int level);
void wtk_vboxebf3_set_echoenable(wtk_vboxebf3_t *vboxebf3, int enable);
void wtk_vboxebf3_set_echolevel(wtk_vboxebf3_t *vboxebf3, int level);
void wtk_vboxebf3_set_denoiseenable(wtk_vboxebf3_t *vboxebf3, int enable);
void wtk_vboxebf3_set_denoisesuppress(wtk_vboxebf3_t *vboxebf3,
                                      float suppress); //[0-1]
void wtk_vboxebf3_set_denoiselevel(wtk_vboxebf3_t *vboxebf3, int level);
void wtk_vboxebf3_set_eqenable(wtk_vboxebf3_t *vboxebf3, int enable);
void wtk_vboxebf3_ssl_delay_new(wtk_vboxebf3_t *vboxebf3);
void wtk_vboxebf3_set_sslenable(wtk_vboxebf3_t *vboxebf3, int enable);

void wtk_vboxebf3_set_delay(wtk_vboxebf3_t *vboxebf3, float mic_delay,
                            float sp_delay);
#ifdef __cplusplus
};
#endif
#endif
