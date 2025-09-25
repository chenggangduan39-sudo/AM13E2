#include "qtk_mod_double.h"
#include "sdk/qtk_api.h"
#include <time.h>
#include <pthread.h>
#ifdef USE_KEROS
#include "sdk/codec/keros/qtk_keros.h"
#endif
// #define ONLAY_PLAY
// #define ONLAY_RECORD

#ifndef OFFLINE_TEST
int qtk_mod_double_rcd_entry(qtk_mod_double_t *m, wtk_thread_t *t);
#endif
#ifdef USE_FOR_DEV
int qtk_mod_double_uart_entry(qtk_mod_double_t *m, wtk_thread_t *t);
#endif
#ifndef __ANDROID__
int qtk_mod_double_usbaudio_entry(qtk_mod_double_t *m, wtk_thread_t *t);
int qtk_mod_double_lineout_entry(qtk_mod_double_t *m, wtk_thread_t *t);
#endif
int qtk_mod_double_qform_entry(qtk_mod_double_t *m, wtk_thread_t *t);
int qtk_mod_double_qform2_entry(qtk_mod_double_t *m, wtk_thread_t *t);
int qtk_mod_double_savepcm_entry(qtk_mod_double_t *m, wtk_thread_t *t);
int qtk_mod_double_merge_entry(qtk_mod_double_t *m, wtk_thread_t *t);

void qtk_mod_double_on_vboxebf(qtk_mod_double_t *mod, char *data, int len);
#ifdef USE_SSL
void qtk_mod_double_on_ssl(qtk_mod_double_t *mod,qtk_ssl_extp_t *extp);
#endif
void qtk_mod_double_on_aec(qtk_mod_double_t *m, qtk_var_t *var);
void qtk_mod_double_on_beamnet(qtk_mod_double_t *m, char *data, int len);
void qtk_mod_double_on_beamnet2(qtk_mod_double_t *m, char *data, int len);
void qtk_mod_double_on_resample(qtk_mod_double_t *m, char *data, int len);
void qtk_mod_double_on_resample2(qtk_mod_double_t *m, char *data, int len);

static void qtk_log_wav_file_new(qtk_mod_double_t *m);
static void qtk_log_wav_file_delete(qtk_mod_double_t *m);
void qtk_mod_double_is_log_audio(qtk_mod_double_t *m);
void qtk_mod_double_set_cpu(qtk_mod_double_t *m, wtk_thread_t *thread, int cpunum);

int qtk_mod_double_engine_new(qtk_mod_double_t *m, int type);
void qtk_mod_double_engine_delete(qtk_mod_double_t *m, int type);
void qtk_mod_double_engine_start(qtk_mod_double_t *m, int type);
void qtk_mod_double_engine_stop(qtk_mod_double_t *m, int type);
int qtk_mod_double_engine_feed(qtk_mod_double_t *m, int type, char *data, int len, int is_end);

static int skip_frame=100;

int qtk_mod_double_msg_bytes(qtk_msg_node_t *node)
{
	int bytes=0;
	bytes+=wtk_strbuf_bytes(node->buf);
	return bytes;
}

int qtk_mod_double_bytes(qtk_mod_double_t *m)
{
	int bytes=0;
	int tmp=0;
	bytes+=wtk_strbuf_bytes(m->check_path_buf);
	bytes+=wtk_strbuf_bytes(m->mul_path);
	bytes+=wtk_strbuf_bytes(m->single_path);
	bytes+=wtk_strbuf_bytes(m->playbuf);
	// if(m->cfg->audio_type == qtk_mod_double_BF_VBOXEBF)
	// {
	// 	bytes+=qtk_vboxebf_bytes(m->vboxebf);
	// }
#ifndef OFFLINE_TEST
	if(m->rcd)
	{
		bytes+=wtk_strbuf_bytes(m->rcd->buf);
	}
#endif
	tmp=wtk_hoard_bytes((wtk_hoard_t *)(&(m->msg->msg_hoard)),(wtk_hoard_bytes_f)qtk_mod_double_msg_bytes);
	bytes+=tmp;
	bytes+=sizeof(qtk_mod_double_t);
	wtk_log_log(m->log, "all=%d bytes=%d hoard_length=%d/%d\n",bytes,tmp,m->msg->msg_hoard.use_length,m->msg->msg_hoard.cur_free);
	return bytes;
}

qtk_mod_double_t *qtk_mod_double_new(qtk_mod_double_cfg_t *cfg)
{
	qtk_mod_double_t *m;
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

	m = (qtk_mod_double_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;
	m->merge_run = 0;
	m->qform_run = 0;
	m->qform2_run = 0;
	m->savepcm_run = 0;
	
	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
	m->single_path = wtk_strbuf_new(64,0);
#ifndef OFFLINE_TEST
	wtk_sem_init(&(m->rcd_sem),0);
#endif
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

		wtk_blockqueue_init(&m->savepcm_queue);
		wtk_thread_init(&m->savepcm_t,(thread_route_handler)qtk_mod_double_savepcm_entry, m);
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
	m->playbuf = wtk_strbuf_new(1024, 1.0);
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_init(&m->usbaudio_queue);
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mod_double_usbaudio_entry, m);
		m->usbaudio = qtk_play_new(&cfg->usbaudio);
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mod_double_lineout_entry, m);
		m->lineout = qtk_play2_new(&cfg->lineout);
	}
