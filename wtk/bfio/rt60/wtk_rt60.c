#include "wtk_rt60.h"
#ifndef WTK_WAV_SHORT_TO_FLOAT
#define WTK_WAV_SHORT_TO_FLOAT(f) ((f) > 0 ? (f / 32767.0) : (f / 32768.0))
#endif
#ifndef WTK_WAV_FLOAT_TO_SHORT
#define WTK_WAV_FLOAT_TO_SHORT(f)                                              \
    ((f) > 0 ? floorf(f * 32767.0 + 0.5) : floorf(f * 32768.0 + 0.5))
#endif

wtk_rt60_t *wtk_rt60_new(wtk_rt60_cfg_t *cfg) {
    wtk_rt60_t *rt60;

    rt60 = (wtk_rt60_t *)wtk_malloc(sizeof(wtk_rt60_t));
    rt60->cfg = cfg;
    rt60->ths = NULL;
    rt60->notify = NULL;

    rt60->mic = wtk_strbufs_new(rt60->cfg->nmicchannel);

    wtk_rt60_reset(rt60);

    return rt60;
}

void wtk_rt60_delete(wtk_rt60_t *rt60) {
    wtk_strbufs_delete(rt60->mic, rt60->cfg->nmicchannel);

    wtk_free(rt60);
}

void wtk_rt60_start(wtk_rt60_t *rt60) {}

void wtk_rt60_reset(wtk_rt60_t *rt60) {
    wtk_strbufs_reset(rt60->mic, rt60->cfg->nmicchannel);
}

void wtk_rt60_set_notify(wtk_rt60_t *rt60, void *ths,
                         wtk_rt60_notify_f notify) {
    rt60->notify = notify;
    rt60->ths = ths;
}

