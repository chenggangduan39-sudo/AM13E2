#include "qtk_msg.h"

#define QTK_MSG_BUFSIZE 10240

qtk_msg_node_t* qtk_msg_node_new(qtk_msg_t *m);
void qtk_msg_node_delete(qtk_msg_node_t *msg);

qtk_msg_t* qtk_msg_new()
{
	qtk_msg_t *m;

	m=wtk_malloc(sizeof(*m));
	wtk_lockhoard_init(&(m->msg_hoard),offsetof(qtk_msg_node_t,hoard_on),100,
		(wtk_new_handler_t)qtk_msg_node_new,
		(wtk_delete_handler_t)qtk_msg_node_delete, 
		m);

	return m;
}

void qtk_msg_delete(qtk_msg_t *m)
{
	wtk_lockhoard_clean(&(m->msg_hoard));
	wtk_free(m);
}

qtk_msg_node_t* qtk_msg_node_new(qtk_msg_t *m)
{
	qtk_msg_node_t *msg;

	msg=(qtk_msg_node_t*)wtk_malloc(sizeof(qtk_msg_node_t));
	msg->buf=wtk_strbuf_new(QTK_MSG_BUFSIZE,1.0);
	msg->type = -1;
	msg->sil = 0;

	return msg;
}

void qtk_msg_node_delete(qtk_msg_node_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
}

qtk_msg_node_t* qtk_msg_pop_node(qtk_msg_t *m)
{
	return  (qtk_msg_node_t*)wtk_lockhoard_pop(&(m->msg_hoard));
}

void qtk_msg_push_node(qtk_msg_t *m,qtk_msg_node_t *msg)
{
	msg->type = -1;
	if(msg->buf->length>QTK_MSG_BUFSIZE)
	{
		wtk_strbuf_resize(msg->buf,QTK_MSG_BUFSIZE);
	}
	wtk_strbuf_reset(msg->buf);
	wtk_lockhoard_push(&(m->msg_hoard),msg);
}
