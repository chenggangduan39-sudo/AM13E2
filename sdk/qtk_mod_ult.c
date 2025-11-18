#include "qtk_mod_ult.h"
#include "sdk/qtk_api.h"
#include <time.h>
#include <pthread.h>
#ifdef USE_KEROS
#include "sdk/codec/keros/qtk_keros.h"
#endif
// #define ONLAY_PLAY
// #define ONLAY_RECORD

int qtk_mod_ult_rcd_entry(qtk_mod_ult_t *m, wtk_thread_t *t);

int qtk_mod_ult_usbaudio_entry(qtk_mod_ult_t *m, wtk_thread_t *t);
int qtk_mod_ult_lineout_entry(qtk_mod_ult_t *m, wtk_thread_t *t);
int qtk_mod_ult_qform_entry(qtk_mod_ult_t *m, wtk_thread_t *t);
int qtk_mod_ult_savepcm_entry(qtk_mod_ult_t *m, wtk_thread_t *t);
void qtk_mod_ult_on_vboxebf(qtk_mod_ult_t *mod, char *data, int len);
static void qtk_log_wav_file_new(qtk_mod_ult_t *m);
static void qtk_log_wav_file_delete(qtk_mod_ult_t *m);
void qtk_mod_ult_is_log_audio(qtk_mod_ult_t *m);
void qtk_mod_ult_set_cpu(qtk_mod_ult_t *m, int cpunum);
void qtk_mod_ult_on_resample(qtk_mod_ult_t *m, char *data, int len);

int qtk_mod_ult_engine_new(qtk_mod_ult_t *m, int type);
void qtk_mod_ult_engine_delete(qtk_mod_ult_t *m, int type);
void qtk_mod_ult_engine_start(qtk_mod_ult_t *m, int type);
void qtk_mod_ult_engine_stop(qtk_mod_ult_t *m, int type);
int qtk_mod_ult_engine_feed(qtk_mod_ult_t *m, int type, char *data, int len, int is_end);

int qtk_mod_ult_msg_bytes(qtk_msg_node_t *node)
{
	int bytes=0;
	bytes+=wtk_strbuf_bytes(node->buf);
	return bytes;
}

int qtk_mod_ult_bytes(qtk_mod_ult_t *m)
{
	int bytes=0;
	int tmp=0;
	bytes+=wtk_strbuf_bytes(m->check_path_buf);
	bytes+=wtk_strbuf_bytes(m->mul_path);
	bytes+=wtk_strbuf_bytes(m->single_path);
	bytes+=wtk_strbuf_bytes(m->playbuf);
	if(m->rcd)
	{
		bytes+=wtk_strbuf_bytes(m->rcd->buf);
	}
	tmp=wtk_hoard_bytes((wtk_hoard_t *)(&(m->msg->msg_hoard)),(wtk_hoard_bytes_f)qtk_mod_ult_msg_bytes);
	bytes+=tmp;
	bytes+=sizeof(qtk_mod_ult_t);
	wtk_log_log(m->log, "all=%d bytes=%d hoard_length=%d/%d\n",bytes,tmp,m->msg->msg_hoard.use_length,m->msg->msg_hoard.cur_free);
	return bytes;
}

qtk_mod_ult_t *qtk_mod_ult_new(qtk_mod_ult_cfg_t *cfg)
{
	qtk_mod_ult_t *m;
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

	m = (qtk_mod_ult_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;

	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
	m->single_path = wtk_strbuf_new(64,0);
	wtk_sem_init(&(m->rcd_sem),0);
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
		wtk_thread_init(&m->savepcm_t,(thread_route_handler)qtk_mod_ult_savepcm_entry, m);
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

	m->playbuf = wtk_strbuf_new(1024, 1.0);
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_init(&m->usbaudio_queue);
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mod_ult_usbaudio_entry, m);
		m->usbaudio = qtk_play_new(&cfg->usbaudio);
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mod_ult_lineout_entry, m);
		m->lineout = qtk_play2_new(&cfg->lineout);
	}

	m->resample = wtk_resample_new(960);
	wtk_resample_set_notify(m->resample, m, (wtk_resample_notify_f)qtk_mod_ult_on_resample);

#ifndef ONLAY_PLAY
	m->rcd =qtk_record_new(&cfg->rcd);
