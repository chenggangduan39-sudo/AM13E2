#ifndef __QTK_LOCKMSG_H__
#define __QTK_LOCKMSG_H__
#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/core/wtk_queue.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
	wtk_queue_node_t qn;
	wtk_queue_node_t hoard_on;
	wtk_strbuf_t *buf;
	int type;
	int data_type;
    int re_send;
}qtk_lockmsg_node_t;

typedef struct
{
	wtk_lockhoard_t msg_hoard;
	int bufsize;
}qtk_lockmsg_t;

qtk_lockmsg_t* qtk_lockmsg_new(int bufsize);
void qtk_lockmsg_delete(qtk_lockmsg_t *m);
qtk_lockmsg_node_t* qtk_lockmsg_pop_node(qtk_lockmsg_t *m);
void qtk_lockmsg_push_node(qtk_lockmsg_t *m,qtk_lockmsg_node_t *msg);

#ifdef __cplusplus
};
#endif
#endif
