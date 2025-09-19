#ifndef WTK_ASR_VAD_WTK_VAD2
#define WTK_ASR_VAD_WTK_VAD2
#include "wtk/asr/vad/wtk_vad.h" 
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vad2 wtk_vad2_t;

typedef enum
{
	WTK_VAD2_START,
	WTK_VAD2_DATA,
	WTK_VAD2_END,
	WTK_VAD2_CANCEL,
}wtk_vad2_cmd_t;

typedef void(*wtk_vad2_notify_f)(void *ths,wtk_vad2_cmd_t cmd,short *data,int len);
typedef void(*wtk_vad2_float_notify_f)(void *ths,wtk_vad2_cmd_t cmd,float *data,int len);

struct wtk_vad2
{
	wtk_vad_t *vad;
	wtk_queue_t q;
	wtk_strbuf_t *buf;
	int max_cache;
	long counter;
	long lst_speech;	//last speech margin time
	long lst_speech2;	//last real speech time;  lst_speech+margin;
	long lst_sil;	//last sil time
	int nspeech;
	int vad_pos;
	long output;
	void *ths;
	wtk_vad2_notify_f notify;
	void *float_ths;
	wtk_vad2_float_notify_f float_notify;
	unsigned sil:1;
	unsigned use_vad_start:1;
};

wtk_vad2_t* wtk_vad2_new(wtk_vad_cfg_t *cfg);
int wtk_vad2_bytes(wtk_vad2_t *vad);
void wtk_vad2_delete(wtk_vad2_t *vad);
void wtk_vad2_start(wtk_vad2_t *vad);
void wtk_vad2_reset(wtk_vad2_t *vad);
void wtk_vad2_set_notify(wtk_vad2_t *vad,void *ths,wtk_vad2_notify_f notify);
void wtk_vad2_feed(wtk_vad2_t *vad,char *data,int len,int is_end);
void wtk_vad2_set_float_notify(wtk_vad2_t *vad,void *ths,wtk_vad2_float_notify_f notify);
void wtk_vad2_feed_float(wtk_vad2_t *vad,float *data,int len,int is_end);
void wtk_vad2_reset_route(wtk_vad2_t *vad);
#ifdef __cplusplus
};
#endif
#endif
