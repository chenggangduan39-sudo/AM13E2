#ifndef CE1F6715_59BF_4148_9E40_BFEE0B23F6BB
#define CE1F6715_59BF_4148_9E40_BFEE0B23F6BB
#include "qtk/actor/qtk_actor_context.h"
#include "qtk/os/qtk_condqueue.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/os/wtk_thread.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_actor_scheduler qtk_actor_scheduler_t;

struct qtk_actor_scheduler {
    qtk_actor_context_hub_t *hub;
    qtk_condqueue_t ctxs;
    int nworker;
    wtk_thread_t *workers;
    unsigned run : 1;
};

int qtk_actor_scheduler_send(const char *from, const char *to, char *msg,
                             int len);
void qtk_actor_scheduler_start();
void qtk_actor_scheduler_stop();

int qtk_actor_context_register(const char *name, void *upval,
                               qtk_actor_new_f creator,
                               qtk_actor_delete_f destroyer,
                               qtk_actor_dispatcher_f dispatcher);

int qtk_actor_context_retire(const char *name);
int qtk_actor_scheduler_new(int nworker);
void qtk_actor_scheduler_delete();

#ifdef __cplusplus
};
#endif
#endif /* CE1F6715_59BF_4148_9E40_BFEE0B23F6BB */