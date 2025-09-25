#include "wtk_tts.h"
void wtk_tts_on_syn(wtk_tts_t *ths,char *data,int bytes);
void wtk_tts_on_pitch(wtk_tts_t *ths,char *data,int bytes);
int wtk_tts_syn_to_pitch(wtk_tts_t *tts,wtk_thread_t *thread);


wtk_tts_msg_t* wtk_tts_msg_new(char *data,int len)
{
	wtk_tts_msg_t *msg;

	msg=(wtk_tts_msg_t*)wtk_malloc(sizeof(wtk_tts_msg_t));
	msg->data=wtk_string_dup_data(data,len);
	msg->type=WTK_TTS_MSG_AUDIO;
	return msg;
}

void wtk_tts_msg_delete(wtk_tts_msg_t *msg)
{
	if(msg->data)
	{
		wtk_string_delete(msg->data);
	}
	wtk_free(msg);
}
wtk_tts_t *wtk_tts_new(wtk_tts_cfg_t *cfg)
{
	return wtk_tts_new2(cfg, NULL, NULL, NULL);
}

wtk_tts_t *wtk_tts_new2(wtk_tts_cfg_t *cfg,wtk_pitch_callback_f start, wtk_pitch_callback_f end, void*ths)
{
	wtk_tts_t *tts;

	tts=(wtk_tts_t*)wtk_malloc(sizeof(wtk_tts_t));
	tts->cfg=cfg;
	tts->parser=wtk_tts_parser_new(&(cfg->parser),cfg->bin_cfg?cfg->bin_cfg->rbin:NULL);
	tts->syn=wtk_syn_new(&(cfg->syn));
	tts->syn->sigp->notify_ths=tts;
	tts->syn->sigp->notify=(wtk_tts_sigp_f)wtk_tts_on_syn;
	tts->notify_info=NULL;
	tts->notify_info_ths=NULL;
	tts->notify_start=NULL;
	tts->notify_start_ths=NULL;
	tts->notify_end=NULL;
	tts->notify_end_ths=NULL;
	tts->pitch=wtk_pitch_new(&(cfg->pitch));
	tts->notify=NULL;
	tts->ths=NULL;
	tts->pitch_shit=0;
	tts->speed=1.0;
	tts->pitch_buf=wtk_strbuf_new(cfg->buf_size,1);
	tts->min_sil_time = cfg->min_sil_time;
	tts->sil_buf=wtk_strbuf_new(1024,1);
	tts->sil_speech_buf=wtk_strbuf_new(1024*10,1);
	tts->snt_sil_time = cfg->snt_sil_time;
	wtk_syn_set_volume_scale(tts->syn,cfg->volume_scale);
	wtk_pitch_set(tts->pitch,tts,(wtk_pitch_noityf_f)wtk_tts_on_pitch);
	tts->pause_hint=0;
	wtk_sem_init(&(tts->pause_sem),0);
	tts->pitch_start=start;
	tts->pitch_end=end;
	tts->pitch_ths=ths;
	if(cfg->use_thread)
	{
		tts->run=1;
		wtk_sem_init(&(tts->wait_sem),0);
		wtk_blockqueue_init(&(tts->msg_q));
		wtk_thread_init(&(tts->thread),(thread_route_handler)wtk_tts_syn_to_pitch,tts);
		wtk_thread_set_name(&(tts->thread),"tts_pitch");
		wtk_thread_start(&(tts->thread));
	}
	//wtk_debug("volume=%f\n",cfg->volume_scale);
	return tts;
}

