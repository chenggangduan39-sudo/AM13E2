#ifndef WTK_VITE_VPRINT_DETECT_WTK_VDETECT
#define WTK_VITE_VPRINT_DETECT_WTK_VDETECT
#include "wtk/core/wtk_type.h" 
#include "wtk_vdetect_cfg.h"
#include "wtk/asr/vprint/parm/wtk_vparm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vdetect wtk_vdetect_t;

struct wtk_vdetect
{
	wtk_vdetect_cfg_t *cfg;
	wtk_vparm_t *parm;
	wtk_vdetect_usr_t *usr;
	wtk_vdetect_usr_t *max_usr;
	double max_llr;
	double mean_prob;
	int frames;
	int index;
};

wtk_vdetect_t* wtk_vdetect_new(wtk_vdetect_cfg_t *cfg,wtk_vparm_cfg_t *parm_cfg);
void wtk_vdetect_delete(wtk_vdetect_t *d);
void wtk_vdetect_reset(wtk_vdetect_t *d);
void wtk_vdetect_start(wtk_vdetect_t *d);
void wtk_vdetect_feed(wtk_vdetect_t *d,char *data,int bytes,int is_end);
void wtk_vdetect_feed_feature(wtk_vdetect_t *d,wtk_vparm_feature_t *f,int is_end);
void wtk_vdetect_print(wtk_vdetect_t *d);
wtk_string_t* wtk_vdetect_get_usr(wtk_vdetect_t *d);
#ifdef __cplusplus
};
#endif
#endif
