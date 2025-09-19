
#ifndef QTK_API_CSRSC_QTK_CSRSC_CFG
#define QTK_API_CSRSC_QTK_CSRSC_CFG

#include "wtk/asr/vad/wtk_vad_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/kws/qtk_sond_cluster_cfg.h"
#include "sdk/api_1/asr/qtk_asr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_csrsc_cfg qtk_csrsc_cfg_t;
struct qtk_csrsc_cfg {
    qtk_asr_cfg_t asr;
    wtk_vad_cfg_t vad;
    wtk_vad_cfg_t *xvad;
    qtk_sond_cluster_cfg_t *sc_cfg;
    char *vad_fn;
    char *sc_fn;
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    unsigned use_scbin:1;
};

int qtk_csrsc_cfg_init(qtk_csrsc_cfg_t *cfg);
int qtk_csrsc_cfg_clean(qtk_csrsc_cfg_t *cfg);
int qtk_csrsc_cfg_update_local(qtk_csrsc_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_csrsc_cfg_update(qtk_csrsc_cfg_t *cfg);
int qtk_csrsc_cfg_update2(qtk_csrsc_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_csrsc_cfg_update_params(qtk_csrsc_cfg_t *cfg, wtk_local_cfg_t *params);
void qtk_csrsc_cfg_update_option(qtk_csrsc_cfg_t *cfg, qtk_option_t *opt);

qtk_csrsc_cfg_t *qtk_csrsc_cfg_new(char *cfg_fn);
void qtk_csrsc_cfg_delete(qtk_csrsc_cfg_t *cfg);

qtk_csrsc_cfg_t *qtk_csrsc_cfg_new_bin(char *bin_fn);
void qtk_csrsc_cfg_delete_bin(qtk_csrsc_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
