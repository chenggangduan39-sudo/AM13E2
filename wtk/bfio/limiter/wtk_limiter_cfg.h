#ifndef WTK_LIMITER_LIMITER_CFG_H
#define WTK_LIMITER_LIMITER_CFG_H
#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_limiter_cfg wtk_limiter_cfg_t;

struct wtk_limiter_cfg{
    float max_amp; // 最大振幅
    float default_lookahead_samples; // 默认前瞻样本数
    float envelope_attack_rate; // 信道包络攻击速率
    float envelope_release_rate; // 信道包络释放速率
    float gain_smoothing_attack; // 增益平滑攻击速率
    float gain_smoothing_release; // 增益平滑释放速率
    float soft_clip_threshold; // 软阈值
    float min_gain; // 最小增益

    unsigned int use_lookahead : 1; // 是否使用前瞻
};

int wtk_limiter_cfg_init(wtk_limiter_cfg_t *cfg);
int wtk_limiter_cfg_clean(wtk_limiter_cfg_t *cfg);
int wtk_limiter_cfg_update_local(wtk_limiter_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_limiter_cfg_update(wtk_limiter_cfg_t *cfg);

#ifdef __cplusplus
};
#endif

#endif