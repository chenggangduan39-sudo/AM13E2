#ifndef WTK_BFIO_QFORM_WTK_COVM
#define WTK_BFIO_QFORM_WTK_COVM
#include "wtk_covm_cfg.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/core/wtk_complex.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    wtk_complex_t *scov;
} wtk_covm_scovmsg_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    wtk_complex_t *ncov;
} wtk_covm_ncovmsg_t;

typedef struct wtk_covm wtk_covm_t;
struct wtk_covm
{
    wtk_covm_cfg_t *cfg;

    int nbin;
    int channel;

    wtk_complex_t **ncov;
    wtk_complex_t **ncovtmp;
    float *ncnt_sum;
    float *n_mask;
	wtk_complex_t **scov;
    wtk_complex_t **scovtmp;
    float *scnt_sum;
    float *s_mask;

    wtk_queue_t qsmsg_q;

    wtk_queue_t *qscov_q;
    wtk_queue_t *qncov_q;
	wtk_hoard_t scov_hoard;
	wtk_hoard_t ncov_hoard;
};

wtk_covm_t* wtk_covm_new(wtk_covm_cfg_t *cfg, int nbin, int channel);

void wtk_covm_delete(wtk_covm_t *covm);

void wtk_covm_reset(wtk_covm_t *covm);

int wtk_covm_feed_fft(wtk_covm_t *covm,wtk_complex_t **fft, int k, int is_ncov);

int wtk_covm_feed_fft2(wtk_covm_t *covm,wtk_complex_t **fft, int k, int is_ncov);

int wtk_covm_feed_fft3(wtk_covm_t *covm,wtk_complex_t *fft, int k, int is_ncov);

int wtk_covm_feed_fft4(wtk_covm_t *covm,wtk_complex_t *fft, int k, int is_ncov, float scale);

#ifdef __cplusplus
};
#endif
#endif