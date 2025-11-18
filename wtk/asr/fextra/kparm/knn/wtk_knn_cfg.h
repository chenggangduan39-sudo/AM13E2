#ifndef WTK_KSR_NNET_WTK_KNN_CFG
#define WTK_KSR_NNET_WTK_KNN_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/wtk_larray.h"
#include "wtk/core/math/wtk_math.h"
#include <ctype.h>
#include "wtk/core/wtk_fixpoint.h"
#include "wtk/asr/fextra/kparm/wtk_fixexp.h"
#include "wtk/asr/fextra/kparm/wtk_fixlog.h"
#include "wtk/core/math/wtk_mat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_knn_cfg wtk_knn_cfg_t;
#define unuse(var) (void)(var)
#define wtk_log(v) log(v)
#define wtk_exp(v) exp(v)
#define wtk_pow(a,b) pow(a,b)
#define wtk_sqrt(a) sqrt(a)
#define wtk_cos(a) cos(a)
#define wtk_sin(a) sin(a)
#define wtk_atan(a) atan(a)
#define wtk_floor(a) floor(a)
#define wtk_fabs(a) fabs(a)
#define wtk_abs(a)  abs(a)
#define wtk_log2(a) log2(a)
typedef enum
{
	WTK_LinearMaskComponent,
	WTK_LinearComponent,
	WTK_AffineComponent,
	WTK_FixedAffineComponent,
	WTK_NaturalGradientAffineComponent,
	WTK_NaturalGradientAffineMaskComponent,
	WTK_RectifiedLinearComponent,
	WTK_BatchNormComponent,
	WTK_NormalizeComponent,
	WTK_LogSoftmaxComponent,
	WTK_SigmoidComponent,
	WTK_ConvolutionComponent,
	WTK_NoOpComponent,
	WTK_GeneralDropoutComponent,
    	WTK_CompactFsmnComponent,
	WTK_LstmNonlinearityComponent,
	WTK_BackpropTruncationComponent,
	WTK_ScaleAndOffsetComponent,
	WTK_StatisticsExtractionComponent,
	WTK_StatisticsPoolingComponent,
}wtk_knn_layer_type_t;

typedef struct
{
	wtk_fixmats_t *a;
	wtk_fixvecs_t *b;
}wtk_knn_ab_short_t;

typedef struct
{
	wtk_fixmatc_t *a;
	wtk_fixvecc_t *b;
}wtk_knn_ab_char_t;

typedef struct
{
	wtk_matf_t *a;
	wtk_vecf_t *b;
	wtk_knn_ab_short_t *fix_short;
	wtk_knn_ab_char_t *fix_char;
}wtk_knn_ab_t;

typedef struct
{
	wtk_matf_t *a;
}wtk_knn_linear_mask_t;

typedef struct
{
    wtk_matf_t *a;
    wtk_fixmatc_t *fixc_a;
	wtk_fixmats_t *fixs_a;
}wtk_knn_linear_t;


typedef struct
{
	//x*a+b;
	wtk_knn_ab_t *ab;
}wtk_knn_fixed_affine_t;

typedef struct
{
	//x*a+b;
	wtk_knn_ab_t *ab;
}wtk_knn_affine_t;

typedef struct
{
	float rms;
	float rms2;
	unsigned add_log_stddev:1;
}wtk_knn_normalize_t;

typedef struct
{
	wtk_knn_ab_t *ab;
}wtk_knn_ng_affine_t;

typedef struct
{
	wtk_matf_t *a;
	wtk_vecf_t *b;
}wtk_knn_ng_mask_affine_t;

typedef struct
{
    int not_impl;
}wtk_rectified_linear_t;

typedef struct
{
    int not_impl;
}wtk_log_softmax_t;

typedef struct
{
    int not_impl;
}wtk_sigmoid_t;

typedef struct
{
	int input_period;
	int output_period;
	int include_var;
	int input_dim;
}wtk_knn_static_extraction_t;

typedef struct 
{
	int input_dim;
	int numlog_count_features;
	int output_stddevs;
	float variance_floor;
}wtk_knn_static_pooling_t;


typedef struct
{
	wtk_fixvecs_t *scale;
	wtk_fixvecs_t *offset;
}wtk_batch_norm_short_t;


typedef struct
{
	wtk_fixvecc_t *scale;
	wtk_fixvecc_t *offset;
}wtk_batch_norm_char_t;

