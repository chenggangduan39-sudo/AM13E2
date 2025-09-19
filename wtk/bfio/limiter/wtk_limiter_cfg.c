#include "wtk_limiter_cfg.h"


int wtk_limiter_cfg_init(wtk_limiter_cfg_t *cfg)
{
    cfg->max_amp = 32000.0f;
    cfg->default_lookahead_samples = 32;
    cfg->envelope_attack_rate = 0.8f;
    cfg->envelope_release_rate = 0.999f;
    cfg->gain_smoothing_attack = 0.3f;
    cfg->gain_smoothing_release = 0.05f;
    cfg->soft_clip_threshold = 0.95f;
    cfg->min_gain = 0.01f;

    cfg->use_lookahead = 0;
    return 0;
}
int wtk_limiter_cfg_clean(wtk_limiter_cfg_t *cfg)
{
    return 0;
}
int wtk_limiter_cfg_update_local(wtk_limiter_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, default_lookahead_samples, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, envelope_attack_rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, envelope_release_rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gain_smoothing_attack, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gain_smoothing_release, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, soft_clip_threshold, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, min_gain, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_lookahead, v);

    return 0;
}
int wtk_limiter_cfg_update(wtk_limiter_cfg_t *cfg)
{
    return 0;
}