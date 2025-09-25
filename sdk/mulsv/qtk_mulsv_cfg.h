#ifndef __SDK_MULSV_QTK_MULSV_CFG_H__
#define __SDK_MULSV_QTK_MULSV_CFG_H__
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/bfio/aec/wtk_aec_cfg.h"
#include "wtk/bfio/wtk_bfio_cfg.h"
#include "wtk/asr/img/qtk_img_cfg.h"
#include "wtk/asr/kws/wtk_svprint_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif


typedef struct qtk_mulsv_cfg qtk_mulsv_cfg_t;

struct qtk_mulsv_cfg {
    wtk_bfio_cfg_t  bfio;
    qtk_img_cfg_t img;
    qtk_img_thresh_cfg_t img1st;
    wtk_aec_cfg_t aec;
    wtk_svprint_cfg_t svprint;
    float frame_samp;
    float wake1st_right_tm;
    float vprint_enroll_loff;
    float vprint_test_loff;
    int cache_tm;
    int wkd_tm;
    int wake1st_channel;
    int wake2nd_channel;
    int vprint_feed_len;
    void *hook;
    unsigned use_total_log : 1;
    unsigned use_aec:1;
    unsigned use_1st:1;
    unsigned use_padding:1;
    unsigned use_2nd_pad_sp:1;
    unsigned use_oppo_log : 1;
    unsigned use_ori_enroll : 1;
    unsigned use_1st_notify_wav : 1;
};


int qtk_mulsv_cfg_init(qtk_mulsv_cfg_t *cfg);
int qtk_mulsv_cfg_clean(qtk_mulsv_cfg_t *cfg);
int qtk_mulsv_cfg_update_local(qtk_mulsv_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_mulsv_cfg_update(qtk_mulsv_cfg_t *cfg);
int qtk_mulsv_cfg_update2(qtk_mulsv_cfg_t *cfg, wtk_source_loader_t *sl);
qtk_mulsv_cfg_t *qtk_mulsv_cfg_new(char *cfg_fn);
qtk_mulsv_cfg_t *qtk_mulsv_cfg_new_bin(char *bin_fn);
void qtk_mulsv_cfg_delete(qtk_mulsv_cfg_t *cfg);
void qtk_mulsv_cfg_delete_bin(qtk_mulsv_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
