#include "qtk_httpc_request.h"

static int qtk_httpc_request_on_in(qtk_httpc_request_t *req,qtk_event_t *event);
static int qtk_httpc_request_on_http(qtk_httpc_request_t *req,qtk_event_t *event);

static qtk_httpc_request_msg_t* qtk_httpc_request_msg_new(qtk_httpc_request_t *req)
{
	qtk_httpc_request_msg_t *msg;

	msg=(qtk_httpc_request_msg_t*)wtk_malloc(sizeof(qtk_httpc_request_msg_t));
	msg->buf=wtk_strbuf_new(8192,1);
	return msg;
}

static int qtk_httpc_request_msg_delete(qtk_httpc_request_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_httpc_request_msg_t* qtk_httpc_request_pop_msg(qtk_httpc_request_t *req)
{
	qtk_httpc_request_msg_t *msg;

	msg=(qtk_httpc_request_msg_t*)wtk_lockhoard_pop(&(req->msg_hoard));
	wtk_strbuf_reset(msg->buf);
	return msg;
}

static void qtk_httpc_request_push_msg(qtk_httpc_request_t *req,qtk_httpc_request_msg_t *msg)
{
	wtk_lockhoard_push(&(req->msg_hoard),msg);
}

static void qtk_httpc_request_add_in_event(qtk_httpc_request_t *req)
{
	req->in_event.want_read = 1;
	req->in_event.fd = req->input_q.pipe_fd[0];
	req->in_event.data = req;
	req->in_event.handler = (qtk_event_handler)qtk_httpc_request_on_in;
	qtk_nk_add_event(req->nk,req->input_q.pipe_fd[0],&(req->in_event));
}

static void qtk_httpc_request_del_in_event(qtk_httpc_request_t *req)
{
	qtk_nk_del_event(req->nk,req->input_q.pipe_fd[0],&(req->in_event));
	qtk_event_reset_sig(&req->in_event);
	req->in_event.fd = -1;
}

static void qtk_httpc_request_add_http_event(qtk_httpc_request_t *req)
{
	req->http_event.want_read = 1;
	req->http_event.nk = 1;
	req->http_event.fd = req->fd;
	req->http_event.data = req;
	req->http_event.handler = (qtk_event_handler)qtk_httpc_request_on_http;
	qtk_nk_add_event(req->nk,req->fd,&req->http_event);
}

static void qtk_httpc_request_del_http_event(qtk_httpc_request_t *req)
{
	qtk_nk_del_event(req->nk,req->fd,&(req->http_event));
	qtk_event_reset_sig(&req->http_event);
	req->http_event.fd = -1;
}

void qtk_httpc_request_input_clean(qtk_httpc_request_t *req)
{
	wtk_queue_node_t *qn;
	qtk_httpc_request_msg_t *msg;

	while(1) {
		qn = wtk_pipequeue_pop(&(req->input_q));
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_httpc_request_msg_t,q_n);
		qtk_httpc_request_push_msg(req,msg);
	}
}

static void qtk_httpc_request_init(qtk_httpc_request_t *req)
{
	req->cfg = NULL;
	req->nk  = NULL;
	req->session = NULL;

	req->fd = INVALID_FD;
	req->dns = NULL;

	req->wstack  = NULL;

	req->handler     = NULL;
	req->handler_ths = NULL;

	req->auth_handler_ths = NULL;
	req->auth_handler     = NULL;

	req->response = NULL;

	req->last_recved = time_get_ms();

	qtk_event_reset_sig(&req->in_event);
	qtk_event_reset_sig(&req->http_event);

	req->auth_req = 0;
	req->err_notify = NULL;
	req->err_ths = NULL;
}

static int qtk_httpc_request_disconnect(qtk_httpc_request_t *req,int syn)
{
	int ret;

	if(req->nk && req->http_event.in_queue) {
		qtk_httpc_request_del_http_event(req);
	}

#ifdef WIN32
	ret = closesocket(req->fd);
#else
	ret = close(req->fd);
#endif
	req->fd = INVALID_FD;

	return ret;
}

#ifdef WIN32
int qtk_httpc_request_set_opt(int fd) {
        int ret = -1;
        int keepalive = 1;
        int reuse = 1;
        int ul = 1;

        if (ioctlsocket(fd, FIONBIO, &ul) == SOCKET_ERROR) {
                goto end;
        }

        ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                         sizeof(reuse));
        if (ret != 0) {
                goto end;
        }

        ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char *)(&keepalive),
                         (socklen_t)sizeof(keepalive));
        if (ret != 0) {
                goto end;
        }
        {
                int tcp_nodelay = 1;
                ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
                                 (void *)&tcp_nodelay, sizeof(int));
        }

        ret = 0;
end:
        return ret;
}
#else
int qtk_httpc_request_set_opt(int fd)
{
    int ret = -1;
    int keepalive = 1;
    int reuse = 1;

    if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK)==-1) {
    	goto end;
    }

    ret = setsockopt(fd,SOL_SOCKET, SO_REUSEADDR, (char*) &reuse,sizeof(reuse));
    if(ret!=0) {
    	goto end;
    }

    ret = setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, (char*) (&keepalive),(socklen_t) sizeof(keepalive));
    if(ret!=0) {
    	goto end;
    }

    {
        int keepalive_time = 3, keepalive_intvl = 3, keepalive_probes = 2;

        ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, (void*) (&keepalive_time),(socklen_t) sizeof(keepalive_time));
        if (ret != 0) {
        	goto end;
        }
        ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL,(void*) (&keepalive_intvl), (socklen_t) sizeof(keepalive_intvl));
        if (ret != 0) {
        	goto end;
        }
        ret = setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, (void*) (&keepalive_probes),(socklen_t) sizeof(keepalive_probes));
    }
    {
        int tcp_nodelay = 1;
        ret = setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (void *)&tcp_nodelay, sizeof(int));
    }

    ret = 0;
end:
    return ret;
}
#endif

