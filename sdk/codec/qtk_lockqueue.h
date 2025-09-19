#ifndef QTK_CORE_QTK_LOCKQUEUE_H_
#define QTK_CORE_QTK_LOCKQUEUE_H_
#include "qtk_queue.h"
#include "qtk_lock.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_lockqueue qtk_lockqueue_t;

#define QTK_LOCK_QUEUE \
	QTK_QUEUE \
	qtk_lock_t l;

struct qtk_lockqueue
{
	QTK_LOCK_QUEUE
};

int qtk_lockqueue_init(qtk_lockqueue_t *q);
int qtk_lockqueue_clean(qtk_lockqueue_t *q);
int qtk_lockqueue_push(qtk_lockqueue_t *q,qtk_queue_node_t *n);
int qtk_lockqueue_push_front(qtk_lockqueue_t *q,qtk_queue_node_t *n);
qtk_queue_node_t* qtk_lockqueue_pop(qtk_lockqueue_t *q);
#ifdef __cplusplus
};
#endif
#endif
