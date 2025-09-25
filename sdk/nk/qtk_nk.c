#include "qtk_nk.h" 

#if defined(QTK_NK_USE_EPOLL)
#include "module/qtk_epoll.h"
static qtk_nk_module_action_t event_actions = qtk_nk_module_mk_action(qtk_epoll);

#elif defined(QTK_NK_USE_KQUEUE)

#else
#include "module/qtk_select.h"
static qtk_nk_module_action_t event_actions = qtk_nk_module_mk_action(qtk_select);
#endif

static int qtk_nk_process(qtk_nk_t *nk,wtk_thread_t *t)
{
	int ret,recvd;

	while(nk->run && nk->handler) {
		recvd = 0;
		ret = event_actions.run_f(nk->handler,nk->looptime,&recvd);
		if(ret != 0) {
			wtk_log_warn(nk->log,"nk process ret = %d",ret);
		}
		if(nk->recvd_handler) {
			nk->recvd_handler(nk->user_data,recvd);
		}
	}
	return 0;
}

void qtk_nk_init(qtk_nk_t *nk)
{
	nk->log      = NULL;
	nk->handler  = NULL;
	nk->looptime = 1000;
	nk->run      = 0;
	nk->pipe_fd[0] = INVALID_FD;
	nk->pipe_fd[1] = INVALID_FD;

	nk->recvd_handler = NULL;
	nk->user_data = NULL;
}

int qtk_nk_on_event(void *ths,qtk_event_t *event)
{
	return 0;
}

qtk_nk_t *qtk_nk_new(wtk_log_t *log,int looptime,int size)
{
	qtk_nk_t *nk;
	int ret;

	nk = (qtk_nk_t*)wtk_malloc(sizeof(qtk_nk_t));

	qtk_nk_init(nk);
	nk->log      = log;
	nk->looptime = looptime;

	nk->handler = event_actions.new_f(log,size);
	if(!nk->handler) {
		wtk_log_warn0(nk->log,"new module failed");
		ret = -1;
		goto end;
	}

#ifdef WIN32
	ret = wtk_socketpair(nk->pipe_fd,0);
#else
	ret = pipe(nk->pipe_fd);
#endif
	if(ret != 0) {
		goto end;
	}
	ret = wtk_fd_set_nonblock(nk->pipe_fd[0]);
	qtk_event_reset_sig(&nk->event);
	nk->event.want_read = 1;
	nk->event.fd = nk->pipe_fd[0];
	nk->event.data = NULL;
	nk->event.handler = (qtk_event_handler)qtk_nk_on_event;
	event_actions.add_event_f(nk->handler,nk->pipe_fd[0],&nk->event);

	nk->run = 1;
	wtk_thread_init(&nk->thread,(thread_route_handler)qtk_nk_process,nk);
	wtk_thread_set_name(&nk->thread,"nk");
	ret = wtk_thread_start(&nk->thread);
	if(ret != 0) {
		wtk_log_warn0(nk->log,"nk start failed");
		goto end;
	}

	ret = 0;
end:
	wtk_log_log(nk->log,"nk new ret %d\n",ret);
	if(ret != 0) {
		qtk_nk_delete(nk);
		nk = NULL;
	}
	return nk;
}

void qtk_nk_delete(qtk_nk_t *nk)
{
        int ret;
        if (nk->run) {
#ifdef WIN32
		send(nk->pipe_fd[1],"m",1,0);
#else
                ret = write(nk->pipe_fd[1], "m", 1);
                if (ret != 1) {
                        wtk_debug("warning\n");
                }
#endif
		nk->run = 0;
		wtk_thread_join(&nk->thread);
		wtk_thread_clean(&nk->thread);
        }

#ifdef WIN32
	closesocket(nk->pipe_fd[0]);
	closesocket(nk->pipe_fd[1]);
#else
	close(nk->pipe_fd[0]);
	close(nk->pipe_fd[1]);
#endif

	if(nk->handler) {
		event_actions.del_f(nk->handler);
	}

	wtk_free(nk);
}

void qtk_nk_set_recvd_handler(qtk_nk_t *nk,void *user_data,qtk_nk_recvd_handler recvd_handler)
{
	nk->recvd_handler = recvd_handler;
	nk->user_data = user_data;
}

int qtk_nk_add_event(qtk_nk_t *nk,int fd,qtk_event_t *event)
{
	if(!nk->handler) {
		return -1;
	}

	return event_actions.add_event_f(nk->handler,fd,event);
}

int qtk_nk_mod_event(qtk_nk_t *nk,int fd,qtk_event_t *event)
{
	if(!nk->handler) {
		return -1;
	}

	return event_actions.mod_event_f(nk->handler,fd,event);
}

int qtk_nk_del_event(qtk_nk_t *nk,int fd,qtk_event_t *event)
{
	if(!nk->handler) {
		return -1;
	}

	return event_actions.del_event_f(nk->handler,fd,event);
}