int wtk_tts_bytes(wtk_tts_t *tts)
{
	int bytes;

	bytes=wtk_tts_parser_bytes(tts->parser);
	//wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));
	bytes+=wtk_syn_bytes(tts->syn);
	//wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));
	//bytes+=wtk_pitch_bytes(tts->pitch);
	//wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));
	bytes+=wtk_strbuf_bytes(tts->pitch_buf);
	bytes+=wtk_strbuf_bytes(tts->sil_buf);
	bytes+=wtk_strbuf_bytes(tts->sil_speech_buf);
//	{
//		double m;
//
//		m=wtk_proc_mem();
//		wtk_debug("m=%f\n",m);
//	}
	//getchar();
	//exit(0);
	return bytes;
}
int wtk_tts_delete(wtk_tts_t *tts)
{
	if(tts->sil_buf)
	{
		wtk_strbuf_delete(tts->sil_buf);
	}
	if(tts->sil_speech_buf)
	{
		wtk_strbuf_delete(tts->sil_speech_buf);
	}
	wtk_sem_clean(&(tts->pause_sem));
	if(tts->cfg->use_thread)
	{
		tts->run=0;
		wtk_blockqueue_wake(&(tts->msg_q));
		wtk_thread_join(&(tts->thread));
		wtk_thread_clean(&(tts->thread));
		wtk_blockqueue_clean(&(tts->msg_q));
		wtk_sem_clean(&(tts->wait_sem));
	}
	wtk_strbuf_delete(tts->pitch_buf);
	wtk_pitch_delete(tts->pitch);
	wtk_tts_parser_delete(tts->parser);
	wtk_syn_delete(tts->syn);
	wtk_free(tts);
	return 0;
}

int wtk_tts_reset(wtk_tts_t *tts)
{
	tts->speed=1.0;
	tts->pitch_shit=0;
	tts->snt_sil_time = tts->cfg->snt_sil_time;
	tts->min_sil_time = tts->cfg->min_sil_time;
	wtk_strbuf_reset(tts->pitch_buf);
	wtk_pitch_reset(tts->pitch);
	wtk_tts_parser_reset(tts->parser);
	wtk_syn_reset(tts->syn);
	wtk_sem_init(&(tts->pause_sem),0);
	return 0;
}

int wtk_tts_pause(wtk_tts_t *tts)
{
	tts->pause_hint=1;
	return 0;
}
int wtk_tts_resume(wtk_tts_t *tts)
{
	tts->pause_hint=0;
	wtk_sem_release(&(tts->pause_sem),1);
	return 0;
}

void wtk_tts_set_volume_scale(wtk_tts_t *tts,float scale)
{
	wtk_syn_set_volume_scale(tts->syn,scale);
}

void wtk_tts_set_stop_hint(wtk_tts_t *tts)
{
	tts->stop_hint=1;
}

void wtk_tts_set_speed(wtk_tts_t *tts,float speed)
{
	tts->speed=speed;
}

void wtk_tts_set_pitch(wtk_tts_t *tts,float pitch)
{
	tts->pitch_shit=pitch;
}

void wtk_tts_set_notify(wtk_tts_t *tts,void *ths,wtk_tts_notify_f notify)
{
	tts->ths=ths;
	tts->notify=notify;
}

void wtk_tts_set_sntsil(wtk_tts_t *tts, int time)
{
	tts->snt_sil_time = time;
}

void wtk_tts_notify(wtk_tts_t *t,char *data,int len)
{
	if(t->notify)
	{
		t->notify(t->ths,data,len);
	}
}
void wtk_tts_set_start_notify(wtk_tts_t*t,void *ths, wtk_tts_notify_f notify)
{
	t->notify_start_ths=ths;
	t->notify_start=notify;
}
void wtk_tts_set_end_notify(wtk_tts_t*t,void *ths, wtk_tts_notify_f notify)
{
	t->notify_end_ths=ths;
	t->notify_end=notify;
}

void wtk_tts_set_info_notify(wtk_tts_t *tts,void *ths,wtk_tts_notify_f notify)
{
	tts->notify_info_ths=ths;
	tts->notify_info=notify;
}

int wtk_tts_get_current_syn_snt_index(wtk_tts_t *tts)
{
	return wtk_syn_get_cur_snt_index(tts->syn);
}

