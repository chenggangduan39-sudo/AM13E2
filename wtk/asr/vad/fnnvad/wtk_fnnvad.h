#ifndef WTK_VAD_DNNVAD_wtk_fnnvad_H_
#define WTK_VAD_DNNVAD_wtk_fnnvad_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/vad/wtk_short_buffer.h"
#include "wtk/asr/vad/wtk_vframe.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk_fnnvad_cfg.h"
#include "wtk_fnnvad_post.h"
#include "wtk/core/wtk_vpool.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fnnvad wtk_fnnvad_t;

typedef struct
{
	wtk_queue_node_t q_n;
	int index;
	float f0;
	float fe;
}wtk_vad_f0_ev_t;

struct wtk_fnnvad
{
	wtk_fnnvad_cfg_t *cfg;
	wtk_fextra_t *parm;
	wtk_queue_t param_output_q;
	//---------- audio section -----------
	wtk_short_buffer_t *frame_buffer;
	wtk_queue_t frame_q;		//wtk_vframe_t queue used for save audio;
	wtk_hoard_t frame_hoard;
	wtk_robin_t *feat_robin;		//save feature,used as window;
	wtk_bit_heap_t *feat_heap;
	int n_frame_index;
	//---------------------------------
	void *raise_ths;
	wtk_vframe_raise_f raise;
	wtk_fnnvad_post_t *post;
	float echo_speech_thresh;
	float last_f0;
	float last_fe;
	int speech_energe_thresh_set_frame;
	float speech_energe_thresh;
	float speech_energe_thresh_lf;
	///int has_speech;
};

wtk_fnnvad_t* wtk_fnnvad_new(wtk_fnnvad_cfg_t *cfg,void *raise_ths,wtk_vframe_raise_f raise);
int wtk_fnnvad_bytes(wtk_fnnvad_t *v);
void wtk_fnnvad_delete(wtk_fnnvad_t *v);
void wtk_fnnvad_reset(wtk_fnnvad_t *v);
int wtk_fnnvad_feed(wtk_fnnvad_t *v,char *data,int bytes,int is_end);
int wtk_fnnvad_feed_float(wtk_fnnvad_t *v,float *data,int len,int is_end);
void wtk_fnnvad_push_vframe(wtk_fnnvad_t *v,wtk_vframe_t *f);
void wtk_fnnvad_raise_feat(wtk_fnnvad_t *v,wtk_fnnvad_feat_t *f,int is_sil);
void wtk_fnnvad_push_feat(wtk_fnnvad_t *v,wtk_fnnvad_feat_t *f);
void wtk_fnnvad_flush_frame_queue(wtk_fnnvad_t *v,wtk_queue_t *q);
void wtk_fnnvad_flush(wtk_fnnvad_t *v);
void wtk_fnnvad_set_speech_thresh(wtk_fnnvad_t *v,float f);
void wtk_fnnvad_set_speech_thresh2(wtk_fnnvad_t *v,float hf,float lf);
//----------------- test section ----------------------
void wtk_fnnvad_raise_dummy(wtk_fnnvad_t *v,wtk_vframe_t *f);
void wtk_fnnvad_print_pend(wtk_fnnvad_t *vad);
#ifdef __cplusplus
};
#endif
#endif
