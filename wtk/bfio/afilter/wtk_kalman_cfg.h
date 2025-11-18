#ifndef WTK_BFIO_AFILTER_WTK_KALMAN_CFG
#define WTK_BFIO_AFILTER_WTK_KALMAN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kalman_cfg wtk_kalman_cfg_t;
struct wtk_kalman_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int channel;     // 总信号通道数
    int nmicchannel; // 近端信号通道数
    int nspchannel;
    int rate;
    int wins;             // 窗长
    int nbin;             // 频点数
    int B;                // 卡尔曼滤波器阶数
    int L;                // 单帧长度
    float A;              // 状态向量 W 的衰减系数。用于控制 W 的更新速率
    float P_init;         // 协方差矩阵 P 的初始值
    float alpha;          // 残差信号协方差 Phi_SS 的平滑系数
    float beta;           // 状态协方差 Phi_delta 的平滑系数
    float update_thresh;  // 更新状态 W 的功率阈值。低于此阈值时不进行更新
    float clip_thresh;    // 协方差矩阵 P 的下限，防止数值不稳定
    float phi_delta_init; // 状态向量协方差初始值
    float phi_ss_init;    // 噪声/干扰的协方差初始值
    int nl;

    unsigned int use_sec_iter : 1; // 是否计算完滤波器参数后进行二阶迭代
    unsigned int use_res : 1; // 是否启用残余回声抑制
};

int wtk_kalman_cfg_init(wtk_kalman_cfg_t *cfg);
int wtk_kalman_cfg_clean(wtk_kalman_cfg_t *cfg);
int wtk_kalman_cfg_update_local(wtk_kalman_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_kalman_cfg_update(wtk_kalman_cfg_t *cfg);
int wtk_kalman_cfg_update2(wtk_kalman_cfg_t *cfg, wtk_source_loader_t *sl);

wtk_kalman_cfg_t *wtk_kalman_cfg_new(char *fn);
void wtk_kalman_cfg_delete(wtk_kalman_cfg_t *cfg);
wtk_kalman_cfg_t *wtk_kalman_cfg_new_bin(char *fn);
void wtk_kalman_cfg_delete_bin(wtk_kalman_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif