#ifndef WTK_BFIO_DEREVERB_WTK_DEREVERB
#define WTK_BFIO_DEREVERB_WTK_DEREVERB
#include "wtk/core/wtk_strbuf.h"
#include "wtk_dereverb_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_dereverb wtk_dereverb_t;
typedef void(*wtk_dereverb_notify_f)(void *ths,short *output,int len,int is_end);

struct wtk_dereverb
{
	wtk_dereverb_cfg_t *cfg;
	wtk_stft2_t *stft;
	wtk_stft2_t *stft2;
	wtk_queue_t stft2_q;

	wtk_admm_t *admm;
    wtk_covm_t *covm;
	wtk_bf_t *bf;

	int end_pos;

    float *fout;
    float *pad;

	wtk_dereverb_notify_f notify;
	void *ths;
};

wtk_dereverb_t* wtk_dereverb_new(wtk_dereverb_cfg_t *cfg);
void wtk_dereverb_delete(wtk_dereverb_t *dereverb);
void wtk_dereverb_start(wtk_dereverb_t *dereverb);
void wtk_dereverb_reset(wtk_dereverb_t *dereverb);
void wtk_dereverb_set_notify(wtk_dereverb_t *dereverb,void *ths,wtk_dereverb_notify_f notify);
void wtk_dereverb_feed(wtk_dereverb_t *dereverb,short **data,int len,int is_end);

void wtk_dereverb_feed2(wtk_dereverb_t *dereverb,short **data,short **lsty, int len,int is_end);

#ifdef __cplusplus
};
#endif
#endif
