#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICHUP
#define WTK_BFIO_QFORM_WTK_qform_pickup2
#include "wtk_qform_pickup2_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_robin.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_qform_pickup2_notify_f)(void *ths,short *data,int len);

typedef struct wtk_qform_pickup2 wtk_qform_pickup2_t;


typedef struct
{
	wtk_queue_node_t q_n;
	char *cohv;
}wtk_qform_pickup2_cohv_t;

struct wtk_qform_pickup2
{
	wtk_qform_pickup2_cfg_t *cfg;

    wtk_stft_t *stft;

	wtk_strbuf_t *input;
	float notch_mem[2];
	float memD;
	float memX;

	wtk_queue_t cohv_q;

	wtk_qmmse_t *qmmse;

	void *ths;
	wtk_qform_pickup2_notify_f notify;
};

wtk_qform_pickup2_t* wtk_qform_pickup2_new(wtk_qform_pickup2_cfg_t *cfg);

void wtk_qform_pickup2_delete(wtk_qform_pickup2_t *qform);

void wtk_qform_pickup2_reset(wtk_qform_pickup2_t *qform);

void wtk_qform_pickup2_set_notify(wtk_qform_pickup2_t *qform,void *ths,wtk_qform_pickup2_notify_f notify);

void wtk_qform_pickup2_feed(wtk_qform_pickup2_t *qform,short *data,int len,char *cohv,int nbin,int is_end);


#ifdef __cplusplus
};
#endif
#endif
