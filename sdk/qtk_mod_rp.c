#include "qtk_mod_rp.h"
#include "sdk/qtk_api.h"
#include <time.h>

int qtk_mod_rp_rcd_entry(qtk_mod_rp_t *m, wtk_thread_t *t);
void qtk_mod_rp_set_cpu(qtk_mod_rp_t *m, int cpunum);
#ifndef __ANDROID__
int qtk_mod_rp_usbaudio_entry(qtk_mod_rp_t *m, wtk_thread_t *t);
int qtk_mod_rp_lineout_entry(qtk_mod_rp_t *m, wtk_thread_t *t);
#endif
int qtk_mod_rp_savepcm_entry(qtk_mod_rp_t *m, wtk_thread_t *t);
static void qtk_log_wav_file_new(qtk_mod_rp_t *m);
static void qtk_log_wav_file_delete(qtk_mod_rp_t *m);

qtk_mod_rp_t *qtk_mod_rp_new(qtk_mod_rp_cfg_t *cfg)
{
	qtk_mod_rp_t *m;
	char tmp[64];
	int ret;

	m = (qtk_mod_rp_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;

	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
#ifndef OFFLINE_TEST
	if(m->cfg->cache_path.len > 0){
		wtk_mkdir(m->cfg->cache_path.data);
		wtk_strbuf_push_f(m->check_path_buf, "%.*s/start_log_audio", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->check_path_buf, 0);
		wtk_strbuf_push_f(m->mul_path, "%.*s/mul.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->mul_path, 0);
		
		wtk_blockqueue_init(&m->savepcm_queue);
		wtk_thread_init(&m->savepcm_t,(thread_route_handler)qtk_mod_rp_savepcm_entry, m);
	}

	if(m->cfg->use_log && m->cfg->cache_path.len > 0){
		snprintf(tmp, 64, "%.*s/qvoice.log", m->cfg->cache_path.len, m->cfg->cache_path.data);
		m->log = wtk_log_new(tmp);
	}
	if(m->cfg->use_log_wav){
		qtk_log_wav_file_new(m);
	}
#endif

	char buf[256];
	wtk_get_build_timestamp(buf);
	printf("BUILD AT %s\n",buf);
	wtk_log_log(m->log, "BUILD AT %s",buf);

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_init(&m->usbaudio_queue);
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mod_rp_usbaudio_entry, m);
		m->usbaudio = qtk_play_new(&cfg->usbaudio);
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mod_rp_lineout_entry, m);
		m->lineout = qtk_play2_new(&cfg->lineout);
	}
	m->rcd =qtk_record_new(&cfg->rcd);
	if(!m->rcd){
		wtk_log_err0(m->log, "record fiald!");
		ret = -1;
   	goto end;
	}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mod_rp_rcd_entry, m);
#endif
#endif


	m->msg = qtk_msg_new();

	m->player_run=0;

	wtk_log_log0(m->log, "RECORD PLAYER NEW OK!");
	wtk_debug("RECORD PLAYER NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mod_rp_delete(m);
      m = NULL;
  	}
	return m;
}

void qtk_mod_rp_delete(qtk_mod_rp_t *m)
{
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
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
	if(m->rcd){
			qtk_record_delete(m->rcd);
	}
#endif
#endif

	if(m->msg){
			qtk_msg_delete(m->msg);
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

	if(m->log){
		wtk_log_delete(m->log);
	}
    wtk_free(m);
}

void qtk_mod_rp_start(qtk_mod_rp_t *m)
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
	if(m->cfg->use_log_wav)
	{
		m->savepcm_run = 1;
		wtk_thread_start(&m->savepcm_t);
	}

    m->rcd_run = 1;
    wtk_thread_start(&m->rcd_t);
#endif
#endif

}

void qtk_mod_rp_stop(qtk_mod_rp_t *m)
{
	wtk_debug("===============>>>>>>>>>>>>>>>>>>>>stop\n");

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	m->rcd_run = 0;
	wtk_thread_join(&m->rcd_t);
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

	if(m->savepcm_run)
	{
		m->savepcm_run = 0;
		wtk_blockqueue_wake(&m->savepcm_queue);
		wtk_thread_join(&m->savepcm_t);
	}

	wtk_debug("===============>>>>>>>>>>>>>>>>>>>>stop\n");
}

void qtk_mod_rp_clean_queue2(qtk_mod_rp_t *m, wtk_blockqueue_t *queue,int nx)
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

void qtk_mod_rp_clean_queue(qtk_mod_rp_t *m, wtk_blockqueue_t *queue)
{
	qtk_mod_rp_clean_queue2(m,queue,0);
}

void qtk_mod_rp_clean_front_queue(qtk_mod_rp_t *m, wtk_blockqueue_t *queue,int count)
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


