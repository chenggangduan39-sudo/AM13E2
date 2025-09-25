#ifndef QTK_NNWRAP_H_
#define QTK_NNWRAP_H_
#include "qtk_nnwrap_cfg.h"
#include "wtk/asr/fextra/torchnn/qtk_torchnn.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_nnwrap qtk_nnwrap_t;

struct qtk_nnwrap
{
	qtk_nnwrap_cfg_t *cfg;
    wtk_kxparm_t *parm;
    wtk_kvad_t *vad;
	qtk_torchnn_t *encoder;
	qtk_torchnn_t *decoder_gender;
	qtk_torchnn_t *decoder_age;
	wtk_strbuf_t *in_buf;
    wtk_svprint_pool_t *pool;
    qtk_blas_matrix_t *xvec_in;
	wtk_strbuf_t *user;
    unsigned vad_state:1;
    int age;
    int gender;
};

qtk_nnwrap_t* qtk_nnwrap_new(qtk_nnwrap_cfg_t *cfg);
void qtk_nnwrap_delete(qtk_nnwrap_t *kws);
int qtk_nnwrap_start(qtk_nnwrap_t *kws);
int qtk_nnwrap_reset(qtk_nnwrap_t *kws);
int qtk_nnwrap_feed(qtk_nnwrap_t *kws,char *data,int bytes,int is_end);
void qtk_nnwrap_get_result(qtk_nnwrap_t *nw, int *age, int *gender);
#ifdef __cplusplus
};
#endif
#endif
