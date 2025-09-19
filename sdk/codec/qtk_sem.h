#ifndef QTK_OS_QTK_SEM_H_
#define QTK_OS_QTK_SEM_H_

#include <semaphore.h>
#include "qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef sem_t qtk_sem_t;
#define qtk_sem_init(s,c) sem_init(s,0,c)
#define qtk_sem_clean(s) sem_destroy(s)
#define qtk_sem_tryacquire(s) sem_trywait(s)
#define qtk_sem_inc(s) sem_post(s)

DLL_API int qtk_sem_acquire(qtk_sem_t *s,int ms);
DLL_API int qtk_sem_release(qtk_sem_t *s, int count);
DLL_API int qtk_sem_drain(qtk_sem_t *s);
#ifdef __cplusplus
};
#endif
#endif
