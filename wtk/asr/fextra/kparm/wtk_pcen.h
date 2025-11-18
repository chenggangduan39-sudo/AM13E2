#ifndef WTK_ASR_PARM_WTK_PCEN
#define WTK_ASR_PARM_WTK_PCEN
#include "wtk/core/wtk_type.h" 
#include "wtk_pcen_cfg.h"
#include "wtk_kfeat.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_pcen wtk_pcen_t;

struct wtk_pcen
{
	wtk_pcen_cfg_t *cfg;
	//void *notify_ths;
	float *last_smooth_vec;
	float *smooth_vec;
	int size;
	//wtk_kfeat_notify_f notify;
};

wtk_pcen_t* wtk_pcen_new(wtk_pcen_cfg_t *cfg,int vec_size);
int wtk_pcen_bytes(wtk_pcen_t *pcen);
void wtk_pcen_delete(wtk_pcen_t *pcen);
void wtk_pcen_reset(wtk_pcen_t *pcen);
//void wtk_pcen_set_notify(wtk_pcen_t *pcen,void *ths,wtk_kfeat_notify_f notify);
void wtk_pcen_feed(wtk_pcen_t *pcen,wtk_kfeat_t *feat);

#ifdef __cplusplus
};
#endif
#endif
