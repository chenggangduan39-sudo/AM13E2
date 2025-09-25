#ifndef WTK_BFIO_VBOX_WTK_CONSIST
#define WTK_BFIO_VBOX_WTK_CONSIST
#include "wtk_consist_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_sem.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    WTK_CONSIST_MICERR_NIL,
    WTK_CONSIST_MICERR_ALIGN,
    WTK_CONSIST_MICERR_MAX,
    WTK_CONSIST_MICERR_CORR,
    WTK_CONSIST_MICERR_ENERGY,
    WTK_CONSIST_SPKERR_NIL,
    WTK_CONSIST_SPKERR_ALIGN,
    WTK_CONSIST_SPKERR_MAX,
    WTK_CONSIST_SPKERR_CORR,
    WTK_CONSIST_SPKERR_ENERGY,
}wtk_consist_micerror_type_t;

typedef struct wtk_consist wtk_consist_t;
typedef void (*wtk_consist_notify_f) (void *ths, wtk_consist_micerror_type_t errtype, int channel);

typedef struct
{
	wtk_queue_node_t qn;
	wtk_queue_node_t hoard_on;
	wtk_strbuf_t *buf;
	int type;
}wtk_consist_msg_node_t;

typedef struct wtk_consist_msg
{
    wtk_lockhoard_t msg_hoard;
}wtk_consist_msg_t;

struct wtk_consist
{
	wtk_consist_cfg_t *cfg;
    // wtk_ssl2_t *ssl;

    wtk_consist_msg_t *msg;
    wtk_thread_t spk_t;
    wtk_blockqueue_t spk_q;
    wtk_sem_t spk_sem;

    wtk_thread_t mic_t;
    wtk_blockqueue_t mic_q;
    wtk_sem_t mic_sem;

    int channel;
    wtk_strbuf_t **mic;
	wtk_strbuf_t **sp;
    wtk_consist_notify_f notify;
    void *ths;
    int *errchn;
    int **errtype;

    float *corr;
    float *energy;

    unsigned mic_run:1;
    unsigned spk_run:1;
    unsigned consist:1;
    unsigned use_mic:1;
};

wtk_consist_t* wtk_consist_new(wtk_consist_cfg_t *cfg);
void wtk_consist_delete(wtk_consist_t *consist);
void wtk_consist_reset(wtk_consist_t *consist);
void wtk_consist_set_notify(wtk_consist_t *consist, void *ths, wtk_consist_notify_f notify);
void wtk_consist_feed(wtk_consist_t *consist,short *data,int len,int is_end);
void wtk_consist_feed2(wtk_consist_t *con,short *data,int len,int is_end);
void wtk_consist_feed3(wtk_consist_t *con,short *data,int len,int is_end);
#ifdef __cplusplus
};
#endif
#endif
