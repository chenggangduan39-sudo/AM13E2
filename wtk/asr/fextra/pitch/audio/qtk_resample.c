#include "wtk/asr/fextra/pitch/audio/qtk_resample.h"
#include "wtk/asr/fextra/pitch/math/qtk_math.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_type.h"

static float _lr_filter_func(qtk_linear_resample_t *lr, float t) {
    float window, filter, filter_cutoff;
    int num_zeros;

    num_zeros = lr->cfg->num_zeros;
    filter_cutoff = lr->cfg->filter_cutoff_hz;

    window = fabs(t) < num_zeros / (2.0 * filter_cutoff)
                 ? 0.5 * (1 + cos(M_2PI * filter_cutoff / num_zeros * t))
                 : 0.0;
    filter = t != 0 ? sin(M_2PI * filter_cutoff * t) / (M_PI * t)
                    : 2 * filter_cutoff;

    return filter * window;
}

static int _lr_set_index_and_weights(qtk_linear_resample_t *lr) {
    double window_width;
    int idx;

    lr->first_index = wtk_malloc(sizeof(int) * lr->output_samples_in_unit);
    lr->weights = wtk_malloc(sizeof(float *) * lr->output_samples_in_unit);
    lr->weights_dim = wtk_malloc(sizeof(int) * lr->output_samples_in_unit);

    window_width = lr->cfg->window_width;

    for (idx = 0; idx < lr->output_samples_in_unit; idx++) {
        double input_t, output_t, delta_t, min_t, max_t;
        int min_input_idx, max_indput_idx, num_indices, idxj, input_idx;

        output_t = idx / cast(double, lr->cfg->samp_rate_out_hz);
        min_t = output_t - window_width;
        max_t = output_t + window_width;

        min_input_idx = ceil(min_t * lr->cfg->samp_rate_in_hz);
        max_indput_idx = floor(max_t * lr->cfg->samp_rate_in_hz);
        num_indices = max_indput_idx - min_input_idx + 1;

        lr->first_index[idx] = min_input_idx;
        lr->weights_dim[idx] = num_indices;
        lr->weights[idx] = wtk_malloc(sizeof(float) * num_indices);

        for (idxj = 0; idxj < num_indices; idxj++) {
            input_idx = min_input_idx + idxj;
            input_t = input_idx / cast(double, lr->cfg->samp_rate_in_hz);
            delta_t = input_t - output_t;
            lr->weights[idx][idxj] =
                _lr_filter_func(lr, delta_t) / lr->cfg->samp_rate_in_hz;
        }
    }

    return 0;
}

static void _lr_get_index(qtk_linear_resample_t *lr, int64_t samp_out,
                          int64_t *first_samp_in, int *samp_out_wrapped) {
    int64_t unit_idx = samp_out / lr->output_samples_in_unit;
    *samp_out_wrapped =
        cast(int, samp_out - unit_idx * lr->output_samples_in_unit);
    *first_samp_in = lr->first_index[*samp_out_wrapped] +
                     unit_idx * lr->intput_samples_in_unit;
}

static int64_t _lr_get_num_outputsamples(qtk_linear_resample_t *lr,
                                         int64_t input_num_sample, int is_end) {
    int tick_freq, window_width_ticks;
    int ticks_per_output_period, ticks_per_input_period;
    int64_t last_output_samp;
    int64_t interval_len_in_ticks;

    tick_freq = qtk_lcm(lr->cfg->samp_rate_in_hz, lr->cfg->samp_rate_out_hz);
    ticks_per_input_period = tick_freq / lr->cfg->samp_rate_in_hz;
    interval_len_in_ticks = input_num_sample * ticks_per_input_period;

    if (!is_end) {
        window_width_ticks = floor(lr->cfg->window_width * tick_freq);
        interval_len_in_ticks -= window_width_ticks;
    }

    if (interval_len_in_ticks <= 0) {
        return 0;
    }

    ticks_per_output_period = tick_freq / lr->cfg->samp_rate_out_hz;
    last_output_samp = interval_len_in_ticks / ticks_per_output_period;

    if (last_output_samp * ticks_per_output_period == interval_len_in_ticks) {
        last_output_samp--;
    }

    return last_output_samp + 1;
}

