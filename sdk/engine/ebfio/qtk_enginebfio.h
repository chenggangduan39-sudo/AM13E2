#ifndef QTK_ENGINE_BFIO_QTK_ENGINEBFIO
#define QTK_ENGINE_BFIO_QTK_ENGINEBFIO

#include "sdk/api_1/bfio/qtk_bfio.h"
#include "sdk/api_1/bfio/qtk_bfio_cfg.h"
#include "sdk/api_1/soundScreen/qtk_soundscreen.h"
#include "sdk/api_1/soundScreen/qtk_soundscreen_cfg.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "sdk/codec/qtk_msg.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/session/qtk_session.h"
#include "sdk/qtk_api.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_enginebfio qtk_enginebfio_t;
#define QTK_BFIO_FEED_STEP (20 * 2 * 16)
struct qtk_enginebfio {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_bfio_cfg_t *bfio_cfg;
    qtk_bfio_t *bfio;
    qtk_soundscreen_cfg_t *soundscreen_cfg;
    qtk_soundscreen_t *soundscreen;
    qtk_vboxebf_cfg_t *vboxebf_cfg;
    qtk_vboxebf_t *vboxebf;
    qtk_msg_t *msg;
    wtk_thread_t qform_t;
    wtk_blockqueue_t bfio_queue;

    void *notify_ths;
    qtk_engine_notify_f notify;
    unsigned feedend : 1;
    unsigned qform_run;
};

qtk_enginebfio_t *qtk_enginebfio_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_enginebfio_delete(qtk_enginebfio_t *e);

int qtk_enginebfio_start(qtk_enginebfio_t *e);
int qtk_enginebfio_feed(qtk_enginebfio_t *e, char *data, int bytes, int is_end);
int qtk_enginebfio_reset(qtk_enginebfio_t *e);

int qtk_enginebfio_cancel(qtk_enginebfio_t *e);
void qtk_enginebfio_set_notify(qtk_enginebfio_t *e, void *ths,
                          qtk_engine_notify_f notify_f);

int qtk_enginebfio_set(qtk_enginebfio_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
