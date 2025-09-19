#include "qtk_gain_estimate.h"

qtk_gain_estimate_t *qtk_gain_estimate_new(){
    qtk_gain_estimate_t *g_est =
        (qtk_gain_estimate_t *)wtk_malloc(sizeof(qtk_gain_estimate_t));
    g_est->mic = wtk_strbuf_new(1024, 1);
    g_est->play = wtk_strbuf_new(1024, 1);
    g_est->Decay = 0.0;
    g_est->gain_echo = 0.0;
    g_est->gain_mic = 0.0;
    g_est->rt60 = 0.0;
    g_est->weight = NULL;
    g_est->snr = 0.0;
    return g_est;
}

void qtk_gain_estimate_delete(qtk_gain_estimate_t *g_est){
    wtk_strbuf_delete(g_est->mic);
    wtk_strbuf_delete(g_est->play);
    if (g_est->weight) {
        wtk_free(g_est->weight);
    }
    wtk_free(g_est);
}

void qtk_gain_estimate_reset(qtk_gain_estimate_t *g_est){
    wtk_strbuf_reset(g_est->mic);
    wtk_strbuf_reset(g_est->play);
}

int qtk_gain_estimate_feed_float(qtk_gain_estimate_t *g_est, float *data, int len, int is_end){
    int i;
    float fv1,fv2;
    int channel = 2;
    for (i = 0; i < len; ++i) {
        fv1 = data[0];
        fv2 = data[1];
        wtk_strbuf_push(g_est->mic, (char *)&(fv1), sizeof(float));
        wtk_strbuf_push(g_est->play, (char *)&(fv2), sizeof(float));
        data += channel;
    }
    return 0;
}

/*
计算信回比(SPR)
PWR_SIG:喇叭播放信号的能量。
PWR_PLYBK:信号被喇叭播放出来，被麦克风录到的声音信号的能量。
*/
static float calulate_SPR(qtk_gain_estimate_t *g_est){
    int n = g_est->mic->pos/sizeof(float),i;
    float *p;
    float PWR_SIG = 0.0;
    float PWR_PLYBK = 0.0;
    float SIG_db,PLYBK_db;
    p = (float*)g_est->mic->data;
    for(i = 0; i < n; i++){
        PWR_PLYBK += *p * *p;
        p++;
    }

    p = (float*)g_est->play->data;
    for(i = 0; i < n; i++){
        PWR_SIG += *p * *p;
        p++;
    }

    SIG_db = 10 * log10f(PWR_SIG + 1e-8);
    PLYBK_db = 10 * log10f(PWR_PLYBK + 1e-8);
    printf(">>>>>>>>>SIG_db:%f, PLYBK_db:%f, SPR:%f\n", SIG_db, PLYBK_db,
           (SIG_db - PLYBK_db));

    return SIG_db - PLYBK_db;
}

/*
rir:空间传递函数
len:rir的长度
decay_db:空间衰减，单位dB
sr:采样率，单位Hz
*/
static float calculate_rt60(qtk_gain_estimate_t *g_est, float *rir, int len,
                            int decay_db, int sr) {
    //阈值处理：将数组 rir 中绝对值小于 1e-3 的元素置为 0，保留绝对值大于 1e-3
    //的元素。
    //噪声去除：通过设置阈值，去除可能的噪声或非常小的值，从而提高后续信号处理步骤的可靠性
    float *rir_tmp = (float *)wtk_malloc(len * sizeof(float));
    for (int i = 0; i < len; ++i) {
        if (fabs(rir[i]) < 1e-3) {
            rir_tmp[i] = 0.0;
        } else {
            rir_tmp[i] = rir[i];
        }
    }
    // float sum_rir = 0.0;
    // for(int i = 0; i < len; ++i){
    //     sum_rir += rir_tmp[i];
    // }
    // printf("sum_rir:%f\n", sum_rir);
    // print_float(rir_tmp, len);

    int n = len;

    float *power = (float *)wtk_malloc(n * sizeof(float));
    float *energy = (float *)wtk_malloc(n * sizeof(float));

    memset(power, 0, n * sizeof(float));
    memset(energy, 0, n * sizeof(float));
    // power = rir_tmp ** 2
    for (int i = 0; i < n; i++) {
        power[i] = powf(rir_tmp[i], 2);
        // printf("i:%d, rir_tmp:%f, power:%f\n", i, rir_tmp[i], power[i]);
    }

    // energy = np.cumsum(power[::-1])[::-1]
    float sum = 0.0;
    for (int i = n - 1; i >= 0; --i) {
        sum += power[i];
        energy[i] = sum;
        // printf("i:%d, energy:%f\n", i, energy[i]);
    }
    // print_float(energy, n);

    // i_nz = np.max(np.where(energy > 0)[0])
    int i_nz = 0;
    for (int i = 0; i < n; ++i) {
        if (energy[i] > 0) {
            i_nz = i;
        }
    }
    // printf("i_nz:%d\n", i_nz);//47997
    // printf("energy[0]=%f\n", energy[0]);

    // energy = energy[:i_nz]
    //  float energy_db[i_nz + 1];
    float *energy_db = (float *)wtk_malloc((i_nz) * sizeof(float));
    float energy_db_0 = 10 * log10f(energy[0]);
    for (int i = 0; i < i_nz; i++) {
        energy_db[i] = 10 * log10f(energy[i]);
        energy_db[i] -= energy_db_0;
    }
    // print_float(energy_db, i_nz + 1);

    // Find i_5db
    // i_5db = np.min(np.where(-5 - energy_db > 0)[0])
    // t_5db = i_5db / sr
    int i_5db = 0;
    for (int i = 0; i < i_nz; i++) {
        if (-5 - energy_db[i] > 0) {
            i_5db = i;
            break;
        }
    }
    float t_5db = i_5db * 1.0 / sr;

    // Find i_decay
    // i_decay = np.min(np.where(-5 - decay_db - energy_db > 0)[0])
    // t_decay = i_decay / sr
    int i_decay = 0;
    for (int i = 0; i < i_nz; i++) {
        if (-5 - decay_db - energy_db[i] > 0) {
            i_decay = i;
            break;
        }
    }
    float t_decay = i_decay * 1.0 / sr;

    // Calculate decay time
    // decay_time = t_decay - t_5db
    float decay_time = t_decay - t_5db;
    // printf("t_5db:%f, t_decay:%f, decay_time:%f\n", t_5db, t_decay,
    // decay_time);

    // Free allocated memory
    wtk_free(power);
    wtk_free(energy);
    wtk_free(energy_db);
    wtk_free(rir_tmp);

    return decay_time;
}

