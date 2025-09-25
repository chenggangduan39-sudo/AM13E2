#ifndef WTK_BFIO_VBOX_WTK_VBOXBF6
#define WTK_BFIO_VBOX_WTK_VBOXBF6
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk_vboxebf6_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vboxebf6 wtk_vboxebf6_t;
typedef void (*wtk_vboxebf6_notify_f)(void *ths, short *output, int len);
typedef void (*wtk_vboxebf6_notify_ssl_f)(void *ths, float ts, float te,
                                          wtk_ssl2_extp_t *nbest_extp,
                                          int nbest);
typedef void (*wtk_vboxebf6_notify_eng_f)(void *ths, float energy, float snr);

typedef struct {
    wtk_vboxebf6_cfg_t *cfg;
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
} wtk_vboxebf6_edra_t;

struct wtk_vboxebf6 {
    wtk_vboxebf6_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **mix;
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

    wtk_complex_t **fft;
    wtk_complex_t **fft_sp;

    wtk_rls_t *erls;
    wtk_nlms_t *enlms;

    wtk_complex_t *fftx;
    wtk_complex_t *ffty;

    wtk_vboxebf6_edra_t *vdr;

    wtk_covm_t *covm;
    wtk_covm_t *echo_covm;
    wtk_bf_t *bf;

    int bfflushnf;

    wtk_maskssl_t *maskssl;
    wtk_maskssl2_t *maskssl2;

    float *out;
    short *mix_out;
    wtk_qmmse_t *qmmse2;
    wtk_qmmse_t *qmmse3;
    wtk_qmmse_t *qmmse4;
    wtk_qmmse_t *qmmse5;
    qtk_ahs_gain_controller_t *gc;
    wtk_complex_t *o_fft;
    wtk_complex_t *o_fft2;
    float *Yf;

    wtk_complex_t *leak_fft;
    float *leak_spec;
    float *leak1;
    float *leak2;
    float *Ef;

    wtk_equalizer_t *eq;
    wtk_limiter_t *limiter;

    void *ths;
    wtk_vboxebf6_notify_f notify;

    void *ssl_ths;
    wtk_vboxebf6_notify_ssl_f notify_ssl;

    void *eng_ths;
    wtk_vboxebf6_notify_eng_f notify_eng;

    int sp_silcnt;
    int sp_silcnt2;
    int mic_silcnt;

    int entropy_in_cnt;
    int entropy_out_cnt;
    int entropy_state;
    int entropy_state2;

    float inmic_scale;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;

    int gc_cnt;

    float last_aec_alg_scale;
    float last_denoise_alg_scale;
    float last_alg_scale;

    float mic_delay;
    float sp_delay;
    int mic_delay_samples;
    int sp_delay_samples;

    int agc_level;
    int ans_level;
    int aec_level;

    unsigned sp_sil : 1;
    unsigned sp_sil2 : 1;
    unsigned mic_sil : 1;
    unsigned agc_on : 1;

    unsigned agc_enable : 1;
    unsigned echo_enable : 1;
    unsigned denoise_enable : 1;
    unsigned ssl_enable : 1;
    unsigned sil_state : 1;
};

wtk_vboxebf6_t *wtk_vboxebf6_new(wtk_vboxebf6_cfg_t *cfg);
void wtk_vboxebf6_delete(wtk_vboxebf6_t *vboxebf6);
void wtk_vboxebf6_start(wtk_vboxebf6_t *vboxebf6);
void wtk_vboxebf6_reset(wtk_vboxebf6_t *vboxebf6);
void wtk_vboxebf6_set_notify(wtk_vboxebf6_t *vboxebf6, void *ths,
                             wtk_vboxebf6_notify_f notify);
void wtk_vboxebf6_set_ssl_notify(wtk_vboxebf6_t *vboxebf6, void *ths,
                                 wtk_vboxebf6_notify_ssl_f notify);
void wtk_vboxebf6_set_eng_notify(wtk_vboxebf6_t *vboxebf6, void *ths,
                                 wtk_vboxebf6_notify_eng_f notify);
/**
 * len=mic array samples
 */
void wtk_vboxebf6_feed(wtk_vboxebf6_t *vboxebf6, short *data, int len,
                       int is_end);

void wtk_vboxebf6_set_micvolume(wtk_vboxebf6_t *vboxebf6, float fscale);
void wtk_vboxebf6_set_agcenable(wtk_vboxebf6_t *vboxebf6, int enable);
void wtk_vboxebf6_set_agclevel(wtk_vboxebf6_t *vboxebf6, int level);
void wtk_vboxebf6_set_echoenable(wtk_vboxebf6_t *vboxebf6, int enable);
void wtk_vboxebf6_set_echolevel(wtk_vboxebf6_t *vboxebf6, int level);
void wtk_vboxebf6_set_denoiseenable(wtk_vboxebf6_t *vboxebf6, int enable);
void wtk_vboxebf6_set_denoisesuppress(wtk_vboxebf6_t *vboxebf6,
                                      float suppress); //[0-1]
void wtk_vboxebf6_set_denoiselevel(wtk_vboxebf6_t *vboxebf6, int level);
void wtk_vboxebf6_ssl_delay_new(wtk_vboxebf6_t *vboxebf6);

void wtk_vboxebf6_set_delay(wtk_vboxebf6_t *vboxebf6, float mic_delay,
                            float sp_delay);
#ifdef __cplusplus
};
#endif
#endif
