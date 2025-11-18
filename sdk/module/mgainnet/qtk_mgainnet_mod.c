#include "qtk_mgainnet_mod.h"
#include <time.h>

#ifndef OFFLINE_TEST
int qtk_mgainnet_mod_rcd_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t);
#endif
#ifdef USE_FOR_DEV
int qtk_mgainnet_mod_uart_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t);
#endif

int qtk_mgainnet_mod_usbaudio_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t);
int qtk_mgainnet_mod_lineout_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t);
int qtk_mgainnet_mod_qform_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t);
static void qtk_log_wav_file_new(qtk_mgainnet_mod_t *m);
static void qtk_log_wav_file_delete(qtk_mgainnet_mod_t *m);
void qtk_mgainnet_is_log_audio(qtk_mgainnet_mod_t *m);

int qtk_mgainnet_mod_engine_new(qtk_mgainnet_mod_t *m);
void qtk_mgainnet_mod_engine_delete(qtk_mgainnet_mod_t *m);
void qtk_mgainnet_mod_engine_start(qtk_mgainnet_mod_t *m);
void qtk_mgainnet_mod_engine_stop(qtk_mgainnet_mod_t *m);
int qtk_mgainnet_mod_engine_feed(qtk_mgainnet_mod_t *m, char *data, int len, int is_end);
void qtk_mgainnet_mod_on_engine_data(qtk_mgainnet_mod_t *m, qtk_var_t *var);
void qtk_mgainnet_mod_on_data(qtk_mgainnet_mod_t *m, char *data, int len);

qtk_mgainnet_mod_t *qtk_mgainnet_mod_new(qtk_session_t *session, qtk_mgainnet_mod_cfg_t *cfg)
{
	qtk_mgainnet_mod_t *m;
	char tmp[64];
	int ret;

	char buf[256];
	wtk_get_build_timestamp(buf);
	printf("BUILD AT %s\n",buf);

	m = (qtk_mgainnet_mod_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;
	m->session = session;

	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
	m->single_path = wtk_strbuf_new(64,0);
#ifndef OFFLINE_TEST
	if(m->cfg->cache_path.len > 0){
		double tt;
		tt=time_get_ms();

		wtk_mkdir(m->cfg->cache_path.data);
		wtk_strbuf_push_f(m->check_path_buf, "%.*s/start_log_audio", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->check_path_buf, 0);
		wtk_strbuf_push_f(m->mul_path, "%.*s/mul.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->mul_path, 0);
		wtk_strbuf_push_f(m->single_path, "%.*s/single.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->single_path, 0);
	}

	if(m->cfg->use_log){
		m->log = session->log;
	}
#endif
	if(m->cfg->use_log_wav)
	{
		qtk_log_wav_file_new(m);
	}

	if(m->cfg->use_resample)
	{
		m->resample = wtk_resample_new(128);
		wtk_resample_set_notify(m->resample, m, (wtk_resample_notify_f)qtk_mgainnet_mod_on_data);
	}

#ifndef OFFLINE_TEST
	m->playbuf = wtk_strbuf_new(1024, 1.0);
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_init(&m->usbaudio_queue);
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mgainnet_mod_usbaudio_entry, m);
		m->usbaudio = qtk_player_new(&cfg->usbaudio, session, m, NULL);
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mgainnet_mod_lineout_entry, m);
		m->lineout = qtk_player2_new(&cfg->lineout);
	}
#endif

#ifndef OFFLINE_TEST
	m->rcd = qtk_recorder_new(&cfg->rcd, session, m, NULL);
	if(!m->rcd){
		ret = -1;
    	goto end;
	}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mgainnet_mod_rcd_entry, m);
#endif

#ifdef USE_FOR_DEV
	m->led = qtk_led_new(&m->cfg->led);
	if(m->led)
	{
		qtk_led_send_cmd(m->led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_GREEN, QTK_LED_CMD_ON);
	}
#endif

	m->msg = qtk_msg_new();
	wtk_thread_init(&m->qform_t,(thread_route_handler)qtk_mgainnet_mod_qform_entry, m);
	wtk_blockqueue_init(&m->bfio_queue);
	ret = qtk_mgainnet_mod_engine_new(m);

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		wtk_blockqueue_init(&m->uart_queue);
		wtk_thread_init(&m->uart_t, (thread_route_handler)qtk_mgainnet_mod_uart_entry, m);
		m->uart_cfg = wtk_main_cfg_new_type(qtk_uart_cfg,m->cfg->uart_fn.data);
		m->uart = qtk_uart_new((qtk_uart_cfg_t *)(m->uart_cfg->cfg));
	}
#endif

	wtk_debug("MOD NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mgainnet_mod_delete(m);
      m = NULL;
  }
	return m;
}

void qtk_mgainnet_mod_delete(qtk_mgainnet_mod_t *m)
{
	wtk_blockqueue_clean(&m->bfio_queue);
#ifndef OFFLINE_TEST
	if(m->cfg->use_lineout){
		wtk_blockqueue_clean(&m->lineout_queue);
		if(m->lineout){
			qtk_player2_delete(m->lineout);
		}
	}
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_clean(&m->usbaudio_queue);
		if(m->usbaudio){
			qtk_player_delete(m->usbaudio);
		}
	}
	if(m->playbuf)
	{
		wtk_strbuf_delete(m->playbuf);
	}
#endif

#ifndef OFFLINE_TEST
	if(m->rcd){
			qtk_recorder_delete(m->rcd);
	}
#endif
	if(m->msg){
			qtk_msg_delete(m->msg);
	}
#ifdef USE_FOR_DEV
	if(m->led){
		qtk_led_send_cmd(m->led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_INVALID, QTK_LED_CMD_OFF);
		qtk_led_delete(m->led);
	}
#endif

	qtk_mgainnet_mod_engine_delete(m);

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		wtk_blockqueue_clean(&m->uart_queue);
		if(m->uart)
		{
			qtk_uart_delete(m->uart);
		}
		if(m->uart_cfg)
		{
			wtk_main_cfg_delete(m->uart_cfg);
		}
	}
#endif

	if(m->resample)
	{
		wtk_resample_delete(m->resample);
	}

	if(m->cfg->use_log_wav)
	{
		qtk_log_wav_file_delete(m);
	}

	if(m->check_path_buf){
		wtk_strbuf_delete(m->check_path_buf);
	}
	if(m->mul_path){
		wtk_strbuf_delete(m->mul_path);
	}
	if(m->single_path){
		wtk_strbuf_delete(m->single_path);
	}

    wtk_free(m);
}

void qtk_mgainnet_mod_set_notify(qtk_mgainnet_mod_t *m,void *notify_ths,qtk_engine_notify_f notify_func)
{
	m->ths = notify_ths;
	m->notify = notify_func;
}

int qtk_mgainnet_mod_start(qtk_mgainnet_mod_t *m)
{
	int ret;
	qtk_mgainnet_mod_engine_start(m);
	m->qform_run = 1;
	wtk_thread_start(&m->qform_t);
	if(m->cfg->use_resample && m->cfg->usbaudio.sample_rate != 16000)
	{
		wtk_resample_start(m->resample, 16000, m->cfg->usbaudio.sample_rate);
	}

#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		m->usbaudio_run = 1;
		wtk_thread_start(&m->usbaudio_t);
	}	
	if(m->cfg->use_lineout){
		m->lineout_run = 1;
		wtk_thread_start(&m->lineout_t);
	}
#endif

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		m->uart_run = 1;
		wtk_thread_start(&m->uart_t);
	}
#endif

#ifndef OFFLINE_TEST
	ret=qtk_recorder_start(m->rcd);
	if(ret!=0)
	{
		wtk_debug("recorder start error!\n");
	}
    m->rcd_run = 1;
    wtk_thread_start(&m->rcd_t);
#endif
	return 0;
}

int qtk_mgainnet_mod_stop(qtk_mgainnet_mod_t *m)
{
#ifndef OFFLINE_TEST
	m->rcd_run = 0;
	wtk_thread_join(&m->rcd_t);
	if(m->rcd)
	{
		qtk_recorder_stop(m->rcd);
	}
#endif
	m->qform_run = 0;
	wtk_blockqueue_wake(&m->bfio_queue);
	wtk_thread_join(&m->qform_t);
	qtk_mgainnet_mod_engine_stop(m);

#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		m->usbaudio_run = 0;
		wtk_blockqueue_wake(&m->usbaudio_queue);
		wtk_thread_join(&m->usbaudio_t);
	}
	if(m->cfg->use_lineout){
		m->lineout_run = 0;
		wtk_blockqueue_wake(&m->lineout_queue);
		wtk_thread_join(&m->lineout_t);
	}
#endif

	if(m->cfg->use_resample && m->cfg->usbaudio.sample_rate != 16000)
	{
		wtk_resample_close(m->resample);
	}

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		if(m->uart_run)
		{
			m->uart_run = 0;
			wtk_blockqueue_wake(&m->uart_queue);
			wtk_thread_join(&m->uart_t);
		}
	}
#endif
	return 0;
}

#ifdef USE_FOR_DEV
int qtk_mgainnet_mod_uart_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	char *out=(char *)wtk_malloc(10);
	int olen;
	int i;
    
	while(m->uart_run){
		qn= wtk_blockqueue_pop(&m->uart_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		qtk_comm_get_number(msg_node->buf->data, msg_node->buf->pos,out, &olen);
		if(m->uart)
		{
			qtk_uart_write(m->uart, out, olen);
		}

		qtk_msg_push_node(m->msg, msg_node);
	}

	wtk_free(out);
}
#endif

#ifdef OFFLINE_TEST
void qtk_mgainnet_mod_feed(qtk_mgainnet_mod_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node;

#if 1 //def LOG_ORI_AUDIO
	msg_node = qtk_msg_pop_node(m->msg);
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
#endif

}
#endif

#ifndef OFFLINE_TEST
int qtk_mgainnet_mod_rcd_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *buf;
	qtk_msg_node_t *msg_node;
	static int count = 0;

	while(m->rcd_run){
		buf = qtk_recorder_read(m->rcd);
		if(buf->pos == 0){
			continue;
		}

		if(m->rcd->cfg->use_log_ori_audio == 0)
		{
			count++;
			if(count == 100){
				qtk_mgainnet_is_log_audio(m); 
				count = 0;
			}
			if(m->mul && m->cfg->use_log_wav){
				wtk_wavfile_write(m->mul, buf->data, buf->pos);
			}

			msg_node = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node->buf, buf->data, buf->pos);
			wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
		}else{
			count++;
			msg_node = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node->buf, buf->data, buf->pos);
			wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
			if(count == 50){
				qtk_mgainnet_is_log_audio(m);
				count = 0;
			}
			if(m->mul && m->cfg->use_log_wav){
				wtk_wavfile_write(m->mul, buf->data, buf->pos);
			}
		}
	}
	return 0;
}
#endif

int qtk_mgainnet_mod_qform_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	
	while(m->qform_run){
		qn= wtk_blockqueue_pop(&m->bfio_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		// if(m->bfio_queue.length > 1)
		// {
		// 	wtk_debug("record =============>>%d\n",m->bfio_queue.length);
		// }
#if 1
		qtk_mgainnet_mod_engine_feed(m, msg_node->buf->data, msg_node->buf->pos, 0);
#endif
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mgainnet_mod_engine_feed(m, NULL, 0, 1);
	return 0;
}

#ifndef OFFLINE_TEST
int qtk_mgainnet_mod_usbaudio_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	double tm=0.0;
	
	char *buf = wtk_malloc(16000);

  	while(m->usbaudio_run){
		qn= wtk_blockqueue_pop(&m->usbaudio_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		// printf(">>>>>len = %d\n", msg_node->buf->pos);
		// if(m->usbaudio_queue.length > 1)
		// {
		// 	wtk_debug("====================>>>>>>>>>>>>>usbaudio_queue=%d\n",m->usbaudio_queue.length);
		// }

		if(first == 1){
			int ret = 0;
			ret = qtk_player_start(m->usbaudio, m->cfg->usbaudio.snd_name, m->cfg->usbaudio.sample_rate, m->cfg->usbaudio.channel, m->cfg->usbaudio.bytes_per_sample);
			if(ret!=0)
			{
				wtk_debug("player start error!\n");
			}

			// memset(buf,0,16000);
			// ret = qtk_player_write2(m->usbaudio, buf, 16000, 1);
			first = 0;
		}
		tm = time_get_ms() - tm;
		if(tm > m->cfg->usbaudio.period_time*2)
		{
			wtk_debug("===========>>>>>>>>>>>>>>>>>>>tm=%f\n",tm);
		}
		if(m->cfg->echo_shift != 1.0f)
		{
			qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
		}
		qtk_player_write2(m->usbaudio, msg_node->buf->data, msg_node->buf->pos, 1);

		if(m->single && m->cfg->use_log_wav){
			wtk_wavfile_write(m->single, msg_node->buf->data, msg_node->buf->pos);
		}

		qtk_msg_push_node(m->msg, msg_node);
		tm = time_get_ms();
	}
	
	wtk_free(buf);
	qtk_player_stop(m->usbaudio);
  	return 0;
}

int qtk_mgainnet_mod_lineout_entry(qtk_mgainnet_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
    
	while(m->lineout_run){
		qn= wtk_blockqueue_pop(&m->lineout_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(first == 1){
			qtk_player2_start(m->lineout);
			first = 0;
		}
		
		if(m->cfg->echo_shift != 1.0f)
		{
			qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
		}
		qtk_player2_write(m->lineout, msg_node->buf->data, msg_node->buf->pos);
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_player2_stop(m->lineout);
	return 0;
}
#endif

void qtk_mgainnet_mod_on_data(qtk_mgainnet_mod_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node, *msg_node2;
	// wtk_debug("==================>>>>>>>>>>>>>>>>>>>>len=%d\n",len);
#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		msg_node = qtk_msg_pop_node(m->msg);
		int nx=0;
		char *ss;
		ss=data;
		while(nx < len)
		{
			wtk_strbuf_push(msg_node->buf, ss+nx, 2);
			wtk_strbuf_push(msg_node->buf, ss+nx, 2);
			nx+=2;
		}

		wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
	}
	if(m->cfg->use_lineout){
		msg_node2 = qtk_msg_pop_node(m->msg);
		int nx=0;
		char *ss;
		ss=data;
		while(nx < len)
		{
			wtk_strbuf_push(msg_node2->buf, ss+nx, 2);
			wtk_strbuf_push(msg_node2->buf, ss+nx, 2);
			nx+=2;
		}
		wtk_blockqueue_push(&m->lineout_queue, &msg_node2->qn);
	}
#endif
}

void qtk_mgainnet_mod_on_engine_data(qtk_mgainnet_mod_t *m, qtk_var_t *var)
{
	qtk_msg_node_t *msg_node, *msg_node2;

	switch (var->type)
	{
	case QTK_SPEECH_DATA_PCM:
		if(m->cfg->use_resample && m->cfg->usbaudio.sample_rate != 16000)
		{
			// wtk_debug("======================+>>>>>>>>>>>>>>>>>>>>>\n");
			wtk_resample_feed(m->resample, var->v.str.data, var->v.str.len, 0);
		}else{
#ifndef OFFLINE_TEST
			if(m->cfg->use_usbaudio){
				msg_node = qtk_msg_pop_node(m->msg);
				if(m->cfg->usbaudio.channel == 1)
				{
					wtk_strbuf_push(msg_node->buf, var->v.str.data, var->v.str.len);
				}else{
					int nx=0;
					int i;
					char *ss;
					ss=var->v.str.data;
					while(nx < var->v.str.len)
					{
						i=0;
						while(i<m->cfg->usbaudio.channel)
						{
							wtk_strbuf_push(msg_node->buf, ss+nx, 2);
							i++;
						}
						nx+=2;
					}
					// wtk_debug("======================>>>>>>>>>>>%d %d %d\n",nx,var->v.str.len, msg_node->buf->pos);
				}
				wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
			}
			if(m->cfg->use_lineout){
				msg_node2 = qtk_msg_pop_node(m->msg);
				if(m->cfg->usbaudio.channel == 1)
				{
					wtk_strbuf_push(msg_node2->buf, var->v.str.data, var->v.str.len);
				}else{
					int nx=0;
					int i;
					char *ss;
					ss=var->v.str.data;
					while(nx < var->v.str.len)
					{
						i=0;
						while(i<m->cfg->usbaudio.channel)
						{
							wtk_strbuf_push(msg_node2->buf, ss+nx, 2);
							i++;
						}
						nx+=2;
					}
				}
				wtk_blockqueue_push(&m->lineout_queue, &msg_node2->qn);
			}
#endif
		}
		if(m->notify)
		{
			m->notify(m->ths, var);
		}
		break;
	default:
		break;
	}
}


int qtk_mgainnet_mod_engine_new(qtk_mgainnet_mod_t *m)
{
	int ret=-1;

	m->engine = qtk_engine_new(m->session, m->cfg->engine_params);
	if(!(m->engine))
	{
		wtk_debug("engine new faild\n");
		goto end;
	}
	qtk_engine_set_notify(m->engine, m, (qtk_engine_notify_f)qtk_mgainnet_mod_on_engine_data);

	ret = 0;
end:
	return ret;
}

void qtk_mgainnet_mod_engine_delete(qtk_mgainnet_mod_t *m)
{
	if(m->engine)
	{
		qtk_engine_delete(m->engine);
	}
}

void qtk_mgainnet_mod_engine_start(qtk_mgainnet_mod_t *m)
{
	if(m->engine)
	{
		qtk_engine_start(m->engine);
	}
}

void qtk_mgainnet_mod_engine_stop(qtk_mgainnet_mod_t *m)
{
	if(m->engine)
	{
		qtk_engine_reset(m->engine);
	}
}

int qtk_mgainnet_mod_engine_feed(qtk_mgainnet_mod_t *m, char *data, int len, int is_end)
{
	int ret=-1;
	if(m->engine)
	{
		ret = qtk_engine_feed(m->engine, data, len, is_end);
	}
	return ret;
}

static void qtk_log_wav_file_new(qtk_mgainnet_mod_t *m)
{
	int channel = m->cfg->rcd.channel-m->cfg->rcd.nskip;
	int bytes_per_sample = m->cfg->rcd.bytes_per_sample;
	int sample_rate = m->cfg->rcd.sample_rate;
	
	if(m->cfg->cache_path.len <= 0){
		return ;
	}
	m->mul = wtk_wavfile_new(sample_rate); 
	m->mul->max_pend = 0;

	wtk_debug("==============>>>>>>>>>>>>>.channel=%d %d %d\n",channel,m->cfg->rcd.channel,m->cfg->rcd.nskip);
	wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);

	wtk_wavfile_open(m->mul, m->mul_path->data);

	m->single = wtk_wavfile_new(sample_rate); 
	m->single->max_pend = 0;
	channel = m->cfg->usbaudio.channel;
	wtk_wavfile_set_channel2(m->single,channel,bytes_per_sample);
	wtk_wavfile_open(m->single, m->single_path->data);

}
static void qtk_log_wav_file_delete(qtk_mgainnet_mod_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);		
	wtk_wavfile_close(m->single);
	wtk_wavfile_delete(m->single);
	m->mul = NULL;
	m->single = NULL;
}

void qtk_mgainnet_is_log_audio(qtk_mgainnet_mod_t *m)
{
	if(m->cfg->cache_path.len <= 0){
		return ;
	}
	if(m->log_audio == 0 && access(m->check_path_buf->data, F_OK)==0){
		qtk_log_wav_file_new(m);
		m->log_audio = 1;
	}else if(m->log_audio == 1 && access(m->check_path_buf->data, F_OK)){
		m->log_audio = 0;
		qtk_log_wav_file_delete(m);
	}
}
