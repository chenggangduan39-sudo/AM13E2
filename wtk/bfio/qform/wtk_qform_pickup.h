#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICHUP
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP
#include "wtk_qform_pickup_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_robin.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void(*wtk_qform_pickup_notify_f)(void *ths,short *data,int len);

typedef struct wtk_qform_pickup wtk_qform_pickup_t;

struct wtk_qform_pickup
{
	wtk_qform_pickup_cfg_t *cfg;

	wtk_strbuf_t **input;

    wtk_stft_t *stft;
    wtk_bf_t *bf;

	wtk_complex_t **ovec;
	wtk_complex_t **ovec_thresh;
    wtk_complex_t **ovec_thresh2;

	float ntheta[10];
	wtk_complex_t ***novec;
	wtk_complex_t ***novec_thresh1;
    wtk_complex_t ***novec_thresh2;

    wtk_complex_t **ncov;
    int *ncnt_sum;
    // wtk_complex_t **ncovtmp;

	wtk_complex_t **scov;
    int *scnt_sum;

    float theta;
    float phi;

    float *cohv;
    float nframe;

	float notch_mem[10][2];
	float memD[10];
	float memX;

	wtk_queue_t stft_q;

	wtk_qmmse_t *qmmse;

    short *output;

	void *ths;
	wtk_qform_pickup_notify_f notify;
};

wtk_qform_pickup_t* wtk_qform_pickup_new(wtk_qform_pickup_cfg_t *cfg);

void wtk_qform_pickup_delete(wtk_qform_pickup_t *qform);

void wtk_qform_pickup_reset(wtk_qform_pickup_t *qform);

void wtk_qform_pickup_set_notify(wtk_qform_pickup_t *qform,void *ths,wtk_qform_pickup_notify_f notify);

void wtk_qform_pickup_start(wtk_qform_pickup_t *qform,float theta,float phi);

void wtk_qform_pickup_feed(wtk_qform_pickup_t *qform,short **data,int len,int is_end);

void wtk_qform_pickup_set_ovec(wtk_qform_pickup_t *qform,float theta,float phi);

void wtk_qform_pickup_set_grav(wtk_qform_pickup_t *qform,short x,short y,short z);


#ifdef __cplusplus
};
#endif
#endif
