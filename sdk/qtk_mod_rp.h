#ifndef __SDK_MOD_RP_H__
#define __SDK_MOD_RP_H__
#include "qtk_mod_rp_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "wtk/core/wtk_wavfile.h"

#ifndef OFFLINE_TEST
#ifndef __ANDROID__
#include "qtk/record/qtk_record.h"
#include "qtk/play/qtk_play.h"
#include "qtk/play/qtk_play2.h"
#endif
#endif
#include "sdk/qtk_comm.h"
#include "speex/speex_resampler.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_mod_rp{
	qtk_mod_rp_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_record_t *rcd;
#ifndef __ANDROID__
	qtk_play_t *usbaudio;
	qtk_play2_t *lineout;
	wtk_thread_t usbaudio_t;
	wtk_thread_t lineout_t;
	wtk_blockqueue_t usbaudio_queue;
	wtk_blockqueue_t lineout_queue;
#endif
#endif

	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t savepcm_t;
	wtk_blockqueue_t savepcm_queue;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;

	unsigned int savepcm_run:1;
	unsigned int rcd_run:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int player_run:1;
}qtk_mod_rp_t;

int qtk_mod_rp_bytes(qtk_mod_rp_t *m);
qtk_mod_rp_t *qtk_mod_rp_new(qtk_mod_rp_cfg_t *cfg);
void qtk_mod_rp_delete(qtk_mod_rp_t *m);
void qtk_mod_rp_start(qtk_mod_rp_t *m);
void qtk_mod_rp_stop(qtk_mod_rp_t *m);

#ifdef __cplusplus
};
#endif
#endif
