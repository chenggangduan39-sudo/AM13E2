#ifndef QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADC
#define QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADC

#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_fd.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/os/wtk_log.h"

#include "qtk_uploadc_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
#define READ_BUF_SIZE 1024
#define RECV_DATA_HDR (10)
typedef struct qtk_uploadc qtk_uploadc_t;

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_strbuf_t *buf;
}qtk_uploadc_msg_t;

typedef enum{
	QTK_UPC_STATE_CONNECT,
	QTK_UPC_STATE_DISCONNECT,
	QTK_UPC_STATE_RECV_PMT,
	QTK_UPC_STATE_RECV_OTA,
	QTK_UPC_STATE_SEND_PMT,    // 发送设备参数信息
}qtk_uploadc_state_t;
typedef enum{
	QTK_UPC_RECV_DATA_PMT,
	QTK_UPC_RECV_DATA_ORD,
	QTK_UPC_RECV_DATA_OTA,
}qtk_uploadc_recv_data_type_t;
typedef void (*qtk_uploadc_notify_f)(void *ths, qtk_uploadc_state_t state, char *data, int len);

struct qtk_uploadc
{
	qtk_uploadc_cfg_t *cfg;
	wtk_blockqueue_t input_q;
	wtk_lockhoard_t msg_hoard;
	wtk_thread_t thread_send;
	wtk_thread_t thread_recv;
	wtk_log_t *log;
	void *ths;
	qtk_uploadc_notify_f notify;
	int buf_size;
	qtk_uploadc_state_t current_state;
	wtk_json_parser_t *parser;
	wtk_lock_t qlock;
	int fd;
	unsigned run_send:1;
	unsigned run_recv:1;
};

qtk_uploadc_t* qtk_uploadc_new(qtk_uploadc_cfg_t *cfg,int buf_size);
void qtk_uploadc_delete(qtk_uploadc_t *upc);

int qtk_uploadc_start(qtk_uploadc_t *upc);
int qtk_uploadc_stop(qtk_uploadc_t *upc);
int qtk_uploadc_reconnect(qtk_uploadc_t *upc);
int qtk_uploadc_is_connect(qtk_uploadc_t *upc);
int qtk_uploadc_feed(qtk_uploadc_t *upc,char *data,int bytes);
void qtk_uploadc_set_notify(qtk_uploadc_t *upc, void *ths, qtk_uploadc_notify_f notify);
void qtk_uploadc_set_log(qtk_uploadc_t *upc, wtk_log_t *log);
#ifdef __cplusplus
};
#endif
#endif
