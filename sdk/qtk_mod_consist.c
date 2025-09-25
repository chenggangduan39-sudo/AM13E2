#include "qtk_mod_consist.h"
#include "qtk/play/qtk_play.h"
#include "sdk/qtk_api.h"
#include <time.h>
#include <pthread.h>

#ifndef OFFLINE_TEST
int qtk_mod_consist_rcd_entry(qtk_mod_consist_t *m, wtk_thread_t *t);
#endif
int qtk_mod_consist_qform_entry(qtk_mod_consist_t *m, wtk_thread_t *t);
int qtk_mod_consist_enspick_entry(qtk_mod_consist_t *m, wtk_thread_t *thread);
void qtk_mod_consist_on_type(qtk_mod_consist_t *m, wtk_consist_micerror_type_t type, int channel);

static void qtk_mod_log_wav_file_new(qtk_mod_consist_t *m);
static void qtk_mod_log_wav_file_delete(qtk_mod_consist_t *m);
void qtk_mod_consist_set_cpu(qtk_mod_consist_t *m, int cpunum);

char *qtk_mod_consist_get_str(qtk_mod_consist_t *m, char *comm);
int qtk_mod_consist_get_int(qtk_mod_consist_t *m, char *comm);

void qtk_mod_consist_on_vboxebf(qtk_mod_consist_t *m, char *data, int len);
void qtk_mod_consist_writetype(qtk_mod_consist_t *m, char *fname,int channel);

qtk_mod_consist_t *qtk_mod_consist_new(qtk_mod_consist_cfg_t *cfg, float recordtime,int mode,int micgain,int backgain)
{
	qtk_mod_consist_t *m;
	char tmp[64];
	int ret;

	m = (qtk_mod_consist_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;
	m->recordlen = recordtime*(cfg->rcd.channel - cfg->rcd.nskip)*32000;
	m->cfg->mode=mode;
	if(micgain > 0)
	{
		m->cfg->rcd.mic_gain=micgain;
	}
	if(backgain > 0)
	{
		m->cfg->rcd.cb_gain=backgain;
	}

	m->mul_path = wtk_strbuf_new(64,0);
#ifndef OFFLINE_TEST
	if(m->cfg->cache_path.len > 0)
	{
		wtk_mkdir(m->cfg->cache_path.data);
		wtk_strbuf_push_f(m->mul_path, "%.*s/mul.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->mul_path, 0);
	}

	if(m->cfg->use_log && m->cfg->cache_path.len > 0){
		snprintf(tmp, 64, "%.*s/qvoice.log", m->cfg->cache_path.len, m->cfg->cache_path.data);
		m->log = wtk_log_new(tmp);
	}
#endif

	char buf[256];
	wtk_get_build_timestamp(buf);
	printf("BUILD AT %s\n",buf);
	wtk_log_log(m->log, "BUILD AT %s",buf);
	wtk_debug("=====================================================\n");
	ret = -1;
	while(ret < 0)
	{
		ret = system("cat /sys/class/dmic/dmic_reg_debug");
	}
	wtk_debug("=====================================================\n");

#ifndef OFFLINE_TEST
	m->rcd = qtk_record_new(&cfg->rcd);
	if(!m->rcd){
		wtk_log_err0(m->log, "record fiald!\n");
		ret = -1;
    	goto end;
	}
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mod_consist_rcd_entry, m);
	m->play = qtk_play_new(&cfg->play);
	if(!m->play)
	{
		wtk_log_err0(m->log, "play fiald!\n");
		ret=-1;
		goto end;
	}
#endif

	wtk_sem_init(&(m->start_sem), 0);
	m->msg = qtk_msg_new();
	wtk_thread_init(&m->qform_t,(thread_route_handler)qtk_mod_consist_qform_entry, m);
	wtk_blockqueue_init(&m->bfio_queue);

	if(m->cfg->use_bfio)
	{
		wtk_thread_init(&m->enspick_t, (thread_route_handler)qtk_mod_consist_enspick_entry, m);
		wtk_blockqueue_init(&m->enspick_queue);
	}

	qtk_mod_log_wav_file_new(m);
	
	m->consist = wtk_consist_new(cfg->consist_cfg);
	if(ret!=0)
	{
		wtk_log_err(m->log, "consist new fiald! ret=%d\n", ret);
		goto end;
	}
	wtk_consist_set_notify(m->consist, m, (wtk_consist_notify_f)qtk_mod_consist_on_type);

	if(m->cfg->use_bfio)
	{
		m->vboxebf_cfg=qtk_vboxebf_cfg_new("/usr/bin/qdreamer/qsound/res/vboxebf/cfg");
		if(m->vboxebf_cfg)
		{
			m->vboxebf = qtk_vboxebf_new(m->vboxebf_cfg);
			if(m->vboxebf)
			{
				qtk_vboxebf_set_notify(m->vboxebf, m, (qtk_vboxebf_notify_f)qtk_mod_consist_on_vboxebf);
				qtk_vboxebf_start(m->vboxebf);
			}
		}
	}

	int i;
	int channel=m->cfg->rcd.channel - m->cfg->rcd.nskip;

	m->err_nil = (int *)wtk_malloc(sizeof(int) * channel);
	m->err_align = (int *)wtk_malloc(sizeof(int) * channel);
	m->err_max = (int *)wtk_malloc(sizeof(int) * channel);
	m->err_corr = (int *)wtk_malloc(sizeof(int) * channel);
	m->err_energy = (int *)wtk_malloc(sizeof(int) * channel);

	wtk_debug("MOD NEW OK!!!\n");
	wtk_log_log0(m->log, "MODULE NEW OK!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mod_consist_delete(m);
      m = NULL;
  	}
	return m;
}

void qtk_mod_consist_delete(qtk_mod_consist_t *m)
{
	wtk_blockqueue_clean(&m->bfio_queue);
	if(m->cfg->use_bfio)
	{
		wtk_blockqueue_clean(&m->enspick_queue);
	}

#ifndef OFFLINE_TEST
	if(m->rcd)
	{
		qtk_record_delete(m->rcd);
	}
	if(m->play)
	{
		qtk_play_delete(m->play);
	}
#endif
	if(m->msg){
			qtk_msg_delete(m->msg);
	}
	
	wtk_sem_clean(&(m->start_sem));

	if(m->consist)
	{
		wtk_consist_delete(m->consist);
	}

	if(m->cfg->use_bfio)
	{
		if(m->vboxebf)
		{
			qtk_vboxebf_delete(m->vboxebf);
			qtk_vboxebf_cfg_delete_bin(m->vboxebf_cfg);
		}
	}

	qtk_mod_log_wav_file_delete(m);

	if(m->err_nil)
	{
		wtk_free(m->err_nil);
	}
	if(m->err_align)
	{
		wtk_free(m->err_align);
	}
	if(m->err_max)
	{
		wtk_free(m->err_max);
	}
	if(m->err_corr)
	{
		wtk_free(m->err_corr);
	}
	if(m->err_energy)
	{
		wtk_free(m->err_energy);
	}

	if(m->mul_path){
		wtk_strbuf_delete(m->mul_path);
	}

	if(m->log){
		wtk_log_delete(m->log);
	}
    wtk_free(m);
}

void qtk_mod_consist_start(qtk_mod_consist_t *m, char *fn)
{
	int i;
	int ret;
	m->is_send = 0;
	m->qform_run = 1;
	wtk_thread_start(&m->qform_t);

	if(m->cfg->use_bfio)
	{
		m->enspick_run =1;
		wtk_thread_start(&m->enspick_t);
	}

#ifndef OFFLINE_TEST
    m->rcd_run = 1;
    wtk_thread_start(&m->rcd_t);
#endif
	
	for(i=0;i<(m->cfg->rcd.channel - m->cfg->rcd.nskip);++i)
	{
		m->err_nil[i]=0;
		m->err_align[i]=0;
		m->err_max[i]=0;
		m->err_corr[i]=0;
		m->err_energy[i]=0;
	}

	wtk_debug("consist start 00!\n");
	if(m->cfg->mode == 1)
	{
		char fname[64]={0};

		for(i=0; i < (m->cfg->rcd.channel - m->cfg->rcd.nskip - 2); ++i)
		{
			snprintf(fname, 64, "/mnt/UDISK/consist/result/mic%d.txt",i+1);
			qtk_mod_consist_writetype(m, fname, i);
		}

		for(i=0; i < 2; ++i)
		{
			snprintf(fname, 64, "/mnt/UDISK/consist/result/spk%d.txt",i+1);
			qtk_mod_consist_writetype(m, fname, i);
		}
	}

	// FILE *ff;
	// char mac[64]={0};
	// char *comm="cat /sys/class/sunxi_info/sys_info |grep \"sunxi_serial\" | awk '{print $3}'";
	// char *result;
	// result = qtk_mod_consist_get_str(m, comm);
	// snprintf(mac, 64, "%.*s/mac",m->cfg->cache_path.len,m->cfg->cache_path.data);
	// ff = fopen(mac,"wb+");
	// fwrite(result, strlen(result), 1, ff);
	// fflush(ff);
	// fclose(ff);
	// wtk_free(result);

	wtk_debug("consist start!\n");

	if(m->cfg->mode >= 1)
	{
#if 0
		short *data;
		char *zerodata;
		data=(short *)wtk_malloc(16000*sizeof(short));
		zerodata = (char *)wtk_malloc(32000*sizeof(char));
		memset(zerodata, 0, 32000);

		int j;
		for(i=0;i<16000;)
		{
			for(j=-16;j<16;++j)
			{
				data[i++]=j;
			}
		}

		int ret=-1;
		qtk_play_start(m->play);
		qtk_play_write(m->play, zerodata, 3200, 1);
		fwrite(zerodata, 3200, 1, cf);

		i=0;
		int pcount=m->recordlen/320000;
		pcount=pcount>>1;
		printf("time:%d\n",pcount);
		while(i < pcount)
		{
			// ret = qtk_play_write(m->play, (char *)data, 32000, 1);
			// if(ret < 0)
			// {
			// 	wtk_debug("play write error %d\n",ret);
			// }
			// sleep(1);
			// usleep(100*1000);
			int j;
			for(j=0;j<32000;)
			{
				fwrite((char *)data, 32*32, 1, cf);
				ret = qtk_play_write(m->play, (char *)data, 32*32, 1);
				if(ret < 0)
				{
					wtk_debug("play write error %d\n",ret);
				}
				usleep(16*1000);
				j+=32*32;
			}
			i++;
		}
		wtk_free(data);
		wtk_free(zerodata);
#else
		char buf[128]={0};
		int card=qtk_play_get_sound_card("UAC1_Gadget");
		printf("card=%d\n",card);
		snprintf(buf, 128, "aplay -D hw:%d,0 -r 16000 -c 1 -f S16_LE %s",card,fn);
		ret = system(buf);
		while(ret < 0)
		{
			ret = system(buf);
		}
#endif
	}
	wtk_sem_acquire(&(m->start_sem), -1);
	wtk_debug("=====================================================\n");
	ret = -1;
	while(ret < 0)
	{
		ret = system("cat /sys/class/dmic/dmic_reg_debug");
	}
	wtk_debug("=====================================================\n");
}

void qtk_mod_consist_stop(qtk_mod_consist_t *m)
{
#ifndef OFFLINE_TEST
	m->rcd_run = 0;
	wtk_thread_join(&m->rcd_t);
#endif
	m->qform_run = 0;
	wtk_blockqueue_wake(&m->bfio_queue);
	wtk_thread_join(&m->qform_t);
	wtk_consist_reset(m->consist);
}


#ifdef OFFLINE_TEST
void qtk_mod_consist_feed(qtk_mod_consist_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node;

	msg_node = qtk_msg_pop_node(m->msg);
	wtk_strbuf_push(msg_node->buf, data, len);
	wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
}
#endif
#ifndef OFFLINE_TEST
int qtk_mod_consist_rcd_entry(qtk_mod_consist_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *buf;
	qtk_msg_node_t *msg_node,*msg_node2;
	int slen=0;
	int mmin=32*(m->cfg->rcd.channel-m->cfg->rcd.nskip)*m->cfg->skip_head_tm;

	// qtk_mod_consist_set_cpu(m, 0);

	while(m->rcd_run){
		buf = qtk_record_read(m->rcd);
		if(buf->pos == 0){
			continue;
		}
		if((slen+buf->pos) <= mmin)
		{
			slen+=buf->pos;
			continue;
		}
		if(m->is_send)
		{
			break;
		}
		if(m->cfg->use_bfio)
		{
			msg_node2 = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node2->buf, buf->data, buf->pos);
			wtk_blockqueue_push(&m->enspick_queue, &msg_node2->qn);
		}

		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, buf->data, buf->pos);
		wtk_blockqueue_push(&m->bfio_queue, &msg_node->qn);
	}
	return 0;
}
#endif

int qtk_mod_consist_qform_entry(qtk_mod_consist_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
	long long sendlen=0;
	int channel=m->cfg->rcd.channel - m->cfg->rcd.nskip;

	// qtk_mod_consist_set_cpu(m, 1);

	while(m->qform_run){
		qn= wtk_blockqueue_pop(&m->bfio_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->bfio_queue.length > 10)
		{
			wtk_debug("record =============>>%d\n",m->bfio_queue.length);
		}
#if 0
		msg_node2 = qtk_msg_pop_node(m->msg);
		msg_node2->type = qtk_mod_consist_BF_vboxebf;
		int i=0;
		int channel = m->cfg->rcd.channel - m->cfg->rcd.nskip;
		while(i < msg_node->buf->pos)
		{
			wtk_strbuf_push(msg_node2->buf, msg_node->buf->data + i, 2);
			i+=2*channel;
		}
		wtk_blockqueue_push(&m->usbaudio_queue, &msg_node2->qn);
#else
		if(m->is_send)
		{
			qtk_msg_push_node(m->msg, msg_node);
			continue;
		}
		if(m->mul)
		{
			wtk_wavfile_write(m->mul, msg_node->buf->data, msg_node->buf->pos);
		}
		if(sendlen <= m->recordlen)
		{
			wtk_consist_feed2(m->consist, (short *)(msg_node->buf->data),(msg_node->buf->pos/channel) >>1, 0);
			sendlen+=msg_node->buf->pos;
		}else{
			m->is_send=1;	
			m->enspick_run=0;
			wtk_blockqueue_wake(&m->enspick_queue);
			wtk_thread_join(&m->enspick_t);
			wtk_consist_feed2(m->consist, (short *)(msg_node->buf->data), (msg_node->buf->pos/channel) >>1, 1);
			wtk_sem_release(&(m->start_sem), 1);
		}
#endif
		qtk_msg_push_node(m->msg, msg_node);
	}
	return 0;
}

