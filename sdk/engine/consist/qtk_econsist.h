#ifndef __SDK_ENGINE_BFIO_QTK_ECONSIST__
#define __SDK_ENGINE_BFIO_QTK_ECONSIST__

#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/bfio/consist/wtk_consist.h"
#include "wtk/core/wtk_wavfile.h"

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_econsist qtk_econsist_t;
#define QTK_BFIO_FEED_STEP (20*2*16)
struct qtk_econsist
{
	qtk_session_t *session;
	qtk_engine_param_t param;
	wtk_consist_cfg_t *cfg;
	wtk_consist_t *cs;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	wtk_strbuf_t *tmpbuf;
	wtk_strbuf_t *rbuf;
	wtk_wavfile_t *swav;
	void *notify_ths;
	qtk_engine_notify_f notify;
	short **buffer;
	int channel;
	unsigned feedend:1;
	unsigned is_pcm_start:1;
};

qtk_econsist_t* qtk_econsist_new(qtk_session_t *session,wtk_local_cfg_t *params);
int qtk_econsist_delete(qtk_econsist_t *e);

int qtk_econsist_start(qtk_econsist_t *e);
int qtk_econsist_feed(qtk_econsist_t *e,char *data,int bytes,int is_end);
int qtk_econsist_reset(qtk_econsist_t *e);

int qtk_econsist_cancel(qtk_econsist_t *e);
void qtk_econsist_set_notify(qtk_econsist_t *e,void *ths,qtk_engine_notify_f notify_f);

int qtk_econsist_set(qtk_econsist_t *e,char *data,int bytes);

#ifdef __cplusplus
};
#endif
#endif
