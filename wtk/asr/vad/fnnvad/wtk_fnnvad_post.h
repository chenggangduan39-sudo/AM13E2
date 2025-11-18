#ifndef WTK_VAD_DNNVAD_wtk_fnnvad_POST_H_
#define WTK_VAD_DNNVAD_wtk_fnnvad_POST_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/asr/vad/wtk_short_buffer.h"
#include "wtk/asr/vad/wtk_vframe.h"
#include "wtk/core/wtk_os.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fnnvad_post wtk_fnnvad_post_t;

typedef enum
{
	WTK_FNNVAD_SIL,
	WTK_FNNVAD_SPEECH,
}wtk_fnnvad_state_t;

typedef struct
{
	wtk_vframe_t *frame;
	unsigned short ref;
	unsigned char is_sil;
}wtk_fnnvad_feat_t;

struct wtk_fnnvad;

typedef enum
{
	wtk_fnnvad_ECHO_INIT,
	wtk_fnnvad_ECHO_WAIT,
	wtk_fnnvad_ECHO_END,
}wtk_fnnvad_echo_state_t;

typedef enum
{
	wtk_fnnvad_SPEECH_INIT,
	wtk_fnnvad_SPEECH_HIGH,
	wtk_fnnvad_SPEECH_LOW,
}wtk_fnnvad_speech_state_t;

struct wtk_fnnvad_post
{
	wtk_fnnvad_state_t state;
	wtk_robin_t *sil_robin;			//save sil wtk_feature;
	wtk_robin_t *speech_robin;		//save speech wtk_feature;
	float speech_engine_thresh;
	float speech_engine_thresh_low;
	int speech_high_frame;
	int speech_sil_frames;
	struct wtk_fnnvad *vad;
	wtk_fnnvad_echo_state_t echo_state;
	wtk_fnnvad_speech_state_t speech_state;
	int detect_high_frames;
	int detect_low_frames;
	float detect_high_start_thresh;
	float detect_low_start_thresh;
	float delete_e;
	unsigned force_sil:1;
};

wtk_fnnvad_post_t* wtk_fnnvad_post_new(struct wtk_fnnvad *vad);
int wtk_fnnvad_post_bytes(wtk_fnnvad_post_t *post);
void  wtk_fnnvad_post_delete(wtk_fnnvad_post_t *p);
void  wtk_fnnvad_post_reset(wtk_fnnvad_post_t *p);
void wtk_fnnvad_post_feed(wtk_fnnvad_post_t *p,wtk_fnnvad_feat_t *feat);
void wtk_fnnvad_flush_end(wtk_fnnvad_post_t *p);
#ifdef __cplusplus
};
#endif
#endif
