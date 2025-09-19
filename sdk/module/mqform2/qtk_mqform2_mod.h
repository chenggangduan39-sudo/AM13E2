#ifndef __SDK_MQFORM2_MOD_H__
#define __SDK_MQFORM2_MOD_H__
#include "qtk_mqform2_mod_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifndef OFFLINE_TEST
#include "sdk/audio/recorder/qtk_recorder.h"
#ifndef __ANDROID__
#include "sdk/audio/player/qtk_player.h"
#include "sdk/audio/player/qtk_player2.h"
#endif
#endif
#ifdef USE_FOR_DEV
#include "sdk/dev/led/qtk_led.h"
#include "sdk/dev/uart/qtk_uart.h"
#endif
#include "sdk/qtk_comm.h"

#include "sdk/api_1/soundScreen/qtk_soundscreen.h"
#include "sdk/api_1/aec/qtk_aec.h"
#include "sdk/api_1/eqform/qtk_eqform.h"
#include "sdk/api_1/beamnet/qtk_beamnet.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum{
	QTK_MQFORM2_MOD_BF_AECNSPICKUP = 1,
	QTK_MQFORM2_MOD_BF_EQFORM,
	QTK_MQFORM2_MOD_BF_SOUNDSCREEN,
	QTK_MQFORM2_MOD_BF_BEAMNET,
	QTK_MQFORM2_MOD_BF_AEC=10,
}qtk_mqform2_mode_bf_type;

typedef struct qtk_mqform2_mod{
	qtk_mqform2_mod_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_recorder_t *rcd;
#ifndef __ANDROID__
	qtk_player_t *usbaudio;
	qtk_player2_t *lineout;
	wtk_thread_t usbaudio_t;
	wtk_thread_t lineout_t;
	wtk_blockqueue_t usbaudio_queue;
	wtk_blockqueue_t lineout_queue;
#endif
#endif
#ifdef USE_FOR_DEV
	qtk_led_t *led;
	wtk_main_cfg_t *uart_cfg;
	qtk_uart_t *uart;
	wtk_thread_t uart_t;
	wtk_blockqueue_t uart_queue;
#endif
	qtk_soundscreen_cfg_t *sc_cfg;
	qtk_soundscreen_t *sc;
	qtk_eqform_cfg_t *eqform_cfg;
	qtk_eqform_t *eqform;
	qtk_aec_cfg_t *aec_cfg;
	qtk_aec_t *aec;
	qtk_beamnet_cfg_t *beamnet_cfg;
	qtk_beamnet_cfg_t *beamnet2_cfg;
	qtk_beamnet_t *beamnet;
	qtk_beamnet_t *beamnet2;
	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t qform_t;
	wtk_thread_t qform2_t;
	wtk_thread_t merge_t;
	wtk_thread_t aec_t;
	wtk_blockqueue_t bfio_queue;
	wtk_blockqueue_t bfio2_queue;
	wtk_blockqueue_t merge_queue;
	wtk_blockqueue_t aec_queue;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *single;
	wtk_strbuf_t **wbf_buf;
	wtk_strbuf_t *playbuf;
	wtk_strbuf_t *tmpbuf;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	wtk_strbuf_t *single_path;
	void *ths;
	qtk_engine_notify_f notify;
	unsigned int rcd_run:1;
	unsigned int qform_run:1;
	unsigned int qform2_run:1;
	unsigned int merge_run:1;
	unsigned int aec_run:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int uart_run:1;
	unsigned int log_audio:1;
}qtk_mqform2_mod_t;

qtk_mqform2_mod_t *qtk_mqform2_mod_new(qtk_session_t *session, qtk_mqform2_mod_cfg_t *cfg);
void qtk_mqform2_mod_delete(qtk_mqform2_mod_t *m);
int qtk_mqform2_mod_start(qtk_mqform2_mod_t *m);
int qtk_mqform2_mod_stop(qtk_mqform2_mod_t *m);
void qtk_mqform2_mod_set_notify(qtk_mqform2_mod_t *w,void *notify_ths,qtk_engine_notify_f notify_func);
#ifdef OFFLINE_TEST
void qtk_mqform2_mod_feed(qtk_mqform2_mod_t *m, char *data, int len);
#endif
#ifdef __cplusplus
};
#endif
#endif
