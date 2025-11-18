#ifndef __QTK_API_QTK_QKWS_CFG__
#define __QTK_API_QTK_QKWS_CFG__

#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/asr/kws/qtk_kws_cfg.h"
#include "wtk/asr/kws/qtk_sond_cluster_cfg.h"
#include "wtk/bfio/maskdenoise/cmask_pse/wtk_cmask_pse_cfg.h"
#include "wtk/asr/vad/wtk_vad_cfg.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf_cfg.h"
#include "sdk/api_1/aec/qtk_aec_cfg.h"

#include "spx/qtk_spx_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_qkws_cfg qtk_qkws_cfg_t;
struct qtk_qkws_cfg {
    qtk_kws_cfg_t *kws_cfg;
    qtk_sond_cluster_cfg_t *sc_cfg;
    wtk_cmask_pse_cfg_t *cmask_pse_cfg;
    char *cfg_fn;
    wtk_vad_cfg_t vad;
    wtk_vad_cfg_t *xvad;
    char *vad_fn;
    qtk_vboxebf_cfg_t *vbox_cfg;
    char *vbox_fn;
    qtk_aec_cfg_t *aec_cfg;
    char *aec_fn;
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    float result_dur;
    int use_mode;
    int channel;
    unsigned use_bin:1;
    unsigned use_kws:1;
    unsigned use_sc:1;
    unsigned use_cmask_pse:1;
    unsigned use_vad:1;
    unsigned use_vboxebf:1;
    unsigned use_aec:1;
    unsigned use_add_zero:1;
};

int qtk_qkws_cfg_init(qtk_qkws_cfg_t *cfg);
int qtk_qkws_cfg_clean(qtk_qkws_cfg_t *cfg);
int qtk_qkws_cfg_update_local(qtk_qkws_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_qkws_cfg_update(qtk_qkws_cfg_t *cfg);
int qtk_qkws_cfg_update2(qtk_qkws_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_qkws_cfg_t *qtk_qkws_cfg_new(char *cfg_fn);
void qtk_qkws_cfg_delete(qtk_qkws_cfg_t *cfg);

qtk_qkws_cfg_t *qtk_qkws_cfg_new_bin(char *bin_fn);
void qtk_qkws_cfg_delete_bin(qtk_qkws_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
