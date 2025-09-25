#include "qtk_mqform_mod.h"
#include <time.h>

#ifndef OFFLINE_TEST
int qtk_mqform_mod_rcd_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);
#endif
#ifdef USE_FOR_DEV
int qtk_mqform_mod_uart_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);
#endif

int qtk_mqform_mod_usbaudio_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);
int qtk_mqform_mod_lineout_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);

#ifdef USE_RECORD_PLAYER
int qtk_mqform_mod_two_player_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);
int qtk_mqform_mod_two_record_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);
#endif
int qtk_mqform_mod_qform_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);
int qtk_mqform_mod_ssl_entry(qtk_mqform_mod_t *m, wtk_thread_t *t);
void qtk_mqform_mod_on_vboxebf(qtk_mqform_mod_t *mod, char *data, int len);
void qtk_mqform_mod_engine_on_data(qtk_mqform_mod_t *m, qtk_var_t *var);
void qtk_mqform_mod_engine_on_aec_data(qtk_mqform_mod_t *m, qtk_var_t *var);
void qtk_mqform_mod_engine_on_ssl(qtk_mqform_mod_t *m, qtk_var_t *var);
static void qtk_log_wav_file_new(qtk_mqform_mod_t *m);
static void qtk_log_wav_file_delete(qtk_mqform_mod_t *m);
void qtk_mqform_is_log_audio(qtk_mqform_mod_t *m);
void qtk_mqform_mod_on_resample(qtk_mqform_mod_t *m, char *data, int len);
void qtk_mod_set_cpu(qtk_mqform_mod_t *m, wtk_thread_t *thread, int cpunum);

int qtk_mqform_mod_engine_new(qtk_mqform_mod_t *m, int type);
void qtk_mqform_mod_engine_delete(qtk_mqform_mod_t *m, int type);
void qtk_mqform_mod_engine_start(qtk_mqform_mod_t *m, int type);
void qtk_mqform_mod_engine_stop(qtk_mqform_mod_t *m, int type);
int qtk_mqform_mod_engine_feed(qtk_mqform_mod_t *m, int type, char *data, int len, int is_end);

qtk_mqform_mod_t *qtk_mqform_mod_new(qtk_session_t *session, qtk_mqform_mod_cfg_t *cfg)
{
	qtk_mqform_mod_t *m;
	char tmp[64];
	int ret;

	char buf[256];
	wtk_get_build_timestamp(buf);
	printf("BUILD AT %s\n",buf);

	m = (qtk_mqform_mod_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;
	m->log = NULL;
	m->in_resample=NULL;
	m->resample=NULL;
	m->vboxebf_cfg=NULL;
	m->vboxebf=NULL;
	m->aec_cfg = NULL;
	m->aec = NULL;
	m->ssl_cfg = NULL;
	m->ssl = NULL;
#ifndef OFFLINE_TEST
	m->usbaudio=NULL;
	m->lineout=NULL;
	m->rcd=NULL;
#endif

	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
	m->single_path = wtk_strbuf_new(64,0);
	m->use_bfio = 1;

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

	// if(m->cfg->use_log){
	// 	m->log = session->log;
	// }

	if(m->cfg->use_log_wav)
	{
		qtk_log_wav_file_new(m);
	}

#ifndef OFFLINE_TEST
	m->playbuf = wtk_strbuf_new(1024, 1.0);
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_init(&m->usbaudio_queue);
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mqform_mod_usbaudio_entry, m);
		m->usbaudio = qtk_player_new(&cfg->usbaudio, session, m, NULL);
		if(!m->usbaudio){ret = -1;goto end;}
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mqform_mod_lineout_entry, m);
		m->lineout = qtk_player2_new(&cfg->lineout);
		if(!m->lineout){ret = -1;goto end;}
	}
#endif
	int channel=m->cfg->rcd.channel-m->cfg->rcd.nskip;

	m->resample = wtk_resample_new(960);
	if(!m->resample){ret = -1;goto end;}
	wtk_resample_set_notify(m->resample, m, (wtk_resample_notify_f)qtk_mqform_mod_on_resample);

#ifdef USE_RECORD_PLAYER
	if(m->cfg->use_record_player)
	{
		wtk_blockqueue_init(&m->two_play_queue);
		wtk_thread_init(&m->two_player_t,(thread_route_handler)qtk_mqform_mod_two_player_entry, m);
		m->two_ply = qtk_player_new(&cfg->two_ply, session, m, NULL);
		if(!m->two_ply){ret = -1;goto end;}
		m->two_rcd = qtk_recorder_new(&cfg->two_rcd, session, m, NULL);
		if(!m->two_rcd){ret = -1;goto end;}
		wtk_thread_init(&m->two_record_t, (thread_route_handler)qtk_mqform_mod_two_record_entry, m);
	}
#endif

#ifndef OFFLINE_TEST
	m->rcd = qtk_recorder_new(&cfg->rcd, session, m, NULL);
	if(!m->rcd){ret = -1;goto end;}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mqform_mod_rcd_entry, m);
#endif

#ifdef USE_FOR_DEV
	m->led = qtk_led_new(&m->cfg->led);
	if(m->led)
	{
		qtk_led_send_cmd(m->led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_GREEN, QTK_LED_CMD_ON);
	}
#endif

	m->msg = qtk_msg_new();
	wtk_thread_init(&m->qform_t,(thread_route_handler)qtk_mqform_mod_qform_entry, m);
	wtk_blockqueue_init(&m->bfio_queue);
	if(m->cfg->use_in_resample)
	{
		m->in_resample = speex_resampler_init(channel, 48000, 16000, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
		if(!m->in_resample){ret = -1;goto end;}
	}
	ret = qtk_mqform_mod_engine_new(m,m->cfg->audio_type);
	if(ret!=0){wtk_log_err(m->log, "audio engine new fiald! type=%d", m->cfg->audio_type);goto end;}

	if(m->cfg->use_ssl)
	{
		wtk_thread_init(&m->ssl_t,(thread_route_handler)qtk_mqform_mod_ssl_entry, m);
		wtk_blockqueue_init(&m->ssl_queue);
		ret = qtk_mqform_mod_engine_new(m,m->cfg->ssl_type);
		if(ret!=0){wtk_log_err(m->log, "ssl engine new fiald! type=%d", m->cfg->ssl_type);goto end;}
	}

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		wtk_blockqueue_init(&m->uart_queue);
		wtk_thread_init(&m->uart_t, (thread_route_handler)qtk_mqform_mod_uart_entry, m);
		m->uart_cfg = wtk_main_cfg_new_type(qtk_uart_cfg,m->cfg->uart_fn.data);
		m->uart = qtk_uart_new((qtk_uart_cfg_t *)(m->uart_cfg->cfg));
	}
#endif

	wtk_debug("MOD NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mqform_mod_delete(m);
      	m = NULL;
	}
	return m;
}

void qtk_mqform_mod_delete(qtk_mqform_mod_t *m)
{
	wtk_blockqueue_clean(&m->bfio_queue);
	if(m->cfg->use_ssl)
	{
		wtk_blockqueue_clean(&m->ssl_queue);
	}

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

#ifdef USE_RECORD_PLAYER
	if(m->cfg->use_record_player)
	{
		wtk_blockqueue_clean(&m->two_play_queue);
		if(m->two_ply){
			qtk_player_delete(m->two_ply);
		}
		if(m->two_rcd){
			qtk_recorder_delete(m->two_rcd);
		}
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

	if(m->in_resample)
	{
		speex_resampler_destroy(m->in_resample);
		m->in_resample = NULL;
	}

	if(m->resample)
	{
		wtk_resample_delete(m->resample);
	}

	qtk_mqform_mod_engine_delete(m, m->cfg->audio_type);
	if(m->cfg->use_ssl)
	{
		qtk_mqform_mod_engine_delete(m, m->cfg->ssl_type);
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

void qtk_mqform_mod_set_notify(qtk_mqform_mod_t *m,void *notify_ths,qtk_engine_notify_f notify_func)
{
	m->ths = notify_ths;
	m->notify = notify_func;
}

int qtk_mqform_mod_set(qtk_mqform_mod_t *m,char *data)
{
	wtk_cfg_file_t *cfile = NULL;
	wtk_cfg_item_t *item;
	wtk_queue_node_t *qn;
	int ret;
	int bytes=strlen(data);
	int card;
	int channel;
	int rate;

	cfile = wtk_cfg_file_new();
	if(!cfile) {return -1;}

	wtk_debug("%.*s\n", bytes, data);
	ret = wtk_cfg_file_feed(cfile,data,bytes);
	if(ret != 0 ) {goto end;}

	for(qn=cfile->main->cfg->queue.pop;qn;qn=qn->next) {
		item = data_offset2(qn,wtk_cfg_item_t,n);
		if(wtk_string_cmp_s(item->key, "in_card") == 0) {
			card = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp_s(item->key, "in_channel") == 0) {
			channel = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp_s(item->key, "in_rate") == 0) {
			rate = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}

		if(wtk_string_cmp_s(item->key, "out_card") == 0) {
			card = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp_s(item->key, "out_channel") == 0) {
			channel = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}
		if(wtk_string_cmp_s(item->key, "out_rate") == 0) {
			rate = wtk_str_atoi(item->value.str->data, item->value.str->len);
		}
	}

end:
	if(cfile) {
		wtk_cfg_file_delete(cfile);
	}
}

int qtk_mqform_mod_start(qtk_mqform_mod_t *m)
{
	int ret;

	if(m->cfg->use_resample)
	{
		wtk_debug("====================>>>>>>>>>>>>>>>>>resample_rate=%d\n",m->cfg->resample_rate);
		wtk_resample_start(m->resample, 16000, m->cfg->resample_rate);
	}
	qtk_mqform_mod_engine_start(m, m->cfg->audio_type);
	m->qform_run = 1;
	wtk_thread_start(&m->qform_t);

	if(m->cfg->use_ssl)
	{
		qtk_mqform_mod_engine_start(m, m->cfg->ssl_type);
		m->ssl_run = 1;
		wtk_thread_start(&m->ssl_t);
	}

#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		m->usbaudio_run = 1;
		wtk_thread_start(&m->usbaudio_t);
	}
	if(m->cfg->use_lineout){
		m->lineout_run = 1;
		// ret=qtk_player2_start(m->lineout);
		// if(ret!=0)
		// {
		// 	wtk_debug("player start error!\n");
		// }
		wtk_thread_start(&m->lineout_t);
	}
#endif

#ifdef USE_RECORD_PLAYER
	if(m->cfg->use_record_player)
	{
		m->two_player_run = 1;
		wtk_thread_start(&m->two_player_t);

		ret=qtk_recorder_start(m->two_rcd);
		if(ret!=0)
		{
			wtk_debug("two recorder start error!\n");
		}
		m->two_record_run = 1;
		wtk_thread_start(&m->two_record_t);
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

int qtk_mqform_mod_stop(qtk_mqform_mod_t *m)
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
	qtk_mqform_mod_engine_stop(m, m->cfg->audio_type);

	if(m->cfg->use_ssl)
	{
		m->ssl_run = 0;
		wtk_blockqueue_wake(&m->ssl_queue);
		wtk_thread_join(&m->ssl_t);
		qtk_mqform_mod_engine_stop(m, m->cfg->ssl_type);
	}

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

	if(m->cfg->use_resample)
	{
		wtk_resample_close(m->resample);
	}

#ifdef USE_RECORD_PLAYER
	if(m->cfg->use_record_player)
	{
		m->two_record_run = 0;
		wtk_thread_join(&m->two_record_t);
		if(m->two_rcd)
		{
			qtk_recorder_stop(m->two_rcd);
		}
		m->two_player_run = 0;
		wtk_blockqueue_wake(&m->two_play_queue);
		wtk_thread_join(&m->two_player_t);
	}
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

void qtk_mqform_mod_clean_queue2(qtk_mqform_mod_t *m, wtk_blockqueue_t *queue,int nx)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int len=queue->length;

	if(nx>0)
	{
		while(queue->length>nx)
		{
			qn= wtk_blockqueue_pop(queue, 0,NULL);
			if(!qn) {
				break;
			}
			msg_node = data_offset2(qn,qtk_msg_node_t,qn);
			qtk_msg_push_node(m->msg, msg_node);
		}
	}else
	{
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
}

void qtk_mqform_mod_clean_queue(qtk_mqform_mod_t *m, wtk_blockqueue_t *queue)
{
	qtk_mqform_mod_clean_queue2(m,queue,0);
}

#ifdef USE_FOR_DEV
int qtk_mqform_mod_uart_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
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
void qtk_mqform_mod_feed(qtk_mqform_mod_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node,*msg_node2;

#if 1 //def LOG_ORI_AUDIO
	msg_node = qtk_msg_pop_node(m->msg);
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
#endif

	if(m->cfg->use_ssl)
	{
		msg_node2 = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node2->buf, data, len);
		wtk_blockqueue_push(&m->ssl_queue, &msg_node2->qn);
	}

}
#endif
#ifndef OFFLINE_TEST
int qtk_mqform_mod_rcd_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
{
	qtk_mod_set_cpu(m, t, 0);
	wtk_strbuf_t *buf;
	qtk_msg_node_t *msg_node, *msg_node2;
	int mchannel=m->cfg->rcd.channel - m->cfg->rcd.nskip - m->cfg->spknum;
	int head_cnt=m->cfg->skip_head_tm * 32 * (m->cfg->rcd.channel - m->cfg->rcd.nskip);
	int skip_len=0;
	int count=0;
	double rtm=0.0;

	while(m->rcd_run){
		buf = qtk_recorder_read(m->rcd);
		if(buf->pos == 0){
			continue;
		}
		if((skip_len+buf->pos) < head_cnt)
		{
			skip_len+=buf->pos;
			continue;
		}

		if(m->cfg->mic_shift != 1.0 || m->cfg->spk_shift != 1.0)
		{
			qtk_data_change_vol2(buf->data, buf->pos, m->cfg->mic_shift, m->cfg->spk_shift, mchannel, m->cfg->spknum);
		}

		count++;
		if(count == 50 && m->cfg->use_log_wav==0)
		{
			qtk_mqform_is_log_audio(m);
			count=0;
		}

		if(m->use_bfio)
		{
			msg_node = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node->buf, buf->data, buf->pos);
			wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
		}else{
			msg_node = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node->buf, buf->data, buf->pos);
			msg_node->type=QTK_MQFORM_MOD_BF_vboxebf;
			wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
		}
		if(m->cfg->use_ssl)
		{
			msg_node2 = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node2->buf, buf->data, buf->pos);
			wtk_blockqueue_push(&m->ssl_queue, &msg_node2->qn);
		}

	}
	return 0;
}
#endif

int qtk_mqform_mod_qform_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
{
	qtk_mod_set_cpu(m, t, 3);
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int inlen;
	int outlen=m->cfg->rcd.sample_rate*m->cfg->rcd.buf_time*m->cfg->rcd.channel*2/1000;
	int channel=m->cfg->rcd.channel-m->cfg->rcd.nskip;
	char *outresample=(char *)wtk_malloc(outlen);
	double tm=0.0;
    
	while(m->qform_run){
		qn= wtk_blockqueue_pop(&m->bfio_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->bfio_queue.length > m->cfg->max_add_count)
		{
			// wtk_debug("record =============>>%d\n",m->bfio_queue.length);
			qtk_mqform_mod_clean_queue2(m, &(m->bfio_queue), 0);
		}

		if(m->cfg->use_in_resample)
		{
			memset(outresample, 0, msg_node->buf->pos);
			inlen=(msg_node->buf->pos >> 1)/(channel);
			outlen=inlen;

			if(m->in_resample)
			{
				speex_resampler_process_interleaved_int(m->in_resample,
											(spx_int16_t *)(msg_node->buf->data), (spx_uint32_t *)(&inlen), 
											(spx_int16_t *)(outresample), (spx_uint32_t *)(&outlen));
			}
		}

		if(m->cfg->use_log_wav || m->log_audio){
			if(m->mul)
			{
				if(m->in_resample && m->cfg->use_in_resample)
				{
					wtk_wavfile_write(m->mul, outresample, (outlen<<1)*(channel));
				}else{
					wtk_wavfile_write(m->mul, msg_node->buf->data, msg_node->buf->pos);
				}
			}
		}

#if 1
		if(m->in_resample && m->cfg->use_in_resample)
		{
			qtk_mqform_mod_engine_feed(m, m->cfg->audio_type, outresample, (outlen<<1)*(channel), 0);
		}else{
			qtk_mqform_mod_engine_feed(m, m->cfg->audio_type, msg_node->buf->data, msg_node->buf->pos, 0);
		}
#endif
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mqform_mod_engine_feed(m, m->cfg->audio_type, NULL, 0, 1);
	if(outresample)
	{
		wtk_free(outresample);
	}
	return 0;
}

int qtk_mqform_mod_ssl_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
{
	qtk_mod_set_cpu(m, t, 2);
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
    
	while(m->ssl_run){
		qn= wtk_blockqueue_pop(&m->ssl_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->ssl_queue.length > m->cfg->max_add_count)
		{
			// wtk_debug("record =============>>%d\n",m->ssl_queue.length);
			qtk_mqform_mod_clean_queue2(m, &(m->ssl_queue), 0);
		}

		qtk_mqform_mod_engine_feed(m, m->cfg->ssl_type, msg_node->buf->data, msg_node->buf->pos, 0);

		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mqform_mod_engine_feed(m, m->cfg->ssl_type, NULL, 0, 1);
	return 0;
}

#ifndef OFFLINE_TEST
int qtk_mqform_mod_usbaudio_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	int zlen=100*m->cfg->usbaudio.channel*m->cfg->usbaudio.sample_rate/1000*2;
	char *zerodata = wtk_malloc(zlen);

	qtk_mod_set_cpu(m, t, 1);
	
	if(1)
	{
		pthread_attr_t thread_attr;
		struct sched_param thread_param;
		int thread_policy;

		pthread_attr_init(&(thread_attr));
		pthread_attr_getschedpolicy(&thread_attr, &thread_policy);
		pthread_attr_getschedparam(&thread_attr, &thread_param);

		wtk_debug("==================== usb audio scurrent priroytiy %d/%d\n",thread_policy,thread_param.sched_priority);
	}

	int ret = 0;
	ret = qtk_player_start(m->usbaudio, m->cfg->usbaudio.snd_name, m->cfg->usbaudio.sample_rate, m->cfg->usbaudio.channel, m->cfg->usbaudio.bytes_per_sample);
	if(ret!=0)
	{
		wtk_debug("player start error!\n");
	}

  	while(m->usbaudio_run){
		qn= wtk_blockqueue_pop(&m->usbaudio_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		// printf(">>>>>len = %d\n", msg_node->buf->pos);

		if(first == 1){
			memset(zerodata,0,zlen);
			ret = qtk_player_write2(m->usbaudio, zerodata, zlen, 1);
			first = 0;
		}
		if(m->cfg->echo_shift != 1.0f)
		{
			qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
		}

		switch(msg_node->type){
		case QTK_MQFORM_MOD_BF_vboxebf:
			qtk_player_write2(m->usbaudio, msg_node->buf->data, msg_node->buf->pos, 1);
			break;
		default:
			break;
		}
		qtk_msg_push_node(m->msg, msg_node);
	}
	
	wtk_free(zerodata);
	if(first==0)
	{
		qtk_player_stop(m->usbaudio);
	}
  return 0;
}

int qtk_mqform_mod_lineout_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
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
	if(first == 0)
	{
		qtk_player2_stop(m->lineout);
	}
	return 0;
}
#endif

#ifdef USE_RECORD_PLAYER
int qtk_mqform_mod_two_player_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	
	qtk_mod_set_cpu(m, t, 1);
	char *buf = wtk_malloc(48000);

  	while(m->two_player_run){
		qn= wtk_blockqueue_pop(&m->two_play_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		// printf(">>>>>len = %d\n", msg_node->buf->pos);

		if(first == 1){
			int ret = 0;
			ret = qtk_player_start(m->two_ply, m->cfg->two_ply.snd_name, m->cfg->two_ply.sample_rate, m->cfg->two_ply.channel, m->cfg->two_ply.bytes_per_sample);
			if(ret!=0)
			{
				wtk_debug("player start error!\n");
			}

			memset(buf,0,48000);
			ret = qtk_player_write2(m->two_ply, buf, 48000, m->cfg->two_ply.channel);
			first = 0;
		}
		// if(m->cfg->echo_shift != 1.0f)
		// {
		// 	qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
		// }

		qtk_player_write2(m->two_ply, msg_node->buf->data, msg_node->buf->pos, m->cfg->two_ply.channel);

		qtk_msg_push_node(m->msg, msg_node);
	}
	
	wtk_free(buf);
	qtk_player_stop(m->two_ply);
  	return 0;
}

int qtk_mqform_mod_two_record_entry(qtk_mqform_mod_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *buf;
	qtk_msg_node_t *msg_node;
	static int count = 0;

	qtk_mod_set_cpu(m, t, 0);

	while(m->two_record_run){
		buf = qtk_recorder_read(m->two_rcd);
		if(buf->pos == 0){
			continue;
		}

		count++;
		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, buf->data, buf->pos);
		wtk_blockqueue_push(&m->two_play_queue, &msg_node->qn);
		if(count == 50){
			qtk_mqform_is_log_audio(m);
			count = 0;
		}
		// if(m->mul){
		// 	wtk_wavfile_write(m->mul, buf->data, buf->pos);
		// }

	}
	return 0;
}
#endif

void qtk_mqform_mod_on_resample(qtk_mqform_mod_t *m, char *data, int len)
{
    qtk_msg_node_t *msg_node, *msg_node2;
	char *ss;
	int nx=0;

#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = QTK_MQFORM_MOD_BF_vboxebf;
		if(m->cfg->usbaudio.channel > 1)
		{
			nx=0;
			ss=data;
			int i;
			while(nx < len)
			{
				for(i=0;i<m->cfg->usbaudio.channel;++i)
				{
					wtk_strbuf_push(msg_node->buf, ss + nx, 2);
				}
				nx+=2;
			}
		}else{
			wtk_strbuf_push(msg_node->buf, data, len);
		}
		wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);

		if(m->usbaudio_queue.length > m->cfg->max_output_length)
		{
			qtk_mqform_mod_clean_queue(m, &(m->usbaudio_queue));
		}
	}
#endif

	if(m->notify)
	{
		qtk_var_t var;
		var.type = QTK_SPEECH_DATA_PCM;
		var.v.str.len = len;
		var.v.str.data = data;
		m->notify(m->ths, &var);
	}

	if(m->cfg->use_log_wav || m->log_audio)
	{
		if(m->single){
			wtk_wavfile_write(m->single, data, len);
		}
	}
}

void qtk_mqform_mod_engine_on_data(qtk_mqform_mod_t *m, qtk_var_t *var)
{
	switch (var->type)
	{
	case QTK_AUDIO_ENERGY:
	case QTK_AEC_DIRECTION:
		if(m->notify)
		{
			m->notify(m->ths, var);
		}
		break;
	default:
		break;
	}
}

void qtk_mqform_mod_engine_on_ssl(qtk_mqform_mod_t *m, qtk_var_t *var)
{
    qtk_msg_node_t *msg_node, *msg_node2;
	char *ss;
	char *data;
	int len;
	int nx=0;

	switch (var->type)
	{
	case QTK_SPEECH_DATA_PCM:
		break;
	case QTK_AEC_DIRECTION:
		wtk_debug("=================>>>>>>>>>>>>>>>>nbest=%d nspecsum=%f theta=%d\n", var->v.ii.nbest, var->v.ii.nspecsum, var->v.ii.theta);
		break;
	default:
		break;
	}
}

void qtk_mqform_mod_engine_on_aec_data(qtk_mqform_mod_t *m, qtk_var_t *var)
{
    qtk_msg_node_t *msg_node, *msg_node2;
	char *ss;
	char *data;
	int len;
	int nx=0;

	switch (var->type)
	{
	case QTK_SPEECH_DATA_PCM:
		data = var->v.str.data;
		len = var->v.str.len;
		if(m->cfg->use_resample)
		{
			wtk_resample_feed(m->resample, data, len, 0);
		}else{
	#ifndef OFFLINE_TEST
			if(m->cfg->use_usbaudio){
				msg_node = qtk_msg_pop_node(m->msg);
				msg_node->type = QTK_MQFORM_MOD_BF_vboxebf;
				if(m->cfg->usbaudio.use_uac && (m->cfg->usbaudio.channel > 1))
				{
					nx=0;
					ss=data;
					int i;
					while(nx < len)
					{
						for(i=0;i<m->cfg->usbaudio.channel;++i)
						{
							wtk_strbuf_push(msg_node->buf, ss + nx, 2);
						}
						nx+=2;
					}
				}else{
					wtk_strbuf_push(msg_node->buf, data, len);
				}
				wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
				if(m->usbaudio_queue.length > m->cfg->max_output_length)
				{
					qtk_mqform_mod_clean_queue(m, &(m->usbaudio_queue));
				}
			}
	#endif
			if(m->notify)
			{
				m->notify(m->ths, var);
			}
			if(m->cfg->use_log_wav || m->log_audio)
			{
				if(m->single){
					wtk_wavfile_write(m->single, data, len);
				}
			}
		}
	#ifndef OFFLINE_TEST
		if(m->cfg->use_lineout){
			msg_node2 = qtk_msg_pop_node(m->msg);
			if(m->cfg->lineout.channel > 1)
			{
				nx=0;
				ss=data;
				int i;
				while(nx < len)
				{
					for(i=0;i<m->cfg->lineout.channel;++i)
					{
						wtk_strbuf_push(msg_node2->buf, ss + nx, 2);
					}
					nx+=2;
				}
			}else{
				wtk_strbuf_push(msg_node2->buf, data, len);
			}
			wtk_blockqueue_push(&m->lineout_queue, &msg_node2->qn);
			if(m->lineout_queue.length > m->cfg->max_output_length)
			{
				qtk_mqform_mod_clean_queue(m, &(m->lineout_queue));
			}
		}
	#endif
		break;
	case QTK_AEC_DIRECTION:
		wtk_debug("=================>>>>>>>>>>>>>>>>nbest=%d nspecsum=%f theta=%d\n", var->v.ii.nbest, var->v.ii.nspecsum, var->v.ii.theta);
		break;
	default:
		break;
	}
}

void qtk_mqform_mod_on_vboxebf(qtk_mqform_mod_t *m, char *data, int len)
{
    qtk_msg_node_t *msg_node, *msg_node2;
	char *ss;
	int nx=0;

	if(m->cfg->use_resample)
	{
		wtk_resample_feed(m->resample, data, len, 0);
	}else{
	#ifndef OFFLINE_TEST
		if(m->cfg->use_usbaudio){
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = QTK_MQFORM_MOD_BF_vboxebf;
			if(m->cfg->usbaudio.use_uac && (m->cfg->usbaudio.channel > 1))
			{
				nx=0;
				ss=data;
				int i;
				while(nx < len)
				{
					for(i=0;i<m->cfg->usbaudio.channel;++i)
					{
						wtk_strbuf_push(msg_node->buf, ss + nx, 2);
					}
					nx+=2;
				}
			}else{
				wtk_strbuf_push(msg_node->buf, data, len);
			}
			wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
			if(m->usbaudio_queue.length > m->cfg->max_output_length)
			{
				qtk_mqform_mod_clean_queue(m, &(m->usbaudio_queue));
			}
		}
		if(m->cfg->use_lineout){
			msg_node2 = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node2->buf, data, len);
			wtk_blockqueue_push(&m->lineout_queue, &msg_node2->qn);
			if(m->lineout_queue.length > m->cfg->max_output_length)
			{
				qtk_mqform_mod_clean_queue(m, &(m->lineout_queue));
			}
		}
	#endif

		if(m->notify)
		{
			qtk_var_t var;
			var.type = QTK_SPEECH_DATA_PCM;
			var.v.str.len = len;
			var.v.str.data = data;
			m->notify(m->ths, &var);
		}

		if(m->cfg->use_log_wav || m->log_audio)
		{
			if(m->single){
				wtk_wavfile_write(m->single, data, len);
			}
		}
	}
}

int qtk_mqform_mod_engine_new(qtk_mqform_mod_t *m, int type)
{
	int ret;
	switch(type){
	case QTK_MQFORM_MOD_BF_vboxebf:
#ifdef USE_VBOXEBF
		printf("MOD_BF_vboxebf resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		m->vboxebf_cfg = qtk_vboxebf_cfg_new(m->cfg->vboxebf_fn.data);
		if(!m->vboxebf_cfg){ret = -1;goto end;}
		m->vboxebf = qtk_vboxebf_new(m->vboxebf_cfg);
		if(!m->vboxebf){ret = -1;goto end;}
		qtk_vboxebf_set_notify(m->vboxebf, m, (qtk_vboxebf_notify_f)qtk_mqform_mod_on_vboxebf);
		qtk_vboxebf_set_notify2(m->vboxebf, m, (qtk_engine_notify_f)qtk_mqform_mod_engine_on_data);
#endif
		break;
	case QTK_MQFORM_MOD_BF_aec:
#ifdef USE_AEC
		printf("MOD_BF_AEC resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		m->aec_cfg = qtk_aec_cfg_new(m->cfg->vboxebf_fn.data);
		if(!m->aec_cfg){ret = -1;goto end;}
		m->aec = qtk_aec_new(m->aec_cfg);
		if(!m->aec){ret = -1;goto end;}
		qtk_aec_set_notify2(m->aec, m, (qtk_engine_notify_f)qtk_mqform_mod_engine_on_aec_data);
#endif
		break;
	case QTK_MQFORM_MOD_BF_SSL:
#ifdef USE_SSL
		printf("MOD_BF_SSL resname=%.*s\n",m->cfg->ssl_fn.len,m->cfg->ssl_fn.data);
		m->ssl_cfg = qtk_ssl_cfg_new(m->cfg->ssl_fn.data);
		if(!m->ssl_cfg){ret = -1;goto end;}
		m->ssl = qtk_ssl_new(m->ssl_cfg);
		if(!m->ssl){ret = -1;goto end;}
		qtk_ssl_set_notify2(m->ssl, m, (qtk_engine_notify_f)qtk_mqform_mod_engine_on_ssl);
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

void qtk_mqform_mod_engine_delete(qtk_mqform_mod_t *m, int type)
{
	switch(type){
	case QTK_MQFORM_MOD_BF_vboxebf:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_delete(m->vboxebf);
		}
		if(m->vboxebf_cfg){
			qtk_vboxebf_cfg_delete(m->vboxebf_cfg);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_aec:
#ifdef USE_AEC
		if(m->aec){
			qtk_aec_delete(m->aec);
		}
		if(m->aec_cfg){
			qtk_aec_cfg_delete(m->aec_cfg);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_SSL:
#ifdef USE_SSL
		if(m->ssl){
			qtk_ssl_delete(m->ssl);
		}
		if(m->ssl_cfg){
			qtk_ssl_cfg_delete(m->ssl_cfg);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mqform_mod_engine_start(qtk_mqform_mod_t *m, int type)
{
	switch(type){
	case QTK_MQFORM_MOD_BF_vboxebf:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_start(m->vboxebf);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_aec:
#ifdef USE_AEC
		if(m->aec){
			qtk_aec_start(m->aec);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_SSL:
#ifdef USE_SSL
		if(m->ssl){
			qtk_ssl_start(m->ssl);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mqform_mod_engine_stop(qtk_mqform_mod_t *m, int type)
{
	switch(type){
	case QTK_MQFORM_MOD_BF_vboxebf:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_reset(m->vboxebf);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_aec:
#ifdef USE_AEC
		if(m->aec){
			qtk_aec_reset(m->aec);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_SSL:
#ifdef USE_SSL
		if(m->ssl){
			qtk_ssl_reset(m->ssl);
		}
#endif
		break;
	default:
		break;
	}
}

int qtk_mqform_mod_engine_feed(qtk_mqform_mod_t *m, int type, char *data, int len, int is_end)
{
	switch(type){
	case QTK_MQFORM_MOD_BF_vboxebf:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_feed(m->vboxebf, data, len, is_end);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_aec:
#ifdef USE_AEC
		if(m->aec){
			qtk_aec_feed(m->aec, data, len, is_end);
		}
#endif
		break;
	case QTK_MQFORM_MOD_BF_SSL:
#ifdef USE_SSL
		if(m->ssl){
			qtk_ssl_feed(m->ssl, data, len, is_end);
		}
#endif
		break;
	default:
		break;
	}
	return 0;
}

static void qtk_log_wav_file_new(qtk_mqform_mod_t *m)
{
	int channel = m->cfg->rcd.channel-m->cfg->rcd.nskip;
	int bytes_per_sample = m->cfg->rcd.bytes_per_sample;
	int sample_rate = 16000;// m->cfg->rcd.sample_rate;

	if(m->cfg->cache_path.len <= 0){
		return;
	}
	m->mul = wtk_wavfile_new(sample_rate); 
	m->mul->max_pend = 0;
	wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);

	wtk_wavfile_open(m->mul, m->mul_path->data);
	m->single = wtk_wavfile_new(m->cfg->usbaudio.sample_rate); 
	m->single->max_pend = 0;
	channel = 1;
	wtk_wavfile_set_channel2(m->single,channel,bytes_per_sample);
	wtk_wavfile_open(m->single, m->single_path->data);
}
static void qtk_log_wav_file_delete(qtk_mqform_mod_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);		
	wtk_wavfile_close(m->single);
	wtk_wavfile_delete(m->single);
	m->mul = NULL;
	m->single = NULL;
}

void qtk_mqform_is_log_audio(qtk_mqform_mod_t *m)
{
	if(m->cfg->cache_path.len <= 0){
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

void qtk_mod_set_cpu(qtk_mqform_mod_t *m, wtk_thread_t *thread, int cpunum)
{
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
	ret = pthread_setaffinity_np(thread->handler, sizeof(cpu_set_t), &cpuset);
    if (ret != 0)
	{
        wtk_log_err(m->log, "pthread_setaffinity_np error %d!\n",ret);
	}
	ret = pthread_getaffinity_np(thread->handler, sizeof(cpu_set_t), &cpuset);
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
