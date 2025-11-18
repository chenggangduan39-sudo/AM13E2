#include "qtk_random.h"
#include "wtk/os/wtk_lock.h"

#define QTK_RANDOM_MAX_LEN 4096

wtk_lock_t random_global_lock = PTHREAD_MUTEX_INITIALIZER;


qtk_random_t* qtk_random_new(int seed)
{
    qtk_random_t *r = NULL;
    r = wtk_malloc(sizeof(*r));
    r->i = 0;
    r->n = QTK_RANDOM_MAX_LEN;
    r->s = seed;
    r->nums = wtk_malloc(sizeof(int)*QTK_RANDOM_MAX_LEN);
    int i = 0;
    wtk_lock_lock(&random_global_lock);
    srand(seed);
    for(i = 0; i < QTK_RANDOM_MAX_LEN; ++i){
        r->nums[i] = rand();
    }
    wtk_lock_unlock(&random_global_lock);

    return r;
}

int qtk_random_rand(qtk_random_t* rand)
{
    return rand->nums[(rand->i++)%rand->n];
}

void qtk_random_reset(qtk_random_t *rand)
{
    rand->i = 0;
}

int qtk_random_delete(qtk_random_t *rand)
{
    wtk_free(rand->nums);
    wtk_free(rand);
    return 0;
}

