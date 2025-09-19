#include "qtk_mod_tt2t.h"
#include "sdk/qtk_api.h"
#include <time.h>
#include <pthread.h>
#ifdef USE_KEROS
#include "sdk/codec/keros/qtk_keros.h"
#endif
// #define ONLAY_PLAY
// #define ONLAY_RECORD

#ifndef OFFLINE_TEST
int qtk_mod_tt2t_rcd_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t);
int qtk_mod_tt2t_lineout_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t);
#endif
int qtk_mod_tt2t_uart_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t);
int qtk_mod_tt2t_uart_read_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t);
int qtk_mod_tt2t_tt2t_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t);
int qtk_mod_tt2t_dett2t_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t);
int qtk_mod_tt2t_savepcm_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t);
static void qtk_log_wav_file_new(qtk_mod_tt2t_t *m);
static void qtk_log_wav_file_delete(qtk_mod_tt2t_t *m);
void qtk_mod_tt2t_is_log_audio(qtk_mod_tt2t_t *m);
void qtk_mod_tt2t_set_cpu(qtk_mod_tt2t_t *m, wtk_thread_t *thread, int cpunum);

int qtk_mod_tt2t_engine_new(qtk_mod_tt2t_t *m, int type);
void qtk_mod_tt2t_engine_delete(qtk_mod_tt2t_t *m, int type);
void qtk_mod_tt2t_engine_start(qtk_mod_tt2t_t *m, int type);
void qtk_mod_tt2t_engine_stop(qtk_mod_tt2t_t *m, int type);
int qtk_mod_tt2t_engine_feed(qtk_mod_tt2t_t *m, int type, char *data, int len);

int pushlen=5*16*1000;
int mul_cnt=0;

int qtk_mod_tt2t_msg_bytes(qtk_msg_node_t *node)
{
	int bytes=0;
	bytes+=wtk_strbuf_bytes(node->buf);
	return bytes;
}

int qtk_mod_tt2t_bytes(qtk_mod_tt2t_t *m)
{
	int bytes=0;
	int tmp=0;
	bytes+=wtk_strbuf_bytes(m->check_path_buf);
	bytes+=wtk_strbuf_bytes(m->mul_path);
	// if(m->cfg->audio_type == qtk_mod_tt2t_BF_tt2t)
	// {
	// 	bytes+=qtk_signal_tt2t_bytes(m->tt2t);
	// }
#ifndef OFFLINE_TEST
	if(m->rcd)
	{
		bytes+=wtk_strbuf_bytes(m->rcd->buf);
	}
#endif
	tmp=wtk_hoard_bytes((wtk_hoard_t *)(&(m->msg->msg_hoard)),(wtk_hoard_bytes_f)qtk_mod_tt2t_msg_bytes);
	bytes+=tmp;
	bytes+=sizeof(qtk_mod_tt2t_t);
	wtk_log_log(m->log, "all=%d bytes=%d hoard_length=%d/%d\n",bytes,tmp,m->msg->msg_hoard.use_length,m->msg->msg_hoard.cur_free);
	return bytes;
}

qtk_mod_tt2t_t *qtk_mod_tt2t_new(qtk_mod_tt2t_cfg_t *cfg)
{
	qtk_mod_tt2t_t *m;
	char tmp[64];
	int ret;

#ifdef USE_KEROS
	ret = qtk_check_keros(NULL);
	if(ret!=0)
	{
		wtk_debug("mod keros auth failed!");
		return NULL;
	}
#endif

	m = (qtk_mod_tt2t_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;

	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
	pushlen = m->cfg->record_time * 16;

	if(m->cfg->cache_path.len > 0){
		double tt;
		tt=time_get_ms();

		wtk_mkdir(m->cfg->cache_path.data);
		wtk_strbuf_push_f(m->check_path_buf, "%.*s/start_log_audio", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->check_path_buf, 0);
		wtk_strbuf_push_f(m->mul_path, "%.*s/mul.%d.wav", m->cfg->cache_path.len, m->cfg->cache_path.data, mul_cnt);
		wtk_strbuf_push_c(m->mul_path, 0);

		wtk_blockqueue_init(&m->savepcm_queue);
		wtk_thread_init(&m->savepcm_t,(thread_route_handler)qtk_mod_tt2t_savepcm_entry, m);
	}

	if(m->cfg->use_log && m->cfg->cache_path.len > 0){
		snprintf(tmp, 64, "%.*s/qvoice.log", m->cfg->cache_path.len, m->cfg->cache_path.data);
		m->log = wtk_log_new(tmp);
	}
	if(m->cfg->use_log_wav)
	{
		qtk_log_wav_file_new(m);
	}

	char buf[256];
	wtk_get_build_timestamp(buf);
	printf("BUILD AT %s\n",buf);
	wtk_log_log(m->log, "BUILD AT %s",buf);

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mod_tt2t_lineout_entry, m);
		m->lineout = qtk_play_new(&cfg->lineout);
	}
