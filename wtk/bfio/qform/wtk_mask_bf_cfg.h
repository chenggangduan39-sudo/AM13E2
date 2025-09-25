#ifndef WTK_BFIO_QFORM_WTK_MASK_BF_CFG_H
#define WTK_BFIO_QFORM_WTK_MASK_BF_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mask_bf_cfg wtk_mask_bf_cfg_t;

struct wtk_mask_bf_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int channel;
    int channelx2;

    int wins;
    int nbin;
    int rate;

    int clip_s;
    int clip_e;
    int low_bin;
    int high_bin;
    int en_low_bin;
    int en_high_bin;
    float post_clip;
    float post_clip_e;
    float init_post_clip;
    int init_cnt;

    float zeta;
    float s_power_thresh;
    float n_power_thresh;
    float s_frame_thresh;
    float s_bin_thresh;
    float n_bin_thresh;
    float En_thresh;
    float scov_alpha;
    float ncov_alpha;
    float s_frame_thresh_e;
    float s_bin_thresh_e;
    float n_bin_thresh_e;
    float En_thresh_e;
    float scov_alpha_e;
    float ncov_alpha_e;
    float snr_thresh;
    float gamma;
    float eye;

    float *snr_acc_tuple;
    float *scov_alpha_tuple;
    float *s_frame_thresh_tuple;
    float *s_bin_thresh_tuple;
    float *n_bin_thresh_tuple;
    float *En_thresh_tuple;
    int snr_acc_tuple_n;

    int rss_iter;
    int update_w_cnt;
    int *update_w_freq;

    unsigned use_ref_change : 1;
};

int wtk_mask_bf_cfg_init(wtk_mask_bf_cfg_t *cfg);
int wtk_mask_bf_cfg_clean(wtk_mask_bf_cfg_t *cfg);
int wtk_mask_bf_cfg_update(wtk_mask_bf_cfg_t *cfg);
int wtk_mask_bf_cfg_update2(wtk_mask_bf_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_mask_bf_cfg_update_local(wtk_mask_bf_cfg_t *cfg, wtk_local_cfg_t *lc);

wtk_mask_bf_cfg_t *wtk_mask_bf_cfg_new(char *fn);
void wtk_mask_bf_cfg_delete(wtk_mask_bf_cfg_t *cfg);
wtk_mask_bf_cfg_t *wtk_mask_bf_cfg_new_bin(char *fn);
void wtk_mask_bf_cfg_delete_bin(wtk_mask_bf_cfg_t *cfg);

void wtk_mask_bf_cfg_set_channel(wtk_mask_bf_cfg_t *cfg, int channel);
#ifdef __cplusplus
};
#endif
#endif
