#ifndef QTK_API_1_ASR_QTK_IASR_ROUTE
#define QTK_API_1_ASR_QTK_IASR_ROUTE

#include "wtk/os/wtk_lockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#include "qtk_iasr.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_iasr_route qtk_iasr_route_t;

typedef struct {
    qtk_iasr_t *iasr;
    wtk_string_t rec;
    int priority;
    int serial;
    unsigned wait : 1;
} qtk_ir_notify_t;

typedef enum {
    QTK_IR_DATA,
    QTK_IR_ERR,
    QTK_IR_NOTHING,
} qtk_ir_state_t;

typedef void (*qtk_iasr_route_notify_f)(void *ths, qtk_ir_state_t state,
                                        qtk_ir_notify_t *notify);

typedef enum {
    QTK_IR_MSG_START,
    QTK_IR_MSG_DATA,
    QTK_IR_MSG_END,
    QTK_IR_MSG_RESET,
    QTK_IR_MSG_CANCEL,
} qtk_ir_type_t;

typedef struct {
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    qtk_ir_type_t type;
    wtk_sem_t *sem;
    wtk_strbuf_t *buf;
} qtk_ir_msg_t;

typedef enum {
    QTK_IR_RUN_INIT,
    QTK_IR_RUN_PROCESS,
    QTK_IR_RUN_END,
} qtk_ir_run_state_t;

struct qtk_iasr_route {
    qtk_session_t *session;
    qtk_iasr_t *iasr;
    qtk_iasr_route_notify_f notify;
    void *notify_ths;
    wtk_blockqueue_t input_q;
    wtk_lockhoard_t msg_hoard;
    wtk_thread_t thread;
    qtk_ir_run_state_t state;
    int serial;
    unsigned run : 1;
    unsigned cancel : 1;
};

qtk_iasr_route_t *qtk_iasr_route_new(qtk_iasr_t *iasr, qtk_session_t *session,
                                     int serial);
void qtk_iasr_route_delete(qtk_iasr_route_t *route);
void qtk_iasr_route_set_notify(qtk_iasr_route_t *route, void *notify_ths,
                               qtk_iasr_route_notify_f notify);

void qtk_iasr_route_start(qtk_iasr_route_t *route);
void qtk_iasr_route_feed(qtk_iasr_route_t *route, char *data, int bytes,
                         int is_end);
void qtk_iasr_route_reset(qtk_iasr_route_t *route);
void qtk_iasr_route_cancel(qtk_iasr_route_t *route);

#ifdef __cplusplus
};
#endif
#endif
