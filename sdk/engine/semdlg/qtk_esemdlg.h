#ifndef QTK_ENGINE_SEMDLG_QTK_ESEMDLG
#define QTK_ENGINE_SEMDLG_QTK_ESEMDLG

#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/api_1/semdlg/qtk_semdlg.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif

#define QTK_ESEMDLG_ACTIVE "<active>"
#define QTK_ESEMDLG_IDLE "<idle>"

typedef struct qtk_esemdlg qtk_esemdlg_t;

typedef enum {
    QTK_ESEMDLG_TXT,
    QTK_ESEMDLG_SET,
    QTK_ESEMDLG_NOTIFY,
} qtk_esemdlg_type_t;

typedef struct {
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    qtk_esemdlg_type_t type;
    wtk_strbuf_t *buf;
} qtk_esemdlg_msg_t;

struct qtk_esemdlg {
    qtk_session_t *session;
    qtk_engine_param_t param;
    qtk_semdlg_cfg_t *cfg;
    qtk_semdlg_t *s;
    qtk_engine_notify_f notify_f;
    void *notify_ths;
    wtk_model_t *model;
    wtk_model_item_t *semdlg_item;
    wtk_blockqueue_t input_q;
    wtk_lockhoard_t msg_hoard;
    wtk_thread_t thread;
    wtk_sem_t start_sem;
    int loop_time;
    unsigned run : 1;
    unsigned init : 1;
};

qtk_esemdlg_t *qtk_esemdlg_new(qtk_session_t *session, wtk_local_cfg_t *params);
int qtk_esemdlg_delete(qtk_esemdlg_t *e);
int qtk_esemdlg_start(qtk_esemdlg_t *e);
int qtk_esemdlg_feed(qtk_esemdlg_t *e, char *data, int bytes, int is_end);
int qtk_esemdlg_reset(qtk_esemdlg_t *e);
void qtk_esemdlg_set_notify(qtk_esemdlg_t *e, void *notify_ths,
                            qtk_engine_notify_f notify_f);
int qtk_esemdlg_set(qtk_esemdlg_t *e, char *data, int bytes);

#ifdef __cplusplus
};
#endif
#endif
