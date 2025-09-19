#ifndef WTK_VITE_REC_DNN_WTK_FNN_CFG_H_
#define WTK_VITE_REC_DNN_WTK_FNN_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "blas/wtk_blas_cfg.h"
#include "flat/wtk_flat_cfg.h"
#include "qlas/wtk_qlas.h"
#include "mlat/wtk_mlat.h"
#include "wtk_fnn_type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fnn_cfg wtk_fnn_cfg_t;


struct wtk_fnn_cfg {
    int win;
    //int in_cols;
    int out_cols;
    int min_flush_frame;
    //int cache_size;
    //-------------------- bias configure -------------
#ifdef USE_BLAS
    wtk_blas_cfg_t blas;
#endif
    wtk_qlas_cfg_t qlas;
    wtk_flat_cfg_t flat;
    wtk_mlat_cfg_t mlat;
    int skip_frame;
    int padding_frame;
    int padding_sil_reset_cnt;
    int padding_sil_set_cnt;
    float sil_thresh;
    float speech_thresh;
    unsigned use_delta:1;
    unsigned use_blas:1;
    unsigned use_qlas:1;
    unsigned use_mlat:1;
    unsigned use_linear_output:1;
    unsigned use_lazy_out:1;
    unsigned attach_htk_log:1;
	unsigned use_expand_vector:1;
	unsigned use_hack_output:1;
};


int wtk_fnn_cfg_init(wtk_fnn_cfg_t *cfg);
int wtk_fnn_cfg_clean(wtk_fnn_cfg_t *cfg);
int wtk_fnn_cfg_update_local(wtk_fnn_cfg_t *cfg, wtk_local_cfg_t *lc);
int wtk_fnn_cfg_update(wtk_fnn_cfg_t *cfg);
int wtk_fnn_cfg_update2(wtk_fnn_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_fnn_cfg_bytes(wtk_fnn_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
