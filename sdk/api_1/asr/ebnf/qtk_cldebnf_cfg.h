#ifndef QTK_API_1_ASR_EBNF_QTK_CLDEBNF_CFG
#define QTK_API_1_ASR_EBNF_QTK_CLDEBNF_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#include "sdk/httpc/qtk_httpc_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_cldebnf_cfg qtk_cldebnf_cfg_t;
struct qtk_cldebnf_cfg {
    qtk_httpc_cfg_t httpc;
    wtk_string_t url_pre;
    int interval;
};

int qtk_cldebnf_cfg_init(qtk_cldebnf_cfg_t *cfg);
int qtk_cldebnf_cfg_clean(qtk_cldebnf_cfg_t *cfg);
int qtk_cldebnf_cfg_update_local(qtk_cldebnf_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_cldebnf_cfg_update(qtk_cldebnf_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
