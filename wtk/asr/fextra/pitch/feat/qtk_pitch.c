#include "wtk/asr/fextra/pitch/feat/qtk_pitch.h"
#include "wtk/asr/fextra/pitch/audio/qtk_dsp.h"
#include "wtk/asr/fextra/pitch/core/qtk_base.h"
#include "wtk/asr/fextra/pitch/math/qtk_math.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_strbuf.h"

typedef struct _frame_info _frame_info_t;
typedef struct _state_info _state_info_t;
typedef struct _nccf_info _nccf_info_t;
typedef struct _pair_if _pair_if_t;

struct _pair_if {
    int first;
    float second;
};

struct _state_info {
    int backpointer;
    float pov_nccf;
};

struct _frame_info {
    _frame_info_t *prev_info;
    int state_offset;
    int cur_best_state;
    _state_info_t *state_info;
    int num_state;
};

struct _nccf_info {
    // Vector<BaseFloat> nccf_pitch_resampled;  // resampled nccf_pitch
    float *nccf_pitch_resampled;
    float avg_norm_prod; // average value of e1 * e2.
    float mean_square_energy;
};

static _frame_info_t *_frame_info_new(int num_state, int state_ffset,
                                      int cur_best_state,
                                      _frame_info_t *prev_info) {
    _frame_info_t *fi =
        wtk_malloc(sizeof(_frame_info_t) + sizeof(_state_info_t) * num_state);
    fi->state_offset = state_ffset;
    fi->cur_best_state = cur_best_state;
    fi->prev_info = prev_info;
    fi->num_state = num_state;
    fi->state_info = cast(_state_info_t *, fi + 1);

    return fi;
}

// TODO Assert nccf_pov dim == num_state
static void _frame_info_set_nccf_pov(_frame_info_t *fi, float *nccf_pov) {
    int idx;

    for (idx = 0; idx < fi->num_state; idx++) {
        fi->state_info[idx].pov_nccf = nccf_pov[idx];
    }
}

static void _frame_info_set_best_state(_frame_info_t *fi, qtk_array_t *lag_nccf,
                                       int best_state) {
    _pair_if_t *pif;
    int lag_nccf_idx = lag_nccf->dim - 1;
    _frame_info_t *cur_fi = fi;
    _frame_info_t *prev_fi;

    while (cur_fi) {
        pif = qtk_array_get(lag_nccf, lag_nccf_idx);
        prev_fi = cur_fi->prev_info;
        if (best_state == cur_fi->cur_best_state) {
            return;
        }
        if (prev_fi != NULL) {
            pif->first = best_state;
        }
        size_t state_info_index = best_state - cur_fi->state_offset;
        QTK_ASSERT(state_info_index < cur_fi->num_state, "found bug\n");
        cur_fi->cur_best_state = best_state;
        best_state = cur_fi->state_info[state_info_index].backpointer;
        if (prev_fi != NULL) {
            pif->second = cur_fi->state_info[state_info_index].pov_nccf;
        }
        cur_fi = prev_fi;
        if (cur_fi) {
            --lag_nccf_idx;
        }
    }
}

static int _frame_info_compute_latency(_frame_info_t *fi, int max_latency) {
    if (max_latency <= 0) {
        return 0;
    }
    int latency = 0;
    int num_states = fi->num_state;
    int min_living_state = 0, max_living_state = num_states - 1;
    _frame_info_t *cur_fi = fi;

    for (; cur_fi != NULL && latency < max_latency;) {
        int offset = cur_fi->state_offset;
        QTK_ASSERT(min_living_state >= offset &&
                       max_living_state - offset < cur_fi->num_state,
                   "found bug\n");
        min_living_state =
            cur_fi->state_info[min_living_state - offset].backpointer;
        max_living_state =
            cur_fi->state_info[max_living_state - offset].backpointer;
        if (min_living_state == max_living_state) {
            return latency;
        }
        cur_fi = cur_fi->prev_info;
        if (cur_fi) {
            latency++;
        }
    }

    return latency;
}

static void _compute_local_cost(qtk_pitch_t *p, float *nccf_pitch) {
    qtk_vec_set(p->local_cost, p->lags_dim, 1.0);
    qtk_vec_add_vec(p->local_cost, p->lags_dim, nccf_pitch, -1.0);
    qtk_vec_add_vecvec(p->local_cost, p->lags_dim, p->lags, nccf_pitch,
                       p->cfg->soft_min_f0, 1.0);
}

