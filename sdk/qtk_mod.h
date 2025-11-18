#ifndef __SDK_MOD_H__
#define __SDK_MOD_H__
#include "qtk_mod_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/core/wtk_wavfile.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifndef OFFLINE_TEST
#ifndef __ANDROID__
#include "qtk/record/qtk_record.h"
#include "qtk/play/qtk_play.h"
#include "qtk/play/qtk_play2.h"
#endif
#endif
#ifdef USE_LED
#include "sdk/dev/led/qtk_led.h"
#endif
#ifdef USE_FOR_DEV
#include "sdk/dev/uart/qtk_uart.h"
#endif
#include "sdk/qtk_comm.h"

#ifdef USE_VBOXEBF
#include "sdk/api_1/vboxebf/qtk_vboxebf.h"
#endif
#ifdef USE_GAINNETBF
#include "sdk/api_1/gainnetbf/qtk_gainnetbf.h"
#endif
#ifdef USE_SSL
#include "sdk/api_1/ssl/qtk_ssl.h"
#endif
#ifdef USE_EQFORM
#include "sdk/api_1/eqform/qtk_eqform.h"
#endif
#ifdef USE_SOUNDSCREEN
#include "sdk/api_1/soundScreen/qtk_soundscreen.h"
#endif
#ifdef USE_AEC
#include "sdk/api_1/aec/qtk_aec.h"
#endif
#ifdef USE_BFIO
#include "sdk/api_1/bfio/qtk_bfio.h"
#endif
#ifdef USE_QKWS
#include "sdk/api_1/kws/qtk_qkws.h"
#endif
#include "wtk/bfio/resample/wtk_resample.h"
#include "speex/speex_resampler.h"
#include <sys/ipc.h>
#ifdef __cplusplus
extern "C" {
#endif

#define SIZE 64
#define MAX 48

typedef struct shm
{
    pid_t pid;
    char buf[MAX];
}SHM;

typedef struct qtk_proc
{
	key_t key;
	SHM *shmaddr;
	int shmid;
	pid_t pid_server;
}qtk_proc_t;

typedef enum{
	QTK_MOD_BF_VBOXEBF = 1,
	QTK_MOD_BF_GAINNETBF,
	QTK_MOD_BF_SSL,
	QTK_MOD_BF_EQFORM,
	QTK_MOD_BF_AEC,
	QTK_MOD_BF_BFIO,
	QTK_MOD_BF_KWS,
	QTK_MOD_BF_SOUNDSCREEN,
	QTK_MOD_BF_DIRECTION,
	QTK_MOD_BF_ASR_TXT,
	QTK_MOD_BF_SAVE_SOURCE,
	QTK_MOD_BF_SAVE_SPK,
}qtk_mode_bf_type;

typedef struct qtk_mod{
	qtk_mod_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_record_t *rcd;
#ifndef __ANDROID__
	qtk_play_t *usbaudio;
	qtk_play2_t *lineout;
	wtk_thread_t usbaudio_t;
	wtk_thread_t lineout_t;
	wtk_blockqueue_t usbaudio_queue;
	wtk_blockqueue_t lineout_queue;
	wtk_sem_t rcd_sem;
#endif
#endif
#ifdef USE_LED
	qtk_led_t *led;
#endif
#ifdef USE_FOR_DEV
	wtk_main_cfg_t *uart_cfg;
	qtk_uart_t *uart;
	wtk_thread_t uart_t;
	wtk_blockqueue_t uart_queue;
#endif

#ifdef USE_VBOXEBF
	qtk_vboxebf_cfg_t *vboxebf_cfg;
	qtk_vboxebf_t *vboxebf;
#endif
#ifdef USE_GAINNETBF
	qtk_gainnetbf_cfg_t *gainnetbf_cfg;
	qtk_gainnetbf_t *gainnetbf;
#endif
#ifdef USE_SSL
	qtk_ssl_cfg_t *ssl_cfg;
	qtk_ssl_t *ssl;
#endif
#ifdef USE_EQFORM
	qtk_eqform_cfg_t *eqform_cfg;
	qtk_eqform_t *eqform;
#endif
#ifdef USE_SOUNDSCREEN
	qtk_soundscreen_cfg_t *soundscreen_cfg;
	qtk_soundscreen_t *soundscreen;
#endif
#ifdef USE_AEC
	qtk_aec_cfg_t *aec_cfg;
	qtk_aec_t *aec;
#endif
#ifdef USE_BFIO
	qtk_bfio_cfg_t *bfio_cfg;
	qtk_bfio_t *bfio;
#endif
#ifdef USE_QKWS
	qtk_qkws_cfg_t *kws_cfg;
	qtk_qkws_t *kws;
#endif
	wtk_resample_t *resample;
	// wtk_resample_t *in_resample;
	SpeexResamplerState *in_resample;
	SpeexResamplerState *out_resample;
	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t qform_t;
	wtk_blockqueue_t bfio_queue;
	wtk_thread_t qform2_t;
	wtk_blockqueue_t bfio2_queue;
	wtk_thread_t qform3_t;
	wtk_blockqueue_t bfio3_queue;
	wtk_thread_t savepcm_t;
	wtk_blockqueue_t savepcm_queue;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *single;
	wtk_strbuf_t *playbuf;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	wtk_strbuf_t *single_path;
	// qtk_proc_t  mem_proc;
	char *tmpresample;
	int minlen;
	long slen;

	unsigned int savepcm_run:1;
	unsigned int rcd_run:1;
	unsigned int qform_run:1;
	unsigned int qform2_run:1;
	unsigned int qform3_run:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int uart_run:1;
	unsigned int log_audio:1;
	unsigned int is_player_start:1;
	unsigned int player_run:1;
}qtk_mod_t;
int qtk_mod_bytes(qtk_mod_t *m);
qtk_mod_t *qtk_mod_new(qtk_mod_cfg_t *cfg);
void qtk_mod_delete(qtk_mod_t *m);
void qtk_mod_start(qtk_mod_t *m);
void qtk_mod_stop(qtk_mod_t *m);

void qtk_mod_start2(qtk_mod_t *m, int sample_rate);
void qtk_mod_stop2(qtk_mod_t *m);
#ifdef OFFLINE_TEST
void qtk_mod_feed(qtk_mod_t *m, char *data, int len);
#endif
#ifdef __cplusplus
};
#endif
#endif
