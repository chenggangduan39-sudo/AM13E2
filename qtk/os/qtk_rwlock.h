#ifndef AB8046E0_589B_42AD_B478_3900D8DBC2F3
#define AB8046E0_589B_42AD_B478_3900D8DBC2F3
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_rwlock qtk_rwlock_t;

#include <pthread.h>

struct qtk_rwlock {
    pthread_rwlock_t lock;
};

static inline void qtk_rwlock_init(struct qtk_rwlock *lock) {
    pthread_rwlock_init(&lock->lock, NULL);
}

static inline void qtk_rwlock_rlock(struct qtk_rwlock *lock) {
    pthread_rwlock_rdlock(&lock->lock);
}

static inline void qtk_rwlock_wlock(struct qtk_rwlock *lock) {
    pthread_rwlock_wrlock(&lock->lock);
}

static inline void qtk_rwlock_wunlock(struct qtk_rwlock *lock) {
    pthread_rwlock_unlock(&lock->lock);
}

static inline void qtk_rwlock_runlock(struct qtk_rwlock *lock) {
    pthread_rwlock_unlock(&lock->lock);
}

#ifdef __cplusplus
};
#endif
#endif /* AB8046E0_589B_42AD_B478_3900D8DBC2F3 */