static void _frame_info_compute_backtraces(qtk_pitch_t *p, _frame_info_t *fi,
                                           float *nccf_pitch,
                                           float *prev_forward_cost,
                                           float *cur_forward_cost) {
    float delta_pitch_sq, inter_frame_factor;
    int num_states, idx, j, last_backpointer = 0;
    int iter;
    int best_j, initial_best_j;
    float best_cost;

    _compute_local_cost(p, nccf_pitch);
    delta_pitch_sq = pow(log(1 + p->cfg->delta_pitch), 2.0);
    inter_frame_factor = delta_pitch_sq * p->cfg->penalty_factor;

    num_states = p->lags_dim;

    for (idx = 0; idx < num_states; idx++) {
        int start_j = last_backpointer;
        best_cost = (start_j - idx) * (start_j - idx) * inter_frame_factor +
                    prev_forward_cost[start_j];
        best_j = start_j;
        for (j = start_j + 1; j < num_states; j++) {
            float this_cost = (j - idx) * (j - idx) * inter_frame_factor +
                              prev_forward_cost[j];
            if (this_cost < best_cost) {
                best_cost = this_cost;
                best_j = j;
            } else {
                break;
            }
        }
        fi->state_info[idx].backpointer = best_j;
        cur_forward_cost[idx] = best_cost;
        p->index_info[idx] = ((int64_t)best_j << 32) | (num_states - 1);
        last_backpointer = best_j;
    }

    for (iter = 0; iter < num_states; iter++) {
        int changed = 0;
        int64_t index_pair;
        int32_t lower_bound, upper_bound;
        if (iter % 2 == 0) {
            last_backpointer = num_states - 1;
            for (idx = num_states - 1; idx >= 0; idx--) {
                index_pair = p->index_info[idx];
                lower_bound = index_pair >> 32;
                upper_bound =
                    min(0x00000000FFFFFFFF & index_pair, last_backpointer);
                if (lower_bound == upper_bound) {
                    last_backpointer = lower_bound;
                    continue;
                }

                best_cost = cur_forward_cost[idx];
                best_j = fi->state_info[idx].backpointer;
                initial_best_j = best_j;

                if (best_j == upper_bound) {
                    last_backpointer = best_j;
                    continue;
                }

                for (j = upper_bound; j > lower_bound + 1; j--) {
                    float this_cost =
                        (j - idx) * (j - idx) * inter_frame_factor +
                        prev_forward_cost[j];
                    if (this_cost < best_cost) {
                        best_cost = this_cost;
                        best_j = j;
                    } else {
                        if (best_j > j) {
                            break;
                        }
                    }
                }
                p->index_info[idx] = (int64_t)lower_bound << 32 | best_j;

                if (best_j != initial_best_j) {
                    cur_forward_cost[idx] = best_cost;
                    fi->state_info[idx].backpointer = best_j;
                    changed = 1;
                }
                last_backpointer = best_j;
            }
        } else {
            last_backpointer = 0;
            for (idx = 0; idx < num_states; idx++) {
                index_pair = p->index_info[idx];
                lower_bound = max(last_backpointer, index_pair >> 32);
                upper_bound = 0x00000000FFFFFFFF & index_pair;
                if (lower_bound == upper_bound) {
                    last_backpointer = lower_bound;
                    continue;
                }
                best_cost = cur_forward_cost[idx];
                best_j = fi->state_info[idx].backpointer;
                initial_best_j = best_j;

                if (best_j == lower_bound) {
                    last_backpointer = best_j;
                    continue;
                }

                for (j = lower_bound; j < upper_bound - 1; j++) {
                    float this_cost =
                        (j - idx) * (j - idx) * inter_frame_factor +
                        prev_forward_cost[j];
                    if (this_cost < best_cost) {
                        best_cost = this_cost;
                        best_j = j;
                    } else {
                        if (best_j < j) {
                            break;
                        }
                    }
                    p->index_info[idx] = (int64_t)best_j << 32 | upper_bound;
                    if (best_j != initial_best_j) {
                        cur_forward_cost[idx] = best_cost;
                        fi->state_info[idx].backpointer = best_j;
                        changed = 1;
                    }
                    last_backpointer = best_j;
                }
            }
            if (!changed) {
                break;
            }
        }
    }
    fi->cur_best_state = -1;
    qtk_vec_add_vec(cur_forward_cost, p->lags_dim, p->local_cost, 1.0);
}

static void _frame_info_delete(_frame_info_t *fi) { wtk_free(fi); }

static _nccf_info_t *_nccf_info_new(float avg_norm_prod,
                                    float mean_square_energy) {
    _nccf_info_t *ncfi = wtk_malloc(sizeof(_nccf_info_t));
    ncfi->avg_norm_prod = avg_norm_prod;
    ncfi->mean_square_energy = mean_square_energy;

    return ncfi;
}

static void _nccf_info_delete(_nccf_info_t *ncfi) { wtk_free(ncfi); }

static void qtk_pitch_frame_info_delete(qtk_pitch_t *p) {
    int idx;
    _frame_info_t **elem;

    for (idx = 0; idx < p->frame_info->dim; idx++) {
        elem = qtk_array_get(p->frame_info, idx);
        _frame_info_delete(*elem);
    }
}