typedef struct 
{
	wtk_knn_ab_t *ab; // linear_para & bias_para
	int input_x_dim;
	int input_y_dim;
	int input_z_dim;
	int filter_x_dim;
	int filter_y_dim;
	int filter_x_step;
	int filter_y_step;
	int num_y_step;
	wtk_veci_t* height_offset;
	int *column_map;
}wtk_knn_conv_t;

typedef struct 
{
	wtk_matf_t *params; 
}wtk_knn_lstm_t;

typedef struct
{
	int dim;
	float scale;
}wtk_knn_backproptrunc_t;

typedef struct 
{
	int dim;
	wtk_vecf_t* scale;
	wtk_vecf_t *offset;
}wtk_knn_scale_offset_t;

typedef struct
{
	int dim;
	wtk_vecf_t *scale;
	wtk_vecf_t *offset;
	wtk_batch_norm_short_t *fix_short;
	wtk_batch_norm_char_t *fix_char;
}wtk_batch_norm_t;

typedef struct 
{
	int dim;
	int block_dim;
	float time_period;
	float dropout_proportion;
}wtk_knn_general_dropout_t;

typedef struct 
{
	int dim;
	float backprop_scale;
}wtk_knn_noop_t;

typedef struct {
    int lstride;
    int rstride;
    int lorder;
    int rorder;
	wtk_matf_t *params;
        wtk_vecf_t *bias;
} wtk_knn_fsmn_t;

typedef struct
{
	wtk_string_t *name;
	wtk_knn_layer_type_t type;
	union
	{
		wtk_knn_linear_mask_t *linear_mask;
		wtk_knn_linear_t *linear;
		wtk_knn_affine_t *affine;
		wtk_knn_fixed_affine_t *fixed_affine;
		wtk_knn_ng_affine_t *ng_affine;
		wtk_knn_ng_mask_affine_t *ng_mask_affine;
		wtk_rectified_linear_t *rectified_linear;
		wtk_batch_norm_t *batch_norm;
		wtk_log_softmax_t *log_softmax;
		wtk_knn_normalize_t *normalize;
		wtk_sigmoid_t *sigmoid;
		wtk_knn_conv_t *conv;
		wtk_knn_lstm_t* lstm;
		wtk_knn_scale_offset_t* so;
		wtk_knn_backproptrunc_t* bp_trunc;
		wtk_knn_noop_t* noop;
		wtk_knn_general_dropout_t* gdrop_out;
		wtk_knn_static_extraction_t *static_extraction;
		wtk_knn_static_pooling_t *static_pooling;
	        wtk_knn_fsmn_t *fsmn;
	}v;
}wtk_knn_layer_t;

typedef enum
{
	WTK_KNN_APPEND,
	WTK_KNN_OFFSET,
	WTK_KNN_IFDEFINED,
	WTK_KNN_SUM,
	WTK_KNN_SCALE,
	WTK_KNN_ROUND,
	WTK_KNN_REPLACE,
}wtk_knn_cmd_type_t;

typedef struct wtk_knn_cmd wtk_knn_cmd_t;
typedef struct wtk_knn_cmd_v wtk_knn_cmd_v_t;

typedef enum
{
	WTK_KNN_CMD_V_I,
	WTK_KNN_CMD_V_STRING,
	WTK_KNN_CMD_V_CMD,
	WTK_KNN_CMD_V_F,
}wtk_knn_cmd_vt_t;

struct wtk_knn_cmd
{
	wtk_knn_cmd_type_t type;
	short nv;
	wtk_knn_cmd_v_t **v;
};

struct wtk_knn_cmd_v
{
	wtk_knn_cmd_vt_t type;
	union{
		int i;
		float f;
		wtk_string_t *str;
		wtk_knn_cmd_t *cmd;
	}v;
};

typedef struct
{
	short layer;
	short offset;
	float scale;
	unsigned sum;
	unsigned replace;
}wtk_knn_input_t;


typedef struct
{
	int nbit;
	char *bit;
	unsigned full:1;
}wtk_knn_bitmap_t;

typedef struct
{
	// wtk_string_t *name;
	wtk_string_t *input;
	int dim_offset;
	int dim;
}wtk_knn_range_layer_t;

typedef struct
{
	wtk_string_t *name;
	wtk_string_t *component;
	//wtk_string_t *objective;
	wtk_knn_layer_t *layer;
	wtk_knn_input_t *layer_input;
	//char *layer_input;
	wtk_knn_bitmap_t *bitmap;
	wtk_knn_cmd_v_t input;
	wtk_knn_range_layer_t* range_layer;
	short shift;
	unsigned char n_input;
	unsigned char tdnn_max_left;
	unsigned char tdnn_max_right;
	unsigned use_tdnn_output:1;
	unsigned use_tdnn_input:1;
	unsigned skip:1;
}wtk_knn_logic_layer_t;

