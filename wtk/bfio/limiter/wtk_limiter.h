#ifndef WTK_LIMITER_LIMITER_H
#define WTK_LIMITER_LIMITER_H
#include "wtk_limiter_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_limiter wtk_limiter_t;

struct wtk_limiter {
    wtk_limiter_cfg_t *cfg;

    float envelope;                // 当前包络值
    float gain;                    // 当前增益值
    float *lookaheadBuffer;        // 前瞻缓冲区
    int bufferSize;                // 缓冲区大小
    int bufferIndex;               // 缓冲区索引
    float maxInFrame;              // 当前帧内最大值
    float releaseCounter;          // 释放计数器
    float softClipThreshold;       // 软限幅阈值
    float prevGain;                // 前一帧增益值
};

wtk_limiter_t *wtk_limiter_new(wtk_limiter_cfg_t *cfg);
void wtk_limiter_delete(wtk_limiter_t *limiter);
void wtk_limiter_start(wtk_limiter_t *limiter);
void wtk_limiter_reset(wtk_limiter_t *limiter);
void wtk_limiter_feed(wtk_limiter_t *limiter, float *data, int len);

#ifdef __cplusplus
};
#endif

#endif