static void qtk_picth_nccf_info_delete(qtk_pitch_t *p) {
    int idx;
    _nccf_info_t **elem;

    for (idx = 0; idx < p->nccf_info->dim; idx++) {
        elem = qtk_array_get(p->nccf_info, idx);
        _nccf_info_delete(*elem);
    }
}

static int _pitch_num_frames_ready(qtk_pitch_t *p) {
    int num_frames = p->lag_nccf->dim, latency = p->frame_latency;
    QTK_ASSERT(latency <= num_frames, "found bug\n");
    return num_frames - latency;
}

static int _pitch_is_last_frame(qtk_pitch_t *p, int frame) {
    int num_ready = _pitch_num_frames_ready(p);

    return p->input_finished && frame + 1 == num_ready;
}

static void _pitch_get_frame(qtk_pitch_t *p, int frame, float* feat, int dim) {
    _pair_if_t *pif;

    pif = qtk_array_get(p->lag_nccf, frame);
    feat[0] = pif->second;
    feat[1] = 1.0 / p->lags[pif->first];
}

static int _num_frames_available(qtk_pitch_t *p, int64_t downsamp_processed) {
    int frame_shift, frame_length;

    frame_shift = p->cfg->nccf_win_shift;
    frame_length = p->cfg->nccf_win_sz;

    if (!p->input_finished) {
        frame_length += p->nccf_last_lag;
    }
    if (downsamp_processed < frame_length) {
        return 0;
    }
    if (!p->cfg->snip_edges) {
        return p->input_finished
                   ? cast(int, downsamp_processed * 1.0f / frame_shift + 0.5f)
                   : cast(int, (downsamp_processed - frame_length / 2) * 1.0f /
                                       frame_shift +
                                   0.5f);
    }

    return cast(int, (downsamp_processed - frame_length) / frame_shift + 1);
}

static float *_select_lags(qtk_pitch_t *p, int *lags_dim) {
    float min_lag, max_lag, lag, *res;
    wtk_strbuf_t *tmp_lags = wtk_strbuf_new(1024, 1);

    min_lag = 1.0 / p->cfg->max_f0;
    max_lag = 1.0 / p->cfg->min_f0;
    *lags_dim = 0;

    for (lag = min_lag; lag < max_lag; lag *= 1.0 + p->cfg->delta_pitch) {
        wtk_strbuf_push(tmp_lags, cast(char *, &lag), sizeof(lag));
        *lags_dim += 1;
    }

    res = wtk_malloc(tmp_lags->pos);
    memcpy(res, tmp_lags->data, tmp_lags->pos);

    wtk_strbuf_delete(tmp_lags);

    return res;
}

static void _set_nccf_resamp(qtk_pitch_t *p) {
    int num_measured_lags;

    num_measured_lags = p->nccf_last_lag - p->nccf_first_lag + 1;
    p->cfg->nccf_resamp.num_samples_in = num_measured_lags;
    p->cfg->nccf_resamp.samp_rate_in = p->cfg->resamp.samp_rate_out_hz;
    p->cfg->nccf_resamp.filter_cutoff = p->cfg->upsamp_cutoff;
    p->cfg->nccf_resamp.sample_points = p->lags_offset;
    p->cfg->nccf_resamp.sample_points_dim = p->lags_dim;
    p->cfg->nccf_resamp.num_zeros = p->cfg->upsamp_filter_width;
    qtk_arbitrary_resample_init(&p->nccf_resamp, &p->cfg->nccf_resamp);
}

static void _update_remainder(qtk_pitch_t *p, float *downsamp_wav,
                              int downsamp_len) {
    int64_t num_frames, next_frame, frame_shift, next_frame_sample;
    int64_t next_downsamped_processed;
    int64_t idx;
    float *cur, *old;

    num_frames = p->frame_info->dim - 1;
    next_frame = num_frames;
    frame_shift = p->cfg->nccf_win_shift;
    next_frame_sample = frame_shift * next_frame;

    p->signal_sumsq += qtk_vec_xdot(downsamp_wav, downsamp_wav, downsamp_len);
    p->signal_sum += qtk_vec_sum(downsamp_wav, downsamp_len);
    next_downsamped_processed = p->downsamp_processed + downsamp_len;

    if (next_frame_sample > next_downsamped_processed) {
        if (p->cfg->nccf_win_sz + p->nccf_last_lag >= frame_shift) {
            wtk_debug("found bug\n");
        }
        qtk_array_resize(p->downsamp_signal_remainder, 0);
    } else {
        qtk_array_resize(p->tmp_downsamp_signal_remainder,
                         next_downsamped_processed - next_frame_sample);
        for (idx = next_frame_sample; idx < next_downsamped_processed; idx++) {
            cur = qtk_array_get(p->tmp_downsamp_signal_remainder,
                                idx - next_frame_sample);
            if (idx >= p->downsamp_processed) { /* current signal */
                *cur = downsamp_wav[idx - p->downsamp_processed];
            } else { /* in old remainder */
                old = qtk_array_get(p->downsamp_signal_remainder,
                                    idx - p->downsamp_processed +
                                        p->downsamp_signal_remainder->dim);
                *cur = *old;
            }
        }
        qtk_array_swap(p->tmp_downsamp_signal_remainder,
                       p->downsamp_signal_remainder);
    }
    p->downsamp_processed = next_downsamped_processed;
}

