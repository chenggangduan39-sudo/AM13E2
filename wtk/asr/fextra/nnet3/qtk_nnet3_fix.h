#ifndef QTK_NNET3_FIX_H_
#define QTK_NNET3_FIX_H_
#include "qtk_nnet3_component.h"
#include "wtk/asr/wfst/kwdec/qtk_wakeup_trans_model_cfg.h"
#include "wtk/asr/wfst/kaldifst/qtk_trans_model_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

void qtk_nnet3_update_fix_f2s(qtk_blas_matrix_t *m,wtk_mats_t *ms,float max_w);
void qtk_nnet3_update_fix_f2s2(wtk_matrix_t *m,wtk_mats_t *ms,float max_w);
void qtk_nnet3_update_fix_f2i(qtk_blas_matrix_t* m,void* mfix,float max_w, int fixed_nbytes);
void qtk_nnet3_write_mats(FILE *f,wtk_mats_t *ms);
void qtk_nnet3_write_mati(FILE *f,void* mfix, int fixed_nbytes);
void qtk_nnet3_write_matrix(FILE *f,qtk_blas_matrix_t *b);
void wtk_nnet3_write_mati2(FILE *f,void* mfix, int fixed_nbytes);

int qtk_nnet3_affine_global_component_source_fill(qtk_affine_global_component_t *ga_com,wtk_source_t *src);
int qtk_nnet3_affine_global_component_source_fill2(qtk_affine_global_component_t *ga_com,wtk_source_t *src, int fixed_nbytes);
int qtk_nnet3_batch_norm_component_source_fill(qtk_batch_norm_component_t *batchnorm_com,wtk_source_t *src);
int qtk_nnet3_activate_component_source_fill(qtk_activate_component_t* ls_com,wtk_source_t *src);
int qtk_nnet3_lstm_component_source_fill(qtk_lstm_nolinearity_component_t* lstm_com,wtk_source_t *src);
int qtk_nnet3_conv_component_source_fill(qtk_timeheight_convolution_component_t* conv_com,wtk_source_t *src);
int qtk_nnet3_scale_offset_component_source_fill(qtk_scale_offset_component_t* scaleoff_com,wtk_source_t *src);
int qtk_nnet3_normalize_component_source_fill(qtk_normalize_component_t* n_com,wtk_source_t *src);
int qtk_nnet3_backpro_tru_component_source_fill(qtk_backprop_truncation_component_t *backpro_com,wtk_source_t *src);

void qtk_affine_global_propagate_fix(nnet3_component_t* com1, wtk_mats_t *input,
                wtk_mats_t *out_put,qtk_blas_matrix_t *outputf, qtk_nnet3_submatrix_info_t *input_info,
                qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_affine_global_propagate_fixi(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info);
void qtk_affine_global_propagate_fixiop1(nnet3_component_t* com1, wtk_mats_t *input,
		wtk_mats_t *out_put,qtk_blas_matrix_t *out_putf, qtk_nnet3_submatrix_info_t *input_info,
		qtk_nnet3_submatrix_info_t *out_put_info, int porder);
#ifdef __ANDROID__
void qtk_affine_global_propagate_fixiop1_neon(nnet3_component_t* com1, wtk_mats_t *input,
                wtk_mats_t *out_put,qtk_blas_matrix_t *outputf, qtk_nnet3_submatrix_info_t *input_info,
                qtk_nnet3_submatrix_info_t *out_put_info, int porder);
#endif
#ifdef USE_NEON64
void qtk_affine_global_propagate_neon64(nnet3_component_t* com1, wtk_mats_t *input,
                wtk_mats_t *out_put,qtk_blas_matrix_t *outputf, qtk_nnet3_submatrix_info_t *input_info,
                qtk_nnet3_submatrix_info_t *out_put_info);
#elif USE_NEON32
void qtk_affine_global_propagate_neon32(nnet3_component_t* com1, wtk_mats_t *input,
                wtk_mats_t *out_put,qtk_blas_matrix_t *outputf, qtk_nnet3_submatrix_info_t *input_info,
                qtk_nnet3_submatrix_info_t *out_put_info);
#endif

void qtk_batchnorm_propagate_fix(nnet3_component_t* com1, wtk_mats_t *src,
                qtk_blas_matrix_t *dst, qtk_nnet3_submatrix_info_t *dst_info,
                qtk_nnet3_submatrix_info_t *src_info);

void qtk_timeheight_convolution_propagate_fix(nnet3_component_t* com1,
                qtk_blas_matrix_t *dst, wtk_mats_t *src,
                qtk_nnet3_submatrix_info_t *dst_info,
                qtk_nnet3_submatrix_info_t *src_info,
                qtk_nnet3_precomputed_indexes_t* index);

void qtk_scale_offset_propagate_fix(nnet3_component_t* com1, qtk_blas_matrix_t *dst,
                wtk_mats_t *src, qtk_nnet3_submatrix_info_t *dst_info,
                qtk_nnet3_submatrix_info_t *src_info);

void qtk_nnet3_wakeup_trans_mdl_write(FILE *f,qtk_wakeup_trans_model_t *wt_mdl);
void qtk_nnet3_trans_mdl_write(FILE *f,qtk_trans_model_t *t_mdl);
void qtk_nnet3_trans_mdl_write_normal(FILE *f,qtk_trans_model_t *t_mdl);
int qtk_nnet3_trans_model_load_chain_fix_bin(qtk_trans_model_cfg_t *t_mdl,wtk_source_t *src);
int qtk_nnet3_trans_model_load_chain2_fix_bin(qtk_trans_model_cfg_t *t_mdl,wtk_source_t *src);
int qtk_nnet3_wakeup_trans_model_load_chain_fix_bin(qtk_wakeup_trans_model_cfg_t* wt_mdl,wtk_source_t *src);

#ifdef __cplusplus
};
#endif
#endif
