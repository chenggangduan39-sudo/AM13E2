#ifndef WTK_VITE_DEC_WTK_VDEC_H_
#define WTK_VITE_DEC_WTK_VDEC_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/vdec/rec/wtk_rec.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk_vdec_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vdec wtk_vdec_t;
struct wtk_vdec
{
	wtk_vdec_cfg_t *cfg;
	wtk_queue_t queue;
	wtk_fextra_t *parm;
	wtk_rec_t *rec;
	wtk_lat_t *net;
	wtk_heap_t *glb_heap;
	wtk_transcription_t* trans;
};

wtk_vdec_t* wtk_vdec_new(wtk_vdec_cfg_t *cfg);
void wtk_vdec_delete(wtk_vdec_t *v);
int wtk_vdec_start(wtk_vdec_t *v);
int wtk_vdec_feed(wtk_vdec_t *v,int state,char *audio,int audio_bytes);
int wtk_vdec_reset(wtk_vdec_t *v);
void wtk_vdec_print(wtk_vdec_t *v);
void wtk_vdec_get_rec(wtk_vdec_t *v,wtk_strbuf_t *buf);
#ifdef __cplusplus
};
#endif
#endif