/*
通过计算信回比(SPR)来估计增益，并分配给echo_shift和mic_shift，适用范围广。
A_max: 最大增益，单位dB
A_offset: 增益偏移，单位dB
SPR:信回比/衰减，单位dB
*/
int qtk_gain_estimate(qtk_gain_estimate_t *g_est, float A_max, float A_offset) {
    float SPR = calulate_SPR(g_est);
    float gain_db = (A_max + SPR + A_offset) / 2;

    g_est->gain_echo = powf(10, gain_db/20);
    g_est->gain_mic = powf(10, gain_db / 20);
    printf("gain_echo:%f, gain_mic:%f\n", g_est->gain_echo, g_est->gain_mic);
    return 0;
}

int qtk_gain_estimate_decay(qtk_gain_estimate_t *g_est, float A_max) {
    float gain_db = (A_max - g_est->Decay) / 2;

    g_est->gain_echo = powf(10, gain_db / 20);
    g_est->gain_mic = powf(10, gain_db / 20);
    return 0;
}

float qtk_gain_estimate_rir(qtk_gain_estimate_t *g_est, float *rir, int len,
                            int sr, int direct_sound_idx, float A_max) {
    float rt60 = 3 * calculate_rt60(g_est, rir, len, 20, sr);
    g_est->rt60 = rt60;
    int N = (int)(rt60 * sr);
    int n = len;
    if (N > n) {
        N = n;
    }
    // print_float(rir, n);
    printf("N:%d, rt60:%f\n", N, rt60);
    float sum_power_before_N = 0.0;
    float sum_power_after_N = 0.0;

    for (int i = 0; i < N; i++) {
        sum_power_before_N += powf(rir[i], 2);
    }

    for (int i = N; i < n; i++) {
        sum_power_after_N += powf(rir[i], 2);
    }
    // wtk_debug("sum_power_before_N:%f, sum_power_after_N:%f\n",
    // sum_power_before_N, sum_power_after_N);
    g_est->snr = 10 * log10(sum_power_before_N) - 10 * log10(sum_power_after_N);

    int a = direct_sound_idx;
    float sum_decay = 0.0;
    for (int i = a; i < len; i++) {
        sum_decay += powf(rir[i], 2);
    }
    g_est->Decay = 10 * log10f(sum_decay);
    qtk_gain_estimate_decay(g_est, A_max);

    return 0;
}

static float cabs_float(wtk_complex_t z) {
    return sqrtf(z.a * z.a + z.b * z.b);
}

