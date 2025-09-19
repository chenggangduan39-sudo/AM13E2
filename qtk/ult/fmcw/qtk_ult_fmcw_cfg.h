#ifndef G_FA1A56827AAB4848A6459191441A5DA9
#define G_FA1A56827AAB4848A6459191441A5DA9
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_complex.h"

typedef struct qtk_ult_fmcw_cfg qtk_ult_fmcw_cfg_t;

struct qtk_ult_fmcw_cfg {
    int nsymbols;
    int sampling_rate;
    float central_freq;
    int bandwidth;
    int period;
    int winsize;
    int subsample_factor;
    int nsamples;
    float range_unit;
    float peak_thresh;
    int channel;
    int sync_pos_tolerance;

    float dis_start;
    float dis_end;

    wtk_complex_t *sweep;
    wtk_complex_t *filter_params;
    wtk_complex_t *carrier;
};

int qtk_ult_fmcw_cfg_init(qtk_ult_fmcw_cfg_t *cfg);
int qtk_ult_fmcw_cfg_clean(qtk_ult_fmcw_cfg_t *cfg);
int qtk_ult_fmcw_cfg_update(qtk_ult_fmcw_cfg_t *cfg);
int qtk_ult_fmcw_cfg_update_local(qtk_ult_fmcw_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_ult_fmcw_cfg_update2(qtk_ult_fmcw_cfg_t *cfg, wtk_source_loader_t *sl);
int qtk_ult_fmcw_cfg_get_signal(qtk_ult_fmcw_cfg_t *cfg, float fc, float scale,
                                float *wav);

#endif
