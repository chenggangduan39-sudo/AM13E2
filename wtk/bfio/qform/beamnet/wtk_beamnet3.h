#ifndef WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET3_H
#define WTK_BFIO_QFORM_BEAMNET_WTK_BEAMNET3_H
#include "wtk/bfio/qform/beamnet/wtk_beamnet3_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_beamnet3 wtk_beamnet3_t;
typedef enum wtk_beamnet3_state wtk_beamnet3_state_e;
typedef enum wtk_beamnet3_event_type wtk_beamnet3_event_type_e;
typedef void(*wtk_beamnet3_notify_f)(void *ths,short *output,int len);
typedef void (*wtk_beamnet3_notify_f2)(void *ths2, wtk_complex_t *fft,
                                       int nbin);

struct wtk_beamnet3 {
    wtk_beamnet3_cfg_t *cfg;

    wtk_strbuf_t **mic;
    wtk_strbuf_t **sp;
    float theta;

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
    wtk_complex_t *fftx;

    float *out;

    void *ths;
    void *ths2;
    wtk_beamnet3_notify_f notify;
    wtk_beamnet3_notify_f2 notify2;

#ifdef ONNX_DEC
    qtk_onnxruntime_t *seperator;
    qtk_onnxruntime_t *beamformer;
    OrtValue **sep_caches;
    OrtValue **beam_caches;
    int *seperator_out_len;
    int *beamformer_out_len;
#endif

    float **feature;
    float *x;
    float *mix_spec;
    float *time_delay;
    float *mic_aptd;
    float *echo_aptd;
    wtk_complex_t **mic_covar;
    wtk_complex_t **ideal_phase_covar;
    wtk_complex_t **ideal_phase_shift;

    wtk_complex_t ***ideal_phase_covars;
    wtk_complex_t ***ideal_phase_shifts;

    wtk_complex_t **freq_covar;
    wtk_complex_t *freq_covar_sum;
    float *LPS;
    float *echo_LPS;
    float **IPDs;
    float *FDDF;
    float *direct_features;
    wtk_complex_t *mean_mat;
    wtk_complex_t **input_norm;

    float *speech_est_real;
    float *speech_est_imag;
    float *noise_est_real;
    float *noise_est_imag;
    wtk_complex_t **speech_covar;
    wtk_complex_t **noise_covar;
    float *beamformer_x;
    float *src_est_real;
    float *src_est_imag;

    float *mix_aptd;
    float *speech_aptd;
    float *noise_aptd;

    float *entropy_E;
    float *entropy_Eb;
    int entropy_silcnt;
    int entropy_in_cnt;
    unsigned entropy_sil:1;

    int delay_nf;
    wtk_complex_t *last_fftx;

    float *gf;
    float *last_gf;

    int nframe;

    wtk_qmmse_t *qmmse;
    qtk_ahs_gain_controller_t *gc;
    wtk_bf_t *bf;
    wtk_covm_t *covm;

    int bfflushnf;

    float bs_scale;
    float bs_last_scale;
    float bs_real_scale;
    int bs_max_cnt;
    float *bs_win;
    int gc_cnt;
    int gc_cnt2;

    float *sim_scov;
    float *sim_ncov;
    int *sim_cnt_sum;

    float *raw_mask;

    int *region_mask;

    unsigned int agc_enable:1;
    unsigned int denoise_enable:1;
};

wtk_beamnet3_t *wtk_beamnet3_new(wtk_beamnet3_cfg_t *cfg);
void wtk_beamnet3_delete(wtk_beamnet3_t *beamnet3);
void wtk_beamnet3_start(wtk_beamnet3_t *beamnet3, float theta, float phi);
void wtk_beamnet3_reset(wtk_beamnet3_t *beamnet3);
void wtk_beamnet3_feed(wtk_beamnet3_t *beamnet3, short *data, int len, int is_end);

void wtk_beamnet3_set_notify(wtk_beamnet3_t *beamnet3, void *ths,
                             wtk_beamnet3_notify_f notify);

void wtk_beamnet3_set_denoiseenable(wtk_beamnet3_t *beamnet3,int enable);
void wtk_beamnet3_set_noise_suppress(wtk_beamnet3_t *beamnet3,float noise_suppress); //[1-100]
void wtk_beamnet3_set_out_scale(wtk_beamnet3_t *beamnet3,float scale);
void wtk_beamnet3_set_agcenable(wtk_beamnet3_t *beamnet3,int enable);

wtk_beamnet3_t *wtk_beamnet3_new2(wtk_beamnet3_cfg_t *cfg);
void wtk_beamnet3_delete2(wtk_beamnet3_t *beamnet3);
void wtk_beamnet3_start2(wtk_beamnet3_t *beamnet3, float theta, float phi);
void wtk_beamnet3_reset2(wtk_beamnet3_t *beamnet3);
void wtk_beamnet3_feed_fft(wtk_beamnet3_t *beamnet3, wtk_complex_t **fft,
                           int len, int is_end);
void wtk_beamnet3_set_notify2(wtk_beamnet3_t *beamnet3,void *ths2,wtk_beamnet3_notify_f2 notify2);

int wtk_beamnet3_set_query_region(wtk_beamnet3_t *beamnet3,float theta1, float theta2);

#ifdef __cplusplus
};
#endif
#endif
