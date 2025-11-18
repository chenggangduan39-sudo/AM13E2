#include "kiss_fft.h"
#include "wtk/core/math/wtk_math.h"
#include "denoise.h"
#include "YIN.h"

typedef struct {
  int init;
  kiss_fft_state *kfft;
} CommonState4YIN;

static CommonState4YIN common;

static void check_init() {
  if (common.init)
    return;
  common.kfft = opus_fft_alloc_twiddles(2 * FRAME_SIZE, NULL, NULL, NULL, 0);
  common.init = 1;
}

void yin_clean(){
  if (common.init){
    free(common.kfft->bitrev);
    free(common.kfft->twiddles);
    free(common.kfft);
  }
}

const float betaDist2[100] = {
    0.012614, 0.022715, 0.030646, 0.036712, 0.041184, 0.044301, 0.046277,
    0.047298, 0.047528, 0.047110, 0.046171, 0.044817, 0.043144, 0.041231,
    0.039147, 0.036950, 0.034690, 0.032406, 0.030133, 0.027898, 0.025722,
    0.023624, 0.021614, 0.019704, 0.017900, 0.016205, 0.014621, 0.013148,
    0.011785, 0.010530, 0.009377, 0.008324, 0.007366, 0.006497, 0.005712,
    0.005005, 0.004372, 0.003806, 0.003302, 0.002855, 0.002460, 0.002112,
    0.001806, 0.001539, 0.001307, 0.001105, 0.000931, 0.000781, 0.000652,
    0.000542, 0.000449, 0.000370, 0.000303, 0.000247, 0.000201, 0.000162,
    0.000130, 0.000104, 0.000082, 0.000065, 0.000051, 0.000039, 0.000030,
    0.000023, 0.000018, 0.000013, 0.000010, 0.000007, 0.000005, 0.000004,
    0.000003, 0.000002, 0.000001, 0.000001, 0.000001, 0.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 0.000000};

PitchState * init_pitchstate() {
  PitchState *pst;
  pst = (PitchState *)malloc(sizeof(PitchState));
  memset(pst, 0, sizeof(*pst));
  return pst;
};

// ======================== 核心算法函数 ========================
void differenceFunction(const float *x, int yin_frameLength, float *df) {
  int tau_max_ = yin_frameLength < tau_max ? yin_frameLength : tau_max;
  int W = yin_frameLength;
  int i = 0;
  // 计算累积平方和
  float *x_cumsum = (float *)calloc(W + 1, sizeof(float));
  x_cumsum[0] = 0;
  for (int i = 0; i < W; ++i) {
    x_cumsum[i + 1] = x_cumsum[i] + x[i] * x[i];
  }

  const float r_t_0 = x_cumsum[W];

  // 使用FFT计算自相关
  check_init();
  kiss_fft_cpx fft_in[2 * W];
  kiss_fft_cpx fc[2 * W];
  for (i = 0; i < W; i++) {
    fft_in[i].r = x[i];
    fft_in[i].i = 0;

    fft_in[i + W].r = 0;
    fft_in[i + W].i = 0;
  }

  opus_fft(common.kfft, fft_in, fc, 0);
  for (i = 0; i < 2 * W; i++) {
    fc[i].r = fc[i].r * 2 * W;
    fc[i].i = fc[i].i * 2 * W;
  }

  // 计算自相关
  kiss_fft_cpx fft_power[2 * W];
  for (i = 0; i < W + 1; i++) {
    fft_power[i].r = fc[i].r * fc[i].r + fc[i].i * fc[i].i;
    fft_power[i].i = 0;
  }
  for (i = 1; i < W; i++) {
    fft_power[W + i].r = fft_power[W - i].r;
    fft_power[W + i].i = 0;
  }

  kiss_fft_cpx conv_[2 * W];
  float *conv = (float *)calloc(2 * W, sizeof(float));
  memset(conv_, 0, sizeof(kiss_fft_cpx) * 2 * W);
  memset(conv, 0, sizeof(float) * 2 * W);
  opus_fft(common.kfft, fft_power, conv_, 0);

  conv[0] = conv_[0].r;
  for (i = 1; i < 2 * W; i++) {
    conv[i] = conv_[2 * W - i].r;
  }
  
  // 计算差分函数
  for (int tau = 0; tau < tau_max_; tau++) {
    float r_t_tau_0 = x_cumsum[W - tau] - x_cumsum[tau];
    df[tau] = r_t_0 + r_t_tau_0 - 2 * conv[tau];
  }

  // 释放内存
  free(x_cumsum);
  free(conv);
}