void wtk_tts_on_pitch(wtk_tts_t *tts,char *data,int bytes)
{
	wtk_strbuf_t *buf=tts->pitch_buf;

	wtk_strbuf_push(buf,data,bytes);
	if(buf->pos>tts->cfg->buf_size)
	{
		wtk_tts_notify(tts,buf->data,buf->pos);
		wtk_strbuf_reset(buf);
	}
}

void wtk_tts_on_syn(wtk_tts_t *tts,char *data,int bytes)
{
	wtk_tts_msg_t *msg;
	short *v;
	int i,len;
	int vx=tts->cfg->max_sil_value;
	wtk_strbuf_t *sil_speech_buf;
	wtk_strbuf_t *sil_buf;
	int min_len;
	wtk_string_t info;

	if(bytes<=0)
	{
		return;
	}
	if(tts->notify_info)
	{
		info=wtk_syn_get_cur_timeinfo(tts->syn);
		if(info.len>0)
		{
			tts->notify_info(tts->notify_info_ths,info.data,info.len);
		}
	}
	len=bytes/2;
	if(tts->min_sil_time>=0 && len>=tts->min_sil_time)
	{
		min_len=tts->min_sil_time*2;
		v=(short*)data;
		sil_buf=tts->sil_buf;
		sil_speech_buf=tts->sil_speech_buf;
		wtk_strbuf_reset(sil_buf);
		wtk_strbuf_reset(sil_speech_buf);
		for(i=0;i<len;++i)
		{
			if(v[i]>vx || v[i]<-vx)
			{
				if(sil_buf->pos>0)
				{
					if(sil_buf->pos>min_len)
					{
						sil_buf->pos=min_len;
					}
					wtk_strbuf_push(sil_speech_buf,sil_buf->data,sil_buf->pos);
					wtk_strbuf_reset(sil_buf);
				}
				wtk_strbuf_push(sil_speech_buf,(char*)(v+i),2);
			}else
			{
				if(sil_buf->pos<min_len)
				{
					wtk_strbuf_push(sil_buf,(char*)(v+i),2);
				}
			}
		}
		if(sil_buf->pos>min_len)
		{
			sil_buf->pos=min_len;
		}
		if(sil_buf->pos>0)
		{
			wtk_strbuf_push(sil_speech_buf,sil_buf->data,sil_buf->pos);
			wtk_strbuf_reset(sil_buf);
		}
		if((tts->syn->cfg->use_stream==0 || tts->syn->is_snt_end) &&tts->snt_sil_time>0)
		{
			len=tts->cfg->snt_sil_time*32;
			for(i=0;i<len;++i)
			{
				wtk_strbuf_push_c(sil_speech_buf,0);
			}
		}
		data=sil_speech_buf->data;
		bytes=sil_speech_buf->pos;
	}
	//wtk_debug("len=%d,bytes=%d min=%d\n",len,bytes,tts->cfg->min_sil_time);
	//exit(0);
	//wtk_debug("shift=%f\n",tts->pitch_shit);
	if(tts->pitch_shit==0)
	{
		wtk_tts_notify(tts,data,bytes);
	}else
	{
		if(tts->cfg->use_thread)
		{
			msg=wtk_tts_msg_new(data,bytes);
			wtk_blockqueue_push(&(tts->msg_q),&(msg->q_n));
		}else
		{
			wtk_pitch_process(tts->pitch,tts->pitch_shit,data,bytes);
		}
	}
}

int wtk_tts_syn_to_pitch(wtk_tts_t *tts,wtk_thread_t *thread)
{
	wtk_queue_node_t *qn;
	wtk_tts_msg_t *msg;

	if (tts->cfg->use_thread && tts->pitch_start){
		tts->pitch_start(tts->pitch_ths);
	}

	while(tts->run)
	{
		qn=wtk_blockqueue_pop(&(tts->msg_q),-1,NULL);
		if(!qn){continue;}
		msg=data_offset2(qn,wtk_tts_msg_t,q_n);
		switch(msg->type)
		{
		case WTK_TTS_MSG_AUDIO:
			if(tts->stop_hint==0)
			{
				wtk_pitch_process(tts->pitch,tts->pitch_shit,msg->data->data,msg->data->len);
			}
			wtk_tts_msg_delete(msg);
			break;
		case WTK_TTS_MSG_STOP:
			wtk_sem_release(&(tts->wait_sem),1);
			break;
		}
	}
	if (tts->cfg->use_thread && tts->pitch_end)
		tts->pitch_end(tts->pitch_ths);
	return 0;
}

