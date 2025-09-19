#ifndef WTK_VAD_GMMVAD_WTK_GMMVAD2
#define WTK_VAD_GMMVAD_WTK_GMMVAD2
#include "wtk/core/wtk_type.h" 
#include "wtk/asr/vad/wtk_short_buffer.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk_gmmvad.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gmmvad2 wtk_gmmvad2_t;

struct wtk_gmmvad2
{
	wtk_gmmvad_t *vad;
	wtk_short_buffer_t *frame_buffer;
	wtk_hoard_t frame_hoard;
	void *raise_ths;
	wtk_vframe_raise_f raise;
	int n_frame_index;
};

wtk_gmmvad2_t* wtk_gmmvad2_new(wtk_gmmvad_cfg_t *cfg,void *raise_ths,wtk_vframe_raise_f raise);
void wtk_gmmvad2_delete(wtk_gmmvad2_t *v);
void wtk_gmmvad2_reset(wtk_gmmvad2_t *v);
void wtk_gmmvad2_feed(wtk_gmmvad2_t *vad,char *data,int bytes,int is_end);
void wtk_gmmvad2_flush_frame_queue(wtk_gmmvad2_t *v,wtk_queue_t *q);
void wtk_gmmvad2_push_vframe(wtk_gmmvad2_t *vad,wtk_vframe_t *f);
#ifdef __cplusplus
};
#endif
#endif
