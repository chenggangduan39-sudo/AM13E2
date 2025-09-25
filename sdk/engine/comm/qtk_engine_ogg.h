#ifndef QTK_ENGINE_COMM_QTK_ENGINE_OGG
#define QTK_ENGINE_COMM_QTK_ENGINE_OGG

#include "sdk/codec/oggenc/qtk_oggenc.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_engine_ogg qtk_engine_ogg_t;
typedef void (*qtk_engine_ogg_notify_func)(void *ths, int type, char *data,
                                           int len);

typedef enum {
    QTK_ENGINE_OGG_START,
    QTK_ENGINE_OGG_DATA,
    QTK_ENGINE_OGG_END,
    QTK_ENGINE_OGG_CANCEL,
} qtk_engine_ogg_type_t;

typedef struct {
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    qtk_engine_ogg_type_t type;
    wtk_strbuf_t *buf;
    int rate;
    int channel;
    int bits;
} qtk_engine_ogg_msg_t;

struct qtk_engine_ogg {
    struct {
        qtk_oggenc_t *ogg;
        qtk_oggenc_cfg_t cfg;
    } ins;
    qtk_engine_ogg_notify_func notify_func;
    void *notify_ths;
    wtk_thread_t thread;
    wtk_blockqueue_t input_q;
    wtk_lockhoard_t msg_hoard;
    unsigned run : 1;
    unsigned cancel : 1;
};

qtk_engine_ogg_t *qtk_engine_ogg_new();
void qtk_engine_ogg_delete(qtk_engine_ogg_t *ogg);
void qtk_engine_ogg_set_notify(qtk_engine_ogg_t *ogg, void *notify_ths,
                               qtk_engine_ogg_notify_func notify_func);

void qtk_engine_ogg_start(qtk_engine_ogg_t *ogg, int rate, int channels,
                          int bits);
void qtk_engine_ogg_feed(qtk_engine_ogg_t *ogg, char *data, int len,
                         int is_end);
void qtk_engine_ogg_cancel(qtk_engine_ogg_t *ogg);

#ifdef __cplusplus
};
#endif
#endif
