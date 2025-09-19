#ifndef QTK_SESSION_CLDLOG_QTK_CLDLOG
#define QTK_SESSION_CLDLOG_QTK_CLDLOG

#include "wtk/os/wtk_blockqueue.h"
#include "sdk/httpc/qtk_httpc.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_cldlog qtk_cldlog_t;
#define qtk_cldlog_log(l,fmt,...) (l ? qtk_cldlog_feed(l,__FUNCTION__,__LINE__,QTK_CLDLOG_DEBUG,fmt,__VA_ARGS__) :  0)
#define qtk_cldlog_log0(l,fmt,...) (l ? qtk_cldlog_feed(l,__FUNCTION__,__LINE__,QTK_CLDLOG_DEBUG,fmt) :  0)
#define qtk_cldlog_warn(l,fmt,...) (l ? qtk_cldlog_feed(l,__FUNCTION__,__LINE__,QTK_CLDLOG_WARNING,fmt,__VA_ARGS__) :  0)
#define qtk_cldlog_warn0(l,fmt,...) (l ? qtk_cldlog_feed(l,__FUNCTION__,__LINE__,QTK_CLDLOG_WARNING,fmt) :  0)
#define qtk_cldlog_err(l,fmt,...) (l ? qtk_cldlog_feed(l,__FUNCTION__,__LINE__,QTK_CLDLOG_ERROR,fmt,__VA_ARGS__) :  0)
#define qtk_cldlog_err0(l,fmt,...) (l ? qtk_cldlog_feed(l,__FUNCTION__,__LINE__,QTK_CLDLOG_ERROR,fmt) :  0)


struct qtk_cldlog
{
	qtk_option_t *opt;
	wtk_strbuf_t *buf;
	wtk_string_t *id;
	struct {
		qtk_httpc_cfg_t cfg;
		qtk_httpc_t *httpc;
	}http;
	wtk_thread_t thread;
	wtk_blockqueue_t msg_q;
	wtk_lockhoard_t msg_hoard;
	unsigned long seNo;
	unsigned run:1;
};

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_strbuf_t *buf;
}qtk_cldlog_msg_t;

typedef enum {
	QTK_CLDLOG_DEBUG,
	QTK_CLDLOG_WARNING,
	QTK_CLDLOG_ERROR,
}qtk_cldlog_level_t;

qtk_cldlog_t* qtk_cldlog_new(void *father);
void qtk_cldlog_delete(qtk_cldlog_t *clog);

int qtk_cldlog_start(qtk_cldlog_t *clog);
int qtk_cldlog_stop(qtk_cldlog_t *clog);

int qtk_cldlog_feed(qtk_cldlog_t *clog,const char *func,int line,int level,char *fmt,...);

#ifdef __cplusplus
};
#endif
#endif
