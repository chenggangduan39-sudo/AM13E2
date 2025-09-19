#include "qtk_lockmsg.h"

qtk_lockmsg_node_t* qtk_lockmsg_node_new(qtk_lockmsg_t *m);
void qtk_lockmsg_node_delete(qtk_lockmsg_node_t *msg);

qtk_lockmsg_t* qtk_lockmsg_new(int bufsize)
{
	qtk_lockmsg_t *m;

	m=malloc(sizeof(*m));
	m->bufsize = bufsize;
	wtk_lockhoard_init(&(m->msg_hoard),offsetof(qtk_lockmsg_node_t,hoard_on),20,
				(wtk_new_handler_t)qtk_lockmsg_node_new,
				(wtk_delete_handler_t)qtk_lockmsg_node_delete, 
				m);
	return m;
}

void qtk_lockmsg_delete(qtk_lockmsg_t *m)
{
	wtk_lockhoard_clean(&(m->msg_hoard));
	free(m);
}

qtk_lockmsg_node_t* qtk_lockmsg_node_new(qtk_lockmsg_t *m)
{
	qtk_lockmsg_node_t *msg;

	msg=(qtk_lockmsg_node_t*)malloc(sizeof(qtk_lockmsg_node_t));
	if(!msg){
		wtk_debug("******************failed\r\n");
		return NULL;
	}
	msg->buf=wtk_strbuf_new(m->bufsize, 1.0);
	if(!msg->buf){
		wtk_debug("******************failed %d\r\n", m->bufsize);
		free(msg);
		return NULL;
	}
	msg->type = -1;
	msg->data_type = -1;
    msg->re_send = 0;
	return msg;
}

void qtk_lockmsg_node_delete(qtk_lockmsg_node_t *msg)
{
	/* wtk_debug(">>>>>>>>>>>>>>>>>>>>>>>>>>>node delete, buf_length = %d\r\n", msg->buf->length); */
	wtk_strbuf_delete(msg->buf);
	free(msg);
}

qtk_lockmsg_node_t* qtk_lockmsg_pop_node(qtk_lockmsg_t *m)
{ 
    //wtk_debug("[%p] pop cur_free = %d  max_free = %d \r\n", m, m->msg_hoard.cur_free, m->msg_hoard.max_free);
	return  (qtk_lockmsg_node_t*)wtk_lockhoard_pop(&(m->msg_hoard));
}

void qtk_lockmsg_push_node(qtk_lockmsg_t *m,qtk_lockmsg_node_t *msg)
{
	wtk_strbuf_reset(msg->buf);
	msg->type = -1;
	msg->data_type = -1;
    msg->re_send = 0;
	wtk_lockhoard_push(&(m->msg_hoard),msg);
    /* wtk_debug("[%p] push cur_free = %d  max_free = %d \r\n", m, m->msg_hoard.cur_free, m->msg_hoard.max_free); */
}
