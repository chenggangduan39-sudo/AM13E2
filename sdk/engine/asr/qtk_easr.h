#ifndef QTK_ASR_QTK_EASR
#define QTK_ASR_QTK_EASR

#include "wtk/core/cfg/wtk_cfg_file.h"

#include "sdk/api_1/asr/qtk_asr.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/engine/qtk_engine.h"
#include "sdk/qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_easr qtk_easr_t;

#ifdef USE_ASR_CUMULA
#define ASR_CUMULA_MAX_COUNT 3
#define qtk_easr_is_cumula(x) (x) >= ASR_CUMULA_MAX_COUNT

typedef enum { QTK_EASR_START, QTK_EASR_DATA, QTK_EASR_END } qtk_easr_type_t;

typedef struct {
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    qtk_easr_type_t type;
    wtk_string_t *v;
} qtk_easr_msg_t;
#endif

struct qtk_easr {
    qtk_engine_param_t param;
    qtk_session_t *session;
    qtk_asr_cfg_t *cfg;
    qtk_asr_t *c;
    qtk_engine_thread_t *thread;
    qtk_engine_callback_t *callback;
    qtk_engine_notify_f notify_f;
    void *notify_ths;
#ifdef USE_ASR_CUMULA
    wtk_blockqueue_t cache_q;
    wtk_lockhoard_t msg_hoard;
    int cumula;
#endif
    int asr_min_bytes;
    int asr_max_bytes;
    int asr_bytes;
    int cancel;
    double delay_tm;
    unsigned ended : 1;
};

qtk_easr_t *qtk_easr_new(qtk_session_t *s, wtk_local_cfg_t *params);
int qtk_easr_delete(qtk_easr_t *e);

int qtk_easr_start(qtk_easr_t *e);
int qtk_easr_feed(qtk_easr_t *e, char *data, int bytes, int is_end);
int qtk_easr_reset(qtk_easr_t *e);

int qtk_easr_cancel(qtk_easr_t *e);
void qtk_easr_set_notify(qtk_easr_t *e, void *notify_ths,
                         qtk_engine_notify_f notify_f);

int qtk_easr_set(qtk_easr_t *e, char *data, int bytes);
int qtk_easr_set_xbnf(qtk_easr_t *e, char *data, int len);
int qtk_easr_update_cmds(qtk_easr_t* e,char* words,int len);
void qtk_easr_get_result(qtk_easr_t *e, qtk_var_t *var);
#ifdef __cplusplus
};
#endif
#endif
