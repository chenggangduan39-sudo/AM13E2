#ifndef __SDK_MOD_COMPRESS_H__
#define __SDK_MOD_COMPRESS_H__
#include "qtk_mod_compress_cfg.h"
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
#endif
#endif
#include "sdk/dev/uart/qtk_uart.h"
#include "sdk/qtk_comm.h"
#include "wtk/bfio/compress/qtk_signal_compress.h"
#include "wtk/bfio/compress/qtk_signal_decompress.h"

#include <sys/ipc.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum{
	qtk_mod_compress_BF_COMPRESS = 1,
	qtk_mod_compress_BF_DECOMPRESS,
	qtk_mod_compress_BF_SAVE_SOURCE,
	qtk_mod_compress_UART_RECV_START,
	qtk_mod_compress_UART_RECV_DATA,
	qtk_mod_compress_UART_RECV_END,
}qtk_mode_bf_type;

typedef struct qtk_mod_compress{
	qtk_mod_compress_cfg_t *cfg;
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
	wtk_thread_t compress_t;
	wtk_thread_t decompress_t;
	wtk_blockqueue_t compress_queue;
	wtk_blockqueue_t decompress_queue;
	wtk_thread_t savepcm_t;
	wtk_blockqueue_t savepcm_queue;
	wtk_sem_t rcdstart_sem;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *decompresswav;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	qtk_signal_compress_cfg_t *compress_cfg;
	qtk_signal_compress_t *compress;
	qtk_signal_decompress_cfg_t *decompress_cfg;
	qtk_signal_decompress_t *decompress;
	
	unsigned int is_rcdstart:1;
	unsigned int savepcm_run:1;
	unsigned int rcd_run:1;
	unsigned int lineout_run:1;
	unsigned int compress_run:1;
	unsigned int decompress_run:1;
	unsigned int uart_read_run:1;
	unsigned int uart_run:1;
	unsigned int log_audio:1;
}qtk_mod_compress_t;

int qtk_mod_compress_bytes(qtk_mod_compress_t *m);
qtk_mod_compress_t *qtk_mod_compress_new(qtk_mod_compress_cfg_t *cfg);
void qtk_mod_compress_delete(qtk_mod_compress_t *m);
void qtk_mod_compress_start(qtk_mod_compress_t *m);
void qtk_mod_compress_stop(qtk_mod_compress_t *m);
void qtk_mod_compress_record_start(qtk_mod_compress_t *m);

#ifdef OFFLINE_TEST
void qtk_mod_compress_feed(qtk_mod_compress_t *m, char *data, int len);
#endif
#ifdef __cplusplus
};
#endif
#endif
