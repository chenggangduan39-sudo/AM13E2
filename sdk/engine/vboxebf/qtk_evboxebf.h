#ifndef __SDK_ENGINE_VBOXEBF_QTK_VBOXEBF__
#define __SDK_ENGINE_VBOXEBF_QTK_VBOXEBF__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf.h"
#include "wtk/core/wtk_strbuf.h"
#include "speex/speex_resampler.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_evboxebf qtk_evboxebf_t;

struct qtk_evboxebf
{
	qtk_session_t *session;
	qtk_vboxebf_t *eqform;
	SpeexResamplerState *oresample;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_vboxebf_cfg_t *cfg;
	wtk_wavfile_t *wav;
	wtk_strbuf_t *outbuf;
	char *outresample;
	int outlen;
	int inlen;
};


qtk_evboxebf_t* qtk_evboxebf_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_evboxebf_delete(qtk_evboxebf_t *e);
int qtk_evboxebf_start(qtk_evboxebf_t *e);
int qtk_evboxebf_feed(qtk_evboxebf_t *e, char *data, int bytes, int is_end);
int qtk_evboxebf_feed2(qtk_evboxebf_t *e, char *input, int in_bytes, char *output, int *out_bytes, int is_end);
int qtk_evboxebf_reset(qtk_evboxebf_t *e);
int qtk_evboxebf_cancel(qtk_evboxebf_t *e);
void qtk_evboxebf_set_notify(qtk_evboxebf_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_evboxebf_set(qtk_evboxebf_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
