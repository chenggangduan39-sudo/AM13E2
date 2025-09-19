#ifndef QTK_ENGINE_WDEC_QTK_EWAKEUP
#define QTK_ENGINE_WDEC_QTK_EWAKEUP

#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/api_1/wdec/qtk_wdec.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ewdec qtk_ewdec_t;

struct qtk_ewdec {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_wdec_cfg_t *cfg;
    qtk_wdec_t *wdec;
    void *notify_ths;
    qtk_engine_notify_f notify;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    unsigned ended : 1;
};

qtk_ewdec_t *qtk_ewdec_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_ewdec_start(qtk_ewdec_t *e);
int qtk_ewdec_feed(qtk_ewdec_t *e, char *data, int bytes, int is_end);
int qtk_ewdec_reset(qtk_ewdec_t *e);
int qtk_ewdec_delete(qtk_ewdec_t *e);

int qtk_ewdec_cancel(qtk_ewdec_t *e);
void qtk_ewdec_set_notify(qtk_ewdec_t *e, void *notify_ths,
                         qtk_engine_notify_f notify);

int qtk_ewdec_set(qtk_ewdec_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
