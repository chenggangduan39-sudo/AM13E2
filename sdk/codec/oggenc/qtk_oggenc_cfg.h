#ifndef QTK_API_HTTPZC_QTK_OGGENC_CFG
#define QTK_API_HTTPZC_QTK_OGGENC_CFG
#include "ogg/ogg.h"
#include "speex/speex.h"
#include "speex/speex_header.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_oggenc_cfg qtk_oggenc_cfg_t;
struct qtk_oggenc_cfg {
    int spx_quality;
    int spx_complexity;
    int spx_vbr;
};

int qtk_oggenc_cfg_init(qtk_oggenc_cfg_t *cfg);
int qtk_oggenc_cfg_clean(qtk_oggenc_cfg_t *cfg);
int qtk_oggenc_cfg_update_local(qtk_oggenc_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_oggenc_cfg_update(qtk_oggenc_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
