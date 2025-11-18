#ifndef WTK_VITE_REC_DNN_WTK_FNN_H_
#define WTK_VITE_REC_DNN_WTK_FNN_H_
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_heap.h"
#include "../wtk_feat.h"
#include "flat/wtk_flat.h"
#include "blas/wtk_blas.h"
#include "wtk_fnn_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fnn wtk_fnn_t;
struct wtk_fextra;

struct wtk_fnn {
    wtk_fnn_cfg_t *cfg;
    struct wtk_fextra *parm;
    //wtk_matrix_t *feat_matrix;            //used for flat;
    union {
    	wtk_qlas_t *qlas;
    	wtk_mlat_t *mlat;
#ifdef USE_BLAS
        wtk_blas_t *blas;
#endif
        wtk_flat_t *flat;
    } v;
    wtk_robin_t *robin;
    wtk_feat_t **features;
    float *padding;
    int sil_count;
    int speech_count;
    int tot_sil;
    int idx;
};

wtk_fnn_t *wtk_fnn_new(wtk_fnn_cfg_t *cfg, struct wtk_fextra *parm);
int wtk_fnn_bytes(wtk_fnn_t *d);
void wtk_fnn_delete(wtk_fnn_t *d);
void wtk_fnn_reset(wtk_fnn_t *d);
void wtk_fnn_feed(wtk_fnn_t *d, wtk_feat_t *f);
void wtk_fnn_flush(wtk_fnn_t *d);
void wtk_fnn_raise_feature(wtk_fnn_t *d, wtk_feat_t *f);
void wtk_fnn_flush_layer(wtk_fnn_t *d,int force);
void wtk_fnn_skip_feature(wtk_fnn_t *d,wtk_feat_t *f);
void wtk_fnn_wait_end(wtk_fnn_t *d);
#ifdef __cplusplus
};
#endif
#endif