int qtk_pitch_init(qtk_pitch_t *p, qtk_pitch_cfg_t *cfg) {
    double outer_min_lag, outer_max_lag;
    _frame_info_t *fi;

    p->heap = wtk_heap_new(1024);
    p->step_heap = wtk_heap_new(1024);

    p->cfg = cfg;
    p->signal_sumsq = 0.0;
    p->signal_sum = 0.0;
    p->downsamp_processed = 0;
    p->input_finished = 0;

    outer_min_lag = 1.0 / cfg->max_f0 - (cfg->upsamp_filter_width /
                                         (2.0 * cfg->resamp.samp_rate_out_hz));
    outer_max_lag = 1.0 / cfg->min_f0 + (cfg->upsamp_filter_width /
                                         (2.0 * cfg->resamp.samp_rate_out_hz));

    p->nccf_first_lag = ceil(cfg->resamp.samp_rate_out_hz * outer_min_lag);
    p->nccf_last_lag = floor(cfg->resamp.samp_rate_out_hz * outer_max_lag);

    p->num_measured_lags = p->nccf_last_lag - p->nccf_first_lag + 1;
    p->full_frame_length = p->cfg->nccf_win_sz + p->nccf_last_lag;
    p->window = wtk_calloc(sizeof(float), p->full_frame_length);
    p->inner_prod = wtk_calloc(sizeof(float), p->num_measured_lags);
    p->norm_prod = wtk_calloc(sizeof(float), p->num_measured_lags);

    p->frame_latency = 0;
    p->lags = _select_lags(p, &p->lags_dim);
    p->lags_offset = wtk_calloc(sizeof(float), p->lags_dim);
    p->forward_cost = wtk_calloc(sizeof(float), p->lags_dim);
    p->cur_forward_cost = wtk_calloc(sizeof(float), p->lags_dim);
    p->local_cost = wtk_calloc(sizeof(float), p->lags_dim);
    p->index_info = wtk_calloc(sizeof(int64_t), p->lags_dim);

    memcpy(p->lags_offset, p->lags, sizeof(float) * p->lags_dim);
    qtk_vec_add_inplace(p->lags_offset, p->lags_dim,
                        -cast(float, p->nccf_first_lag) /
                            cast(float, p->cfg->resamp.samp_rate_out_hz));

    qtk_linear_resample_init(&p->rs, &p->cfg->resamp);
    _set_nccf_resamp(p);
    p->frame_info = qtk_array_new(2, sizeof(_frame_info_t *));
    p->nccf_info = qtk_array_new(2, sizeof(_nccf_info_t *));
    fi = _frame_info_new(p->lags_dim, 0, -1, NULL);
    qtk_array_push(p->frame_info, &fi);
    p->downsamp_signal_remainder = qtk_array_new(2, sizeof(float));
    p->tmp_downsamp_signal_remainder = qtk_array_new(2, sizeof(float));
    p->forward_cost_remainder = 0.0;
    p->lag_nccf = qtk_array_new(2, sizeof(_pair_if_t));
    p->ud = NULL;
    p->notify = NULL;
    p->cur_frame = 0;
    p->samp_per_chunk = p->cfg->frames_per_chunk * p->cfg->frame_shift_ms *
                        p->cfg->resamp.samp_rate_in_hz / 1000.0f;
    if (p->samp_per_chunk > 0) {
        p->chunk = wtk_malloc(sizeof(float) * p->samp_per_chunk);
    } else {
        p->chunk = NULL;
    }
    p->chunk_pos = 0;
    qtk_pitch_post_init(&p->post, &cfg->post);
    qtk_pitch_post_set_sourcer(
        &p->post, cast(int (*)(void *), _pitch_num_frames_ready),
        cast(void (*)(void *, int, float *,int), _pitch_get_frame),
        cast(int (*)(void *, int), _pitch_is_last_frame), p);

    return 0;
}

qtk_pitch_t *qtk_pitch_new(qtk_pitch_cfg_t *cfg) {
    qtk_pitch_t *p;

    p = wtk_malloc(sizeof(qtk_pitch_t));
    qtk_pitch_init(p, cfg);

    return p;
}

