#ifndef QTK_CORE_QTK_BLOCKQUEUE_H_
#define QTK_CORE_QTK_BLOCKQUEUE_H_
#include "qtk_queue.h"
#include "qtk_lockqueue.h"
#include "qtk_sem.h"
#include "qtk_api.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_blockqueue qtk_blockqueue_t;

struct qtk_blockqueue
{
	QTK_LOCK_QUEUE
	qtk_sem_t sem;
};

DLL_API int qtk_blockqueue_init(qtk_blockqueue_t *q);
DLL_API int qtk_blockqueue_clean(qtk_blockqueue_t *q);
DLL_API int qtk_blockqueue_push(qtk_blockqueue_t *q,qtk_queue_node_t *n);
DLL_API qtk_queue_node_t* qtk_blockqueue_pop(qtk_blockqueue_t *q,int ms,int *is_timeout);
DLL_API int qtk_blockqueue_wake(qtk_blockqueue_t *q);
DLL_API int qtk_blockqueue_push_front(qtk_blockqueue_t *q,qtk_queue_node_t *n);
#ifdef __cplusplus
};
#endif
#endif
