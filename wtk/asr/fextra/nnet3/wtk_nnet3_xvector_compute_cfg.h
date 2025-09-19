#ifndef WTK_wtk_NNET3_XVECTOR_COMPUTE_CFG_H_
#define WTK_wtk_NNET3_XVECTOR_COMPUTE_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/asr/fextra/kparm/wtk_kxparm_cfg.h"
#ifndef HEXAGON
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_nnet3_xvector_compute_cfg wtk_nnet3_xvector_compute_cfg_t;

struct wtk_nnet3_xvector_compute_cfg
{
#ifdef ONNX_DEC
        qtk_onnxruntime_cfg_t onnx;
#endif
    wtk_kxparm_cfg_t kxparm;
    wtk_vecf_t *mean_vec;
    char *mean_vec_fn;
    wtk_matf_t *transform;
    char *transform_fn;
    unsigned use_onnx:1;
    unsigned use_parm : 1;
    unsigned use_ivector_mean:1;
    unsigned use_ivector_norm_len:1;
    unsigned use_normalize:1;
};

int wtk_nnet3_xvector_compute_cfg_init(wtk_nnet3_xvector_compute_cfg_t *cfg);
int wtk_nnet3_xvector_compute_cfg_clean(wtk_nnet3_xvector_compute_cfg_t *cfg);
int wtk_nnet3_xvector_compute_cfg_update_local(wtk_nnet3_xvector_compute_cfg_t *cfg, wtk_local_cfg_t *main);
int wtk_nnet3_xvector_compute_cfg_update(wtk_nnet3_xvector_compute_cfg_t *cfg);
int wtk_nnet3_xvector_compute_cfg_update2(wtk_nnet3_xvector_compute_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif
