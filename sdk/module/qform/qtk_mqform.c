#include "qtk_mqform.h"

#define LED_ON 		0x80
#define LED_OFF		0x81

void qtk_mqform_on_wakeup(qtk_mqform_t *w,qtk_var_t *var);
void qtk_mqform_on_data(qtk_mqform_t *qform,char *data,int len);
#ifndef OFFLINE_TEST
int qtk_mqform_recorder_run(qtk_mqform_t *qform, wtk_thread_t *thread);
int qtk_mqform_player_run(qtk_mqform_t *qform,wtk_thread_t *thread);
#endif

int qtk_mqform_engine_run(qtk_mqform_t *qform, wtk_thread_t *thread);
qtk_mqform_msg_t* qtk_mqform_msg_new(qtk_mqform_t *qform);
int qtk_mqform_msg_delete(qtk_mqform_msg_t *node);
void qtk_mqform_data_change_vol(char *data, int bytes, float shift);
int qtk_mqform_qn_clean(qtk_mqform_t *qform,wtk_blockqueue_t *queue, int poplen);
int qtk_mqform_qn_clean2(qtk_mqform_t *qform);
static void qtk_log_wav_file_new(qtk_mqform_t *m);
static void qtk_log_wav_file_delete(qtk_mqform_t *m);
void qtk_module_mqform_is_log_audio(qtk_mqform_t *m);
int qtk_mqform_record_pylay_start(qtk_mqform_t *qform);

void qtk_mqform_init(qtk_mqform_t *m)
{
	m->cfg = NULL;
	m->session = NULL;
	m->qform = NULL;
	m->mic = NULL;
	m->echo = NULL;
	m->notify_ths = NULL;
	m->notify_func = NULL;
	m->vad_no = 0;
	m->log_audio = 0;
	m->engine_run = 0;
	m->player_run = 0;
	m->recorder_run = 0;
#ifndef OFFLINE_TEST
	m->recorder = NULL;
	m->player = NULL;
#endif
	m->sample = NULL;
	m->mul_q = NULL;
	m->buf_q = NULL;
	m->qformbuf = NULL;
}

qtk_mqform_t* qtk_mqform_new(qtk_session_t *session,qtk_mqform_cfg_t *cfg)
{
	qtk_mqform_t *w;
	char path[256];
	int ret;

	w = (qtk_mqform_t *)wtk_malloc(sizeof(qtk_mqform_t));
	qtk_mqform_init(w);

	w->cfg = cfg;
	w->session = session;

	w->check_path_buf = wtk_strbuf_new(64,0);
	w->mul_path = wtk_strbuf_new(64,0);
	w->single_path = wtk_strbuf_new(64,0);

	if(w->session->opt.cache_path->len > 0){
		double tt;
		tt=time_get_ms();

		wtk_mkdir(w->session->opt.cache_path->data);
		wtk_strbuf_push_f(w->check_path_buf, "%.*s/start_log_audio", w->session->opt.cache_path->len, w->session->opt.cache_path->data);
		wtk_strbuf_push_c(w->check_path_buf, 0);
		wtk_strbuf_push_f(w->mul_path, "%.*s/mul.wav", w->session->opt.cache_path->len, w->session->opt.cache_path->data);
		wtk_strbuf_push_c(w->mul_path, 0);
		wtk_strbuf_push_f(w->single_path, "%.*s/single.wav", w->session->opt.cache_path->len, w->session->opt.cache_path->data);
		wtk_strbuf_push_c(w->single_path, 0);
	}

	if(w->session->opt.log_wav || w->cfg->use_log_wav)
	{
		qtk_log_wav_file_new(w);
	}

#ifndef OFFLINE_TEST
	if(cfg->use_recorder)
	{
		w->recorder = qtk_recorder_new(&cfg->recorder_cfg, w->session, w, NULL);
		if(NULL == w->recorder)
		{
			wtk_log_log0(w->session->log, "recorder error");
			goto end;
		}
	}

	if(cfg->use_player)
	{
		w->player = qtk_player_new(&cfg->player_cfg, w->session, w, NULL);
		if(NULL == w->player){
			wtk_log_log0(w->session->log, "player error\n");
			goto end;
		}
	}
#endif

	if(cfg->use_sample)
	{
    	w->sample=wtk_resample_new(960);
	}

    w->qformbuf=wtk_strbuf_new(1024,1.0);
	w->outbuf = wtk_strbuf_new(1024, 1.0);

    w->mul_q = (wtk_blockqueue_t*)wtk_malloc(2*sizeof(wtk_blockqueue_t));
    if(NULL == w->mul_q)
    {
		ret=-1;
        goto end;
    }
    w->single_q = w->mul_q + 1;

	w->buf_q = wtk_malloc(sizeof(wtk_lockhoard_t));
    if(NULL == w->buf_q)
    {
		ret = -1;
        goto end; 
    }

	wtk_log_log(w->session->log,"bfio params = [%s].",cfg->sqform);
	w->qform = qtk_engine_new(session,cfg->sqform);
	if(!w->qform) {
		if(cfg->debug)
		{
			wtk_debug("bfio engine new failed.\n");
		}
		wtk_log_warn0(w->session->log,"bfio engine new failed.");
		ret = -1;
		goto end;
	}
	qtk_engine_set_notify(w->qform,w,(qtk_engine_notify_f)qtk_mqform_on_wakeup);

	wtk_lock_init(&w->mlock);

	//wtk_debug("==================>>>log_wav=%d\n",w->session->opt.log_wav);
	ret = 0;
end:
	wtk_log_log(w->session->log,"ret = %d.",ret);
	if(ret != 0) {
		qtk_mqform_delete(w);
		w = NULL;
	}
	return w;

}

void qtk_mqform_delete(qtk_mqform_t *w)
{
    if(w->qformbuf)
    {
        wtk_strbuf_delete(w->qformbuf);
    }
	if(w->outbuf)
	{
		wtk_strbuf_delete(w->outbuf);
	}
#ifndef OFFLINE_TEST
    if(w->recorder)
    {
        qtk_recorder_delete(w->recorder);
        w->recorder = NULL;
    }
    if(w->player)
    {
        qtk_player_delete(w->player);
        w->player = NULL;
    }
#endif

	if(w->sample)
	{
    	wtk_resample_delete(w->sample);
		w->sample = NULL;
	}
    if(w->mul_q)
    {
        wtk_free(w->mul_q);
        w->mul_q = NULL;
    }

    if(w->buf_q)
    {
        wtk_free(w->buf_q);
        w->buf_q = NULL;
    }

	if(w->qform){
		qtk_engine_delete(w->qform);
	}

	if(w->cfg->use_log_wav || w->session->opt.log_wav)
	{
		qtk_log_wav_file_delete(w);
	}

	if(w->check_path_buf){
		wtk_strbuf_delete(w->check_path_buf);
	}
	if(w->mul_path){
		wtk_strbuf_delete(w->mul_path);
	}
	if(w->single_path){
		wtk_strbuf_delete(w->single_path);
	}

	wtk_lock_clean(&w->mlock);
	wtk_free(w);
}

int qtk_mqform_thread_init(qtk_mqform_t *qform)
{
	int ret = 0;
    
	// wtk_debug("=========>>use_record=%d use_player=%d use_sample=%d\n",qform->cfg->use_recorder,qform->cfg->use_player,qform->cfg->use_sample);

	if(qform->cfg->use_recorder)
	{
		wtk_thread_init(&qform->engine_thread,(thread_route_handler)qtk_mqform_engine_run,(void*)qform);
		wtk_thread_set_name(&qform->engine_thread,"engine");
#ifndef OFFLINE_TEST
		wtk_thread_init(&qform->recorder_thread,(thread_route_handler)qtk_mqform_recorder_run, (void*)qform); 
		wtk_thread_set_name(&qform->recorder_thread,"recorder");
#endif
	}
    
#ifndef OFFLINE_TEST
	if(qform->cfg->use_player)
	{
		wtk_thread_init(&qform->player_thread,(thread_route_handler)qtk_mqform_player_run,(void*)qform);
		wtk_thread_set_name(&qform->player_thread,"player");	
	}
#endif

	if(qform->cfg->use_sample)
	{
    	wtk_resample_set_notify(qform->sample,qform,(wtk_resample_notify_f)qtk_mqform_on_data);
	}

    wtk_blockqueue_init(qform->mul_q); 
    wtk_blockqueue_init(qform->single_q);

    wtk_lockhoard_init(qform->buf_q,offsetof(qtk_mqform_msg_t,node),8,(wtk_new_handler_t)qtk_mqform_msg_new,(wtk_delete_handler_t)qtk_mqform_msg_delete,qform); 

	return ret;
}

int qtk_mqform_start(qtk_mqform_t *qform)
{
	int ret;
	qtk_mqform_thread_init(qform);


	ret = qtk_engine_start(qform->qform);

	if(qform->cfg->use_recorder)
	{
		if(0 == qform->engine_run)
		{
			qform->engine_run = 1;
			if(ret != 0) {
				wtk_log_log0(qform->session->log,"bfio engine start failed.");
				goto end;
			}
			wtk_thread_start(&qform->engine_thread);
		}
	}

    ret=qtk_mqform_record_pylay_start(qform);


	ret = 0;
end:
	return ret;
}

int qtk_mqform_record_pylay_start(qtk_mqform_t *qform)
{
    int ret;

#ifndef OFFLINE_TEST
	if(qform->cfg->use_recorder)
	{
		if(0 == qform->recorder_run)
		{
			qform->recorder_run = 1;
			ret=qtk_recorder_start(qform->recorder);
			if(ret!=0)
			{
				wtk_debug("recorder start error!\n");
				goto end;
			}
			wtk_thread_start(&qform->recorder_thread);

		}
	}
#endif

	if(qform->cfg->use_sample)
	{
    	wtk_resample_start(qform->sample,16000,qform->cfg->resample_rate);
	}

	qform->player_rate=qform->cfg->resample_rate;

#ifndef OFFLINE_TEST
	if(qform->cfg->use_player)
	{
		if(0 == qform->player_run){
			qform->player_run = 1;
			wtk_thread_start(&qform->player_thread);
		}
	}
#endif

    ret=0;
end:
    return ret;
}

int qtk_mqform_stop(qtk_mqform_t *qform)
{
	if(qform->cfg->use_recorder)
	{
#ifndef OFFLINE_TEST
		if(1 == qform->recorder_run)
		{
			wtk_log_log0(qform->session->log,"stop recorder\n");
			qform->recorder_run = 0;
			wtk_thread_join(&qform->recorder_thread);
			if(qform->recorder)
			{
				qtk_recorder_stop(qform->recorder);
			}
			wtk_log_log0(qform->session->log,"stop recorder end\n");
		}
#endif	
		if(1 == qform->engine_run)
		{
			wtk_log_log0(qform->session->log,"trsn_start stop\n");
			qform->engine_run = 0;
			wtk_blockqueue_wake(qform->mul_q);
			wtk_thread_join(&qform->engine_thread);
			wtk_log_log0(qform->session->log,"trsn_start stop end\n");
		}
	}

	if(qform->qform)
	{
		qtk_engine_feed(qform->qform,0,0,1);
		qtk_engine_reset(qform->qform);
	}

#ifndef OFFLINE_TEST
	if(qform->cfg->use_player)
	{
		if(1 == qform->player_run){
			wtk_log_log0(qform->session->log,"player stop\n");
			qform->player_run = 0;
			wtk_blockqueue_wake(qform->single_q);
			wtk_thread_join(&qform->player_thread);
			if(qform->player)
			{
				qtk_player_stop(qform->player);
			}
			wtk_log_log0(qform->session->log,"player stop end\n");
		}
	}
#endif
	return 0;
}

void qtk_mqform_set_notify(qtk_mqform_t *w,void *notify_ths,qtk_engine_notify_f notify_func)
{
	w->notify_func = notify_func;
	w->notify_ths = notify_ths;
}

#ifdef OFFLINE_TEST
int qtk_mqform_feed(qtk_mqform_t *w, char *data, int bytes)
{
	qtk_mqform_msg_t *node = NULL;
	node = qtk_mqform_msg_new(w);
	wtk_strbuf_push(node->data,data,bytes);
	wtk_blockqueue_push(w->mul_q,&node->node);
}
#endif

void qtk_mqform_on_wakeup(qtk_mqform_t *w,qtk_var_t *var)
{
	qtk_mqform_msg_t *node = NULL;
	char *ss;
    int nx=0,i=0;
	qtk_var_t ov;

	switch(var->type){
		case QTK_SPEECH_DATA_PCM:
		    if(w->cfg->echo_shift != 1.0){
				qtk_mqform_data_change_vol(var->v.str.data, var->v.str.len, w->cfg->echo_shift);
			}
			if(w->session->opt.log_wav || w->cfg->use_log_wav || w->log_audio)
			{
				wtk_wavfile_write(w->echo, var->v.str.data, var->v.str.len);
			}
			if(w->cfg->use_sample)
			{
				wtk_resample_feed(w->sample, var->v.str.data, var->v.str.len, 0);
			}else {
#ifndef OFFLINE_TEST
				if(w->cfg->use_player)
				{
					node = qtk_mqform_msg_new(w);
					nx=0;
					ss=var->v.str.data;
					while(nx<var->v.str.len)
					{
						for(i=0;i<w->cfg->player_cfg.channel;++i)
						{
							wtk_strbuf_push(node->data,ss+nx,2);
						}
						// wtk_strbuf_push(node->data,ss+nx,2);
						// wtk_strbuf_push(node->data,ss+nx,2);
						nx+=2;
					}
					wtk_blockqueue_push(w->single_q,&node->node);

					if(w->single_q->length > w->cfg->max_output_length)
					{
						qtk_mqform_qn_clean2(w);
					}
				}
#endif				
				if(w->cfg->out_channel > 1)
				{
					wtk_strbuf_reset(w->outbuf);
					nx=0;
					ss=var->v.str.data;
					while(nx < var->v.str.len)
					{
						for(i=0;i<w->cfg->out_channel;++i)
						{
							wtk_strbuf_push(w->outbuf, ss+nx, 2);
						}
						nx+=2;
					}
					ov.type = var->type;
					ov.v.str.data = w->outbuf->data;
					ov.v.str.len = w->outbuf->pos;
					if(w->notify_func){
						w->notify_func(w->notify_ths,&ov);
					}
				}else{
					if(w->notify_func){
						w->notify_func(w->notify_ths,var);
					}
				}
			}
			break;
		case QTK_AEC_DIRECTION:
			// wtk_debug("theta=%d\n",var->v.ii.theta);
			break;
		default:
			break;
	}

	// if(w->notify_func){
	// 	w->notify_func(w->notify_ths,var);
	// }
}

void qtk_mqform_on_data(qtk_mqform_t *qform,char *data,int len)
{
	qtk_var_t var;
	char *ss;
	int nx=0,i=0;
	qtk_mqform_msg_t *node = NULL;
#ifndef OFFLINE_TEST
	if(qform->cfg->use_player)
	{	
		node = qtk_mqform_msg_new(qform);
		nx=0;
		ss=data;
		while(nx<len)
		{
			for(i=0;i<qform->cfg->player_cfg.channel;++i)
			{
				wtk_strbuf_push(node->data,ss+nx,2);
			}
			// wtk_strbuf_push(node->data,ss+nx,2);
			// wtk_strbuf_push(node->data,ss+nx,2);
			nx+=2;
		}
		wtk_blockqueue_push(qform->single_q,&node->node);

		if(qform->single_q->length > qform->cfg->max_output_length)
        {
            qtk_mqform_qn_clean2(qform);
        }
	}
#endif

	if(qform->cfg->out_channel > 1)
	{
		wtk_strbuf_reset(qform->outbuf);
		nx=0;
		ss=data;
		while(nx < len)
		{
			for(i=0;i<qform->cfg->out_channel;++i)
			{
				wtk_strbuf_push(qform->outbuf, ss+nx, 2);
			}
			nx+=2;
		}
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.data = qform->outbuf->data;
		var.v.str.len = qform->outbuf->pos;
		if(qform->notify_func){
			qform->notify_func(qform->notify_ths,&var);
		}
	}else{
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.data = data;
		var.v.str.len = len;
		if(qform->notify_func){
			qform->notify_func(qform->notify_ths, &var);
		}
	}
}

#ifndef OFFLINE_TEST
int qtk_mqform_recorder_run(qtk_mqform_t *qform, wtk_thread_t *thread)
{
    wtk_strbuf_t *buf = NULL;
    qtk_mqform_msg_t *node = NULL;
    static int contint_n = 0;
    static int discard_count = 0;
    int ret;
    char msg_data_hdr[QTK_MQFORM_MSG_DATA_HDR_LEN];
	int count=0;
	
    while(qform->recorder_run)
    {
        buf = qtk_recorder_read(qform->recorder);
        if(buf->pos <= 0) 
        {   
            wtk_log_log0(qform->session->log,"recorder error");
            continue;
        }
        if(qform->recorder->cfg->mic_shift != 1.0){
            qtk_mqform_data_change_vol(buf->data, buf->pos, qform->recorder->cfg->mic_shift);
        }

		count++;
		if(count == 50 && qform->cfg->use_log_wav==0 && qform->session->opt.log_wav==0)
		{
			qtk_module_mqform_is_log_audio(qform);
			count=0;
		}
		// if(qform->session->opt.log_wav)
		// {
		// 	wtk_wavfile_write(qform->mic,buf->data,buf->pos);
		// }

        node = qtk_mqform_msg_new(qform);
        wtk_strbuf_push(node->data,buf->data,buf->pos);
        wtk_blockqueue_push(qform->mul_q,&node->node);
    }
    return 0;
}
#endif

int qtk_mqform_engine_run(qtk_mqform_t *qform, wtk_thread_t *thread)
{
    wtk_queue_node_t *qn = NULL;
    qtk_mqform_msg_t *node = NULL;
    int i, j;
	short *pv = NULL;
	int len;
    int si=1;
	if(qform->cfg->use_dsp)
	{
		si=2;
	}

    int channels = qform->cfg->recorder_cfg.channel-qform->cfg->recorder_cfg.nskip;

    while(qform->engine_run)
    {
        qn = wtk_blockqueue_pop(qform->mul_q,-1,NULL); 
        if(!qn) continue;
        node = data_offset2(qn,qtk_mqform_msg_t,node);
        if(node->data->pos > 0){
			// if(qform->mul_q->length > 10)
			// {
			// 	wtk_debug("-==============>>>length=%d\n",qform->mul_q->length);
			// }
#ifdef USE_ORDER
            if(qform->session->opt.log_wav || qform->cfg->use_log_wav || qform->log_audio)
            {
                wtk_wavfile_write(qform->mic, node->data->data, node->data->pos);
            }
            qtk_engine_feed(qform->qform, node->data->data, node->data->pos, 0);
#else
			wtk_strbuf_reset(qform->qformbuf);
			if(qform->cfg->use_dsp)
			{
				char *s,*e;
				s=node->data->data;
				e=node->data->data + node->data->pos;

				while(s<e)
				{
					for(i=si;i<channels;++i)
					{
						wtk_strbuf_push(qform->qformbuf,s + 2*i, 2);
					}
					wtk_strbuf_push(qform->qformbuf,s, 2*si);
					s+=(2*channels);
				}
			}else
			{
				wtk_strbuf_push(qform->qformbuf, node->data->data, node->data->pos);
			}
            if(qform->session->opt.log_wav || qform->cfg->use_log_wav || qform->log_audio)
            {
                wtk_wavfile_write(qform->mic,qform->qformbuf->data,qform->qformbuf->pos);
            }
            qtk_engine_feed(qform->qform, qform->qformbuf->data, qform->qformbuf->pos, 0);
#endif
        }
        qtk_mqform_msg_delete(node);
    }

    return 0;
}

#ifndef OFFLINE_TEST
int qtk_mqform_player_run(qtk_mqform_t *qform,wtk_thread_t *thread)
{
    wtk_queue_node_t *qn = NULL;
    qtk_mqform_msg_t *node = NULL;
	int n = 0;
	int ret;
	int is_start=0;
	int zlen=32*qform->cfg->player_cfg.channel*100;
	char *buf = wtk_malloc(zlen);
	double tm=time_get_ms();

	while(qform->player_run)
	{
		qn = wtk_blockqueue_pop(qform->single_q,-1,NULL);
		if(!qn) continue;
		node = data_offset2(qn,qtk_mqform_msg_t,node);
		n = node->data->pos%2;

		if(is_start == 0)
		{
			ret=qtk_player_start(qform->player,qform->cfg->player_cfg.snd_name, qform->player_rate, qform->cfg->player_cfg.channel, 2);
			if(ret!=0)
			{
				wtk_debug("player start error!\n");
				qtk_mqform_msg_delete(node);
				continue;
			}else
			{
				#if 1
					memset(buf,0,zlen);
					ret = qtk_player_write(qform->player,buf,zlen);
					if(ret > 0){
						wtk_debug("play zero buf %d\n",ret);
					}
				#endif
				is_start = 1;
			}
		}

		// tm = time_get_ms() - tm;
		// if(tm > node->data->pos/128.0)
		// {
		// 	wtk_debug("------------------>>>>>%f/%f\n",tm,node->data->pos/128.0);
		// }
		// tm = time_get_ms();


		if(qform->cfg->use_player)
		{
			ret = qtk_player_write(qform->player,node->data->data,node->data->pos-n);
			if(ret < 0)
			{
				// wtk_debug("==========================>>>>player failed\n");
				qtk_player_stop(qform->player);
				is_start = 0;
			}
		}

		qtk_mqform_msg_delete(node);
        if(qform->single_q->length > 30)
        {
			//wtk_debug("=======================================>>>>>>>clean =%d\n",qform->single_q->length);
            qtk_mqform_qn_clean(qform, qform->single_q, qform->single_q->length);
        }
	}
	wtk_free(buf);
	return 0;	
}
#endif

