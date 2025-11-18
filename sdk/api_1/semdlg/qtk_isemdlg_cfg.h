#ifndef QTK_API_1_SEMDLG_QTK_ISEMDLG_CFG
#define QTK_API_1_SEMDLG_QTK_ISEMDLG_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/semdlg/wtk_semdlg_cfg.h"

#include "sdk/spx/qtk_spx_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_isemdlg_cfg qtk_isemdlg_cfg_t;
struct qtk_isemdlg_cfg {
    wtk_queue_node_t q_n;
    qtk_spx_cfg_t spx;
    wtk_semdlg_cfg_t *semdlg;
    wtk_local_cfg_t *attach;
    wtk_string_t semdlg_fn;
    wtk_string_t name;
    unsigned use_bin : 1;
    unsigned use_spx : 1;
    unsigned lc_custom : 1;
};

int qtk_isemdlg_cfg_init(qtk_isemdlg_cfg_t *cfg);
int qtk_isemdlg_cfg_clean(qtk_isemdlg_cfg_t *cfg);
int qtk_isemdlg_cfg_update_local(qtk_isemdlg_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_isemdlg_cfg_update(qtk_isemdlg_cfg_t *cfg);
int qtk_isemdlg_cfg_update2(qtk_isemdlg_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_isemdlg_cfg_update_option(qtk_isemdlg_cfg_t *cfg, qtk_option_t *opt);
void qtk_isemdlg_cfg_update_params(qtk_isemdlg_cfg_t *cfg,
                                   wtk_local_cfg_t *params);

#ifdef __cplusplus
};
#endif
#endif
