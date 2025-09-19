#include "wtk/asr/fextra/pitch/feat/qtk_pitch_post.h"
#include "wtk/asr/fextra/pitch/core/qtk_base.h"
#include "wtk/asr/fextra/pitch/math/qtk_math.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_alloc.h"

typedef struct {
    int cur_num_frames;
    double sum_pov;
    double sum_log_pitch_pov;
    unsigned input_finished : 1;
} _normalization_stats_t;

static void _normalization_stats_initializer(void *p) {
    _normalization_stats_t *stat = cast(_normalization_stats_t *, p);
    stat->cur_num_frames = -1;
    stat->sum_pov = 0.0;
    stat->sum_log_pitch_pov = 0.0;
    stat->input_finished = 0;
}

static float _nccf_to_pov_feature(float nccf) {
    if (nccf > 1.0) {
        nccf = 1.0;
    } else if (nccf < -1.0) {
        nccf = -1.0;
    }
    return pow((1.0001 - nccf), 0.15) - 1.0;
}

static float _nccf_to_pov(float n) {
    float ndash = fabs(n);
    if (ndash > 1.0) {
        ndash = 1.0;
    }
    float r = -5.2 + 5.4 * exp(7.5 * (ndash - 1.0)) + 4.8 * ndash -
              2.0 * exp(-10.0 * ndash) +
              4.2 * exp(20.0 * (ndash - 1.0));
    float p = 1.0 / (1 + exp(-1.0 * r));

    return p;
}

static float _get_pov_feature(qtk_pitch_post_t *pp, int frame) {
    float feat[2];
    pp->sourcer.get_frame(pp->sourcer.ud, frame, feat,2);
    float nccf = feat[0];

    return pp->cfg->pov_scale * _nccf_to_pov_feature(nccf) +
           pp->cfg->pov_offset;
}

static void _get_normalization_window(qtk_pitch_post_t *pp, int t,
                                      int src_frame_ready, int *window_begin,
                                      int *window_end) {
    int left_context = pp->cfg->normalization_left_context;
    int right_context = pp->cfg->normalization_right_context;
    *window_begin = max(0, t - left_context);
    *window_end = min(t + right_context + 1, src_frame_ready);
}

static void _update_normalization_stats(qtk_pitch_post_t *pp, int frame) {
    float tmp[2];
    float accurate_pov, log_pitch;
    int f;
    QTK_ASSERT(frame >= 0, "found bug\n");
    if (pp->normalization_stats->dim <= frame) {
        qtk_array_resize1(pp->normalization_stats, frame + 1,
                          _normalization_stats_initializer);
    }
    int cur_num_frames = pp->sourcer.frame_ready(pp->sourcer.ud);
    int input_finished =
        pp->sourcer.is_last_frame(pp->sourcer.ud, cur_num_frames - 1);
    _normalization_stats_t *this_stats =
        qtk_array_get(pp->normalization_stats, frame);
    if (this_stats->cur_num_frames == cur_num_frames &&
        this_stats->input_finished == input_finished) {
        return;
    }
    int this_window_begin, this_window_end;
    _get_normalization_window(pp, frame, cur_num_frames, &this_window_begin,
                              &this_window_end);
    if (frame > 0) {
        _normalization_stats_t *prev_stats =
            qtk_array_get(pp->normalization_stats, frame - 1);
        if (prev_stats->cur_num_frames == cur_num_frames &&
            prev_stats->input_finished == input_finished) {
            *this_stats = *prev_stats;
            int prev_window_begin, prev_window_end;
            _get_normalization_window(pp, frame - 1, cur_num_frames,
                                      &prev_window_begin, &prev_window_end);
            if (this_window_begin != prev_window_begin) {
                QTK_ASSERT(this_window_begin == (prev_window_begin + 1),
                           "found bug");
                pp->sourcer.get_frame(pp->sourcer.ud, prev_window_begin, tmp,2);
                accurate_pov = _nccf_to_pov(tmp[0]);
                log_pitch = log(tmp[1]);
                this_stats->sum_pov -= accurate_pov;
                this_stats->sum_log_pitch_pov -= accurate_pov * log_pitch;
            }
            if (this_window_end != prev_window_end) {
                QTK_ASSERT(this_window_end == (prev_window_end + 1),
                           "found bug");
                pp->sourcer.get_frame(pp->sourcer.ud, prev_window_end, tmp,2);
                accurate_pov = _nccf_to_pov(tmp[0]);
                log_pitch = log(tmp[1]);
                this_stats->sum_pov += accurate_pov;
                this_stats->sum_log_pitch_pov += accurate_pov * log_pitch;
            }
            return;
        }
    }

    this_stats->cur_num_frames = cur_num_frames;
    this_stats->input_finished = input_finished;
    this_stats->sum_pov = 0.0;
    this_stats->sum_log_pitch_pov = 0.0;

    for (f = this_window_begin; f < this_window_end; f++) {
        pp->sourcer.get_frame(pp->sourcer.ud, f, tmp,2);
        accurate_pov = _nccf_to_pov(tmp[0]);
        log_pitch = log(tmp[1]);
        this_stats->sum_pov += accurate_pov;
        this_stats->sum_log_pitch_pov += log_pitch * accurate_pov;
    }
}

static float _get_raw_log_pitch_feature(qtk_pitch_post_t *pp, int frame) {
    float feat[2];
    pp->sourcer.get_frame(pp->sourcer.ud, frame, feat,2);

    return log(feat[1]);
}

