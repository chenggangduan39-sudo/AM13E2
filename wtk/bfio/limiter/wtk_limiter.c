#include "wtk_limiter.h"

// 更新包络检测器 - 使用更平滑的包络跟踪
static float updateEnvelope(wtk_limiter_t *limiter, float sample, float currentEnvelope) {
    float envelope_attack_rate = limiter->cfg->envelope_attack_rate;
    float envelope_release_rate = limiter->cfg->envelope_release_rate;
    float absSample = fabsf(sample);
    
    // 使用RMS包络检测，减少频谱泄露
    float rmsSample = absSample * absSample;
    float rmsEnvelope = currentEnvelope * currentEnvelope;
    
    if (rmsSample > rmsEnvelope) {
        // 快速攻击
        rmsEnvelope = rmsSample * (1.0f - envelope_attack_rate) + rmsEnvelope * envelope_attack_rate;
    } else {
        // 慢速释放
        rmsEnvelope = rmsSample * (1.0f - envelope_release_rate) + rmsEnvelope * envelope_release_rate;
    }
    
    return sqrtf(rmsEnvelope);
}

// 软限幅函数 - 提供更平滑的过渡
static float softClip(wtk_limiter_t *limiter, float sample, float threshold) {
    float max_amp = limiter->cfg->max_amp;
    float absSample = fabsf(sample);
    if (absSample <= threshold) {
        return sample;
    }
    
    float sign = sample > 0 ? 1.0f : -1.0f;
    
    // 使用三次多项式进行软限幅，减少高频失真
    float overshoot = absSample - threshold;
    float softThreshold = max_amp * 1.05f; // 软限幅开始生效的阈值
    
    if (absSample < softThreshold) {
        // 在软限幅区域：使用多项式曲线平滑过渡
        float ratio = (absSample - threshold) / (softThreshold - threshold);
        float softValue = threshold + (softThreshold - threshold) * (3.0f * ratio * ratio - 2.0f * ratio * ratio * ratio);
        return sign * softValue;
    }
    
    // 硬限幅区域
    return sign * softThreshold;
}

// 计算所需增益 - 使用更平滑的增益曲线
static float calculateTargetGain(wtk_limiter_t *limiter, float maxValue) {
    float max_amp = limiter->cfg->max_amp;
    float min_gain = limiter->cfg->min_gain;
    if (maxValue <= max_amp) {
        return 1.0f;
    }
    
    // 使用对数函数使增益变化更平滑
    float overshootRatio = maxValue / max_amp;
    float targetGain = 1.0f / log10f(overshootRatio + 1.0f);
    
    return fmaxf(targetGain, min_gain);
}

// 应用增益平滑 - 使用余弦过渡减少频谱泄露
static float applyGainSmoothing(wtk_limiter_t *limiter, float targetGain) {
    float gain_smoothing_attack = limiter->cfg->gain_smoothing_attack;
    float gain_smoothing_release = limiter->cfg->gain_smoothing_release;
    float min_gain = limiter->cfg->min_gain;
    float smoothingFactor;
    float newGain;
    
    if (targetGain < limiter->gain) {
        // 攻击阶段：需要快速降低增益
        smoothingFactor = gain_smoothing_attack;
        limiter->releaseCounter = 0.0f; // 重置释放计数器
    } else {
        // 释放阶段：缓慢恢复增益
        smoothingFactor = gain_smoothing_release;
        
        // 增加释放延迟，避免快速恢复导致爆破音
        limiter->releaseCounter += 1.0f;
        if (limiter->releaseCounter < 10.0f) {
            smoothingFactor *= 0.3f; // 初始释放阶段更慢
        }
    }
    
    // 应用指数平滑
    newGain = limiter->gain * (1.0f - smoothingFactor) + targetGain * smoothingFactor;
    
    // 确保增益在合理范围内
    return fmaxf(fminf(newGain, 1.0f), min_gain);
}

