#ifndef QTK_UTIL_SPX_QTK_SPX_QUEUE
#define QTK_UTIL_SPX_QTK_SPX_QUEUE

#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"

#include "qtk_spx_msg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_spx_queue qtk_spx_queue_t;

typedef struct {
    unsigned long counter;
    wtk_blockqueue_t spx_q;
} qtk_spx_queue_item_t;

struct qtk_spx_queue {
    wtk_log_t *log;
    qtk_spx_queue_item_t *array;
    qtk_spx_msg_delete_func delete_func;
    void *delete_ths;
    int max_size;
    int capacity;
    int front;
    int near_m;
};

qtk_spx_queue_t *qtk_spx_queue_new(wtk_log_t *log, int max_size);
void qtk_spx_queue_delete(qtk_spx_queue_t *sq);
void qtk_spx_queue_reset(qtk_spx_queue_t *sq);
void qtk_spx_queue_set_delete_func(qtk_spx_queue_t *sq, void *ths,
                                   qtk_spx_msg_delete_func delete_func);

int qtk_spx_queue_touch(qtk_spx_queue_t *sq, unsigned long counter);
int qtk_spx_queue_clean(qtk_spx_queue_t *sq);
int qtk_spx_queue_clean2(qtk_spx_queue_t *sq);

int qtk_spx_queue_feed(qtk_spx_queue_t *sq, unsigned long counter,
                       qtk_spx_msg_t *msg);
wtk_blockqueue_t *qtk_spx_queue_focus(qtk_spx_queue_t *sq);

#ifdef __cplusplus
};
#endif
#endif
