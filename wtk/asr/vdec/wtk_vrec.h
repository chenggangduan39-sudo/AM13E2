#ifndef WTK_ASR_VDEC_WTK_VREC
#define WTK_ASR_VDEC_WTK_VREC
#include "wtk/core/wtk_type.h" 
#include "wtk_vrec_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vrec wtk_vrec_t;

struct wtk_vrec
{
	wtk_vrec_cfg_t *cfg;
	wtk_rec_t *rec;
	wtk_lat_t *net;
	wtk_heap_t *glb_heap;
};

wtk_vrec_t* wtk_vrec_new(wtk_vrec_cfg_t *cfg,wtk_hmmset_t *hmmset,float frame_dur);
void wtk_vrec_delete(wtk_vrec_t *r);
void wtk_vrec_start(wtk_vrec_t *r);
void wtk_vrec_reset(wtk_vrec_t *r);
void wtk_vrec_feed(wtk_vrec_t *r,wtk_vector_t *obs);
#ifdef __cplusplus
};
#endif
#endif
