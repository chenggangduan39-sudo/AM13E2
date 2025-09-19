#ifndef WTK_CED_H_
#define WTK_CED_H_
#include "wtk/asr/ced/wtk_ced_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_str.h"
#ifdef ONNX_DEC
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_ced wtk_ced_t;
typedef enum wtk_ced_state wtk_ced_state_e;
typedef enum wtk_ced_event_type wtk_ced_event_type_e;
typedef void (*wtk_ced_notify_f)(void *ths, int type, int start, int end);

/*
 * sound event type
 */
enum wtk_ced_event_type { WTK_CED_CRY = 0, WTK_CED_SHOUT, WTK_CED_OTHER };

struct wtk_ced {
    wtk_ced_cfg_t *cfg;
    wtk_string_t *fn;
    int prediction_index;
    void *notify_ths;
    wtk_ced_notify_f notify;
    wtk_strbuf_t *wav;
    int crying_start;
    int crying_end;
    int crying_len;
    int shout_start;
    int shout_end;
    int shout_len;
    unsigned crying_flag : 1;
    unsigned shout_flag : 1;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx;
#endif
};

wtk_ced_t *wtk_ced_new(wtk_ced_cfg_t *cfg);
void wtk_ced_delete(wtk_ced_t *ced);
void wtk_ced_reset(wtk_ced_t *ced);
void wtk_ced_feed(wtk_ced_t *ced, char *data, int len, int is_end);
void wtk_ced_set_notify(wtk_ced_t *ced, wtk_ced_notify_f notify, void *ths);
#ifdef __cplusplus
};
#endif
#endif
