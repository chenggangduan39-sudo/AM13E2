#ifndef __SDK_ENGINE_TINYBF_QTK_ETINY_BF_CFG_H__
#define __SDK_ENGINE_TINYBF_QTK_ETINY_BF_CFG_H__

#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "wtk/bfio/qform/wtk_qform9.h"
//#include "wtk/bfio/vbox/wtk_gainnet_bf_stereo_cfg.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf_stereo_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_etinybf_cfg qtk_etinybf_cfg_t;

struct qtk_etinybf_cfg {
    wtk_qform9_cfg_t qform9;
    //wtk_gainnet_bf_stereo_cfg_t vboxebf;
    wtk_gainnet_bf_stereo_cfg_t vboxebf;
    int input_channel;
    float theta;
    float phi;
    union {
        wtk_main_cfg_t *main_cfg;
        wtk_mbin_cfg_t *mbin_cfg;
    };
    unsigned use_qform9 : 1;
    unsigned use_vboxebf : 1;
};

int qtk_etinybf_cfg_init(qtk_etinybf_cfg_t *cfg);
int qtk_etinybf_cfg_clean(qtk_etinybf_cfg_t *cfg);
int qtk_etinybf_cfg_update_local(qtk_etinybf_cfg_t *cfg, wtk_local_cfg_t *main);
int qtk_etinybf_cfg_update(qtk_etinybf_cfg_t *cfg);
int qtk_etinybf_cfg_update2(qtk_etinybf_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_etinybf_cfg_t *qtk_etinybf_cfg_new(const char *cfn);
qtk_etinybf_cfg_t *qtk_etinybf_cfg_new_bin(const char *cfn);
void qtk_etinybf_cfg_delete(qtk_etinybf_cfg_t *cfg);
void qtk_etinybf_cfg_delete_bin(qtk_etinybf_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif