#ifndef __QTK_AUDIO_QTK_RESAMPLE_CFG_H__
#define __QTK_AUDIO_QTK_RESAMPLE_CFG_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_linear_resample_cfg qtk_linear_resample_cfg_t;
typedef struct qtk_arbitrary_resample_cfg qtk_arbitrary_resample_cfg_t;

struct qtk_linear_resample_cfg {
    int samp_rate_in_hz;
    int samp_rate_out_hz;
    float filter_cutoff_hz;
    int num_zeros;
    float window_width;
};

int qtk_linear_resample_cfg_init(qtk_linear_resample_cfg_t *cfg);
int qtk_linear_resample_cfg_clean(qtk_linear_resample_cfg_t *cfg);
int qtk_linear_resample_cfg_update(qtk_linear_resample_cfg_t *cfg);

struct qtk_arbitrary_resample_cfg {
    int num_samples_in;
    float samp_rate_in;
    float *sample_points;
    int sample_points_dim;
    int num_zeros;
    float filter_cutoff;
};

int qtk_arbitrary_resample_cfg_init(qtk_arbitrary_resample_cfg_t *cfg);
int qtk_arbitrary_resample_cfg_clean(qtk_arbitrary_resample_cfg_t *cfg);
int qtk_arbitrary_resample_cfg_update(qtk_arbitrary_resample_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
