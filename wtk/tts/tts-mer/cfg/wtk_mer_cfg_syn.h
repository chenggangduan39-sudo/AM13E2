#ifndef WTK_MER_SYN_CFG_H_
#define WTK_MER_SYN_CFG_H_
#include "tts-mer/wtk_mer_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mer_syn_dnn_cfg
{
    wtk_heap_t *heap;
    wtk_array_t *w_arr;
    wtk_array_t *b_arr;
    wtk_vecf_t *norm;
    wtk_vecf_t *mvn;
    char **w_fn;
    char **b_fn;
    char *mvn_fn;
    char *norm_fn;
    int n_in;
} wtk_mer_cfg_syn_dnn_t;

typedef struct wtk_mer_syn_qes
{
    wtk_stridx_t *continous;
    wtk_larray_t *discrete_larr;
}wtk_mer_syn_qes_t;

typedef struct
{
    wtk_vecf_t *bap;
    wtk_vecf_t *lf0;
    wtk_vecf_t *mgc;
    wtk_vecf_t *vuv;
    char *fn_bap;
    char *fn_lf0;
    char *fn_mgc;
    char *fn_vuv;
} wtk_mer_covar_t;

typedef struct
{
    wtk_heap_t *heap;
	wtk_array_t *layer_type;
    wtk_array_t *layer_num;
    char *qes_fn;
    wtk_mer_covar_t covar;
    wtk_mer_cfg_syn_dnn_t dur; /* duration */
    wtk_mer_cfg_syn_dnn_t act; /* acoustic */
    wtk_mer_syn_qes_t qes; /* questions */
    int use_dur:1;
} wtk_mer_cfg_syn_t;

int wtk_mer_cfg_syn_init(wtk_mer_cfg_syn_t *cfg);
int wtk_mer_cfg_syn_clean(wtk_mer_cfg_syn_t *cfg);
int wtk_mer_cfg_syn_update_local(wtk_mer_cfg_syn_t *cfg, wtk_local_cfg_t *lc);
void wtk_mer_cfg_syn_update2(wtk_mer_cfg_syn_t *syn, wtk_source_loader_t *sl);

int wtk_mer_cfg_syn_dnn_init(wtk_mer_cfg_syn_dnn_t *dnn_cfg);
int wtk_mer_cfg_syn_dnn_clean(wtk_mer_cfg_syn_dnn_t *dnn_cfg);
int wtk_mer_cfg_syn_dnn_update_local(wtk_mer_cfg_syn_dnn_t *dnn_cfg, wtk_local_cfg_t *lc, int layer_size, int layer_num[]);
void wtk_mer_cfg_syn_dnn_update2(wtk_mer_cfg_syn_dnn_t *dnn_cfg, wtk_source_loader_t *sl);

#ifdef __cplusplus
}
#endif
#endif
