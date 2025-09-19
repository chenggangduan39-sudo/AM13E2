
#ifndef QTK_API_CSR_QTK_CSR_CFG
#define QTK_API_CSR_QTK_CSR_CFG

#include "wtk/asr/vad/wtk_vad_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#include "sdk/api_1/asr/qtk_asr_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_csr_cfg qtk_csr_cfg_t;
struct qtk_csr_cfg {
    qtk_asr_cfg_t asr;
    wtk_vad_cfg_t vad;
    wtk_vad_cfg_t *xvad;
    char *vad_fn;
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
};

int qtk_csr_cfg_init(qtk_csr_cfg_t *cfg);
int qtk_csr_cfg_clean(qtk_csr_cfg_t *cfg);
int qtk_csr_cfg_update_local(qtk_csr_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_csr_cfg_update(qtk_csr_cfg_t *cfg);
int qtk_csr_cfg_update2(qtk_csr_cfg_t *cfg, wtk_source_loader_t *sl);

void qtk_csr_cfg_update_params(qtk_csr_cfg_t *cfg, wtk_local_cfg_t *params);
void qtk_csr_cfg_update_option(qtk_csr_cfg_t *cfg, qtk_option_t *opt);

qtk_csr_cfg_t *qtk_csr_cfg_new(char *cfg_fn);
void qtk_csr_cfg_delete(qtk_csr_cfg_t *cfg);

qtk_csr_cfg_t *qtk_csr_cfg_new_bin(char *bin_fn);
void qtk_csr_cfg_delete_bin(qtk_csr_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
