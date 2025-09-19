#include "qtk_global_msg.h"
#include "wtk/core/wtk_os.h"
#include "wtk/os/wtk_lockhoard.h"
#include "qtk_queue.h"
#include "qtk_blockqueue.h"
#include "qtk_strbuf.h"

#define qtk_global_msg_BUFSIZE 10240

qtk_global_msg_t *msg=NULL;

typedef enum{
	QTK_GLOBAL_MSG_DATA=1,
	QTK_GLOBAL_MSG_THETA,
}qtk_global_msg_type_t;

struct qtk_global_msg_node
{
	qtk_queue_node_t qn;
	qtk_queue_node_t hoard_on;
	qtk_strbuf_t *buf;
	int theta;
	int type;
};

struct qtk_global_msg
{
	wtk_lockhoard_t msg_hoard;
	qtk_blockqueue_t msg_q;
};

qtk_global_msg_node_t* qtk_global_msg_node_new(qtk_global_msg_t *m);
void qtk_global_msg_node_delete(qtk_global_msg_node_t *msg);

qtk_global_msg_t* qtk_global_msg_new()
{
	qtk_global_msg_t *m;

	m=wtk_malloc(sizeof(*m));
	wtk_lockhoard_init(&(m->msg_hoard),offsetof(qtk_global_msg_node_t,hoard_on),300,
		(wtk_new_handler_t)qtk_global_msg_node_new,
		(wtk_delete_handler_t)qtk_global_msg_node_delete, 
		m);
	
	qtk_blockqueue_init(&(m->msg_q));

	return m;
}

void qtk_global_msg_delete(qtk_global_msg_t *m)
{
	wtk_lockhoard_clean(&(m->msg_hoard));
	qtk_blockqueue_clean(&(m->msg_q));
	wtk_free(m);
}

qtk_global_msg_t *qtk_global_msg_get_handle()
{
	if(!msg)
	{
		msg=qtk_global_msg_new();
	}
	return msg;
}

void qtk_global_msg_free()
{
	if(msg)
	{
		qtk_global_msg_delete(msg);
		msg=NULL;
	}
}

qtk_global_msg_node_t* qtk_global_msg_node_new(qtk_global_msg_t *m)
{
	qtk_global_msg_node_t *msg;

	msg=(qtk_global_msg_node_t*)wtk_malloc(sizeof(qtk_global_msg_node_t));
	msg->buf=qtk_strbuf_new(qtk_global_msg_BUFSIZE,1.0);
	msg->type = -1;
	msg->theta = -1;

	return msg;
}

void qtk_global_msg_node_delete(qtk_global_msg_node_t *msg)
{
	qtk_strbuf_delete(msg->buf);
	wtk_free(msg);
}

qtk_global_msg_node_t* qtk_global_msg_pop_node(qtk_global_msg_t *m)
{
	return  (qtk_global_msg_node_t*)wtk_lockhoard_pop(&(m->msg_hoard));
}

void qtk_global_msg_push_node(qtk_global_msg_t *m,qtk_global_msg_node_t *msg)
{
	msg->type = -1;
	if(msg->buf->length>qtk_global_msg_BUFSIZE)
	{
		wtk_strbuf_resize(msg->buf,qtk_global_msg_BUFSIZE);
	}
	wtk_strbuf_reset(msg->buf);
	wtk_lockhoard_push(&(m->msg_hoard),msg);
}

char *qtk_global_msg_pop(qtk_global_msg_t *m, int *len, int *theta)
{
	qtk_global_msg_node_t *msg_node;
	qtk_queue_node_t *qn;
	char *data=NULL;

	qn= qtk_blockqueue_pop(&m->msg_q,-1,NULL);
	if(!qn) {
		*len=-1;
		*theta=-1;
		return NULL;
	}
	msg_node = data_offset2(qn,qtk_global_msg_node_t,qn);
	
	switch (msg_node->type)
	{
	case QTK_GLOBAL_MSG_DATA:
		data = malloc(msg_node->buf->pos);
		memcpy(data, msg_node->buf->data, msg_node->buf->pos);
		break;
	case QTK_GLOBAL_MSG_THETA:
		*theta=msg_node->theta;
		*len=-1;
		break;	
	default:
		break;
	}
	qtk_global_msg_push_node(m, msg_node);
	return data;
}

int qtk_global_msg_push(qtk_global_msg_t *m, char *data, int len, int theta)
{
	qtk_global_msg_node_t *msg_node;

	msg_node = qtk_global_msg_pop_node(m);
	if(len > 0)
	{
		msg_node->type = QTK_GLOBAL_MSG_DATA;
		qtk_strbuf_push(msg_node->buf, data, len);
	}
	msg_node->type = QTK_GLOBAL_MSG_THETA;
	msg_node->theta=theta;
	qtk_blockqueue_push(&(m->msg_q), &(msg_node->qn));
	return 0;
}