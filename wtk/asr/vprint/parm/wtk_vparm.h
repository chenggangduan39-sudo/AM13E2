#ifndef WTK_VITE_VPRINT_WTK_VPARM
#define WTK_VITE_VPRINT_WTK_VPARM
#include "wtk/core/wtk_type.h" 
#include "wtk_vparm_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vparm wtk_vparm_t;

typedef enum
{
	WTK_VPARM_INIT,
	WTK_VPARM_SIL,
	WTK_VPARM_SPEECH,
}wtk_vparm_state_t;

typedef struct
{
	union {
		wtk_fixi_t *fix;
		wtk_feat_t *feature;
	}v;
	wtk_vparm_t *parm;
	int index;
	unsigned use_fix:1;
}wtk_vparm_feature_t;

struct wtk_vparm
{
	wtk_vparm_cfg_t *cfg;
	wtk_fextra_t *parm;
	wtk_vad_t *vad2;
	wtk_queue_t parm_output_q;
	wtk_queue_t vad_output_q;
	wtk_queue_t sil_q;
	wtk_vparm_state_t state;
	wtk_fixi_t *fix;
	float scale;
};

wtk_vparm_t* wtk_vparm_new(wtk_vparm_cfg_t *cfg);
void wtk_vparm_delete(wtk_vparm_t *v);
void wtk_vparm_reset(wtk_vparm_t *v);
void wtk_vparm_start(wtk_vparm_t *v);
void wtk_vparm_feed(wtk_vparm_t *v,char *data,int bytes,int is_end);
wtk_fixi_t* wtk_vparm_get_fixi(wtk_vparm_t *vp,wtk_feat_t *f,float scale);
#ifdef __cplusplus
};
#endif
#endif
