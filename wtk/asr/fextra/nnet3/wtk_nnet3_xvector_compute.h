#ifndef WTK_wtk_NNET3_XVECTOR_COMPUTE_H_
#define WTK_wtk_NNET3_XVECTOR_COMPUTE_H_
#include "wtk_nnet3_xvector_compute_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/asr/fextra/torchnn/qtk_torchnn.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wtk_nnet3_xvector_compute_kxparm_notify_f)(void *ths,wtk_kfeat_t *feat);

typedef struct wtk_nnet3_xvector_compute wtk_nnet3_xvector_compute_t;
typedef struct wtk_svprint_pool wtk_svprint_pool_t;

struct wtk_svprint_pool
{
	qtk_torchnn_linear_t *asp;
	qtk_torchnn_linear_t *fc;
	qtk_blas_matrix_t *attention;

	qtk_blas_matrix_t *h;
	qtk_blas_matrix_t *w;
	qtk_blas_matrix_t *mu;
	qtk_blas_matrix_t *rh;
	qtk_blas_matrix_t *output;
	qtk_blas_matrix_t *xvector;
};

struct wtk_nnet3_xvector_compute
{
    wtk_nnet3_xvector_compute_cfg_t *cfg;
    wtk_kxparm_t *kxparm;
    qtk_torchnn_t *nn;
    wtk_vecf_t *spk_mean;
    wtk_vecf_t *transform_out;
    wtk_svprint_pool_t *pool;
    wtk_strbuf_t *xvec_in_buf;
    qtk_blas_matrix_t *xvec_in;
    wtk_vecf_t *xvec;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx;
    wtk_strbuf_t *wav;
    wtk_strbuf_t *feat;
    int num_feats;
    int64_t shape[3];
    int64_t shape2;
#endif
    wtk_nnet3_xvector_compute_kxparm_notify_f kxparm_func;
    void *ths;
    int spk_cnt;
    int in_idx;
};

#define wtk_net3_xvector_compute_get_spk_cnt(x) ((x)->spk_cnt)
int xvector_pool_read(wtk_nnet3_xvector_compute_t *x,wtk_source_t *src);
wtk_svprint_pool_t* wtk_nnet3_xvector_pool_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
wtk_svprint_pool_t* wtk_nnet3_xvector_pool_read2(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void wtk_nnet3_xvector_pool_delete(wtk_svprint_pool_t *asp);
void wtk_nnet3_xvector_compute_pool(wtk_svprint_pool_t *asp,qtk_blas_matrix_t *in);
void wtk_nnet3_xvector_compute_pool2(wtk_svprint_pool_t *asp,qtk_blas_matrix_t *in);
void wtk_nnet3_xvector_compute_set_nn_notify(wtk_nnet3_xvector_compute_t *x);
wtk_nnet3_xvector_compute_t* wtk_nnet3_xvector_compute_new(wtk_nnet3_xvector_compute_cfg_t *cfg);
wtk_nnet3_xvector_compute_t* wtk_nnet3_xvector_compute_new2(wtk_nnet3_xvector_compute_cfg_t *cfg);
void wtk_nnet3_xvector_compute_delete(wtk_nnet3_xvector_compute_t *x);
int wtk_nnet3_xvector_compute_start(wtk_nnet3_xvector_compute_t *x);
void wtk_nnet3_xvector_compute_reset(wtk_nnet3_xvector_compute_t *x);
void wtk_nnet3_xvector_compute_reset2(wtk_nnet3_xvector_compute_t *x);
int wtk_nnet3_xvector_compute_feed(wtk_nnet3_xvector_compute_t *x, short *data, int len, int is_end);
wtk_vecf_t* wtk_nnet3_xvector_compute_normalize(wtk_nnet3_xvector_compute_t *x);
wtk_vecf_t *wtk_nnet3_xvector_compute(wtk_nnet3_xvector_compute_t *x);
void wtk_nnet3_xvector_compute_normalize_reset(wtk_nnet3_xvector_compute_t *x);
void wtk_nnet3_xvector_compute_set_kind_notify(wtk_nnet3_xvector_compute_t *x,void *ths,wtk_nnet3_xvector_compute_kxparm_notify_f func);
void wtk_nnet3_xvector_feed_feat(wtk_nnet3_xvector_compute_t *x,float *f,int num_frames, int frame_col);
#ifdef __cplusplus
};
#endif
#endif
