#ifndef __SDK_MULT_MODULT_H__
#define __SDK_MULT_MODULT_H__
#include "qtk_mult_mod_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifndef OFFLINE_TEST
#include "sdk/audio/recorder/qtk_recorder.h"
#include "sdk/audio/player/qtk_player.h"
#include "sdk/audio/player/qtk_player2.h"
#endif
#include "sdk/qtk_comm.h"

#include "sdk/api_1/ultrasonic/qtk_ultrasonic.h"
#include "wtk/bfio/resample/wtk_resample.h"
#include "speex/speex_resampler.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum{
	qtk_mult_mod_BF_VBOXEBF = 1,
	qtk_mult_mod_BF_GAINNETBF,
	qtk_mult_mod_BF_SSL,
	qtk_mult_mod_BF_EQFORM,
	qtk_mult_mod_BF_AEC,
	qtk_mult_mod_BF_DIRECTION,
	qtk_mult_mod_BF_ASR_TXT,
	qtk_mult_mod_BF_SAVE_SOURCE,
	qtk_mult_mod_BF_SAVE_SPK,
}qtk_mult_mode_bf_type;

typedef struct qtk_mult_mod{
	qtk_mult_mod_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_recorder_t *rcd;
	qtk_player_t *usbaudio;
	qtk_player2_t *lineout;
	wtk_thread_t usbaudio_t;
	wtk_thread_t lineout_t;
	wtk_blockqueue_t usbaudio_queue;
	wtk_blockqueue_t lineout_queue;
#endif
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
	void *ths;
	qtk_engine_notify_f notify;

	unsigned int savepcm_run:1;
	unsigned int rcd_run:1;
	unsigned int qform_run:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int log_audio:1;
}qtk_mult_mod_t;

qtk_mult_mod_t *qtk_mult_mod_new(qtk_session_t *session, qtk_mult_mod_cfg_t *cfg);
void qtk_mult_mod_delete(qtk_mult_mod_t *m);
int qtk_mult_mod_set(qtk_mult_mod_t *m,char *data);
int qtk_mult_mod_start(qtk_mult_mod_t *m);
int qtk_mult_mod_stop(qtk_mult_mod_t *m);
int qtk_mult_mod_set(qtk_mult_mod_t *m,char *data);
void qtk_mult_mod_set_notify(qtk_mult_mod_t *w,void *notify_ths,qtk_engine_notify_f notify_func);
#ifdef OFFLINE_TEST
void qtk_mult_mod_feed(qtk_mult_mod_t *m, char *data, int len);
#endif

#ifdef __cplusplus
};
#endif
#endif
