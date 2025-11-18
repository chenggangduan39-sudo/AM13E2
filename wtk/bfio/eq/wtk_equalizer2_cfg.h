#ifndef WTK_EQUALIZER2_CFG_H
#define WTK_EQUALIZER2_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/math/wtk_math.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_equalizer2_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int band_count;
    float *rate;  // 频段边界频率数组 (Hz)
    float *value;  // 各频段增益数组 (dB)
    float sfreq;  // 采样率 (Hz)

    int nfir;  // FIR系数数量（抽头数）
    float kdb; // 凯泽窗参数，控制旁瓣水平 (30=锐利截止, 60=缓慢截止)
    float *cf;

    unsigned use_pffft;

} wtk_equalizer2_cfg_t;

int wtk_equalizer2_cfg_init(wtk_equalizer2_cfg_t *cfg);
int wtk_equalizer2_cfg_clean(wtk_equalizer2_cfg_t *cfg);
int wtk_equalizer2_cfg_update_local(wtk_equalizer2_cfg_t *cfg,
                                    wtk_local_cfg_t *lc);
int wtk_equalizer2_cfg_update(wtk_equalizer2_cfg_t *cfg);
int wtk_equalizer2_cfg_update2(wtk_equalizer2_cfg_t *cfg,
                               wtk_source_loader_t *sl);

wtk_equalizer2_cfg_t *wtk_equalizer2_cfg_new(char *fn);
void wtk_equalizer2_cfg_delete(wtk_equalizer2_cfg_t *cfg);
wtk_equalizer2_cfg_t *wtk_equalizer2_cfg_new_bin(char *fn);
void wtk_equalizer2_cfg_delete_bin(wtk_equalizer2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif