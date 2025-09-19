#ifndef __QBL_OS_QBL_COND_H__
#define __QBL_OS_QBL_COND_H__
#pragma once
#include <pthread.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef pthread_cond_t qtk_cond_t;

#define qtk_cond_init(cond_ptr) pthread_cond_init(cond_ptr, NULL)
#define qtk_cond_clean(cond_ptr) pthread_cond_destroy(cond_ptr)
#define qtk_cond_wakeup(cond_ptr) pthread_cond_signal(cond_ptr)
#define qtk_cond_wakeup_all(cond_ptr) pthread_cond_broadcast(cond_ptr)

#define qtk_cond_wait(cond_ptr, lock_ptr) pthread_cond_wait(cond_ptr, lock_ptr)

#ifdef __cplusplus
};
#endif
#endif
