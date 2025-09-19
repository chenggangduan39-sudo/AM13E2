#ifndef QTK_ENGINE_BFIO_QTK_EBFIO
#define QTK_ENGINE_BFIO_QTK_EBFIO

#include "wtk/bfio/wtk_bfio.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/session/qtk_session.h"
#include "sdk/qtk_api.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_ebfio qtk_ebfio_t;
#define QTK_BFIO_FEED_STEP (20 * 2 * 16)
struct qtk_ebfio {
    qtk_session_t *session;
    qtk_engine_param_t param;
    wtk_bfio_cfg_t *cfg;
    wtk_bfio_t *b;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    void *notify_ths;
    qtk_engine_notify_f notify;
    short **buffer;
    int channel;
    unsigned feedend : 1;
};

qtk_ebfio_t *qtk_ebfio_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_ebfio_delete(qtk_ebfio_t *e);

int qtk_ebfio_start(qtk_ebfio_t *e);
int qtk_ebfio_feed(qtk_ebfio_t *e, char *data, int bytes, int is_end);
int qtk_ebfio_reset(qtk_ebfio_t *e);

int qtk_ebfio_cancel(qtk_ebfio_t *e);
void qtk_ebfio_set_notify(qtk_ebfio_t *e, void *ths,
                          qtk_engine_notify_f notify_f);

int qtk_ebfio_set(qtk_ebfio_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