typedef struct
{
	short id;
	short knn_id;
}wtk_knn_map_t;



struct wtk_knn_cfg
{
	wtk_local_cfg_t *lc_prior;
	wtk_knn_logic_layer_t **logic_layer;
	wtk_knn_layer_t **layer;
	int nlogic_layer;
	int rt_nlogic_layer;
	int nlayer;
	int skip;
	int offset_compress;
	int input_dim;
	int ivector_dim;
	int output_dim;
	char *skip_layer;
	char *fn;
	char *bin_fn;
    char *xvec_fn;
    wtk_str_hash_t *xvec_map;
	wtk_fixlog_t *softmax_fixl;
	wtk_fixexp_t *softmax_fixe;
	wtk_fixexp_t *fixe;

	short *layer_shift;
	wtk_vecf_t *prior;
	wtk_knn_map_t *map;
	int nmap;
	wtk_knn_bitmap_t *input_map;
	int max_shift;
    int left_context;
    int right_context;
	char *output_name;
	unsigned char tdnn_max_left;
	unsigned char tdnn_max_right;
	unsigned use_tdnn:1;
	unsigned use_bin:1;
	unsigned use_prior:1;
	unsigned use_exp:1;
	unsigned use_fast_skip:1;
	unsigned use_fast_exp:1;
	unsigned use_fixpoint:1;
	unsigned use_fixchar:1;
	unsigned use_fixchar0_short:1;
	unsigned use_fixshort:1;
	unsigned calc_shift:1;
	unsigned debug:1;
	unsigned update_offset:1;
	unsigned use_add_log:1;
	unsigned use_round;
};


