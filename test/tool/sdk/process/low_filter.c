#include <stdint.h>
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_alloc.h"

void print_usage() {
    printf("mask_bf_net usage:\n");
    printf("\t-i input wav  file\n");
    printf("\t-o output dir\n");
    printf("\n\n");
}

static int out_len = 0;
static int repeat = 0;

// 滤波器状态结构体（单通道）
typedef struct {
    float b0, b1, b2; // 分子系数
    float a1, a2;     // 分母系数
    float x1, x2;     // 输入历史
    float y1, y2;     // 输出历史
} LowPassFilter;

// 初始化100Hz低通滤波器（采样率默认44100Hz）
void init_100hz_lpf(LowPassFilter *filter) {
    // 滤波器参数计算（Butterworth二阶低通）
    const float sample_rate = 48000.0f;
    const float cutoff_freq = 100.0f;
    const float Q = 0.7071f; // Butterworth Q值

    float w0 = 2 * 3.1415926535f * cutoff_freq / sample_rate;
    float alpha = sinf(w0) / (2 * Q);

    float cos_w0 = cosf(w0);
    float b0 = (1 - cos_w0) / 2;
    float b1 = 1 - cos_w0;
    float b2 = (1 - cos_w0) / 2;
    float a0 = 1 + alpha;
    float a1 = -2 * cos_w0;
    float a2 = 1 - alpha;

    // 归一化系数
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;

    // 清零历史状态
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}

// 流式处理函数（原位处理）
void process_100hz_lpf(short *data, int len, LowPassFilter *filter) {
    for (int i = 0; i < len; i++) {
        // 将short转换为float（保持精度）
        float input = (float)data[i];

        // 应用滤波器
        float output = filter->b0 * input + filter->b1 * filter->x1 +
                       filter->b2 * filter->x2 - filter->a1 * filter->y1 -
                       filter->a2 * filter->y2;

        // 更新历史状态
        filter->x2 = filter->x1;
        filter->x1 = input;
        filter->y2 = filter->y1;
        filter->y1 = output;

        // 将结果写回（带饱和处理）
        if (output > 32767.0f) {
            data[i] = 32767;
        } else if (output < -32768.0f) {
            data[i] = -32768;
        } else {
            data[i] = (short)output;
        }
    }
}

static void mask_bf_net_test_file(LowPassFilter filter, char *ifn, char *ofn) {
    wtk_riff_t *riff;
    wtk_wavfile_t *wav;
    short *pv;
    int channel = 1;
    int rate = 48000;
    int len, bytes;
    int ret;
    double t;
    int cnt;
    // wtk_strbuf_t **input;
    // int n;
    // short *pv[10];
    // int i;

    wav = wtk_wavfile_new(rate);
    wav->max_pend = 0;
    wtk_wavfile_open(wav, ofn);

    riff = wtk_riff_new();
    wtk_riff_open(riff, ifn);

    len = 32 * 16;
    bytes = sizeof(short) * channel * len;
    pv = (short *)wtk_malloc(sizeof(short) * channel * len);

    cnt = 0;
    t = time_get_ms();

    while (1) {
        ret = wtk_riff_read(riff, (char *)pv, bytes);
        if (ret <= 0) {
            wtk_debug("break ret=%d\n", ret);
            break;
        }
        // wtk_msleep(64);
        len = ret / (sizeof(short) * channel);
        cnt += len;
        process_100hz_lpf(pv, len, &filter);
        wtk_wavfile_write(wav, (char *)pv, len * sizeof(short));
        // wtk_debug("deley %f\n",(cnt-out_len)/16.0);
    }

    t = time_get_ms() - t;
    wtk_debug("rate=%f t=%f\n", t / (cnt / (rate / 1000.0)), t);

    wtk_riff_delete(riff);
    wtk_wavfile_delete(wav);
    wtk_free(pv);
}

int main(int argc, char **argv) {
    wtk_arg_t *arg;
    char *output = NULL;
    char *wav = NULL;

    arg = wtk_arg_new(argc, argv);
    if (!arg) {
        goto end;
    }
    wtk_arg_get_str_s(arg, "i", &wav);
    wtk_arg_get_str_s(arg, "o", &output);


    // 创建滤波器实例
    LowPassFilter lpf;
    init_100hz_lpf(&lpf);
    
    if (wav) {
        mask_bf_net_test_file(lpf, wav, output);
    }
end:
    return 0;
}