// 处理单帧音频数据
static void processFrame(wtk_limiter_t *limiter, float* input, int frameSize) {
    float max_amp = limiter->cfg->max_amp;
    // 1. 前瞻处理（如果启用）
    float maxValue = 0.0f;
    int i;
    
    if (limiter->lookaheadBuffer) {
        // 前瞻处理：填充前瞻缓冲区
        for (i = 0; i < frameSize; i++) {
            // 将当前样本存入前瞻缓冲区
            limiter->lookaheadBuffer[limiter->bufferIndex] = input[i];
            limiter->bufferIndex = (limiter->bufferIndex + 1) % limiter->bufferSize;
            
            // 更新当前帧最大值
            float absSample = fabsf(input[i]);
            if (absSample > limiter->maxInFrame) {
                limiter->maxInFrame = absSample;
            }
        }
        
        // 2. 基于前瞻缓冲区计算最大值
        for (i = 0; i < limiter->bufferSize; i++) {
            float absSample = fabsf(limiter->lookaheadBuffer[i]);
            if (absSample > maxValue) {
                maxValue = absSample;
            }
        }
    } else {
        // 无前瞻模式：计算当前帧最大值
        for (i = 0; i < frameSize; i++) {
            float absSample = fabsf(input[i]);
            if (absSample > maxValue) {
                maxValue = absSample;
            }
        }
        limiter->maxInFrame = maxValue;
    }
    
    // 3. 计算所需增益
    float targetGain = calculateTargetGain(limiter, maxValue);
    
    // 4. 应用平滑增益变化
    limiter->gain = applyGainSmoothing(limiter, targetGain);
    
    // 5. 应用增益到当前帧
    for (i = 0; i < frameSize; i++) {
        // 应用增益
        float processed = input[i] * limiter->gain;
        
        // 应用软限幅
        processed = softClip(limiter, processed, limiter->softClipThreshold);
        
        // 确保输出不超过最大幅度
        if (processed > max_amp) {
            input[i] = max_amp;
        } else if (processed < -max_amp) {
            input[i] = -max_amp;
        } else {
            input[i] = processed;
        }
        
        // 更新包络检测器
        limiter->envelope = updateEnvelope(limiter, input[i], limiter->envelope);
    }
    
    // 保存当前增益用于下一帧平滑
    limiter->prevGain = limiter->gain;
    
    // 重置帧内最大值
    limiter->maxInFrame = 0.0f;
}

wtk_limiter_t *wtk_limiter_new(wtk_limiter_cfg_t *cfg)
{
    wtk_limiter_t *limiter = (wtk_limiter_t *)wtk_malloc(sizeof(wtk_limiter_t));

    limiter->cfg = cfg;

    limiter->envelope = 0.0f;
    limiter->gain = 1.0f;
    limiter->prevGain = 1.0f;
    limiter->bufferIndex = 0;
    limiter->maxInFrame = 0.0f;
    limiter->releaseCounter = 0.0f;
    limiter->softClipThreshold = cfg->max_amp * cfg->soft_clip_threshold;

    limiter->lookaheadBuffer = NULL;
    if (cfg->use_lookahead) {
        limiter->bufferSize = cfg->default_lookahead_samples;
        limiter->lookaheadBuffer = (float *)wtk_malloc(limiter->bufferSize * sizeof(float));
    }

    wtk_limiter_reset(limiter);
    return limiter;

}
void wtk_limiter_delete(wtk_limiter_t *limiter)
{
    if (limiter->lookaheadBuffer) {
        wtk_free(limiter->lookaheadBuffer);
    }
    wtk_free(limiter);
}
void wtk_limiter_start(wtk_limiter_t *limiter)
{

}
void wtk_limiter_reset(wtk_limiter_t *limiter)
{
    if (limiter->lookaheadBuffer) {
        memset(limiter->lookaheadBuffer, 0, limiter->bufferSize * sizeof(float));
    }
}
void wtk_limiter_feed(wtk_limiter_t *limiter, float *data, int len)
{
    processFrame(limiter, data, len);
}