#endif
#endif

	m->resample = wtk_resample_new(960);
	wtk_resample_set_notify(m->resample, m, (wtk_resample_notify_f)qtk_mod_double_on_resample);
	m->resample2 = wtk_resample_new(960);
	wtk_resample_set_notify(m->resample2, m, (wtk_resample_notify_f)qtk_mod_double_on_resample2);

#ifndef OFFLINE_TEST
#ifndef ONLAY_PLAY
	m->rcd =qtk_record_new(&cfg->rcd);
//	if(!m->rcd){
//		wtk_log_err0(m->log, "record fiald!");
//		ret = -1;
//    	goto end;
//	}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mod_double_rcd_entry, m);
#endif
#endif

	m->msg = qtk_msg_new();
	wtk_thread_init(&m->qform_t,(thread_route_handler)qtk_mod_double_qform_entry, m);
	wtk_blockqueue_init(&m->bfio_queue);

	wtk_thread_init(&m->qform2_t,(thread_route_handler)qtk_mod_double_qform2_entry, m);
	wtk_blockqueue_init(&m->qform_queue);

	wtk_thread_init(&m->merge_t, (thread_route_handler)qtk_mod_double_merge_entry, m);
	wtk_blockqueue_init(&m->merge_queue);

	ret = qtk_mod_double_engine_new(m, 7);
	if(ret!=0)
	{
		wtk_log_err(m->log, "audio engine new fiald! type=%d", 6);
		goto end;
	}
#ifdef USE_BEAMNET
	m->beamnet2_cfg = qtk_beamnet_cfg_new(m->cfg->qform2_fn.data);
	if(!m->beamnet2_cfg)
	{
		ret=-1;
		goto end;
	}
	m->beamnet2 = qtk_beamnet_new(m->beamnet2_cfg);
	if(m->beamnet2)
	{
		qtk_beamnet_set_notify(m->beamnet2, m, (qtk_beamnet_notify_f)qtk_mod_double_on_beamnet2);
	}
#endif

	m->zdata = (char *)wtk_malloc(4096);
	memset(m->zdata, 0, 4096);

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		wtk_blockqueue_init(&m->uart_queue);
		wtk_thread_init(&m->uart_t, (thread_route_handler)qtk_mod_double_uart_entry, m);
		m->uart_cfg = wtk_main_cfg_new_type(qtk_uart_cfg,m->cfg->uart_fn.data);
		m->uart = qtk_uart_new((qtk_uart_cfg_t *)(m->uart_cfg->cfg));
	}
#endif
	m->is_player_start=0;
	m->player_run=0;

	/////////////////////

	wtk_log_log0(m->log, "MODULE NEW OK!");
	wtk_debug("MOD NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mod_double_delete(m);
      m = NULL;
  	}
	return m;
}

void qtk_mod_double_delete(qtk_mod_double_t *m)
{
	wtk_blockqueue_clean(&m->bfio_queue);
	wtk_blockqueue_clean(&m->qform_queue);

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	wtk_sem_clean(&(m->rcd_sem));
	if(m->cfg->use_lineout){
		wtk_blockqueue_clean(&m->lineout_queue);
		if(m->lineout){
			qtk_play2_delete(m->lineout);
		}
	}
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_clean(&m->usbaudio_queue);
		if(m->usbaudio){
			qtk_play_delete(m->usbaudio);
		}
	}

	wtk_blockqueue_clean(&m->savepcm_queue);

	if(m->playbuf)
	{
		wtk_strbuf_delete(m->playbuf);
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

	if(m->resample)
	{
		wtk_resample_delete(m->resample);
	}
	if(m->resample2)
	{
		wtk_resample_delete(m->resample2);
	}

#ifdef USE_BEAMNET
	if(m->beamnet2)
	{
		qtk_beamnet_delete(m->beamnet2);
	}
	if(m->beamnet2_cfg)
	{
		qtk_beamnet_cfg_delete(m->beamnet2_cfg);
	}
#endif
	qtk_mod_double_engine_delete(m, 7);

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
	if(m->log){
		wtk_log_delete(m->log);
	}
	if(m->zdata){
		wtk_free(m->zdata);
	}
    wtk_free(m);
}

void qtk_mod_double_start(qtk_mod_double_t *m)
{
	//first start audio play thread;
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		int thread_policy;

		m->usbaudio_run = 1;
		wtk_thread_start(&m->usbaudio_t);
	}	
	if(m->cfg->use_lineout){
		m->lineout_run = 1;
		wtk_thread_start(&m->lineout_t);
	}
#endif
#endif

	if(m->cfg->use_resample)
	{
		wtk_resample_start(m->resample, 16000, m->cfg->resample_rate);
	}
	if(m->cfg->use_resample)
	{
		wtk_resample_start(m->resample2, 16000, m->cfg->resample_rate);
	}

	m->merge_run = 1;
	wtk_thread_start(&m->merge_t);

#ifdef USE_BEAMNET
	// m->beamnet2_cfg->fix_theta = 50;
	qtk_beamnet_start(m->beamnet2);
#endif
	m->qform_run = 1;
	wtk_thread_start(&m->qform_t);

	// m->beamnet_cfg->fix_theta = 130;
	qtk_mod_double_engine_start(m, 7);
	m->qform2_run = 1;
	wtk_thread_start(&m->qform2_t);

	// if(m->cfg->use_log_wav)
	{
		m->savepcm_run = 1;
		wtk_thread_start(&m->savepcm_t);
	}

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		m->uart_run = 1;
		wtk_thread_start(&m->uart_t);
	}
#endif

#ifndef OFFLINE_TEST
#ifndef ONLAY_PLAY
    m->rcd_run = 1;
    wtk_thread_start(&m->rcd_t);
#endif
#endif
}

void qtk_mod_double_stop(qtk_mod_double_t *m)
{
#ifndef OFFLINE_TEST
	m->rcd_run = 0;
	wtk_thread_join(&m->rcd_t);
#endif

	m->qform_run = 0;
	wtk_blockqueue_wake(&m->bfio_queue);
	wtk_thread_join(&m->qform_t);
#ifdef USE_BEAMNET
	qtk_beamnet_reset(m->beamnet2);
#endif

	m->qform2_run = 0;
	wtk_blockqueue_wake(&m->qform_queue);
	wtk_thread_join(&m->qform2_t);
	qtk_mod_double_engine_stop(m, 7);

	m->merge_run = 0;
	wtk_blockqueue_wake(&m->merge_queue);
	wtk_thread_join(&m->merge_t);

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

	if(m->cfg->use_resample)
	{
		wtk_resample_close(m->resample);
	}
	if(m->cfg->use_resample)
	{
		wtk_resample_close(m->resample2);
	}

	if(m->savepcm_run)
	{
		m->savepcm_run = 0;
		wtk_blockqueue_wake(&m->savepcm_queue);
		wtk_thread_join(&m->savepcm_t);
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
}

void qtk_mod_double_clean_queue2(qtk_mod_double_t *m, wtk_blockqueue_t *queue,int nx)
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

int  qtk_mod_double_clean_sil_queue(qtk_mod_double_t *m, wtk_blockqueue_t *q)
{
	wtk_queue_node_t *qn;
	qtk_msg_node_t *msg_node;
	int nx=0;

	wtk_debug("======================+>>>>>>>>>>>>>..sil_queue\n");
	if(q->length<=0){return nx;}

	wtk_lock_lock(&(q->l));
	for(qn=q->pop;qn;qn=qn->next)
	{
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(msg_node->sil)
		{
			wtk_queue_remove((wtk_queue_t*)q,qn);
			msg_node = data_offset2(qn,qtk_msg_node_t,qn);
			qtk_msg_push_node(m->msg, msg_node);
			++nx;
			break;
		}
	}
	wtk_lock_unlock(&(q->l));
	return nx;
}


void qtk_mod_double_clean_queue(qtk_mod_double_t *m, wtk_blockqueue_t *queue)
{
	qtk_mod_double_clean_queue2(m,queue,0);
}

void qtk_mod_double_clean_front_queue(qtk_mod_double_t *m, wtk_blockqueue_t *queue,int count)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int i=0;

	while(++i<count)
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
int qtk_mod_double_uart_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	char *out=(char *)wtk_malloc(10);
	int olen;
	int i;
    
	while(m->uart_run){
		qn= wtk_blockqueue_pop(&m->uart_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		switch (msg_node->type)
		{
		case qtk_mod_double_BF_ASR_TXT:
			qtk_comm_get_number(msg_node->buf->data, msg_node->buf->pos,out, &olen);
			if(m->uart)
			{
				qtk_uart_write(m->uart, out, olen);
			}
			break;
		case qtk_mod_double_BF_DIRECTION:
			if(m->uart)
			{
				qtk_uart_write(m->uart, msg_node->buf->data, msg_node->buf->pos);
			}
			break;
		default:
			break;
		}

		qtk_msg_push_node(m->msg, msg_node);
	}

	wtk_free(out);
}
#endif

#ifdef OFFLINE_TEST
void qtk_mod_double_feed(qtk_mod_double_t *m, char *data, int len)
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
	// wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
	wtk_blockqueue_push(&m->qform_queue, &msg_node->qn);
#endif

}
#endif
#ifndef OFFLINE_TEST
int qtk_mod_double_rcd_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node, *msg_node2;
	int mchannel=m->cfg->rcd.channel - m->cfg->rcd.nskip - m->cfg->spknum;
	int head_cnt=m->cfg->skip_head_tm * 32 * (m->cfg->rcd.channel - m->cfg->rcd.nskip);
	int skip_len=0;
	int count=0;
	double tm=0.0;

	qtk_mod_double_set_cpu(m, t, 0);
	//m->rcd =qtk_record_new(&(m->cfg->rcd));
	//wtk_sem_acquire(&(m->rcd_sem),-1);

	while(m->rcd_run){
		// tm = time_get_ms();
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

		if(m->cfg->mic_shift != 1.0 || m->cfg->spk_shift != 1.0)
		{
			qtk_data_change_vol2(rbuf->data, rbuf->pos, m->cfg->mic_shift, m->cfg->spk_shift, mchannel, m->cfg->spknum);
		}

		count++;
		if(count == 50 && m->cfg->use_log_wav==0)
		{
			qtk_mod_double_is_log_audio(m);
			count=0;
		}
		
		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
		//wtk_debug("queue length = %d\n", m->qform_queue.length);
		wtk_blockqueue_push(&m->qform_queue, &msg_node->qn);

		msg_node2 = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node2->buf, rbuf->data, rbuf->pos);
		wtk_blockqueue_push(&m->bfio_queue, &msg_node2->qn);
		// tm = time_get_ms() - tm;
		// wtk_debug("=================>>>>>>>>>>>>>>>>>>>>>.tm=%f\n",tm);
	}
	return 0;
}
#endif

int qtk_mod_double_merge_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	qtk_msg_node_t *msg_node3;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *buf1;
	wtk_strbuf_t *buf2;
	int i;
	int pos=1024;

	buf1 = wtk_strbuf_new(2048, 1.0);
	buf2 = wtk_strbuf_new(2048, 1.0);

	wtk_strbuf_reset(buf1);
	wtk_strbuf_reset(buf2);

	qtk_mod_double_set_cpu(m, t, 0);

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
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
			if(m->cfg->use_usbaudio)
			{
				msg_node2 = qtk_msg_pop_node(m->msg);
				msg_node2->type = qtk_mod_double_BF_VBOXEBF;
				i=0;
				while(i<pos)
				{
					wtk_strbuf_push(msg_node2->buf, buf1->data+i, 2);
					wtk_strbuf_push(msg_node2->buf, buf2->data+i, 2);
					i+=2;
				}
				wtk_blockqueue_push(&m->usbaudio_queue, &msg_node2->qn);
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
		}


		qtk_msg_push_node(m->msg, msg_node);
	}

	wtk_strbuf_delete(buf1);
	wtk_strbuf_delete(buf2);
}

int qtk_mod_double_qform2_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	wtk_queue_node_t *qn;

	qtk_mod_double_set_cpu(m, t, 2);

	while(m->qform2_run){
		qn= wtk_blockqueue_pop(&m->qform_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug)
		{
			if(m->qform_queue.length > 2)
			{
				wtk_debug("qform =============>>%d\n",m->qform_queue.length);
				wtk_log_log(m->log, "qform =============>>%d",m->qform_queue.length);
			}
		}

		if(m->cfg->use_log_wav || m->log_audio)
		{
			msg_node2 = qtk_msg_pop_node(m->msg);
			msg_node2->type = qtk_mod_double_BF_SAVE_SOURCE;
			wtk_strbuf_push(msg_node2->buf, msg_node->buf->data, msg_node->buf->pos);
			wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
		}

		qtk_mod_double_engine_feed(m, 7, msg_node->buf->data, msg_node->buf->pos, 0);
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mod_double_engine_feed(m, 7, NULL, 0, 1);
	return 0;
}

int qtk_mod_double_qform_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	wtk_queue_node_t *qn;

	qtk_mod_double_set_cpu(m, t, 3);

	while(m->qform_run){
		qn= wtk_blockqueue_pop(&m->bfio_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug)
		{
			if(m->bfio_queue.length > 2)
			{
				wtk_debug("qform =============>>%d\n",m->bfio_queue.length);
				wtk_log_log(m->log, "qform =============>>%d",m->bfio_queue.length);
			}
		}

		if(m->cfg->use_log_wav || m->log_audio)
		{
			msg_node2 = qtk_msg_pop_node(m->msg);
			msg_node2->type = qtk_mod_double_BF_SAVE_SOURCE;
			wtk_strbuf_push(msg_node2->buf, msg_node->buf->data, msg_node->buf->pos);
			wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
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

#include "wtk/core/wtk_os.h"

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
int qtk_mod_double_usbaudio_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
	int first = 1;
	int delaytime=64,writelen;
	long ret,poplen;
	double wtm=0;
	int wret=0;
	int type;
	// wtk_strbuf_t *buf=m->playbuf;
	wtk_blockqueue_t *usbaudio_queue;
	int kx=0;
	int ki=0;
	double last_t,xt=0,xt2,xt3;
	int zlen=100*m->cfg->usbaudio.channel*m->cfg->usbaudio.sample_rate/1000*2;
	char *zerodata = wtk_malloc(zlen);
	int mmcount=0;

	qtk_mod_double_set_cpu(m, t, 1);

	usbaudio_queue=&(m->usbaudio_queue);
	last_t=0;
  	while(m->usbaudio_run){
		qn= wtk_blockqueue_pop(usbaudio_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		// wtk_debug("===========================>>>>>>>>>>>>>>>>>>>>>tm=%f %d\n",time_get_ms() - xt2, msg_node->buf->pos);
  		// xt2=time_get_ms();

		if(m->cfg->debug)
		{
			if(wtm > 5.0)
			{
				wtk_debug("wwwwwwwwwwwwwwwwwwwww>>%f\n",wtm);
				wtk_log_log(m->log, "wwwwwwwwwwwwwwwwwwwwww>>%f",wtm);
			}
			if(m->usbaudio_queue.length > 3)
			{
				wtk_debug("usbaudio =============>>%d %f\n",m->usbaudio_queue.length,wtm);
				wtk_log_log(m->log, "usbaudio =============>>%d",m->usbaudio_queue.length);
			}
		}

		if(first)
		{
			qtk_play_start(m->usbaudio);
		#if 1
			memset(zerodata, 0, zlen);
			ret = qtk_play_write(m->usbaudio, zerodata, zlen, 1);
			if(ret){
				wtk_debug("play zero buf %d\n",ret);
			}
		#endif
			m->is_player_start=1;
			first=0;
		}

		type=msg_node?msg_node->type:qtk_mod_double_BF_VBOXEBF;
		switch(type){
			case qtk_mod_double_BF_VBOXEBF:
				// if(m->cfg->use_log_wav || m->log_audio)
				// {
				// 	msg_node2 = qtk_msg_pop_node(m->msg);
				// 	msg_node2->type = qtk_mod_double_BF_SAVE_SPK;
				// 	wtk_strbuf_push(msg_node2->buf, msg_node->buf->data, msg_node->buf->pos);
				// 	wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
				// }
				ret = qtk_play_write(m->usbaudio, msg_node->buf->data, msg_node->buf->pos, 1);
				break;
			default:
				break;
		}
			// xt3=time_get_ms();
		if(msg_node)
		{
			qtk_msg_push_node(m->msg, msg_node);
		}
	}
	wtk_free(zerodata);
	qtk_play_stop(m->usbaudio);
  	return 0;
}

int qtk_mod_double_lineout_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	qtk_mod_double_set_cpu(m, t, 1);
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	int zlen=m->cfg->sil_time *m->cfg->lineout.channel *m->cfg->lineout.sample_rate/1000*2;
	char *zerodata = wtk_malloc(zlen);

	qtk_play2_start(m->lineout);
    
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
		if(first == 1){
			qtk_play2_write(m->lineout, zerodata, zlen);
			first = 0;
		}

		qtk_play2_write(m->lineout, msg_node->buf->data, msg_node->buf->pos);
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_play2_stop(m->lineout);
	if(zerodata)
	{
		wtk_free(zerodata);
	}
	return 0;
}
#endif
#endif

int qtk_mod_double_savepcm_entry(qtk_mod_double_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
#ifdef USE_R328
	m->slen=0;
#endif

	qtk_mod_double_set_cpu(m, t, 0);
    
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
		case qtk_mod_double_BF_SAVE_SOURCE:
			if(m->mul){
				wtk_wavfile_write(m->mul, msg_node->buf->data, msg_node->buf->pos);
			}
			break;
		case qtk_mod_double_BF_SAVE_SPK:
			if(m->single)
			{
				wtk_wavfile_write(m->single, msg_node->buf->data, msg_node->buf->pos);
			}
			break;
		default:
			break;
		}

		qtk_msg_push_node(m->msg, msg_node);
	}

	return 0;
}

// double samtm=0.0;
void qtk_mod_double_on_resample2(qtk_mod_double_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node;

	msg_node = qtk_msg_pop_node(m->msg);
	msg_node->type = 2;
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->merge_queue, &msg_node->qn);
}

void qtk_mod_double_on_resample(qtk_mod_double_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node;

	msg_node = qtk_msg_pop_node(m->msg);
	msg_node->type = 1;
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->merge_queue, &msg_node->qn);
}

void qtk_mod_double_on_qform(qtk_mod_double_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node, *msg_node2;
	static int frame=0;

	if(frame<=skip_frame)
	{
		++frame;
	}

	if(frame!=skip_frame)
	{
		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, data, len);
		//wtk_debug("queue length = %d\n", m->bfio_queue.length);
		wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
	}

	if(frame<=skip_frame)
	{
		msg_node2 = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node2->buf, m->zdata, len);
		//wtk_debug("queue length = %d\n", m->bfio_queue.length);
		wtk_blockqueue_push(&m->bfio_queue, &msg_node2->qn);
	}
}

void qtk_mod_double_on_beamnet(qtk_mod_double_t *m, char *data, int len)
{
	if(m->cfg->echo_shift != 1.0f)
	{
		qtk_data_change_vol(data, len, m->cfg->echo_shift);
	}

	if(m->cfg->use_resample)
	{
		wtk_resample_feed(m->resample, data, len, 0);
	}else{
		qtk_msg_node_t *msg_node;

		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 1;
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->merge_queue, &msg_node->qn);
	}
}

void qtk_mod_double_on_beamnet2(qtk_mod_double_t *m, char *data, int len)
{
	if(m->cfg->echo_shift != 1.0f)
	{
		qtk_data_change_vol(data, len, m->cfg->echo_shift);
	}

	if(m->cfg->use_resample)
	{
		wtk_resample_feed(m->resample2, data, len, 0);
	}else{
		qtk_msg_node_t *msg_node;

		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 2;
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->merge_queue, &msg_node->qn);
	}
}

void qtk_mod_double_push_output(qtk_mod_double_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node, *msg_node2, *msg_node3;
	int nx;
	char *ss;

	if(m->cfg->echo_shift != 1.0f)
	{
		qtk_data_change_vol(data, len, m->cfg->echo_shift);
	}
	if(m->cfg->use_resample)
	{
		wtk_resample_feed(m->resample, data, len, 0);
	}else{
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
		if(m->cfg->use_usbaudio){
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_double_BF_VBOXEBF;
			msg_node->sil=0;
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
				qtk_mod_double_clean_queue(m, &(m->usbaudio_queue));
			}
		}
#endif
#endif
	}

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_lineout){
		msg_node2 = qtk_msg_pop_node(m->msg);
		if(m->cfg->lineout.use_uac && (m->cfg->lineout.channel > 1))
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
			qtk_mod_double_clean_queue(m, &(m->lineout_queue));
		}
	}
#endif
#endif

	// if(m->cfg->use_log_wav || m->log_audio)
	// {
	// 	msg_node3 = qtk_msg_pop_node(m->msg);
	// 	msg_node3->type = qtk_mod_double_BF_SAVE_SPK;
	// 	wtk_strbuf_push(msg_node3->buf, data, len);
	// 	wtk_blockqueue_push(&m->savepcm_queue, &msg_node3->qn);
	// }

	// wtk_strbuf_pop(m->playbuf, NULL, len);
}

void qtk_mod_double_on_eqform(qtk_mod_double_t *m, char *data, int len)
{
	qtk_mod_double_push_output(m, data, len);
}

void qtk_mod_double_on_vboxebf(qtk_mod_double_t *m, char *data, int len)
{
	static int on_frames=0;
	++on_frames;

	// wtk_debug("=================>>>>>>>>>>>>>>>vboxebf len=%d on_frames=%d\n", len, on_frames);
	if(on_frames>=skip_frame)
	{
		on_frames=skip_frame;
		qtk_mod_double_push_output(m, data, len);
	}
}

void qtk_mod_double_on_aec(qtk_mod_double_t *m, qtk_var_t *var)
{
	switch (var->type)
	{
	case QTK_SPEECH_DATA_PCM:
		qtk_mod_double_push_output(m, var->v.str.data, var->v.str.len);
		break;
	case QTK_AEC_DIRECTION:
		wtk_debug("nbest=%d theta=%d phi=%d\n",var->v.ii.nbest,var->v.ii.theta,var->v.ii.phi);
		break;
	default:
		break;
	}
}

#ifdef USE_SSL
void qtk_mod_double_on_ssl(qtk_mod_double_t *mod,qtk_ssl_extp_t *extp)
{
	wtk_debug("ssl===>theta =%d\n",extp[0].theta);
}
#endif

void qtk_mod_double_on_vboxebf_ssl(qtk_mod_double_t *mod, qtk_var_t *var)
{
	switch (var->type)
	{
	case QTK_AEC_DIRECTION:
		wtk_debug("nbest=%d theta=%d phi=%d\n",var->v.ii.nbest,var->v.ii.theta,var->v.ii.phi);
		break;
	
	default:
		break;
	}
}

