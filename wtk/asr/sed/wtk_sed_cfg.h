#ifndef WTK_SED_CFG_H_
#define WTK_SED_CFG_H_
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_sed_cfg wtk_sed_cfg_t;
typedef struct wtk_sed_event_params wtk_sed_event_params_t;

/*
 * the prediction process of state machine para
 */
struct wtk_sed_event_params {
    float high_threshold;
    float low_threshold;
    int high_n_smooth;
    int low_n_smooth;
    int n_salt;
};

struct wtk_sed_cfg {
    int sample_duration;
    int wav_step;
    int wav_cross;
    int sample_rate;
    int onnx_output_index;
    int byte_per_sample;
    int index_per_second;
    void *hook;
    float seconds_per_index;
    wtk_sed_event_params_t shout_params;
    wtk_sed_event_params_t cry_params;
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t onnx;
#endif
};

int wtk_sed_cfg_init(wtk_sed_cfg_t *cfg);
int wtk_sed_cfg_clean(wtk_sed_cfg_t *cfg);
int wtk_sed_cfg_update(wtk_sed_cfg_t *cfg);
int wtk_sed_cfg_update2(wtk_sed_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_sed_cfg_update_local(wtk_sed_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_sed_event_params_init(wtk_sed_event_params_t *ep);
int wtk_sed_event_params_update_local(wtk_sed_event_params_t *ep,
                                      wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif
