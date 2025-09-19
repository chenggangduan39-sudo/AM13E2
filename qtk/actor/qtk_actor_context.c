#include "qtk/actor/qtk_actor_context.h"
#include "qtk/os/qtk_atomic.h"
#include "qtk/os/qtk_rwlock.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/wtk_type.h"
#include "wtk/os/wtk_lock.h"
#include "wtk/os/wtk_lockhoard.h"

typedef struct {
    hash_str_node_t node;
} hub_node_t;

qtk_actor_context_t *qtk_actor_context_new(void *upval, qtk_actor_new_f creator,
                                           qtk_actor_delete_f destroyer,
                                           qtk_actor_dispatcher_f dispatcher) {
    qtk_actor_context_t *ctx = wtk_malloc(sizeof(qtk_actor_context_t));
    wtk_lock_init(&ctx->lock);
    wtk_queue_init(&ctx->input_q);
    ctx->scheduled = 0;
    ctx->creator = creator;
    ctx->destroyer = destroyer;
    ctx->impl = creator ? creator(upval) : NULL;
    ctx->dispatcher = dispatcher;
    ctx->ref = 1;
    return ctx;
}

static void qtk_actor_context_delete(qtk_actor_context_t *ctx) {
    wtk_lock_clean(&ctx->lock);
    if (ctx->destroyer) {
        ctx->destroyer(ctx->impl);
    }
    wtk_free(ctx);
}

void qtk_actor_context_release(qtk_actor_context_t *ctx) {
    if (QTK_ATOM_DEC(&ctx->ref) == 0) {
        qtk_actor_context_delete(ctx);
    }
}

void qtk_actor_context_dispatch(qtk_actor_context_t *ctx,
                                qtk_actor_context_hub_t *hub) {
    wtk_queue_node_t *node;
    int ctx_release = 0;
    qtk_actor_msg_t *msg;
    while (1) {
        wtk_lock_lock(&ctx->lock);
        node = wtk_queue_pop(&ctx->input_q);
        if (node == NULL) {
            ctx->scheduled = 0;
            wtk_lock_unlock(&ctx->lock);
            break;
        }
        wtk_lock_unlock(&ctx->lock);
        msg = data_offset2(node, qtk_actor_msg_t, node);
        if (ctx->dispatcher(ctx->impl, msg, cast(void **, &ctx->dispatcher))) {
            ctx_release = 1;
        }
        qtk_actor_context_hub_delete_msg(hub, msg);
    }

    qtk_actor_context_release(ctx);

    if (ctx_release) {
        qtk_actor_context_hub_retire(hub, ctx->name);
        qtk_actor_context_release(ctx);
    }
}

void *hash_node_new_(void *arg) {
    hash_str_node_t *node = wtk_malloc(sizeof(hub_node_t));
    return node;
}

int hash_node_delete_(void *data) {
    hub_node_t *node = cast(hub_node_t *, data);
    wtk_free(node);
    return 0;
}

qtk_actor_context_hub_t *qtk_actor_context_hub_new() {
    qtk_actor_context_hub_t *hub;
    hub = wtk_malloc(sizeof(qtk_actor_context_hub_t));
    hub->hash = wtk_str_hash_new(10);
    qtk_rwlock_init(&hub->lock);
    wtk_lockhoard_init(&hub->hash_node, offsetof(hash_str_node_t, n), 10,
                       hash_node_new_, hash_node_delete_, NULL);
    return hub;
}

void qtk_actor_context_hub_delete(qtk_actor_context_hub_t *hub) {
    wtk_str_hash_delete(hub->hash);
    wtk_lockhoard_clean(&hub->hash_node);
    wtk_free(hub);
}

int qtk_actor_context_hub_register(qtk_actor_context_hub_t *hub,
                                   qtk_actor_context_t *ctx, const char *name) {
    hub_node_t *hash_node;
    int name_len = strlen(name);
    if (name_len >= sizeof(cast(qtk_actor_context_t *, 0))->name) {
        return -1;
    }
    hash_node = wtk_lockhoard_pop(&hub->hash_node);
    memcpy(ctx->name, name, name_len);
    ctx->name[name_len] = 0;
    qtk_rwlock_wlock(&hub->lock);
    wtk_str_hash_add_node(hub->hash, cast(char *, name), name_len, ctx,
                          &hash_node->node);
    qtk_rwlock_wunlock(&hub->lock);
    return 0;
}

qtk_actor_context_t *qtk_actor_context_hub_retire(qtk_actor_context_hub_t *hub,
                                                  const char *name) {
    hash_str_node_t *hash_node;
    qtk_actor_context_t *ctx;
    hub_node_t *hub_node;
    int name_len = strlen(name);
    qtk_rwlock_wlock(&hub->lock);
    hash_node = wtk_str_hash_remove(hub->hash, cast(char *, name), name_len);
    qtk_rwlock_wunlock(&hub->lock);
    if (hash_node == NULL) {
        return NULL;
    }
    hub_node = data_offset2(hash_node, hub_node_t, node);
    ctx = hub_node->node.value;
    wtk_lockhoard_push(&hub->hash_node, hub_node);
    return ctx;
}

qtk_actor_context_t *qtk_actor_context_find(qtk_actor_context_hub_t *hub,
                                            const char *name) {
    qtk_actor_context_t *ctx;
    qtk_rwlock_rlock(&hub->lock);
    ctx = wtk_str_hash_find(hub->hash, cast(char *, name), strlen(name));
    if (ctx) {
        qtk_actor_context_grab(ctx);
    }
    qtk_rwlock_runlock(&hub->lock);
    return ctx;
}

qtk_actor_msg_t *qtk_actor_context_hub_new_msg(qtk_actor_context_hub_t *hub,
                                               const wtk_string_t *from,
                                               int len) {
    const wtk_string_t fake = {NULL, 0};
    if (from == NULL) {
        from = &fake;
    }
    qtk_actor_msg_t *msg =
        wtk_malloc(sizeof(qtk_actor_msg_t) + len + from->len);
    msg->cap = len + from->len;
    msg->len = len;
    msg->data = cast(char *, msg + 1);
    msg->from.data = msg->data + len;
    msg->from.len = from->len;
    if (from->len > 0) {
        memcpy(msg->from.data, from->data, from->len);
    }
    return msg;
}

void qtk_actor_context_hub_delete_msg(qtk_actor_context_hub_t *hub,
                                      qtk_actor_msg_t *msg) {
    wtk_free(msg);
}
