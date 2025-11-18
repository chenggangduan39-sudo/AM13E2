#include "qtk_auin.h" 

int qtk_auin_run(qtk_auin_t *a,wtk_thread_t *t);

qtk_auin_msg_t* qtk_auin_msg_new(qtk_auin_msg_type_t type)
{
	qtk_auin_msg_t *msg;

	msg=(qtk_auin_msg_t*)wtk_malloc(sizeof(qtk_auin_msg_t));
	msg->type=type;
	return msg;
}

void qtk_auin_msg_delete(qtk_auin_msg_t *msg)
{
	wtk_free(msg);
}

qtk_auin_t* qtk_auin_new(qtk_auin_cfg_t *cfg,wtk_log_t *log)
{
	qtk_auin_t *a;

	a=(qtk_auin_t*)wtk_malloc(sizeof(qtk_auin_t));
	a->cfg=cfg;
	a->log=log;
	a->process=NULL;
	a->process_ths=NULL;
	a->auin_start=NULL;
	a->auin_read=NULL;
	a->auin_stop=NULL;
	a->auin_clean=NULL;
	a->run=0;
	a->state=QTK_AUIN_RUN;
	wtk_sem_init(&(a->signal_sem),0);
	wtk_blockqueue_init(&(a->input_q));
	wtk_thread_init(&(a->thread),(thread_route_handler)qtk_auin_run,a);
	wtk_thread_set_name(&(a->thread),"auin");

	return a;
}

void qtk_auin_set_callback(qtk_auin_t *a,void *ths,qtk_auin_start_f start,qtk_auin_read_f read,qtk_auin_stop_f stop,qtk_auin_clean_f clean)
{
	a->auin_start=start;
	a->auin_read=read;
	a->auin_stop=stop;
	a->auin_clean=clean;
	a->ths=ths;
}

void qtk_auin_delete(qtk_auin_t *a)
{
	if(a->run) {
		qtk_auin_stop(a);
	}
	wtk_thread_clean(&(a->thread));
	wtk_blockqueue_clean(&(a->input_q));
	wtk_sem_clean(&(a->signal_sem));
	wtk_free(a);
}

int qtk_auin_start(qtk_auin_t *a)
{
	if(a->run) {
		return -1;
	}

	a->run=1;
	return wtk_thread_start(&(a->thread));
}

void qtk_auin_stop(qtk_auin_t *a)
{
	wtk_queue_node_t *qn;
	qtk_auin_msg_t *msg;

	if(a->run) {
		a->run = 0;
		wtk_blockqueue_wake(&(a->input_q));
		wtk_thread_join(&(a->thread));

		while (1) {
			qn = wtk_blockqueue_pop(&(a->input_q), 0, NULL);
			if (!qn) {
				break;
			}
			msg = data_offset2(qn, qtk_auin_msg_t, q_n);
			qtk_auin_msg_delete(msg);
		}
	}
}

void qtk_auin_set_process(qtk_auin_t *a,void *ths,qtk_auin_process_f process)
{
	a->process=process;
	a->process_ths=ths;
}

int qtk_auin_run(qtk_auin_t *a,wtk_thread_t *t)
{
	wtk_strbuf_t *buf;
	int ret;
	wtk_queue_node_t *qn;
	qtk_auin_msg_t *msg;
	wtk_blockqueue_t *input_q=&(a->input_q);

	if(a->cfg->debug)
	{
		wtk_debug("auin start.\n");
	}
	wtk_log_log0(a->log,"auin start.");
	ret = a->auin_start(a->ths);
	if(ret !=0 ) {
		if(a->cfg->debug)
		{
			wtk_debug("[ERROR]:auin Recorder start failed.\n");
		}
		wtk_log_log0(a->log,"[ERROR]:auin Recorder start failed.");
		goto end;
	}
	while(a->run)
	{
		if(a->state==QTK_AUIN_RUN)
		{
			buf=a->auin_read(a->ths);
			if(input_q->length>0)
			{
				while(input_q->length>0)
				{
					qn=wtk_blockqueue_pop(input_q,0,NULL);
					if(!qn){break;}
					msg=data_offset2(qn,qtk_auin_msg_t,q_n);
					switch(msg->type)
					{
					case QTK_AUIN_MSG_PAUSE:
						a->state=QTK_AUIN_PAUSE;
						a->auin_stop(a->ths);
						break;
					case QTK_AUIN_MSG_RESUME:
						break;
					}
					wtk_sem_release(&(a->signal_sem),1);
					qtk_auin_msg_delete(msg);
				}
				if(a->state!=QTK_AUIN_RUN)
				{
					continue;
				}
			}
			if(buf->pos < 0)
			{
				if(a->cfg->debug)
				{
					wtk_debug("[ERROR]:auin recorder failed %d.\n",buf->pos);
				}
				if(a->cfg->err_exit) {
					if(a->auin_stop)
					{
						a->auin_stop(a->ths);
					}
					if(a->auin_clean)
					{
						a->auin_clean(a->ths);
					}
					goto end;
				}
			}
			if(buf->pos>0 && a->process)
			{
				a->process(a->process_ths,buf->data,buf->pos);
			}
		}else if(a->state==QTK_AUIN_PAUSE)
		{
			qn=wtk_blockqueue_pop(input_q,-1,NULL);
			if(qn)
			{
				msg=data_offset2(qn,qtk_auin_msg_t,q_n);
				switch(msg->type)
				{
				case QTK_AUIN_MSG_PAUSE:
					break;
				case QTK_AUIN_MSG_RESUME:
					a->state=QTK_AUIN_RUN;
					ret=a->auin_start(a->ths);
					break;
				}
				wtk_sem_release(&(a->signal_sem),1);
				qtk_auin_msg_delete(msg);
			}
		}
	}

	if(a->auin_stop)
	{
		a->auin_stop(a->ths);
	}
	if(a->auin_clean)
	{
		a->auin_clean(a->ths);
	}
end:
	if(a->cfg->debug)
	{
		wtk_debug("auin stop.\n");
	}
	wtk_log_log0(a->log,"auin stop.");
	return ret;
}

void qtk_auin_restart_record(qtk_auin_t *a)
{
	qtk_auin_stop(a);
	qtk_auin_start(a);
}

void qtk_auin_pause(qtk_auin_t *a)
{
	qtk_auin_msg_t *msg;

	//wtk_auin_stop(a);
	msg=qtk_auin_msg_new(QTK_AUIN_MSG_PAUSE);
	wtk_blockqueue_push(&(a->input_q),&(msg->q_n));
	wtk_sem_acquire(&(a->signal_sem),-1);
}

void qtk_auin_resume(qtk_auin_t *a)
{
	qtk_auin_msg_t *msg;

	//wtk_auin_start(a);
	msg=qtk_auin_msg_new(QTK_AUIN_MSG_RESUME);
	wtk_blockqueue_push(&(a->input_q),&(msg->q_n));
	wtk_sem_acquire(&(a->signal_sem),-1);
}
