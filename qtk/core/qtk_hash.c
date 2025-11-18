#include "qtk_hash.h"

qtk_hash_t *qtk_hash_new(int nslot) {
    qtk_hash_t *hash;

    hash = (qtk_hash_t *)wtk_malloc(sizeof(qtk_hash_t));
    hash->nslot = nslot;
    hash->slot = (wtk_queue_t **)wtk_malloc(sizeof(wtk_queue_t *) * nslot);
    memset(hash->slot, 0, sizeof(wtk_queue_t *) * nslot);
    hash->heap = wtk_heap_new(4096);
    return hash;
}

int qtk_hash_delete(qtk_hash_t *hash) {
    wtk_heap_delete(hash->heap);
    wtk_free(hash->slot);
    wtk_free(hash);

    return 0;
}

int qtk_hash_add(qtk_hash_t *hash, int key, void *ths) {
    qtk_hash_node_t *node;
    unsigned int index;
    int ret;

    node =
        (qtk_hash_node_t *)wtk_heap_malloc(hash->heap, sizeof(qtk_hash_node_t));
    node->key = key;
    node->ths = ths;
    index = key % hash->nslot;

    if (!hash->slot[index]) {
        hash->slot[index] =
            (wtk_queue_t *)wtk_heap_malloc(hash->heap, sizeof(wtk_queue_t));
        wtk_queue_init(hash->slot[index]);
    }

    ret = wtk_queue_push(hash->slot[index], &node->q_n);
    return ret;
}

void *qtk_hash_find(qtk_hash_t *hash, int key) {
    wtk_queue_node_t *qn;
    qtk_hash_node_t *node = NULL;
    unsigned int index;

    index = key % hash->nslot;
    if (!hash->slot[index]) {
        goto end;
    }

    for (qn = hash->slot[index]->pop; qn; qn = qn->next) {
        node = data_offset2(qn, qtk_hash_node_t, q_n);
        if (node->key == key) {
            goto end;
        }
    }

    node = NULL;
end:
    return node ? node->ths : NULL;
}

int qtk_hash_walk(qtk_hash_t *hash, void *user_data,
                  qtk_hash_walk_func walk_func) {
    wtk_queue_node_t *qn;
    qtk_hash_node_t *node;
    int i;

    if (!walk_func) {
        return -1;
    }

    for (i = 0; i < hash->nslot; ++i) {
        if (!hash->slot[i]) {
            continue;
        }
        for (qn = hash->slot[i]->pop; qn; qn = qn->next) {
            node = data_offset2(qn, qtk_hash_node_t, q_n);
            walk_func(user_data, node);
        }
    }

    return 0;
}
