#ifndef WTK_QTK_NNET3_COMPONENT_H_
#define WTK_QTK_NNET3_COMPONENT_H_
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "qtk_nnet3_compution.h"
#include "wtk/core/math/wtk_mat.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct nnet3_component nnet3_component_t;
typedef struct OnlineNaturalGradient OnlineNaturalGradient_t;
typedef struct qtk_affine_global_component qtk_affine_global_component_t;
typedef struct qtk_affine_componet qtk_affine_componet_t;
typedef struct qtk_natural_gradient_affine_component qtk_natural_gradient_affine_component_t;
typedef struct qtk_rectified_linear_component qtk_rectified_linear_component_t;
typedef struct qtk_normalize_component qtk_normalize_component_t;
typedef struct qtk_activate_component qtk_activate_component_t;
typedef struct qtk_natural_gradient_per_element_scale_component qtk_natural_gradient_per_element_scale_component_t;
typedef struct qtk_element_wise_product_component qtk_element_wise_product_component_t;
typedef struct qtk_dropout_component qtk_dropout_component_t;
typedef struct qtk_backprop_truncation_component qtk_backprop_truncation_component_t;
typedef struct qtk_batch_norm_component qtk_batch_norm_component_t;
typedef struct qtk_linear_component qtk_linear_component_t;
typedef struct qtk_lstm_nolinearity_component qtk_lstm_nolinearity_component_t;

typedef struct qtk_timeheight_convolution_model qtk_timeheight_convolution_model_t;
typedef struct qtk_timeheight_convolution_component qtk_timeheight_convolution_component_t;
typedef struct qtk_scale_offset_component qtk_scale_offset_component_t;
typedef struct qtk_max_pooling_component qtk_max_pooling_component_t;
typedef struct qtk_pnorm_component qtk_pnorm_component_t;
typedef struct qtk_compactfsmn_component qtk_compactfsmn_component_t;
typedef struct qtk_blockweight_component qtk_blockweight_component_t;
typedef struct qtk_general_dropout_component qtk_general_dropout_component_t;
typedef struct qtk_natrual_gradient_affine_mask_component qtk_natrual_gradient_affine_mask_component_t;
typedef struct qtk_linear_mask_component qtk_linear_mask_component_t;
typedef struct qtk_permute_component qtk_permute_component_t;
typedef enum {
    QTK_AffineGlobalComponent = 0,
    QTK_RectifiedLinearComponent,
    QTK_NormalizeComponent,
    QTK_ActivateComponent,
    QTK_ElementwiseProductComponent,
    QTK_NaturalGradientPerElementScaleComponent,
    QTK_DropoutComponent,
    QTK_BackpropTruncationComponent,
    QTK_NoopComponent,
    QTK_BatchNormComponent,
    QTK_LstmNonlinearityComponent,
    QTK_TimeHeightConvolutionComponent,
    QTK_ScaleAndOffsetComponent,
    QTK_MaxPoolingComponent,
    QTK_PnormComponent,
    QTK_CompactFsmnComponent,
    QTK_BlockWeightComponent,
    QTK_GeneralDropoutComponent,
    QTK_NaturalGradientAffineMaskComponent,
    QTK_LinearMaskComponent,
    QTK_PermuteComponent
} qtk_nnet3_component_type;

typedef enum
{
	QTK_AffineComponent,
	QTK_FixedAffineComponent,
	QTK_NaturalGradientAffineComponent,
	QTK_LinearComponent
}qtk_nnet3_affine_type;

typedef enum {
    QTK_LogSoftmaxComponent,
    QTK_SoftmaxComponent,
    QTK_SigmoidComponent,
    QTK_TanhComponent
} qtk_nnet3_activate_type;

struct nnet3_component
{
	void *component;
	qtk_nnet3_component_type type;
};

struct qtk_affine_global_component
{
	wtk_mats_t *ws;
	qtk_blas_matrix_t *w;
        qtk_blas_matrix_t *b;
//	qtk_affine_componet_t *a_com;		/*they are		
//	qtk_fixed_affine_componet_t *fa_com;     not need now;*/
        qtk_natural_gradient_affine_component_t	*nga_com;
	qtk_linear_component_t *linear_com;
	qtk_nnet3_affine_type type;
};

struct qtk_natrual_gradient_affine_mask_component
{
	qtk_blas_matrix_t *raw;
    qtk_blas_matrix_t *bias;
    qtk_blas_matrix_t *rawt;
    qtk_blas_matrix_t *mask;
    qtk_blas_matrix_t *maskt;
	int block_size;
	int block_col;//col/block_size
	int num_block;
	int row;
	float* f;
	int *block;
	int *block2;
	int *in;
	int *ncblock;    //number of blocks one column.
};

