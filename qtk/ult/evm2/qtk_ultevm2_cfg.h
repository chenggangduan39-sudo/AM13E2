#ifndef EE68CBBA_4D67_5FF3_6F3F_92224D91EB9F
#define EE68CBBA_4D67_5FF3_6F3F_92224D91EB9F
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_ultevm2_cfg qtk_ultevm2_cfg_t;

struct qtk_ultevm2_cfg {
    int fc_sig;
    float fc_search_width;
    int sample_rate;
    int ws1;
    int ws2;
    int winsize;
    int fftsize;

    float beta;
    float span;
    float passband;
    int median_filter_len;
    int Q;
    float slide_dur;
    int frame_size;
    int tri_filter_gap;
    int tri_dropbin;
    float feat_max_diff_thresh;
    float feat_max_alpha;
    float active_enter_trap_dur;
    float active_leave_trap_dur;
    int fc_est_len;

    float sps;
    int tri_numbin;
    int active_enter_trap;
    int active_leave_trap;
};

int qtk_ultevm2_cfg_init(qtk_ultevm2_cfg_t *cfg);
int qtk_ultevm2_cfg_clean(qtk_ultevm2_cfg_t *cfg);
int qtk_ultevm2_cfg_update(qtk_ultevm2_cfg_t *cfg);
int qtk_ultevm2_cfg_update2(qtk_ultevm2_cfg_t *cfg, wtk_source_loader_t *loader);
int qtk_ultevm2_cfg_update_local(qtk_ultevm2_cfg_t *cfg, wtk_local_cfg_t *lc);

#endif /* EE68CBBA_4D67_5FF3_6F3F_92224D91EB9F */
