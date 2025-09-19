#ifndef WTK_BFIO_QFORM_WTK_RTJOIN2_CFG
#define WTK_BFIO_QFORM_WTK_RTJOIN2_CFG
#include "wtk/bfio/agc/qtk_gain_controller.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rtjoin2_cfg wtk_rtjoin2_cfg_t;
struct wtk_rtjoin2_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int rate;

    int channel;
    int *mic_channel;
    int nmicchannel;
    int *sp_channel;
    int nspchannel;
    int clip_s;
    int clip_e;

    float spenr_thresh;
    int spenr_cnt;
    float micenr_thresh;
    int micenr_cnt;

    int kernel_len;
    int N1;
    int N2;
    int N;
    int slid_len;
    int conv_len;
    float sp_conv_thresh;

    float align_duration; // 对齐信号的持续时间
    float align_freq;     // 对齐信号的频率
    float align_amp;      // 对齐信号的幅度
    float align_amp2;     // 对齐信号中间周期的幅度
    short *play_signal;   // 播放的对齐信号
    int play_signal_len;  // 播放对齐信号的长度
    float *align_signal;  // 对齐参考信号
    int align_signal_len; // 对齐参考信号的长度
    int align_min_frame;
    int align_data_len;
    int align_conv_len;
    float align_thresh;
    float align_min_thresh;
    float align_mean_thresh;
    int min_thresh_cnt;
    int play_cnt;
    int blank_frame;

    int rob_winsize;             // 短期滑动窗口大小
    float rob_conf_decay;        // 置信度统计量衰减系数
    float rob_mad_threshold;     // MAD异常值阈值
    float rob_reliability_ratio; // 可靠性判断阈值 (MAD倍数)
    float rob_prob_mu;           // 置信度统计量均值
    float rob_prob_sigma;        // 置信度统计量标准差
    float rob_long_mu;           // 长期延迟均值
    float rob_long_sigma;        // 长期延迟标准差
    float rob_long_mad;          // 长期延迟MAD值
    int delay_in_cnt;
    int delay_frame_cnt;
    float rob_long_alpha1;
    float rob_long_alpha2;

    float power_alpha;
    float weight_alpha;
    float weight_thresh;

    qtk_ahs_gain_controller_cfg_t gc;
    float gc_gain;
    float gc_min_thresh;
    int gc_cnt;

    float max_out;
    int out_agc_level;

    wtk_equalizer_cfg_t eq;

    int featm_lm;
    int featsp_lm;
    char *aecmdl_fn;
    wtk_gainnet2_cfg_t *gainnet2;
    wtk_bankfeat_cfg_t bankfeat;

    unsigned use_eq : 1;
    unsigned use_control_bs : 1;
    unsigned use_conv_fft : 1;
    unsigned use_conv_power : 1;
    unsigned use_rob_filter : 1;
    unsigned use_mul_out : 1;
    unsigned use_avg_mix : 1;
    unsigned use_bs_win : 1;
    unsigned use_gc : 1;
    unsigned use_gainnet2 : 1;
    unsigned use_rbin_res : 1;
    unsigned use_align_signal : 1;
};

int wtk_rtjoin2_cfg_init(wtk_rtjoin2_cfg_t *cfg);
int wtk_rtjoin2_cfg_clean(wtk_rtjoin2_cfg_t *cfg);
int wtk_rtjoin2_cfg_update_local(wtk_rtjoin2_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_rtjoin2_cfg_update(wtk_rtjoin2_cfg_t *cfg);
int wtk_rtjoin2_cfg_update2(wtk_rtjoin2_cfg_t *cfg, wtk_source_loader_t *sl);

void wtk_rtjoin2_cfg_set_channel(wtk_rtjoin2_cfg_t *cfg, int channel,
                                 char *micchannel, char *spkchannel);

wtk_rtjoin2_cfg_t *wtk_rtjoin2_cfg_new(char *fn);
void wtk_rtjoin2_cfg_delete(wtk_rtjoin2_cfg_t *cfg);
wtk_rtjoin2_cfg_t *wtk_rtjoin2_cfg_new_bin(char *fn);
void wtk_rtjoin2_cfg_delete_bin(wtk_rtjoin2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
