#ifndef WTK_ASR_PARM_WTK_LDA
#define WTK_ASR_PARM_WTK_LDA
#include "wtk/core/wtk_type.h" 
#include "wtk_lda_cfg.h"
#include "wtk_kfeat.h"
#include "wtk/core/wtk_robin.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lda wtk_lda_t;

struct wtk_kparm;

struct wtk_lda
{
	wtk_lda_cfg_t *cfg;
	wtk_robin_t *robin;
	int vec_size;
	struct wtk_kparm *parm;
	float *input_vec;
};

int wtk_lda_bytes(wtk_lda_t *lda,int v);
wtk_lda_t* wtk_lda_new(wtk_lda_cfg_t *cfg,int v);
void wtk_lda_delete(wtk_lda_t *lda);
void wtk_lda_reset(wtk_lda_t *lda);
void wtk_lda_feed(wtk_lda_t *lda,wtk_kfeat_t *feat,int is_end);
void wtk_lda_flush(wtk_lda_t *lda);
#ifdef __cplusplus
};
#endif
#endif