int qtk_mqform_qn_clean(qtk_mqform_t *qform,wtk_blockqueue_t *queue, int poplen)
{
    wtk_queue_node_t *qn = NULL; 
    qtk_mqform_msg_t *node = NULL;

	int i;
	for(i=0;i<poplen;++i)
    {
        qn = wtk_blockqueue_pop(queue,0,NULL);
        if(!qn) break;
        node = (qtk_mqform_msg_t*)data_offset2(qn,qtk_mqform_msg_t,node);
        qtk_mqform_msg_delete(node);
    }
    return 0;
}

int qtk_mqform_qn_clean2(qtk_mqform_t *qform)
{
    wtk_queue_node_t *qn = NULL; 
    qtk_mqform_msg_t *node = NULL;
	int i=0;
	int len=qform->single_q->length;
	wtk_lock_lock(&qform->mlock);
    while(i<len)
    {
        qn = wtk_blockqueue_pop(qform->single_q,0,NULL);
        if(!qn) break;
        node = (qtk_mqform_msg_t*)data_offset2(qn,qtk_mqform_msg_t,node);
        qtk_mqform_msg_delete(node);
		++i;
    }
	wtk_lock_unlock(&qform->mlock);
    return 0;
}

//buf operation
qtk_mqform_msg_t* qtk_mqform_msg_new(qtk_mqform_t *qform)
{
    qtk_mqform_msg_t *node = NULL;
    node = wtk_malloc(sizeof(qtk_mqform_msg_t));
    if(NULL == node)
    {
        goto end;
    }
    memset(node,0,sizeof(qtk_mqform_msg_t));
    node->data = wtk_strbuf_new(512,1.0);
    if(NULL == node->data)
    {
        wtk_log_log0(qform->session->log,"strbuf new error");
        qtk_mqform_msg_delete(node);
        node = NULL;
        goto end;
    }
end:
    return node;
}

int qtk_mqform_msg_delete(qtk_mqform_msg_t *node)
{
    if(NULL == node)
    {
        return 0; 
    }
    if(node->data)
    {
        wtk_strbuf_delete(node->data);
    }
    wtk_free(node);
    
    return 0;
}

static void qtk_log_wav_file_new(qtk_mqform_t *m)
{
	int channel = m->cfg->recorder_cfg.channel-m->cfg->recorder_cfg.nskip;
	int bytes_per_sample = m->cfg->recorder_cfg.bytes_per_sample;
	int sample_rate = 16000;// m->cfg->rcd.sample_rate;

	if(m->session->opt.cache_path->len <= 0){
		return;
	}
	m->mic = wtk_wavfile_new(sample_rate); 
	m->mic->max_pend = 0;
	wtk_wavfile_set_channel2(m->mic,channel,bytes_per_sample);

	wtk_wavfile_open(m->mic, m->mul_path->data);
	m->echo = wtk_wavfile_new(m->cfg->player_cfg.sample_rate); 
	m->echo->max_pend = 0;
	channel = 1;
	wtk_wavfile_set_channel2(m->echo,channel,bytes_per_sample);
	wtk_wavfile_open(m->echo, m->single_path->data);
}
static void qtk_log_wav_file_delete(qtk_mqform_t *m)
{
	wtk_wavfile_close(m->mic);
	wtk_wavfile_delete(m->mic);		
	wtk_wavfile_close(m->echo);
	wtk_wavfile_delete(m->echo);
	m->mic = NULL;
	m->echo = NULL;
}

void qtk_module_mqform_is_log_audio(qtk_mqform_t *m)
{
	if(m->session->opt.cache_path->len <= 0){
		return;
	}
	if(m->log_audio == 0 && access(m->check_path_buf->data, F_OK)==0){
		qtk_log_wav_file_new(m);
		m->log_audio = 1;
	}else if(m->log_audio == 1 && access(m->check_path_buf->data, F_OK)){
		m->log_audio = 0;
		qtk_log_wav_file_delete(m);
	}
}

void qtk_mqform_data_change_vol(char *data, int bytes, float shift)
{
	short *ps, *pe;
	int num;

	ps = (short *)data;
	pe = (short *)(data + bytes);

	while(ps < pe){
		num = (*ps) *shift;
		if(num > 32767){
			*ps = 32767;
		}else if(num < -32768){
			*ps = -32768;
		}else{
			*ps = num;
		}
		++ps;
	}
}