int qtk_mod_double_engine_new(qtk_mod_double_t *m, int type)
{
	int ret;
	switch(type){
	case qtk_mod_double_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		printf("MOD_BF_VBOXEBF resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		wtk_log_log(m->log, "MOD_BF_VBOXEBF resname=%.*s",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		m->vboxebf_cfg = qtk_vboxebf_cfg_new(m->cfg->vboxebf_fn.data);
		if(!m->vboxebf_cfg){
			ret = -1;
			goto end;
		}
		m->vboxebf = qtk_vboxebf_new(m->vboxebf_cfg);
		if(!m->vboxebf){
			ret = -1;goto end;
		}
		qtk_vboxebf_set_notify(m->vboxebf, m, (qtk_vboxebf_notify_f)qtk_mod_double_on_vboxebf);
		qtk_vboxebf_set_notify2(m->vboxebf, m, (qtk_engine_notify_f)qtk_mod_double_on_vboxebf_ssl);
#endif
		break;
	case qtk_mod_double_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		printf("MOD_BF_GAINNETBF resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		wtk_log_log(m->log, "MOD_BF_GAINNETBF resname=%.*s",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		m->gainnetbf_cfg = qtk_gainnetbf_cfg_new(m->cfg->vboxebf_fn.data);
		if(!m->gainnetbf_cfg)
		{
			ret=-1;
			goto end;
		}
		m->gainnetbf = qtk_gainnetbf_new(m->gainnetbf_cfg);
		if(!m->gainnetbf)
		{
			ret=-1;goto end;
		}
		qtk_gainnetbf_set_notify(m->gainnetbf, m, (qtk_gainnetbf_notify_f)qtk_mod_double_on_vboxebf);
#endif
		break;
	case qtk_mod_double_BF_SSL:
#ifdef USE_SSL
		printf("MOD_BF_SSL resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		wtk_log_log(m->log, "MOD_BF_SSL resname=%.*s",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		m->ssl_cfg = qtk_ssl_cfg_new(m->cfg->vboxebf_fn.data);
		if(!m->ssl_cfg)
		{
			ret=-1;
			goto end;
		}
		m->ssl = qtk_ssl_new(m->ssl_cfg);
		if(!m->ssl)
		{
			ret=-1;goto end;
		}
		qtk_ssl_set_notify(m->ssl, m, (qtk_ssl_notify_f)qtk_mod_double_on_ssl);
#endif
		break;
	case qtk_mod_double_BF_EQFORM:
#ifdef USE_EQFORM
		printf("qtk_mod_double_BF_EQFORM resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		wtk_log_log(m->log, "qtk_mod_double_BF_EQFORM resname=%.*s",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		m->eqform_cfg = qtk_eqform_cfg_new(m->cfg->vboxebf_fn.data);
		if(!m->eqform_cfg)
		{
			ret=-1;
			goto end;
		}
		m->eqform = qtk_eqform_new(m->eqform_cfg);
		if(!m->eqform)
		{
			ret=-1;
			goto end;
		}
		qtk_eqform_set_notify(m->eqform, m, (qtk_eqform_notify_f)qtk_mod_double_on_eqform);
#endif
		break;
	case qtk_mod_double_BF_AEC:
#ifdef USE_AEC
		printf("qtk_mod_double_BF_AEC resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		wtk_log_log(m->log, "qtk_mod_double_BF_AEC resname=%.*s",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		m->aec_cfg = qtk_aec_cfg_new(m->cfg->vboxebf_fn.data);
		if(!m->aec_cfg)
		{
			ret=-1;
			goto end;
		}
		m->aec = qtk_aec_new(m->aec_cfg);
		if(!m->aec)
		{
			ret=-1;
			goto end;
		}
		qtk_aec_set_notify2(m->aec, m, (qtk_engine_notify_f)qtk_mod_double_on_aec);
#endif
		break;
	case qtk_mod_double_BF_QFORM:
#ifdef USE_QFORM
		printf("qtk_mod_double_BF_QFORM resname=%.*s\n",m->cfg->qform_fn.len,m->cfg->qform_fn.data);
		wtk_log_log(m->log, "qtk_mod_double_BF_QFORM resname=%.*s",m->cfg->qform_fn.len,m->cfg->qform_fn.data);
		m->qform_cfg = qtk_qform_cfg_new(m->cfg->qform_fn.data);
		if(!m->qform_cfg)
		{
			ret=-1;
			goto end;
		}
		m->qform = qtk_qform_new(m->qform_cfg);
		if(!m->qform)
		{
			ret=-1;
			goto end;
		}
		qtk_qform_set_notify(m->qform, m, (qtk_qform_notify_f)qtk_mod_double_on_qform);
#endif
		break;
	case qtk_mod_double_BF_BEAMNET:
#ifdef USE_BEAMNET
		printf("qtk_mod_double_BF_BEAMNET resname=%.*s\n",m->cfg->qform_fn.len,m->cfg->qform_fn.data);
		wtk_log_log(m->log, "qtk_mod_double_BF_BEAMNET resname=%.*s",m->cfg->qform_fn.len,m->cfg->qform_fn.data);
		m->beamnet_cfg = qtk_beamnet_cfg_new(m->cfg->qform_fn.data);
		if(!m->beamnet_cfg)
		{
			ret=-1;
			goto end;
		}
		m->beamnet = qtk_beamnet_new(m->beamnet_cfg);
		if(!m->beamnet)
		{
			ret=-1;
			goto end;
		}
		qtk_beamnet_set_notify(m->beamnet, m, (qtk_beamnet_notify_f)qtk_mod_double_on_beamnet);
#endif
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

void qtk_mod_double_engine_delete(qtk_mod_double_t *m, int type)
{
	switch(type){
	case qtk_mod_double_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_delete(m->vboxebf);
		}
		if(m->vboxebf_cfg){
			qtk_vboxebf_cfg_delete(m->vboxebf_cfg);
		}
#endif
		break;
	case qtk_mod_double_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		if(m->gainnetbf)
		{
			qtk_gainnetbf_delete(m->gainnetbf);
		}
		if(m->gainnetbf_cfg)
		{
			qtk_gainnetbf_cfg_delete(m->gainnetbf_cfg);
		}
#endif
		break;
	case qtk_mod_double_BF_SSL:
#ifdef USE_SSL
		if(m->ssl)
		{
			qtk_ssl_delete(m->ssl);
		}
		if(m->ssl_cfg)
		{
			qtk_ssl_cfg_delete(m->ssl_cfg);
		}
#endif
		break;
	case qtk_mod_double_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform)
		{
			qtk_eqform_delete(m->eqform);
		}
		if(m->eqform_cfg)
		{
			qtk_eqform_cfg_delete(m->eqform_cfg);
		}
#endif
		break;
	case qtk_mod_double_BF_AEC:
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
	case qtk_mod_double_BF_QFORM:
#ifdef USE_QFORM
		if(m->qform)
		{
			qtk_qform_delete(m->qform);
		}
		if(m->qform_cfg)
		{
			qtk_qform_cfg_delete(m->qform_cfg);
		}
#endif
		break;
	case qtk_mod_double_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet)
		{
			qtk_beamnet_delete(m->beamnet);
		}
		if(m->beamnet_cfg)
		{
			qtk_beamnet_cfg_delete(m->beamnet_cfg);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mod_double_engine_start(qtk_mod_double_t *m, int type)
{
	switch(type){
	case qtk_mod_double_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_start(m->vboxebf);
		}
#endif
		break;
	case qtk_mod_double_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		if(m->gainnetbf)
		{
			qtk_gainnetbf_start(m->gainnetbf);
		}
#endif
		break;
	case qtk_mod_double_BF_SSL:
#ifdef USE_SSL
		if(m->ssl)
		{
			qtk_ssl_start(m->ssl);
		}
#endif
		break;
	case qtk_mod_double_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform)
		{
			qtk_eqform_start(m->eqform);
		}
#endif
		break;
	case qtk_mod_double_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_start(m->aec);
		}
#endif
		break;
	case qtk_mod_double_BF_QFORM:
#ifdef USE_QFORM
		if(m->qform)
		{
			qtk_qform_start(m->qform);
		}
#endif
		break;
	case qtk_mod_double_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet)
		{
			qtk_beamnet_start(m->beamnet);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mod_double_engine_stop(qtk_mod_double_t *m, int type)
{
	switch(type){
	case qtk_mod_double_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_reset(m->vboxebf);
		}
#endif
		break;
	case qtk_mod_double_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		if(m->gainnetbf)
		{
			qtk_gainnetbf_reset(m->gainnetbf);
		}
#endif
		break;
	case qtk_mod_double_BF_SSL:
#ifdef USE_SSL
		if(m->ssl)
		{
			qtk_ssl_reset(m->ssl);
		}
#endif
		break;
	case qtk_mod_double_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform)
		{
			qtk_eqform_reset(m->eqform);
		}
#endif
		break;
	case qtk_mod_double_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_reset(m->aec);
		}
#endif
		break;
	case qtk_mod_double_BF_QFORM:
#ifdef USE_QFORM
		if(m->qform)
		{
			qtk_qform_reset(m->qform);
		}
#endif
		break;
	case qtk_mod_double_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet)
		{
			qtk_beamnet_reset(m->beamnet);
		}
#endif
		break;
	default:
		break;
	}
}

int qtk_mod_double_engine_feed(qtk_mod_double_t *m, int type, char *data, int len, int is_end)
{
	switch(type){
	case qtk_mod_double_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_feed(m->vboxebf, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_double_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		if(m->gainnetbf)
		{
			qtk_gainnetbf_feed(m->gainnetbf, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_double_BF_SSL:
#ifdef USE_SSL
		if(m->ssl)
		{
			qtk_ssl_feed(m->ssl, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_double_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform)
		{
			qtk_eqform_feed(m->eqform, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_double_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_feed(m->aec, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_double_BF_QFORM:
#ifdef USE_QFORM
		if(m->qform)
		{
			qtk_qform_feed(m->qform, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_double_BF_BEAMNET:
#ifdef USE_BEAMNET
		if(m->beamnet)
		{
			qtk_beamnet_feed(m->beamnet, data, len, is_end);
		}
#endif
		break;
	default:
		break;
	}
	return 0;
}

int qtk_mod_double_get_ddff(qtk_mod_double_t *m)
{
	char result[1024];
	char *ddff="df | grep /dev/by-name/UDISK | awk '{print $4}'";
	FILE *fp=popen(ddff, "r");
	if(fp < 0)
	{
		wtk_debug("read df fiald.\n");
	}

	while(fgets(result, sizeof(result), fp) != NULL)
	{
		if('\n' == result[strlen(result)-1])
		{
			result[strlen(result)-1] = '\0';
		}
	}

	fclose(fp);
	return wtk_str_atoi2(result, strlen(result), NULL);
}

static void qtk_log_wav_file_new(qtk_mod_double_t *m)
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
	m->mul = wtk_wavfile_new(16000); 
	m->mul->max_pend = 0;

	wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);
	
	wtk_wavfile_open(m->mul, m->mul_path->data);

  m->single = wtk_wavfile_new(m->cfg->usbaudio.sample_rate); 
  m->single->max_pend = 0;
  channel = m->cfg->usbaudio.channel;
  wtk_wavfile_set_channel2(m->single,channel,bytes_per_sample);
  wtk_wavfile_open(m->single, m->single_path->data);

#ifdef USE_R328
  m->minlen = qtk_mod_double_get_ddff(m);
#endif
}

static void qtk_log_wav_file_delete(qtk_mod_double_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);		
	wtk_wavfile_close(m->single);
	wtk_wavfile_delete(m->single);
	m->mul = NULL;
	m->single = NULL;
}

void qtk_mod_double_is_log_audio(qtk_mod_double_t *m)
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
		m->slen = 0;
	}
}

void qtk_mod_double_set_cpu(qtk_mod_double_t *m, wtk_thread_t *thread, int cpunum)
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