struct qtk_linear_mask_component
{
	qtk_blas_matrix_t *raw;
    qtk_blas_matrix_t *rawt;
    qtk_blas_matrix_t *mask;
    qtk_blas_matrix_t *maskt;
	int block_size;
	int block_col;//col/block_size
	int num_block;
	int row;
	float* f;
	int *block;
	int *block2;
	int *in;
};

struct  qtk_affine_componet
{
	float orthonormal_constraint_;
};

struct OnlineNaturalGradient
{
	int rank;
	int update_period;
	float num_samples_history;
	float num_minibatches_history;
	float alpha;
	float target_rms;
	float delta;
	int frozen;
	int t;
	int self_debug;
	wtk_matrix_t *w;
	float rho_t;
	wtk_matrix_t *d;
};

struct qtk_natural_gradient_affine_component
{
	float max_change;
	float l2_regularize;
	float learning_rate;
	float learning_rate_factor;
	OnlineNaturalGradient_t * in;
	OnlineNaturalGradient_t* out;
	float orthonormal_constraint;
};

struct qtk_rectified_linear_component
{
	int dim;
	int block_dim;
	wtk_matrix_t *value_sum;
	wtk_matrix_t *deriv_sum;
	wtk_matrix_t *oderiv_sum;
	double count;
	double num_dims_self_repaired_;
	double num_dims_processed_;
	float self_repair_lower_threshold_;
	float self_repair_upper_threshold_;
	float self_repair_scale_;
};

struct qtk_normalize_component
{
	  float kSquaredNormFloor;
	  int input_dim_;
	  int block_dim_;
	  float target_rms_;
	  int add_log_stddev_;
};

//struct qtk_log_softmax_component
struct qtk_activate_component   //contains softmax,sigmoid,tanh by now
{
	int dim;
	int block_dim;
	wtk_matrix_t *value_sum;
	wtk_matrix_t *deriv_sum;
	wtk_matrix_t *oderiv_sum;
	double count;
	double num_dims_self_repaired_;
	double num_dims_processed_;
	float self_repair_lower_threshold_;
	float self_repair_upper_threshold_;
	float self_repair_scale_;
	qtk_nnet3_activate_type type;
};

struct qtk_element_wise_product_component
{
	int input_dim;
	int output_dim;
};

struct qtk_natural_gradient_per_element_scale_component
{
	OnlineNaturalGradient_t * preconditioner;
	wtk_matrix_t *scale;
	wtk_mats_t *scale2;		
};

struct qtk_dropout_component
{
	int dim;
	float dropout_proportion;
	int dropout_per_frame;
};

struct qtk_backprop_truncation_component
{
	int dim;
	float scale;
	float clipping_threshold;
	float zeroing_threshold;
	int zeroing_interval;
	int recurrence_interval;
	double num_clipped;
	double num_zeroed;
	double count;
	double count_zeroing_boundaries;
};

struct qtk_batch_norm_component
{
	int dim;
	int block_dim;
	float epsilon;
	float target_rms;
	int test_mode;
	double count;
	qtk_blas_matrix_t *stats_sum;
	qtk_blas_matrix_t *stats_sumsq;
//  CuVector<double> stats_sum_;
//  CuVector<double> stats_sumsq_;	
	qtk_blas_matrix_t *offset;
	qtk_blas_matrix_t *scale;
	wtk_mats_t *scale2;
	wtk_mats_t *offset2;
	qtk_sub_matrix_t* input;
	qtk_sub_matrix_t* output;
	qtk_sub_matrix_t* in_reshape;
	qtk_sub_matrix_t* out_reshape;
};

struct qtk_linear_component
{
	OnlineNaturalGradient_t * in;
        OnlineNaturalGradient_t* out;
	float orthonormal_constraint;
	int use_natural_gradient;	
};

struct qtk_lstm_nolinearity_component
{
	qtk_blas_matrix_t* params;
	wtk_mats_t* params2;
};

struct qtk_timeheight_convolution_model
{
	int num_filters_in;
	int num_filters_out;
	int height_in;
	int height_out;
};

struct qtk_timeheight_convolution_component
{
	qtk_timeheight_convolution_model_t *model;
	qtk_blas_matrix_t* linear_params;
	qtk_blas_matrix_t* bias_params;
	wtk_mats_t* linear_params2;
	wtk_mats_t* bias_params2;
	qtk_sub_matrix_t *input;
	qtk_sub_matrix_t *output;
	qtk_sub_matrix_t *output_shape;
};

