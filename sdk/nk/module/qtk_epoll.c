#include "qtk_epoll.h"

static void qtk_epoll_init(qtk_epoll_t *e)
{
	e->log  = NULL;
	e->fd   = INVALID_FD;
	e->size = 0;
	e->moniter_events = 0;
	e->events = NULL;
#ifdef QTK_EPOLL_USE_EPOLLET
	e->et = 1;
#else
	e->et = 0;
#endif
	e->bw_on_sig=0;
}

qtk_epoll_t* qtk_epoll_new(wtk_log_t *log,int size)
{
	qtk_epoll_t *e;
	int ret;

	e = (qtk_epoll_t*)wtk_malloc(sizeof(qtk_epoll_t));
	qtk_epoll_init(e);

	e->log = log;
	e->size = size;
	e->events = (struct epoll_event*)wtk_calloc(e->size,sizeof(struct epoll_event));
	if(!e->events) {
		wtk_log_warn0(e->log,"calloc events failed");
		ret = -1;
		goto end;
	}

	e->fd = epoll_create(e->size);
	wtk_log_log(e->log,"create fd %d",e->fd);
	if(e->fd == INVALID_FD) {
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	if(ret != 0) {
		qtk_epoll_del(e);
		e = NULL;
	}
	return e;
}

void qtk_epoll_del(qtk_epoll_t *e)
{
	if(e->fd != INVALID_FD) {
		close(e->fd);
		e->fd = INVALID_FD;
	}

	if(e->events) {
		wtk_free(e->events);
	}

	wtk_free(e);
}

static uint32_t qtk_epoll_get_event_flags(qtk_epoll_t* e,qtk_event_t *event)
{
	uint32_t events;

	events=EPOLLRDHUP|EPOLLHUP|EPOLLERR;
	if(event->want_read) {
		events |= EPOLLIN;
	}
	if(event->want_write || event->writepending) {
		events |= EPOLLOUT;
	}
	if(e->et) {
		events |= EPOLLET;
	}
	return events;
}

int qtk_epoll_add_event(qtk_epoll_t *e,int fd,qtk_event_t *event)
{
	struct epoll_event ev;
	int ret;


	wtk_log_log(e->log,"add event fd %d",fd);
	if(e->moniter_events >= e->size) {
		wtk_log_warn(e->log,"monitor event %d / %d",e->moniter_events,e->size);
		ret = -1;
		goto end;
	}

	ev.events = qtk_epoll_get_event_flags(e,event);
	ev.data.ptr = (void*)event;
	ret = epoll_ctl(e->fd,EPOLL_CTL_ADD,fd,&ev);
	if(ret != 0) {
		wtk_log_warn(e->log,"add event failed fd %d",fd);
		qtk_event_print(event,e->log);
		goto end;
	}
	++e->moniter_events;

	event->in_queue = 1;
	event->readepolled  = (ev.events & EPOLLIN)?1:0;
	event->errepolled   = (ev.events & EPOLLERR)?1:0;
	event->writeepolled = (ev.events & EPOLLOUT)?1:0;

end:
	return ret;
}

int qtk_epoll_mod_event(qtk_epoll_t *e,int fd,qtk_event_t *event)
{
	struct epoll_event ev;
	int ret;

	wtk_log_log(e->log,"mod event fd %d",fd);

	ev.events = qtk_epoll_get_event_flags(e,event);
	ev.data.ptr = (void*)event;
	ret = epoll_ctl(e->fd,EPOLL_CTL_MOD,fd,&ev);
	if(ret != 0) {
		wtk_log_warn(e->log,"mod event failed fd %d",fd);
		qtk_event_print(event,e->log);
		goto end;
	}

	event->in_queue = 1;
	event->readepolled  = (ev.events & EPOLLIN)?1:0;
	event->errepolled   = (ev.events & EPOLLERR)?1:0;
	event->writeepolled = (ev.events & EPOLLOUT)?1:0;

end:
	return ret;
}

int qtk_epoll_del_event(qtk_epoll_t *e,int fd,qtk_event_t *event)
{
	int ret;

	wtk_log_log(e->log,"del event fd %d",fd);
	ret = epoll_ctl(e->fd,EPOLL_CTL_DEL,fd,NULL);
	if(ret != 0) {
		wtk_log_warn(e->log,"del event failed fd %d",fd);
		qtk_event_print(event,e->log);
		goto end;
	}
	--e->moniter_events;

	event->in_queue = 0;
end:
	return ret;
}

int qtk_epoll_run(qtk_epoll_t *e,int looptime,int *recvd)
{
	struct epoll_event *epoll_event;
	qtk_event_t *event;
	uint32_t events;
	int ret,i;

	while(1) {
		ret = epoll_wait(e->fd,e->events,e->moniter_events+1,looptime);
		if(ret == -1 && errno == EINTR) {
			if(e->bw_on_sig) {
				ret = 0;
				break;
			}
		} else {
			break;
		}
	}

	if(ret <= 0) {
		goto end;
	}

	for(i=0; i<ret; ++i) {
		epoll_event = &(e->events[i]);
		events = epoll_event->events;
		event = (qtk_event_t*) (epoll_event->data.ptr);
		event->read  = events & EPOLLIN?1:0;
		event->write = events & EPOLLOUT?1:0;
		event->error = events & EPOLLERR?1:0;
		if(!event->eof) {
			event->eof = events & EPOLLHUP?1:0;
		}
		if(!event->reof) {
			event->reof = events & EPOLLRDHUP?1:0;
		}
		if(event->reof) {
			event->read = 1;
		}

		if(event->read && event->nk) {
			++(*recvd);
		}

		event->handler(event->data,event);
	}

	ret = 0;
end:
	return ret;
}
