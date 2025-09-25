#ifndef QTK_API_1_SEMDLG_QTK_SEMDLG_CFG
#define QTK_API_1_SEMDLG_QTK_SEMDLG_CFG

#include "qtk_isemdlg_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_semdlg_cfg qtk_semdlg_cfg_t;

struct qtk_semdlg_cfg {
    wtk_queue_t semdlg_q;
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
};

int qtk_semdlg_cfg_init(qtk_semdlg_cfg_t *cfg);
int qtk_semdlg_cfg_clean(qtk_semdlg_cfg_t *cfg);
int qtk_semdlg_cfg_update_local(qtk_semdlg_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_semdlg_cfg_update(qtk_semdlg_cfg_t *cfg);
int qtk_semdlg_cfg_update2(qtk_semdlg_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_semdlg_cfg_update_params(qtk_semdlg_cfg_t *cfg,
                                  wtk_local_cfg_t *params);
void qtk_semdlg_cfg_update_option(qtk_semdlg_cfg_t *cfg, qtk_option_t *opt);

qtk_semdlg_cfg_t *qtk_semdlg_cfg_new(char *fn);
void qtk_semdlg_cfg_delete(qtk_semdlg_cfg_t *cfg);

qtk_semdlg_cfg_t *qtk_semdlg_cfg_new_bin(char *bin_fn);
void qtk_semdlg_cfg_delete_bin(qtk_semdlg_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
