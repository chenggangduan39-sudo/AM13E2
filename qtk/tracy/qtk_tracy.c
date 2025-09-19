#include "qtk/tracy/qtk_tracy.h"
#include "libwebsockets.h"
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_stack.h"
#include "wtk/core/wtk_str.h"
#include "wtk/os/wtk_thread.h"

typedef enum {
    MSG_TIMESTAMP,
    MSG_TXT,
    MSG_TENSOR,
} tracy_msg_type_t;

typedef struct {
    wtk_queue_node_t node;
    tracy_msg_type_t type;
    wtk_string_t tag;
    union {
        wtk_string_t txt;
        double timestamp;
        struct {
            qtk_tracy_tensor_type_t t;
            int *shape;
            int shape_len;
            void *payload;
        } tensor;
    };
} tracy_msg_t;

static int tracy_tensor_type_size_[] = {sizeof(float)};

int tracy_msg_serialize_(wtk_stack_t *wbuf, tracy_msg_t *msg) {
    int typ = msg->type;

    wtk_stack_push(wbuf, (const char *)&typ, sizeof(typ));
    wtk_stack_push(wbuf, (const char *)&msg->tag.len, sizeof(msg->tag.len));
    wtk_stack_push(wbuf, msg->tag.data, msg->tag.len);
    switch (typ) {
    case MSG_TIMESTAMP:
        wtk_stack_push(wbuf, (const char *)&msg->timestamp,
                       sizeof(msg->timestamp));
        break;
    case MSG_TXT:
        wtk_stack_push(wbuf, (const char *)&msg->txt.len, sizeof(msg->txt.len));
        wtk_stack_push(wbuf, msg->txt.data, msg->txt.len);
        break;
    case MSG_TENSOR:
        typ = msg->tensor.t;
        wtk_stack_push(wbuf, (const char *)&typ, sizeof(typ));
        int elem_sz = tracy_tensor_type_size_[typ];
        int elem_num = 1;
        for (int i = 0; i < msg->tensor.shape_len; i++) {
            elem_num *= msg->tensor.shape[i];
        }
        wtk_stack_push(wbuf, (const char *)&msg->tensor.shape_len,
                       sizeof(msg->tensor.shape_len));
        wtk_stack_push(wbuf, (const char *)msg->tensor.shape,
                       sizeof(int) * msg->tensor.shape_len);
        wtk_stack_push(wbuf, (const char *)msg->tensor.payload,
                       elem_num * elem_sz);
        break;
    }
    return 0;
}

static void tracy_msg_set_timestamp_(tracy_msg_t *msg, wtk_string_t *tag) {
    msg->timestamp = time_get_ms();
    msg->type = MSG_TIMESTAMP;
    msg->tag = *tag;
}

static void tracy_msg_set_txt_(tracy_msg_t *msg, wtk_string_t *tag,
                               wtk_string_t *txt) {
    msg->type = MSG_TXT;
    msg->tag = *tag;
    msg->txt = *txt;
}

static void tracy_msg_set_tensor_(tracy_msg_t *msg, wtk_string_t *tag,
                                  qtk_tracy_tensor_type_t type, int *shape,
                                  int shape_len, void *payload) {
    msg->type = MSG_TENSOR;
    msg->tag = *tag;
    msg->tensor.payload = payload;
    msg->tensor.shape = shape;
    msg->tensor.shape_len = shape_len;
    msg->tensor.t = type;
}

static int tracy_write_(qtk_tracy_t *t, struct lws *wsi) {
    char buf[LWS_PRE + 4096];
    while (1) {
        wtk_lock_lock(&t->guard);
        int len = wtk_stack_pop2(t->wbuf, buf + LWS_PRE, sizeof(buf) - LWS_PRE);
        wtk_lock_unlock(&t->guard);
        if (len > 0) {
            lws_write(wsi, (unsigned char *)buf + LWS_PRE, len,
                      LWS_WRITE_BINARY);
        }
        if (len != sizeof(buf) - LWS_PRE) {
            break;
        }
    }
    return 0;
}

static int tracy_cb_(struct lws *wsi, enum lws_callback_reasons reason,
                     void *user, void *in, size_t len) {
    qtk_tracy_t *t = user;
    switch (reason) {
    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        break;
    case LWS_CALLBACK_CLIENT_RECEIVE:
        break;
    case LWS_CALLBACK_CLIENT_WRITEABLE:
        tracy_write_(t, wsi);
        break;
    case LWS_CALLBACK_WSI_DESTROY:
        wtk_lock_lock(&t->guard);
        t->cli = NULL;
        wtk_lock_unlock(&t->guard);
        break;
    default:;
    }

    return 0;
}

