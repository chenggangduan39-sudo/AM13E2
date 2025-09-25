#ifndef WTK_ASR_FEXTRA_FNN_QLAS_WTK_QLAS
#define WTK_ASR_FEXTRA_FNN_QLAS_WTK_QLAS
#include "wtk/core/wtk_type.h" 
#include "wtk_qlas_cfg.h"
#include "../../wtk_feat.h"
#include "wtk/core/wtk_robin.h"
#include "wtk_qlasasm.h"
#ifdef USE_NEON
#include <arm_neon.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_qlas wtk_qlas_t;
struct wtk_fnn;

typedef struct
{
	wtk_veci_t *mul;
	union
	{
		wtk_vecb_t *boutput;
		wtk_vecs_t *soutput;
	}output;
	wtk_vecf_t *softmax;
}wtk_qlas_fix_t;

struct wtk_qlas
{
	wtk_qlas_cfg_t *cfg;
	struct wtk_fnn *fnn;
	wtk_vecf_t *input_feat;
	wtk_vecf_t **output;
	wtk_robin_t *robin;
	wtk_feat_t *last_feat;
	float *bias;
	union
	{
		wtk_vecs_t *sinput;
		wtk_vecb_t *binput;
	}fixinput;
	wtk_qlas_fix_t *fix_output;
	int index;
	//wtk_dnn_layer_t *last_l;
	wtk_qlas_layer_t *last_l;
	float last_scale;
};

wtk_qlas_t* wtk_qlas_new(wtk_qlas_cfg_t *cfg,struct wtk_fnn *fnn);
int wtk_qlas_bytes(wtk_qlas_t *qlas);
void wtk_qlas_delete(wtk_qlas_t *qlas);
void wtk_qlas_reset(wtk_qlas_t *qlas);
void wtk_qlas_process_layer(wtk_qlas_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f);
void wtk_qlas_flush(wtk_qlas_t *f);
void wtk_qlas_flush_end(wtk_qlas_t *f);

int wtk_qlas_matb_mul_row_value(char *p1,signed char *p2,int col);
int wtk_qlas_matb_mul_row_asm_value(char *p1,signed char *p2,int col);
int wtk_qlas_mats_mul_row_asm_value( short *p1,short *p2,int col);
int wtk_qlas_mats_mul_row_value( short *p1,short *p2,int col);
float wtk_qlas_get_dnn_value(wtk_qlas_t *d,wtk_feat_t *f,int index);
#ifdef __cplusplus
};
#endif
#endif
