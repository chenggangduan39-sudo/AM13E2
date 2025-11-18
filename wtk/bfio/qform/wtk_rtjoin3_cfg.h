#ifndef WTK_BFIO_QFORM_WTK_RTJOIN3_CFG
#define WTK_BFIO_QFORM_WTK_RTJOIN3_CFG
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rtjoin3_cfg wtk_rtjoin3_cfg_t;
struct wtk_rtjoin3_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int nbin;
    int rate;

    int channel;
    int *mic_channel;
    int nmicchannel;
    int clip_s;
    int clip_e;

    float micenr_thresh;
    int micenr_cnt;

    wtk_qmmse_cfg_t qmmse;

    float max_out;
    int out_agc_level;

    wtk_equalizer_cfg_t eq;

    float limiter_thresh;

    unsigned use_eq : 1;
    unsigned use_control_bs : 1;
    unsigned use_mul_out : 1;
    unsigned use_bs_win : 1;
    unsigned use_qmmse : 1;
    unsigned use_limiter : 1;
};

int wtk_rtjoin3_cfg_init(wtk_rtjoin3_cfg_t *cfg);
int wtk_rtjoin3_cfg_clean(wtk_rtjoin3_cfg_t *cfg);
int wtk_rtjoin3_cfg_update_local(wtk_rtjoin3_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_rtjoin3_cfg_update(wtk_rtjoin3_cfg_t *cfg);
int wtk_rtjoin3_cfg_update2(wtk_rtjoin3_cfg_t *cfg, wtk_source_loader_t *sl);

wtk_rtjoin3_cfg_t *wtk_rtjoin3_cfg_new(char *fn);
void wtk_rtjoin3_cfg_delete(wtk_rtjoin3_cfg_t *cfg);
wtk_rtjoin3_cfg_t *wtk_rtjoin3_cfg_new_bin(char *fn);
void wtk_rtjoin3_cfg_delete_bin(wtk_rtjoin3_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