void cumulativeMeanNormalizedDifferenceFunction(float *df, int len,
                                                float *cmndf) {
  cmndf[0] = 1.0;
  float cumsum = 0.0;
  for (int i = 1; i < len; i++) {
    cumsum += df[i];
    cmndf[i] = (cumsum > 1e-10) ? (df[i] * i / cumsum) : 1.0;
  }
}

void PyinProb(float *yinBuffer, float *peakProb, int *candidateCount,
              float **candidateTau, float **probability) {
  const int nThreshold = 100;
  float thresholds[100];
  for (int i = 0; i < nThreshold; ++i)
    thresholds[i] = 0.01 * i;

  float minVal = 1e7;
  int tau = tau_min;

  // 临时存储候选值
  float *tmpTau = (float *)malloc((tau_max - tau_min) * sizeof(float));
  float *tmpProb = (float *)malloc((tau_max - tau_min) * sizeof(float));
  *candidateCount = 0;

  while (tau < tau_max - 1) {
    if (yinBuffer[tau] < thresholds[nThreshold - 1] &&
        yinBuffer[tau + 1] < yinBuffer[tau]) {
      // 找到局部最小值
      while (tau < tau_max - 1 && yinBuffer[tau + 1] < yinBuffer[tau])
        tau++;

      if (yinBuffer[tau] < minVal && tau > 2) {
        minVal = yinBuffer[tau];
      }

      // 计算概率
      float prob = 0.0;
      for (int i = nThreshold - 1; i >= 0 && thresholds[i] > yinBuffer[tau];
           i--) {
        prob += betaDist2[i];
      }

      // 保存候选值
      tmpTau[*candidateCount] = tau;
      tmpProb[*candidateCount] = prob;
      (*candidateCount)++;
    }
    tau++;
  }

  // 分配结果内存
  *candidateTau = (float *)malloc(*candidateCount * sizeof(float));
  *probability = (float *)malloc(*candidateCount * sizeof(float));
  memcpy(*candidateTau, tmpTau, *candidateCount * sizeof(float));
  memcpy(*probability, tmpProb, *candidateCount * sizeof(float));

  free(tmpTau);
  free(tmpProb);
}

float RMS(const float *input, int len) {
  float sum = 0.0;
  for (int i = 0; i < len; ++i)
    sum += input[i] * input[i];
  return sqrt(sum / len);
}

int yinPitch(float *cmdf) {
  for (int tau = tau_min; tau < tau_max; tau++) {
    if (cmdf[tau] < harmo_th) {
      while (tau < tau_max - 1 && cmdf[tau + 1] < cmdf[tau])
        tau++;
      return tau;
    }
  }
  return 0;
}

