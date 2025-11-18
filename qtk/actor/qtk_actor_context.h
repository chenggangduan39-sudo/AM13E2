#ifndef CEE2EC19_70A0_4A6A_A1BD_69B7D0E8D73C
#define CEE2EC19_70A0_4A6A_A1BD_69B7D0E8D73C
#include "qtk/os/qtk_rwlock.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/os/wtk_lock.h"
#include "wtk/os/wtk_lockhoard.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void *qtk_actor_impl_t;
typedef struct qtk_actor_context qtk_actor_context_t;
typedef struct qtk_actor_context_hub qtk_actor_context_hub_t;
typedef struct qtk_actor_msg qtk_actor_msg_t;
typedef int (*qtk_actor_dispatcher_f)(qtk_actor_impl_t upval,
                                      qtk_actor_msg_t *msg,
                                      void **new_dispatcher);
typedef qtk_actor_impl_t (*qtk_actor_new_f)(void *);
typedef void (*qtk_actor_delete_f)(qtk_actor_impl_t);

struct qtk_actor_context {
    char name[128];
    wtk_lock_t lock;
    wtk_queue_t input_q;
    wtk_queue_node_t node;
    qtk_actor_impl_t impl;
    qtk_actor_dispatcher_f dispatcher;
    qtk_actor_new_f creator;
    qtk_actor_delete_f destroyer;
    int ref;
    unsigned scheduled : 1;
};

struct qtk_actor_msg {
    wtk_queue_node_t node;
    wtk_string_t from;
    char *data;
    int len;
    int cap;
};

struct qtk_actor_context_hub {
    wtk_str_hash_t *hash;
    qtk_rwlock_t lock;
    wtk_lockhoard_t hash_node;
};

qtk_actor_context_t *qtk_actor_context_new(void *upval, qtk_actor_new_f creator,
                                           qtk_actor_delete_f destroyer,
                                           qtk_actor_dispatcher_f dispatcher);
void qtk_actor_context_release(qtk_actor_context_t *ctx);
void qtk_actor_context_dispatch(qtk_actor_context_t *ctx,
                                qtk_actor_context_hub_t *hub);

#define qtk_actor_context_grab(ctx) QTK_ATOM_INC(&ctx->ref)

qtk_actor_context_hub_t *qtk_actor_context_hub_new();
void qtk_actor_context_hub_delete(qtk_actor_context_hub_t *hub);
int qtk_actor_context_hub_register(qtk_actor_context_hub_t *hub,
                                   qtk_actor_context_t *ctx, const char *name);
qtk_actor_context_t *qtk_actor_context_hub_retire(qtk_actor_context_hub_t *hub,
                                                  const char *name);
qtk_actor_msg_t *qtk_actor_context_hub_new_msg(qtk_actor_context_hub_t *hub,
                                               const wtk_string_t *from,
                                               int len);
void qtk_actor_context_hub_delete_msg(qtk_actor_context_hub_t *hub,
                                      qtk_actor_msg_t *msg);
qtk_actor_context_t *qtk_actor_context_find(qtk_actor_context_hub_t *hub,
                                            const char *name);

#define qtk_actor_context_hub_new_msg_from_s(hub, from, len)                   \
    qtk_actor_context_hub_new_msg(                                             \
        hub, &(wtk_string_t){cast(char *, from), strlen(from)}, len)

#ifdef __cplusplus
};
#endif
#endif /* CEE2EC19_70A0_4A6A_A1BD_69B7D0E8D73C */