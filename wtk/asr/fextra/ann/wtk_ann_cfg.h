#ifndef WTK_VITE_ANN_WTK_ANN_CFG_H_
#define WTK_VITE_ANN_WTK_ANN_CFG_H_
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk_ann_wbop.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ann_cfg wtk_ann_cfg_t;
typedef struct wtk_ann_res wtk_ann_res_t;

typedef struct
{
	wtk_matrix_t *mean;
	wtk_matrix_t *bias;
}wtk_ann_normal_t;


struct wtk_ann_res
{
	wtk_ann_cfg_t *cfg;
	wtk_ann_normal_t left_normal;
	wtk_ann_normal_t right_normal;
	wtk_ann_normal_t merge_normal;
	wtk_ann_wb_t left_wb;
	wtk_ann_wb_t right_wb;
	wtk_ann_wb_t merge_wb;
	wtk_matrix_t *pca_mat;
	wtk_matrix_t *hlda_mat;
};

struct wtk_ann_cfg
{
	char *hlda_fn;
	char *pca_fn;
	char *left_normal_fn;
	char *right_normal_fn;
	char *merge_normal_fn;
	char *left_wb_fn;	//weight bias
	char *right_wb_fn;
	char *merge_wb_fn;
	int normal_rows;
	int normal_cols;
	int hide_rows;
	int hide_cols;
	int out_rows;
	int out_cols;
	int win;
	int reduce_row;
	int merge_cols;
	int merge_rows;
	int align;
// =============   parm Section ===============
	wtk_fextra_cfg_t ann_parm;
	wtk_fextra_cfg_t phn_parm;
// =============   resource section ===========
	wtk_ann_res_t res;
	unsigned use_hlda:1;
};

int wtk_ann_res_delete(wtk_ann_res_t *res);
int wtk_ann_res_init(wtk_ann_res_t *r,wtk_ann_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_ann_res_clean(wtk_ann_res_t *r);

//==========  configure section ===========
int wtk_ann_cfg_init(wtk_ann_cfg_t *cfg);
int wtk_ann_cfg_update(wtk_ann_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_ann_cfg_update2(wtk_ann_cfg_t *cfg);
int wtk_ann_cfg_clean(wtk_ann_cfg_t *cfg);
void wtk_ann_cfg_set_example(wtk_ann_cfg_t *cfg);
void wtk_ann_cfg_print(wtk_ann_cfg_t *cfg);
int wtk_ann_cfg_update_local(wtk_ann_cfg_t* cfg,wtk_local_cfg_t *lc);

//========= matrxi load functuioin ========
#define wtk_ann_res_load_matrix_s(s,b,row,col,t,n) wtk_ann_res_load_matrix(s,b,row,col,t,n,sizeof(n)-1)
wtk_matrix_t* wtk_ann_res_load_matrix(wtk_source_t *s,wtk_strbuf_t *b,int rows,int cols,int t,char *n,int n_bytes);
#ifdef __cplusplus
};
#endif
#endif