float Pyin_refine_pitch(const float *candidate_pitch_period,
                        const float *candidate_prob, float probability_smoothed,
                        float *pitch_confidence_context,
                        float prev_pitch_period, float cur_pitch_period,
                        int candidate_count) {
  const float threshold_offset = 0.1;
  const float threshold_onset = 0.1;

  if (candidate_count > 0) {
    if (prev_pitch_period > 0) {
      if (cur_pitch_period > 0) {
        // 音高跳跃检测
        float ratio = (float)prev_pitch_period / cur_pitch_period;
        if (ratio >= 1.5 || ratio <= 0.6) {
          float *cost = (float *)malloc(candidate_count * sizeof(float));
          for (int i = 0; i < candidate_count; i++) {
            float jump =
                1.0 - fabs(log2(prev_pitch_period / candidate_pitch_period[i]));
            cost[i] = jump + candidate_prob[i];
          }

          // 选择最佳候选
          float maxCost = cost[0];
          int bestIndex = 0;
          for (int i = 1; i < candidate_count; i++) {
            if (cost[i] > maxCost) {
              maxCost = cost[i];
              bestIndex = i;
            }
          }

          int pitch_period = candidate_pitch_period[bestIndex];
          free(cost);
          return pitch_period;
        }
      } else {
        // 从有声音到无声音
        if (probability_smoothed > threshold_offset) {
          float *cost = (float *)malloc(candidate_count * sizeof(float));
          for (int i = 0; i < candidate_count; i++) {
            float jump =
                1.0 - fabs(log2(prev_pitch_period / candidate_pitch_period[i]));
            cost[i] = jump + candidate_prob[i];
          }

          float maxCost = cost[0];
          int bestIndex = 0;
          for (int i = 1; i < candidate_count; i++) {
            if (cost[i] > maxCost) {
              maxCost = cost[i];
              bestIndex = i;
            }
          }

          int pitch_period = candidate_pitch_period[bestIndex];
          free(cost);
          return pitch_period;
        }
      }
    } else {
      // 从无声音到有声音
      if (cur_pitch_period > 0) {
        float min_prob = pitch_confidence_context[0];
        for (int i = 1; i < 3; i++) {

          if (min_prob > pitch_confidence_context[i]) {
            min_prob = pitch_confidence_context[i];
          }
        }

        if (min_prob < threshold_onset) {
          return 0;
        }
      }
    }
  }
  return cur_pitch_period;
}

void Pyin_process(PitchState *ps, int yin_frameSize) {
  // 计算差分函数和CMNDF
  float *df = (float *)malloc(tau_max * sizeof(float));
  differenceFunction(ps->YIN_BUFF, yin_frameSize, df);

  float *cmndf = (float *)malloc(tau_max * sizeof(float));
  cumulativeMeanNormalizedDifferenceFunction(df, tau_max, cmndf);

  // 概率计算
  float *peakProb = (float *)calloc(tau_max, sizeof(float));
  memset(peakProb,0,sizeof(float) * tau_max);
  int candidateCount = 0;
  float *candidateTau = NULL, *probability = NULL;
  PyinProb(cmndf, peakProb, &candidateCount, &candidateTau, &probability);

  // 计算RMS并调整概率
  float rms = RMS(ps->YIN_BUFF, yin_frameSize);
  if (rms < YIN_lowAmp) {
    for (int i = 0; i < candidateCount; ++i) {
      probability[i] *= (rms + 0.01 * YIN_lowAmp) / (1.01 * YIN_lowAmp);
    }
  }
  // 计算基频
  int bestTau = yinPitch(cmndf);
  ps->pitch_period = bestTau;

  // 计算概率
  float maxProb = 0;
  for (int i = 0; i < candidateCount; ++i) {
    if (probability[i] > maxProb)
      maxProb = probability[i];
  }
  // 平滑概率
  ps->pitch_confidence = (1 - YIN_prob_smooth_factor) * (ps->pitch_confidence) +
                         YIN_prob_smooth_factor * maxProb;

  if (ps->pitch_confidence < 0.1) {
    ps->pitch_period = 0;
  }

  // refine pitch
  RNN_MOVE(ps->pitch_confidence_context, &ps->pitch_confidence_context[1], 2);
  ps->pitch_confidence_context[2] = ps->pitch_confidence;

  ps->pitch_period =
      Pyin_refine_pitch(candidateTau, probability, ps->pitch_confidence,
                        ps->pitch_confidence_context, ps->prev_pitch_period,
                        ps->pitch_period, candidateCount);
  ps->prev_pitch_period = ps->pitch_period;
  if (ps->pitch_period <= PITCH_MIN_PERIOD) {
    ps->pitch_period = 0;
  }
  if (ps->pitch_period >= PITCH_MAX_PERIOD) {
    ps->pitch_period = 0;
  }

  // 清理资源
  free(df);
  free(cmndf);
  free(peakProb);
  free(candidateTau);
  free(probability);
}