static int qtk_httpc_request_connect(qtk_httpc_request_t *req)
{
	qtk_dns_t *dns = req->dns;
	struct timeval tv;
	fd_set set;
	int i;
	int ret = -1;

	wtk_log_log(req->session->log,"request host/port/url = [%.*s] / [%.*s] / [%.*s] .",
				req->cfg->host.len,req->cfg->host.data,
				req->cfg->port.len,req->cfg->port.data,
				req->cfg->url.len,req->cfg->url.data
				);

	if(req->cfg->host.len <= 0) {
		wtk_log_warn0(req->session->log,"request host not be seted.");
		goto end;
	}

	ret = qtk_dns_process(dns,req->cfg->host.data,req->cfg->host.len,req->cfg->port.data,req->cfg->port.len);
	if(ret != 0) {
		wtk_log_warn(req->session->log,"request %s:%s not found.",req->cfg->host.data,req->cfg->port.data);
		_qtk_warning(req->session,_QTK_DNS_PARSE_FAILED);
		goto end;
	}

	wtk_log_log(req->session->log,"sa_family = %d",dns->addr->sa_family);
	switch(dns->addr->sa_family) {
	case AF_INET:
		req->fd = socket(AF_INET,SOCK_STREAM,0);
		break;
	case AF_INET6:
		req->fd = socket(AF_INET6,SOCK_STREAM,0);
		break;
	default:
		wtk_log_warn(req->session->log,"request %s:%s  socktype = %d.",req->cfg->host.data,req->cfg->port.data,dns->addr->sa_family);
		break;
	}

	if(req->fd == INVALID_FD) {
		wtk_log_err0(req->session->log,"request socket init failed.\n");
		_qtk_warning(req->session,_QTK_SOCKET_NEW_FAILED);
		goto end;
	}

	if(req->cfg->timeout < 0) {
		ret = connect(req->fd,dns->addr,dns->addrlen);
	} else {
		wtk_fd_set_nonblock(req->fd);
		ret = connect(req->fd,dns->addr,dns->addrlen);

#ifdef WIN32
		int err = WSAGetLastError();
		if (ret == 0) {
			wtk_fd_set_block(req->fd);
		} else if (err == WSAEWOULDBLOCK) {
			tv.tv_sec = req->cfg->timeout / 1000;
			tv.tv_usec = (req->cfg->timeout % 1000) * 1e3;
			FD_ZERO(&set);
			FD_SET(req->fd, &set);
			if (select(req->fd + 1, NULL, &set, NULL, &tv) > 0) {
				wtk_fd_set_block(req->fd);
				ret = 0;
			} else {
				ret = -1;
			}
		}
#else
        if(ret == 0) {
			wtk_fd_set_block(req->fd);
		} else if (errno == EINPROGRESS) {
			tv.tv_sec = req->cfg->timeout / 1000;
			tv.tv_usec = (req->cfg->timeout % 1000) *1e3;
			FD_ZERO(&set);
			FD_SET(req->fd,&set);
			if(select(req->fd+1,NULL,&set,NULL,&tv) > 0) {
				wtk_fd_set_block(req->fd);
				ret = 0;
			}else{
				ret = -1;
			}
		}
#endif

    }

	if(ret != 0) {
		wtk_log_warn0(req->session->log,"connect failed");
		_qtk_warning(req->session,_QTK_HTTP_CONNECT_FAILED);
		goto end;
	}

	if(req->nk) {
		qtk_httpc_request_set_opt(req->fd);
		qtk_httpc_request_add_http_event(req);
	} else if(req->cfg->timeout > 0) {
#ifdef WIN32
		setsockopt(req->fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&(req->cfg->timeout), sizeof(int));
		setsockopt(req->fd, SOL_SOCKET, SO_SNDTIMEO, (char*)&(req->cfg->timeout), sizeof(int));
#else
		tv.tv_sec = req->cfg->timeout / 1000;
		tv.tv_usec = (req->cfg->timeout % 1000) * 1e3;
		setsockopt(req->fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		setsockopt(req->fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
	}

#if 1
	if (req->auth_handler && !req->auth_req) {
		req->auth_req = 1;
		ret = req->auth_handler(req->auth_handler_ths);
		req->auth_req = 0;
		if (ret != 0) {
			wtk_log_warn(req->session->log, "auth ret = %d", ret);
			_qtk_warning(req->session, _QTK_HTTP_AUTH_FAILED);
			goto end;
		}
	}
#endif

    ret = 0;
end:
	if(ret != 0 && req->fd > 0) {
		qtk_httpc_request_disconnect(req,0);
	}
	return ret;
}

qtk_httpc_request_t* qtk_httpc_request_new(qtk_httpc_cfg_t *cfg,qtk_nk_t *nk,qtk_session_t *session)
{
	qtk_httpc_request_t *req;
	int ret;

	req = (qtk_httpc_request_t*)wtk_malloc(sizeof(qtk_httpc_request_t));
	qtk_httpc_request_init(req);

	req->cfg    = cfg;
	req->nk     = nk;
	req->session = session;

	req->dns = qtk_dns_new(&cfg->dns,session->log);

	if(req->nk) {
		wtk_lockhoard_init(&(req->msg_hoard),offsetof(qtk_httpc_request_msg_t,hoard_n),20,
						(wtk_new_handler_t)qtk_httpc_request_msg_new,
						(wtk_delete_handler_t)qtk_httpc_request_msg_delete,
						req);
		wtk_pipequeue_init(&(req->input_q));
		qtk_httpc_request_add_in_event(req);
		req->wstack = wtk_stack_new(4096,8192,1);
	}

	req->response = qtk_http_response_new(session->log);
	qtk_http_response_reset(req->response);

    ret = qtk_httpc_request_connect(req);
    if(ret != 0) {
		wtk_log_warn(req->session->log,"request connect failed and fd = %d.",req->fd);
	}

	return req;
}

void qtk_httpc_request_delete(qtk_httpc_request_t *req)
{
	if(req->nk) {
		qtk_httpc_request_del_in_event(req);
		wtk_pipequeue_clean(&(req->input_q));
		wtk_lockhoard_clean(&(req->msg_hoard));
		wtk_stack_delete(req->wstack);
	}
	if(req->fd != INVALID_FD) {
		qtk_httpc_request_disconnect(req,0);
	}

	if(req->response) {
		qtk_http_response_delete(req->response);
	}

	if(req->dns) {
		qtk_dns_delete(req->dns);
	}

	wtk_free(req);
}

void qtk_httpc_request_set_handler(qtk_httpc_request_t *req,void *handler_ths,qtk_httpc_request_handler_f handler)
{
	req->handler = handler;
	req->handler_ths = handler_ths;
}

void qtk_httpc_request_set_auth_handler(qtk_httpc_request_t *req,
		void *auth_handler_ths,
		qtk_httpc_request_auth_handler_f auth_handler)
{
	int ret;
	req->auth_handler     = auth_handler;
	req->auth_handler_ths = auth_handler_ths;

	if(req->fd != INVALID_FD) {
		req->auth_req = 1;
		ret = req->auth_handler(req->auth_handler_ths);
		req->auth_req = 0;
		if(ret != 0) {
			_qtk_warning(req->session,_QTK_HTTP_AUTH_FAILED);
			qtk_httpc_request_disconnect(req,0);
		}
	}
}

void qtk_httpc_request_connect_reset(qtk_httpc_request_t *req)
{
	qtk_httpc_request_disconnect(req,0);
	if(req->nk) {
		qtk_httpc_request_input_clean(req);
		wtk_stack_reset(req->wstack);
		qtk_http_response_reset(req->response);
	}
}

static void qtk_httpc_request_notify_rep(qtk_httpc_request_t *req)
{
	if(req->response->status != 200 && req->response->status != 100) {
		wtk_log_warn(req->session->log,"reponse status == %d [%.*s]",req->response->status,
					req->response->body->pos,req->response->body->data);
	}

	if(req->handler) {
		req->handler(req->handler_ths,req->response);
	} else {
		qtk_http_response_print(req->response);
	}
}

static int qtk_httpc_request_reconnect(qtk_httpc_request_t *req)
{
	int ret;
	ret = qtk_httpc_request_connect(req);
	return ret;
}

static int qtk_httpc_request_req(qtk_httpc_request_t *req,char *data, int bytes)
{
	char buf[QTK_HTTPC_REQUEST_RCV_BUFSIZE];
	int ret;
	int writed;
	int readed;
	int left,done;
	int trys;

	writed = 0;
	while(writed < bytes) {
		ret = send(req->fd,data+writed,bytes-writed,0);
		if(ret <= 0) {
#ifdef WIN32
			int err = WSAGetLastError();
			if (err == WSAEINTR || err == 0) {
#else
			if(errno == EINTR || errno == 0) {
#endif
				continue;
			}
			break;
		}
		writed += ret;
	}

	if(writed != bytes) {
		wtk_log_warn(req->session->log,"send data failed.writed / bytes %d / %d.\n",writed,bytes);
		_qtk_warning(req->session,_QTK_HTTP_SND_FAILED);
		ret = -1;
		goto end;
	}

    qtk_http_response_reset(req->response);
    done = 0;
	readed = 0;
	trys = 3;
	while(!done) {
        ret = recv(req->fd, buf, QTK_HTTPC_REQUEST_RCV_BUFSIZE, 0);
        if(ret <= 0) {
#ifdef WIN32
			int err = WSAGetLastError();
			if (err == WSAEINTR || err == WSAEINPROGRESS) {
				continue;
			} else if (err == WSAEWOULDBLOCK && trys > 0) {
				--trys;
				continue;
			}
#else
            if(errno == EINTR || errno == EINPROGRESS) {
				continue;
			} else if(errno == EAGAIN && trys > 0) {
				--trys;
				continue;
			}
#endif
            break;
		}

        // wtk_debug("recv = [%.*s]\n",ret,buf);
        readed += ret;
		ret = qtk_http_response_feed(req->response,buf,ret,&left,&done);

		if(ret != 0) {
			_qtk_warning(req->session,_QTK_HTTP_RESPONSE_FMTERR);
			goto end;
		}

		if(!done && left > 0) {
			_qtk_warning(req->session,_QTK_HTTP_RESPONSE_FMTERR);
			ret = -1;
			goto end;
		}

		if(done) {
			qtk_httpc_request_notify_rep(req);
		} else if (req->cfg->use_stage && req->response->body->pos > 0) {
			qtk_httpc_request_notify_rep(req);
		}
	}

	if(readed <= 0) {
		_qtk_warning(req->session,_QTK_HTTP_RCV_FAILED);
		ret = -1;
		goto end;
	}

	if(!done) {
		_qtk_warning(req->session,_QTK_HTTP_RESPONSE_FMTERR);
		ret = -1;
		goto end;
	}
	ret = 0;
end:
	if(ret != 0) {
		qtk_httpc_request_disconnect(req,0);
		qtk_http_response_reset(req->response);
	}

    return ret;
}

int qtk_httpc_request_feed(qtk_httpc_request_t *req,char *data,int bytes)
{
	qtk_httpc_request_msg_t *msg;
	int ret = 0;
	if(req->fd == INVALID_FD) {
		if(req->err_notify){
			wtk_log_log0(req->session->log,"feed err_notify begin");
			req->err_notify(req->err_ths);
			wtk_log_log0(req->session->log,"feed err_notify end");
		}
		// feed断网会阻塞处理回调,所以iasr_route那里会堆积一点数据
		return -1;
    }

    if(req->nk) {
		msg = qtk_httpc_request_pop_msg(req);
		wtk_strbuf_push(msg->buf,data,bytes);
		wtk_pipequeue_push(&(req->input_q),&(msg->q_n));
	} else {
		ret = qtk_httpc_request_req(req,data,bytes);
	}
	return ret;
}

static void qtk_httpc_request_touch_epoll(qtk_httpc_request_t *req)
{
	qtk_event_t *event = &(req->http_event);

	if(event->writepending) {
		if(!event->writeepolled) {
			qtk_nk_mod_event(req->nk,req->fd,event);
		}
	} else {
		if(event->writeepolled) {
			qtk_nk_mod_event(req->nk,req->fd,event);
		}
	}
}

static int qtk_httpc_request_flush(qtk_httpc_request_t *req)
{
	wtk_stack_t *wstack = req->wstack;
	stack_block_t *b;
	int ret;
	int writed;
	int count;
	int len;

	count = 0;
	for(b=wstack->pop;b;b=b->next){
        len=b->push - b->pop;
        if(len <= 0) {
        	continue;
        }
        writed = 0;
        while(writed < len) {
        	ret = send(req->fd,b->pop+writed,len-writed,0);
        	if(ret <= 0) {
#ifdef WIN32
				int err = WSAGetLastError();
				if (err == WSAEINTR || err == 0) {
#else
				if (errno == EINTR || errno == 0) {
#endif
					continue;
				}
				break;
        	}
            writed += ret;
        }
        count += writed;
        if(writed < len) {
        	break;
        }
	}
	if(count > 0) {
		wtk_stack_pop(wstack,0,count);
	}

	return count;
}

static int qtk_httpc_request_on_in(qtk_httpc_request_t *req,qtk_event_t *event)
{
	wtk_queue_node_t *qn;
	qtk_httpc_request_msg_t *msg;
	wtk_stack_t *wstack = req->wstack;
	int ret;
	int writed;

	if(req->fd == INVALID_FD) {
		return -1;
	}

	while(1) {
		qn = wtk_pipequeue_pop(&(req->input_q));
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_httpc_request_msg_t,q_n);

		if(msg->buf->pos <= 0) {
			qtk_httpc_request_push_msg(req,msg);
			continue;
		}

        // wtk_debug("send = [%.*s]\n",msg->buf->pos,msg->buf->data);
        if(wstack->len > 0) {
			wtk_stack_push(wstack,msg->buf->data,msg->buf->pos);
			ret = qtk_httpc_request_flush(req);
			if(ret <= 0) {
				wtk_log_warn(req->session->log,"request flush ret = %d.",ret);
				qtk_httpc_request_push_msg(req,msg);
				ret=-1;
				goto end;
			}
		} else {
			writed = 0;
			while(writed < msg->buf->pos) {
				ret = send(req->fd,msg->buf->data+writed,msg->buf->pos-writed,0);
				if(ret < 0) {
#ifdef WIN32
					int err = WSAGetLastError();
					if (err == WSAEINTR || err == 0) {
#else
					if (errno == EINTR || errno == 0) {
#endif
						continue;
					}
					break;
				}
				writed += ret;
			}

			if(writed < msg->buf->pos) {
				//当发送缓冲区满时，便会缓存数据
				wtk_log_warn(req->session->log,"network bad, write:buf_pos==%d:%d.",writed,msg->buf->pos);
				wtk_stack_push(wstack,msg->buf->data+writed,msg->buf->pos-writed);
			}
		}
		qtk_httpc_request_push_msg(req,msg);
	}

	req->http_event.writepending = (wstack->len > 0);
	qtk_httpc_request_touch_epoll(req);

	ret = 0;
end:
	if(ret != 0) {
		qtk_httpc_request_disconnect(req,1);
		qtk_httpc_request_input_clean(req);
		wtk_stack_reset(req->wstack);
		qtk_http_response_reset(req->response);
	}
	return ret;
}

static int qtk_httpc_request_read(qtk_httpc_request_t *req)
{
	char buf[QTK_HTTPC_REQUEST_RCV_BUFSIZE];
	int ret;
	int left,done;
	int readed=0;
	int len;

	while(1) {
		ret = recv(req->fd,buf,QTK_HTTPC_REQUEST_RCV_BUFSIZE,0);
		if(ret <= 0) {
#ifdef WIN32
			int err = WSAGetLastError();
			if (err == WSAEINTR || err == 0) {
#else
			if(errno == EINTR || errno == 0) {
#endif
				continue;
			}
			break;
		}

        // wtk_debug("recvd = [%.*s]\n",ret,buf);

        readed += ret;
		left = ret;
		len = ret;
		while(left > 0) {
			ret = qtk_http_response_feed(req->response,buf+len-left,left,&left,&done);
			if(ret != 0) {
				wtk_log_warn(req->session->log,"response format illegal. state = %d.",req->response->state);
				_qtk_warning(req->session,_QTK_HTTP_RESPONSE_FMTERR);
				goto end;
			}

			if(!done && left > 0) {
				wtk_log_warn0(req->session->log,"response format illegal.not done and has left.");
				_qtk_warning(req->session,_QTK_HTTP_RESPONSE_FMTERR);
				ret = -1;
				goto end;
			}

			if(done) {
				qtk_httpc_request_notify_rep(req);
				qtk_http_response_reset(req->response);
			} else if (req->cfg->use_stage && req->response->body->pos > 0) {
				qtk_httpc_request_notify_rep(req);
			}
		}
	}

	if(readed <= 0) {
		_qtk_warning(req->session,_QTK_HTTP_RCV_FAILED);
		ret = -1;
		goto end;
	}

	ret = 0;
end:
    return ret;
}

static int qtk_httpc_request_on_http(qtk_httpc_request_t *req,qtk_event_t *event)
{
	int ret;
	int close = 0;

	if(event->read || event->reof) {
		ret = qtk_httpc_request_read(req);
		if(ret != 0) {
			wtk_log_warn0(req->session->log,"request read failed.");
			goto end;
		}
	}

	if(event->write) {
		ret = qtk_httpc_request_flush(req);
		if(ret <= 0) {
			wtk_log_warn(req->session->log,"request flush ret = %d",ret);
		}
		ret = 0;
	}

	if(event->error) {
		wtk_log_warn0(req->session->log,"request socket err.");
		_qtk_warning(req->session,_QTK_SOCKET_ERROR);
		ret = -1;
		goto end;
	}

	if(event->eof || event->reof) {
		wtk_log_warn0(req->session->log,"request peer close socket.");
		_qtk_warning(req->session,_QTK_SOCKET_HANGUP);
		close = 1;
		goto end;
	}

	req->http_event.writepending = (req->wstack->len>0);
	qtk_httpc_request_touch_epoll(req);
end:
	if(ret != 0) {
		close = 1;
	}
	if(close) {
		qtk_httpc_request_disconnect(req,1);
		qtk_httpc_request_input_clean(req);
		wtk_stack_reset(req->wstack);
		qtk_http_response_reset(req->response);
	}
	return 0;
}

void qtk_httpc_request_update_hostport(qtk_httpc_request_t *req,wtk_string_t *host,wtk_string_t *port)
{
	qtk_httpc_request_connect_reset(req);
	if(host) {
		wtk_string_set(&req->cfg->host,host->data,host->len);
	}
	if(port) {
		wtk_string_set(&req->cfg->port,port->data,port->len);
	}
	qtk_httpc_request_connect(req);
	//qtk_httpc_request_reconnect(req);
}
void qtk_httpc_request_set_err_notify(qtk_httpc_request_t *req,void *ths,qtk_httpc_request_err_notify_f notify)
{
	req->err_ths = ths;
	req->err_notify = notify;
}