/*
利用rir测量最小扩声增益，适用于纯kalman
rir:空间传递函数
len:rir长度
direct_sound_idx:直达声的索引
L:kalman滤波器的抽头数
*/
float qtk_gain_estimate_rir_v2(qtk_gain_estimate_t *g_est, float *rir, int len,
                               int direct_sound_idx, int L) {
    int sr = 16000;
    int THRESHOLD = 0.05;
    float rt60 = 3 * calculate_rt60(g_est, rir, len, 20, sr);
    // rt60 = 0.2188125;
    g_est->rt60 = rt60;
    printf("rt60:%f\n", rt60);
    int N = (int)(rt60 * sr);
    int start_idx = direct_sound_idx + L * 128;
    wtk_debug("start_idx:%d, direct_sound_idx:%d, L:%d\n", start_idx,
              direct_sound_idx, L);
    if (start_idx >= len) {
        return 0; //防止越界
    }
    int sig_len = N;
    // qtk_gain_estimate_new2(g_est, rir + start_idx, sig_len, 128);
    if (N > len || N < start_idx) {
        return 15;
    }

    //    sig_len =  sig_len % 32 == 0? sig_len : sig_len + (32 - sig_len % 32);

    wtk_debug("N:%d, sig_len:%d\n", N, sig_len);

    float *tmp_weight = (float *)wtk_malloc(sig_len * sizeof(float));
    wtk_debug("sig_len:%d, start_idx:%d\n", sig_len, start_idx);
    memcpy(tmp_weight, rir, sig_len * sizeof(float));

    for (int i = 0; i < start_idx; i++) {
        tmp_weight[i] = 0.0;
    }

    int n = sig_len;
    // N = n * 2;
    g_est->drft = wtk_drft_new(n);
    g_est->weight =
        (wtk_complex_t *)wtk_malloc((n / 2 + 1) * sizeof(wtk_complex_t));
    float *abs_rir = (float *)malloc((n / 2 + 1) * sizeof(float));
    float *gain = (float *)malloc((n / 2 + 1) * sizeof(float));
    int abs_count = 0;

    // print_float(tmp_weight, sig_len);
    wtk_drft_fft2(g_est->drft, tmp_weight, g_est->weight);
    // print_float(g_est->weight->a, sig_len / 2 +1);
    for (int i = 0; i < n / 2 + 1; ++i) {
        g_est->weight[i].a *= n;
        g_est->weight[i].b *= n;
        // printf("i:%d, a:%f, b:%f\n", i, g_est->weight[i].a,
        // g_est->weight[i].b);
    }
    for (int i = 0; i < n / 2 + 1; ++i) {
        float abs_value = cabs_float(g_est->weight[i]); // 取复数的模
        if (abs_value > 0.02) {
            // printf("i:%d, abs_value:%f\n", i, abs_value);
            abs_rir[abs_count] = abs_value;
            gain[abs_count] = 1.0 / (abs_rir[abs_count] + 1e-8);
            abs_count++;
        }
    }
    if ((abs_count == 0)) {
        return 15;
    }

    // for(int i = 0; i < abs_count; ++i){
    //     printf("i:%d, abs_rir:%0.9f, gain:%0.9f\n", i, abs_rir[i], gain[i]);
    // }

    // 计算直方图
    int bin_counts[50] = {0}; // 直方图计数
    float bin_edges[51];      // 直方图边缘
    float max_gain = gain[0];
    float min_gain = gain[0];

    for (int i = 0; i < abs_count; i++) { //确定直方图的上下界
        if (gain[i] > max_gain) {
            max_gain = gain[i];
        }
        if (gain[i] < min_gain) {
            min_gain = gain[i];
        }
    }

    float bin_width = (max_gain - min_gain) / 50;
    // 计算直方图边缘
    for (int i = 0; i < 51; i++) {
        bin_edges[i] = min_gain + i * bin_width; // 根据需求调整
    }

    // 统计增益到直方图
    for (int i = 0; i < abs_count; i++) {
        int bin_index = (int)((gain[i] - min_gain) / bin_width);
        // printf("i:%d, bin_index:%d, gain:%f\n", i, bin_index, gain[i]);
        if (bin_index >= 0 && bin_index < 50) {
            bin_counts[bin_index]++;
        } else if (bin_index == 50) {
            bin_counts[49]++; //大于等于最大值，归入最大值区间
        }
    }
    // print_int(bin_counts, 50);
    // print_float(bin_edges, 51);

    // 计算累计分布
    float cumulative_sum = 0.0;
    float total_sum = 0;
    for (int i = 0; i < 50; i++) {
        total_sum += bin_counts[i]; //计算累计和
    }

    for (int i = 0; i < 50; i++) {
        cumulative_sum += bin_counts[i];
        if (cumulative_sum / total_sum > THRESHOLD) {
            min_gain = bin_edges[i]; //取所在区间的下界
            break;
        }
    }
    min_gain /= 2; // 减小3dB

    wtk_free(tmp_weight);
    // wtk_debug("min_gain:%f\n", min_gain);
    wtk_free(abs_rir);
    wtk_free(gain);
    // printf("min_gain:%f\n", min_gain);
    return min_gain;
}

int qtk_gain_estimate_feed(qtk_gain_estimate_t *g_est, short *data, int len, int is_end){
    int i;
    float fv1,fv2;
    int channel = 2;
    for (i = 0; i < len; ++i) {
        fv1 = data[0] * 1.0 / 32768.0;
        fv2 = data[1] * 1.0 / 32768.0;
        wtk_strbuf_push(g_est->mic, (char *)&(fv1), sizeof(float));
        wtk_strbuf_push(g_est->play, (char *)&(fv2), sizeof(float));
        data += channel;
    }
    return 0;
}
