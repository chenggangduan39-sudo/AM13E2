#ifndef WTK_BFIO_QFORM_WTK_QENVELOPE
#define WTK_BFIO_QFORM_WTK_QENVELOPE
#include "wtk_qenvelope_cfg.h"
#include "wtk/core/wtk_hoard.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    WTK_QENVELOPE_CREST,
    WTK_QENVELOPE_TROUGH,
    WTK_QENVELOPE_FLAT,
}wtk_qenvelope_state_t;

typedef struct
{
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    float specsum;

    void *hook;
}wtk_qenvelope_msg_t;

typedef void(*wtk_qenvelope_notify_f)(void *ths,wtk_qenvelope_msg_t *msg,wtk_qenvelope_state_t state,int is_end, int idx);

typedef struct wtk_qenvelope wtk_qenvelope_t;
struct wtk_qenvelope
{
    wtk_qenvelope_cfg_t *cfg;

    float nframe;
    float last_specm;
    wtk_queue_t qenvelope_q;
    wtk_hoard_t msg_hoard;
    
    wtk_queue_t qenvelope_q2;
    float crest;
    float trough[2];
    float last_specm2;
    int right_nf;

    FILE *qenvl_fn;

    wtk_qenvelope_notify_f notify;
    void *ths;

    int idx;

    unsigned envelope_start:1;
    unsigned envelope2_start:1;
    unsigned has_crest:1;
    unsigned has_crest2:1;
    unsigned has_troughl:1;
    unsigned has_troughr:1;
};


wtk_qenvelope_t* wtk_qenvelope_new(wtk_qenvelope_cfg_t *cfg);

void wtk_qenvelope_delete(wtk_qenvelope_t *qenvelope);

void wtk_qenvelope_reset(wtk_qenvelope_t *qenvelope);

void wtk_qenvelope_set_notify(wtk_qenvelope_t *qenvelope,void *ths,wtk_qenvelope_notify_f notify);

void wtk_qenvelope_feed(wtk_qenvelope_t *qenvelope,float specsum,void *hook,int is_end);

#ifdef __cplusplus
};
#endif
#endif