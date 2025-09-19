#ifndef A5DCAA6D_BEAB_421D_B538_49F2CE0F412D
#define A5DCAA6D_BEAB_421D_B538_49F2CE0F412D

#include "qtk/tracy/qtk_tracy_cfg.h"
#include "wtk/core/wtk_stack.h"
#include "wtk/core/wtk_str.h"
#include "wtk/os/wtk_lock.h"
#include "wtk/os/wtk_thread.h"

typedef struct qtk_tracy qtk_tracy_t;

typedef enum {
    QTK_TRACY_TENSOR_TYPE_FP32,
} qtk_tracy_tensor_type_t;

struct qtk_tracy {
    qtk_tracy_cfg_t *cfg;
    wtk_thread_t th;

    wtk_lock_t guard;
    wtk_stack_t *wbuf;
    void *cli;

    unsigned run : 1;
};

qtk_tracy_t *qtk_tracy_new(qtk_tracy_cfg_t *cfg);
int qtk_tracy_start(qtk_tracy_t *t);
int qtk_tracy_stop(qtk_tracy_t *t);
void qtk_tracy_delete(qtk_tracy_t *t);
int qtk_tracy_log_timestamp(wtk_string_t *tag);
int qtk_tracy_log_tensor(wtk_string_t *tag, qtk_tracy_tensor_type_t type,
                         int *shape, int shape_len, void *payload);
int qtk_tracy_log_txt(wtk_string_t *tag, wtk_string_t *payload);
int qtk_tracy_log_printf(wtk_string_t *tag, const char *fmt, ...);

#define qtk_tracy_log_timestamp_s(tag)                                         \
    qtk_tracy_log_timestamp(&(wtk_string_t)wtk_string(tag))
#define qtk_tracy_log_tensor_s(tag, type, shape, shape_len, payload)           \
    qtk_tracy_log_tensor(&(wtk_string_t)wtk_string(tag), type, shape,          \
                         shape_len, payload)
#define qtk_tracy_log_txt_ss(tag, payload)                                     \
    qtk_tracy_log_txt(&(wtk_string_t)wtk_string(tag),                          \
                      &(wtk_string_t)wtk_string(payload))
#define qtk_tracy_log_txt_s(tag, payload)                                      \
    qtk_tracy_log_txt(&(wtk_string_t)wtk_string(tag), payload)
#define qtk_tracy_log_printf_s(tag, fmt, ...)                                  \
    qtk_tracy_log_printf(&(wtk_string_t)wtk_string(tag), fmt, __VA_ARGS__)

#endif /* A5DCAA6D_BEAB_421D_B538_49F2CE0F412D */
