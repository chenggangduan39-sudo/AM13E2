#ifndef WTK_VITE_ANN_WTK_ANN_H_
#define WTK_VITE_ANN_WTK_ANN_H_
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk_ann_cfg.h"
#include "wtk_ann_stream.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ann wtk_ann_t;

struct wtk_ann
{
	wtk_queue_t  ann_queue;
	wtk_queue_t  phn_queue;
	wtk_hoard_t feature_hoard;
	wtk_ann_cfg_t *cfg;
	wtk_fextra_t  *ann_parm;
	wtk_fextra_t  *phn_parm;
	wtk_robin_t  *ann_robin;
	wtk_matrix_t *left_w;
	wtk_matrix_t *right_w;
	wtk_vector_t **v_array;
	wtk_vector_t *left_factor;
	wtk_vector_t *right_factor;
	wtk_matrix_t *dct_matrix;
	wtk_matrix_t *dct_multi_matrx;
	wtk_ann_stream_t *left_stream;
	wtk_ann_stream_t *right_stream;
	wtk_ann_stream_t *merge_stream;
	wtk_matrix_t *pca_mat;
	wtk_matrix_t *phn_ann_mat;
	wtk_matrix_t *out_mat;
	int feature_cols;
	int half;		//width of left(right) feature array.
	int n_frame_index;
	wtk_queue_t *output_queue;
};

wtk_ann_t* wtk_ann_new(wtk_ann_cfg_t *cfg);
int wtk_ann_delete(wtk_ann_t *a);
int wtk_ann_init(wtk_ann_t *a,wtk_ann_cfg_t *cfg);
int wtk_ann_clean(wtk_ann_t *a);
int wtk_ann_reset(wtk_ann_t *a);
int wtk_ann_feed(wtk_ann_t *a,int s,short *data,int samples);
wtk_feat_t *wtk_ann_new_feature(wtk_ann_t *a);
int wtk_ann_push_feature(wtk_ann_t *a,wtk_feat_t *f);
wtk_feat_t* wtk_ann_pop_feature(wtk_ann_t *a);
#ifdef __cplusplus
};
#endif
#endif