static float _get_delta_pitch_feature(qtk_pitch_post_t *pp, int frame) {
    int f, idx;
    int context = pp->cfg->delta_window;
    int start_frame = max(0, frame - context),
        end_frame =
            min(frame + context + 1, pp->sourcer.frame_ready(pp->sourcer.ud)),
        frames_in_window = end_frame - start_frame;
    float *feats = wtk_malloc(sizeof(float) * frames_in_window * 1);
    float dfeat, noise, ans;

    for (f = start_frame; f < end_frame; f++) {
        *(MAT_GET(feats, 1, f - start_frame, 0)) =
            _get_raw_log_pitch_feature(pp, f);
    }

    int delta_feats_ncol = 1 * (pp->cfg->delta.order + 1);
    float *delta_feats =
        wtk_malloc(sizeof(float) * frames_in_window * delta_feats_ncol);

    for (idx = 0; idx < frames_in_window; idx++) {
        float *output = MAT_GET_ROW(delta_feats, delta_feats_ncol, idx);
        qtk_feature_delta_process(pp->delta, feats, output, frames_in_window, 1,
                                  idx);
    }

    while (pp->delta_feature_noise->dim <= frame) {
        noise = qtk_rand_gauss() * pp->cfg->delta_pitch_noise_stddev;
        qtk_array_push(pp->delta_feature_noise, &noise);
    }

    dfeat = *MAT_GET(delta_feats, delta_feats_ncol, frame - start_frame, 1);
    noise = *cast(float *, qtk_array_get(pp->delta_feature_noise, frame));
    ans = (dfeat + noise) * pp->cfg->delta_pitch_scale;

    wtk_free(delta_feats);
    wtk_free(feats);

    return ans;
}

static float _get_normalized_log_pitch_feature(qtk_pitch_post_t *pp,
                                               int frame) {
    _update_normalization_stats(pp, frame);
    _normalization_stats_t *cur_stat =
        qtk_array_get(pp->normalization_stats, frame);
    float log_pitch = _get_raw_log_pitch_feature(pp, frame),
          avg_log_pitch = cur_stat->sum_log_pitch_pov / cur_stat->sum_pov,
          normalized_log_pitch = log_pitch - avg_log_pitch;

    return normalized_log_pitch * pp->cfg->pitch_scale;
}

int qtk_pitch_post_init(qtk_pitch_post_t *pp, qtk_pitch_post_cfg_t *cfg) {
    pp->cfg = cfg;
    pp->feat_dim = cfg->add_pov_feature ? 1 : 0;
    if (cfg->add_normalized_log_pitch) {
        pp->feat_dim++;
    }
    if (cfg->add_delta_pitch) {
        pp->feat_dim++;
    }
    if (cfg->add_raw_log_pitch) {
        pp->feat_dim++;
    }
    pp->delta = qtk_feature_delta_new(&cfg->delta);
    pp->delta_feature_noise = qtk_array_new(2, sizeof(float));
    pp->normalization_stats = qtk_array_new(2, sizeof(_normalization_stats_t));

    return 0;
}

int qtk_pitch_post_clean(qtk_pitch_post_t *pp) {
    qtk_feature_delta_delete(pp->delta);
    qtk_array_delete(pp->delta_feature_noise);
    qtk_array_delete(pp->normalization_stats);
    return 0;
}

int qtk_pitch_post_reset(qtk_pitch_post_t *pp) {
    qtk_array_clear(pp->normalization_stats);
    return 0;
}

int qtk_pitch_post_get_frame(qtk_pitch_post_t *pp, int frame, float *res,int dim) {
    int idx = 0;
    int frame_delayed = frame < pp->cfg->delay ? 0 : frame - pp->cfg->delay;

    QTK_ASSERT(frame_delayed < pp->sourcer.frame_ready(pp->sourcer.ud),
               "found bug\n");

    if (pp->cfg->add_pov_feature) {
        res[idx++] = _get_pov_feature(pp, frame_delayed);
    }
    if (pp->cfg->add_normalized_log_pitch) {
        res[idx++] = _get_normalized_log_pitch_feature(pp, frame_delayed);
    }
    if (pp->cfg->add_delta_pitch) {
        res[idx++] = _get_delta_pitch_feature(pp, frame_delayed);
    }
    if (pp->cfg->add_raw_log_pitch) {
        res[idx++] = _get_raw_log_pitch_feature(pp, frame_delayed);
    }

    return 0;
}

int qtk_pitch_post_set_sourcer(
    qtk_pitch_post_t *pp, int (*frame_ready)(void *ud),
    void (*get_frame)(void *, int frame, float* feat,int dim),
    int (*is_last_frame)(void *ud, int frame), void *ud) {
    pp->sourcer.ud = ud;
    pp->sourcer.frame_ready = frame_ready;
    pp->sourcer.get_frame = get_frame;
    pp->sourcer.is_last_frame = is_last_frame;

    return 0;
}

int qtk_pitch_post_nums_ready(qtk_pitch_post_t *pp) {
    int src_frame_ready = pp->sourcer.frame_ready(pp->sourcer.ud);
    if (src_frame_ready == 0) {
        return 0;
    }
    if (pp->sourcer.is_last_frame(pp->sourcer.ud, src_frame_ready - 1)) {
        return src_frame_ready + pp->cfg->delay;
    }
    return max(0, src_frame_ready - pp->cfg->normalization_right_context +
                      pp->cfg->delay);
}

int qtk_pitch_post_get_feat_dim(qtk_pitch_post_t *pp) { return pp->feat_dim; }
