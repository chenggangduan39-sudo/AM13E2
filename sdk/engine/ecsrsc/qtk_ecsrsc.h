
#ifndef __QTK_ENGINE_QCSRSC_QTK_ECSRSC__
#define __QTK_ENGINE_QCSRSC_QTK_ECSRSC__

#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/api_1/csrsc/qtk_csrsc.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ecsrsc qtk_ecsrsc_t;

struct qtk_ecsrsc {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_csrsc_cfg_t *cfg;
    qtk_csrsc_t *csc;
    qtk_engine_notify_f notify_f;
    void *notify_ths;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    int cancel;
    char *enrollfn;
};

qtk_ecsrsc_t *qtk_ecsrsc_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_ecsrsc_delete(qtk_ecsrsc_t *e);

int qtk_ecsrsc_start(qtk_ecsrsc_t *e);
int qtk_ecsrsc_feed(qtk_ecsrsc_t *e, char *data, int bytes, int is_end);
int qtk_ecsrsc_reset(qtk_ecsrsc_t *e);

int qtk_ecsrsc_cancel(qtk_ecsrsc_t *e);
void qtk_ecsrsc_set_notify(qtk_ecsrsc_t *e, void *notify_ths,
                          qtk_engine_notify_f notify_f);

int qtk_ecsrsc_set(qtk_ecsrsc_t *e, char *data, int bytes);
char *qtk_ecsrsc_get_fn(qtk_ecsrsc_t *e);

#ifdef __cplusplus
};
#endif
#endif
