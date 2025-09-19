#include "qtk_mqform2_mod.h"
#include <time.h>

#ifndef OFFLINE_TEST
int qtk_mqform2_mod_rcd_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
#endif
#ifdef USE_FOR_DEV
int qtk_mqform2_mod_uart_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
#endif
#ifndef __ANDROID__
int qtk_mqform2_mod_usbaudio_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
int qtk_mqform2_mod_lineout_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
#endif
int qtk_mqform2_mod_qform_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
int qtk_mqform2_mod_aec_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
int qtk_mqform2_mod_qform2_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
int qtk_mqform2_mod_merge_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t);
void qtk_mqform2_mod_on_soundscreen(qtk_mqform2_mod_t *mod, char *data, int len);
void qtk_mqform2_mod_on_eqform(qtk_mqform2_mod_t *m, char *data, int len);
void qtk_mqform2_mod_on_aec(qtk_mqform2_mod_t *m, short **data, int len, int is_end);
void qtk_mqform2_mod_on_beamnet2(qtk_mqform2_mod_t *m, char *data, int len);
static void qtk_log_wav_file_new(qtk_mqform2_mod_t *m);
static void qtk_log_wav_file_delete(qtk_mqform2_mod_t *m);
void qtk_is_log_audio(qtk_mqform2_mod_t *m);
void qtk_mqform2_mod_set_cpu(qtk_mqform2_mod_t *m, wtk_thread_t *thread, int cpunum);

int qtk_mqform2_mod_engine_new(qtk_mqform2_mod_t *m, int type);
void qtk_mqform2_mod_engine_delete(qtk_mqform2_mod_t *m, int type);
void qtk_mqform2_mod_engine_start(qtk_mqform2_mod_t *m, int type);
void qtk_mqform2_mod_engine_stop(qtk_mqform2_mod_t *m, int type);
int qtk_mqform2_mod_engine_feed(qtk_mqform2_mod_t *m, int type, char *data, int len, int is_end);
int qtk_mqform2_mod_engine_feed2(qtk_mqform2_mod_t *m, int type, short **data, int len, int is_end);

qtk_mqform2_mod_t *qtk_mqform2_mod_new(qtk_session_t *session, qtk_mqform2_mod_cfg_t *cfg)
{
	qtk_mqform2_mod_t *m;
	char tmp[64];
	int ret;

	char buf[256];
	wtk_get_build_timestamp(buf);
	printf("BUILD AT %s\n",buf);

	m = (qtk_mqform2_mod_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;

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
		wtk_strbuf_push_f(m->mul_path, "%.*s/mul.%f.wav", m->cfg->cache_path.len, m->cfg->cache_path.data, tt);
		wtk_strbuf_push_c(m->mul_path, 0);
		wtk_strbuf_push_f(m->single_path, "%.*s/single.%f.wav", m->cfg->cache_path.len, m->cfg->cache_path.data, tt);
		wtk_strbuf_push_c(m->single_path, 0);
	}
	if(m->cfg->use_log){
		m->log = session->log;
	}
#endif

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	m->playbuf = wtk_strbuf_new(1024, 1.0);
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_init(&m->usbaudio_queue);
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mqform2_mod_usbaudio_entry, m);
		m->usbaudio = qtk_player_new(&cfg->usbaudio, session, m, NULL);
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mqform2_mod_lineout_entry, m);
		m->lineout = qtk_player2_new(&cfg->lineout);
	}
#endif
#endif

#ifndef OFFLINE_TEST
	m->rcd = qtk_recorder_new(&cfg->rcd, session, m, NULL);
	if(!m->rcd){
		ret = -1;
    	goto end;
	}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mqform2_mod_rcd_entry, m);
#endif

#ifdef USE_FOR_DEV
	m->led = qtk_led_new(&m->cfg->led);
	if(m->led)
	{
		qtk_led_send_cmd(m->led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_GREEN, QTK_LED_CMD_ON);
	}
#endif

	m->msg = qtk_msg_new();
	wtk_thread_init(&m->qform_t,(thread_route_handler)qtk_mqform2_mod_qform_entry, m);
	wtk_blockqueue_init(&m->bfio_queue);
	ret = qtk_mqform2_mod_engine_new(m,m->cfg->audio_type);
	if(m->cfg->audio2_type > 0)
	{
		wtk_thread_init(&m->qform2_t,(thread_route_handler)qtk_mqform2_mod_qform2_entry, m);
		wtk_blockqueue_init(&m->bfio2_queue);
#ifdef USE_BEAMNET
		printf("MOD_BF_BEAMNET2 resname=%.*s\n",m->cfg->beamnet2_fn.len,m->cfg->beamnet2_fn.data);
		m->beamnet2_cfg = qtk_beamnet_cfg_new(m->cfg->beamnet2_fn.data);
		if(!m->beamnet2_cfg){
			ret = -1; goto end;
		}
		m->beamnet2 = qtk_beamnet_new(m->beamnet2_cfg);
		if(!m->beamnet2){
			ret = -1; goto end;
		}
		qtk_beamnet_set_notify(m->beamnet2, m, (qtk_beamnet_notify_f)qtk_mqform2_mod_on_beamnet2);
#endif
		wtk_thread_init(&m->merge_t,(thread_route_handler)qtk_mqform2_mod_merge_entry, m);
		wtk_blockqueue_init(&m->merge_queue);
	}

	if(m->cfg->use_aec)
	{
		wtk_thread_init(&m->aec_t, (thread_route_handler)qtk_mqform2_mod_aec_entry, m);
		wtk_blockqueue_init(&m->aec_queue);
		ret = qtk_mqform2_mod_engine_new(m, QTK_MQFORM2_MOD_BF_AEC);
	}
	

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		wtk_blockqueue_init(&m->uart_queue);
		wtk_thread_init(&m->uart_t, (thread_route_handler)qtk_mqform2_mod_uart_entry, m);
		m->uart_cfg = wtk_main_cfg_new_type(qtk_uart_cfg,m->cfg->uart_fn.data);
		m->uart = qtk_uart_new((qtk_uart_cfg_t *)(m->uart_cfg->cfg));
	}
#endif

	wtk_debug("MOD NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mqform2_mod_delete(m);
      m = NULL;
  }
	return m;
}

void qtk_mqform2_mod_delete(qtk_mqform2_mod_t *m)
{
	wtk_blockqueue_clean(&m->bfio_queue);

#ifndef __ANDROID__
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

	qtk_mqform2_mod_engine_delete(m, m->cfg->audio_type);

	if(m->cfg->audio2_type > 0)
	{
		wtk_blockqueue_clean(&m->bfio2_queue);
		wtk_blockqueue_clean(&m->merge_queue);
#ifdef USE_BEAMNET
		if(m->beamnet2){
			qtk_beamnet_delete(m->beamnet2);
		}
		if(m->beamnet2_cfg){
			qtk_beamnet_cfg_delete(m->beamnet2_cfg);
		}
#endif
	}

	if(m->cfg->use_aec)
	{
		qtk_mqform2_mod_engine_delete(m, QTK_MQFORM2_MOD_BF_AEC);
	}

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

void qtk_mqform2_mod_set_notify(qtk_mqform2_mod_t *m,void *notify_ths,qtk_engine_notify_f notify_func)
{
	m->ths = notify_ths;
	m->notify = notify_func;
}

int qtk_mqform2_mod_start(qtk_mqform2_mod_t *m)
{
	int ret;
	qtk_mqform2_mod_engine_start(m, m->cfg->audio_type);
	m->qform_run = 1;
	wtk_thread_start(&m->qform_t);
	
	if(m->cfg->audio2_type > 0)
	{
#ifdef USE_BEAMNET
		if(m->beamnet2){
			qtk_beamnet_start(m->beamnet2);
		}
#endif
		m->qform2_run = 1;
		wtk_thread_start(&m->qform2_t);
		m->merge_run = 1;
		wtk_thread_start(&m->merge_t);
	}

	if(m->cfg->use_aec)
	{
		qtk_mqform2_mod_engine_start(m, QTK_MQFORM2_MOD_BF_AEC);
		m->aec_run = 1;
		wtk_thread_start(&m->aec_t);
	}
	
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		m->usbaudio_run = 1;
		wtk_thread_start(&m->usbaudio_t);
	}
	if(m->cfg->use_lineout){
		m->lineout_run = 1;
		// ret=qtk_player2_start(m->lineout);
		if(ret!=0)
		{
			wtk_debug("player start error!\n");
		}
		wtk_thread_start(&m->lineout_t);
	}
#endif
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

int qtk_mqform2_mod_stop(qtk_mqform2_mod_t *m)
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
	qtk_mqform2_mod_engine_stop(m, m->cfg->audio_type);

	if(m->cfg->audio2_type > 0)
	{
		m->qform2_run = 0;
		wtk_blockqueue_wake(&m->bfio2_queue);
		wtk_thread_join(&m->qform2_t);

		m->merge_run = 0;
		wtk_blockqueue_wake(&m->merge_queue);
		wtk_thread_join(&m->merge_t);
#ifdef USE_BEAMNET
		if(m->beamnet2){
			qtk_beamnet_reset(m->beamnet2);
		}
#endif	
	}

	if(m->cfg->use_aec)
	{
		m->aec_run = 0;
		wtk_blockqueue_wake(&m->aec_queue);
		wtk_thread_join(&m->aec_t);
		qtk_mqform2_mod_engine_stop(m, QTK_MQFORM2_MOD_BF_AEC);
	}

#ifndef __ANDROID__
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
#endif

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


void qtk_mqform2_mod_clean_queue(qtk_mqform2_mod_t *m, wtk_blockqueue_t *queue)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int len=queue->length;

	int i;
	for(i=0;i<len;++i)
	{
		qn= wtk_blockqueue_pop(queue, 1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		qtk_msg_push_node(m->msg, msg_node);
	}
}

#ifdef USE_FOR_DEV
int qtk_mqform2_mod_uart_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
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
void qtk_mqform2_mod_feed(qtk_mqform2_mod_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node;

#if 0 //def LOG_ORI_AUDIO
	msg_node = qtk_msg_pop_node(m->msg);
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);

	{
		short *pv,*pv1;
		int i,j,k,len;
		int pos,pos1;
		int b;
		int channel=10;
		int nskip=5;
		char skip_channels[]={0, 1, 6, 7, 9};
			
		pv = pv1 = (short*)data;
		pos = pos1 = 0;
		len = len / (2 * channel);
		for(i=0;i < len; ++i){
			for(j=0;j < channel; ++j) {
				b = 0;
				for(k=0;k<nskip;++k) {
					if(j == skip_channels[k]) {
						b = 1;
					}
				}
				if(b) {
					++pos1;
				} else {
						pv[pos++] = pv1[pos1++];
				}
			}
			}
			len = pos << 1;
	}

	if(m->cfg->bfkasr_type > 0)
	{
		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->bfkasr_queue, &msg_node->qn);
	}
#else
	msg_node = qtk_msg_pop_node(m->msg);
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
#endif

}
#endif
#ifndef OFFLINE_TEST
int qtk_mqform2_mod_rcd_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
{
	qtk_mqform2_mod_set_cpu(m, t, 0);
	qtk_var_t var;
	wtk_strbuf_t *buf;
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	wtk_strbuf_t *tbuf=NULL;
	static int count = 0;
	int first=1;
	double tm=time_get_ms();
	tbuf = wtk_strbuf_new(1024, 1.0f);
	int channel=m->cfg->rcd.channel - m->cfg->rcd.nskip;
	int i=0;

	while(m->rcd_run){
		buf = qtk_recorder_read(m->rcd);
		if(buf->pos == 0){
			continue;
		}

		count++;
		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, buf->data, buf->pos);
		if(m->cfg->use_aec)
		{
			wtk_blockqueue_push(&m->aec_queue, &msg_node->qn);
		}else{
			wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
		}
		if(m->cfg->audio2_type > 0)
		{
			msg_node2 = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node2->buf, buf->data, buf->pos);
			wtk_blockqueue_push(&m->bfio2_queue, &msg_node2->qn);
		}
		if(count == 50){
			qtk_is_log_audio(m);
			count = 0;
		}
		if(m->mul){
			wtk_wavfile_write(m->mul, buf->data, buf->pos);
		}
		wtk_strbuf_reset(tbuf);
		i=0;
		while(i<buf->pos)
		{
			wtk_strbuf_push(tbuf, buf->data+i+(2*m->cfg->out_source_channel), 2);
			i+=(2*channel);
		}
		if(m->notify)
		{
			var.type = QTK_VAR_SOURCE_AUDIO;
			var.v.str.data = tbuf->data;
			var.v.str.len = tbuf->pos;
			m->notify(m->ths, &var);
		}
	}
	wtk_strbuf_delete(tbuf);
	return 0;
}
#endif

int qtk_mqform2_mod_qform_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
{
	qtk_mqform2_mod_set_cpu(m, t, 1);
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
    
	while(m->qform_run){
		qn= wtk_blockqueue_pop(&m->bfio_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->bfio_queue.length > 10)
		{
			wtk_debug("record =============>>%d\n",m->bfio_queue.length);
		}
#if 1
		qtk_mqform2_mod_engine_feed(m, m->cfg->audio_type, msg_node->buf->data, msg_node->buf->pos, 0);
#else
		msg_node2 = qtk_msg_pop_node(m->msg);
		msg_node2->type = QTK_MQFORM2_MOD_BF_AECNSPICKUP;
		int channel=m->cfg->rcd.channel - m->cfg->rcd.nskip;
		int i=0;
		while(i<msg_node->buf->pos)
		{
			wtk_strbuf_push(msg_node2->buf, msg_node->buf->data+i, 2);
			i+=(2*channel);
		}
		wtk_blockqueue_push(&m->usbaudio_queue, &msg_node2->qn);
#endif
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mqform2_mod_engine_feed(m, m->cfg->audio_type, NULL, 0, 1);
	return 0;
}

int qtk_mqform2_mod_qform2_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
{
	qtk_mqform2_mod_set_cpu(m, t, 1);
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
    
	while(m->qform2_run){
		qn= wtk_blockqueue_pop(&m->bfio2_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->bfio2_queue.length > 10)
		{
			wtk_debug("record =============>>%d\n",m->bfio2_queue.length);
		}
#ifdef USE_BEAMNET
		qtk_beamnet_feed(m->beamnet2, msg_node->buf->data, msg_node->buf->pos, 0);
#endif

		qtk_msg_push_node(m->msg, msg_node);
	}

#ifdef USE_BEAMNET
	qtk_beamnet_feed(m->beamnet2, NULL, 0, 1);
#endif
	return 0;
}

int qtk_mqform2_mod_merge_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	qtk_msg_node_t *msg_node3;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *buf1;
	wtk_strbuf_t *buf2;
	wtk_strbuf_t *allbuf;
	int i;
	int pos=1024;

	buf1 = wtk_strbuf_new(2048, 1.0);
	buf2 = wtk_strbuf_new(2048, 1.0);
	allbuf = wtk_strbuf_new(4096, 1.0);

	wtk_strbuf_reset(buf1);
	wtk_strbuf_reset(buf2);
	wtk_strbuf_reset(allbuf);

	qtk_mqform2_mod_set_cpu(m, t, 0);

	while(m->merge_run){
		qn= wtk_blockqueue_pop(&m->merge_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		if(msg_node->type == 1)
		{
			wtk_strbuf_push(buf1, msg_node->buf->data, msg_node->buf->pos);
		}else if(msg_node->type == 2)
		{
			wtk_strbuf_push(buf2, msg_node->buf->data, msg_node->buf->pos);
		}
		if(buf1->pos >= pos && buf2->pos >= pos)
		{
			i=0;
			while(i < pos)
			{
				wtk_strbuf_push(allbuf, buf1->data+i, 2);
				wtk_strbuf_push(allbuf, buf2->data+i, 2);
				i+=2;
			}
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
			if(m->cfg->use_usbaudio)
			{
				msg_node2 = qtk_msg_pop_node(m->msg);
				msg_node2->type = QTK_MQFORM2_MOD_BF_EQFORM;
				wtk_strbuf_push(msg_node2->buf, allbuf->data, allbuf->pos);
				// i=0;
				// while(i<pos)
				// {
				// 	wtk_strbuf_push(msg_node2->buf, buf1->data+i, 2);
				// 	wtk_strbuf_push(msg_node2->buf, buf2->data+i, 2);
				// 	i+=2;
				// }
				wtk_blockqueue_push(&m->usbaudio_queue, &msg_node2->qn);
			}

			if(m->notify)
			{
				qtk_var_t var;
				var.type = QTK_SPEECH_DATA_PCM;
				var.v.str.len = allbuf->pos;
				var.v.str.data = allbuf->data;
				m->notify(m->ths, &var);
			}
			if(m->single){
				wtk_wavfile_write(m->single, allbuf->data, allbuf->pos);
			}
			// if(m->cfg->use_lineout)
			// {
			// 	msg_node3 = qtk_msg_pop_node(m->msg);
			// 	i=0;
			// 	while(i<pos)
			// 	{
			// 		wtk_strbuf_push(msg_node3->buf, buf1->data+i, 2);
			// 		wtk_strbuf_push(msg_node3->buf, buf2->data+i, 2);
			// 		i+=2;
			// 	}
			// 	wtk_blockqueue_push(&m->lineout_queue, &msg_node3->qn);
			// }
#endif
#endif
			wtk_strbuf_pop(buf1, NULL, pos);
			wtk_strbuf_pop(buf2, NULL, pos);
			wtk_strbuf_reset(allbuf);
		}


		qtk_msg_push_node(m->msg, msg_node);
	}

	wtk_strbuf_delete(buf1);
	wtk_strbuf_delete(buf2);
	wtk_strbuf_delete(allbuf);
	return 0;
}

int qtk_mqform2_mod_aec_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;

	while(m->aec_run){
		qn= wtk_blockqueue_pop(&m->aec_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->aec_queue.length > 10)
		{
			wtk_debug("record =============>>%d\n",m->aec_queue.length);
		}
#if 1
		qtk_mqform2_mod_engine_feed(m, QTK_MQFORM2_MOD_BF_AEC, msg_node->buf->data, msg_node->buf->pos, 0);
#endif
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mqform2_mod_engine_feed(m, QTK_MQFORM2_MOD_BF_AEC, NULL, 0, 1);
}

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
int qtk_mqform2_mod_usbaudio_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
{
	qtk_mqform2_mod_set_cpu(m, t, 0);
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	int ret=-1;
	int zlen=32*100;
	
	char *buf = wtk_malloc(zlen);

  while(m->usbaudio_run){
		qn= wtk_blockqueue_pop(&m->usbaudio_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		// printf(">>>>>len = %d\n", msg_node->buf->pos);
		// wtk_debug("usbaudio_queue len = %d  bfio_queue len = %d\n", m->usbaudio_queue.length, m->bfio_queue.length);

		if(first == 1){
			int ret = 0;
			ret = qtk_player_start(m->usbaudio, m->cfg->usbaudio.snd_name, m->cfg->usbaudio.sample_rate, m->cfg->usbaudio.channel, m->cfg->usbaudio.bytes_per_sample);
			if(ret!=0)
			{
				wtk_debug("player start error!\n");
			}
			memset(buf,0,zlen);
			ret = qtk_player_write2(m->usbaudio, buf, zlen, 1);
			first = 0;
		}
		if(m->cfg->echo_shift != 1.0f)
		{
			qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
		}
		wtk_strbuf_push(m->playbuf, msg_node->buf->data, msg_node->buf->pos);
		
		if(m->cfg->use_olddevice)
		{
			switch(msg_node->type){
				case QTK_MQFORM2_MOD_BF_SOUNDSCREEN:
					ret = qtk_player_write2(m->usbaudio, m->playbuf->data, m->playbuf->pos, m->sc->cfg->n_theta);
					break;
				default:
					break;
			}
		}else{
			ret = qtk_player_write(m->usbaudio, msg_node->buf->data, msg_node->buf->pos);
		}
		if(ret > 0 && ret < m->playbuf->pos)
		{
			wtk_strbuf_pop(m->playbuf, NULL, ret);
		}else
		{
			wtk_strbuf_reset(m->playbuf);
		}
		// if(ret < 0)
		// {
		// 	wtk_debug("================>>>usbaudio player write error %d\n",ret);
		// }
		qtk_msg_push_node(m->msg, msg_node);
	}
	
	wtk_free(buf);
	qtk_player_stop(m->usbaudio);
  return 0;
}

int qtk_mqform2_mod_lineout_entry(qtk_mqform2_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	int ret=0;
    
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
		ret = qtk_player2_write(m->lineout, msg_node->buf->data, msg_node->buf->pos);
		if(ret < 0)
		{
			wtk_debug("========================>lineout player write error %d\n",ret);
		}
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_player2_stop(m->lineout);
	return 0;
}
#endif
#endif

void qtk_mqform2_mod_on_beamnet2(qtk_mqform2_mod_t *m, char *data, int len)
{
	if(m->cfg->audio2_type > 0)
	{
		qtk_msg_node_t *msg_node;
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 2;
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->merge_queue, &msg_node->qn);
	}
}

void qtk_mqform2_mod_on_soundscreen(qtk_mqform2_mod_t *m, char *data, int len)
{
	if(m->cfg->audio2_type > 0)
	{
		qtk_msg_node_t *msg_node;
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 1;
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->merge_queue, &msg_node->qn);
	}else{
		qtk_msg_node_t *msg_node, *msg_node2;
		
	#ifndef __ANDROID__
	#ifndef OFFLINE_TEST
		if(m->cfg->use_usbaudio){
			if(m->usbaudio_queue.length > m->cfg->max_output_length)
			{
				qtk_mqform2_mod_clean_queue(m, &(m->usbaudio_queue));
			}
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = QTK_MQFORM2_MOD_BF_SOUNDSCREEN;

			if(m->cfg->audio_type == QTK_MQFORM2_MOD_BF_SOUNDSCREEN)
			{
				wtk_strbuf_push(msg_node->buf, data, len);
			}else{
				if(m->cfg->usbaudio.channel == 1)
				{
					wtk_strbuf_push(msg_node->buf, data, len);
				}else{
					int pos = 0, i =0;
					while(pos < len)
					{
						for(i = 0; i < m->cfg->usbaudio.channel; i++)
						{
							wtk_strbuf_push(msg_node->buf, data+pos, 2);
						}
						pos+=2;
					}
				}
			}
			wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
		}
		if(m->cfg->use_lineout){
			if(m->lineout_queue.length > m->cfg->max_output_length)
			{
				qtk_mqform2_mod_clean_queue(m, &(m->lineout_queue));
			}
			msg_node2 = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node2->buf, data, len);
			wtk_blockqueue_push(&m->lineout_queue, &msg_node2->qn);
		}
	#endif
	#endif

		// char tmp[len];
		// int k = 0, i, j;
		// bzero(tmp, len);
		// int step = len / m->sc_cfg->n_theta;
		// for(i = 0; i < step; i+= 2){
		// 	for(j = 0;  j< m->sc_cfg->n_theta; j++){
		// 		memcpy(tmp+k, data+j*step+i,2);
		// 		k+=2;
		// 	}
		// }
		if(m->single){
			wtk_wavfile_write(m->single, data, len);
		}
		if(m->notify)
		{
			qtk_var_t var;
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.len = len;
			var.v.str.data = data;
			m->notify(m->ths, &var);
		}
	}
}

void qtk_mqform2_mod_on_eqform(qtk_mqform2_mod_t *m, char *data, int len)
{
    qtk_msg_node_t *msg_node, *msg_node2;

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		if(m->usbaudio_queue.length > m->cfg->max_output_length)
		{
			wtk_debug("======================>>>>>>>>>>>>>>>>>>>%d\n",m->usbaudio_queue.length);
			qtk_mqform2_mod_clean_queue(m, &(m->usbaudio_queue));
		}
		wtk_debug("=================>>>>>>>>>>>>>>>>>>>>>>>>\n");
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = QTK_MQFORM2_MOD_BF_EQFORM;
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
	}
	if(m->cfg->use_lineout){
		if(m->lineout_queue.length > m->cfg->max_output_length)
		{
			qtk_mqform2_mod_clean_queue(m, &(m->lineout_queue));
		}
		msg_node2 = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node2->buf, data, len);
		wtk_blockqueue_push(&m->lineout_queue, &msg_node2->qn);
	}
#endif
#endif

	if(m->notify)
	{
		qtk_var_t var;
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.len = len;
		var.v.str.data = data;
		m->notify(m->ths, &var);
	}
	if(m->single){
		wtk_wavfile_write(m->single, data, len);
	}
}


void qtk_mqform2_mod_on_aec(qtk_mqform2_mod_t *m, short **data, int len, int is_end)
{
	qtk_mqform2_mod_engine_feed2(m, m->cfg->audio_type, data, len, is_end);
}

int qtk_mqform2_mod_engine_new(qtk_mqform2_mod_t *m, int type)
{
	int ret;
	switch(type){
	case QTK_MQFORM2_MOD_BF_EQFORM:
#ifdef USE_EQFORM
		printf("MOD_BF_EQFORM resname=%.*s\n",m->cfg->eqform_fn.len,m->cfg->eqform_fn.data);
		m->eqform_cfg = qtk_eqform_cfg_new(m->cfg->eqform_fn.data);
		if(!m->eqform_cfg){
			ret = -1; goto end;
		}
		m->eqform = qtk_eqform_new(m->eqform_cfg);
		if(!m->eqform){
			ret = -1; goto end;
		}
		qtk_eqform_set_notify(m->eqform, m, (qtk_eqform_notify_f)qtk_mqform2_mod_on_eqform);
#endif
		break;
	case QTK_MQFORM2_MOD_BF_SOUNDSCREEN:
#ifdef USE_SOUNDSCREEN
		printf("MOD_BF_SOUNDSCREEN resname=%.*s\n",m->cfg->soundscreen_fn.len,m->cfg->soundscreen_fn.data);
		m->sc_cfg = qtk_soundscreen_cfg_new(m->cfg->soundscreen_fn.data);		
		if(!m->sc_cfg){
			ret = -1; goto end;
		}
		m->sc = qtk_soundscreen_new(m->sc_cfg);
		if(!m->sc){
			ret = -1; goto end;
		}
		qtk_soundscreen_set_notify(m->sc, m, (qtk_soundscreen_notify_f)qtk_mqform2_mod_on_soundscreen);
#endif
		break;
	case QTK_MQFORM2_MOD_BF_BEAMNET:
#ifdef USE_BEAMNET
		printf("MOD_BF_BEAMNET resname=%.*s\n",m->cfg->beamnet_fn.len,m->cfg->beamnet_fn.data);
		m->beamnet_cfg = qtk_beamnet_cfg_new(m->cfg->beamnet_fn.data);
		if(!m->beamnet_cfg){
			ret = -1; goto end;
		}
		m->beamnet = qtk_beamnet_new(m->beamnet_cfg);
		if(!m->beamnet){
			ret = -1; goto end;
		}
		qtk_beamnet_set_notify(m->beamnet, m, (qtk_beamnet_notify_f)qtk_mqform2_mod_on_soundscreen);
#endif
		break;
	case QTK_MQFORM2_MOD_BF_AEC:
#ifdef USE_AEC
		printf("MOD_BF_AEC resname=%.*s\n",m->cfg->aec_fn.len,m->cfg->aec_fn.data);
		m->aec_cfg = qtk_aec_cfg_new(m->cfg->aec_fn.data);
		if(!m->aec_cfg){
			ret = -1; goto end;
		}
		m->aec = qtk_aec_new(m->aec_cfg);
		if(!m->aec){
			ret = -1; goto end;
		}
		qtk_aec_set_notify(m->aec, m, (qtk_aec_notify_f)qtk_mqform2_mod_on_aec);
#endif
		break;
	default:
		wtk_debug("not supported bf type, please check res/cfg.\n");
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	return ret;
}

void qtk_mqform2_mod_engine_delete(qtk_mqform2_mod_t *m, int type)
{
	switch(type){
	case QTK_MQFORM2_MOD_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform){
			qtk_eqform_delete(m->eqform);
		}
		if(m->eqform_cfg){
			qtk_eqform_cfg_delete(m->eqform_cfg);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_SOUNDSCREEN:
#ifdef USE_SOUNDSCREEN
		if(m->sc){
			qtk_soundscreen_delete(m->sc);
		}
		if(m->sc_cfg){
			qtk_soundscreen_cfg_delete(m->sc_cfg);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet){
			qtk_beamnet_delete(m->beamnet);
		}
		if(m->beamnet_cfg){
			qtk_beamnet_cfg_delete(m->beamnet_cfg);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_delete(m->aec);
		}
		if(m->aec_cfg)
		{
			qtk_aec_cfg_delete(m->aec_cfg);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mqform2_mod_engine_start(qtk_mqform2_mod_t *m, int type)
{
	switch(type){
	case QTK_MQFORM2_MOD_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform){
			qtk_eqform_start(m->eqform);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_SOUNDSCREEN:
#ifdef USE_SOUNDSCREEN
		if(m->sc){
			qtk_soundscreen_start(m->sc);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet){
			qtk_beamnet_start(m->beamnet);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_start(m->aec);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mqform2_mod_engine_stop(qtk_mqform2_mod_t *m, int type)
{
	switch(type){
	case QTK_MQFORM2_MOD_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform){
			qtk_eqform_reset(m->eqform);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_SOUNDSCREEN:
#ifdef USE_SOUNDSCREEN
		if(m->sc){
			qtk_soundscreen_reset(m->sc);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet){
			qtk_beamnet_reset(m->beamnet);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_reset(m->aec);
		}
#endif
		break;
	default:
		break;
	}
}

int qtk_mqform2_mod_engine_feed(qtk_mqform2_mod_t *m, int type, char *data, int len, int is_end)
{
	switch(type){
	case QTK_MQFORM2_MOD_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform){
			qtk_eqform_feed(m->eqform, data, len, is_end);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_SOUNDSCREEN:
#ifdef USE_SOUNDSCREEN
		if(m->sc){
			qtk_soundscreen_feed(m->sc, data, len, is_end);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet){
			qtk_beamnet_feed(m->beamnet, data, len, is_end);
		}
#endif
		break;
	case QTK_MQFORM2_MOD_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_feed(m->aec, data, len, is_end);
		}
#endif
		break;
	default:
		break;
	}
	return 0;
}

int qtk_mqform2_mod_engine_feed2(qtk_mqform2_mod_t *m, int type, short **data, int len, int is_end)
{
	switch(type){
	case QTK_MQFORM2_MOD_BF_SOUNDSCREEN:
#ifdef USE_SOUNDSCREEN
		if(m->sc){
			qtk_soundscreen_feed2(m->sc, data, len, is_end);
		}
#endif
		break;
	default:
		break;
	}
	return 0;
}

static void qtk_log_wav_file_new(qtk_mqform2_mod_t *m)
{
#ifndef __ANDROID__
	int channel = m->cfg->rcd.channel-m->cfg->rcd.nskip;
	int bytes_per_sample = m->cfg->rcd.bytes_per_sample;
	int sample_rate = m->cfg->rcd.sample_rate;
#else
	int channel = 10;
	int bytes_per_sample = 2;
	int sample_rate = 16000;
#endif	
	if(m->cfg->cache_path.len <= 0){
		return ;
	}
	m->mul = wtk_wavfile_new(sample_rate); 
	m->mul->max_pend = 0;

	wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);

	wtk_wavfile_open(m->mul, m->mul_path->data);

	m->single = wtk_wavfile_new(sample_rate); 
	m->single->max_pend = 0;
	if(m->cfg->audio_type == QTK_MQFORM2_MOD_BF_SOUNDSCREEN){
		channel = m->sc_cfg->n_theta; 
	}else{
		channel = 1; 
	}
	if(m->cfg->audio2_type > 0)
	{
		channel = 2;
	}
	wtk_wavfile_set_channel2(m->single,channel,bytes_per_sample);
	wtk_wavfile_open(m->single, m->single_path->data);

}
static void qtk_log_wav_file_delete(qtk_mqform2_mod_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);		
	wtk_wavfile_close(m->single);
	wtk_wavfile_delete(m->single);

	m->mul = NULL;
	m->single = NULL;
}

void qtk_is_log_audio(qtk_mqform2_mod_t *m)
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

void qtk_mqform2_mod_set_cpu(qtk_mqform2_mod_t *m, wtk_thread_t *thread, int cpunum)
{
#ifdef USE_R328
	cpu_set_t cpuset;
	int ret;
	int num=0;

	num = sysconf(_SC_NPROCESSORS_CONF);
	wtk_log_log(m->log,"cpu number=%d",num);
	CPU_ZERO(&cpuset);
	// __CPU_ZERO_S(0, &cpuset);
	CPU_SET(cpunum, &cpuset);
	ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
	{
        wtk_log_err(m->log, "pthread_setaffinity_np error %d!\n",ret);
	}
	ret = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
	{
        wtk_log_err(m->log, "pthread_getaffinity_np error %d!\n",ret);
	}

	wtk_log_log0(m->log, "Set returned by pthread_getaffinity_np() contained:\n");
    printf("Set returned by pthread_getaffinity_np() contained:\n");
	int j;
    for (j = 0; j < 2; j++)
	{
        if (CPU_ISSET(j, &cpuset))
		{
			wtk_log_log(m->log, "    CPU %d\n", j);
            printf("    CPU %d\n", j);
		}
	}
#endif

#ifdef USE_SLB
	cpu_set_t cpuset;
	int ret;
	int num=0;

	num = sysconf(_SC_NPROCESSORS_CONF);
	wtk_log_log(m->log,"cpu number=%d",num);
	// CPU_ZERO(&cpuset);
	__CPU_ZERO_S(0, &cpuset);
	// CPU_SET(cpunum, &cpuset);
	__CPU_SET_S(cpunum, sizeof(cpu_set_t), &cpuset);
	ret = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
	{
        wtk_log_err(m->log, "pthread_setaffinity_np error %d!\n",ret);
	}
	ret = pthread_getaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
	{
        wtk_log_err(m->log, "pthread_getaffinity_np error %d!\n",ret);
	}

	wtk_log_log0(m->log, "Set returned by pthread_getaffinity_np() contained:\n");
    printf("Set returned by pthread_getaffinity_np() contained:\n");
	int j;
    for (j = 0; j < 4; j++)
	{
		// if(CPU_ISSET(j, &cpuset))
        if (__CPU_ISSET_S(j, sizeof(cpu_set_t), &cpuset))
		{
			wtk_log_log(m->log, "    CPU %d\n", j);
            printf("    CPU %d\n", j);
		}
	}
#endif
}
