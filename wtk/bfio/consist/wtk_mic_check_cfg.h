#ifndef WTK_BFIO_CONSIST_WTK_MIC_CHECK_CFG_H
#define WTK_BFIO_CONSIST_WTK_MIC_CHECK_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mic_check_cfg wtk_mic_check_cfg_t;

typedef enum {
    WTK_MIC_CHECK_TYPE_NONE = 0,
    WTK_MIC_CHECK_RCD_CHECK,
    WTK_MIC_CHECK_PLAY_CHECK,
} wtk_mic_check_type_t;

struct wtk_mic_check_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int wins;
    int rate;
    float sv;

    int channel;
    int *mic_channel;
    int nmicchannel;
    int *sp_channel;
    int nspchannel;

    float mic_scale;
    float sp_scale;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    int clip_s;
    int clip_e;

    float alpha;
    float eng_thresh;
    float var_thresh;
    float sc_thresh;

    float eng_thresh2;
    float var_thresh2;
    float sc_thresh2;

    float rcd_thresh;
    float play_thresh;
    float play_thresh2;

    wtk_mic_check_type_t type;

    unsigned use_pffft : 1;
    unsigned use_fft : 1;
};

int wtk_mic_check_cfg_init(wtk_mic_check_cfg_t *cfg);
int wtk_mic_check_cfg_clean(wtk_mic_check_cfg_t *cfg);
int wtk_mic_check_cfg_update(wtk_mic_check_cfg_t *cfg);
int wtk_mic_check_cfg_update2(wtk_mic_check_cfg_t *cfg,
                                wtk_source_loader_t *sl);
int wtk_mic_check_cfg_update_local(wtk_mic_check_cfg_t *cfg,
                                     wtk_local_cfg_t *lc);

wtk_mic_check_cfg_t *wtk_mic_check_cfg_new(char *fn);
void wtk_mic_check_cfg_delete(wtk_mic_check_cfg_t *cfg);
wtk_mic_check_cfg_t *wtk_mic_check_cfg_new_bin(char *fn);
void wtk_mic_check_cfg_delete_bin(wtk_mic_check_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