//	if(!m->rcd){
//		wtk_log_err0(m->log, "record fiald!");
//		ret = -1;
//    	goto end;
//	}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mod_ult_rcd_entry, m);
#endif

	m->msg = qtk_msg_new();
	wtk_thread_init(&m->qform_t,(thread_route_handler)qtk_mod_ult_qform_entry, m);
	wtk_blockqueue_init(&m->bfio_queue);
	m->in_resample = NULL;
	if(m->cfg->use_in_resample)
	{
		m->in_resample = speex_resampler_init(m->cfg->rcd.channel-m->cfg->rcd.nskip, 48000, 16000, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
	}
	ret = qtk_mod_ult_engine_new(m,m->cfg->audio_type);
	if(ret!=0)
	{
		wtk_log_err(m->log, "audio engine new fiald! type=%d", m->cfg->audio_type);
		goto end;
	}

	wtk_log_log0(m->log, "MODULE NEW OK!");
	wtk_debug("MOD NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mod_ult_delete(m);
      m = NULL;
  	}
	return m;
}

void qtk_mod_ult_delete(qtk_mod_ult_t *m)
{
	wtk_blockqueue_clean(&m->bfio_queue);

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

	if(m->rcd){
			qtk_record_delete(m->rcd);
	}
	if(m->msg){
			qtk_msg_delete(m->msg);
	}

	if(m->in_resample)
	{
		speex_resampler_destroy(m->in_resample);
		m->in_resample = NULL;
	}

	if(m->resample)
	{
		wtk_resample_delete(m->resample);
	}

	qtk_mod_ult_engine_delete(m, m->cfg->audio_type);

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
    wtk_free(m);
}

void qtk_mod_ult_start(qtk_mod_ult_t *m)
{
	if(m->cfg->use_usbaudio){
		int thread_policy;

		m->usbaudio_run = 1;
		wtk_thread_start(&m->usbaudio_t);
	}	
	if(m->cfg->use_lineout){
		m->lineout_run = 1;
		wtk_thread_start(&m->lineout_t);
	}

	if(m->cfg->use_resample)
	{
		wtk_resample_start(m->resample, 16000, m->cfg->resample_rate);
	}

	qtk_mod_ult_engine_start(m, m->cfg->audio_type);
	m->qform_run = 1;
	wtk_thread_start(&m->qform_t);

	// if(m->cfg->use_log_wav)
	{
		m->savepcm_run = 1;
		wtk_thread_start(&m->savepcm_t);
	}

#ifndef ONLAY_PLAY
    m->rcd_run = 1;
    wtk_thread_start(&m->rcd_t);
#endif
}

void qtk_mod_ult_stop(qtk_mod_ult_t *m)
{
	m->rcd_run = 0;
	wtk_thread_join(&m->rcd_t);

	m->qform_run = 0;
	wtk_blockqueue_wake(&m->bfio_queue);
	wtk_thread_join(&m->qform_t);
	qtk_mod_ult_engine_stop(m, m->cfg->audio_type);

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

	if(m->cfg->use_resample)
	{
		wtk_resample_close(m->resample);
	}

	if(m->savepcm_run)
	{
		m->savepcm_run = 0;
		wtk_blockqueue_wake(&m->savepcm_queue);
		wtk_thread_join(&m->savepcm_t);
	}
}

void qtk_mod_ult_clean_queue2(qtk_mod_ult_t *m, wtk_blockqueue_t *queue,int nx)
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

int qtk_mod_ult_check_sil_queue(wtk_blockqueue_t *q)
{
	int n=0;
	wtk_queue_node_t *qn;
	qtk_msg_node_t *msg_node;

	wtk_lock_lock(&(q->l));
	for(qn=q->pop;qn;qn=qn->next)
	{
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(msg_node->sil)
		{
			wtk_queue_remove((wtk_queue_t*)q,qn);
			break;
		}
	}
	wtk_lock_unlock(&(q->l));
}

int  qtk_mod_ult_clean_sil_queue(qtk_mod_ult_t *m, wtk_blockqueue_t *q)
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


void qtk_mod_ult_clean_queue(qtk_mod_ult_t *m, wtk_blockqueue_t *queue)
{
	qtk_mod_ult_clean_queue2(m,queue,0);
}

