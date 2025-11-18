#ifndef WTK_ASR_VAD_FEVAD_WTK_FEVAD
#define WTK_ASR_VAD_FEVAD_WTK_FEVAD
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/asr/vad/wtk_short_buffer.h"
#include "wtk_fevad_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fevad wtk_fevad_t;

/**
 * A SIMPLE BUT EFFICIENT REAL-TIME VOICE ACTIVITY DETECTION ALGORITHM
 */
typedef enum
{
	WTK_FEVAD_INIT,
	WTK_FEVAD_UPDATE,
	//WTK_FEVAD_SPEECH,
	//WTK_FEVAD_SIL,
}wtk_fevad_state_t;

typedef enum
{
	WTK_FEVAD_SIL,
	WTK_FEVAD_SPEECH,
}wtk_fevad_speech_state_t;

typedef int(*wtk_fevad_check_speech_f)(void *ths,int idx,float E,float F,float SFM,float min_e,float mean_start_e);

struct wtk_fevad
{
	wtk_fevad_cfg_t *cfg;
	wtk_stft_t *stft;
	int frame_size;
	wtk_hoard_t frame_hoard;
	void *raise_ths;
	wtk_vframe_raise_f raise;
	int n_frame_index;
	wtk_queue_t bak_q;
	wtk_fevad_state_t state;
	wtk_fevad_speech_state_t speech_state;
	int idx;
	int sil_cnt;
	float sfms;
	float mean_start_sil_e;
	float Min_E;
	float Min_F;
	float Min_Fv;
	float Min_SFM;
	float Thresh_E;
	float Thresh_F;
	float Thresh_Fv;
	float Thresh_SF;
	void *ths;
	short notch_mem[2];

	int lx;
	float fx;
	wtk_fevad_check_speech_f check_speech;
};

wtk_fevad_t* wtk_fevad_new(wtk_fevad_cfg_t *cfg,void *raise_ths,wtk_vframe_raise_f raise);
void wtk_fevad_delete(wtk_fevad_t *vad);
void wtk_fevad_reset(wtk_fevad_t *vad);
void wtk_fevad_set_notify(wtk_fevad_t *vad,void *ths,wtk_fevad_check_speech_f check_speech);
void wtk_fevad_feed(wtk_fevad_t *vad,char *data,int len,int is_end);
void wtk_fevad_feed_float(wtk_fevad_t *vad,float *data,int len,int is_end);
void wtk_fevad_flush_frame_queue(wtk_fevad_t *vad,wtk_queue_t *q);
void wtk_fevad_push_vframe(wtk_fevad_t *vad,wtk_vframe_t *vf);
#ifdef __cplusplus
};
#endif
#endif
