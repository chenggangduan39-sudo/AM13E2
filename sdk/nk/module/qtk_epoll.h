#ifndef QTK_MISC_NK_MODULE_QTK_EPOLL
#define QTK_MISC_NK_MODULE_QTK_EPOLL

#include <sys/epoll.h>
#include <errno.h>
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/os/wtk_sem.h"
#include "wtk/os/wtk_log.h"

#include "sdk/nk/qtk_event.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_epoll qtk_epoll_t;

#ifndef EPOLLRDHUP
#define EPOLLRDHUP 0
#endif

struct qtk_epoll
{
	wtk_log_t *log;
	int fd;
	int size;
	int moniter_events;
	struct epoll_event *events;
	unsigned et:1;
	unsigned bw_on_sig:1;
};

qtk_epoll_t* qtk_epoll_new(wtk_log_t *log,int size);
void qtk_epoll_del(qtk_epoll_t *e);
int qtk_epoll_run(qtk_epoll_t *e,int looptime,int *recvd);

int qtk_epoll_add_event(qtk_epoll_t *e,int fd,qtk_event_t *event);
int qtk_epoll_mod_event(qtk_epoll_t *e,int fd,qtk_event_t *event);
int qtk_epoll_del_event(qtk_epoll_t *e,int fd,qtk_event_t *event);


#ifdef __cplusplus
};
#endif
#endif