#endif
#endif

#ifndef OFFLINE_TEST
#ifndef ONLAY_PLAY
	m->rcd =qtk_record_new(&cfg->rcd);
//	if(!m->rcd){
//		wtk_log_err0(m->log, "record fiald!");
//		ret = -1;
//    	goto end;
//	}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mod_tt2t_rcd_entry, m);
#endif
#endif

	m->msg = qtk_msg_new();
	wtk_thread_init(&m->tt2t_t,(thread_route_handler)qtk_mod_tt2t_tt2t_entry, m);
	wtk_blockqueue_init(&m->tt2t_queue);

	wtk_thread_init(&m->dett2t_t,(thread_route_handler)qtk_mod_tt2t_dett2t_entry, m);
	wtk_blockqueue_init(&m->dett2t_queue);

	ret = qtk_mod_tt2t_engine_new(m, 1);
	if(ret!=0)
	{
		wtk_log_err(m->log, "audio engine new fiald! type=%d", m->cfg->audio_type);
		goto end;
	}
	ret = qtk_mod_tt2t_engine_new(m, 2);
	if(ret!=0)
	{
		wtk_log_err(m->log, "audio engine new fiald! type=%d", 6);
		goto end;
	}

	m->dett2twav = wtk_wavfile_new(8000);
	wtk_sem_init(&m->rcdstart_sem, 0);

	if(m->cfg->use_uart)
	{
		wtk_blockqueue_init(&m->uart_queue);
		wtk_thread_init(&m->uart_t, (thread_route_handler)qtk_mod_tt2t_uart_entry, m);
		wtk_thread_init(&m->uart_read_t, (thread_route_handler)qtk_mod_tt2t_uart_read_entry, m);
		m->uart_cfg = wtk_main_cfg_new_type(qtk_uart_cfg,m->cfg->uart_fn.data);
		m->uart = qtk_uart_new((qtk_uart_cfg_t *)(m->uart_cfg->cfg));
	}

	wtk_log_log0(m->log, "MODULE NEW OK!");
	wtk_debug("MOD NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mod_tt2t_delete(m);
      m = NULL;
  	}
	return m;
}

void qtk_mod_tt2t_delete(qtk_mod_tt2t_t *m)
{
	wtk_blockqueue_clean(&m->tt2t_queue);
	wtk_blockqueue_clean(&m->dett2t_queue);
	wtk_blockqueue_clean(&m->savepcm_queue);

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_lineout){
		wtk_blockqueue_clean(&m->lineout_queue);
		if(m->lineout){
			qtk_play_delete(m->lineout);
		}
	}
#endif
#endif

#ifndef OFFLINE_TEST
	if(m->rcd){
			qtk_record_delete(m->rcd);
	}
#endif
	if(m->msg){
			qtk_msg_delete(m->msg);
	}

	qtk_mod_tt2t_engine_delete(m, 1);
	qtk_mod_tt2t_engine_delete(m, 2);

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
	if(m->cfg->use_log_wav)
	{
		qtk_log_wav_file_delete(m);
	}

	if(m->dett2twav)
	{
		wtk_wavfile_delete(m->dett2twav);
	}

	wtk_sem_clean(&m->rcdstart_sem);

	if(m->check_path_buf){
		wtk_strbuf_delete(m->check_path_buf);
	}
	if(m->mul_path){
		wtk_strbuf_delete(m->mul_path);
	}
	if(m->log){
		wtk_log_delete(m->log);
	}
    wtk_free(m);
}

void qtk_mod_tt2t_start(qtk_mod_tt2t_t *m)
{
	m->is_rcdstart=0;
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_lineout){
		m->lineout_run = 1;
		wtk_thread_start(&m->lineout_t);
	}
#endif
#endif
	qtk_mod_tt2t_engine_start(m, 1);
	m->tt2t_run = 1;
	wtk_thread_start(&m->tt2t_t);

	qtk_mod_tt2t_engine_start(m, 2);
	m->dett2t_run = 1;
	wtk_thread_start(&m->dett2t_t);

	// if(m->cfg->use_log_wav)
	{
		m->savepcm_run = 1;
		wtk_thread_start(&m->savepcm_t);
	}

	if(m->cfg->use_uart)
	{
		m->uart_read_run = 1;
		wtk_thread_start(&m->uart_read_t);
		m->uart_run = 1;
		wtk_thread_start(&m->uart_t);
	}

#ifndef OFFLINE_TEST
#ifndef ONLAY_PLAY
    m->rcd_run = 1;
    wtk_thread_start(&m->rcd_t);
#endif
#endif
	wtk_debug("MOD START OK!!!\n");
}

void qtk_mod_tt2t_record_start(qtk_mod_tt2t_t *m)
{
	if(m->mul == NULL){
		wtk_strbuf_reset(m->mul_path);
		mul_cnt++;
		wtk_strbuf_push_f(m->mul_path, "%.*s/mul.%d.wav", m->cfg->cache_path.len, m->cfg->cache_path.data, mul_cnt);
		wtk_strbuf_push_c(m->mul_path, 0);
		qtk_log_wav_file_new(m);
	}
	m->is_rcdstart = 1;
	usleep(m->cfg->record_time*1000);
	wtk_sem_acquire(&m->rcdstart_sem, -1);
}

void qtk_mod_tt2t_stop(qtk_mod_tt2t_t *m)
{
#ifndef OFFLINE_TEST
	m->rcd_run = 0;
	wtk_thread_join(&m->rcd_t);
#endif

	if(m->cfg->use_uart)
	{
		if(m->uart_read_run)
		{
			m->uart_read_run = 0;
			wtk_thread_join(&m->uart_read_t);
		}
		if(m->uart_run)
		{
			m->uart_run = 0;
			wtk_blockqueue_wake(&m->uart_queue);
			wtk_thread_join(&m->uart_t);
		}
	}

	m->tt2t_run = 0;
	wtk_blockqueue_wake(&m->tt2t_queue);
	wtk_thread_join(&m->tt2t_t);
	qtk_mod_tt2t_engine_stop(m, 1);

	m->dett2t_run = 0;
	wtk_blockqueue_wake(&m->dett2t_queue);
	wtk_thread_join(&m->dett2t_t);
	qtk_mod_tt2t_engine_stop(m, 2);

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_lineout){
		m->lineout_run = 0;
		wtk_blockqueue_wake(&m->lineout_queue);
		wtk_thread_join(&m->lineout_t);
	}
#endif
#endif

	if(m->savepcm_run)
	{
		m->savepcm_run = 0;
		wtk_blockqueue_wake(&m->savepcm_queue);
		wtk_thread_join(&m->savepcm_t);
	}
}

void qtk_mod_tt2t_clean_queue2(qtk_mod_tt2t_t *m, wtk_blockqueue_t *queue,int nx)
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

void qtk_mod_tt2t_clean_queue(qtk_mod_tt2t_t *m, wtk_blockqueue_t *queue)
{
	qtk_mod_tt2t_clean_queue2(m,queue,0);
}

int qtk_mod_tt2t_grep_start2(char *data, int len, int *alen)
{
	int pos=0;
	char head[4]={0xEB,0x90,0xAA,0xAA};
	while(pos < len)
	{
		if(memcmp(data+pos, head, 4) == 0)
		{
			short dlen;
			memcpy(&dlen, data+pos+4, 2);
			*alen=pos;
			return dlen;
		}
		pos++;
	}
	*alen=pos;
	return -1;
}

int qtk_mod_tt2t_grep_end(char *data, int len, int *alen)
{
	int pos=0;
	char end[4]={0xED,0x03,0xBF,0xBF};
	while(pos < len)
	{
		if(memcmp(data+pos, end, 4) == 0)
		{
			*alen=pos-2;
			return pos-2;
		}
		pos++;
	}
	*alen=pos;
	return -1;
}

static uint8_t crc8(const uint8_t *data, int len)
{
    unsigned crc = 0;
    int i, j;
    for (j = len; j; j--, data++)
    {
        crc ^= (*data << 8);
        for (i = 8; i; i--)
        {
            if (crc & 0x8000)
                crc ^= (0x1070 << 3);
            crc <<= 1;
        }
    }
    return (uint8_t)(crc >> 8);
}

int qtk_mod_tt2t_uart_read_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	char *recv_buf=(char *)wtk_malloc(1024);
	qtk_mode_bf_type type=qtk_mod_tt2t_UART_RECV_START;
	wtk_strbuf_t *outbuf=wtk_strbuf_new(1024,0);
	wtk_strbuf_t *databuf=wtk_strbuf_new(1024,0);
	int ret,pos,alen,datalen=0;
	
	while(m->uart_read_run){
		memset(recv_buf, 0, sizeof(recv_buf));
		ret = qtk_uart_read(m->uart, recv_buf, 12);
		if(ret > 0)
		{
			wtk_strbuf_push(outbuf, recv_buf, ret);
			switch (type)
			{
			case qtk_mod_tt2t_UART_RECV_START:
				pos = qtk_mod_tt2t_grep_start2(outbuf->data, outbuf->pos, &alen);
				if(pos >= 0)
				{
					datalen = pos;
					if(outbuf->pos >= pos+alen+6+6)
					{
						wtk_strbuf_push(databuf, outbuf->data+alen+6, datalen);
						wtk_strbuf_pop(outbuf, NULL, alen+datalen+6+6);
						datalen = 0;
						type=qtk_mod_tt2t_UART_RECV_END;
					}else{
						wtk_strbuf_push(databuf, outbuf->data+alen+6, outbuf->pos-alen-6);
						datalen = datalen - (outbuf->pos-alen-6);
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos);
						type=qtk_mod_tt2t_UART_RECV_DATA;
					}
				}else{
					wtk_strbuf_pop(outbuf, NULL, alen-4);
				}
				break;
			case qtk_mod_tt2t_UART_RECV_DATA:
				pos = qtk_mod_tt2t_grep_end(outbuf->data, outbuf->pos, &alen);
				if(pos >= 0)
				{
					// wtk_debug("end=[");
					// int i;
					// for(i=0; i<outbuf->pos; ++i)
					// {
					// 	printf("%02X ", outbuf->data[i]);
					// }
					// printf("]\n");
					// wtk_debug("==================>>>>>>>>>>>>pos=%d outbuf->pos=%d databuf->pos=%d datalen=%d alen=%d\n", pos, outbuf->pos, databuf->pos, datalen, alen);
					if(alen > 2 && datalen > 0)
					{
						wtk_strbuf_push(databuf, outbuf->data, alen-2);
					}
					wtk_strbuf_pop(outbuf, NULL, alen+4);
					type=qtk_mod_tt2t_UART_RECV_END;
				}else{
					if(outbuf->pos >= datalen)
					{
						wtk_strbuf_push(databuf, outbuf->data, datalen);
						wtk_strbuf_pop(outbuf, NULL, datalen);
						datalen = 0;
					}else{
						wtk_strbuf_push(databuf, outbuf->data, outbuf->pos);
						datalen = datalen - outbuf->pos;
						wtk_strbuf_pop(outbuf, NULL, outbuf->pos);
					}
					// wtk_debug("==================>>>>>>>>>>>>databuf->pos=%d outbuf->pos=%d datalen=%d\n",databuf->pos ,outbuf->pos, datalen);
				}
				break;
			default:
				break;
			}
			if(type == qtk_mod_tt2t_UART_RECV_END)
			{
				wtk_debug("recv data len=%d\n", databuf->pos);
				//TODO push data to dett2t queue
				msg_node = qtk_msg_pop_node(m->msg);
				wtk_strbuf_push(msg_node->buf, databuf->data, databuf->pos);
				wtk_blockqueue_push(&m->dett2t_queue, &msg_node->qn);
				wtk_strbuf_reset(databuf);
				type=qtk_mod_tt2t_UART_RECV_START;
			}
		}
	}
	if(recv_buf)
	{
		wtk_free(recv_buf);
	}
	if(outbuf)
	{
		wtk_strbuf_delete(outbuf);
	}
	if(databuf)
	{
		wtk_strbuf_delete(databuf);
	}
}

int qtk_mod_tt2t_uart_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	char head[4]={0xEB,0x90,0xAA,0xAA};
	char end[4]={0xED,0x03,0xBF,0xBF};
	wtk_strbuf_t *sendbuf=wtk_strbuf_new(1024,0);

	while(m->uart_run){
		qn= wtk_blockqueue_pop(&m->uart_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		wtk_strbuf_reset(sendbuf);
		wtk_strbuf_push(sendbuf, head, 4);
		short slen=msg_node->buf->pos;
		wtk_strbuf_push(sendbuf, (char *)&slen, 2);
		wtk_strbuf_push(sendbuf, msg_node->buf->data, msg_node->buf->pos);
		short cc = crc8(msg_node->buf->data, msg_node->buf->pos);
		wtk_strbuf_push(sendbuf, (char *)&cc, 2);
		wtk_strbuf_push(sendbuf, end, 4);

		// wtk_debug("===============>>>>>>uart write %d\n", sendbuf->pos);
		qtk_uart_write(m->uart, sendbuf->data, sendbuf->pos);

		qtk_msg_push_node(m->msg, msg_node);
	}

	if(sendbuf)
	{
		wtk_strbuf_delete(sendbuf);
	}
	return 0;
}

#ifdef OFFLINE_TEST
void qtk_mod_tt2t_feed(qtk_mod_tt2t_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node;

#if 0 //def LOG_ORI_AUDIO
	msg_node = qtk_msg_pop_node(m->msg);
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->tt2t_queue, &msg_node->qn);

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
	// wtk_blockqueue_push(&m->tt2t_queue, &msg_node->qn);
	wtk_blockqueue_push(&m->tt2t_queue, &msg_node->qn);
