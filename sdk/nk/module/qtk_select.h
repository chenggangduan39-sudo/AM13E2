#ifndef QTK_MISC_NK_MODULE_QTK_SELECT
#define QTK_MISC_NK_MODULE_QTK_SELECT

#include "sdk/nk/qtk_event.h"

#if defined(WIN32) || defined(_WIN32)
#include "winsock2.h"
#endif

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_select qtk_select_t;
struct qtk_select
{
	wtk_log_t *log;
	fd_set *read_set;
	fd_set *write_set;
	fd_set *error_set;
	int nevents;
	int max_fd;
	int monitor_event;
	wtk_queue_t event_q;
	unsigned bw_on_sig:1;
};

qtk_select_t* qtk_select_new(wtk_log_t *log,int size);
void qtk_select_del(qtk_select_t *s);

int qtk_select_run(qtk_select_t *s,int looptime,int *recvd);

int qtk_select_add_event(qtk_select_t *s,int fd,qtk_event_t *event);
int qtk_select_mod_event(qtk_select_t *s,int fd,qtk_event_t *event);
int qtk_select_del_event(qtk_select_t *s,int fd,qtk_event_t *event);

#ifdef __cplusplus
};
#endif
#endif