void qtk_mod_ult_clean_front_queue(qtk_mod_ult_t *m, wtk_blockqueue_t *queue,int count)
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

void qtk_mod_ult_feed(qtk_mod_ult_t *m, char *data, int len)
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

int qtk_mod_ult_rcd_entry(qtk_mod_ult_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node, *msg_node2;
	int mchannel=m->cfg->rcd.channel - m->cfg->rcd.nskip - m->cfg->spknum;
	int head_cnt=m->cfg->skip_head_tm * 32 * (m->cfg->rcd.channel - m->cfg->rcd.nskip);
	int skip_len=0;
	int count=0;
	double tm=0.0;

	qtk_mod_ult_set_cpu(m, 1);
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
			qtk_mod_ult_is_log_audio(m);
			count=0;
		}
		

		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
		//wtk_debug("queue length = %d\n", m->bfio_queue.length);
		wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
		// rlen+=(rbuf->pos/(8*96));
		// tm = time_get_ms() - tm;
		// wtk_debug("=================>>>>>>>>>>>>>>>>>>>>>.tm=%f\n",tm);

	}
	return 0;
}

int qtk_mod_ult_qform_entry(qtk_mod_ult_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	wtk_queue_node_t *qn;
	int mmcount=0;
	int inlen;
	int outlen=m->cfg->rcd.sample_rate*m->cfg->rcd.buf_time*m->cfg->rcd.channel*2/1000;
	char *outresample=(char *)wtk_malloc(outlen);

	// wtk_strbuf_reset(m->playbuf);

	qtk_mod_ult_set_cpu(m, 3);

	while(m->qform_run){
		if(m->bfio_queue.length > m->cfg->max_add_count)
		{
			wtk_debug( "qform clean=============>>%d\n",m->bfio_queue.length);
			wtk_log_log(m->log, "qform clean=============>>%d",m->bfio_queue.length);
			qtk_mod_ult_clean_queue(m, &m->bfio_queue);
		}
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
			// if(++mmcount == 300)
			// {
			// 	mmcount=0;
			// 	qtk_mod_ult_bytes(m);
			// }
		}
		// if(++mmcount == 100)
		// {
		// 	mmcount=0;
		// 	wtk_debug("====================>>>>>bfio_queue lenght=%d\n",m->bfio_queue.length);
		// }

		memset(outresample, 0, msg_node->buf->pos);
		inlen=(msg_node->buf->pos >> 1)/(m->cfg->rcd.channel-m->cfg->rcd.nskip);
		outlen=inlen;

		if(m->in_resample)
		{
			speex_resampler_process_interleaved_int(m->in_resample,
										(spx_int16_t *)(msg_node->buf->data), (spx_uint32_t *)(&inlen), 
										(spx_int16_t *)(outresample), (spx_uint32_t *)(&outlen));
		}

		if(m->cfg->use_log_wav || m->log_audio)
		{
			msg_node2 = qtk_msg_pop_node(m->msg);
			msg_node2->type = qtk_mod_ult_BF_SAVE_SOURCE;
			if(m->in_resample)
			{
				wtk_strbuf_push(msg_node2->buf, outresample, (outlen<<1)*(m->cfg->rcd.channel-m->cfg->rcd.nskip));
			}else{
				wtk_strbuf_push(msg_node2->buf, msg_node->buf->data, msg_node->buf->pos);
			}
			wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
		}

#ifndef ONLAY_RECORD
		if(m->in_resample)
		{
			qtk_mod_ult_engine_feed(m, m->cfg->audio_type, outresample, (outlen<<1)*(m->cfg->rcd.channel-m->cfg->rcd.nskip), 0);
		}else{
			qtk_mod_ult_engine_feed(m, m->cfg->audio_type, msg_node->buf->data, msg_node->buf->pos, 0);
		}
#endif
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mod_ult_engine_feed(m, m->cfg->audio_type, NULL, 0, 1);
	if(outresample)
	{
		wtk_free(outresample);
	}
	return 0;
}

#include "wtk/core/wtk_os.h"

