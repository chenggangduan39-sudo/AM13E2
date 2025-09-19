#ifndef __SDK_MOD_DOUBLE_H__
#define __SDK_MOD_DOUBLE_H__
#include "qtk_mod_double_cfg.h"
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
#ifdef USE_FOR_DEV
#include "sdk/dev/uart/qtk_uart.h"
#endif
#include "sdk/qtk_comm.h"

#ifdef USE_GAINNETBF
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
#ifdef USE_AEC
#include "sdk/api_1/aec/qtk_aec.h"
#endif
#ifdef USE_QFORM
#include "sdk/api_1/qform/qtk_qform.h"
#endif
#ifdef USE_BEAMNET
#include "sdk/api_1/beamnet/qtk_beamnet.h"
#endif
#include "wtk/bfio/resample/wtk_resample.h"
#include <sys/ipc.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	qtk_mod_double_BF_VBOXEBF = 1,
	qtk_mod_double_BF_GAINNETBF,
	qtk_mod_double_BF_SSL,
	qtk_mod_double_BF_EQFORM,
	qtk_mod_double_BF_AEC,
	qtk_mod_double_BF_QFORM,
	qtk_mod_double_BF_BEAMNET,
	qtk_mod_double_BF_DIRECTION,
	qtk_mod_double_BF_ASR_TXT,
	qtk_mod_double_BF_SAVE_SOURCE,
	qtk_mod_double_BF_SAVE_SPK,
}qtk_mode_bf_type;

typedef struct qtk_mod_double{
	qtk_mod_double_cfg_t *cfg;
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
#ifdef USE_AEC
	qtk_aec_cfg_t *aec_cfg;
	qtk_aec_t *aec;
#endif
#ifdef USE_QFORM
	qtk_qform_cfg_t *qform_cfg;
	qtk_qform_t *qform;
#endif
#ifdef USE_BEAMNET
	qtk_beamnet_cfg_t *beamnet_cfg;
	qtk_beamnet_cfg_t *beamnet2_cfg;
	qtk_beamnet_t *beamnet;
	qtk_beamnet_t *beamnet2;
#endif
	wtk_resample_t *resample;
	wtk_resample_t *resample2;
	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t qform_t;
	wtk_thread_t qform2_t;
	wtk_blockqueue_t bfio_queue;
	wtk_blockqueue_t qform_queue;
	wtk_thread_t savepcm_t;
	wtk_blockqueue_t savepcm_queue;
	wtk_thread_t merge_t;
	wtk_blockqueue_t merge_queue;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *single;
	wtk_strbuf_t *playbuf;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	wtk_strbuf_t *single_path;
	char *zdata;
	int minlen;
	long slen;

	unsigned int savepcm_run:1;
	unsigned int rcd_run:1;
	unsigned int merge_run:1;
	unsigned int qform_run:1;
	unsigned int qform2_run:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int uart_run:1;
	unsigned int log_audio:1;
	unsigned int is_player_start:1;
	unsigned int player_run:1;
}qtk_mod_double_t;
int qtk_mod_double_bytes(qtk_mod_double_t *m);
qtk_mod_double_t *qtk_mod_double_new(qtk_mod_double_cfg_t *cfg);
void qtk_mod_double_delete(qtk_mod_double_t *m);
void qtk_mod_double_start(qtk_mod_double_t *m);
void qtk_mod_double_stop(qtk_mod_double_t *m);

#ifdef OFFLINE_TEST
void qtk_mod_double_feed(qtk_mod_double_t *m, char *data, int len);
#endif
#ifdef __cplusplus
};
#endif
#endif
