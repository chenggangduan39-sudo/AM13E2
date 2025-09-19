#ifndef D058430D_AE36_4156_8B2E_007797B90616
#define D058430D_AE36_4156_8B2E_007797B90616

#include "qtk/cv/tracking/qtk_mot_sort.h"
#include "qtk/mdl/qtk_sonicnet_cfg.h"
#include "qtk/ult/fmcw/qtk_ult_fmcw_cfg.h"
#include "qtk/ult/qtk_ult_msc2d_cfg.h"
#include "qtk/ult/qtk_ult_ofdm_cfg.h"
#include "qtk/ult/qtk_ult_perception_cfg.h"
#include "qtk/ult/qtk_ultm2_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_ult_track_cfg qtk_ult_track_cfg_t;

struct qtk_ult_track_cfg {
    qtk_ult_ofdm_cfg_t ofdm;
    qtk_ult_fmcw_cfg_t fmcw;
    qtk_ultm2_cfg_t ultm2;
    qtk_ult_perception_cfg_t perception;
    qtk_sonicnet_cfg_t sonicnet;
    qtk_mot_sort_cfg_t sort;
    qtk_ult_msc2d_cfg_t msc2d;
    qtk_ult_msc2d_cfg_t y_msc2d;
    int channel;
    int upsamp;
    int left_ctx;
    int right_ctx;
    float range_prob_alpha;
    float aptd_dynamic_alpha;
    float nn_prob;
    float nn_vad_thresh;
    int max_tgt;
    int max_tracklet;
    float gamma;
    int range_search_width;
    int angle_search_width;
    float act_cir_amp_e;
    float hist_alpha;
    float warmup_cir_thresh;
    int height_search_width;

    unsigned use_latest_cir : 1;
    unsigned inactive_halt : 1;
    unsigned startup_warmup : 1;
    unsigned with_height : 1;
    unsigned use_fmcw : 1;
    unsigned use_perception : 1;
};

int qtk_ult_track_cfg_init(qtk_ult_track_cfg_t *cfg);
int qtk_ult_track_cfg_clean(qtk_ult_track_cfg_t *cfg);
int qtk_ult_track_cfg_update(qtk_ult_track_cfg_t *cfg);
int qtk_ult_track_cfg_update2(qtk_ult_track_cfg_t *cfg, wtk_source_loader_t *loader);
int qtk_ult_track_cfg_update_local(qtk_ult_track_cfg_t *cfg,
                                   wtk_local_cfg_t *lc);

#endif /* D058430D_AE36_4156_8B2E_007797B90616 */