struct qtk_convolution_compution
{
	int num_filters_in;
	int num_filters_out;
	int height_in;
	int height_out;
	int num_t_in;
	int num_t_out;
	int num_images;
};

struct qtk_scale_offset_component
{
	int dim;
	qtk_blas_matrix_t* scales;
	wtk_mats_t* scales2;
	wtk_mats_t* offsets2;
	qtk_blas_matrix_t* nonzero_scales;
	qtk_blas_matrix_t* offsets;
	int use_natural_gradient;
};

struct qtk_max_pooling_component
{
	int input_x_dim;
	int input_y_dim;
	int input_z_dim;
	int pool_x_size;
	int pool_y_size;
	int pool_z_size;
	int pool_x_step;
	int pool_y_step;
	int pool_z_step;
	int pool_size;
	int output_dim;
};

struct qtk_pnorm_component
{
	int input_dim;
	int output_dim;
};

struct qtk_compactfsmn_component
{
	int l_order;
	int r_order;
	int l_stride;
	int r_stride;
	qtk_blas_matrix_t* params;
        qtk_blas_matrix_t *BiasParams;
        int *time_offset;
	qtk_sub_matrix_t* in;
	qtk_sub_matrix_t* out;
	qtk_sub_matrix_t* in_part;
	qtk_sub_matrix_t* out_part;
};

struct qtk_blockweight_component {
    int BlockDim;
    int NumBlocks;
};

struct qtk_general_dropout_component
{
	int dim;
	int block_dim;
	int time_period;
	float dropout_proportion;
	int continuous;
	int test_mode;
};

struct qtk_permute_component
{
	int* column_map;
	int dim;
};

int qtk_general_dropout_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
int qtk_affine_global_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
int qtk_affine_global_component_read2(nnet3_component_t* com, wtk_source_t *src,wtk_strbuf_t *buf,float max_w, unsigned int is_fixed, int fixed_nbytes, int use_custom_acc, int porder);
int qtk_fixed_affine_componet_read(qtk_affine_global_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
int qtk_natural_gradient_affine_component_read(qtk_affine_global_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
int qtk_natural_gradient_affine_component_read2(qtk_affine_global_component_t* com,wtk_source_t *src, wtk_strbuf_t *buf,float max_w,unsigned int is_fixed, int fixed_nbytes, int use_custom_acc, int porder);
int qtk_affine_component_read(qtk_affine_global_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
int qtk_affine_component_read2(qtk_affine_global_component_t* com, wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed, int fixed_nbytes, int use_custom_acc, int porder);
int qtk_rectified_linear_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf);
int qtk_normalize_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf);
//int qtk_log_softmax_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf);
int qtk_activate_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf);
int qtk_element_wise_product_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf);
int qtk_natural_gradient_per_element_scale_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
int qtk_dropout_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf);
int qtk_backprop_truncation_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf);
int qtk_noop_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf);
int qtk_batch_norm_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed);
int qtk_linear_component_read(qtk_affine_global_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed);
int qtk_lstm_nolinearity_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed);
int qtk_timeheight_convolution_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed);
int qtk_scale_offset_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed);
int qtk_max_pooling_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed);
int qtk_pnorm_component_read(nnet3_component_t* com,wtk_source_t* src,wtk_strbuf_t* buf,float max_w,unsigned int is_fixed);

void qtk_nnet3_component_mat_add(qtk_blas_matrix_t *input,qtk_blas_matrix_t *output,
                qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *output_info,float alpha);
/*void qtk_fix_affine_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
		qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_affine_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
		qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_natural_gradient_affine_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);*/
void qtk_general_dropout_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
        qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info,qtk_nnet3_precomputed_indexes_t* index);


void qtk_rectified_linear_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_normallize_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_activate_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_element_wise_product_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_natural_gradient_per_element_scale_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_dropout_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_backprop_truncation_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst,
		qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_noop_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst,
                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_batchnorm_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *src,qtk_blas_matrix_t *dst,
                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
//void qtk_linear_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
 //               qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_lstm_nolinearity_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_timeheight_convolution_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info,qtk_nnet3_precomputed_indexes_t* index);
void qtk_scale_offset_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);
void qtk_max_pooling_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);

void qtk_pnorm_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *dst,qtk_blas_matrix_t *src,
                qtk_nnet3_submatrix_info_t *dst_info,qtk_nnet3_submatrix_info_t *src_info);

