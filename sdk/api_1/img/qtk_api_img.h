#ifndef __QTK_api_img_H__
#define __QTK_api_img_H__
#include "qtk_api_img_cfg.h"
#include "sdk/session/qtk_session.h"
#include "sdk/api_1/asr/qtk_asr.h"
#include "wtk/asr/img/qtk_img.h"
#include "sdk/codec/qtk_msg.h"
#include "wtk/asr/vad/wtk_vad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	QTK_API_IMG_TYPE_SPEECH_START,
	QTK_API_IMG_TYPE_SPEECH_END,
	QTK_API_IMG_TYPE_WAKE,
}qtk_api_img_type_t;


typedef void(*qtk_api_img_notify_f)(void *ths, qtk_api_img_type_t type, char *data, int len);

typedef struct qtk_api_img{
	qtk_api_img_cfg_t *cfg;
	qtk_session_t *session;
	qtk_img_rec_t *img;
	wtk_vad_t *vad;
	wtk_queue_t vad_q;
	void *ths;
	qtk_api_img_notify_f notify;
	wtk_wavfile_t *wav_asr;
	unsigned int sil:1;
}qtk_api_img_t;


qtk_api_img_t *qtk_api_img_new(qtk_api_img_cfg_t *cfg, qtk_session_t *session);
void qtk_api_img_delete(qtk_api_img_t *api_img);
int qtk_api_img_start(qtk_api_img_t *api_img);
int qtk_api_img_reset(qtk_api_img_t *api_img);
int qtk_api_img_feed(qtk_api_img_t *api_img, char *data, int bytes, int is_end);
void qtk_api_img_set_notify(qtk_api_img_t *api_img, void *ths, qtk_api_img_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
