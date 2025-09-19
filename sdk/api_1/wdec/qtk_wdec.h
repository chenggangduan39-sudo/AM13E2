#ifndef QTK_API_WDEC_QTK_TTS
#define QTK_API_WDEC_QTK_TTS

#include "wtk/asr/wdec/wtk_wdec.h"
#include "wtk/asr/wfst/kwdec2/wtk_kwdec2.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "qtk_wdec_cfg.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	QTK_WDEC_VAD_START,
	QTK_WDEC_VAD_DATA,
	QTK_WDEC_VAD_END,
	QTK_WDEC_VAD_CANCEL,
    QTK_DECODER_WRAPPER_CFG_H_,
}qtk_wdec_cmd_t;

typedef struct qtk_wdec qtk_wdec_t;
typedef void (*qtk_wdec_notify_f)(void *ths,qtk_wdec_cmd_t cmd,float fs,float fe,short *data, int len);

struct qtk_wdec {
    qtk_wdec_cfg_t *cfg;
    wtk_wdec_t *wdec;
	wtk_kwdec2_t *kwdec2;
    qtk_session_t *session;
	wtk_vad_t *vad;
	wtk_queue_t vad_q;

	void *ths_info;
	qtk_wdec_notify_f notify_info;

	void *eths;
	qtk_engine_notify_f enotify;

    float wake_fs;
    float wake_fe;
    float wake_prob;
	wtk_lock_t lock;
	unsigned int sil:1;
};

qtk_wdec_t *qtk_wdec_new(qtk_wdec_cfg_t *cfg, qtk_session_t *session);
void qtk_wdec_delete(qtk_wdec_t *wdec);

int qtk_wdec_start(qtk_wdec_t *wdec);
int qtk_wdec_feed(qtk_wdec_t *wdec, char *data, int len, int is_end);
int qtk_wdec_reset(qtk_wdec_t *wdec);
void qtk_wdec_set_wake_words(qtk_wdec_t *wdec,char *data,int bytes);

void qtk_wdec_set_notify_info(qtk_wdec_t *wdec, void *ths_info,qtk_wdec_notify_f notify_info);
void qtk_wdec_set_notify(qtk_wdec_t *wdec, void *eths,qtk_engine_notify_f notify_f);
#ifdef __cplusplus
};
#endif
#endif
