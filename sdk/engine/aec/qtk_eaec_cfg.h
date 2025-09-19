#ifndef __SDK_ENGINE_AEC_QTK_EAEC_CFG_H__
#define __SDK_ENGINE_AEC_QTK_EAEC_CFG_H__

#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_eaec_cfg qtk_eaec_cfg_t;

struct qtk_eaec_cfg {
    wtk_aec_cfg_t aec;
    int input_channel;
    int sp_channel;
    union {
        wtk_main_cfg_t *main_cfg;
        wtk_mbin_cfg_t *mbin_cfg;
    };
};

int qtk_eaec_cfg_init(qtk_eaec_cfg_t *cfg);
int qtk_eaec_cfg_clean(qtk_eaec_cfg_t *cfg);
int qtk_eaec_cfg_update_local(qtk_eaec_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_eaec_cfg_update(qtk_eaec_cfg_t *cfg);
int qtk_eaec_cfg_update2(qtk_eaec_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_eaec_cfg_t *qtk_eaec_cfg_new(const char *cfn);
qtk_eaec_cfg_t *qtk_eaec_cfg_new_bin(const char *cfn);
void qtk_eaec_cfg_delete(qtk_eaec_cfg_t *cfg);
void qtk_eaec_cfg_delete_bin(qtk_eaec_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