static void _lr_set_remainder(qtk_linear_resample_t *lr, float *input,
                              int input_dim) {
    int idx, input_idx;
    float *tmp;
    int old_remainder_dim;

    tmp = lr->input_remainder;
    lr->input_remainder = lr->old_remainder;
    old_remainder_dim = lr->input_remainder_dim;

    if (lr->input_remainder_dim == 0) {
        lr->input_remainder_dim =
            ceil(lr->cfg->samp_rate_in_hz * lr->cfg->num_zeros /
                     lr->cfg->filter_cutoff_hz);
    }
    lr->old_remainder = tmp;

    for (idx = -lr->input_remainder_dim; idx < 0; idx++) {
        input_idx = idx + input_dim;
        if (input_idx >= 0) {
            lr->input_remainder[idx + lr->input_remainder_dim] =
                input[input_idx];
        } else if (input_idx + old_remainder_dim >= 0) {
            lr->input_remainder[idx + lr->input_remainder_dim] =
                lr->old_remainder[input_idx + old_remainder_dim];
        }
    }
}

int qtk_linear_resample_init(qtk_linear_resample_t *lr,
                             qtk_linear_resample_cfg_t *cfg) {
    memset(lr, 0, sizeof(*lr));
    int input_remainder_dim;
    lr->cfg = cfg;
    lr->base_freq = qtk_gcd(cfg->samp_rate_in_hz, cfg->samp_rate_out_hz);
    lr->intput_samples_in_unit = cfg->samp_rate_in_hz / lr->base_freq;
    lr->output_samples_in_unit = cfg->samp_rate_out_hz / lr->base_freq;
    input_remainder_dim =
        ceil(lr->cfg->samp_rate_in_hz * lr->cfg->num_zeros /
                 lr->cfg->filter_cutoff_hz);
    lr->input_remainder_dim = 0;
    lr->input_remainder = wtk_calloc(sizeof(float), input_remainder_dim);
    lr->old_remainder = wtk_calloc(sizeof(float), input_remainder_dim);

    _lr_set_index_and_weights(lr);
    qtk_linear_resample_reset(lr);

    return 0;
}

void qtk_linear_resample_clean(qtk_linear_resample_t *lr) {
    int idx;

    for (idx = 0; idx < lr->output_samples_in_unit; idx++) {
        wtk_free(lr->weights[idx]);
    }
    wtk_free(lr->weights);
    wtk_free(lr->first_index);
    wtk_free(lr->input_remainder);
    wtk_free(lr->old_remainder);
    wtk_free(lr->weights_dim);
}

int qtk_linear_resample_reset(qtk_linear_resample_t *lr) {
    lr->intput_sample_offset = 0;
    lr->output_sample_offset = 0;
    lr->input_remainder_dim = 0;
    return 0;
}

int qtk_linear_resample_get_output_dim(qtk_linear_resample_t *lr, int input_dim,
                                       int is_end) {
    int tot_input_samp = lr->intput_sample_offset + input_dim;
    int tot_output_samp = _lr_get_num_outputsamples(lr, tot_input_samp, is_end);
    return tot_output_samp - lr->output_sample_offset;
}

float *qtk_linear_resample_process(qtk_linear_resample_t *lr, float *output,
                                   float *input, int input_dim, int is_end) {
    int tot_input_samp, tot_output_samp;
    int64_t samp_out;

    tot_input_samp = lr->intput_sample_offset + input_dim;
    tot_output_samp = _lr_get_num_outputsamples(lr, tot_input_samp, is_end);

    for (samp_out = lr->output_sample_offset; samp_out < tot_output_samp;
         samp_out++) {
        int64_t first_samp_in;
        int samp_out_wrapped;
        int first_input_idx;
        float *weights, cur_output;
        int weights_dim;
        int input_idx, output_idx;

        _lr_get_index(lr, samp_out, &first_samp_in, &samp_out_wrapped);
        weights = lr->weights[samp_out_wrapped];
        weights_dim = lr->weights_dim[samp_out_wrapped];
        first_input_idx = cast(int, first_samp_in - lr->intput_sample_offset);

        if (first_input_idx >= 0 &&
            first_input_idx + weights_dim <= input_dim) {
            cur_output =
                qtk_vec_xdot(input + first_input_idx, weights, weights_dim);
        } else {
            int idx;
            float weight;
            cur_output = 0.0;
            for (idx = 0; idx < weights_dim; idx++) {
                weight = weights[idx];
                input_idx = first_input_idx + idx;
                if (input_idx < 0 && lr->input_remainder_dim + input_idx >= 0) {
                    cur_output +=
                        weight * lr->input_remainder[lr->input_remainder_dim +
                                                     input_idx];
                } else if (input_idx >= 0 && input_idx < input_dim) {
                    cur_output += weight * input[input_idx];
                } else if (input_idx >= input_dim) {
                    if (!is_end) {
                        wtk_debug("found bug\n");
                    }
                }
            }
        }
        output_idx = cast(int, samp_out - lr->output_sample_offset);
        output[output_idx] = cur_output;
    }

    if (is_end) {
        qtk_linear_resample_reset(lr);
    } else {
        _lr_set_remainder(lr, input, input_dim);
        lr->intput_sample_offset = tot_input_samp;
        lr->output_sample_offset = tot_output_samp;
    }

    return output;
}

