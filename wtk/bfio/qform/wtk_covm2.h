#ifndef WTK_BFIO_QFORM_WTK_COVM2
#define WTK_BFIO_QFORM_WTK_COVM2
#include "wtk_covm2_cfg.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_complex_t *scov;
    float *w;
} wtk_covm2_scovmsg_t;

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_complex_t *ncov;
    float *w;
} wtk_covm2_ncovmsg_t;

typedef struct wtk_covm2 wtk_covm2_t;
struct wtk_covm2
{
    wtk_covm2_cfg_t *cfg;

    int nbin;
    int channel;

    wtk_complex_t **ncov;
    float *ncnt_sum;
	wtk_complex_t **scov;
    float *scnt_sum;

    wtk_queue_t *qscov_q;
    wtk_queue_t *qncov_q;
};

wtk_covm2_t* wtk_covm2_new(wtk_covm2_cfg_t *cfg, int nbin, int channel);

void wtk_covm2_delete(wtk_covm2_t *covm2);

void wtk_covm2_reset(wtk_covm2_t *covm2);

int wtk_covm2_feed_fft(wtk_covm2_t *covm2,wtk_complex_t *fft, int k, float *w, int is_ncov);


#ifdef __cplusplus
};
#endif
#endif