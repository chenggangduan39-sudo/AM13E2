#include "qtk_uploadc.h" 
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <errno.h>

static qtk_uploadc_msg_t* qtk_uploadc_msg_new(qtk_uploadc_t *upc);
static int qtk_uploadc_msg_delete(qtk_uploadc_msg_t *msg);
static qtk_uploadc_msg_t* qtk_uploadc_pop_msg(qtk_uploadc_t *upc);
static void qtk_uploadc_push_msg(qtk_uploadc_t *upc,qtk_uploadc_msg_t *msg);
static void qtk_uploadc_clean_q(qtk_uploadc_t *upc);
static int qtk_uploadc_snd_run_send(qtk_uploadc_t *upc, wtk_thread_t *thread);
static int qtk_uploadc_snd_run_recv(qtk_uploadc_t *upc, wtk_thread_t *thread);
static void qtk_uploadc_connect(qtk_uploadc_t *upc,int timeout);
static void qtk_uploadc_send(qtk_uploadc_t *upc,char *data,int bytes);
static int qtk_uploadc_send2(qtk_uploadc_t *upc,char *data,int bytes);
static void qtk_uploadc_disconnect(qtk_uploadc_t *upc);
int qtk_logsize_check(wtk_log_t *log);

static long fail_to_upc = 0;

qtk_uploadc_t* qtk_uploadc_new(qtk_uploadc_cfg_t *cfg,int buf_size)
{
	qtk_uploadc_t *upc;

	upc = (qtk_uploadc_t*)wtk_malloc(sizeof(*upc));
	if(!upc) {
		return NULL;
	}
	memset(upc,0,sizeof(*upc));

	upc->cfg = cfg;
	upc->buf_size = buf_size;
	upc->fd = -1;
	upc->parser = wtk_json_parser_new();
	upc->current_state = QTK_UPC_STATE_DISCONNECT;
	wtk_lock_init(&upc->qlock);
	wtk_blockqueue_init(&upc->input_q);
	wtk_lockhoard_init(&upc->msg_hoard,offsetof(qtk_uploadc_msg_t,hoard_n),10,
			(wtk_new_handler_t)qtk_uploadc_msg_new,
			(wtk_delete_handler_t)qtk_uploadc_msg_delete,
			upc
			);
	wtk_thread_init(&upc->thread_send,(thread_route_handler)qtk_uploadc_snd_run_send,upc);
	wtk_thread_set_name(&upc->thread_send,"upload send");
	wtk_thread_init(&upc->thread_recv, (thread_route_handler)qtk_uploadc_snd_run_recv, upc);
	wtk_thread_set_name(&upc->thread_recv, "upload recv");
	return upc;
}

void qtk_uploadc_delete(qtk_uploadc_t *upc)
{
	if(upc->run_send) {
		qtk_uploadc_stop(upc);
	}
	wtk_thread_clean(&upc->thread_send);
	wtk_thread_clean(&upc->thread_recv);
	wtk_blockqueue_clean(&upc->input_q);
	wtk_lockhoard_clean(&upc->msg_hoard);
	wtk_json_parser_delete(upc->parser);
	wtk_lock_clean(&upc->qlock);
	wtk_free(upc);
}

int qtk_uploadc_start(qtk_uploadc_t *upc)
{
	qtk_uploadc_clean_q(upc);
	upc->run_send = 1;
	wtk_thread_start(&upc->thread_send);
	upc->run_recv = 1;
	wtk_thread_start(&upc->thread_recv);
	return 0;
}

int qtk_uploadc_stop(qtk_uploadc_t *upc)
{
	upc->run_send = 0;
	wtk_blockqueue_wake(&upc->input_q);
	wtk_thread_join(&upc->thread_send);
	upc->run_recv = 0;
	wtk_thread_join(&upc->thread_recv);
	qtk_uploadc_clean_q(upc);

	return 0;
}

int qtk_uploadc_feed(qtk_uploadc_t *upc,char *data,int bytes)
{
	qtk_uploadc_msg_t *msg;
	wtk_queue_node_t *qn;
	qtk_uploadc_msg_t *msg_d;

	if(upc->run_send){
		msg = qtk_uploadc_pop_msg(upc);
		if(!msg) {
			return -1;
		}
		wtk_strbuf_push(msg->buf,data,bytes);
		// wtk_debug("**** queue length : %d data len : %d\n", upc->input_q.length, bytes);
		if(upc->input_q.length >= 2000) {
			// wtk_debug("remove top of queue\n");
			wtk_lock_lock(&upc->qlock);
			qn = wtk_blockqueue_pop(&upc->input_q, 500, NULL);
			wtk_lock_unlock(&upc->qlock);
			msg_d = data_offset2(qn, qtk_uploadc_msg_t, q_n);
			fail_to_upc += msg_d->buf->pos;
			wtk_debug("**************** fail to upload %d bytes(count)\n", fail_to_upc);
			if(upc->log) {
				wtk_log_warn(upc->log, "[WARN] *** fail to upload %d bytes(count)\n", fail_to_upc);
				qtk_logsize_check(upc->log);
			}
			qtk_uploadc_push_msg(upc, msg_d);
		}
		wtk_blockqueue_push(&upc->input_q,&msg->q_n);
	}

	return 0;
}

void qtk_uploadc_set_notify(qtk_uploadc_t *upc, void *ths, qtk_uploadc_notify_f notify)
{
	upc->ths = ths;
	upc->notify = notify;
}
static void qtk_uploadc_connect(qtk_uploadc_t *upc,int timeout)
{
	struct sockaddr_in inaddr;
	struct timeval tv;
	fd_set set;
	int sockfd = -1;
	int ret;
	int reuse = 1;
	int so_error = -1;
	int so_option = sizeof(int);

	memset(&inaddr,0,sizeof(inaddr));
	inaddr.sin_family = AF_INET;
	inaddr.sin_port = htons((unsigned short)atoi(upc->cfg->port_buf->data));
	upc->cfg->host_buf->data[upc->cfg->host_buf->pos] ='\0';

	inet_pton(AF_INET,upc->cfg->host_buf->data,&inaddr.sin_addr);

	sockfd =  socket(AF_INET,SOCK_STREAM,0);
	if(sockfd < 0) {
		ret = -1;
		if(upc->log) {
			wtk_log_warn0(upc->log, "[WARN] *** socket failed");
		}
		goto end;
	}

	if(timeout <= 0) {
		ret = connect(sockfd,(struct sockaddr*)&inaddr,sizeof(struct sockaddr_in));
		if(ret != 0) {
			wtk_debug("ret = %d  err: %s\n", ret, strerror(errno));
			if(upc->log) {
				wtk_log_err(upc->log, "[ERROR] >>> connect host error : %s", strerror(errno));
			}
			goto end;
		}
	} else {
		wtk_fd_set_nonblock(sockfd);
		ret = connect(sockfd,(struct sockaddr*)&inaddr,sizeof(struct sockaddr_in));
		if(ret == 0) {
			// wtk_fd_set_block(sockfd);
		} else if (errno == EINPROGRESS) {
			tv.tv_sec = timeout / 1000;
			tv.tv_usec = (timeout % 1000) * 1000;
			FD_ZERO(&set);
			FD_SET(sockfd,&set);
			if(select(sockfd+1,NULL,&set,NULL,&tv) > 0) {
				ret = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char *)&so_error, &so_option);
				if(ret == -1) {
					if(upc->log) {
						wtk_log_err(upc->log, "[ERROR] >>> get socket option error : %s", strerror(errno));
					}
					goto end;
				}
				if(so_error) {
					errno = so_error;
					if(upc->log) {
						wtk_log_err(upc->log, "[ERROR] >>> connect host timeout error : %s", strerror(errno));
					}
					ret = -1;
					goto end;
				} else {
					ret = 0;
				}
			} else {
				ret = -1;
				goto end;
			}
		}
		wtk_fd_set_block(sockfd);
	}

	ret = setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(char*)&reuse,sizeof(reuse));
	if(ret != 0) {
		goto end;
	}
	if(timeout > 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
//		setsockopt(sockfd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
//		setsockopt(sockfd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
	}

end:
	if(ret == 0) {
		upc->fd = sockfd;
		upc->current_state = QTK_UPC_STATE_CONNECT;
		if(upc->log) {
			wtk_log_log(upc->log, "[DEBUG] === build connection with host, fd = %d", upc->fd);
		}
		if(upc->notify){
			upc->notify(upc->ths, QTK_UPC_STATE_CONNECT, NULL, 0);
		}
	} else if(sockfd > 0) {
		if(upc->log) {
			wtk_log_err(upc->log, "[ERROR] >>> build connection failed, cancel fd = %d", sockfd);
		}
		if(upc->notify){
			upc->notify(upc->ths, QTK_UPC_STATE_DISCONNECT, NULL, 0);
		}
		close(sockfd);
		upc->fd = -1;
	}
}

static void qtk_uploadc_disconnect(qtk_uploadc_t *upc)
{
	wtk_debug("************** disconnect\n");
	if(upc->log) {
		wtk_log_log0(upc->log, "[DEBUG] === device disconnect to pc");
	}
	close(upc->fd);
	upc->fd = -1;
	upc->current_state = QTK_UPC_STATE_DISCONNECT;
	if(upc->notify){
		upc->notify(upc->ths, QTK_UPC_STATE_DISCONNECT, NULL, 0);
	}
}

int qtk_uploadc_reconnect(qtk_uploadc_t *upc)
{
	qtk_uploadc_disconnect(upc);
	return 0;
}

int qtk_uploadc_is_connect(qtk_uploadc_t *upc)
{
	while(1)
	{
		qtk_uploadc_connect(upc,upc->cfg->timeout);
		if(upc->fd < 0) {
			usleep(1000*100);
			//wtk_debug("connect failed\n");
			continue;
		}else{
			break;
		}
	}
}

static void qtk_uploadc_send(qtk_uploadc_t *upc,char *data,int bytes)
{
	int writed;
	int ret;

	writed = 0;
	while(writed < bytes) {
		ret = send(upc->fd,data+writed,bytes-writed,0);
		if(ret <= 0) {
			if(errno == EINTR || errno == 0) {
				continue;
			}
			break;
		}
		writed += ret;
	}

	if(writed != bytes) {
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	if(ret != 0) {
		close(upc->fd);
		upc->fd = -1;
	}
}

static int  qtk_uploadc_send2(qtk_uploadc_t *upc,char *data,int bytes)
{
	int ret;
	// fd_set wset;
	// struct timeval tv;

	// FD_ZERO(&wset);
	// FD_SET(upc->fd, &wset);
	// tv.tv_sec = 5;
	// tv.tv_usec = 0;

	// while(select(upc->fd+1,NULL,&wset,NULL,&tv) == 0)
	// {
	// 	printf("**************** send timeout and discard data \n");
	// 	if(upc->input_q.length >= 1000) {
	// 		return 0;
	// 	}
	// }
	// wtk_msleep(5000);
	ret = wtk_fd_write_string(upc->fd,data,bytes);
	return ret;
}


static int qtk_uploadc_snd_run_send(qtk_uploadc_t *upc, wtk_thread_t *thread)
{
	qtk_uploadc_msg_t *msg = NULL;
	wtk_queue_node_t *qn = NULL;
	int ret;

	while(upc->run_send) {
		wtk_lock_lock(&upc->qlock);
		qn = wtk_blockqueue_pop(&upc->input_q,-1,NULL);
		wtk_lock_unlock(&upc->qlock);
		if(!qn) {
			continue;
		}
		msg = data_offset2(qn,qtk_uploadc_msg_t,q_n);
		if(upc->fd < 0) {
			wtk_log_log(upc->log, "[DEBUG] === send run pop a msg, upc->fd = %d\n", upc->fd);
			if(upc->log) {
				qtk_logsize_check(upc->log);
			}
		}
		if(upc->fd < 0) {
			qtk_uploadc_connect(upc,upc->cfg->timeout);
			if(upc->fd < 0) {
				wtk_debug("connect failed\n");
				qtk_uploadc_push_msg(upc,msg);
				continue;
			}
			wtk_debug(">>>>>>>>>>>>>>>>>>>>>>>>connect success\n");
		}
		if(upc->cfg->send_string){
			ret = qtk_uploadc_send2(upc,msg->buf->data,msg->buf->pos);
			if(ret != 0){
				wtk_debug(">>>>>>>>>>>>>send failed\n");
				qtk_uploadc_disconnect(upc);
				usleep(100);
			}
		}else{
			qtk_uploadc_send(upc,msg->buf->data,msg->buf->pos);
		}
		qtk_uploadc_push_msg(upc,msg);
	}
	qtk_uploadc_disconnect(upc);

	return 0;
}

static int qtk_uploadc_snd_run_recv(qtk_uploadc_t *upc, wtk_thread_t *thread)
{
	wtk_json_item_t *type = NULL;
	wtk_string_t *v = NULL;
	char buf[READ_BUF_SIZE];
	int ret;
	fd_set rdset;
	struct timeval tv;
	qtk_uploadc_recv_data_type_t data_type = -1;

	while(upc->run_recv){
		tv.tv_sec = 0;
		tv.tv_usec = 500*1000;
		FD_ZERO(&rdset);
		FD_SET(upc->fd, &rdset);
		ret = select(upc->fd + 1, &rdset, NULL, NULL, &tv);
		if(ret <= 0){
			if(errno == EPROTO || errno == EINTR || errno == EWOULDBLOCK || errno == ECONNABORTED){
				continue;
			}
		}
		if(ret > 0){
			if(upc->cfg->send_string && upc->fd > 0){
				v = wtk_fd_read_string(upc->fd);
				if(v){
					// wtk_debug("upc recv: %.*s\n", v->len, v->data);
					wtk_json_parser_reset(upc->parser);
					wtk_json_parser_parse(upc->parser, v->data, RECV_DATA_HDR);
					type = wtk_json_obj_get_s(upc->parser->json->main, "type");
					if(type && type->type == WTK_JSON_NUMBER) {
						data_type = (int)type->v.number;
					}
					switch(data_type){
						case QTK_UPC_RECV_DATA_PMT:
							if(upc->notify){
								upc->notify(upc->ths, QTK_UPC_STATE_RECV_PMT, v->data, v->len);
							}
							break;
						case QTK_UPC_RECV_DATA_ORD:
							if(upc->notify) {
								upc->notify(upc->ths, QTK_UPC_STATE_SEND_PMT, NULL, 0);
							}
							break;
						case QTK_UPC_RECV_DATA_OTA:
							if(upc->notify){
								upc->notify(upc->ths, QTK_UPC_STATE_RECV_OTA, v->data + RECV_DATA_HDR, v->len - RECV_DATA_HDR);
							}
							break;
					}
					wtk_string_delete(v);
				}else{
					qtk_uploadc_reconnect(upc);
					qtk_uploadc_is_connect(upc);
				}
			}else{
				ret = recv(upc->fd, buf, READ_BUF_SIZE, 0);
				if(ret > 0){
					wtk_debug("upc recv: %.*s\n", ret, buf);
				}
			}
		}
		
	}
	return 0;
}

static qtk_uploadc_msg_t* qtk_uploadc_msg_new(qtk_uploadc_t *upc)
{
	qtk_uploadc_msg_t *msg;

	msg = (qtk_uploadc_msg_t*)wtk_malloc(sizeof(*msg));
	if(!msg) {
		return NULL;
	}
	msg->buf = wtk_strbuf_new(upc->buf_size,1);
	if(!msg->buf) {
		wtk_free(msg);
		return NULL;
	}

	return msg;
}

static int qtk_uploadc_msg_delete(qtk_uploadc_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_uploadc_msg_t* qtk_uploadc_pop_msg(qtk_uploadc_t *upc)
{
	qtk_uploadc_msg_t *msg;

	msg = wtk_lockhoard_pop(&upc->msg_hoard);
	if(!msg) {
		return NULL;
	}
	wtk_strbuf_reset(msg->buf);
	return msg;
}

static void qtk_uploadc_push_msg(qtk_uploadc_t *upc,qtk_uploadc_msg_t *msg)
{
	wtk_lockhoard_push(&upc->msg_hoard,msg);
}

static void qtk_uploadc_clean_q(qtk_uploadc_t *upc)
{
	qtk_uploadc_msg_t *msg;
	wtk_queue_node_t *qn;

	while(1) {
		qn = wtk_blockqueue_pop(&upc->input_q,0,NULL);
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_uploadc_msg_t,q_n);
		qtk_uploadc_push_msg(upc,msg);
	}
}

void qtk_uploadc_set_log(qtk_uploadc_t *upc, wtk_log_t *log)
{
	upc->log = log;
}

int qtk_logsize_check(wtk_log_t *log)
{
    int flen = 0;

    flen = ftell(log->f);
    if(flen > 8096000) {
        wtk_log_clean(log);
        wtk_log_init(log, "./run.log");
    }
    return 0;
}