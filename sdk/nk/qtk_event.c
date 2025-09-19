#include "qtk_event.h"


void qtk_event_print(qtk_event_t *e,wtk_log_t *log)
{
	if(log) {
		wtk_log_log0(log,"============ evt ==============\n");
		wtk_log_log(log,"in_queue:\t%d",e->in_queue);
		wtk_log_log(log,"want read:\t%d",e->want_read);
		wtk_log_log(log,"want write:\t%d",e->want_write);
		wtk_log_log(log,"read:\t\t%d",e->read);
		wtk_log_log(log,"write:\t\t%d",e->write);
		wtk_log_log(log,"reof:\t\t%d",e->reof);
		wtk_log_log(log,"eof:\t\t%d",e->eof);
		wtk_log_log(log,"error:\t\t%d",e->error);
		wtk_log_log(log,"readpolled:\t%d",e->readepolled);
	    wtk_log_log(log,"writeepolled:\t%d",e->writeepolled);
	    wtk_log_log(log,"errpolled:\t%d",e->errepolled);
	    wtk_log_log(log,"writepending:\t%d",e->writepending);
	    wtk_log_log(log,"fd:\t%d",e->fd);
	    wtk_log_log(log,"nk:\t%d",e->nk);
	} else {
		printf("============ evt ==============\n");
		printf("in queue:\t%d\n",e->in_queue);
		printf("want read:\t%d\n",e->want_read);
		printf("want write:\t%d\n",e->want_write);
		printf("read:\t\t%d\n",e->read);
		printf("write:\t\t%d\n",e->write);
		printf("reof:\t\t%d\n",e->reof);
		printf("eof:\t\t%d\n",e->eof);
		printf("error:\t\t%d\n",e->error);
		printf("readpolled:\t%d\n",e->readepolled);
	    printf("writeepolled:\t%d\n",e->writeepolled);
	    printf("errpolled:\t%d\n",e->errepolled);
	    printf("writepending:\t%d\n",e->writepending);
	    printf("fd:\t%d\n",e->fd);
	    printf("nk:\t%d\n",e->nk);
	}
}