int wtk_tts_process(wtk_tts_t *tts,char *txt,int txt_bytes)
{
	wtk_tts_snt_t **snt,*s;
	wtk_tts_lab_t* lab;
	wtk_tts_msg_t msg;
	int ret;
	int i;
	wtk_strbuf_t *buf=wtk_strbuf_new(256,1);

	//tts->pitch_shit=0;
	//wtk_debug("=================> pitch=%f,speed=%f\n",tts->pitch_shit,tts->speed);
	wtk_strbuf_push(buf,txt,txt_bytes);
	wtk_strbuf_strip(buf);
	txt=buf->data;
	txt_bytes=buf->pos;
	if (tts->notify_start){
		tts->notify_start(tts->notify_start_ths, txt, txt_bytes);
	}
	tts->stop_hint=0;
	lab=wtk_tts_parser_to_snt(tts->parser,txt,txt_bytes);
	if(!lab){goto end;}
	snt=(wtk_tts_snt_t**)lab->snts->slot;
	for(i=0;i<lab->snts->nslot && tts->stop_hint==0 ;++i)
	{
		//wtk_debug("snt[%d]=[%.*s]=%d\n",i,snt[i]->snt->len,snt[i]->snt->data,tts->stop_hint);
		if (tts->pause_hint){
			wtk_sem_acquire(&(tts->pause_sem),-1);
		}
		s=snt[i];
		ret=wtk_tts_parser_process_snt(tts->parser,s);
		if(ret!=0){continue;}
		if(tts->stop_hint)
		{
			break;
		}
		//tts->parser->lab->speech_speed=tts->speed;
		ret=wtk_syn_process_snt(tts->syn,s,tts->speed);
		if(ret!=0){continue;}
	}
	if(tts->cfg->use_thread && tts->pitch_shit!=0)
	{
		msg.type=WTK_TTS_MSG_STOP;
		wtk_blockqueue_push(&(tts->msg_q),&(msg.q_n));
		wtk_sem_acquire(&(tts->wait_sem),-1);
	}
	ret=0;
end:
	//wtk_debug("==================> tts end\n");
	wtk_tts_notify(tts,NULL,0);
	if(tts->notify_end){
		tts->notify_end(tts->notify_end_ths, txt, txt_bytes);
	}
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_tts_process2(wtk_tts_t *tts,char *txt,int txt_bytes)
{
	wtk_tts_msg_t msg;
	int ret;

	ret=wtk_tts_parser_process(tts->parser,txt,txt_bytes);
	if(ret!=0){goto end;}
	tts->parser->lab->speech_speed=tts->speed;
	ret=wtk_syn_process(tts->syn,tts->parser->lab);
	if(ret!=0){goto end;}
	if(tts->cfg->use_thread && tts->pitch_shit!=0)
	{
		msg.type=WTK_TTS_MSG_STOP;
		wtk_blockqueue_push(&(tts->msg_q),&(msg.q_n));
		wtk_sem_acquire(&(tts->wait_sem),-1);
	}
	ret=0;
end:
	wtk_tts_notify(tts,NULL,0);
	return ret;
}

void wtk_tts_defpron_setwrd(wtk_tts_t* tts, wtk_string_t* k, wtk_string_t* v)
{
	wtk_tts_parser_defpron_wrd(tts->parser, k, v);
}

wtk_string_t wtk_tts_get_cur_timeinfo(wtk_tts_t *s)
{
	return wtk_syn_get_cur_timeinfo(s->syn);
}
