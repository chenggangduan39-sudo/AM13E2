#ifndef __SDK_MULSV_QTK_MULSV_H__
#define __SDK_MULSV_QTK_MULSV_H__
#include "wtk/asr/img/qtk_img.h"
#include "wtk/bfio/wtk_bfio.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "qtk_mulsv_cfg.h"
#include "wtk/core/qtk_lockmsg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/asr/kws/wtk_svprint.h"
#include "wtk/asr/fextra/torchnn/qtk_torchnn.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define USE_MULSV_WAKEUP1ST_LOG (1)
//#define USE_MULSV_WAKEUP2ND_LOG (1)
//#define USE_MULSV_VPRINT_LOG (1)
//#define USE_BUF_TIME (1)
//#define USE_2ND_OFFLINE (1)

typedef void (*qtk_mulsv_notify_f)(void *ths, float conf, char *data, int len);
typedef struct qtk_mulsv qtk_mulsv_t;

enum {
    QTK_MULSV_MSG_START = 0,
    QTK_MULSV_MSG_RESET,
    QTK_MULSV_MSG_END,
    QTK_MULSV_MSG_1ST_PCM,
    QTK_MULSV_MSG_2ND_PCM,
    QTK_MULSV_MSG_ENROLL_START,
    QTK_MULSV_MSG_ENROLL_END,
    QTK_MULSV_MSG_VPRINT_THRESH,
    QTK_MULSV_MSG_NOTIFY_BIAS
};

struct qtk_mulsv{
    qtk_mulsv_cfg_t *cfg;
    qtk_mulsv_notify_f notify;
    void *notify_ths;
    wtk_aec_t *aec;
    wtk_bfio_t *bfio;
    qtk_img_rec_t *img;
    wtk_svprint_t *svprint;
    wtk_blockqueue_t q;
    qtk_lockmsg_t *msg_pool;
    wtk_queue_t cache_q;
    wtk_thread_t t;
    wtk_strbuf_t *wkd1st_pcm, *wkd2nd_pcm, *name;
    char **data_ptr;
    int cnt_1st, cnt_2nd, cnt_vprint;
    float in_tm, img_ds;
    double st, ed;
    uint64_t pop_len;
    int64_t total_time_len, cancel_time_len;
    unsigned total_log : 1, run : 1, eval : 1, main_thread_eval : 1;
};


qtk_mulsv_t* qtk_mulsv_new(qtk_mulsv_cfg_t *cfg);
void qtk_mulsv_delete(qtk_mulsv_t *m);
int qtk_mulsv_start(qtk_mulsv_t *m);
void qtk_mulsv_reset(qtk_mulsv_t *m);
void qtk_mulsv_set_notify(qtk_mulsv_t *m,void *ths,qtk_mulsv_notify_f notify);
int qtk_mulsv_feed(qtk_mulsv_t *m,char *data,int len,int is_end);
int qtk_mulsv_feed2(qtk_mulsv_t *m,char *data,int len,int data_type);

#ifdef __cplusplus
};
#endif
#endif
