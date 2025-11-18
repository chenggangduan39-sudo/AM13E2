#ifndef __SDK_ENGINE_GAINNET_BF_QTK_GAINNETBF__
#define __SDK_ENGINE_GAINNET_BF_QTK_GAINNETBF__

#include "qtk_api.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/api_1/gainnetbf/qtk_gainnetbf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_egainnetbf qtk_egainnetbf_t;

struct qtk_egainnetbf
{
	qtk_session_t *session;
	qtk_gainnetbf_t *eqform;
	qtk_engine_param_t param;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
	void *notify_ths;
	qtk_engine_notify_f notify;
	qtk_gainnetbf_cfg_t *cfg;
};


qtk_egainnetbf_t* qtk_egainnetbf_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_egainnetbf_delete(qtk_egainnetbf_t *e);
int qtk_egainnetbf_start(qtk_egainnetbf_t *e);
int qtk_egainnetbf_feed(qtk_egainnetbf_t *e, char *data, int bytes, int is_end);
int qtk_egainnetbf_reset(qtk_egainnetbf_t *e);
int qtk_egainnetbf_cancel(qtk_egainnetbf_t *e);
void qtk_egainnetbf_set_notify(qtk_egainnetbf_t *e, void *ths, qtk_engine_notify_f notify_f);
int qtk_egainnetbf_set(qtk_egainnetbf_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
