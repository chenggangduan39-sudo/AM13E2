#include "qtk/actor/qtk_actor_scheduler.h"
#include "qtk/actor/qtk_actor_context.h"
#include "qtk/os/qtk_condqueue.h"
#include "wtk/os/wtk_lock.h"
#include "wtk/os/wtk_thread.h"

static qtk_actor_scheduler_t *sch = NULL;

static void *worker_routine_(qtk_actor_scheduler_t *sch) {
    wtk_queue_node_t *node;
    sch->run = 1;
    wtk_lock_lock(&sch->ctxs.lock);
    while (1) {
        node = wtk_queue_pop(&sch->ctxs.queue);
        if (node) {
            wtk_lock_unlock(&sch->ctxs.lock);
            qtk_actor_context_dispatch(
                data_offset2(node, qtk_actor_context_t, node), sch->hub);
            wtk_lock_lock(&sch->ctxs.lock);
        } else {
            if (!sch->run) {
                break;
            }
            qtk_cond_wait(&sch->ctxs.cond, &sch->ctxs.lock);
        }
    }
    wtk_lock_unlock(&sch->ctxs.lock);
    return NULL;
}

int qtk_actor_scheduler_new(int nworker) {
    sch = wtk_malloc(sizeof(qtk_actor_scheduler_t));
    qtk_condqueue_init(&sch->ctxs);
    sch->run = 0;
    sch->nworker = nworker;
    sch->workers = wtk_malloc(sizeof(wtk_thread_t) * nworker);
    sch->hub = qtk_actor_context_hub_new();
    for (int i = 0; i < nworker; i++) {
        wtk_thread_init(sch->workers + i,
                        cast(thread_route_handler, worker_routine_), sch);
    }
    return 0;
}

void qtk_actor_scheduler_delete() {
    qtk_actor_context_hub_delete(sch->hub);
    wtk_free(sch->workers);
    wtk_free(sch);
}

void qtk_actor_scheduler_start() {
    for (int i = 0; i < sch->nworker; i++) {
        wtk_thread_start(sch->workers + i);
    }
}

void qtk_actor_scheduler_stop() {
    sch->run = 0;
    qtk_condqueue_wake(&sch->ctxs);
    for (int i = 0; i < sch->nworker; i++) {
        wtk_thread_join(sch->workers + i);
    }
}

int qtk_actor_scheduler_send(const char *from, const char *to, char *msg,
                             int len) {
    qtk_actor_context_t *dst;
    qtk_actor_msg_t *m;
    int scheduled;

    dst = qtk_actor_context_find(sch->hub, to);
    if (dst == NULL) {
        return -1;
    }

    m = qtk_actor_context_hub_new_msg_from_s(sch->hub, from, len);
    if (len > 0) {
        memcpy(m->data, msg, len);
    }

    wtk_lock_lock(&dst->lock);
    scheduled = dst->scheduled;
    wtk_queue_push(&dst->input_q, &m->node);
    if (scheduled == 0) {
        dst->scheduled = 1;
        qtk_condqueue_push(&sch->ctxs, &dst->node);
    } else {
        qtk_actor_context_release(dst);
    }
    wtk_lock_unlock(&dst->lock);

    return 0;
}

int qtk_actor_context_register(const char *name, void *upval,
                               qtk_actor_new_f creator,
                               qtk_actor_delete_f destroyer,
                               qtk_actor_dispatcher_f dispatcher) {
    qtk_actor_context_t *ctx =
        qtk_actor_context_new(upval, creator, destroyer, dispatcher);
    return qtk_actor_context_hub_register(sch->hub, ctx, name);
}

int qtk_actor_context_retire(const char *name) {
    qtk_actor_context_t *ctx;
    ctx = qtk_actor_context_hub_retire(sch->hub, name);
    if (ctx == NULL) {
        return -1;
    }
    qtk_actor_context_release(ctx);
    return 0;
}
