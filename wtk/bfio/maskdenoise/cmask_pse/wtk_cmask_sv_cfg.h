#ifndef WTK_BFIO_MASKDENOISE_CMASK_SV_WTK_CMASK_SV_CFG_H
#define WTK_BFIO_MASKDENOISE_CMASK_SV_WTK_CMASK_SV_CFG_H
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_fbank.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_cmask_sv_cfg wtk_cmask_sv_cfg_t;

struct wtk_cmask_sv_cfg {

#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t emb;
#endif
    wtk_fbank_cfg_t fbank;
    unsigned use_onnx:1;
    unsigned use_cmvn:1;
};

int wtk_cmask_sv_cfg_init(wtk_cmask_sv_cfg_t *cfg);
int wtk_cmask_sv_cfg_clean(wtk_cmask_sv_cfg_t *cfg);
int wtk_cmask_sv_cfg_update(wtk_cmask_sv_cfg_t *cfg);
int wtk_cmask_sv_cfg_update2(wtk_cmask_sv_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_cmask_sv_cfg_update_local(wtk_cmask_sv_cfg_t *cfg, wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif
