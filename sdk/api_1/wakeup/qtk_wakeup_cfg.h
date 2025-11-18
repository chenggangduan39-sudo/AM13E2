
#ifndef QTK_API_1_WAKEUP_QTK_WAKEUP_CFG
#define QTK_API_1_WAKEUP_QTK_WAKEUP_CFG

#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/asr/img/qtk_img_cfg.h"
#include "wtk/bfio/wtk_kvadwake_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_wakeup_cfg qtk_wakeup_cfg_t;
struct qtk_wakeup_cfg {
	qtk_img_cfg_t *img;
    wtk_kvadwake_cfg_t *kvwake_cfg;
    char *cfg_fn;
    wtk_main_cfg_t *img_main_cfg;
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    unsigned use_bin : 1;
    unsigned use_kvadwake:1;
    unsigned use_img:1;
};

int qtk_wakeup_cfg_init(qtk_wakeup_cfg_t *cfg);
int qtk_wakeup_cfg_clean(qtk_wakeup_cfg_t *cfg);
int qtk_wakeup_cfg_update_local(qtk_wakeup_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_wakeup_cfg_update(qtk_wakeup_cfg_t *cfg);
int qtk_wakeup_cfg_update2(qtk_wakeup_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_wakeup_cfg_t *qtk_wakeup_cfg_new(char *cfg_fn);
void qtk_wakeup_cfg_delete(qtk_wakeup_cfg_t *cfg);

qtk_wakeup_cfg_t *qtk_wakeup_cfg_new_bin(char *bin_fn);
void qtk_wakeup_cfg_delete_bin(qtk_wakeup_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
