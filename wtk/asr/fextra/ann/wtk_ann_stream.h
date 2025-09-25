#ifndef WTK_VITE_ANN_WTK_ANN_STREAM_H_
#define WTK_VITE_ANN_WTK_ANN_STREAM_H_
#include "wtk/core/math/wtk_matrix.h"
#include "wtk_ann_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ann_stream wtk_ann_stream_t;
struct wtk_ann_stream
{
	wtk_ann_normal_t *normal;
	wtk_ann_wb_t *wb;
	wtk_matrix_t *dct_matrix;
	wtk_matrix_t *hid_matrix;
	wtk_matrix_t *out_matrix;
};

wtk_ann_stream_t* wtk_ann_stream_new(wtk_ann_normal_t *normal,
		wtk_ann_wb_t* wb);
int wtk_ann_stream_init(wtk_ann_stream_t *s,wtk_ann_normal_t *normal,
		wtk_ann_wb_t* wb);
int wtk_ann_stream_delete(wtk_ann_stream_t *s);
int wtk_ann_stream_process(wtk_ann_stream_t *s,wtk_matrix_t *mul_mat,wtk_matrix_t *dct_mat,wtk_matrix_t *fea_mat);
int wtk_ann_stream_do_merge(wtk_ann_stream_t *s);
#ifdef __cplusplus
};
#endif
#endif