int qtk_mod_consist_enspick_entry(qtk_mod_consist_t *m, wtk_thread_t *thread)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;

	while(m->enspick_run){
		qn= wtk_blockqueue_pop(&m->enspick_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->enspick_queue.length > 10)
		{
			wtk_debug("record =============>>%d\n",m->enspick_queue.length);
		}

		qtk_vboxebf_feed(m->vboxebf, msg_node->buf->data, msg_node->buf->pos, 0);

		qtk_msg_push_node(m->msg, msg_node);
	}

	qtk_vboxebf_feed(m->vboxebf, NULL, 0, 1);
	qtk_vboxebf_reset(m->vboxebf);
}

void qtk_mod_consist_writetype(qtk_mod_consist_t *m, char *fname,int channel)
{
	char buf[30]={0};
	FILE *ff=NULL;

	while(ff == NULL)
	{
		ff = fopen(fname,"wb+");
	}
	snprintf(buf, 30, "%d %d %d %d %d", m->err_nil[channel], m->err_align[channel], m->err_max[channel], m->err_corr[channel], m->err_energy[channel]);
	if(ff)
	{
		fwrite(buf, strlen(buf), 1, ff);
		fflush(ff);
		fclose(ff);
	}
}

void qtk_mod_consist_on_vboxebf(qtk_mod_consist_t *m, char *data, int len)
{

}

void qtk_mod_consist_on_type(qtk_mod_consist_t *m, wtk_consist_micerror_type_t type, int channel)
{
	char fname[50]={0};
	int schannel=0;

	switch (type)
	{
	case WTK_CONSIST_MICERR_NIL:
		schannel=channel - 1;
		m->err_nil[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/mic%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_MICERR_NIL==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_ALIGN:
		schannel=channel - 1;
		m->err_align[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/mic%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_MICERR_ALIGN==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_MAX:
		schannel=channel - 1;
		m->err_max[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/mic%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_MICERR_MAX==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_CORR:
		schannel=channel - 1;
		m->err_corr[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/mic%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_MICERR_CORR==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_MICERR_ENERGY:
		schannel=channel - 1;
		m->err_energy[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/mic%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_MICERR_ENERGY==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_NIL:
		schannel=channel + m->cfg->consist_cfg->channel - 1;
		m->err_nil[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/spk%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_SPKERR_NIL==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_ALIGN:
		schannel=channel + m->cfg->consist_cfg->channel - 1;
		m->err_align[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/spk%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_SPKERR_ALIGN==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_MAX:
		schannel = channel + m->cfg->consist_cfg->channel - 1;
		m->err_max[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/spk%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_SPKERR_MAX==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_CORR:
		schannel = channel + m->cfg->consist_cfg->channel - 1;
		m->err_corr[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/spk%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_SPKERR_CORR==================+>>>>%d\n",channel);
		break;
	case WTK_CONSIST_SPKERR_ENERGY:
		schannel = channel + m->cfg->consist_cfg->channel - 1;
		m->err_energy[schannel]=1;
		snprintf(fname, 50, "/mnt/UDISK/consist/result/spk%d.txt",channel);
		wtk_debug("=========WTK_CONSIST_SPKERR_ENERGY==================+>>>>%d\n",channel);
		break;
	default:
		break;
	}
	qtk_mod_consist_writetype(m, fname, schannel);
}


char *qtk_mod_consist_get_str(qtk_mod_consist_t *m, char *comm)
{
	char *result;
	char *ddff=comm;

	result = (char *)wtk_malloc(1024);

	FILE *fp=popen(ddff, "r");
	if(fp < 0)
	{
		wtk_debug("read df fiald.\n");
	}

	while(fgets(result, 1024, fp) != NULL)
	{
		if('\n' == result[strlen(result)-1])
		{
			result[strlen(result)-1] = '\0';
		}
	}
	fclose(fp);
	return result;
}

int qtk_mod_consist_get_int(qtk_mod_consist_t *m, char *comm)
{
	char *result=qtk_mod_consist_get_str(m, comm);
	int ret = wtk_str_atoi2(result, strlen(result), NULL);
	wtk_free(result);
	return ret;
}

static void qtk_mod_log_wav_file_new(qtk_mod_consist_t *m)
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

	m->minlen = qtk_mod_consist_get_int(m, "df | grep /dev/by-name/UDISK | awk '{print $4}'");
}
static void qtk_mod_log_wav_file_delete(qtk_mod_consist_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);		
	m->mul = NULL;
}

void qtk_mod_consist_set_cpu(qtk_mod_consist_t *m, int cpunum)
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
}
