#ifndef __SDK_MGAINNET_MOD_H__
#define __SDK_MGAINNET_MOD_H__
#include "qtk_mgainnet_mod_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/resample/wtk_resample.h"

#ifndef OFFLINE_TEST
#include "sdk/audio/recorder/qtk_recorder.h"
#include "sdk/audio/player/qtk_player.h"
#include "sdk/audio/player/qtk_player2.h"
#endif
#ifdef USE_FOR_DEV
#include "sdk/dev/led/qtk_led.h"
#include "sdk/dev/uart/qtk_uart.h"
#endif
#include "sdk/qtk_comm.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_mgainnet_mod{
	qtk_mgainnet_mod_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_recorder_t *rcd;
	qtk_player_t *usbaudio;
	qtk_player2_t *lineout;
	wtk_thread_t usbaudio_t;
	wtk_thread_t lineout_t;
	wtk_blockqueue_t usbaudio_queue;
	wtk_blockqueue_t lineout_queue;
#endif
#ifdef USE_FOR_DEV
	qtk_led_t *led;
	wtk_main_cfg_t *uart_cfg;
	qtk_uart_t *uart;
	wtk_thread_t uart_t;
	wtk_blockqueue_t uart_queue;
#endif
	qtk_session_t *session;
	qtk_engine_t *engine;
	wtk_resample_t *resample;
	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t qform_t;
	wtk_blockqueue_t bfio_queue;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *single;
	wtk_strbuf_t *playbuf;
	wtk_strbuf_t *tmpbuf;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	wtk_strbuf_t *single_path;
	void *ths;
	qtk_engine_notify_f notify;
	unsigned int rcd_run:1;
	unsigned int qform_run:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int uart_run:1;
	unsigned int log_audio:1;
}qtk_mgainnet_mod_t;

qtk_mgainnet_mod_t *qtk_mgainnet_mod_new(qtk_session_t *session, qtk_mgainnet_mod_cfg_t *cfg);
void qtk_mgainnet_mod_delete(qtk_mgainnet_mod_t *m);
int qtk_mgainnet_mod_start(qtk_mgainnet_mod_t *m);
int qtk_mgainnet_mod_stop(qtk_mgainnet_mod_t *m);
void qtk_mgainnet_mod_set_notify(qtk_mgainnet_mod_t *w,void *notify_ths,qtk_engine_notify_f notify_func);
#ifdef OFFLINE_TEST
void qtk_mgainnet_mod_feed(qtk_mgainnet_mod_t *m, char *data, int len);
#endif
#ifdef __cplusplus
};
#endif
#endif
