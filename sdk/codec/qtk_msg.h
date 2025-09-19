#ifndef __QTK_MSG_H__
#define __QTK_MSG_H__
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/core/wtk_queue.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef struct
{
	wtk_queue_node_t qn;
	wtk_queue_node_t hoard_on;
	wtk_strbuf_t *buf;
	short type;
	short sil;
}qtk_msg_node_t;

typedef struct
{
	wtk_lockhoard_t msg_hoard;
}qtk_msg_t;

qtk_msg_t* qtk_msg_new();
void qtk_msg_delete(qtk_msg_t *m);
qtk_msg_node_t* qtk_msg_pop_node(qtk_msg_t *m);
void qtk_msg_push_node(qtk_msg_t *m,qtk_msg_node_t *msg);
#ifdef __cplusplus
};
#endif
#endif
