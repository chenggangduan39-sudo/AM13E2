#ifndef QTK_MISC_NK_QTK_EVENT
#define QTK_MISC_NK_QTK_EVENT

#include "wtk/core/wtk_type.h"
#include "wtk/os/wtk_log.h"
#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_event qtk_event_t;
typedef int(*qtk_event_handler)(void* data,qtk_event_t *e);

#define qtk_event_epolled(e) ((e)->in_queue)
#define qtk_event_reset_sig(e) (e)->in_queue=\
							   (e)->want_read=\
							   (e)->want_write=\
							   (e)->read=\
							   (e)->write=\
							   (e)->eof=\
							   (e)->reof=\
							   (e)->error=\
							   (e)->writepending=\
							   (e)->writeepolled=\
							   (e)->readepolled=\
							   (e)->errepolled=\
							   (e)->nk=\
							   0;

struct qtk_event
{
	void *data;
	qtk_event_handler handler;
	wtk_queue_node_t qn;
	int fd;
	unsigned in_queue:1;
	unsigned want_read:1;
	unsigned want_write:1;
	unsigned read:1;
	unsigned write:1;
	unsigned eof:1;
	unsigned reof:1;
	unsigned error:1;
	unsigned writepending:1;
	unsigned writeepolled:1;
	unsigned readepolled:1;
	unsigned errepolled:1;
	unsigned nk:1;
};

void qtk_event_print(qtk_event_t *e,wtk_log_t *log);

#ifdef __cplusplus
};
#endif
#endif
