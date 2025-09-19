#ifndef __EXECUTOR_QBL_THREAD_POOL_EXECUTOR_H__
#define __EXECUTOR_QBL_THREAD_POOL_EXECUTOR_H__
#pragma once
#include "qtk/executor/qtk_executor.h"
#ifdef __cplusplus
extern "C" {
#endif

qtk_executor_t *qtk_thread_pool_executor_new(int pool_sz);
void qtk_thread_pool_executor_delete(qtk_executor_t *e);

#ifdef __cplusplus
};
#endif
#endif
