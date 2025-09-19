#ifndef WTK_VAD_WTK_VAD_H_
#define WTK_VAD_WTK_VAD_H_
#include "wtk/asr/vad/annvad/wtk_annvad.h"
#include "wtk/asr/vad/fnnvad/wtk_fnnvad.h"
#include "wtk_vad_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vad wtk_vad_t;

typedef enum
{
	WTK_VAD_SIL,
	WTK_VAD_MIN_SPEECH,
	WTK_VAD_SPEECH,
	WTK_VAD_SPEECH_END,
}wtk_vad_state_t;

struct wtk_vad
{
	wtk_vad_cfg_t *cfg;
	union
	{
		wtk_annvad_t *annvad;
		wtk_fnnvad_t *dnnvad;
		wtk_gmmvad2_t *gmmvad2;
		wtk_fevad_t *fevad;
		wtk_kvad_t *kvad;
	}route;
	wtk_queue_t cache_q;
	wtk_queue_t *output_queue;
	wtk_vad_state_t state;
	int speech_end_sil_frames;
	int min_speech_count;
	int left_margin;
	int right_margin;
};

/**
 *	@brief output_queue is wtk_vframe_t queue;
 */
wtk_vad_t* wtk_vad_new(wtk_vad_cfg_t *cfg,wtk_queue_t *output_queue);
int wtk_vad_bytes(wtk_vad_t *v);
void wtk_vad_delete(wtk_vad_t *v);
void wtk_vad_set_margin(wtk_vad_t *v,int left,int right);
int wtk_vad_start(wtk_vad_t *v);
int wtk_vad_reset(wtk_vad_t *v);
int wtk_vad_restart(wtk_vad_t *vad);
int wtk_vad_feed(wtk_vad_t *vad,char *data,int bytes,int is_end);
int wtk_vad_feed_float(wtk_vad_t *vad,float *data,int len,int is_end);
void wtk_vad_flush(wtk_vad_t *vad);
/**
 * @brief when frame poped from output_queue and no longer used, call wtk_vad_push_vframe
 * 	to send vframe back to vad;
 */
void wtk_vad_push_vframe(wtk_vad_t *vad,wtk_vframe_t *f);
void wtk_vad_get_left_data(wtk_vad_t *v,wtk_string_t *data);
void wtk_vad_set_speech_thresh(wtk_vad_t *v,float f);
void wtk_vad_set_speech_thresh2(wtk_vad_t *v,float hf,float lf);

typedef void(*wtk_vad_flush_data_f)(void *ths,char *data,int len);
/**
 * @brief 获取vad缓存数据;
 */
void wtk_vad_flush_cached_audio(wtk_vad_t *v,void *ths,wtk_vad_flush_data_f flush);

void wtk_vad_peek_unuse_data(wtk_vad_t *v,wtk_strbuf_t *buf);
//======================== example =====================
void wtk_vad_test_file(wtk_vad_t *vad,char *wav_fn);

void wtk_vad_queue_print_mlf(wtk_queue_t *q,float frame_dur,FILE *s);

void wtk_vad_queue_print_p(wtk_queue_t *q,float frame_dur,FILE *s);

void wtk_vad_print_mlf(wtk_vad_t *vad);

int wtk_vad_get_output_left_sil_margin(wtk_vad_t *vad);

int wtk_vad_get_frame_step(wtk_vad_t *vad);

float wtk_vad_get_frame_dur(wtk_vad_t *vad);

void wtk_vad_test_file2(char *cfg,char *fn);

void wtk_vad_print_pend(wtk_vad_t *vad);

void wtk_vad_conf_set(wtk_vad_t *v,float power,float speech,float sil, int left, int right);
void wtk_vad_conf_print(wtk_vad_t *v);

#ifdef __cplusplus
};
#endif
#endif