int qtk_mod_ult_usbaudio_entry(qtk_mod_ult_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	int ret;
	short *bufdata;
	int datalen;
	int pushlen=32*m->cfg->usbaudio.sample_rate*2/1000;
	wtk_strbuf_t *playbuf=wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(playbuf);
	qtk_mod_ult_set_cpu(m, 1);
	double tm;

  	while(m->usbaudio_run){
		if(first)
		{
			datalen = qtk_ultrasonic_get_signal(m->ult, &bufdata);
			qtk_play_start(m->usbaudio);
			first=0;
			tm = time_get_ms();
		}
		
		if(m->cfg->usbaudio.channel == 1)
		{
			wtk_strbuf_push(playbuf, bufdata, datalen<<1);
			if(playbuf->pos >= pushlen)
			{
				ret = qtk_play_write(m->usbaudio, playbuf->data, playbuf->pos, 1);
				usleep(playbuf->pos/32 * 1000);
				wtk_strbuf_pop(playbuf, NULL, pushlen);
			}
		}else{
			int i=0;
			while(i < datalen)
			{
				wtk_strbuf_push(playbuf, bufdata + i, 2);
				wtk_strbuf_push(playbuf, bufdata + i, 2);
				i++;
			}
			if(playbuf->pos >= (pushlen*2))
			{
				if(m->cfg->use_log_wav || m->log_audio)
				{
					msg_node = qtk_msg_pop_node(m->msg);
					msg_node->type = qtk_mod_ult_BF_SAVE_SPK;
					wtk_strbuf_push(msg_node->buf, playbuf->data, playbuf->pos);

					wtk_blockqueue_push(&m->savepcm_queue, &msg_node->qn);
				}
				ret = qtk_play_write(m->usbaudio, playbuf->data, playbuf->pos, 1);
				tm = time_get_ms() - tm;
				// wtk_debug("=====================>>>>>>>>>>>>>>>>>>....tm=%f pushlen=%d pos=%d\n",tm,pushlen,playbuf->pos);
				usleep((31-tm) *  1000);
				tm = time_get_ms();
				wtk_strbuf_pop(playbuf, NULL, pushlen*2);
			}
		}
	}
	qtk_play_stop(m->usbaudio);
	wtk_strbuf_delete(playbuf);
  	return 0;
}

int qtk_mod_ult_lineout_entry(qtk_mod_ult_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
    
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
			qtk_play2_start(m->lineout);
			first = 0;
		}

		qtk_play2_write(m->lineout, msg_node->buf->data, msg_node->buf->pos);
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_play2_stop(m->lineout);
	return 0;
}

int qtk_mod_ult_savepcm_entry(qtk_mod_ult_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;

	qtk_mod_ult_set_cpu(m, 0);
    
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
		case qtk_mod_ult_BF_SAVE_SOURCE:
			if(m->mul){
				wtk_wavfile_write(m->mul, msg_node->buf->data, msg_node->buf->pos);
			}
			break;
		case qtk_mod_ult_BF_SAVE_SPK:
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

void qtk_mod_ult_on_resample(qtk_mod_ult_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node, *msg_node2, *msg_node3;
	char *ss;
	int nx=0;

	if(m->cfg->use_usbaudio){
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = qtk_mod_ult_BF_VBOXEBF;
		msg_node->sil=0;
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
			qtk_mod_ult_clean_queue(m, &(m->usbaudio_queue));
		}
	}
}

void qtk_mod_ult_on_eqform(qtk_mod_ult_t *m, char *data, int len)
{
    qtk_msg_node_t *msg_node, *msg_node2, *msg_node3;
	char *ss;
	int nx=0;
	// static double vtm;
	// vtm = time_get_ms() - vtm;
	// wtk_debug("===========================>>>>>>>>>>>>>>>>>>>>>tm=%f len=%d\n", vtm, len);
	// vtm = time_get_ms();

	if(m->cfg->echo_shift != 1.0f)
	{
		qtk_data_change_vol(data, len, m->cfg->echo_shift);
	}

	if(m->cfg->use_resample)
	{
		wtk_resample_feed(m->resample, data, len, 0);
	}else{
		if(m->cfg->use_usbaudio){
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_ult_BF_VBOXEBF;
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
				qtk_mod_ult_clean_queue(m, &(m->usbaudio_queue));
			}
		}
	}

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
			qtk_mod_ult_clean_queue(m, &(m->lineout_queue));
		}
	}
}

