#ifndef QTK_MODULE_BFIO_QTK_MQFORM
#define QTK_MODULE_BFIO_QTK_MQFORM

#include "wtk/core/wtk_wavfile.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/bfio/resample/wtk_resample.h"
#ifndef OFFLINE_TEST
#include "sdk/audio/recorder/qtk_recorder.h"
#include "sdk/audio/player/qtk_player.h"
#endif

#include "sdk/qtk_api.h"
#include "qtk_mqform_cfg.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
#define QTK_MQFORM_MSG_DATA_HDR_LEN (128)

typedef struct qtk_mqform qtk_mqform_t;

struct qtk_mqform
{
	qtk_mqform_cfg_t *cfg;
	qtk_session_t *session;
	qtk_engine_t *qform;

#ifndef OFFLINE_TEST
	qtk_recorder_t *recorder;
	qtk_player_t *player;
	wtk_thread_t recorder_thread;
	wtk_thread_t player_thread;
#endif
	wtk_thread_t engine_thread;
	wtk_blockqueue_t *mul_q;
	wtk_blockqueue_t *single_q;
	wtk_strbuf_t *qformbuf;
	wtk_strbuf_t *outbuf;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	wtk_strbuf_t *single_path;
    wtk_resample_t *sample;
	wtk_lock_t mlock;

	wtk_lockhoard_t *buf_q;
	wtk_wavfile_t *mic;
	wtk_wavfile_t *echo;
	void *notify_ths;
	qtk_engine_notify_f notify_func;
	int vad_no;
	int player_rate;

	unsigned int recorder_run:1;
	unsigned int player_run:1;
	unsigned int engine_run:1;
	unsigned int log_audio:1;
};

typedef struct qtk_mqform_msg{
    wtk_queue_node_t node;    
    wtk_strbuf_t *data;
}qtk_mqform_msg_t;

qtk_mqform_t* qtk_mqform_new(qtk_session_t *session,qtk_mqform_cfg_t *cfg);
void qtk_mqform_delete(qtk_mqform_t *w);

int qtk_mqform_start(qtk_mqform_t *w);
int qtk_mqform_stop(qtk_mqform_t *w);
#ifdef OFFLINE_TEST
int qtk_mqform_feed(qtk_mqform_t *w, char *data, int bytes);
#endif

void qtk_mqform_set_notify(qtk_mqform_t *w,void *notify_ths,qtk_engine_notify_f notify_func);

#ifdef __cplusplus
};
#endif
#endif