void qtk_pitch_clean(qtk_pitch_t *p) {
    qtk_linear_resample_clean(&p->rs);
    qtk_arbitrary_resample_clean(&p->nccf_resamp);
    qtk_pitch_post_clean(&p->post);
    if (p->lags) {
        wtk_free(p->lags);
    }
    if (p->lags_offset) {
        wtk_free(p->lags_offset);
    }
    if (p->forward_cost) {
        wtk_free(p->forward_cost);
    }
    if (p->frame_info) {
        qtk_pitch_frame_info_delete(p);
        qtk_array_delete(p->frame_info);
    }
    qtk_picth_nccf_info_delete(p);
    qtk_array_delete(p->nccf_info);
    if (p->downsamp_signal_remainder) {
        qtk_array_delete(p->downsamp_signal_remainder);
    }
    if (p->tmp_downsamp_signal_remainder) {
        qtk_array_delete(p->tmp_downsamp_signal_remainder);
    }
    qtk_array_delete(p->lag_nccf);
    wtk_free(p->window);
    wtk_free(p->inner_prod);
    wtk_free(p->norm_prod);
    wtk_free(p->cur_forward_cost);
    wtk_free(p->local_cost);
    wtk_free(p->index_info);
    if (p->chunk) {
        wtk_free(p->chunk);
    }

    wtk_heap_delete(p->heap);
    wtk_heap_delete(p->step_heap);
}

void qtk_pitch_delete(qtk_pitch_t *p) {
    qtk_pitch_clean(p);
    wtk_free(p);
}

static void _pitch_extract_frame(qtk_pitch_t *p, float *downsamp_wav,
                                 int downsamp_len, int64_t sample_idx,
                                 float *window, int window_dim) {
    int full_frame_length;
    int sub_frame_length, sub_frame_idx;
    int offset, remainder_offset;

    full_frame_length = window_dim;
    offset = cast(int, sample_idx - p->downsamp_processed);

    if (sample_idx < 0) {
        QTK_ASSERT(!p->cfg->snip_edges, "found bug\n");
        sub_frame_length = sample_idx + full_frame_length;
        sub_frame_idx = full_frame_length - sub_frame_length;
        QTK_ASSERT(sub_frame_idx <= 0 || sub_frame_length <= 0, "found bug\n");
        memset(window, 0, sizeof(float) * window_dim);
        return _pitch_extract_frame(p, downsamp_wav, downsamp_len, 0,
                                    window + sub_frame_idx, sub_frame_length);
    }

    if (offset + full_frame_length > downsamp_len) {
        QTK_ASSERT(p->input_finished, "found bug\n");
        sub_frame_length = downsamp_len - offset;
        QTK_ASSERT(sub_frame_length > 0, "found bug\n");
        memset(window, 0, sizeof(float) * window_dim);
        return _pitch_extract_frame(p, downsamp_wav, downsamp_len, sample_idx,
                                    window, sub_frame_length);
    }

    if (offset > 0) {
        memcpy(window, downsamp_wav + offset,
               full_frame_length * sizeof(float));
    } else {
        remainder_offset = offset + p->downsamp_signal_remainder->dim;
        QTK_ASSERT(remainder_offset >= 0, "found bug\n");
        QTK_ASSERT(offset + full_frame_length > 0, "found bug\n");
        memcpy(window,
               qtk_array_get(p->downsamp_signal_remainder, remainder_offset),
               -offset * sizeof(float));
        memcpy(window - offset, downsamp_wav,
               (full_frame_length + offset) * sizeof(float));
    }

    if (p->cfg->preemph_coeff != 0.0) {
        qtk_dsp_preemph(window, window_dim, p->cfg->preemph_coeff);
    }
}

static void _compute_correlation(qtk_pitch_t *p) {
    float mean, e1, e2, sum;
    int lag;

    mean = qtk_vec_sum(p->window, p->cfg->nccf_win_sz) / p->cfg->nccf_win_sz;
    qtk_vec_add_inplace(p->window, p->full_frame_length, -mean);

    e1 = qtk_vec_xdot(p->window, p->window, p->cfg->nccf_win_sz);

    for (lag = p->nccf_first_lag; lag <= p->nccf_last_lag; lag++) {
        e2 =
            qtk_vec_xdot(p->window + lag, p->window + lag, p->cfg->nccf_win_sz);
        sum = qtk_vec_xdot(p->window, p->window + lag, p->cfg->nccf_win_sz);
        p->inner_prod[lag - p->nccf_first_lag] = sum;
        p->norm_prod[lag - p->nccf_first_lag] = e1 * e2;
    }

    qtk_vec_add_inplace(p->window, p->full_frame_length, mean);
}

static void _compute_nccf(qtk_pitch_t *p, float nccf_ballast, float *nccf_vec) {
    int lag;
    float denominator, numerator, nccf;

    for (lag = 0; lag < p->num_measured_lags; lag++) {
        numerator = p->inner_prod[lag];
        denominator = pow(p->norm_prod[lag] + nccf_ballast, 0.5);
        nccf = denominator != 0.0 ? numerator / denominator : 0.0;
        QTK_ASSERT(nccf < 1.01 && nccf > -1.01, "found bug\n");
        nccf_vec[lag] = nccf;
    }
}

