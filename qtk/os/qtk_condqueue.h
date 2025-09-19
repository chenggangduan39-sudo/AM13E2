#ifndef __QBL_OS_QBL_CONDQUEUE_H__
#define __QBL_OS_QBL_CONDQUEUE_H__
#pragma once
#include "qtk/os/qtk_cond.h"
#include "wtk/os/wtk_lock.h"
#include "wtk/core/wtk_queue.h"
#include "qtk/core/qtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    wtk_queue_t queue;
    qtk_cond_t cond;
    wtk_lock_t lock;
} qtk_condqueue_t;

static QTK_INLINE void qtk_condqueue_init(qtk_condqueue_t *q) {
    wtk_queue_init(&q->queue);
    qtk_cond_init(&q->cond);
    wtk_lock_init(&q->lock);
}

static QTK_INLINE void qtk_condqueue_clean(qtk_condqueue_t *q) {
    qtk_cond_clean(&q->cond);
    wtk_lock_clean(&q->lock);
}

static QTK_INLINE int qtk_condqueue_push(qtk_condqueue_t *q, wtk_queue_node_t *node) {
    wtk_lock_lock(&q->lock);
    wtk_queue_push(&q->queue, node);
    qtk_cond_wakeup(&q->cond);
    wtk_lock_unlock(&q->lock);
    return 0;
}

static QTK_INLINE int qtk_condqueue_wake(qtk_condqueue_t *q) {
    wtk_lock_lock(&q->lock);
    qtk_cond_wakeup_all(&q->cond);
    wtk_lock_unlock(&q->lock);
    return 0;
}

#ifdef __cplusplus
};
#endif
#endif
