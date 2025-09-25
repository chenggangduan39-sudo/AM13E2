#ifndef QTK_CORE_QTK_HASH
#define QTK_CORE_QTK_HASH

#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_hash qtk_hash_t;

typedef struct {
    wtk_queue_node_t q_n;
    int key;
    void *ths;
} qtk_hash_node_t;

typedef void (*qtk_hash_walk_func)(void *user_data, qtk_hash_node_t *node);

struct qtk_hash {
    int nslot;
    wtk_heap_t *heap;
    wtk_queue_t **slot;
};

qtk_hash_t *qtk_hash_new(int nslot);
int qtk_hash_delete(qtk_hash_t *hash);

int qtk_hash_add(qtk_hash_t *hash, int key, void *ths);
void *qtk_hash_find(qtk_hash_t *hash, int key);

int qtk_hash_walk(qtk_hash_t *hash, void *user_data,
                  qtk_hash_walk_func walk_func);

#ifdef __cplusplus
};
#endif
#endif
