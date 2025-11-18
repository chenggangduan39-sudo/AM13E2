#ifdef USE_BLAS
#ifndef WTK_VITE_PARM_POST_DNN_WTK_BLAS_CFG_H_
#define WTK_VITE_PARM_POST_DNN_WTK_BLAS_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "../wtk_fnn_type.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_blas_cfg wtk_blas_cfg_t;

/**
1. trans file:  x*window+bias*window
{{{
<bias> 880 880
v 880
...
<window> 880 880
v 880
...
}}}

2. net file:  wx+b
{{{
<biasedlinearity> 256 880
m 256 880
--
v 256
<sigmoid> 256 256
<biasedlinearity> 256 256
m 256 256
--
v 256
<sigmoid> 256 256
<biasedlinearity> 2 256
m 2 256
v 2
<softmax> 2 2
}}}

11*80=880
|1*880|
   |   (x+bias)*window
|1*880|*|1*880|+bias*window  //对应的行相乘
   |
|1*880|
   |  wx+b
|1*880|*|880*256|+|1*256|
   |
|1*256|
   | ..

 */

typedef struct
{
	unsigned int len;
	unsigned int bytes;
	//double *v;		//[0,...]
	float *v;
}wtk_blas_vector_t;

typedef struct
{
	unsigned int row;
	unsigned int col;
	//double *m;
	float *m;
}wtk_blas_matrix_t;

#define wtk_blas_matrix_index(mx,r,c) ((mx)->col*r+c)
#define wtk_blas_matrix_get(mx,r,c) ((mx)->m[(mx)->col*r+c])

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_fnn_post_type_t type;
	wtk_blas_matrix_t *w;   //wx+b
	//wtk_blas_matrix_t *b;
	wtk_blas_vector_t *b;
	wtk_blas_vector_t *rescale;
	int in_dim;
	int out_dim;
}wtk_blas_layer_t;

typedef struct
{
	wtk_blas_vector_t *b;	//bias (x+bias)*window =>归一化  = x*window+bias*window
	wtk_blas_vector_t *w; 	//window
}wtk_blas_trans_t;

struct wtk_blas_cfg
{
	char *net_fn;
	char *trans_fn;
    char *last_trans_fn;
	//-------------------data section -------
	int align;
	int cache_size;
	//------------------- in cols -------------
	int in_cols;
	int max_row;
	int max_col;
	//-------------------- in bytes -----------
	int in_col_bytes;
	//------------------- resource section ----
	wtk_blas_trans_t *trans;
	wtk_blas_trans_t *expand_trans;
    wtk_vector_t *last_trans;
	wtk_queue_t layer_q;		//wtk_blas_layer_t queue
	unsigned char is_bin:1;
};


int wtk_blas_cfg_init(wtk_blas_cfg_t *cfg);
int wtk_blas_cfg_clean(wtk_blas_cfg_t *cfg);
int wtk_blas_cfg_update_local(wtk_blas_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_blas_cfg_update2(wtk_blas_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_blas_cfg_update(wtk_blas_cfg_t *cfg);

int wtk_blas_cfg_out_cols(wtk_blas_cfg_t *cfg);
int wtk_blas_cfg_in_rows(wtk_blas_cfg_t *cfg);
int wtk_blas_cfg_in_cols(wtk_blas_cfg_t *cfg);
//--------------- math section -------------------
wtk_blas_vector_t* wtk_blas_vector_new(int align,int n);
void wtk_blas_vector_delete(wtk_blas_vector_t *v);
void wtk_blas_vector_print(wtk_blas_vector_t *v);
wtk_blas_trans_t*  wtk_blas_trans_new();
void wtk_blas_trans_delete(wtk_blas_trans_t *t);
wtk_blas_matrix_t* wtk_blas_matrix_new(int align,int row,int col);
void wtk_blas_matrix_delete(wtk_blas_matrix_t *m);
wtk_blas_layer_t* wtk_blas_layer_new();
void wtk_blas_layer_delete(wtk_blas_layer_t *l);
void wtk_blas_layer_print(wtk_blas_layer_t *l);
void wtk_blas_vector_mult(wtk_blas_vector_t* src,wtk_blas_vector_t *dst);
void wtk_blas_vector_mult2(float* src,int len,wtk_blas_vector_t *dst);
int wtk_blas_cfg_bytes(wtk_blas_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
#endif