#endif

}
#endif
#ifndef OFFLINE_TEST
int qtk_mod_tt2t_rcd_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node, *msg_node2;
	int mchannel=m->cfg->rcd.channel - m->cfg->rcd.nskip;
	int head_cnt=m->cfg->skip_head_tm * 32 * (m->cfg->rcd.channel - m->cfg->rcd.nskip);
	int skip_len=0;
	int count=0;
	int rcd_len=0;

	qtk_mod_tt2t_set_cpu(m, t, 1);

	while(m->rcd_run){
		rbuf = qtk_record_read(m->rcd);
		if(rbuf->pos <=0)
		{
			if(m->cfg->debug)
			{
				wtk_debug("read bytes=%d\n",rbuf->pos);
				wtk_log_log(m->log, "record read error %d",rbuf->pos);
			}
			continue;
		}
		if((skip_len+rbuf->pos) < head_cnt)
		{
			skip_len+=rbuf->pos;
			wtk_strbuf_reset(rbuf);
			continue;
		}

		if(m->cfg->mic_shift != 1.0)
		{
			qtk_data_change_vol(rbuf->data, rbuf->pos, m->cfg->mic_shift);
		}

		if(m->is_rcdstart)
		{
			rcd_len+=rbuf->pos;
			msg_node = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
			//wtk_debug("queue length = %d\n", m->tt2t_queue.length);
			wtk_blockqueue_push(&m->tt2t_queue, &msg_node->qn);
			if(rcd_len >= pushlen)
			{
				printf("end recording.\n");
				m->is_rcdstart = 0;
				rcd_len = 0;
			}
		}
	}
	return 0;
}
#endif

int qtk_mod_dett2t_grep_str(char *str, char *data, int len)
{
	int pos=0;
	char *head=str;

	while(pos < len)
	{
		if(memcmp(data+pos, head, strlen(head)) == 0)
		{
			return pos;
		}
		pos++;
	}
	return -1;
}

int qtk_mod_tt2t_dett2t_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *dett2tbuf=wtk_strbuf_new(1024,0);
	wtk_strbuf_t *tmpbuf=wtk_strbuf_new(1024,0);
	int ret=-1,is_start=1,is_end=0;
	wtk_strbuf_reset(dett2tbuf);
	wtk_strbuf_reset(tmpbuf);

	char *headstr="start";
	char *endstr="end";
	int head_len=strlen(headstr);
	int end_len=strlen(endstr);

	qtk_mod_tt2t_set_cpu(m, t, 2);

	while(m->dett2t_run){
		qn= wtk_blockqueue_pop(&m->dett2t_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug)
		{
			if(m->dett2t_queue.length > 2)
			{
				wtk_debug("tt2t =============>>%d\n",m->dett2t_queue.length);
				wtk_log_log(m->log, "tt2t =============>>%d",m->dett2t_queue.length);
			}
		}
		wtk_strbuf_push(tmpbuf, msg_node->buf->data, msg_node->buf->pos);
		if(is_start)
		{
			ret = qtk_mod_dett2t_grep_str(headstr, tmpbuf->data, tmpbuf->pos);
			if(ret >= 0)
			{
				int start_pos=ret;
				ret = qtk_mod_dett2t_grep_str(endstr, tmpbuf->data, tmpbuf->pos);
				// wtk_debug("==========>>>>>>>>>>>>>>>>is start=%d is_end=%d ret=%d start_pos=%d\n", is_start, is_end, ret, start_pos);
				if(ret >= 0 && ret > start_pos)
				{
					wtk_strbuf_push(dett2tbuf, tmpbuf->data+start_pos+head_len, ret-head_len-start_pos);
					wtk_strbuf_pop(tmpbuf, NULL, ret+end_len);
				}else{
					wtk_strbuf_push(dett2tbuf, tmpbuf->data+start_pos+head_len, tmpbuf->pos-start_pos-head_len-end_len);
					wtk_strbuf_pop(tmpbuf, NULL, tmpbuf->pos-end_len);
				}
				is_start = 0;
				is_end=0;
			}else{
				wtk_strbuf_pop(tmpbuf, NULL, tmpbuf->pos - head_len);
			}
		}else{
			ret = qtk_mod_dett2t_grep_str(endstr, tmpbuf->data, tmpbuf->pos);
			if(ret >= 0)
			{
				if(ret > 0)
				{
					wtk_strbuf_push(dett2tbuf, tmpbuf->data, ret);
				}
				wtk_strbuf_pop(tmpbuf, NULL, ret+end_len);
				is_end=1;
				is_start = 1;
			}else{
				wtk_strbuf_push(dett2tbuf, tmpbuf->data, tmpbuf->pos - end_len);
				wtk_strbuf_pop(tmpbuf, NULL, tmpbuf->pos - end_len);
			}
		}
		if(is_end)
		{
			//TODO push data to dett2t engine
			int64_t codec_len[4]={0};
			memcpy(codec_len, dett2tbuf->data, sizeof(int64_t)*4);
			// qtk_signal_dett2t_feed(m->dett2t, (int64_t *)(dett2tbuf->data+sizeof(int64_t)*4), codec_len);
			// qtk_signal_dett2t_get_result(m->dett2t);
			// qtk_signal_dett2t_reset(m->dett2t);
			wtk_strbuf_reset(dett2tbuf);
			is_end = 0;
		}

		qtk_msg_push_node(m->msg, msg_node);
	}
	if(dett2tbuf)
	{
		wtk_strbuf_delete(dett2tbuf);
	}
	if(tmpbuf)
	{
		wtk_strbuf_delete(tmpbuf);
	}
	return 0;
}

int qtk_mod_tt2t_tt2t_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	qtk_msg_node_t *msg_node3;
	wtk_queue_node_t *qn;
	char zdata[1024]={0};
	wtk_strbuf_t *tt2tbuf=wtk_strbuf_new(1024,0);
	int flen=0,i;
	int tt2t_len=0;

	qtk_mod_tt2t_set_cpu(m, t, 3);

	while(m->tt2t_run){
		qn= wtk_blockqueue_pop(&m->tt2t_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug)
		{
			if(m->tt2t_queue.length > 2)
			{
				wtk_debug("tt2t =============>>%d\n",m->tt2t_queue.length);
				wtk_log_log(m->log, "tt2t =============>>%d",m->tt2t_queue.length);
			}
		}
		flen+=msg_node->buf->pos;
		if(m->cfg->use_log_wav || m->log_audio)
		{
			msg_node3 = qtk_msg_pop_node(m->msg);
			msg_node3->type = qtk_mod_tt2t_BF_SAVE_SOURCE;
			wtk_strbuf_push(msg_node3->buf, msg_node->buf->data, msg_node->buf->pos);
			wtk_blockqueue_push(&m->savepcm_queue, &msg_node3->qn);
		}
		//TODO push data to tt2t engine
		// qtk_signal_tt2t_feed(m->tt2t, msg_node->buf->data, msg_node->buf->pos>>1);
		if(flen >= pushlen)
		{
			flen=0;
			int64_t *data;
			int64_t *data_len;
			int data_cnt;
			int64_t adatalen=0;
			//TODO get data from tt2t engine
			printf("============== tt2t start ======================\n");
			// data = qtk_signal_tt2t_get_result(m->tt2t, &data_len, &data_cnt);
			printf("============== tt2t end   ======================\n");
			// wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>>>>>>tt2t end datacnt=%d data_len=%d/%d/%d/%d\n",data_cnt,data_len[0],data_len[1],data_len[2],data_len[3]);
			wtk_strbuf_push(tt2tbuf, "start", strlen("start"));
			wtk_strbuf_push(tt2tbuf, (char *)data_len, sizeof(int64_t)*data_cnt);
			// int j;
			for(i=0;i<data_cnt;++i)
			{
				// printf("data[%d]={",i);
				// for(j=0;j<data_len[i];++j)
				// {
				// 	printf("%d ",data[adatalen+j]);
				// }
				// printf("}\n");
				adatalen+=data_len[i];
			}
			wtk_strbuf_push(tt2tbuf, (char *)data, adatalen*sizeof(int64_t));
			wtk_strbuf_push(tt2tbuf, "end", strlen("end"));

			file_write_buf("./tmp.txt", tt2tbuf->data+5, tt2tbuf->pos-8);

			// wtk_debug("tt2t len=%d\n",tt2tbuf->pos);
			i=tt2tbuf->pos%1024;
			if(i!=0)
			{
				wtk_strbuf_push(tt2tbuf, zdata, 1024-i);
			}
			// wtk_debug("tt2t len=%d\n",tt2tbuf->pos);
			tt2t_len=tt2tbuf->pos;
			char *ss=tt2tbuf->data;
			char *ee=ss+tt2tbuf->pos;
			int step=1024;
			while(ss<ee)
			{
				i = (ee-ss)>step?step:(ee-ss);
				msg_node2 = qtk_msg_pop_node(m->msg);
				wtk_strbuf_push(msg_node2->buf, ss, i);
				wtk_blockqueue_push(&m->uart_queue, &msg_node2->qn);
				ss+=i;
			}
			wtk_strbuf_reset(tt2tbuf);
			// qtk_signal_tt2t_reset(m->tt2t);
			qtk_log_wav_file_delete(m);

			printf("============= Player start ==========\n");
			usleep(m->cfg->record_time*1000);
			printf("============= Player end   ==========\n");
			wtk_sem_inc(&m->rcdstart_sem);
		}

		qtk_msg_push_node(m->msg, msg_node);
	}
	wtk_strbuf_delete(tt2tbuf);
	return 0;
}


#ifndef __ANDROID__
#ifndef OFFLINE_TEST
int qtk_mod_tt2t_lineout_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	char pcmd[1024]={0};

	qtk_mod_set_cpu(m, t, 2);
	if(first == 1){
		qtk_play_start(m->lineout);
		first = 0;
	}
    
	while(m->lineout_run){
		qn= wtk_blockqueue_pop(&m->lineout_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->lineout_queue.length > 10)
		{
			wtk_debug("lineout =============>>%d\n",m->lineout_queue.length);
			wtk_log_log(m->log, "lineout =============>>%d",m->lineout_queue.length);
		}

		// if(m->cfg->lineout_shift != 1.0)
		// {
		// 	qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->lineout_shift);
		// }
		if(msg_node->type == 2)
		{
			snprintf(pcmd, sizeof(pcmd), "aplay -Dhw:%s dett2t.wav", m->cfg->lineout.snd_name);
		}else{
			qtk_play_write(m->lineout, msg_node->buf->data, msg_node->buf->pos, 0);
		}
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_play_stop(m->lineout);
	return 0;
}
#endif
#endif

int qtk_mod_tt2t_savepcm_entry(qtk_mod_tt2t_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
#ifdef USE_R328
	m->slen=0;
#endif

	qtk_mod_tt2t_set_cpu(m, t, 0);
    
	while(m->savepcm_run){
		qn= wtk_blockqueue_pop(&m->savepcm_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->savepcm_queue.length > 10  && m->cfg->debug)
		{
			wtk_debug("savepcm =============>>%d\n",m->savepcm_queue.length);
			wtk_log_log(m->log, "savepcm ===============>>%d",m->savepcm_queue.length);
		}

		switch (msg_node->type)
		{
		case qtk_mod_tt2t_BF_SAVE_SOURCE:
			if(m->mul){
				wtk_wavfile_write(m->mul, msg_node->buf->data, msg_node->buf->pos);
			}
			break;
		default:
			break;
		}

		qtk_msg_push_node(m->msg, msg_node);
	}

	return 0;
}

void qtk_mod_tt2t_on_dett2t(qtk_mod_tt2t_t *m, short *data, int len)
{
	if(m->dett2twav)
	{
		wtk_wavfile_open(m->dett2twav, "dett2t.wav");
		wtk_wavfile_write(m->dett2twav, (char*)data, len<<1);
		wtk_wavfile_close(m->dett2twav);
	}

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_lineout){
		qtk_msg_node_t *msg_node;
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 2;
		// if(m->cfg->lineout.use_uac && (m->cfg->lineout.channel > 1))
		// {
		// 	char *ss;
		// 	int nx=0;
		// 	nx=0;
		// 	ss=data;
		// 	int i;
		// 	while(nx < len)
		// 	{
		// 		for(i=0;i<m->cfg->lineout.channel;++i)
		// 		{
		// 			wtk_strbuf_push(msg_node->buf, ss + nx, 2);
		// 		}
		// 		nx+=2;
		// 	}
		// }else{
		// 	wtk_strbuf_push(msg_node->buf, data, len);
		// }
		wtk_blockqueue_push(&m->lineout_queue, &msg_node->qn);
	}
#endif
#endif

}

int qtk_mod_tt2t_engine_new(qtk_mod_tt2t_t *m, int type)
{
	int ret;
	switch(type){
	case qtk_mod_tt2t_BF_tt2t:
		printf("MOD BF tt2t resname=%.*s\n",m->cfg->tt2t_fn.len,m->cfg->tt2t_fn.data);
		wtk_log_log(m->log, "MOD BF tt2t resname=%.*s",m->cfg->tt2t_fn.len,m->cfg->tt2t_fn.data);
		// if(m->cfg->use_bin)
		// {
		// 	m->tt2t_cfg = qtk_signal_tt2t_cfg_new_bin(m->cfg->tt2t_fn.data);
		// }else{
		// 	m->tt2t_cfg = qtk_signal_tt2t_cfg_new(m->cfg->tt2t_fn.data);
		// }
		// if(!m->tt2t_cfg){
		// 	ret = -1;
		// 	goto end;
		// }
		// m->tt2t = qtk_signal_tt2t_new(m->tt2t_cfg);
		// if(!m->tt2t){
		// 	ret = -1;goto end;
		// }
		break;
	case qtk_mod_tt2t_BF_DEtt2t:
		printf("MOD BF DEtt2t resname=%.*s\n",m->cfg->dett2t_fn.len,m->cfg->dett2t_fn.data);
		wtk_log_log(m->log, "MOD BF DEtt2t resname=%.*s",m->cfg->dett2t_fn.len,m->cfg->dett2t_fn.data);
		// if(m->cfg->use_bin){
		// 	m->dett2t_cfg = qtk_signal_dett2t_cfg_new_bin(m->cfg->dett2t_fn.data);
		// }else{
		// 	m->dett2t_cfg = qtk_signal_dett2t_cfg_new(m->cfg->dett2t_fn.data);
		// }
		// if(!m->dett2t_cfg)
		// {
		// 	ret=-1;
		// 	goto end;
		// }
		// m->dett2t = qtk_signal_dett2t_new(m->dett2t_cfg);
		// if(!m->dett2t)
		// {
		// 	ret=-1;
		// 	goto end;
		// }
		// qtk_signal_dett2t_set_notify(m->dett2t, m, (qtk_signal_dett2t_notify_f)qtk_mod_tt2t_on_dett2t);
		break;
	default:
		wtk_debug("not supported bf type, please check res/cfg.\n");
		wtk_log_warn0(m->log, "not supported bf type, please check res/cfg.");
		ret = -1;
		goto end;
	}

	ret = 0;
end:
	return ret;
}

void qtk_mod_tt2t_engine_delete(qtk_mod_tt2t_t *m, int type)
{
	switch(type){
	case qtk_mod_tt2t_BF_tt2t:
		// if(m->tt2t){
		// 	qtk_signal_tt2t_delete(m->tt2t);
		// }
		// if(m->tt2t_cfg){
		// 	if(m->cfg->use_bin){
		// 		qtk_signal_tt2t_cfg_delete_bin(m->tt2t_cfg);
		// 	}else{
		// 		qtk_signal_tt2t_cfg_delete(m->tt2t_cfg);
		// 	}
		// }
		break;
	case qtk_mod_tt2t_BF_DEtt2t:
		// if(m->dett2t)
		// {
		// 	qtk_signal_dett2t_delete(m->dett2t);
		// }
		// if(m->dett2t_cfg)
		// {
		// 	if(m->cfg->use_bin){
		// 		qtk_signal_dett2t_cfg_delete_bin(m->dett2t_cfg);
		// 	}else{
		// 		qtk_signal_dett2t_cfg_delete(m->dett2t_cfg);
		// 	}
		// }
		break;
	default:
		break;
	}
}

void qtk_mod_tt2t_engine_start(qtk_mod_tt2t_t *m, int type)
{
	switch(type){
	case qtk_mod_tt2t_BF_tt2t:
		// if(m->tt2t){
		// 	qtk_signal_tt2t_start(m->tt2t);
		// }
		break;
	case qtk_mod_tt2t_BF_DEtt2t:
		// if(m->dett2t)
		// {
		// 	qtk_signal_dett2t_start(m->dett2t);
		// }
		break;
	default:
		break;
	}
}

void qtk_mod_tt2t_engine_stop(qtk_mod_tt2t_t *m, int type)
{
	switch(type){
	case qtk_mod_tt2t_BF_tt2t:
		// if(m->tt2t){
		// 	qtk_signal_tt2t_reset(m->tt2t);
		// }
		break;
	case qtk_mod_tt2t_BF_DEtt2t:
		// if(m->dett2t)
		// {
		// 	qtk_signal_dett2t_reset(m->dett2t);
		// }
		break;
	default:
		break;
	}
}

int qtk_mod_tt2t_engine_feed(qtk_mod_tt2t_t *m, int type, char *data, int len)
{
	switch(type){
	case qtk_mod_tt2t_BF_tt2t:
		// if(m->tt2t){
		// 	qtk_signal_tt2t_feed(m->tt2t, data, len);
		// }
		break;
	case qtk_mod_tt2t_BF_DEtt2t:
		// if(m->dett2t)
		// {
		// 	qtk_signal_dett2t_feed(m->dett2t, data, len);
		// }
		break;
	default:
		break;
	}
	return 0;
}

static void qtk_log_wav_file_new(qtk_mod_tt2t_t *m)
{
#ifndef __ANDROID__
	int channel = m->cfg->rcd.channel-m->cfg->rcd.nskip;
	int bytes_per_sample = m->cfg->rcd.bytes_per_sample;
	int sample_rate = m->cfg->rcd.sample_rate;
#else
	int channel = m->cfg->rcd.channel-m->cfg->rcd.nskip;
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
}

static void qtk_log_wav_file_delete(qtk_mod_tt2t_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);
	m->mul = NULL;
}

void qtk_mod_tt2t_is_log_audio(qtk_mod_tt2t_t *m)
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

void qtk_mod_tt2t_set_cpu(qtk_mod_tt2t_t *m, wtk_thread_t *thread, int cpunum)
{
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
}