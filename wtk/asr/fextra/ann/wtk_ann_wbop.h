#ifndef WTK_VITE_ANN_WTK_ANN_WBOP_H_
#define WTK_VITE_ANN_WTK_ANN_WBOP_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/math/wtk_matrix.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ann_wbop wtk_ann_wbop_t;

typedef struct
{
	int hide_rows;
	int hide_cols;
	int out_rows;
	int out_cols;
}wtk_ann_wb_cfg_t;

/**
 * @brief weight matrix and bias matrix; wb short for [w]eight and [b]ias;
 */
typedef struct
{
	wtk_matrix_t *hid_w;
	wtk_matrix_t *out_w;
	wtk_matrix_t *hid_b;
	wtk_matrix_t *out_b;
}wtk_ann_wb_t;

struct wtk_ann_wbop
{
	wtk_ann_wb_t *wb;
	wtk_matrix_t *hide_matrix;
	wtk_matrix_t *out_matrix;
};

void wtk_ann_sigmoid(float *f,int len);
void wtk_ann_sigmoid2(float *f,int len);
void wtk_ann_sigmoid4(float *f,int len);
void wtk_ann_softmax(float* a,int len);
void wtk_ann_softmax2(float* a,int len);
void wtk_ann_softmax3(float* a,int len);
void wtk_ann_softmax4(float* a,int len);
void wtk_ann_softmax_vf(float* a,int len);
void wtk_ann_wb_init(wtk_ann_wb_t *w);
int wtk_ann_wb_clean(wtk_ann_wb_t *w);
void wtk_ann_wb_cfg_print(wtk_ann_wb_cfg_t *cfg);
wtk_ann_wbop_t* wtk_ann_wbop_new(wtk_ann_wb_t *wb);
void wtk_ann_wbop_delete(wtk_ann_wbop_t *w);
void wtk_ann_wbop_process(wtk_ann_wbop_t *w,wtk_matrix_t *m);
#ifdef __cplusplus
};
#endif
#endif
