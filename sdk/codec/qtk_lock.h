#ifndef QTK_OS_QTK_LOCK_H_
#define QTK_OS_QTK_LOCK_H_

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef pthread_mutex_t qtk_lock_t;
#define qtk_lock_init(l) pthread_mutex_init(l,0)
#define qtk_lock_clean(l) pthread_mutex_destroy(l)
#define qtk_lock_lock(l) pthread_mutex_lock(l)
#define qtk_lock_unlock(l) pthread_mutex_unlock(l)
#define qtk_lock_trylock(l) pthread_mutex_trylock(l)

#ifdef __cplusplus
};
#endif
#endif
