#ifndef WTK_ASR_PARM_WTK_DELTA
#define WTK_ASR_PARM_WTK_DELTA
#include "wtk/core/wtk_type.h" 
#include "wtk_delta_cfg.h"
#include "wtk/core/wtk_robin.h"
#include "wtk_kfeat.h"
#include "./wtk/core/wtk_fixpoint.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_delta wtk_delta_t;

struct wtk_kparm;

struct wtk_delta
{
	wtk_delta_cfg_t *cfg;
	wtk_robin_t **robin;
	int vec_size;
	float sigma[3];
	int fix_sigma[3];
	short pos[3];
	struct wtk_kparm *parm;
};

wtk_delta_t* wtk_delta_new(wtk_delta_cfg_t *cfg,int vec_size);
int wtk_delta_bytes(wtk_delta_t *delta);
void wtk_delta_delete(wtk_delta_t *delta);
void wtk_delta_reset(wtk_delta_t *delta);
void wtk_delta_feed(wtk_delta_t *delta,wtk_kfeat_t *feat);
void wtk_delta_flush(wtk_delta_t *delta);

void wtk_delta_feed_fix(wtk_delta_t *delta,wtk_kfeat_t *feat);
void wtk_delta_flush_fix(wtk_delta_t *delta);
#ifdef __cplusplus
};
#endif
#endif
