#ifndef QTK_ENGINE_COMM_QTK_ENGINE_THREAD
#define QTK_ENGINE_COMM_QTK_ENGINE_THREAD

#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*qtk_engine_thread_start_f)(void *ths);
typedef int (*qtk_engine_thread_data_f)(void *ths, char *data, int bytes);
typedef int (*qtk_engine_thread_end_f)(void *ths);
typedef void (*qtk_engine_thread_reset_f)(void *ths);
typedef void (*qtk_engine_thread_set_notify_f)(void *ths, void *notify_ths,
                                               qtk_engine_notify_f notify_f);
typedef void (*qtk_engine_thread_cancel_f)(void *ths);
typedef void (*qtk_engine_thread_err_f)(void *ths);
typedef void (*qtk_engine_thread_set_f)(void *ths, char *data, int bytes);

typedef struct {
    void *ths;
    qtk_engine_thread_start_f start_f;
    qtk_engine_thread_data_f data_f;
    qtk_engine_thread_end_f end_f;
    qtk_engine_thread_reset_f reset_f;
    qtk_engine_thread_cancel_f cancel_f;
    qtk_engine_thread_set_notify_f set_notify_f;
    qtk_engine_thread_err_f err_f;
    qtk_engine_thread_set_f set_f;
} qtk_engine_callback_t;

qtk_engine_callback_t *qtk_engine_callback_new();
void qtk_engine_callback_delete(qtk_engine_callback_t *callback);

typedef struct qtk_engine_thread qtk_engine_thread_t;

typedef enum {
    QTK_ENGINE_THREAD_START,
    QTK_ENGINE_THREAD_DATA,
    QTK_ENGINE_THREAD_END,
    QTK_ENGINE_THREAD_RESET,
    QTK_ENGINE_THREAD_CANCEL,
    QTK_ENGINE_THREAD_SET_NOTIFY,
    QTK_ENGINE_THREAD_SET,
} qtk_engine_thread_type_t;

typedef struct {
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    qtk_engine_thread_type_t type;
    wtk_strbuf_t *buf;
} qtk_engine_thread_msg_t;

struct qtk_engine_thread {
    wtk_log_t *log;
    qtk_engine_callback_t *callback;
    wtk_strbuf_t *buf;
    wtk_thread_t thread;
    wtk_blockqueue_t input_q;
    wtk_lockhoard_t msg_hoard;
    wtk_sem_t sem;
    int buf_size;
    int cache;
    int cancel;
    unsigned run : 1;
    unsigned use_step : 1;
    unsigned syn : 1;
    unsigned is_released : 1;
};

qtk_engine_thread_t *qtk_engine_thread_new(qtk_engine_callback_t *callback,
                                           wtk_log_t *log, char *name,
                                           int buf_size, int cache,
                                           int use_step, int syn);
void qtk_engine_thread_delete(qtk_engine_thread_t *t, int syn);

void qtk_engine_thread_start(qtk_engine_thread_t *t);
void qtk_engine_thread_feed(qtk_engine_thread_t *t, char *data, int bytes,
                            int is_end);
void qtk_engine_thread_reset(qtk_engine_thread_t *t);
void qtk_engine_thread_cancel(qtk_engine_thread_t *t);
void qtk_engine_thread_set_notify(qtk_engine_thread_t *t, void *notify_ths,
                                  qtk_engine_notify_f notify_f);
void qtk_engine_thread_set(qtk_engine_thread_t *t, char *data, int bytes);

void qtk_engine_thread_set_str(qtk_engine_thread_t *t,
                               qtk_engine_thread_type_t type, char *data,
                               int bytes);
void qtk_engine_thread_set_num(qtk_engine_thread_t *t,
                               qtk_engine_thread_type_t type, double num);
void qtk_engine_thread_set_int(qtk_engine_thread_t *t,
                               qtk_engine_thread_type_t type, int i);

#ifdef __cplusplus
};
#endif
#endif