#include "wtk/core/wtk_os.h"

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
int qtk_mod_rp_rcd_entry(qtk_mod_rp_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node, *msg_node2;
	qtk_mod_rp_set_cpu(m, 1);

	SpeexResamplerState *srsp = NULL;
	int channel = m->cfg->rcd.channel - m->cfg->rcd.nskip;
	srsp = speex_resampler_init(channel, 16000, 48000, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);

	int outresample_size = 96*100*channel;
	char *outresample = (char *)wtk_malloc(outresample_size);

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

		if(m->cfg->use_log_wav){
			msg_node2 = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node2->buf, rbuf->data, rbuf->pos);
			wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
		}

		if(m->cfg->use_resample)
		{
			double tm=time_get_ms();
            int inlen, outlen;
            memset(outresample, 0, outresample_size);
            inlen=(rbuf->pos >> 1)/channel;
            outlen=48*m->cfg->rcd.buf_time;

			speex_resampler_process_interleaved_int(srsp,
										(spx_int16_t *)(rbuf->data), (spx_uint32_t *)(&inlen), 
										(spx_int16_t *)(outresample), (spx_uint32_t *)(&outlen));
			// wtk_debug("resample time=%f\n",time_get_ms()-tm);
			if(m->cfg->use_usbaudio)
			{
				// wtk_debug("==============>>>>>>>>>>>>>rbuf->pos=%d channel=%d outlen=%d/%d outresample_size=%d\n", rbuf->pos, channel, outlen, (outlen<<1)*channel, outresample_size);
				msg_node = qtk_msg_pop_node(m->msg);
				wtk_strbuf_push(msg_node->buf, outresample, (outlen<<1)*channel);
				//wtk_debug("queue length = %d\n", m->bfio_queue.length);
				wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
			}
		}else{
			if(m->cfg->use_usbaudio)
			{
				msg_node = qtk_msg_pop_node(m->msg);
				if(channel == 1 && m->cfg->usbaudio.channel == 4){
					int pos=0;
					while(pos < rbuf->pos)
					{
						for (int i = 0; i < 4; i++) { 
                		wtk_strbuf_push(msg_node->buf, rbuf->data+ pos,2);
						}
						pos+=2;
					}
				}else if(channel == m->cfg->usbaudio.channel){
					wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
				}
				//wtk_debug("queue length = %d\n", m->bfio_queue.length);
				wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
			}
			if(m->cfg->use_lineout)
			{
				msg_node = qtk_msg_pop_node(m->msg);
				if(m->cfg->lineout.channel == 2 && channel == 1)
				{
					int pos=0;
					while(pos < rbuf->pos)
					{
						wtk_strbuf_push(msg_node->buf, rbuf->data+pos, 2);
						wtk_strbuf_push(msg_node->buf, rbuf->data+pos, 2);
						pos+=2;
					}
				}else if(channel == m->cfg->lineout.channel){
					wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
				}
				//wtk_debug("queue length = %d\n", m->bfio_queue.length);
				wtk_blockqueue_push(&m->lineout_queue, &msg_node->qn);
			}
		}
		// rlen+=(rbuf->pos/(8*96));
	}

    if (srsp) {
        speex_resampler_destroy(srsp);
        srsp = NULL;
    }
	return 0;
}

int qtk_mod_rp_usbaudio_entry(qtk_mod_rp_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;
	long ret;
	wtk_blockqueue_t *usbaudio_queue;
	int zlen=100*m->cfg->usbaudio.channel*m->cfg->usbaudio.sample_rate/1000*2;
	
	qtk_mod_rp_set_cpu(m, 1);
	
	char *zerodata = wtk_malloc(zlen);

	usbaudio_queue=&(m->usbaudio_queue);
	if(first)
	{
		qtk_play_start(m->usbaudio);
	#if 0
		memset(zerodata, 0, zlen);
		ret = qtk_play_write(m->usbaudio, zerodata, zlen, 1);
		if(ret){
			wtk_debug("play zero buf %d\n",ret);
		}
	#endif
		first=0;
	}

  	while(m->usbaudio_run){
		qn= wtk_blockqueue_pop(usbaudio_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		ret = qtk_play_write(m->usbaudio, msg_node->buf->data, msg_node->buf->pos, 1);
		
		if(msg_node)
		{
			qtk_msg_push_node(m->msg, msg_node);
		}
	}

	qtk_play_stop(m->usbaudio);
  	return 0;
}

int qtk_mod_rp_lineout_entry(qtk_mod_rp_t *m, wtk_thread_t *t)
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
#endif
#endif

int qtk_mod_rp_savepcm_entry(qtk_mod_rp_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;

    
	while(m->savepcm_run){
		qn= wtk_blockqueue_pop(&m->savepcm_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		if(m->mul){
			wtk_wavfile_write(m->mul, msg_node->buf->data, msg_node->buf->pos);
		}


		qtk_msg_push_node(m->msg, msg_node);
	}

	return 0;
}

static void qtk_log_wav_file_new(qtk_mod_rp_t *m)
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

static void qtk_log_wav_file_delete(qtk_mod_rp_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);
	m->mul = NULL;
}

void qtk_mod_rp_set_cpu(qtk_mod_rp_t *m, int cpunum)
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