float *wtk_rt60_estimate_rir(wtk_rt60_t *rt60, int *len) {
    // float *sweep = rt60->cfg->sweep;
    float *inv_sweep = rt60->cfg->inv_sweep;
    int sweep_len = rt60->cfg->sweep_len;
    
    wtk_strbuf_t *mic = rt60->mic[0];
    int pos = mic->pos / sizeof(float);
    int i;
    float *fv = (float *)mic->data;
    float *rir;
    int left_idx, right_idx;

    int rir_len = pos + sweep_len - 1;
    int fft_size = 1;
    int pulse_idx = 0;
    float max_val = 0.0f;
    int max_idx = 0;
    while (fft_size < rir_len) {
        fft_size *= 2;
    }
    int nbin = fft_size / 2 + 1;

    wtk_drft_t *drft = wtk_drft_new(fft_size);
    float *tmp_in = (float *)wtk_malloc(fft_size * sizeof(float));

    wtk_complex_t *fft_kernel = (wtk_complex_t *)wtk_malloc(fft_size * sizeof(wtk_complex_t));
    wtk_complex_t *fft_in = (wtk_complex_t *)wtk_malloc(fft_size * sizeof(wtk_complex_t));
    wtk_complex_t *fft_out = (wtk_complex_t *)wtk_malloc(fft_size * sizeof(wtk_complex_t));


    memset(tmp_in, 0, fft_size * sizeof(float));

    memcpy(tmp_in, inv_sweep, sweep_len * sizeof(float));
    wtk_drft_fft3(drft, tmp_in, fft_kernel);

    // memset(tmp_in, 0, fft_size * sizeof(float));
    // memcpy(tmp_in, sweep, sweep_len * sizeof(float));

    // wtk_drft_fft3(drft, tmp_in, fft_in);

    // for (i = 0; i < nbin; ++i) {
    //     fft_out[i].a = fft_kernel[i].a * fft_in[i].a - fft_kernel[i].b * fft_in[i].b;
    //     fft_out[i].b = fft_kernel[i].a * fft_in[i].b + fft_kernel[i].b * fft_in[i].a;
    // }

    // wtk_drft_ifft3(drft, fft_out, tmp_in);

    // #include "wtk/core/wtk_wavfile.h"
    // wtk_wavfile_t *wav;
    // wav = wtk_wavfile_new(rt60->cfg->rate);
    // wtk_wavfile_set_channel(wav, 1);
    // wtk_wavfile_open(wav, "sweep.wav");
    // short *wav_data = (short *)wtk_malloc(sizeof(short) * fft_size);
    // for (i = 0; i < fft_size; ++i) {
    //     wav_data[i] = WTK_WAV_FLOAT_TO_SHORT(tmp_in[i] * 0.2);
    // }
    // wtk_wavfile_write(wav, (char *)wav_data, sizeof(short) * fft_size);
    // wtk_free(wav_data);
    // wtk_wavfile_close(wav);

    // pulse_idx = wtk_float_argmax(tmp_in, rir_len);
    // max_val = tmp_in[pulse_idx] * tmp_in[pulse_idx];
    // printf("pulse_idx = %d, max_val = %f\n", pulse_idx, max_val);

    pulse_idx = floor(rt60->cfg->rate * rt60->cfg->t) - 1;
    max_val = 1.0;
    // printf("pulse_idx = %d, max_val = %f\n", pulse_idx, max_val);

    memset(tmp_in, 0, fft_size * sizeof(float));
    memcpy(tmp_in, fv, pos * sizeof(float));

    wtk_drft_fft3(drft, tmp_in, fft_in);

    for (i = 0; i < nbin; ++i) {
        fft_out[i].a = fft_kernel[i].a * fft_in[i].a - fft_kernel[i].b * fft_in[i].b;
        fft_out[i].b = fft_kernel[i].a * fft_in[i].b + fft_kernel[i].b * fft_in[i].a;
    }
    wtk_drft_ifft3(drft, fft_out, tmp_in);


    max_idx = wtk_float_argmax(tmp_in + pulse_idx, rir_len - pulse_idx);
    left_idx = max(0, max_idx + pulse_idx - 20);
    right_idx = min(rir_len - 1, max_idx + pulse_idx + 16000 * 2);

    *len = right_idx - left_idx + 1;
    rir = (float *)wtk_malloc((right_idx - left_idx + 1) * sizeof(float));
    memcpy(rir, tmp_in + left_idx, (right_idx - left_idx + 1) * sizeof(float));

    for (i = 0; i < *len; ++i) {
        rir[i] /= max_val;
    }

    wtk_drft_delete(drft);
    wtk_free(tmp_in);
    wtk_free(fft_kernel);
    wtk_free(fft_in);
    wtk_free(fft_out);

    return rir;
}


// 计算线性回归（最小二乘法）
void wtk_rt60_linear_regression(float* x, float* y, int n, float* slope, float* intercept) {
    float sum_x = 0.0f, sum_y = 0.0f;
    float sum_xx = 0.0f, sum_xy = 0.0f;
    
    for (int i = 0; i < n; i++) {
        sum_x += x[i];
        sum_y += y[i];
        sum_xx += x[i] * x[i];
        sum_xy += x[i] * y[i];
    }
    
    float denominator = n * sum_xx - sum_x * sum_x;
    
    // 避免除以零
    if (fabsf(denominator) < FLT_EPSILON) {
        *slope = 0.0f;
        *intercept = 0.0f;
        return;
    }
    
    *slope = (n * sum_xy - sum_x * sum_y) / denominator;
    *intercept = (sum_y - *slope * sum_x) / n;
}

