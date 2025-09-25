#ifndef __QTK_UART_OTA_H__
#define __QTK_UART_OTA_H__
#include "qtk_uart_ota_cfg.h"
#include "sdk/dev/uart/qtk_uart.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_blockqueue.h"
#include "sdk/codec/qtk_msg.h"
#ifdef _cplusplus
extern "C"{
#endif

typedef enum {
	MSG_TYPE_GET_VERSION,
	MSG_TYPE_OTA_DATA_START,
	MSG_TYPE_OTA_DATA_DATA,
	MSG_TYPE_OTA_DATA_END,
}msg_type_t;

typedef enum {
	OTA_SEND_VERSION,
	OTA_SEND_OTA_RLT,
}ota_send_type_t;

typedef struct qtk_uart_ota {
	qtk_uart_ota_cfg_t *cfg;
	qtk_uart_t *uart;
	qtk_msg_t *msg;
	wtk_thread_t read_thread;
	wtk_thread_t process_thread;
	wtk_blockqueue_t queue;
	wtk_strbuf_t *buf;
	wtk_strbuf_t *md5;
	unsigned char start:1;
	unsigned char read_thread_run:1;
	unsigned char process_thread_run:1;
}qtk_uart_ota_t;

qtk_uart_ota_t *qtk_uart_ota_new(qtk_uart_ota_cfg_t *cfg);
void qtk_uart_ota_delete(qtk_uart_ota_t *ota);
int qtk_uart_ota_start(qtk_uart_ota_t *ota);
int qtk_uart_ota_stop(qtk_uart_ota_t *ota);

#ifdef _cplusplus
};
#endif
#endif
