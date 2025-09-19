#ifndef QTK_SESSION_ERRCODE_QTK_ERRCODE
#define QTK_SESSION_ERRCODE_QTK_ERRCODE

#include "qtk/core/qtk_hash.h"
#include "sdk/qtk_api.h"
#include "sdk/qtk_err.h"
#include "sdk/session/option/qtk_option.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_thread.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_errcode qtk_errcode_t;

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	qtk_errcode_level_t level;
	int errcode;
	wtk_strbuf_t *extraErrdata;
}qtk_errcode_node_t;

struct qtk_errcode
{
	wtk_log_t *log;
        qtk_hash_t *hash;
        qtk_errcode_handler handler;
	void *handler_ths;
	wtk_blockqueue_t err_q;
	wtk_lockhoard_t err_hoard;
	wtk_thread_t thead;
	qtk_errcode_level_t level;
	unsigned run:1;
	unsigned use_thread:1;
};

qtk_errcode_t* qtk_errcode_new(int use_thread);
void qtk_errcode_delete(qtk_errcode_t *ec);
void qtk_errcode_set_handler(qtk_errcode_t *ec,
		void *handler_ths,
		qtk_errcode_handler handler
		);

void qtk_errcode_set_level(qtk_errcode_t *ec,qtk_errcode_level_t level);
void qtk_errcode_set_log(qtk_errcode_t *ec,wtk_log_t *log);

void qtk_errcode_feed(qtk_errcode_t *ec,qtk_errcode_level_t level,int code, char *extraErrdata, int len);
int qtk_errcode_read(qtk_errcode_t *ec,qtk_errcode_level_t *level,int *code);
char* qtk_errcode_tostring(qtk_errcode_t *ec,int code);

#ifdef __cplusplus
};
#endif
#endif