// 计算RT60（使用Schroeder积分法）
float wtk_rt60_rt60ulate_rt60(wtk_rt60_t* rt60, float* impulse_response, int len) {
    float fs = (float)rt60->cfg->rate;
    float max_edc = 0.0f;
    
    float *energy = (float *)wtk_malloc(len * sizeof(float));
    float *time = (float *)wtk_malloc(len * sizeof(float));
    float *edc = (float *)wtk_malloc(len * sizeof(float));
    float *edc_dB = (float *)wtk_malloc(len * sizeof(float));

    // 1. 计算能量和Schroeder积分
    for (int i = 0; i < len; i++) {
        // 计算平方能量
        energy[i] = impulse_response[i] * impulse_response[i];
        
        // 预计算时间轴（只做一次）
        time[i] = i  * 1.0 / fs;
    }
    
    // 2. Schroeder反向积分（从尾部向前累加）
    float accum = 0.0f;
    for (int i = len - 1; i >= 0; i--) {
        accum += energy[i];
        edc[i] = accum;
        if (accum > max_edc) max_edc = accum;
    }
    
    // 3. 归一化到dB并寻找区间
    int start_idx = -1, end_idx = -1;
    float min_value = FLT_MAX;
    
    for (int i = 0; i < len; i++) {
        // 避免除以零
        if (max_edc < FLT_EPSILON) {
            edc_dB[i] = -120.0f; // 极小的dB值
        } else {
            edc_dB[i] = 10.0f * log10f(edc[i] / max_edc);
        }
        
        // 更新最小值
        if (edc_dB[i] < min_value) min_value = edc_dB[i];
        
        // 寻找-5dB点
        if (start_idx == -1 && edc_dB[i] < -5.0f) {
            start_idx = i;
        }
        
        // 寻找-25dB点（使用-30dB作为结束点，与原始Python代码一致）
        if (edc_dB[i] < -30.0f) {
            end_idx = i;
            break;
        }
    }
    
    // 4. 验证衰减区间有效性
    // 如果整个曲线衰减不足-5dB，或者结束点在开始点之前
    if (start_idx == -1 || end_idx <= start_idx || (end_idx - start_idx) < 5) {
        // 尝试使用衰减到-60dB的20%点作为替代方案
        if (min_value < -60.0f) {
            start_idx = (int)(len * 0.1); // 10%处作为起点
            for (end_idx = len - 1; end_idx > start_idx; end_idx--) {
                if (edc_dB[end_idx] > -60.0f) {
                    end_idx = end_idx > start_idx + 5 ? end_idx : start_idx + 5;
                    break;
                }
            }
            
            if (end_idx <= start_idx || (end_idx - start_idx) < 5) {
                // 仍然无效，返回错误值
                return -1.0f; 
            }
        } else {
            return -1.0f; // 无效值表示计算失败
        }
    }
    
    // 5. 线性回归计算斜率
    float slope, intercept;
    wtk_rt60_linear_regression(time + start_idx, 
                      edc_dB + start_idx, 
                      end_idx - start_idx + 1, 
                      &slope, &intercept);
    

    wtk_free(energy);
    wtk_free(time);
    wtk_free(edc);
    wtk_free(edc_dB);
    // 6. 计算RT60（使用优化的计算方式）
    if (fabsf(slope) < FLT_EPSILON) {
        return -1.0f; // 避免除以零
    }

    return -60.0f / slope;
}

void wtk_rt60_feed(wtk_rt60_t *rt60, short *data, int len, int is_end) {
    int i, j;
    int nmicchannel = rt60->cfg->nmicchannel;
    int *mic_channel = rt60->cfg->mic_channel;
    int channel = rt60->cfg->channel;
    wtk_strbuf_t **mic = rt60->mic;
    float fv;

    for (i = 0; i < len; ++i) {
        for (j = 0; j < nmicchannel; ++j) {
            fv = WTK_WAV_SHORT_TO_FLOAT(data[mic_channel[j]]);
            wtk_strbuf_push(mic[j], (char *)&(fv), sizeof(float));
        }
        data += channel;
    }

    if (is_end) {
        int rir_len;
        float *rir = wtk_rt60_estimate_rir(rt60, &rir_len);
        float rt60_val = wtk_rt60_rt60ulate_rt60(rt60, rir, rir_len);
        // printf("RT60: %f\n", rt60_val);
        if (rt60->notify) {
            rt60->notify(rt60->ths, rt60_val);
        }
        wtk_free(rir);
    }
}

short *wtk_rt60_get_play_signal(wtk_rt60_t *rt60, int *len) {
    *len = rt60->cfg->sweep_len;
    return rt60->cfg->play_sweep;
}
