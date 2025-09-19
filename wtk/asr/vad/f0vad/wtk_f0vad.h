#ifndef WTK_ASR_VAD_F0VAD_WTK_F0VAD
#define WTK_ASR_VAD_F0VAD_WTK_F0VAD
#include "wtk/asr/fextra/f0/wtk_f0.h" 
#include "wtk/asr/vad/wtk_short_buffer.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk_f0vad_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_f0vad wtk_f0vad_t;

struct wtk_f0vad
{
	wtk_f0vad_cfg_t *cfg;
	wtk_f0_t *f0;
	wtk_short_buffer_t *frame_buffer;
	wtk_hoard_t frame_hoard;
	void *raise_ths;
	wtk_vframe_raise_f raise;
	int n_frame_index;
};

wtk_f0vad_t* wtk_f0vad_new(wtk_f0vad_cfg_t *cfg);
void wtk_f0vad_delete(wtk_f0vad_t *v);
void wtk_f0vad_reset(wtk_f0vad_t *v);
void wtk_f0vad_feed(wtk_f0vad_t *vad,char *data,int bytes,int is_end);
void wtk_f0vad_flush_frame_queue(wtk_f0vad_t *v,wtk_queue_t *q);
void wtk_f0vad_push_vframe(wtk_f0vad_t *vad,wtk_vframe_t *f);
#ifdef __cplusplus
};
#endif
#endif
