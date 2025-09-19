#include "qtk/executor/qtk_thread_pool_executor.h"
#include "qtk/core/qtk_type.h"
#include "wtk/core/wtk_alloc.h"
#include "qtk/os/qtk_condqueue.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lock.h"

typedef struct {
    qtk_executor_t interface;
    int pool_sz;
    wtk_thread_t *workers;
    qtk_condqueue_t tasks;
    wtk_lock_t tasks_pool_guard;
    wtk_vpool_t *tasks_pool;
    unsigned run : 1;
} _thread_pool_executor_t;

typedef struct {
    qtk_executor_task_t task;
    wtk_queue_node_t node;
} _task_container_t;

static int _executor_start(_thread_pool_executor_t *e) {
    int i;

    e->run = 1;
    for (i = 0; i < e->pool_sz; i++)  {
        wtk_thread_start(e->workers + i);
    }
    return 0;
}

static int _executor_stop(_thread_pool_executor_t *e) {
    int i;

    e->run = 0;
    qtk_condqueue_wake(&e->tasks);
    for (i = 0; i < e->pool_sz; i++)  {
        wtk_thread_join(e->workers + i);
    }
    return 0;
}

static int _executor_add_task(_thread_pool_executor_t *e, qtk_executor_task_t task) {
    _task_container_t *tc;

    wtk_lock_lock(&e->tasks_pool_guard);
    tc = wtk_vpool_pop(e->tasks_pool);
    wtk_lock_unlock(&e->tasks_pool_guard);
    tc->task = task;
    qtk_condqueue_push(&e->tasks, &tc->node);
    return 0;
}

static void _thread_pool_executor_set_interface(_thread_pool_executor_t *e) {
    e->interface.start = cast(int (*)(void *), _executor_start);
    e->interface.stop = cast(int (*)(void *), _executor_stop);
    e->interface.add_task = cast(int (*)(void *, qtk_executor_task_t task), _executor_add_task);
}

static void *_worker_routine(_thread_pool_executor_t *e, wtk_thread_t *th) {
    wtk_queue_node_t *node;
    _task_container_t *tc;

    wtk_lock_lock(&e->tasks.lock);
    while (1) {
        node = wtk_queue_pop(&e->tasks.queue);
        if (node) {
            wtk_lock_unlock(&e->tasks.lock);
            tc = data_offset2(node, _task_container_t, node);
            tc->task.impl(tc->task.upval);

            wtk_lock_lock(&e->tasks_pool_guard);
            wtk_vpool_push(e->tasks_pool, tc);
            wtk_lock_unlock(&e->tasks_pool_guard);

            wtk_lock_lock(&e->tasks.lock);
        } else {
            if (!e->run) {
                break;
            }
            qtk_cond_wait(&e->tasks.cond, &e->tasks.lock);
        }
    }
    wtk_lock_unlock(&e->tasks.lock);

    return NULL;
}

qtk_executor_t *qtk_thread_pool_executor_new(int pool_sz) {
    int i;
    _thread_pool_executor_t *e = wtk_malloc(sizeof(_thread_pool_executor_t) + pool_sz * sizeof(wtk_thread_t));
    e->pool_sz = pool_sz;
    e->workers = cast(wtk_thread_t *, e + 1);
    for (i = 0; i < e->pool_sz; i++) {
        wtk_thread_init(e->workers + i, cast(thread_route_handler, _worker_routine), e);
    }
    _thread_pool_executor_set_interface(e);
    qtk_condqueue_init(&e->tasks);
    wtk_lock_init(&e->tasks_pool_guard);
    e->tasks_pool = wtk_vpool_new(sizeof(_task_container_t), pool_sz);
    e->run = 0;
    return &e->interface;
}

void qtk_thread_pool_executor_delete(qtk_executor_t *e) {
    _thread_pool_executor_t *impl = cast(_thread_pool_executor_t *, e);
    qtk_condqueue_clean(&impl->tasks);
    wtk_lock_clean(&impl->tasks_pool_guard);
    wtk_vpool_delete(impl->tasks_pool);
    wtk_free(impl);
}
