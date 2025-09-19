#ifndef QTK_MISC_NK_QTK_NK
#define QTK_MISC_NK_QTK_NK

#include "wtk/os/wtk_pipequeue.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_log.h"
#include "qtk_event.h"
#include "module/qtk_nk_module.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_nk qtk_nk_t;

typedef void(*qtk_nk_recvd_handler)(void *user_data,int recvd);

struct qtk_nk
{
	wtk_log_t *log;
	void *handler;
	qtk_nk_recvd_handler recvd_handler;
	void *user_data;
	wtk_thread_t thread;
	int pipe_fd[2];
	qtk_event_t event;
	int looptime;
	unsigned run:1;
};

qtk_nk_t *qtk_nk_new(wtk_log_t *log,int looptime,int size);
void qtk_nk_delete(qtk_nk_t *nk);
void qtk_nk_set_recvd_handler(qtk_nk_t *nk,void *user_data,qtk_nk_recvd_handler recvd_handler);

int qtk_nk_add_event(qtk_nk_t *nk,int fd,qtk_event_t *event);
int qtk_nk_mod_event(qtk_nk_t *nk,int fd,qtk_event_t *event);
int qtk_nk_del_event(qtk_nk_t *nk,int fd,qtk_event_t *event);

#ifdef __cplusplus
};
#endif
#endif
