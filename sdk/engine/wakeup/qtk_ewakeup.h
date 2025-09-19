#ifndef QTK_ENGINE_WAKEUP_QTK_EWAKEUP
#define QTK_ENGINE_WAKEUP_QTK_EWAKEUP

#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/api_1/wakeup/qtk_wakeup.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ewakeup qtk_ewakeup_t;

struct qtk_ewakeup {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_wakeup_cfg_t *cfg;
    qtk_wakeup_t *t;
    void *notify_ths;
    qtk_engine_notify_f notify;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    unsigned ended : 1;
};

qtk_ewakeup_t *qtk_ewakeup_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_ewakeup_start(qtk_ewakeup_t *e);
int qtk_ewakeup_feed(qtk_ewakeup_t *e, char *data, int bytes, int is_end);
int qtk_ewakeup_reset(qtk_ewakeup_t *e);
int qtk_ewakeup_delete(qtk_ewakeup_t *e);

int qtk_ewakeup_cancel(qtk_ewakeup_t *e);
void qtk_ewakeup_set_notify(qtk_ewakeup_t *e, void *notify_ths,
                         qtk_engine_notify_f notify);

int qtk_ewakeup_set(qtk_ewakeup_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