static void _recompute_backtraces(qtk_pitch_t *p) {
    int frame, idx;
    int num_frames = p->frame_info->dim - 1;
    _frame_info_t **fi;
    QTK_ASSERT(num_frames <= p->cfg->recompute_frame, "found bug\n");
    QTK_ASSERT(p->nccf_info->dim == num_frames, "found bug\n");

    if (num_frames == 0) {
        return;
    }
    double num_samp = p->downsamp_processed, sum = p->signal_sum,
           sumsq = p->signal_sumsq, mean = sum / num_samp;
    float mean_square = sumsq / num_samp - mean * mean;

    int must_recompute = 0;
    float threshold = 0.01;

    for (frame = 0; frame < num_frames; frame++) {
        _nccf_info_t **nip = qtk_array_get(p->nccf_info, frame);
        if (!qtk_approx_eqf((*nip)->mean_square_energy, mean_square,
                            threshold)) {
            must_recompute = 1;
            break;
        }
    }

    if (!must_recompute) {
        goto end;
    }

    int num_states = p->lags_dim, basic_frame_length = p->cfg->nccf_win_sz;

    float new_nccf_ballast =
        pow(mean_square * basic_frame_length, 2) * p->cfg->nccf_ballast;
    double forward_cost_remainder = 0.0;

    float *forward_cost = wtk_heap_zalloc(p->heap, num_states * sizeof(float));
    float *next_forward_cost =
        wtk_heap_zalloc(p->heap, num_states * sizeof(float));

    for (frame = 0; frame < num_frames; frame++) {
        _nccf_info_t **nip = qtk_array_get(p->nccf_info, frame);
        float old_mean_square = (*nip)->mean_square_energy,
              avg_norm_prod = (*nip)->avg_norm_prod,
              old_nccf_ballast =
                  pow(old_mean_square * basic_frame_length, 2) *
                  p->cfg->nccf_ballast,
              nccf_scale = pow((old_nccf_ballast + avg_norm_prod) /
                                       (new_nccf_ballast + avg_norm_prod),
                                   0.5);
        qtk_vec_scale((*nip)->nccf_pitch_resampled, p->lags_dim, nccf_scale);

        fi = qtk_array_get(p->frame_info, frame + 1);
        _frame_info_compute_backtraces(p, *fi, (*nip)->nccf_pitch_resampled,
                                       forward_cost, next_forward_cost);

        float *tmp_cost = forward_cost;
        forward_cost = next_forward_cost;
        next_forward_cost = tmp_cost;

        float remainder = qtk_vec_min(forward_cost, p->lags_dim);
        forward_cost_remainder += remainder;
        qtk_vec_add_inplace(forward_cost, p->lags_dim, -remainder);
    }

    p->forward_cost_remainder = forward_cost_remainder;
    memcpy(p->forward_cost, forward_cost, sizeof(float) * p->lags_dim);

    int best_final_state;
    qtk_vec_min1(p->forward_cost, p->lags_dim, &best_final_state);

    if (p->lag_nccf->dim != num_frames) {
        qtk_array_resize(p->lag_nccf, num_frames);
    }

    fi = qtk_array_back(p->frame_info);
    _frame_info_set_best_state(*fi, p->lag_nccf, best_final_state);
    p->frame_latency =
        _frame_info_compute_latency(*fi, p->cfg->max_frames_latency);

end:
    for (idx = 0; idx < p->nccf_info->dim; idx++) {
        _nccf_info_t **nip = qtk_array_get(p->nccf_info, idx);
        _nccf_info_delete(*nip);
    }
    qtk_array_clear(p->nccf_info);
}

