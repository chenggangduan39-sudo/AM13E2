#ifndef __QTK_AUDIO_QTK_RESAMPLE_H__
#define __QTK_AUDIO_QTK_RESAMPLE_H__
#include "wtk/asr/fextra/pitch/audio/qtk_resample_cfg.h"
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_linear_resample qtk_linear_resample_t;
typedef struct qtk_arbitrary_resample qtk_arbitrary_resample_t;

struct qtk_linear_resample {
    qtk_linear_resample_cfg_t *cfg;
    int base_freq;
    int intput_samples_in_unit;
    int output_samples_in_unit;

    int64_t intput_sample_offset;
    int64_t output_sample_offset;

    int *first_index;
    float **weights;
    float *input_remainder;
    float *old_remainder;
    int input_remainder_dim;

    int *weights_dim;
};

int qtk_linear_resample_init(qtk_linear_resample_t *lr,
                             qtk_linear_resample_cfg_t *cfg);
void qtk_linear_resample_clean(qtk_linear_resample_t *lr);
int qtk_linear_resample_reset(qtk_linear_resample_t *lr);
float *qtk_linear_resample_process(qtk_linear_resample_t *lr, float *output,
                                   float *input, int len, int is_end);
int qtk_linear_resample_get_output_dim(qtk_linear_resample_t *lr, int len,
                                       int is_end);

struct qtk_arbitrary_resample {
    qtk_arbitrary_resample_cfg_t *cfg;
    int *first_index;
    float **weights;
    int *weights_dim;
};

int qtk_arbitrary_resample_init(qtk_arbitrary_resample_t *ar,
                                qtk_arbitrary_resample_cfg_t *cfg);
void qtk_arbitrary_resample_clean(qtk_arbitrary_resample_t *ar);
int qtk_arbitrary_resample_reset(qtk_arbitrary_resample_t *ar);
int qtk_arbitrary_resample_process(qtk_arbitrary_resample_t *ar, float *input,
                                   float *output, int irow, int icol);

#ifdef __cplusplus
};
#endif
#endif
