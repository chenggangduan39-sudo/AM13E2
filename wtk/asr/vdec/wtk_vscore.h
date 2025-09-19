#ifndef WTK_ASR_VDEC_WTK_VSCORE
#define WTK_ASR_VDEC_WTK_VSCORE
#include "wtk/core/wtk_type.h" 
#include "wtk_vscore_cfg.h"
#include "wtk_fa.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vscore wtk_vscore_t;

struct wtk_vscore
{
	wtk_vscore_cfg_t *cfg;
	wtk_queue_t queue;
	wtk_fextra_t *parm;
	wtk_vrec_t *fa;
	wtk_vrec_t *loop;
	wtk_fa_t *score;
};


wtk_vscore_t* wtk_vscore_new(wtk_vscore_cfg_t *cfg);
void wtk_vscore_delete(wtk_vscore_t *s);
void wtk_vscore_start(wtk_vscore_t *s);
void wtk_vscore_reset(wtk_vscore_t *s);
void wtk_vscore_feed(wtk_vscore_t *s,char *data,int len,int is_end);
#ifdef __cplusplus
};
#endif
#endif