void qtk_mod_ult_on_vboxebf(qtk_mod_ult_t *m, char *data, int len)
{
#if 0
    qtk_msg_node_t *msg_node, *msg_node2, *msg_node3;
	char *ss;
	int nx=0;

	if(m->cfg->echo_shift != 1.0f)
	{
		qtk_data_change_vol(data, len, m->cfg->echo_shift);
	}

	if(m->cfg->use_resample)
	{
		wtk_resample_feed(m->resample, data, len, 0);
	}else{
		if(m->cfg->use_usbaudio){
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_ult_BF_VBOXEBF;
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
				qtk_mod_ult_clean_queue(m, &(m->usbaudio_queue));
			}
		}
	}

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
			qtk_mod_ult_clean_queue(m, &(m->lineout_queue));
		}
	}
	// if(m->cfg->use_log_wav || m->log_audio)
	// {
	// 	msg_node3 = qtk_msg_pop_node(m->msg);
	// 	msg_node3->type = qtk_mod_ult_BF_SAVE_SPK;
	// 	wtk_strbuf_push(msg_node3->buf, data, len);
	// 	wtk_blockqueue_push(&m->savepcm_queue, &msg_node3->qn);
	// }

	// wtk_strbuf_pop(m->playbuf, NULL, len);
#endif
}

int qtk_mod_ult_engine_new(qtk_mod_ult_t *m, int type)
{
	int ret = -1;
	m->ult_cfg = qtk_ultrasonic_cfg_new(m->cfg->vboxebf_fn.data);
	if(!m->ult_cfg){
		ret=-1;
		goto end;
	}
	m->ult = qtk_ultrasonic_new(m->ult_cfg);
	if(!m->ult){
		ret=-1;
		goto end;
	}
	qtk_ultrasonic_set_notify(m->ult, m, (qtk_ultrasonic_notify_f)qtk_mod_ult_on_vboxebf);

	ret = 0;
end:
	return ret;
}

void qtk_mod_ult_engine_delete(qtk_mod_ult_t *m, int type)
{
	if(m->ult)
	{
		qtk_ultrasonic_delete(m->ult);
	}
	if(m->ult_cfg)
	{
		qtk_ultrasonic_cfg_delete(m->ult_cfg);
	}

}

void qtk_mod_ult_engine_start(qtk_mod_ult_t *m, int type)
{
	if(m->ult)
	{
		qtk_ultrasonic_start(m->ult);
	}
}

void qtk_mod_ult_engine_stop(qtk_mod_ult_t *m, int type)
{
	if(m->ult)
	{
		qtk_ultrasonic_reset(m->ult);
	}
}

int qtk_mod_ult_engine_feed(qtk_mod_ult_t *m, int type, char *data, int len, int is_end)
{
	if(m->ult)
	{
		qtk_ultrasonic_feed(m->ult, data, len, is_end);
	}
	return 0;
}

static void qtk_log_wav_file_new(qtk_mod_ult_t *m)
{
	int channel = m->cfg->rcd.channel-m->cfg->rcd.nskip;
	int bytes_per_sample = m->cfg->rcd.bytes_per_sample;
	int sample_rate = m->cfg->rcd.sample_rate;
	
	if(m->cfg->cache_path.len <= 0){
		return ;
	}
	m->mul = wtk_wavfile_new(m->cfg->rcd.sample_rate); 
	m->mul->max_pend = 0;

	wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);
	
	wtk_wavfile_open(m->mul, m->mul_path->data);

  m->single = wtk_wavfile_new(m->cfg->usbaudio.sample_rate); 
  m->single->max_pend = 0;
  channel = m->cfg->usbaudio.channel;
  wtk_wavfile_set_channel2(m->single,channel,bytes_per_sample);
  wtk_wavfile_open(m->single, m->single_path->data);
}

static void qtk_log_wav_file_delete(qtk_mod_ult_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);		
	wtk_wavfile_close(m->single);
	wtk_wavfile_delete(m->single);
	m->mul = NULL;
	m->single = NULL;
}

void qtk_mod_ult_is_log_audio(qtk_mod_ult_t *m)
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

void qtk_mod_ult_set_cpu(qtk_mod_ult_t *m, int cpunum)
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
