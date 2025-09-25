#ifndef QTK_ENGINE_CSR_QTK_ECSR
#define QTK_ENGINE_CSR_QTK_ECSR

#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_os.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/api_1/csr/qtk_csr.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ecsr qtk_ecsr_t;

struct qtk_ecsr {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_csr_cfg_t *cfg;
    qtk_csr_t *c;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    qtk_engine_notify_f notify_func;
    void *notify_ths;
    int cancel;
    unsigned end_flag : 1;
};

qtk_ecsr_t *qtk_ecsr_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_ecsr_delete(qtk_ecsr_t *e);

int qtk_ecsr_start(qtk_ecsr_t *e);
int qtk_ecsr_reset(qtk_ecsr_t *e);
int qtk_ecsr_feed(qtk_ecsr_t *e, char *data, int len, int is_end);

int qtk_ecsr_cancel(qtk_ecsr_t *e);
void qtk_ecsr_set_notify(qtk_ecsr_t *e, void *ths, qtk_engine_notify_f notify);

int qtk_ecsr_set(qtk_ecsr_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
