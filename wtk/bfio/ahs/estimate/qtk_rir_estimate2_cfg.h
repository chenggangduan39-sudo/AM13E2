#ifndef QTK_RIR_ESTIMATE2_CFG
#define QTK_RIR_ESTIMATE2_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_rir_estimate2_cfg qtk_rir_estimate2_cfg_t;

struct qtk_rir_estimate2_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    float distance;
    float max_val;
    float rt60;
    float lookahead;
    float st;
    int rir_duration;

    int hop_size;

    int channel;
    int rate;
    float max_freq;
    float min_freq;
    int code_len;
    int win_len;

    float code_tms;
    float win_tms;
    float energy_shift;

    float rir_lookahead_thresh;
    float rir_lookahead_len;

    unsigned int use_hanning:1;
};

int qtk_rir_estimate2_cfg_init(qtk_rir_estimate2_cfg_t *cfg);
int qtk_rir_estimate2_cfg_clean(qtk_rir_estimate2_cfg_t *cfg);
int qtk_rir_estimate2_cfg_update_local(qtk_rir_estimate2_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_rir_estimate2_cfg_update(qtk_rir_estimate2_cfg_t *cfg);
int qtk_rir_estimate2_cfg_update2(qtk_rir_estimate2_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_rir_estimate2_cfg_t* qtk_rir_estimate2_cfg_new(char *fn);
void qtk_rir_estimate2_cfg_delete(qtk_rir_estimate2_cfg_t *cfg);
qtk_rir_estimate2_cfg_t* qtk_rir_estimate2_cfg_new_bin(char *fn);
void qtk_rir_estimate2_cfg_delete_bin(qtk_rir_estimate2_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif