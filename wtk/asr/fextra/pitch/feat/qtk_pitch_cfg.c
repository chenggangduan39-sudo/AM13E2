#include "wtk/asr/fextra/pitch/feat/qtk_pitch_cfg.h"
#include "wtk/core/wtk_type.h"

int qtk_pitch_cfg_init(qtk_pitch_cfg_t *cfg) {
    qtk_linear_resample_cfg_init(&cfg->resamp);
    qtk_arbitrary_resample_cfg_init(&cfg->nccf_resamp);
    qtk_pitch_post_cfg_init(&cfg->post);
    cfg->min_f0 = 50;
    cfg->max_f0 = 400;
    cfg->soft_min_f0 = 10.0;
    cfg->nccf_ballast_online = 0;
    cfg->snip_edges = 1;
    cfg->frame_length_ms = 25;
    cfg->frame_shift_ms = 10;
    cfg->upsamp_filter_width = 5;
    cfg->delta_pitch = 0.005;
    cfg->preemph_coeff = 0.0;
    cfg->nccf_ballast = 7000;
    cfg->recompute_frame = 500;
    cfg->penalty_factor = 0.1;
    cfg->max_frames_latency = 0;
    cfg->frames_per_chunk = 0;
    cfg->simulate_first_pass_online = 0;
    cfg->use_post = 0;

    return 0;
}

int qtk_pitch_cfg_update(qtk_pitch_cfg_t *cfg) {
    qtk_linear_resample_cfg_update(&cfg->resamp);
    qtk_arbitrary_resample_cfg_update(&cfg->nccf_resamp);
    qtk_pitch_post_cfg_update(&cfg->post);
    cfg->nccf_win_sz =
        (cfg->resamp.samp_rate_out_hz * cfg->frame_length_ms) / 1000;
    cfg->nccf_win_shift =
        (cfg->resamp.samp_rate_out_hz * cfg->frame_shift_ms) / 1000;
    cfg->upsamp_cutoff = cfg->resamp.samp_rate_out_hz * 0.5;
    if (cfg->simulate_first_pass_online == 1 && cfg->frames_per_chunk <= 0) {
        wtk_debug("Confict Config [simulate_first_pass_online] = %d, "
                  "[frames_per_chunk] = %d\n",
                  cfg->simulate_first_pass_online, cfg->frames_per_chunk);
        return -1;
    }
    return 0;
}

int qtk_pitch_cfg_clean(qtk_pitch_cfg_t *cfg) {
    qtk_linear_resample_cfg_clean(&cfg->resamp);
    qtk_arbitrary_resample_cfg_clean(&cfg->nccf_resamp);
    qtk_pitch_post_cfg_clean(&cfg->post);
    return 0;
}
