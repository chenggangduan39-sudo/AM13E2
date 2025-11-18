#ifndef QTK_RIR_ESTIMATE_CFG
#define QTK_RIR_ESTIMATE_CFG
#include "qtk_rir_estimate2_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_rir_estimate_cfg qtk_rir_estimate_cfg_t;

typedef enum {
    WTK_RIR_ESTIMATE,
    WTK_WHITE_NOISE_ESTIMATE,
    WTK_GCC_ESTIMATE,
    WTK_RIR_ESTIMATE2,
} qtk_estimate_type_t;

struct qtk_rir_estimate_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    float distance;
    float max_val;
    float rt60;
    float lookahead;
    float st;
    char *lsweep_fn;
    char *ref_fn;
    float *inv_log_sweep;
    int nsweep;
    float rir_duration;

    qtk_estimate_type_t est_type;
    char *kind;
    int hop_size;
    int rate;

    qtk_rir_estimate2_cfg_t sweep2;
};

int qtk_rir_estimate_cfg_init(qtk_rir_estimate_cfg_t *cfg);
int qtk_rir_estimate_cfg_clean(qtk_rir_estimate_cfg_t *cfg);
int qtk_rir_estimate_cfg_update_local(qtk_rir_estimate_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_rir_estimate_cfg_update(qtk_rir_estimate_cfg_t *cfg);
int qtk_rir_estimate_cfg_update2(qtk_rir_estimate_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_rir_estimate_cfg_t* qtk_rir_estimate_cfg_new(char *fn);
void qtk_rir_estimate_cfg_delete(qtk_rir_estimate_cfg_t *cfg);
qtk_rir_estimate_cfg_t* qtk_rir_estimate_cfg_new_bin(char *fn);
void qtk_rir_estimate_cfg_delete_bin(qtk_rir_estimate_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