static int qtk_pitch_process(qtk_pitch_t *p, float *data, int len, int is_end) {
    int downsamp_len;
    int start_frame, end_frame;
    int num_new_frames;
    float *downsamp_wav = NULL;
    double cur_sumsq, cur_sum;
    int64_t cur_num_samp;

    p->input_finished = is_end;
    downsamp_len = qtk_linear_resample_get_output_dim(&p->rs, len, is_end);
    downsamp_wav = wtk_heap_malloc(p->step_heap, sizeof(float) * downsamp_len);
    qtk_linear_resample_process(&p->rs, downsamp_wav, data, len, is_end);

    cur_sumsq = p->signal_sumsq;
    cur_sum = p->signal_sum;
    cur_num_samp = p->downsamp_processed;

    if (!p->cfg->nccf_ballast_online) {
        cur_num_samp += downsamp_len;
        cur_sumsq += qtk_vec_xdot(downsamp_wav, downsamp_wav, downsamp_len);
        cur_sum += qtk_vec_sum(downsamp_wav, downsamp_len);
    }
    end_frame = _num_frames_available(p, p->downsamp_processed + downsamp_len);
    start_frame = p->frame_info->dim - 1;
    num_new_frames = end_frame - start_frame;
    if (num_new_frames == 0) {
        _update_remainder(p, downsamp_wav, downsamp_len);
        goto end;
    }

    int num_measured_lags, num_resamp_lags, frame_shift, basic_frame_length,
        full_frame_length;
    int frame;
    int64_t start_sample, end_sample, prev_frame_end_sample = 0;
    float *nccf_pitch, *nccf_pov;

    num_measured_lags = p->num_measured_lags;
    num_resamp_lags = p->lags_dim;
    frame_shift = p->cfg->nccf_win_shift;
    basic_frame_length = p->cfg->nccf_win_sz;
    full_frame_length = p->full_frame_length;

    nccf_pitch = wtk_heap_malloc(p->step_heap, sizeof(float) * num_new_frames *
                                                   num_measured_lags);
    nccf_pov = wtk_heap_malloc(p->step_heap, sizeof(float) * num_new_frames *
                                                 num_measured_lags);

    for (frame = start_frame; frame < end_frame; frame++) {
        start_sample = p->cfg->snip_edges
                           ? cast(int64_t, frame * frame_shift)
                           : cast(int64_t, ((frame + 0.5) * frame_shift) -
                                               full_frame_length / 2);
        _pitch_extract_frame(p, downsamp_wav, downsamp_len, start_sample,
                             p->window, p->full_frame_length);

        if (p->cfg->nccf_ballast_online) {
            int64_t new_part_dim;
            end_sample =
                start_sample + full_frame_length - p->downsamp_processed;
            QTK_ASSERT(end_sample > 0, "found bug\n");
            if (end_sample > downsamp_len) {
                QTK_ASSERT(p->input_finished, "found bug\n");
                end_sample = downsamp_len;
            }
            new_part_dim = end_sample - prev_frame_end_sample;
            cur_num_samp += new_part_dim;
            cur_sumsq += qtk_vec_xdot(downsamp_wav + prev_frame_end_sample,
                                      downsamp_wav + prev_frame_end_sample,
                                      new_part_dim);
            cur_sum +=
                qtk_vec_sum(downsamp_wav + prev_frame_end_sample, new_part_dim);
            prev_frame_end_sample = end_sample;
        }

        _compute_correlation(p);

        double mean_square =
            cur_sumsq / cur_num_samp - pow(cur_sum / cur_num_samp, 2.0);
        double nccf_ballast_pov = 0.0;
        double nccf_ballast_pitch =
            pow(mean_square * basic_frame_length, 2) * p->cfg->nccf_ballast;

        _compute_nccf(
            p, nccf_ballast_pitch,
            MAT_GET_ROW(nccf_pitch, num_measured_lags, frame - start_frame));
        _compute_nccf(
            p, nccf_ballast_pov,
            MAT_GET_ROW(nccf_pov, num_measured_lags, frame - start_frame));

        if (frame < p->cfg->recompute_frame) {
            double avg_norm_prod =
                qtk_vec_sum(p->norm_prod, p->num_measured_lags) /
                p->num_measured_lags;
            _nccf_info_t *ni = _nccf_info_new(avg_norm_prod, mean_square);
            qtk_array_push(p->nccf_info, &ni);
        }
    }

    float *nccf_pitch_resampled = wtk_heap_zalloc(
        p->heap, sizeof(float) * num_new_frames * num_resamp_lags);
    float *nccf_pov_resampled = wtk_heap_zalloc(
        p->heap, sizeof(float) * num_new_frames * num_resamp_lags);

    qtk_arbitrary_resample_process(&p->nccf_resamp, nccf_pitch,
                                   nccf_pitch_resampled, num_new_frames,
                                   num_measured_lags);
    qtk_arbitrary_resample_process(&p->nccf_resamp, nccf_pov,
                                   nccf_pov_resampled, num_new_frames,
                                   num_measured_lags);

    _update_remainder(p, downsamp_wav, downsamp_len);

    for (frame = start_frame; frame < end_frame; frame++) {
        _frame_info_t **prev_info, *cur_info;
        prev_info = qtk_array_back(p->frame_info);
        cur_info = _frame_info_new((*prev_info)->num_state, 0, -1, *prev_info);
        _frame_info_set_nccf_pov(cur_info, MAT_GET_ROW(nccf_pov_resampled,
                                                       num_resamp_lags,
                                                       frame - start_frame));
        _frame_info_compute_backtraces(p, cur_info,
                                       MAT_GET_ROW(nccf_pitch_resampled,
                                                   num_resamp_lags,
                                                   frame - start_frame),
                                       p->forward_cost, p->cur_forward_cost);
        float *tmp_cost = p->cur_forward_cost;
        p->cur_forward_cost = p->forward_cost;
        p->forward_cost = tmp_cost;

        float remainder = qtk_vec_min(p->forward_cost, p->lags_dim);
        p->forward_cost_remainder += remainder;
        qtk_vec_add_inplace(p->forward_cost, p->lags_dim, -remainder);
        qtk_array_push(p->frame_info, &cur_info);

        if (frame < p->cfg->recompute_frame) {
            _nccf_info_t **nip = qtk_array_get(p->nccf_info, frame);
            (*nip)->nccf_pitch_resampled = MAT_GET_ROW(
                nccf_pitch_resampled, num_resamp_lags, frame - start_frame);
        }
        if (frame == p->cfg->recompute_frame - 1 &&
            !p->cfg->nccf_ballast_online) {
            _recompute_backtraces(p);
        }
    }

    int best_final_state;
    qtk_vec_min1(p->forward_cost, p->lags_dim, &best_final_state);
    qtk_array_resize(p->lag_nccf, p->frame_info->dim - 1);

    _frame_info_t **fi;
    fi = qtk_array_back(p->frame_info);
    _frame_info_set_best_state(*fi, p->lag_nccf, best_final_state);
    p->frame_latency =
        _frame_info_compute_latency(*fi, p->cfg->max_frames_latency);

    if (is_end) {
        int num_frames = p->frame_info->dim - 1;
        if (num_frames < p->cfg->recompute_frame &&
            !p->cfg->nccf_ballast_online) {
            _recompute_backtraces(p);
        }
        p->frame_latency = 0;
    }

end:
    wtk_heap_reset(p->step_heap);
    return 0;
}

