#ifndef __SDK_MOD_ULT_H__
#define __SDK_MOD_ULT_H__
#include "qtk_mod_ult_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/core/wtk_wavfile.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"

#include "qtk/record/qtk_record.h"
#include "qtk/play/qtk_play.h"
#include "qtk/play/qtk_play2.h"
#include "sdk/qtk_comm.h"
#include "sdk/api_1/ultrasonic/qtk_ultrasonic.h"
#include "wtk/bfio/resample/wtk_resample.h"
#include "speex/speex_resampler.h"
#include <sys/ipc.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	qtk_mod_ult_BF_VBOXEBF = 1,
	qtk_mod_ult_BF_GAINNETBF,
	qtk_mod_ult_BF_SSL,
	qtk_mod_ult_BF_EQFORM,
	qtk_mod_ult_BF_AEC,
	qtk_mod_ult_BF_DIRECTION,
	qtk_mod_ult_BF_ASR_TXT,
	qtk_mod_ult_BF_SAVE_SOURCE,
	qtk_mod_ult_BF_SAVE_SPK,
}qtk_mod_ulte_bf_type;

typedef struct qtk_mod_ult{
	qtk_mod_ult_cfg_t *cfg;
	qtk_record_t *rcd;
	qtk_play_t *usbaudio;
	qtk_play2_t *lineout;
	wtk_thread_t usbaudio_t;
	wtk_thread_t lineout_t;
	wtk_blockqueue_t usbaudio_queue;
	wtk_blockqueue_t lineout_queue;
	wtk_sem_t rcd_sem;

	qtk_ultrasonic_cfg_t *ult_cfg;
	qtk_ultrasonic_t *ult;
	wtk_resample_t *resample;
	// wtk_resample_t *in_resample;
	SpeexResamplerState *in_resample;
	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t qform_t;
	wtk_blockqueue_t bfio_queue;
	wtk_thread_t savepcm_t;
	wtk_blockqueue_t savepcm_queue;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *single;
	wtk_strbuf_t *playbuf;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	wtk_strbuf_t *single_path;

	unsigned int savepcm_run:1;
	unsigned int rcd_run:1;
	unsigned int qform_run:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int log_audio:1;
}qtk_mod_ult_t;
int qtk_mod_ult_bytes(qtk_mod_ult_t *m);
qtk_mod_ult_t *qtk_mod_ult_new(qtk_mod_ult_cfg_t *cfg);
void qtk_mod_ult_delete(qtk_mod_ult_t *m);
void qtk_mod_ult_start(qtk_mod_ult_t *m);
void qtk_mod_ult_stop(qtk_mod_ult_t *m);
void qtk_mod_ult_feed(qtk_mod_ult_t *m, char *data, int len);

#ifdef __cplusplus
};
#endif
#endif
