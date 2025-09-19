#ifndef WTK_SED_H_
#define WTK_SED_H_
#include "wtk/asr/sed/wtk_sed_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_str.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_sed wtk_sed_t;
typedef struct wtk_sed_state_node wtk_sed_state_node_t;
typedef struct wtk_sed_win_node wtk_sed_win_node_t;
typedef enum wtk_sed_state wtk_sed_state_e;
typedef enum wtk_sed_event_type wtk_sed_event_type_e;
typedef void (*wtk_sed_notify_f)(void *ths, int type, float start, float end);

/*
 * sound event type
 */
enum wtk_sed_event_type { WTK_SED_CRY = 0, WTK_SED_SHOUT };

/*
 * the prediction process of state machine type
 */
enum wtk_sed_state {
    WTK_SED_NO_EVENT = 0,
    WTK_SED_IS_EVENT,
    WTK_SED_WAIT_EVENT
};

/*
 * the prediction process of state machine node to save the information of
 * prob queue
 */
struct wtk_sed_state_node {
    int begin;
    int end;
    int len;
    int wait_len;
    int high_n_smooth;
    int low_n_smooth;
    int n_salt;
    int prediction_index;
    int is_high;
    float high_threshold;
    float low_threshold;
    wtk_sed_event_type_e etype;
    wtk_sed_state_e state;
};

/*
 * the window node of struct
 */
struct wtk_sed_win_node {
    int avr_count;
    int num;
    int classes;
    float *prob;
};

struct wtk_sed {
    float *mean;
    wtk_sed_cfg_t *cfg;
    wtk_sed_state_node_t *cry;
    wtk_sed_state_node_t *shout;
    wtk_sed_notify_f notify;
    void *notify_ths;
    wtk_sed_win_node_t **win_node;
    wtk_robin_t *robin;
    wtk_string_t *fn;
    wtk_strbuf_t *wav;
#ifdef ONNX_DEC
    qtk_onnxruntime_t *onnx;
#endif
};

wtk_sed_t *wtk_sed_new(wtk_sed_cfg_t *cfg);
void wtk_sed_delete(wtk_sed_t *sed);
void wtk_sed_reset(wtk_sed_t *sed);
void wtk_sed_feed(wtk_sed_t *sed, char *data, int len, int is_end);
void wtk_sed_set_notify(wtk_sed_t *sed, wtk_sed_notify_f notify, void *ths);
void wtk_sed_set_para(wtk_sed_t *sed, int cry_smooth, int cry_salt, float cry_h,
                      float cry_l, int shout_smooth, int shout_salt,
                      float shout_h, float shout_l);
#ifdef __cplusplus
};
#endif
#endif