static void _pitch_raise_feat(qtk_pitch_t *p) {
    float feat[4];
    if (!p->cfg->use_post) {
        for (; p->cur_frame < _pitch_num_frames_ready(p); p->cur_frame++) {
            _pitch_get_frame(p, p->cur_frame, feat,4);
            p->notify(p->cur_frame, feat, p->ud);
        }
    } else {
        for (; p->cur_frame < qtk_pitch_post_nums_ready(&p->post);
             p->cur_frame++) {
            qtk_pitch_post_get_frame(&p->post, p->cur_frame, feat,4);
            p->notify(p->cur_frame, feat, p->ud);
        }
    }
}

int qtk_pitch_feed(qtk_pitch_t *p, float *data, int len) {
    int cur_offset = 0, samp_per_chunk = p->samp_per_chunk;

    if (p->notify == NULL) {
        return 0;
    }

    if (samp_per_chunk <= 0) {
        qtk_pitch_process(p, data, len, 0);
    } else {
        while (cur_offset < len) {
            int need_cpy = min(samp_per_chunk - p->chunk_pos, len - cur_offset);
            memcpy(p->chunk + p->chunk_pos, data + cur_offset,
                   need_cpy * sizeof(float));
            if (p->chunk_pos == samp_per_chunk) {
                qtk_pitch_process(p, p->chunk, samp_per_chunk, 0);
                p->chunk_pos = 0;
                if (p->cfg->simulate_first_pass_online) {
                    _pitch_raise_feat(p);
                }
            } else {
                p->chunk_pos += need_cpy;
            }
            cur_offset += need_cpy;
        }
    }

    return 0;
}

int qtk_pitch_finish(qtk_pitch_t *p) {
	if (p->samp_per_chunk > 0 && p->chunk_pos > 0) {
		qtk_pitch_process(p, p->chunk, p->chunk_pos, 0);
	}
    qtk_pitch_process(p, NULL, 0, 1);
    _pitch_raise_feat(p);

    return 0;
}

void qtk_pitch_reset(qtk_pitch_t *p) {
    _frame_info_t *fi;

    wtk_heap_reset(p->heap);
    wtk_heap_reset(p->step_heap);
    p->signal_sumsq = 0.0;
    p->signal_sum = 0.0;
    p->downsamp_processed = 0;
    p->input_finished = 0;
    p->frame_latency = 0;
    p->cur_frame = 0;
    p->chunk_pos = 0;

    memset(p->forward_cost, 0, sizeof(float) * p->lags_dim);
    qtk_pitch_frame_info_delete(p);
    qtk_array_clear(p->frame_info);
    fi = _frame_info_new(p->lags_dim, 0, -1, NULL);
    qtk_array_push(p->frame_info, &fi);
    qtk_array_clear(p->nccf_info);

    qtk_linear_resample_reset(&p->rs);
    qtk_arbitrary_resample_reset(&p->nccf_resamp);
    qtk_pitch_post_reset(&p->post);
}

void qtk_pitch_set_notify(qtk_pitch_t *p, qtk_pitch_notify_t notify, void *ud) {
    p->ud = ud;
    p->notify = notify;
}

int qtk_pitch_get_feat_dim(qtk_pitch_t *p) {
    return p->cfg->use_post ? qtk_pitch_post_get_feat_dim(&p->post) : 2;
}