static struct lws *tracy_connect_(struct lws_context *ctx, qtk_tracy_t *t) {
    struct lws_client_connect_info ccinfo;
    memset(&ccinfo, 0, sizeof(ccinfo));
    ccinfo.context = ctx;
    ccinfo.address = t->cfg->host;
    ccinfo.port = t->cfg->port;
    ccinfo.path = "/";
    ccinfo.host = lws_canonical_hostname(ctx);
    ccinfo.origin = "origin";
    ccinfo.protocol = "tracy";
    ccinfo.userdata = t;
    return lws_client_connect_via_info(&ccinfo);
}

static int tracy_route_(qtk_tracy_t *t, wtk_thread_t *th) {
    struct lws_context *ctx;
    wtk_stack_t *wbuf = wtk_stack_new(1024, 4 * 1024 * 1024, 1.2);
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    struct lws_protocols protocols[] = {
        {"tracy", tracy_cb_, 0, 0},
        {NULL, NULL, 0, 0},
    };
    info.gid = -1;
    info.uid = -1;
    info.protocols = protocols;
    info.port = CONTEXT_PORT_NO_LISTEN;
    ctx = lws_create_context(&info);
    while (t->run) {
        wtk_lock_lock(&t->guard);
        if (t->cli == NULL) {
            wtk_msleep(100);
            t->cli = tracy_connect_(ctx, t);
        }
        wtk_lock_unlock(&t->guard);
        lws_service(ctx, 1);
    }
    wtk_debug("tracy route exit\n");
    lws_context_destroy(ctx);
    wtk_stack_delete(wbuf);
    return 0;
}

qtk_tracy_t *qtk_tracy_new(qtk_tracy_cfg_t *cfg) {
    qtk_tracy_t *t;
    t = wtk_malloc(sizeof(qtk_tracy_t));
    t->cfg = cfg;
    t->run = 0;
    wtk_lock_init(&t->guard);
    t->wbuf = wtk_stack_new(1024, 4 * 1024 * 1024, 1.2);
    t->cli = NULL;
    wtk_thread_init(&t->th, (thread_route_handler)tracy_route_, t);
    return t;
}

int qtk_tracy_start(qtk_tracy_t *t) {
    t->run = 1;
    wtk_thread_start(&t->th);
    return 0;
}

int qtk_tracy_stop(qtk_tracy_t *t) {
    t->run = 0;
    wtk_thread_join(&t->th);
    return 0;
}

void qtk_tracy_delete(qtk_tracy_t *t) {
    wtk_lock_clean(&t->guard);
    wtk_stack_delete(t->wbuf);
    wtk_free(t);
}

static void tracy_raise_msg(qtk_tracy_t *t, tracy_msg_t *msg) {
    wtk_lock_lock(&t->guard);
    if (t->cli) {
        tracy_msg_serialize_(t->wbuf, msg);
        lws_callback_on_writable(t->cli);
    }
    wtk_lock_unlock(&t->guard);
}

int qtk_tracy_log_timestamp(wtk_string_t *tag) {
    extern qtk_tracy_t *glb_tracy;
    tracy_msg_t msg;
    tracy_msg_set_timestamp_(&msg, tag);
    tracy_raise_msg(glb_tracy, &msg);
    return 0;
}

int qtk_tracy_log_tensor(wtk_string_t *tag, qtk_tracy_tensor_type_t type,
                         int *shape, int shape_len, void *payload) {
    extern qtk_tracy_t *glb_tracy;
    tracy_msg_t msg;
    tracy_msg_set_tensor_(&msg, tag, type, shape, shape_len, payload);
    tracy_raise_msg(glb_tracy, &msg);
    return 0;
}

int qtk_tracy_log_txt(wtk_string_t *tag, wtk_string_t *payload) {
    extern qtk_tracy_t *glb_tracy;
    tracy_msg_t msg;
    tracy_msg_set_txt_(&msg, tag, payload);
    tracy_raise_msg(glb_tracy, &msg);
    return 0;
}

int qtk_tracy_log_printf(wtk_string_t *tag, const char *fmt, ...) {
    char buf[4096];
    int len;
    va_list ap;
    va_start(ap, fmt);
    len = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    return qtk_tracy_log_txt(tag, &(wtk_string_t){buf, len});
}