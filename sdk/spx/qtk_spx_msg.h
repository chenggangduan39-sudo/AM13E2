#ifndef QTK_API_SPX_QTK_SPX_MSG
#define QTK_API_SPX_QTK_SPX_MSG

#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_spx_msg qtk_spx_msg_t;

typedef void (*qtk_spx_msg_delete_func)(void *ths, qtk_spx_msg_t *msg);

typedef enum {
    QTK_SPX_MSG_APPEND,
    QTK_SPX_MSG_END,
    QTK_SPX_MSG_ERR,
    QTK_SPX_MSG_UNKNOW,
} qtk_spx_msg_type_t;

struct qtk_spx_msg {
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    qtk_spx_msg_type_t type;
    wtk_strbuf_t *lua;
    wtk_strbuf_t *body;
    unsigned long count;
};

#ifdef __cplusplus
};
#endif
#endif
