#ifndef WTK_BFIO_MASKDENOISE_WTK_FBANK_CFG_H
#define WTK_BFIO_MASKDENOISE_WTK_FBANK_CFG_H
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifndef MILLISECONDS_TO_SECONDS
#define MILLISECONDS_TO_SECONDS 0.001
#endif
#ifndef EPSILON
#define EPSILON 1.1920928955078125e-07
#endif
#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559005
#endif
typedef struct wtk_fbank_cfg wtk_fbank_cfg_t;

typedef struct wtk_kaldi_fbank_cfg
{
    float blackman_coeff;
    int channel;
    float dither;
    float energy_floor;
    float frame_length;
    float frame_shift;
    float high_freq;
    unsigned htk_compat:1;
    float low_freq;
    float min_duration;
    int num_mel_bins;
    float preemphasis_coefficient;
    unsigned raw_energy:1;
    unsigned remove_dc_offset:1;
    unsigned round_to_power_of_two:1;
    float sample_frequency;
    unsigned snip_edges:1;
    unsigned subtract_mean:1;
    unsigned use_energy:1;
    unsigned use_log_fbank:1;
    unsigned use_power:1;
    float vtln_high;
    float vtln_low;
    float vtln_warp;
    char *window_type;

    int window_shift;
    int window_size;
    int padded_window_size;

    float *window;

    int num_fft_bins;
    float nyquist;
    float fft_bin_width;
    float mel_low_freq;
    float mel_high_freq;
    float mel_freq_delta;
    float *bin;
    float *left_mel;
    float *center_mel;
    float *right_mel;
    float *center_freqs;
    float *mel;

    float **up_slope;
    float **down_slope;
    float **bins;

}wtk_kaldi_fbank_cfg_t;
struct wtk_fbank_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    int num_fbank;

    wtk_kaldi_fbank_cfg_t kaldi_fbank;

    unsigned use_kaldi_fbank:1;
};

int wtk_fbank_cfg_init(wtk_fbank_cfg_t *cfg);
int wtk_fbank_cfg_clean(wtk_fbank_cfg_t *cfg);
int wtk_fbank_cfg_update_local(wtk_fbank_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_fbank_cfg_update(wtk_fbank_cfg_t *cfg);
int wtk_fbank_cfg_update2(wtk_fbank_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_fbank_cfg_t* wtk_fbank_cfg_new(char *fn);
void wtk_fbank_cfg_delete(wtk_fbank_cfg_t *cfg);
wtk_fbank_cfg_t* wtk_fbank_cfg_new_bin(char *fn);
void wtk_fbank_cfg_delete_bin(wtk_fbank_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