char* wtk_knn_layer_type_str(wtk_knn_layer_type_t type);
//void wtk_knn_ng_affine_calc(wtk_knn_ng_affine_t *layer,wtk_vecf_t *input,wtk_vecf_t *output);
//void wtk_knn_affine_calc(wtk_knn_affine_t *layer,wtk_vecf_t *input,wtk_vecf_t *output);
float wtk_knn_affine_calc2(wtk_knn_affine_t *layer,wtk_vecf_t *input,int id);
//void wtk_knn_fixed_affine_calc(wtk_knn_fixed_affine_t *layer,wtk_vecf_t *input,wtk_vecf_t *output);
float wtk_knn_fixed_affine_calc2(wtk_knn_fixed_affine_t *layer,wtk_vecf_t *input,int id);
void wtk_rectified_linear_calc(wtk_rectified_linear_t *layer,wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_rectified_linear_calc_fix(wtk_rectified_linear_t *layer,wtk_veci_t *input,wtk_veci_t *output);
void wtk_rectified_linear_calc_fix_char(wtk_rectified_linear_t *layer,wtk_vecs2_t *input,wtk_vecs2_t *output);
void wtk_batch_norm_calc(wtk_batch_norm_t *layer,wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_batch_norm_calc_fix(wtk_batch_norm_t *layer,wtk_veci_t *input,wtk_veci_t *output);
void wtk_batch_norm_calc_fix_char(wtk_batch_norm_t *layer,wtk_vecs2_t *input,wtk_vecs2_t *output);
void wtk_log_softmax_calc(wtk_log_softmax_t *layer,wtk_vecf_t *input);
void wtk_normalize_calc(wtk_knn_normalize_t *normal,wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_knn_sigmoid(wtk_vecf_t *m,wtk_vecf_t *o);

void wtk_knn_cmd_print(wtk_knn_cmd_t *cmd);
void wtk_knn_cmd_v_print(wtk_knn_cmd_v_t *v);

int wtk_knn_layer_get_output(wtk_knn_layer_t *layer);

int wtk_knn_cfg_bytes(wtk_knn_cfg_t *cfg);
int wtk_knn_cfg_init(wtk_knn_cfg_t *cfg);
int wtk_knn_cfg_clean(wtk_knn_cfg_t *cfg);
int wtk_knn_cfg_update_local(wtk_knn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_knn_cfg_update(wtk_knn_cfg_t *cfg);
int wtk_knn_cfg_update2(wtk_knn_cfg_t *cfg,wtk_source_loader_t *sl);
void wtk_knn_cmd_print(wtk_knn_cmd_t *cmd);

void wtk_knn_cfg_write_bin(wtk_knn_cfg_t *cfg,char *fn);

int wtk_knn_cfg_get_knn_id(wtk_knn_cfg_t *cfg,int id);

void wtk_knn_cfg_print_logic(wtk_knn_cfg_t *cfg);

int wtk_knn_bitmap_check_index(wtk_knn_bitmap_t *map,int index,int skip);

void wtk_knn_ab_calc_fix(wtk_knn_ab_t *ab,wtk_veci_t *input,wtk_veci_t *output);
void wtk_knn_ab_calc(wtk_knn_ab_t *layer,wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_fsmn_calc(wtk_knn_fsmn_t *fsmn, wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_log_softmax_calc_fix(wtk_veci_t *input);
void wtk_log_softmax_calc_fix_short(wtk_fixexp_t *fixe,wtk_fixlog_t *fixl,wtk_vecs2_t *input,wtk_vecs2_t *output);

void wtk_knn_ab_calc_fix_sc(wtk_knn_ab_t *ab,wtk_vecs2_t *input,wtk_vecs2_t *output);
void wtk_knn_ab_calc_fix_ic(wtk_knn_ab_t *ab,wtk_veci_t *input,wtk_vecs2_t *output);
void wtk_knn_ab_calc_fix_is(wtk_knn_ab_t *ab,wtk_veci_t *input,wtk_vecs2_t *output);
void wtk_knn_ab_calc_fix_ss(wtk_knn_ab_t *ab,wtk_vecs2_t *input,wtk_vecs2_t *output);
void wtk_knn_linear_calc_fix_is(wtk_knn_linear_t *l, wtk_veci_t *input, wtk_vecs2_t *output);
void wtk_knn_linear_calc_fix_ic(wtk_knn_linear_t *l, wtk_veci_t *input, wtk_vecs2_t *output);
void wtk_knn_linear_calc_fix_sc(wtk_knn_linear_t *l, wtk_vecs2_t *input, wtk_vecs2_t *output);
void wtk_knn_linear_calc_fix_ss(wtk_knn_linear_t *l, wtk_vecs2_t *input, wtk_vecs2_t *output);

void wtk_batch_norm_calc_fix_short(wtk_batch_norm_t *layer,wtk_vecs2_t *input,wtk_vecs2_t *output);

void wtk_knn_linear_mask_calc(wtk_knn_linear_mask_t *m,wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_knn_linear_calc(wtk_knn_linear_t *m,wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_knn_ng_mask_affine_calc(wtk_knn_ng_mask_affine_t *layer,wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_knn_conv_calc(wtk_knn_conv_t *conv, wtk_vecf_t *input,wtk_vecf_t *output);
void wtk_knn_conv_calc_fix_sc(wtk_knn_conv_t *conv, wtk_vecs2_t *input, wtk_vecs2_t *output);
void wtk_knn_conv_calc_fix_ic(wtk_knn_conv_t *conv, wtk_veci_t *input, wtk_vecs2_t *output);
void wtk_knn_conv_calc_fix_is(wtk_knn_conv_t *conv, wtk_veci_t *input, wtk_vecs2_t *output);
void wtk_knn_conv_calc_fix_ss(wtk_knn_conv_t *conv, wtk_vecs2_t *input, wtk_vecs2_t *output);
wtk_fixmatc_t* wtk_fixmatc_read_fix(wtk_source_t *src);

/*
 * 4096 - 15000
 * like:
 *    f=log(v/(1<<12));
 *    v=f<<12;
 */
int wtk_knn_fix_log(int v);

/**
 * 0 - -20000
 * like:
 *    f=exp(v/(1<<12))
 *    v=f<<12;
 */
int wtk_knn_fix_exp(int v);
wtk_vecf_t* wtk_knn_read_vector(wtk_source_t *src,wtk_strbuf_t *buf);
wtk_matf_t*  wtk_knn_read_matrix(wtk_source_t *src,wtk_strbuf_t *buf);

#ifdef USE_NNET3_COMPILER
wtk_knn_logic_layer_t* qtk_knn_logic_layer_new(char *name, char *component, char *input);
wtk_knn_layer_t *qtk_knn_layer_new(char *name, int type, void *arg1, void *arg2);
wtk_knn_layer_t *qtk_knn_layer_new_fix_char(char *name, int type, void *arg1, void *arg2);
int wtk_knn_cfg_update_dep(wtk_knn_cfg_t *cfg);
#endif
void wtk_knn_cfg_update_skip(wtk_knn_cfg_t *cfg);
wtk_matf_t*  wtk_knn_read_matrix(wtk_source_t *src,wtk_strbuf_t *buf);
float* wtk_knn_get_xvector(wtk_knn_cfg_t *cfg,char* name);
#ifdef __cplusplus
};
#endif
#endif
