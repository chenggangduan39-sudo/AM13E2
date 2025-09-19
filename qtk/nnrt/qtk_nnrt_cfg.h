#ifndef E9E3F857_2EFA_487D_AB40_270D225AB18A
#define E9E3F857_2EFA_487D_AB40_270D225AB18A
#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_nnrt_cfg qtk_nnrt_cfg_t;

struct qtk_nnrt_cfg {
#ifdef QTK_NNRT_NCNN
    char *ncnn_model_fn;
    char *ncnn_param_fn;
    wtk_local_cfg_t *metadata;
    wtk_string_t *param_data;
    unsigned ncnn_use_winograd_conv : 1;
#endif
#ifdef QTK_NNRT_ONNXRUNTIME
    char *onnx_fn;
#endif
#ifdef QTK_NNRT_SS_IPU
    char *mdl_fn;
#endif
    char *model;
    int num_threads;
    wtk_string_t *bin_data;
    unsigned use_ncnn : 1;
    unsigned use_onnxruntime : 1;
    unsigned use_ss_ipu : 1;
    unsigned use_rknpu : 1;
    unsigned use_libtorch : 1;
    unsigned enable_profiling : 1;
};

int qtk_nnrt_cfg_init(qtk_nnrt_cfg_t *cfg);
int qtk_nnrt_cfg_clean(qtk_nnrt_cfg_t *cfg);
int qtk_nnrt_cfg_update(qtk_nnrt_cfg_t *cfg);
int qtk_nnrt_cfg_update_local(qtk_nnrt_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_nnrt_cfg_update2(qtk_nnrt_cfg_t *cfg, wtk_source_loader_t *sl);

#endif /* E9E3F857_2EFA_487D_AB40_270D225AB18A */
