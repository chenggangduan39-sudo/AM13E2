#ifndef QTK_API_TTSC_QTK_CLDTTS_CFG
#define QTK_API_TTSC_QTK_CLDTTS_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#include "sdk/spx/qtk_spx_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_cldtts_cfg qtk_cldtts_cfg_t;

struct qtk_cldtts_cfg {
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_spx_cfg_t spx;
    unsigned use_split : 1;
    unsigned use_mp3dec : 1;
};

int qtk_cldtts_cfg_init(qtk_cldtts_cfg_t *cfg);
int qtk_cldtts_cfg_clean(qtk_cldtts_cfg_t *cfg);
int qtk_cldtts_cfg_update_local(qtk_cldtts_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_cldtts_cfg_update(qtk_cldtts_cfg_t *cfg);
int qtk_cldtts_cfg_update2(qtk_cldtts_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_cldtts_cfg_update_option(qtk_cldtts_cfg_t *cfg, qtk_option_t *opt);
void qtk_cldtts_cfg_update_params(qtk_cldtts_cfg_t *cfg, wtk_local_cfg_t *lc);

qtk_cldtts_cfg_t *qtk_cldtts_cfg_new(char *fn);
void qtk_cldtts_cfg_delete(qtk_cldtts_cfg_t *cfg);

qtk_cldtts_cfg_t *qtk_cldtts_cfg_new_bin(char *bin_fn);
void qtk_cldtts_cfg_delete_bin(qtk_cldtts_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
