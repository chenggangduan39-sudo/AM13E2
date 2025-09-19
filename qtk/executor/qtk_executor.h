#ifndef __EXECUTOR_QBL_EXECUTOR_H__
#define __EXECUTOR_QBL_EXECUTOR_H__
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *upval;
    void (*impl)(void *upval);
} qtk_executor_task_t;

typedef struct {
    int (*start)(void *ths);
    int (*add_task)(void *ths, qtk_executor_task_t task);
    int (*stop)(void *ths);
} qtk_executor_t;

#define qtk_executor_start(e)  (e)->start(e)
#define qtk_executor_stop(e)   (e)->stop(e)
#define qtk_executor_add_task(e, task)   (e)->add_task(e, task)

#ifdef __cplusplus
};
#endif
#endif