void qtk_affine_global_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
                qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
#ifdef USE_AFFINE_FAST
void qtk_affine_global_propagate_fast(nnet3_component_t *com1,
                                      qtk_blas_matrix_t *input,
                                      qtk_blas_matrix_t *out_put,
                                      qtk_nnet3_submatrix_info_t *input_info,
                                      qtk_nnet3_submatrix_info_t *out_put_info);
#endif
void qtk_affine_global_propagate_avx(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
                qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_affine_global_propagate_avxcustom(nnet3_component_t* com1, qtk_blas_matrix_t *input,
		qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int porder);

void qtk_affine_global_propagate_avx2custom(nnet3_component_t* com1, qtk_blas_matrix_t *input,
		qtk_blas_matrix_t *out_put, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int porder);

void qtk_affine_global_delete(nnet3_component_t* com,unsigned int is_fixed);
//void qtk_fix_affine_delete(nnet3_component_t* com);
//void qtk_affine_delete(nnet3_component_t* com);
//void qtk_natural_gradient_affine_delete(nnet3_component_t* com);
void qtk_rectified_linear_delete(nnet3_component_t* com);
void qtk_normallize_delete(nnet3_component_t* com);
void qtk_activate_delete(nnet3_component_t* com);
void qtk_element_wise_product_delete(nnet3_component_t* com);
void qtk_natural_gradient_per_element_scale_delete(nnet3_component_t* com,unsigned int is_fixed);
void qtk_dropout_delete(nnet3_component_t* com);
void qtk_backprop_truncation_delete(nnet3_component_t* com);
void qtk_lstm_nolinearity_delete(nnet3_component_t* com,unsigned int is_fixed);
void qtk_timeheight_convolution_delete(nnet3_component_t* com,unsigned int is_fixed);
void qtk_batchnorm_delete(nnet3_component_t* com,unsigned int is_fixed);
void qtk_scale_offset_delete(nnet3_component_t* com,unsigned int is_fixed);
void qtk_max_pooling_delete(nnet3_component_t* com,unsigned int is_fixed);
void qtk_pnorm_delete(nnet3_component_t *com, unsigned int is_fixed);
int qtk_blockweight_component_read(nnet3_component_t *com, wtk_source_t *src,
                                   wtk_strbuf_t *buf, float max_w,
                                   unsigned int is_fixed);
void qtk_blockweight_component_propagate(nnet3_component_t *com1,
                                         qtk_blas_matrix_t *dst,
                                         qtk_blas_matrix_t *src,
                                         qtk_nnet3_submatrix_info_t *dst_info,
                                         qtk_nnet3_submatrix_info_t *src_info);
int qtk_fsmn_component_read(nnet3_component_t *com, wtk_source_t *src,
                            wtk_strbuf_t *buf, float max_w,
                            unsigned int is_fixed);
void qtk_fsmn_component_propagate(nnet3_component_t *com1,
                                  qtk_blas_matrix_t *dst,
                                  qtk_blas_matrix_t *src,
                                  qtk_nnet3_submatrix_info_t *dst_info,
                                  qtk_nnet3_submatrix_info_t *src_info,
                                  qtk_nnet3_precomputed_indexes_t *index);
void qtk_fsmn_component_delete(nnet3_component_t* com,unsigned int is_fixed);
int qtk_namask_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
int qtk_namask_component_read2(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed,int use_custom_acc);
void qtk_namask_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_namask_component_propagate_custom(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
#ifdef USE_AVX
void qtk_namask_component_propagate_avx(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_namask_component_propagate_avxcustom(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
//void qtk_namask_component_propagate_avx3(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
//        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
#endif
void qtk_namask_component_delete(nnet3_component_t* com,unsigned int is_fixed);

int qtk_linearmask_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
void qtk_linearmask_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_linearmask_component_delete(nnet3_component_t* com,unsigned int is_fixed);

int qtk_permute_component_read(nnet3_component_t* com,wtk_source_t *src,wtk_strbuf_t *buf,float max_w,unsigned int is_fixed);
void qtk_permute_component_propagate(nnet3_component_t* com1,qtk_blas_matrix_t *input,qtk_blas_matrix_t *out_put,
        qtk_nnet3_submatrix_info_t *input_info,qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_permute_component_delete(nnet3_component_t* com,unsigned int is_fixed);

void wtk_nnet3_log(float *p,int n);

#ifdef __cplusplus
};
#endif
#endif
