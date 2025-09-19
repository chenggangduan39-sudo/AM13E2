#include "qtk_select.h"

static void qtk_select_init(qtk_select_t *s)
{
	s->log = NULL;

	s->read_set = NULL;
	s->write_set = NULL;
	s->error_set = NULL;

	s->nevents = 0;
	s->max_fd  = -1;
	s->monitor_event = 0;
	s->bw_on_sig = 0;
}

qtk_select_t* qtk_select_new(wtk_log_t *log,int size)
{
	qtk_select_t *s;

	s = (qtk_select_t*)wtk_malloc(sizeof(qtk_select_t));
	qtk_select_init(s);

	s->log = log;
	s->nevents =  min(FD_SETSIZE,size);
	s->read_set  = (fd_set*)wtk_calloc(1,sizeof(fd_set));
	s->write_set = (fd_set*)wtk_calloc(1,sizeof(fd_set));
	s->error_set = (fd_set*)wtk_calloc(1,sizeof(fd_set));
	wtk_queue_init(&s->event_q);

	return s;
}

void qtk_select_del(qtk_select_t *s)
{
	wtk_free(s->read_set);
	wtk_free(s->write_set);
	wtk_free(s->error_set);
	wtk_free(s);
}

int qtk_select_run(qtk_select_t *s,int looptime,int *recvd)
{
	wtk_queue_node_t *qn;
	fd_set r_set,w_set,e_set;
	qtk_event_t *event;
	struct timeval tv;
	int ret;

	if(s->max_fd == -1) {
		for(qn=s->event_q.pop;qn;qn=qn->next) {
			event = data_offset2(qn,qtk_event_t,qn);
			if(event->fd > s->max_fd) {
				s->max_fd = event->fd;
			}
		}
	}
	if(s->max_fd == -1) {
		return -1;
	}

	while(1) {
		r_set = *s->read_set;
		w_set = *s->write_set;
		e_set = *s->error_set;

		tv.tv_sec = looptime / 1000;
		tv.tv_usec = (looptime % 1000) *1000;

		ret = select(s->max_fd+1,&r_set,&w_set,&e_set,&tv);

		if(ret == -1 && errno == EINTR) {
			if(!s->bw_on_sig) {
				continue;
			}
		} else if(ret > 0) {
			for(qn=s->event_q.pop;qn;qn=qn->next) {
				event = data_offset2(qn,qtk_event_t,qn);
				event->read =  FD_ISSET(event->fd,&r_set);
				event->write = FD_ISSET(event->fd,&w_set);
				event->error = FD_ISSET(event->fd,&e_set);

				if(event->read || event->write || event->error) {
					event->handler(event->data,event);
					ret -= event->read + event->write + event->error;
					if(ret == 0) {
						break;
					}
				}

				if(event->read && event->nk) {
					++(*recvd);
				}
			}
		}
		break;
	}
	return ret;
}

int qtk_select_add_event(qtk_select_t *s,int fd,qtk_event_t *event)
{
	wtk_log_log(s->log,"add event fd %d want_read %d want_write %d writependding %d",
			event->fd,
			event->want_read,
			event->want_write,
			event->writepending
			);
	if(s->monitor_event >= s->nevents) {
		wtk_log_warn(s->log,"monitor overflow %d nevents = %d",s->monitor_event,s->nevents);
		return -1;
	}

	FD_SET(event->fd,s->error_set);
	event->errepolled = 1;

	if(event->want_read) {
		FD_SET(event->fd,s->read_set);
		event->readepolled = 1;
	}

	if(event->want_write || event->writepending) {
		FD_SET(event->fd,s->write_set);
		event->writeepolled = 1;
	}

	event->in_queue = 1;
	if(event->fd > s->max_fd) {
		s->max_fd = event->fd;
	}
	++s->monitor_event;

	wtk_queue_push(&s->event_q,&event->qn);
	return 0;
}

int qtk_select_mod_event(qtk_select_t *s,int fd,qtk_event_t *event)
{
	if(!event->errepolled) {
		FD_SET(event->fd,s->error_set);
	}

	if(event->want_read) {
        if (!event->readepolled) {
            FD_SET(event->fd, s->read_set);
		}
		event->readepolled = 1;
	} else {
        if (event->readepolled) {
            FD_CLR(event->fd, s->read_set);
        }          
		event->readepolled = 0;
	}

	if(event->want_write || event->writepending) {
        if (!event->writeepolled) {
            FD_SET(event->fd, s->write_set);
        }
		event->writeepolled = 1;
	} else {
        if (event->writeepolled) {
            FD_CLR(event->fd, s->write_set);
        }
		event->writeepolled = 0;
	}

	return 0;
}

int qtk_select_del_event(qtk_select_t *s,int fd,qtk_event_t *event)
{
	wtk_log_log(s->log,"del event fd = %d",event->fd);

	if(event->errepolled) {
		FD_CLR(event->fd,s->error_set);
		event->errepolled = 0;
	}
	if(event->readepolled) {
		FD_CLR(event->fd,s->read_set);
	}
	if(event->writeepolled) {
		FD_CLR(event->fd,s->write_set);
	}
	event->in_queue = 0;
	if(event->fd == s->max_fd) {
		s->max_fd = -1;
	}
	--s->monitor_event;
	wtk_queue_remove(&s->event_q,&event->qn);

	return 0;
}
