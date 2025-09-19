#ifndef QTK_ULT_QTK_ULTM2_CFG
#define QTK_ULT_QTK_ULTM2_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/fft/wtk_stft.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ultm2_cfg qtk_ultm2_cfg_t;
struct qtk_ultm2_cfg {
    int channel;
    int rate;
    int ws1;
    int ws2;
    int mic_fft;
    int echo_fft;
    float *mic_kernel;
    float *echo_kernel;
    int mic_h_fs;
    int mic_h_fe;
    int mic_h_fft;
    float *mic_h_kernel;
    int s2f_size;
    float mic_skip_f;
    float mic_min_f;
    float mic_min_f2;
    float echo_min_f;
    float echo_min_f2;
    float echo_range1;
    float echo_range2;
    int echo_hint1;
    int echo_hint2;
    int cut_hint2;
    int cut_hint;
    float scale;
    float harm_thresh;
    float harm_thresh2;
    int harm_hint;
    int harm_hint2;

    float *r_zc_pad_a;
    float *r_zc_pad_b;
    int L;
    int align_cnt;

    char use_aligned;
    char use_harmonic_d;
    char use_echo;
    char debug;
    unsigned int use_echo_align:1;
};

int qtk_ultm2_cfg_init(qtk_ultm2_cfg_t *cfg);
int qtk_ultm2_cfg_clean(qtk_ultm2_cfg_t *cfg);
int qtk_ultm2_cfg_update_local(qtk_ultm2_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_ultm2_cfg_update(qtk_ultm2_cfg_t *cfg);
float *qtk_ultm2_cfg_get_signal(qtk_ultm2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
