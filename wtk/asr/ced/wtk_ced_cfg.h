#ifndef WTK_CED_CFG_H_
#define WTK_CED_CFG_H_
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime_cfg.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_ced_cfg wtk_ced_cfg_t;

struct wtk_ced_cfg {
    int hop_size;
    int chunk_size;
    int sample_rate;
    int byte_per_sample;
    int onnx_output_index;
    int classes_num;
    int start_nframe_threshold;
    int end_nframe_threshold;
    void *hook;
    float energy_threshold;
    unsigned use_energy : 1;
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t onnx;
#endif
};

int wtk_ced_cfg_init(wtk_ced_cfg_t *cfg);
int wtk_ced_cfg_clean(wtk_ced_cfg_t *cfg);
int wtk_ced_cfg_update(wtk_ced_cfg_t *cfg);
int wtk_ced_cfg_update2(wtk_ced_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_ced_cfg_update_local(wtk_ced_cfg_t *cfg, wtk_local_cfg_t *lc);
#ifdef __cplusplus
};
#endif
#endif