static float _ar_filter_func(qtk_arbitrary_resample_t *lr, float t) {
    float window, filter, filter_cutoff;
    int num_zeros;

    num_zeros = lr->cfg->num_zeros;
    filter_cutoff = lr->cfg->filter_cutoff;

    window = fabs(t) < num_zeros / (2.0 * filter_cutoff)
                 ? 0.5 * (1 + cos(M_2PI * filter_cutoff / num_zeros * t))
                 : 0.0;
    filter = t != 0 ? sin(M_2PI * filter_cutoff * t) / (M_PI * t)
                    : 2 * filter_cutoff;

    return filter * window;
}

static void _ar_set_index_and_weights(qtk_arbitrary_resample_t *ar) {
    int num_samples;
    int idx, idxj;
    float filter_width, t, t_min, t_max, delta_t;
    int idx_min, idx_max, num_indices;

    num_samples = ar->cfg->sample_points_dim;
    ar->first_index = wtk_malloc(sizeof(int) * num_samples);
    ar->weights = wtk_malloc(sizeof(float *) * num_samples);
    ar->weights_dim = wtk_malloc(sizeof(int) * num_samples);

    filter_width = ar->cfg->num_zeros / (2.0 * ar->cfg->filter_cutoff);
    for (idx = 0; idx < num_samples; idx++) {
        t = ar->cfg->sample_points[idx];
        t_min = t - filter_width;
        t_max = t + filter_width;
        idx_min = ceil(ar->cfg->samp_rate_in * t_min);
        idx_max = floor(ar->cfg->samp_rate_in * t_max);
        idx_min = max(idx_min, 0);
        idx_max = min(idx_max, ar->cfg->num_samples_in - 1);
        ar->first_index[idx] = idx_min;
        num_indices = idx_max - idx_min + 1;
        ar->weights[idx] = wtk_malloc(sizeof(float) * num_indices);
        ar->weights_dim[idx] = num_indices;

        for (idxj = 0; idxj < num_indices; idxj++) {
            delta_t = ar->cfg->sample_points[idx] -
                      (idx_min + idxj) / ar->cfg->samp_rate_in;
            ar->weights[idx][idxj] =
                _ar_filter_func(ar, delta_t) / ar->cfg->samp_rate_in;
        }
    }
}

int qtk_arbitrary_resample_init(qtk_arbitrary_resample_t *ar,
                                qtk_arbitrary_resample_cfg_t *cfg) {
    ar->cfg = cfg;
    _ar_set_index_and_weights(ar);

    return 0;
}

void qtk_arbitrary_resample_clean(qtk_arbitrary_resample_t *ar) {
    int idx, num_samples;

    num_samples = ar->cfg->sample_points_dim;
    for (idx = 0; idx < num_samples; idx++) {
        wtk_free(ar->weights[idx]);
    }
    wtk_free(ar->weights);
    wtk_free(ar->weights_dim);
    wtk_free(ar->first_index);
}

int qtk_arbitrary_resample_reset(qtk_arbitrary_resample_t *ar) { return 0; }

// input (nrow, num_measured_lags) ouput (nrow, lags_dim)
int qtk_arbitrary_resample_process(qtk_arbitrary_resample_t *ar, float *input,
                                   float *output, int irow, int icol) {
    int idx, m, n, num_samples;

    num_samples = ar->cfg->sample_points_dim; // lags_dim

    for (idx = 0; idx < num_samples; idx++) {
        for (m = 0; m < irow; m++) {
            float *dest = output + m * num_samples + idx;
            float *input_p = input + m * icol + ar->first_index[idx];
            float *weights = ar->weights[idx];
            float res = 0;
            for (n = 0; n < ar->weights_dim[idx]; n++) {
                res += *input_p++ * *weights++;
            }
            *dest = res;
        }
    }
    return 0;
}
