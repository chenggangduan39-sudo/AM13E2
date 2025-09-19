#ifndef __SDK_MOD_TT2T_H__
#define __SDK_MOD_TT2T_H__
#include "qtk_mod_tt2t_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "wtk/core/wtk_wavfile.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "wtk/core/cfg/wtk_main_cfg.h"

#ifndef OFFLINE_TEST
#ifndef __ANDROID__
#include "qtk/record/qtk_record.h"
#include "qtk/play/qtk_play.h"
#endif
#endif
#include "sdk/dev/uart/qtk_uart.h"
#include "sdk/qtk_comm.h"

#include <sys/ipc.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	qtk_mod_tt2t_BF_tt2t = 1,
	qtk_mod_tt2t_BF_DEtt2t,
	qtk_mod_tt2t_BF_SAVE_SOURCE,
	qtk_mod_tt2t_UART_RECV_START,
	qtk_mod_tt2t_UART_RECV_DATA,
	qtk_mod_tt2t_UART_RECV_END,
}qtk_mode_bf_type;

typedef struct qtk_mod_tt2t{
	qtk_mod_tt2t_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_record_t *rcd;
	qtk_play_t *lineout;
	wtk_thread_t lineout_t;
	wtk_blockqueue_t lineout_queue;
#endif
	wtk_main_cfg_t *uart_cfg;
	qtk_uart_t *uart;
	wtk_thread_t uart_t;
	wtk_thread_t uart_read_t;
	wtk_blockqueue_t uart_queue;
	wtk_log_t *log;
	wtk_thread_t rcd_t;
	wtk_thread_t tt2t_t;
	wtk_thread_t dett2t_t;
	wtk_blockqueue_t tt2t_queue;
	wtk_blockqueue_t dett2t_queue;
	wtk_thread_t savepcm_t;
	wtk_blockqueue_t savepcm_queue;
	wtk_sem_t rcdstart_sem;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *dett2twav;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	
	unsigned int is_rcdstart:1;
	unsigned int savepcm_run:1;
	unsigned int rcd_run:1;
	unsigned int lineout_run:1;
	unsigned int tt2t_run:1;
	unsigned int dett2t_run:1;
	unsigned int uart_read_run:1;
	unsigned int uart_run:1;
	unsigned int log_audio:1;
}qtk_mod_tt2t_t;

int qtk_mod_tt2t_bytes(qtk_mod_tt2t_t *m);
qtk_mod_tt2t_t *qtk_mod_tt2t_new(qtk_mod_tt2t_cfg_t *cfg);
void qtk_mod_tt2t_delete(qtk_mod_tt2t_t *m);
void qtk_mod_tt2t_start(qtk_mod_tt2t_t *m);
void qtk_mod_tt2t_stop(qtk_mod_tt2t_t *m);
void qtk_mod_tt2t_record_start(qtk_mod_tt2t_t *m);

#ifdef OFFLINE_TEST
void qtk_mod_tt2t_feed(qtk_mod_tt2t_t *m, char *data, int len);
#endif
#ifdef __cplusplus
};
#endif
#endif
