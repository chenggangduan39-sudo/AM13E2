#include "qtk_spx_queue.h"

static int qtk_spx_queue_is_empty(qtk_spx_queue_t *sq) {
    if (sq->capacity == 0) {
        return 1;
    }
    return 0;
}

static int qtk_spx_queue_is_full(qtk_spx_queue_t *sq) {
    if (sq->capacity == sq->max_size) {
        return 1;
    }
    return 0;
}

static void qtk_spx_queue_init(qtk_spx_queue_t *sq) {
    sq->max_size = 0;
    sq->capacity = 0;
    sq->front = 0;
    sq->near_m = 0;
    sq->array = NULL;
    sq->log = NULL;

    sq->delete_func = NULL;
    sq->delete_ths = NULL;
}

qtk_spx_queue_t *qtk_spx_queue_new(wtk_log_t *log, int max_size) {
    qtk_spx_queue_t *sq;
    int i;

    sq = (qtk_spx_queue_t *)wtk_malloc(sizeof(qtk_spx_queue_t));
    qtk_spx_queue_init(sq);

    sq->log = log;
    sq->max_size = max_size;
    sq->array = (qtk_spx_queue_item_t *)wtk_malloc(
        sizeof(qtk_spx_queue_item_t) * sq->max_size);
    for (i = 0; i < sq->max_size; ++i) {
        sq->array[i].counter = 0;
        wtk_blockqueue_init(&sq->array[i].spx_q);
    }

    wtk_log_log(sq->log, "spx queue max_size %d", sq->max_size);
    return sq;
}

void qtk_spx_queue_delete(qtk_spx_queue_t *sq) {
    int i;

    qtk_spx_queue_reset(sq);
    for (i = 0; i < sq->max_size; ++i) {
        wtk_blockqueue_clean(&sq->array[i].spx_q);
    }
    wtk_free(sq->array);
    wtk_free(sq);
}

void qtk_spx_queue_reset(qtk_spx_queue_t *sq) {
    wtk_blockqueue_t *spx_q;
    wtk_queue_node_t *qn;
    qtk_spx_msg_t *msg;
    int i;

    for (i = 0; i < sq->max_size; ++i) {
        spx_q = &sq->array[i].spx_q;
        while (1) {
            qn = wtk_blockqueue_pop(spx_q, 0, NULL);
            if (!qn) {
                break;
            }
            msg = data_offset2(qn, qtk_spx_msg_t, q_n);
            if (sq->delete_func) {
                sq->delete_func(sq->delete_ths, msg);
            }
        }
        sq->array[i].counter = 0;
    }
    sq->capacity = 0;
    sq->front = 0;
    sq->near_m = 0;

    //	i = sq->front;
    //	while(i != sq->near) {
    //		spx_q = &sq->array[i].spx_q;
    //		while(1) {
    //			qn = wtk_blockqueue_pop(spx_q,0,NULL);
    //			if(!qn) {
    //				break;
    //			}
    //			msg = data_offset2(qn,qtk_spx_msg_t,q_n);
    //			if(sq->delete_func) {
    //				sq->delete_func(sq->delete_ths,msg);
    //			}
    //		}
    //		sq->array[i].counter = 0;
    //	}
}

void qtk_spx_queue_set_delete_func(qtk_spx_queue_t *sq, void *delete_ths,
                                   qtk_spx_msg_delete_func delete_func) {
    sq->delete_func = delete_func;
    sq->delete_ths = delete_ths;
}

int qtk_spx_queue_touch(qtk_spx_queue_t *sq, unsigned long counter) {
    if (qtk_spx_queue_is_full(sq)) {
        wtk_log_log(sq->log, "spx queue full %d %d %d %d", sq->max_size,
                    sq->capacity, sq->front, sq->near_m);
        return -1;
    }

    sq->array[sq->near_m].counter = counter;
    sq->near_m = (sq->near_m + 1) % sq->max_size;
    ++sq->capacity;

    return 0;
}

int qtk_spx_queue_clean(qtk_spx_queue_t *sq) {
    wtk_blockqueue_t *spx_q;
    wtk_queue_node_t *qn;
    qtk_spx_msg_t *msg;

    if (qtk_spx_queue_is_empty(sq)) {
        wtk_log_log(sq->log, "spx queue empty %d %d %d %d", sq->max_size,
                    sq->capacity, sq->front, sq->near_m);
        return -1;
    }

    // wtk_log_log(sq->log,"clean counter %ld",sq->array[sq->front].counter);
    sq->array[sq->front].counter = 0;
    spx_q = &sq->array[sq->front].spx_q;
    while (1) {
        qn = wtk_blockqueue_pop(spx_q, 0, NULL);
        if (!qn) {
            break;
        }
        msg = data_offset2(qn, qtk_spx_msg_t, q_n);
        if (sq->delete_func) {
            sq->delete_func(sq->delete_ths, msg);
        }
    }
    sq->front = (sq->front + 1) % sq->max_size;
    --sq->capacity;

    return 0;
}

int qtk_spx_queue_clean2(qtk_spx_queue_t *sq) {
    wtk_blockqueue_t *spx_q;
    wtk_queue_node_t *qn;
    qtk_spx_msg_t *msg;

    if (qtk_spx_queue_is_empty(sq)) {
        wtk_log_log(sq->log, "spx queue empty %d %d %d %d", sq->max_size,
                    sq->capacity, sq->front, sq->near_m);
        return -1;
    }

    // wtk_log_log(sq->log,"clean2 counter %ld",sq->array[sq->near].counter);
    sq->array[sq->near_m].counter = 0;
    spx_q = &sq->array[sq->near_m].spx_q;
    while (1) {
        qn = wtk_blockqueue_pop(spx_q, 0, NULL);
        if (!qn) {
            break;
        }
        msg = data_offset2(qn, qtk_spx_msg_t, q_n);
        if (sq->delete_func) {
            sq->delete_func(sq->delete_ths, msg);
        }
    }
    sq->near_m = (--sq->near_m) >= 0 ? sq->near_m : sq->near_m + sq->max_size;
    --sq->capacity;

    return 0;
}

int qtk_spx_queue_feed(qtk_spx_queue_t *sq, unsigned long counter,
                       qtk_spx_msg_t *msg) {
    int ret;
    int i;

    if (qtk_spx_queue_is_empty(sq)) {
        wtk_log_log(sq->log, "spx queue empty %d %d %d %d", sq->max_size,
                    sq->capacity, sq->front, sq->near_m);
        if (sq->delete_func) {
            sq->delete_func(sq->delete_ths, msg);
        }
        return -1;
    }

    ret = -1;
    i = sq->front;
    while (i != sq->near_m) {
        if (counter == sq->array[i].counter) {
            wtk_blockqueue_push(&sq->array[i].spx_q, &msg->q_n);
            ret = 0;
            goto end;
        }
        i = (i + 1) % sq->max_size;
    }

end:
    if (ret != 0) {
        wtk_log_log(sq->log, "spx no counter %ld", counter);
        if (sq->delete_func) {
            sq->delete_func(sq->delete_ths, msg);
        }
    }
    return ret;
}

wtk_blockqueue_t *qtk_spx_queue_focus(qtk_spx_queue_t *sq) {
    if (qtk_spx_queue_is_empty(sq)) {
        wtk_log_log(sq->log, "spx queue empty %d %d %d %d", sq->max_size,
                    sq->capacity, sq->front, sq->near_m);
        return NULL;
    }

    return &sq->array[sq->front].spx_q;
}
