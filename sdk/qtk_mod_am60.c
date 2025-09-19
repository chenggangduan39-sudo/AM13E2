#include "qtk_mod_am60.h"
#include "sdk/qtk_api.h"
#include <time.h>
#include <pthread.h>
#ifdef USE_KEROS
#include "sdk/codec/keros/qtk_keros.h"
#endif
// #define ONLAY_PLAY
// #define ONLAY_RECORD

static long rlen=0;
static long plen=0;
static long qlen=0;
static int once=0;
static int once2=0;
static wtk_thread_t mem_t;
static wtk_thread_t mem2_t;
static qtk_proc_t  mem_proc;

#ifndef OFFLINE_TEST
int qtk_mod_am60_micrcd_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
int qtk_mod_am60_spkrcd_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
#endif
#ifdef USE_FOR_DEV
int qtk_mod_am60_uart_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
#endif
#ifndef __ANDROID__
int qtk_mod_am60_usbaudio_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
int qtk_mod_am60_lineout_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
#endif
int qtk_mod_am60_qform_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
int qtk_mod_am60_qform2_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
int qtk_mod_am60_qform3_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
int qtk_mod_am60_savepcm_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
int qtk_mod_am60_marge_entry(qtk_mod_am60_t *m, wtk_thread_t *t);
void qtk_mod_am60_on_vboxebf(qtk_mod_am60_t *mod, char *data, int len);
#ifdef USE_SSL
void qtk_mod_am60_on_ssl(qtk_mod_am60_t *mod,qtk_ssl_extp_t *extp);
#endif
void qtk_mod_am60_on_aec(qtk_mod_am60_t *m, qtk_var_t *var);
static void qtk_log_wav_file_new(qtk_mod_am60_t *m);
static void qtk_log_wav_file_delete(qtk_mod_am60_t *m);
static void qtk_is_log_audio(qtk_mod_am60_t *m);
void qtk_mod_am60_set_cpu(qtk_mod_am60_t *m, wtk_thread_t *thread, int cpunum);
void qtk_mod_am60_on_resample(qtk_mod_am60_t *m, char *data, int len);

int qtk_mod_am60_engine_new(qtk_mod_am60_t *m, int type);
void qtk_mod_am60_engine_delete(qtk_mod_am60_t *m, int type);
void qtk_mod_am60_engine_start(qtk_mod_am60_t *m, int type);
void qtk_mod_am60_engine_stop(qtk_mod_am60_t *m, int type);
int qtk_mod_am60_engine_feed(qtk_mod_am60_t *m, int type, char *data, int len, int is_end);

#ifdef USE_3308
int qtk_mod_am60_proc_send_pid(qtk_mod_am60_t *m);
int qtk_mod_am60_proc_read(qtk_mod_am60_t *m);
int qtk_mod_am60_proc_write(qtk_mod_am60_t *m,char *data,int len);

// void myfunc(int sig);
static void myfunc(int sig, siginfo_t *info, void *ptr);
static void myfunc2(int sig, siginfo_t *info, void *ptr);
#endif	

int qtk_mod_am60_msg_bytes(qtk_msg_node_t *node)
{
	int bytes=0;
	bytes+=wtk_strbuf_bytes(node->buf);
	return bytes;
}

int qtk_mod_am60_bytes(qtk_mod_am60_t *m)
{
	int bytes=0;
	int tmp=0;
	bytes+=wtk_strbuf_bytes(m->check_path_buf);
	bytes+=wtk_strbuf_bytes(m->mul_path);
	bytes+=wtk_strbuf_bytes(m->single_path);
	bytes+=wtk_strbuf_bytes(m->playbuf);
	// if(m->cfg->audio_type == qtk_mod_am60_BF_VBOXEBF)
	// {
	// 	bytes+=qtk_vboxebf_bytes(m->vboxebf);
	// }
#ifndef OFFLINE_TEST
	if(m->micrcd)
	{
		bytes+=wtk_strbuf_bytes(m->micrcd->buf);
	}
#endif
	tmp=wtk_hoard_bytes((wtk_hoard_t *)(&(m->msg->msg_hoard)),(wtk_hoard_bytes_f)qtk_mod_am60_msg_bytes);
	bytes+=tmp;
	bytes+=sizeof(qtk_mod_am60_t);
	wtk_log_log(m->log, "all=%d bytes=%d hoard_length=%d/%d\n",bytes,tmp,m->msg->msg_hoard.use_length,m->msg->msg_hoard.cur_free);
	return bytes;
}

qtk_mod_am60_t *qtk_mod_am60_new(qtk_mod_am60_cfg_t *cfg)
{
	qtk_mod_am60_t *m;
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

	m = (qtk_mod_am60_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;

	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
	m->single_path = wtk_strbuf_new(64,0);
#ifndef OFFLINE_TEST
	wtk_sem_init(&(m->micrcd_sem),0);
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
		wtk_thread_init(&m->savepcm_t,(thread_route_handler)qtk_mod_am60_savepcm_entry, m);
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
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mod_am60_usbaudio_entry, m);
		m->usbaudio = qtk_play_new(&cfg->usbaudio);
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mod_am60_lineout_entry, m);
		m->lineout = qtk_play2_new(&cfg->lineout);
	}
#endif
#endif

	m->resample = wtk_resample_new(960);
	wtk_resample_set_notify(m->resample, m, (wtk_resample_notify_f)qtk_mod_am60_on_resample);
#ifndef OFFLINE_TEST
#ifndef ONLAY_PLAY
	m->micrcd =qtk_record_new(&cfg->micrcd);
//	if(!m->micrcd){
//		wtk_log_err0(m->log, "record fiald!");
//		ret = -1;
//    	goto end;
//	}
	wtk_thread_init(&m->micrcd_t, (thread_route_handler)qtk_mod_am60_micrcd_entry, m);

	m->spkrcd =qtk_record_new(&cfg->spkrcd);
//	if(!m->micrcd){
//		wtk_log_err0(m->log, "record fiald!");
//		ret = -1;
//    	goto end;
//	}
	wtk_thread_init(&m->spkrcd_t, (thread_route_handler)qtk_mod_am60_spkrcd_entry, m);
#endif
#endif

#ifdef USE_LED
	m->led = qtk_led_new(&m->cfg->led);
	if(m->led)
	{
		qtk_led_send_cmd(m->led, QTK_LED_TYPE_LED2, QTK_LED_COLOR_GREEN, QTK_LED_CMD_ON);
	}
#endif

	m->msg = qtk_msg_new();
	wtk_thread_init(&m->qform_t,(thread_route_handler)qtk_mod_am60_qform_entry, m);
	wtk_blockqueue_init(&m->bfio_queue);
	if(m->cfg->audio2_type > 0)
	{
		wtk_thread_init(&m->qform2_t,(thread_route_handler)qtk_mod_am60_qform2_entry, m);
		wtk_blockqueue_init(&m->bfio2_queue);
	}
	if(m->cfg->audio3_type > 0)
	{
		wtk_thread_init(&m->qform3_t,(thread_route_handler)qtk_mod_am60_qform3_entry, m);
		wtk_blockqueue_init(&m->bfio3_queue);
	}
	wtk_thread_init(&m->marge_t, (thread_route_handler)qtk_mod_am60_marge_entry, m);
	wtk_blockqueue_init(&m->marge_queue);

	m->in_resample = NULL;
	if(m->cfg->use_in_resample)
	{
		m->in_resample = speex_resampler_init(m->cfg->micrcd.channel-m->cfg->micrcd.nskip, 48000, 16000, SPEEX_RESAMPLER_QUALITY_DESKTOP, NULL);
	}
	ret = qtk_mod_am60_engine_new(m,m->cfg->audio_type);
	if(ret!=0)
	{
		wtk_log_err(m->log, "audio engine new fiald! type=%d", m->cfg->audio_type);
		goto end;
	}
	if(m->cfg->audio2_type > 0)
	{
		ret = qtk_mod_am60_engine_new(m,m->cfg->audio2_type);
		if(ret!=0)
		{
			wtk_log_err(m->log, "audio2 engine new fiald! type=%d", m->cfg->audio2_type);
			goto end;
		}
	}
	if(m->cfg->audio3_type > 0)
	{
		ret = qtk_mod_am60_engine_new(m,m->cfg->audio3_type);
		if(ret!=0)
		{
			wtk_log_err(m->log, "audio3 engine new fiald! type=%d", m->cfg->audio3_type);
			goto end;
		}
	}

#ifdef USE_FOR_DEV
	if(m->cfg->use_uart)
	{
		wtk_blockqueue_init(&m->uart_queue);
		wtk_thread_init(&m->uart_t, (thread_route_handler)qtk_mod_am60_uart_entry, m);
		m->uart_cfg = wtk_main_cfg_new_type(qtk_uart_cfg,m->cfg->uart_fn.data);
		m->uart = qtk_uart_new((qtk_uart_cfg_t *)(m->uart_cfg->cfg));
	}
#endif
	m->is_player_start=0;
	m->player_run=0;

#ifdef USE_3308
	mem_proc.shmaddr=NULL;
	// 创建一个提取码
#ifdef USE_KTC3308
    if ((mem_proc.key = ftok("/ktc/version.txt", 'z')) == -1)
    {
#else
    if ((mem_proc.key = ftok("/oem/qdreamer/qsound/version.txt", 'z')) == -1)
    {
#endif
        perror("ftok");
        return -1;
    }
    printf("===>>>key %d\n",mem_proc.key);
    // 创建一个共享空间
    if ((mem_proc.shmid = shmget(mem_proc.key, SIZE, IPC_CREAT | 0666)) == -1)
    {
        perror("shmget");
        return -1;
    }
    printf("===>>>shmid %d\n",mem_proc.shmid);
    // 设置共享空间地址
    if ((mem_proc.shmaddr = shmat(mem_proc.shmid, NULL, 0)) == (SHM *)-1)
    {
        perror("shmat");
        return -1;
    }
    // 注册信号
	wtk_thread_init(&mem_t,(thread_route_handler)qtk_mod_am60_proc_read, m);
	wtk_thread_init(&mem2_t,(thread_route_handler)qtk_mod_am60_proc_send_pid, m);
	// signal(SIGUSR1,myfunc);
	struct sigaction act, oact;
	act.sa_sigaction = myfunc; //指定信号处理回调函数
	sigemptyset(&act.sa_mask); // 阻塞集为空
	act.sa_flags = SA_SIGINFO; // 指定调用 signal_handler
 // 注册信号 SIGINT
	sigaction(SIGRTMIN, &act, &oact);
	// signal(SIGUSR2,myfunc2);
	struct sigaction act2, oact2;
	act2.sa_sigaction = myfunc2; //指定信号处理回调函数
	sigemptyset(&act2.sa_mask); // 阻塞集为空
	act2.sa_flags = SA_SIGINFO; // 指定调用 signal_handler
 // 注册信号 SIGINT
	sigaction(SIGRTMIN+2, &act2, &oact2);
	mem_proc.shmaddr->pid = getpid();
#endif

	wtk_log_log0(m->log, "MODULE NEW OK!");
	wtk_debug("MOD NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mod_am60_delete(m);
      m = NULL;
  	}
	return m;
}

void qtk_mod_am60_delete(qtk_mod_am60_t *m)
{
	wtk_blockqueue_clean(&m->bfio_queue);
	if(m->cfg->audio2_type > 0)
	{
		wtk_blockqueue_clean(&m->bfio2_queue);
	}
	if(m->cfg->audio3_type > 0)
	{
		wtk_blockqueue_clean(&m->bfio3_queue);
	}

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	wtk_sem_clean(&(m->micrcd_sem));
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
	if(m->micrcd){
			qtk_record_delete(m->micrcd);
	}
	if(m->spkrcd){
			qtk_record_delete(m->spkrcd);
	}
#endif
	if(m->msg){
			qtk_msg_delete(m->msg);
	}
#ifdef USE_LED
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

	qtk_mod_am60_engine_delete(m, m->cfg->audio_type);

	if(m->cfg->audio2_type > 0)
	{
		qtk_mod_am60_engine_delete(m, m->cfg->audio2_type);
	}
	if(m->cfg->audio3_type > 0)
	{
		qtk_mod_am60_engine_delete(m, m->cfg->audio3_type);
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
	if(m->log){
		wtk_log_delete(m->log);
	}
    wtk_free(m);
}

void qtk_mod_am60_start(qtk_mod_am60_t *m)
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

#ifndef USE_3308
	if(m->cfg->use_resample)
	{
		wtk_resample_start(m->resample, 16000, m->cfg->resample_rate);
	}
#endif

	qtk_mod_am60_engine_start(m, m->cfg->audio_type);
	m->qform_run = 1;
	wtk_thread_start(&m->qform_t);

	if(m->cfg->audio2_type > 0)
	{
		qtk_mod_am60_engine_start(m, m->cfg->audio2_type);
		m->qform2_run = 1;
		wtk_thread_start(&m->qform2_t);
	}
	if(m->cfg->audio3_type > 0)
	{
		qtk_mod_am60_engine_start(m, m->cfg->audio3_type);
		m->qform3_run = 1;
		wtk_thread_start(&m->qform3_t);
	}

	m->marge_run = 1;
	wtk_thread_start(&m->marge_t);

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
#ifdef USE_3308
	wtk_thread_start(&mem2_t);
#endif

#ifndef OFFLINE_TEST
#ifndef ONLAY_PLAY
    m->micrcd_run = 1;
    wtk_thread_start(&m->micrcd_t);

    m->spkrcd_run = 1;
    wtk_thread_start(&m->spkrcd_t);
#endif
#endif
}

void qtk_mod_am60_stop(qtk_mod_am60_t *m)
{
#ifndef OFFLINE_TEST
	m->micrcd_run = 0;
	wtk_thread_join(&m->micrcd_t);
	m->spkrcd_run = 0;
	wtk_thread_join(&m->spkrcd_t);
#endif

	m->qform_run = 0;
	wtk_blockqueue_wake(&m->bfio_queue);
	wtk_thread_join(&m->qform_t);
	qtk_mod_am60_engine_stop(m, m->cfg->audio_type);

	if(m->cfg->audio2_type > 0)
	{
		m->qform2_run = 0;
		wtk_blockqueue_wake(&m->bfio2_queue);
		wtk_thread_join(&m->qform2_t);
		qtk_mod_am60_engine_stop(m, m->cfg->audio2_type);
	}
	if(m->cfg->audio3_type > 0)
	{
		m->qform3_run = 0;
		wtk_blockqueue_wake(&m->bfio3_queue);
		wtk_thread_join(&m->qform3_t);
		qtk_mod_am60_engine_stop(m, m->cfg->audio3_type);
	}
	m->marge_run = 0;
	wtk_blockqueue_wake(&m->marge_queue);
	wtk_thread_join(&m->marge_t);

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

#ifndef USE_3308
	if(m->cfg->use_resample)
	{
		wtk_resample_close(m->resample);
	}
#endif

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
#ifdef USE_3308
	wtk_thread_join(&mem2_t);
#endif
}

void qtk_mod_am60_start2(qtk_mod_am60_t *m, int sample_rate)
{
	if(sample_rate != 16000)
	{
		m->cfg->use_resample=1;
	}else{
		m->cfg->use_resample=0;
	}
	m->cfg->usbaudio.sample_rate=sample_rate;
	wtk_debug("================================>>>>>>>>>>>>>>>rate=%d\n",m->cfg->usbaudio.sample_rate);
	if(m->cfg->use_resample)
	{
		if(m->cfg->use_log_wav || m->log_audio)
		{
			if(m->single)
			{
				wtk_wavfile_close(m->single);
				wtk_wavfile_delete(m->single);
				m->single=NULL;
			}
			int channel;
			m->single = wtk_wavfile_new(sample_rate); 
			m->single->max_pend = 0;
			channel = m->cfg->usbaudio.channel;
			wtk_wavfile_set_channel2(m->single, channel, 2);
			wtk_wavfile_open(m->single, m->single_path->data);
		}
#if (defined USE_3308)
		wtk_resample_start(m->resample, 16000, sample_rate);
#else
		wtk_resample_start(m->resample, 16000, m->cfg->resample_rate);
#endif
	}
	m->player_run=1;
}

void qtk_mod_am60_stop2(qtk_mod_am60_t *m)
{
	m->player_run=0;
	if(m->cfg->use_resample)
	{
		wtk_resample_close(m->resample);
	}
	m->is_player_start=0;
}

void qtk_mod_am60_clean_queue2(qtk_mod_am60_t *m, wtk_blockqueue_t *queue,int nx)
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

static int wtk_qmode_check_sil_queue(wtk_blockqueue_t *q)
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

int  qtk_mod_am60_clean_sil_queue(qtk_mod_am60_t *m, wtk_blockqueue_t *q)
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


void qtk_mod_am60_clean_queue(qtk_mod_am60_t *m, wtk_blockqueue_t *queue)
{
	qtk_mod_am60_clean_queue2(m,queue,0);
}

void qtk_mod_am60_clean_front_queue(qtk_mod_am60_t *m, wtk_blockqueue_t *queue,int count)
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
int qtk_mod_am60_uart_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	char *out=(char *)wtk_malloc(10);
	int olen;
	int i;
	qtk_mod_am60_set_cpu(m, t, 0);
    
	while(m->uart_run){
		qn= wtk_blockqueue_pop(&m->uart_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		switch (msg_node->type)
		{
		case qtk_mod_am60_BF_ASR_TXT:
			qtk_comm_get_number(msg_node->buf->data, msg_node->buf->pos,out, &olen);
			if(m->uart)
			{
				qtk_uart_write(m->uart, out, olen);
			}
			break;
		case qtk_mod_am60_BF_DIRECTION:
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
void qtk_mod_am60_feed(qtk_mod_am60_t *m, char *data, int len)
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
int qtk_mod_am60_micrcd_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node, *msg_node2;
	int mchannel=m->cfg->micrcd.channel - m->cfg->micrcd.nskip - m->cfg->spknum;
	int head_cnt=m->cfg->skip_head_tm * 32 * (m->cfg->micrcd.channel - m->cfg->micrcd.nskip);
	int skip_len=0;
	int count=0;
	double tm=0.0;

	qtk_mod_am60_set_cpu(m, t, 1);

	while(m->micrcd_run){
		// tm = time_get_ms();
		rbuf = qtk_record_read(m->micrcd);
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

		count++;
		if(count == 50 && m->cfg->use_log_wav==0)
		{
			qtk_is_log_audio(m);
			count=0;
		}

		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 1;
		wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
		wtk_blockqueue_push(&m->marge_queue, &msg_node->qn);

	}
	return 0;
}

int qtk_mod_am60_spkrcd_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node, *msg_node2;
	int mchannel=m->cfg->spkrcd.channel - m->cfg->spkrcd.nskip - m->cfg->spknum;
	int head_cnt=m->cfg->skip_head_tm * 32 * (m->cfg->spkrcd.channel - m->cfg->spkrcd.nskip);
	int skip_len=0;
	int count=0;
	double tm=0.0;

	qtk_mod_am60_set_cpu(m, t, 1);

	while(m->spkrcd_run){
		// tm = time_get_ms();
		rbuf = qtk_record_read(m->spkrcd);
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

		if(m->cfg->spk_shift != 1.0)
		{
			qtk_data_change_vol(rbuf->data, rbuf->pos, m->cfg->spk_shift);
		}

		count++;
		if(count == 50 && m->cfg->use_log_wav==0)
		{
			qtk_is_log_audio(m);
			count=0;
		}

		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 2;
		wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
		wtk_blockqueue_push(&m->marge_queue, &msg_node->qn);
	}
	return 0;
}
#endif

int qtk_mod_am60_marge_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
	int mchannel=m->cfg->micrcd.channel - m->cfg->micrcd.nskip;
	int schannel=m->cfg->spkrcd.channel - m->cfg->spkrcd.nskip;
	int mlen=mchannel * 1024;
	int slen=schannel * 1024;
	int tlen=0,i=0;
	wtk_strbuf_t *mbuf = wtk_strbuf_new(1024*16, 1.0);
	wtk_strbuf_t *sbuf = wtk_strbuf_new(1024*2, 1.0);

	qtk_mod_am60_set_cpu(m, t, 1);

	wtk_strbuf_reset(mbuf);
	wtk_strbuf_reset(sbuf);

	while(m->marge_run){
		qn= wtk_blockqueue_pop(&m->marge_queue, -1, NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug)
		{
			if(m->marge_queue.length > 2)
			{
				wtk_debug("marge_queue =============>>%d\n",m->marge_queue.length);
				wtk_log_log(m->log, "marge_queue =============>>%d",m->marge_queue.length);
			}
		}
		
		if(msg_node->type == 1)
		{
			wtk_strbuf_push(mbuf, msg_node->buf->data, msg_node->buf->pos);
		}else if(msg_node->type == 2){
			wtk_strbuf_push(sbuf, msg_node->buf->data, msg_node->buf->pos);
		}
		if(mbuf->pos >= mlen && sbuf->pos >= slen)
		{
			msg_node2 = qtk_msg_pop_node(m->msg);
			tlen=min(mlen/mchannel, slen/schannel);
			i=0;
			while(i < tlen)
			{
				wtk_strbuf_push(msg_node2->buf, mbuf->data+(i*mchannel), 2*mchannel);
				wtk_strbuf_push(msg_node2->buf, sbuf->data+(i*schannel), 2*schannel);
				i+=2;
			}
			wtk_blockqueue_push(&m->bfio_queue, &msg_node2->qn);
			wtk_strbuf_pop(mbuf, NULL, i*mchannel);
			wtk_strbuf_pop(sbuf, NULL, i*schannel);
		}

		qtk_msg_push_node(m->msg, msg_node);
	}

	wtk_strbuf_delete(mbuf);
	wtk_strbuf_delete(sbuf);
	return 0;
}

int qtk_mod_am60_qform_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *msg_node2;
	wtk_queue_node_t *qn;
	int mmcount=0;
	int inlen;
	int mchannel=m->cfg->micrcd.channel - m->cfg->micrcd.nskip;
	int schannel=m->cfg->spkrcd.channel - m->cfg->spkrcd.nskip;
	int outlen=m->cfg->micrcd.sample_rate*m->cfg->micrcd.buf_time*(mchannel+schannel)*2/1000;
	char *outresample=(char *)wtk_malloc(outlen);

	// wtk_strbuf_reset(m->playbuf);

	qtk_mod_am60_set_cpu(m, t, 3);

	while(m->qform_run){
		if(m->bfio_queue.length > m->cfg->max_add_count)
		{
#ifndef USE_3308
			wtk_debug( "qform clean=============>>%d\n",m->bfio_queue.length);
			wtk_log_log(m->log, "qform clean=============>>%d",m->bfio_queue.length);
#endif
			qtk_mod_am60_clean_queue(m, &m->bfio_queue);
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
			// 	qtk_mod_am60_bytes(m);
			// }
		}
		// if(++mmcount == 100)
		// {
		// 	mmcount=0;
		// 	wtk_debug("====================>>>>>bfio_queue lenght=%d\n",m->bfio_queue.length);
		// }

		// wtk_debug("=====================L>>>>>>>>>>>outlen=%d pos=%d\n",outlen,msg_node->buf->pos);
		memset(outresample, 0, msg_node->buf->pos);
		inlen=(msg_node->buf->pos >> 1)/(mchannel+schannel);
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
			msg_node2->type = qtk_mod_am60_BF_SAVE_SOURCE;
			if(m->in_resample)
			{
				wtk_strbuf_push(msg_node2->buf, outresample, (outlen<<1)*(m->cfg->micrcd.channel-m->cfg->micrcd.nskip));
			}else{
				wtk_strbuf_push(msg_node2->buf, msg_node->buf->data, msg_node->buf->pos);
			}
			wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
		}
		//wtk_debug("===========> record msg=%d\n",msg_node->buf->pos);
#if 0
		if(m->usbaudio_queue.length >30)
		{
			wtk_debug("=======================>>>>>>>>>>>>%d\n",m->usbaudio_queue.length);
			qtk_mod_am60_clean_queue(m, &(m->usbaudio_queue));
		}
		msg_node2 = qtk_msg_pop_node(m->msg);
		msg_node2->type = qtk_mod_am60_BF_VBOXEBF;
		int i=0,j=0;
		int channel = m->cfg->micrcd.channel - m->cfg->micrcd.nskip;
		while(i < msg_node->buf->pos)
		{
			for(j=0;j<m->cfg->usbaudio.channel;++j)
			{
				wtk_strbuf_push(msg_node2->buf, msg_node->buf->data + i + j*2, 2);
			}
			// wtk_strbuf_push(msg_node2->buf, msg_node->buf->data + i+2, 2);
			i+=2*channel;
		}
		wtk_blockqueue_push(&m->usbaudio_queue, &msg_node2->qn);
#else
#ifndef ONLAY_RECORD
		// int i=0,j=0;
		// int channel = m->cfg->micrcd.channel - m->cfg->micrcd.nskip;
		// while(i < msg_node->buf->pos)
		// {
		// 	for(j=0;j<m->cfg->usbaudio.channel;++j)
		// 	{
		// 		wtk_strbuf_push(m->playbuf, msg_node->buf->data + i + j*2, 2);
		// 	}
		// 	i+=2*channel;
		// }
		if(m->in_resample)
		{
			qtk_mod_am60_engine_feed(m, m->cfg->audio_type, outresample, (outlen<<1)*(m->cfg->micrcd.channel-m->cfg->micrcd.nskip), 0);
		}else{
			qtk_mod_am60_engine_feed(m, m->cfg->audio_type, msg_node->buf->data, msg_node->buf->pos, 0);
		}
#endif
#endif
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mod_am60_engine_feed(m, m->cfg->audio_type, NULL, 0, 1);
	if(outresample)
	{
		wtk_free(outresample);
	}
	return 0;
}

int qtk_mod_am60_qform2_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;

	qtk_mod_am60_set_cpu(m, t, 0);

	while(m->qform2_run){
		qn= wtk_blockqueue_pop(&m->bfio2_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug)
		{
			if(m->bfio2_queue.length > 2)
			{
				wtk_debug("qform =============>>%d\n",m->bfio2_queue.length);
				wtk_log_log(m->log, "qform =============>>%d",m->bfio2_queue.length);
			}
		}

		// if(m->cfg->use_log_wav || m->log_audio)
		// {
		// 	msg_node2 = qtk_msg_pop_node(m->msg);
		// 	msg_node2->type = qtk_mod_am60_BF_SAVE_SOURCE;
		// 	wtk_strbuf_push(msg_node2->buf, msg_node->buf->data, msg_node->buf->pos);
		// 	wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
		// }
		
		qtk_mod_am60_engine_feed(m, m->cfg->audio2_type, msg_node->buf->data, msg_node->buf->pos, 0);

		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mod_am60_engine_feed(m, m->cfg->audio2_type, NULL, 0, 1);

	return 0;
}

int qtk_mod_am60_qform3_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;

	qtk_mod_am60_set_cpu(m, t, 0);

	while(m->qform3_run){
		qn= wtk_blockqueue_pop(&m->bfio3_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug)
		{
			if(m->bfio3_queue.length > 2)
			{
				wtk_debug("qform =============>>%d\n",m->bfio3_queue.length);
				wtk_log_log(m->log, "qform =============>>%d",m->bfio3_queue.length);
			}
		}
		
		qtk_mod_am60_engine_feed(m, m->cfg->audio3_type, msg_node->buf->data, msg_node->buf->pos, 0);

		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_mod_am60_engine_feed(m, m->cfg->audio3_type, NULL, 0, 1);

	return 0;
}

#include "wtk/core/wtk_os.h"

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
int qtk_mod_am60_usbaudio_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
	int first = 1;
	long ret;
	double wtm=0;
	int type;
	wtk_blockqueue_t *usbaudio_queue;
	int zlen=100*m->cfg->usbaudio.channel*m->cfg->usbaudio.sample_rate/1000*2;
	char *zerodata = wtk_malloc(zlen);
	double tm=0.0;

	qtk_mod_am60_set_cpu(m, t, 2);

	usbaudio_queue=&(m->usbaudio_queue);

  	while(m->usbaudio_run){
		qn= wtk_blockqueue_pop(usbaudio_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

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


#if (defined USE_3308)
		if(m->player_run)
#endif
		{
			if(first)
			{
				qtk_play_start(m->usbaudio);
				if(m->cfg->audio_type == qtk_mod_am60_BF_BFIO)
				{
					while(m->usbaudio_queue.length < 5)
					{
						usleep(4*1000);
					}
				}
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

			type=msg_node?msg_node->type:qtk_mod_am60_BF_VBOXEBF;
			switch(type){
			case qtk_mod_am60_BF_VBOXEBF:
				if(m->cfg->use_log_wav || m->log_audio)
				{
					msg_node2 = qtk_msg_pop_node(m->msg);
					msg_node2->type = qtk_mod_am60_BF_SAVE_SPK;
					wtk_strbuf_push(msg_node2->buf, msg_node->buf->data, msg_node->buf->pos);
					wtk_blockqueue_push(&m->savepcm_queue, &msg_node2->qn);
				}
				// if(m->cfg->audio_type == qtk_mod_am60_BF_BFIO)
				// {
				// 	tm = time_get_ms() - tm;
				// 	// wtk_debug("=====================>>>>>>>>>>>>>time=%f length=%d\n",tm,m->usbaudio_queue.length);
				// 	if(tm <= 32.0 && tm > 0.0)
				// 	{
				// 		// if(m->usbaudio_queue.length > 10)
				// 		// {
				// 			// if(m->usbaudio_queue.length > 15)
				// 			// {
				// 			// 	usleep((30.0 - tm)*1000);
				// 			// }else
				// 			// {
				// 			// 	usleep((31.0 - tm)*1000);
				// 			// }
				// 			// posoffset=2*2;
				// 		// }
				// 		// wtk_debug("======>>>>>>>>>>>>>>>>>>>>.tm=%f\n",tm);
				// 		usleep((32.0 - tm)*1000);
				// 	}
				// 	tm = time_get_ms();
				// }
				ret = qtk_play_write(m->usbaudio, msg_node->buf->data, msg_node->buf->pos, 1);
				break;
			default:
				break;
			}
		}
#if (defined USE_3308)
		if(m->is_player_start == 0 && first == 0)
		{
			qtk_play_stop(m->usbaudio);
			first=1;
		}
#endif

		if(msg_node)
		{
			qtk_msg_push_node(m->msg, msg_node);
		}
	}
	wtk_free(zerodata);
	qtk_play_stop(m->usbaudio);
  	return 0;
}

int qtk_mod_am60_lineout_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int first = 1;

	qtk_mod_am60_set_cpu(m, t, 2);
    
	while(m->lineout_run){
		qn= wtk_blockqueue_pop(&m->lineout_queue,-1,NULL);
		if(!qn) {
			continue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
#ifndef USE_3308
		if(m->lineout_queue.length > 10)
		{
			wtk_debug("lineout =============>>%d\n",m->lineout_queue.length);
			wtk_log_log(m->log, "lineout =============>>%d",m->lineout_queue.length);
		}
#endif
		if(first == 1){
			qtk_play2_start(m->lineout);
			first = 0;
		}

		if(m->cfg->lineout_shift != 1.0)
		{
			qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->lineout_shift);
		}
		qtk_play2_write(m->lineout, msg_node->buf->data, msg_node->buf->pos);
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_play2_stop(m->lineout);
	return 0;
}
#endif
#endif

int qtk_mod_am60_savepcm_entry(qtk_mod_am60_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
#ifdef USE_R328
	m->slen=0;
#endif

	qtk_mod_am60_set_cpu(m, t, 0);
    
	while(m->savepcm_run){
		qn= wtk_blockqueue_pop(&m->savepcm_queue,-1,NULL);
		if(!qn) {
			break;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
#ifndef USE_3308
		if(m->savepcm_queue.length > 10  && m->cfg->debug)
		{
			wtk_debug("savepcm =============>>%d\n",m->savepcm_queue.length);
			wtk_log_log(m->log, "savepcm ===============>>%d",m->savepcm_queue.length);
		}
#endif

// #ifdef USE_R328
// 		if(m->slen < (m->minlen-8192)*1024)
// 		{
// 			m->slen+=msg_node->buf->pos;
// 		}else{
// 			qtk_msg_push_node(m->msg, msg_node);
// 			continue;
// 		}
// #endif

		switch (msg_node->type)
		{
		case qtk_mod_am60_BF_SAVE_SOURCE:
			if(m->mul){
				wtk_wavfile_write(m->mul, msg_node->buf->data, msg_node->buf->pos);
			}
			break;
		case qtk_mod_am60_BF_SAVE_SPK:
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

// 保留整数的低8位
unsigned char qtk_mod_am60_keep_low_byte(int value) {
    return (unsigned char)(value & 0xFF);
}

// 保留整数的高8位
unsigned char qtk_mod_am60_keep_high_byte(int value) {
    return (unsigned char)((value >> 8) & 0xFF);
}

// double samtm=0.0;

void qtk_mod_am60_on_resample(qtk_mod_am60_t *m, char *data, int len)
{
	qtk_msg_node_t *msg_node, *msg_node2, *msg_node3;
	char *ss;
	int nx=0;

	// samtm = time_get_ms() - samtm;
	// wtk_debug("=====================>>>>>>>>>>>>>>>>>>>>>>>>>>>resample tm=%f len=%d/%f\n",samtm,len,len/96.0);
	// samtm = time_get_ms();

#ifndef __ANDROID__
#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = qtk_mod_am60_BF_VBOXEBF;
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
			wtk_debug("usbaudio queue max length = %d\n", m->usbaudio_queue.length);
			qtk_mod_am60_clean_queue(m, &(m->usbaudio_queue));
		}
	}
	// if(m->cfg->use_lineout){
	// 	msg_node2 = qtk_msg_pop_node(m->msg);
	// 	wtk_strbuf_push(msg_node2->buf, data, len);
	// 	wtk_blockqueue_push(&m->lineout_queue, &msg_node2->qn);
	// 	if(m->lineout_queue.length > m->cfg->max_output_length)
	// 	{
	// 		qtk_mod_am60_clean_queue(m, &(m->lineout_queue));
	// 	}
	// }
#endif
#endif
}

void qtk_mod_am60_on_eqform(qtk_mod_am60_t *m, char *data, int len)
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
#ifndef __ANDROID__
#ifndef OFFLINE_TEST
		if(m->cfg->use_usbaudio){
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_am60_BF_VBOXEBF;
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
				qtk_mod_am60_clean_queue(m, &(m->usbaudio_queue));
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
			qtk_mod_am60_clean_queue(m, &(m->lineout_queue));
		}
	}
#endif
#endif

	// if(m->cfg->use_log_wav || m->log_audio)
	// {
	// 	msg_node3 = qtk_msg_pop_node(m->msg);
	// 	msg_node3->type = qtk_mod_am60_BF_SAVE_SPK;
	// 	wtk_strbuf_push(msg_node3->buf, data, len);
	// 	wtk_blockqueue_push(&m->savepcm_queue, &msg_node3->qn);
	// }

	// wtk_strbuf_pop(m->playbuf, NULL, len);
}

void qtk_mod_am60_on_vboxebf(qtk_mod_am60_t *m, char *data, int len)
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

	if(m->cfg->audio3_type > 0)
	{
		msg_node = qtk_msg_pop_node(m->msg);
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->bfio3_queue, &msg_node->qn);
	}else{
		if(m->cfg->use_resample)
		{
			wtk_resample_feed(m->resample, data, len, 0);
		}else{
	#ifndef __ANDROID__
	#ifndef OFFLINE_TEST
			if(m->cfg->use_usbaudio){
				msg_node = qtk_msg_pop_node(m->msg);
				msg_node->type = qtk_mod_am60_BF_VBOXEBF;
				msg_node->sil=0;
	#ifdef USE_VBOXEBF
				if(m->cfg->audio_type == qtk_mod_am60_BF_VBOXEBF)
				{
					if(m->vboxebf_cfg->use_vboxebf3)
					{
						msg_node->sil=m->vboxebf->vebf3->mic_sil;
					}else if(m->vboxebf_cfg->use_vboxebf4)
					{
						msg_node->sil=m->vboxebf->vebf4->mic_sil;
					}
				}
	#endif
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
					qtk_mod_am60_clean_queue(m, &(m->usbaudio_queue));
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
				qtk_mod_am60_clean_queue(m, &(m->lineout_queue));
			}
		}
	#endif
	#endif
	}
	// if(m->cfg->use_log_wav || m->log_audio)
	// {
	// 	msg_node3 = qtk_msg_pop_node(m->msg);
	// 	msg_node3->type = qtk_mod_am60_BF_SAVE_SPK;
	// 	wtk_strbuf_push(msg_node3->buf, data, len);
	// 	wtk_blockqueue_push(&m->savepcm_queue, &msg_node3->qn);
	// }

	// wtk_strbuf_pop(m->playbuf, NULL, len);
}

void qtk_mod_am60_on_aec(qtk_mod_am60_t *m, qtk_var_t *var)
{
	qtk_msg_node_t *msg_node, *msg_node2;
	char *ss;
	char *data=NULL;
	int len=0;
	int nx=0;

	switch (var->type)
	{
	case QTK_SPEECH_DATA_PCM:
		data=var->v.str.data;
		len=var->v.str.len;
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
				msg_node->type = qtk_mod_am60_BF_VBOXEBF;
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
					qtk_mod_am60_clean_queue(m, &(m->usbaudio_queue));
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
				qtk_mod_am60_clean_queue(m, &(m->lineout_queue));
			}
		}
#endif
#endif
		break;
	case QTK_AEC_DIRECTION:
		wtk_debug("nbest=%d theta=%d phi=%d\n",var->v.ii.nbest,var->v.ii.theta,var->v.ii.phi);
		break;
	
	default:
		break;
	}
}


void qtk_mod_am60_on_bfio(qtk_mod_am60_t *m, qtk_var_t *var) {
   	qtk_msg_node_t *msg_node, *msg_node2;
	char *ss;
	char *data=NULL;
	int len;
	int nx=0;

	switch (var->type)
	{
	case QTK_SPEECH_DATA_PCM:
		data=var->v.str.data;
		len=var->v.str.len;
        if(m->cfg->echo_shift != 1.0)
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
				msg_node->type = qtk_mod_am60_BF_VBOXEBF;
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
					wtk_debug("usbaudio queue max length = %d\n", m->usbaudio_queue.length);
					qtk_mod_am60_clean_queue(m, &(m->usbaudio_queue));
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
				qtk_mod_am60_clean_queue(m, &(m->lineout_queue));
			}
		}
#endif
#endif
        break;
    case QTK_SPEECH_START:
        wtk_debug("=================> vad start\n");
		break;
    case QTK_SPEECH_END:
        wtk_debug("=================> vad end\n");
        break;
    case QTK_AEC_CANCEL_DATA:
        wtk_debug("=================> vad cancel len=%d\n",var->v.i);
        break;
    case QTK_AEC_WAKE:
        wtk_debug("=================> wake\n");
#ifdef USE_FOR_DEV
#if (defined  USE_AM32) || (defined  USE_R328)
		if(m->cfg->use_uart)
		{
			qtk_msg_node_t *msg_node;
			unsigned char data[16]={0};
			float energy=1000000.0f;
			int qtheta=500;

			data[0]=0xff;
			data[1]=0x01;
			data[2]=qtk_mod_am60_keep_low_byte(qtheta);
			data[3]=qtk_mod_am60_keep_high_byte(qtheta);
			memcpy(data+4, &energy, 4);
			data[8]=0x01;

			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_am60_BF_DIRECTION;
			wtk_strbuf_push(msg_node->buf, data, 9);
			wtk_blockqueue_push(&m->uart_queue, &msg_node->qn);
		}
#endif
#endif
        break;
    case QTK_AEC_DIRECTION:
        wtk_debug("=================> theta = %d\n",var->v.ii.theta);
#ifdef USE_FOR_DEV
#ifdef USE_AM32
		if(m->cfg->use_uart)
		{
			if(var->v.ii.theta != -1)
			{
				qtk_msg_node_t *msg_node;
				unsigned char data[16]={0};
				float energy=0;
				int qtheta=var->v.ii.theta;

				data[0]=0xff;
				data[1]=0x00;
				data[2]=qtk_mod_am60_keep_low_byte(qtheta);
				data[3]=qtk_mod_am60_keep_high_byte(qtheta);
				memcpy(data+4, &energy, 4);
				data[8]=0x01;

				msg_node = qtk_msg_pop_node(m->msg);
				msg_node->type = qtk_mod_am60_BF_DIRECTION;
				wtk_strbuf_push(msg_node->buf, data, 9);
				wtk_blockqueue_push(&m->uart_queue, &msg_node->qn);
			}
		}
#endif
#endif
        break;
    default:
        break;
    }
}

void qtk_mod_am60_on_kws(qtk_mod_am60_t *mod, qtk_var_t *var)
{
	switch (var->type)
	{
	case QTK_EVAL_TEXT:
		wtk_debug("kws result===>%.*s\n",var->v.str.len,var->v.str.data);
		break;
	default:
		break;
	}
}

#ifdef USE_SSL
void qtk_mod_am60_on_ssl(qtk_mod_am60_t *mod,qtk_ssl_extp_t *extp)
{
	wtk_debug("ssl===>theta =%d\n",extp[0].theta);
}
#endif

void qtk_mod_am60_on_vboxebf_ssl(qtk_mod_am60_t *mod, qtk_var_t *var)
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

int qtk_mod_am60_engine_new(qtk_mod_am60_t *m, int type)
{
	int ret;
	switch(type){
	case qtk_mod_am60_BF_VBOXEBF:
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
		qtk_vboxebf_set_notify(m->vboxebf, m, (qtk_vboxebf_notify_f)qtk_mod_am60_on_vboxebf);
		qtk_vboxebf_set_notify2(m->vboxebf, m, (qtk_engine_notify_f)qtk_mod_am60_on_vboxebf_ssl);
#endif
		break;
	case qtk_mod_am60_BF_GAINNETBF:
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
		qtk_gainnetbf_set_notify(m->gainnetbf, m, (qtk_gainnetbf_notify_f)qtk_mod_am60_on_vboxebf);
#endif
		break;
	case qtk_mod_am60_BF_SSL:
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
		qtk_ssl_set_notify(m->ssl, m, (qtk_ssl_notify_f)qtk_mod_am60_on_ssl);
#endif
		break;
	case qtk_mod_am60_BF_EQFORM:
#ifdef USE_EQFORM
		printf("qtk_mod_am60_BF_EQFORM resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		wtk_log_log(m->log, "qtk_mod_am60_BF_EQFORM resname=%.*s",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
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
		qtk_eqform_set_notify(m->eqform, m, (qtk_eqform_notify_f)qtk_mod_am60_on_eqform);
#endif
		break;
	case qtk_mod_am60_BF_AEC:
#ifdef USE_AEC
		printf("qtk_mod_am60_BF_AEC resname=%.*s\n",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
		wtk_log_log(m->log, "qtk_mod_am60_BF_AEC resname=%.*s",m->cfg->vboxebf_fn.len,m->cfg->vboxebf_fn.data);
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
		qtk_aec_set_notify2(m->aec, m, (qtk_engine_notify_f)qtk_mod_am60_on_aec);
#endif
		break;
	case qtk_mod_am60_BF_BFIO:
#ifdef USE_BFIO
		printf("qtk_mod_am60_BF_BFIO resname=%.*s\n",m->cfg->qform_fn.len,m->cfg->qform_fn.data);
		wtk_log_log(m->log, "qtk_mod_am60_BF_BFIO resname=%.*s",m->cfg->qform_fn.len,m->cfg->qform_fn.data);
		m->bfio_cfg = qtk_bfio_cfg_new(m->cfg->qform_fn.data);
		if(!m->bfio_cfg)
		{
			ret=-1;
			goto end;
		}
		m->bfio = qtk_bfio_new(m->bfio_cfg);
		if(!m->bfio)
		{
			ret=-1;
			goto end;
		}
		qtk_bfio_set_notify2(m->bfio, m, (qtk_engine_notify_f)qtk_mod_am60_on_bfio);
#endif
		break;
	case qtk_mod_am60_BF_KWS:
#ifdef USE_QKWS
		printf("qtk_mod_am60_BF_KWS resname=%.*s\n",m->cfg->kws_fn.len,m->cfg->kws_fn.data);
		wtk_log_log(m->log, "qtk_mod_am60_BF_KWS resname=%.*s",m->cfg->kws_fn.len,m->cfg->kws_fn.data);
		m->kws_cfg = qtk_qkws_cfg_new(m->cfg->kws_fn.data);
		if(!m->kws_cfg)
		{
			ret=-1;
			goto end;
		}
		m->kws = qtk_qkws_new(m->kws_cfg, NULL);
		if(!m->kws)
		{
			ret=-1;
			goto end;
		}
		qtk_qkws_set_notify(m->kws, m, (qtk_engine_notify_f)qtk_mod_am60_on_kws);
		wtk_debug("enroll_fn=%.*s\n",m->cfg->enroll_fn.len,m->cfg->enroll_fn.data);
		qtk_qkws_set_enroll_fn(m->kws, m->cfg->enroll_fn.data, m->cfg->enroll_fn.len);
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

void qtk_mod_am60_engine_delete(qtk_mod_am60_t *m, int type)
{
	switch(type){
	case qtk_mod_am60_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_delete(m->vboxebf);
		}
		if(m->vboxebf_cfg){
			qtk_vboxebf_cfg_delete(m->vboxebf_cfg);
		}
#endif
		break;
	case qtk_mod_am60_BF_GAINNETBF:
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
	case qtk_mod_am60_BF_SSL:
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
	case qtk_mod_am60_BF_EQFORM:
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
	case qtk_mod_am60_BF_AEC:
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
	case qtk_mod_am60_BF_BFIO:
#ifdef USE_BFIO
		if(m->bfio)
		{
			qtk_bfio_delete(m->bfio);
		}
		if(m->bfio_cfg)
		{
			qtk_bfio_cfg_delete(m->bfio_cfg);
		}
#endif
		break;
	case qtk_mod_am60_BF_KWS:
#ifdef USE_QKWS
		if(m->kws)
		{
			qtk_qkws_delete(m->kws);
		}
		if(m->kws_cfg)
		{
			qtk_qkws_cfg_delete(m->kws_cfg);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mod_am60_engine_start(qtk_mod_am60_t *m, int type)
{
	switch(type){
	case qtk_mod_am60_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_start(m->vboxebf);
		}
#endif
		break;
	case qtk_mod_am60_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		if(m->gainnetbf)
		{
			qtk_gainnetbf_start(m->gainnetbf);
		}
#endif
		break;
	case qtk_mod_am60_BF_SSL:
#ifdef USE_SSL
		if(m->ssl)
		{
			qtk_ssl_start(m->ssl);
		}
#endif
		break;
	case qtk_mod_am60_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform)
		{
			qtk_eqform_start(m->eqform);
		}
#endif
		break;
	case qtk_mod_am60_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_start(m->aec);
		}
#endif
		break;
	case qtk_mod_am60_BF_BFIO:
#ifdef USE_BFIO
		if(m->bfio)
		{
			qtk_bfio_start(m->bfio);
		}
#endif
		break;
	case qtk_mod_am60_BF_KWS:
#ifdef USE_QKWS
		if(m->kws)
		{
			qtk_qkws_start(m->kws);
		}
#endif
		break;
	default:
		break;
	}
}

void qtk_mod_am60_engine_stop(qtk_mod_am60_t *m, int type)
{
	switch(type){
	case qtk_mod_am60_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_reset(m->vboxebf);
		}
#endif
		break;
	case qtk_mod_am60_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		if(m->gainnetbf)
		{
			qtk_gainnetbf_reset(m->gainnetbf);
		}
#endif
		break;
	case qtk_mod_am60_BF_SSL:
#ifdef USE_SSL
		if(m->ssl)
		{
			qtk_ssl_reset(m->ssl);
		}
#endif
		break;
	case qtk_mod_am60_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform)
		{
			qtk_eqform_reset(m->eqform);
		}
#endif
		break;
	case qtk_mod_am60_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_reset(m->aec);
		}
#endif
		break;
	case qtk_mod_am60_BF_BFIO:
#ifdef USE_BFIO
		if(m->bfio)
		{
			qtk_bfio_reset(m->bfio);
		}
#endif
		break;
	case qtk_mod_am60_BF_KWS:
#ifdef USE_QKWS
		if(m->kws)
		{
			qtk_qkws_reset(m->kws);
		}
#endif
		break;
	default:
		break;
	}
}

int qtk_mod_am60_engine_feed(qtk_mod_am60_t *m, int type, char *data, int len, int is_end)
{
	switch(type){
	case qtk_mod_am60_BF_VBOXEBF:
#ifdef USE_VBOXEBF
		if(m->vboxebf){
			qtk_vboxebf_feed(m->vboxebf, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_am60_BF_GAINNETBF:
#ifdef USE_GAINNETBF
		if(m->gainnetbf)
		{
			qtk_gainnetbf_feed(m->gainnetbf, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_am60_BF_SSL:
#ifdef USE_SSL
		if(m->ssl)
		{
			qtk_ssl_feed(m->ssl, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_am60_BF_EQFORM:
#ifdef USE_EQFORM
		if(m->eqform)
		{
			qtk_eqform_feed(m->eqform, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_am60_BF_AEC:
#ifdef USE_AEC
		if(m->aec)
		{
			qtk_aec_feed(m->aec, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_am60_BF_BFIO:
#ifdef USE_BFIO
		if(m->bfio)
		{
			qtk_bfio_feed(m->bfio, data, len, is_end);
		}
#endif
		break;
	case qtk_mod_am60_BF_KWS:
#ifdef USE_QKWS
		if(m->kws)
		{
			qtk_qkws_feed(m->kws, data, len, is_end);
		}
#endif
		break;
	default:
		break;
	}
	return 0;
}

int qtk_mod_am60_get_ddff(qtk_mod_am60_t *m)
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

static void qtk_log_wav_file_new(qtk_mod_am60_t *m)
{
#ifndef __ANDROID__
	int mchannel=m->cfg->micrcd.channel - m->cfg->micrcd.nskip;
	int schannel=m->cfg->spkrcd.channel - m->cfg->spkrcd.nskip;
	int channel = mchannel + schannel;
	int bytes_per_sample = m->cfg->micrcd.bytes_per_sample;
	int sample_rate = m->cfg->micrcd.sample_rate;
#else
	int channel = m->cfg->micrcd.channel-m->cfg->micrcd.nskip;
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
  m->minlen = qtk_mod_am60_get_ddff(m);
#endif
}

static void qtk_log_wav_file_delete(qtk_mod_am60_t *m)
{
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);
	wtk_wavfile_close(m->single);
	wtk_wavfile_delete(m->single);
	m->mul = NULL;
	m->single = NULL;
}

static void qtk_is_log_audio(qtk_mod_am60_t *m)
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

void qtk_mod_am60_set_cpu(qtk_mod_am60_t *m, wtk_thread_t *thread, int cpunum)
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
#else
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
#ifdef USE_3308
int qtk_mod_am60_proc_read(qtk_mod_am60_t *m)
{
	// 读取程序
	char buf[32] = {0};
	char tmpbuf[1024]={0};
	char buf2[24]={0};
	int val,val2;
	char *pv=NULL;
	if (once == 1)
	{
		wtk_debug("=====>>>> mem_proc.pid_server = %d\n", mem_proc.pid_server);
		// wtk_debug("read:[%s]\n", mem_proc.shmaddr->buf);
		memcpy(buf, mem_proc.shmaddr->buf, MAX);
		wtk_debug("====>>>>>read buf=[%s]\n", buf); // 读取到sound_uart发送的请求

		if (strncmp(buf, "GET_MICVOLUME", 13) == 0)//检索当前麦克风音量级别
		{
			int ret;
			char buf2[32]={0};
			wtk_debug("======>>>>GET_MICVOLUME = %d\n", m->vboxebf->vebf4->agc_enable);
			val = m->vboxebf->vebf4->agc_enable;
			if(val==0)
			{
				float s;
				
				wtk_debug("mic_shift=%0.2f\n",m->vboxebf->vebf4->inmic_scale);
				s=((m->vboxebf->vebf4->inmic_scale-3.5)/(11-3.5))*100;
				ret=sprintf(buf2,"%d%%",(int)s);
				wtk_debug("GET_MICVOLUME=%s\n",buf2);
				qtk_mod_am60_proc_write(m,buf2,strlen(buf2));
			}else{
				float s;
				wtk_debug("+=====>>>>>>>agc_a=%f\n",m->vboxebf->vebf4->cfg->agc_a);
				s =( 1- ( ( m->vboxebf->vebf4->cfg->agc_a - 0.19) / (0.69 - 0.19) ) )*100;
				ret=sprintf(buf2,"%d%%",(int)s);//0.19-0.69
				wtk_debug("GET_MICVOLUME=%s\n",buf2);
				qtk_mod_am60_proc_write(m,buf2,strlen(buf2));
				// qtk_mod_am60_proc_write(m,"-1",2);
			}
			// if(val==1)//开启agc
			// {
			// 	float s;
			// 	wtk_debug("+=====>>>>>>>agc_a=%f\n",m->vboxebf->vebf4->cfg->agc_a);
			// 	s =( 1- ( ( m->vboxebf->vebf4->cfg->agc_a - 0.19) / (0.69 - 0.19) ) )*100;
			// 	ret=sprintf(buf2,"%d%%",(int)s);//0.19-0.69
			// 	wtk_debug("GET_MICVOLUME=%s\n",buf2);
			// 	qtk_mod_am60_proc_write(m,buf2,strlen(buf2));
			// }else{
			// 	float s;
				
			// 	// wtk_debug()
			// 	qtk_mod_am60_proc_write(m,"-1",2);
			// }
		}else if (strncmp(buf, "SET_MICVOLUME", 13) == 0)//设置当前麦克风音量级别
		{
			int ret=-2;
			float set;
			if(!m->vboxebf->vebf4->agc_enable)//没有开启agc
			{
				pv=strstr(buf,"\"set\":");//需要从百分比转换一下
				if(pv!=NULL)
				{
					set=(15.5 - 1.2) * (atof(pv+6) / 100 ) + 1.0;//6 
					// set=(11 - 3.7) * (atof(pv+6) / 100 ) + 3.5;//6 
					// set=7.5 * (1.0 - atof(pv+6) / 100 ) + 0.19;
					wtk_debug("==============>>>>SET_MICVOLUME=%f\n",set);
					wtk_vboxebf4_set_micvolume(m->vboxebf->vebf4,set);
					FILE *fn;
#if (defined  USE_AM32) || (defined  USE_802A) || (defined  USE_AM60)
					if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
					{
						fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r+");
#else
					if (access("/oem/uart.cfg", F_OK) == 0 && access("/oem/uart.cfg", W_OK) == 0)
					{
						fn = fopen("/oem/uart.cfg", "r+");
#endif
						ret = fread(tmpbuf, sizeof(tmpbuf), 1, fn);
						wtk_debug("================ret=%d fread=[%s]\n", ret,tmpbuf);
						pv = strstr(tmpbuf, "mic_shift2=");
						if(pv)
						{
							sprintf(buf2,"mic_shift2=%0.1f;",set);
							memcpy(pv,buf2,strlen(buf2));
							fseek(fn, 0, SEEK_SET);
							ret = fwrite(tmpbuf, strlen(tmpbuf), 1, fn);
							wtk_debug("================fwrite=%d\n", ret);
						}
						fflush(fn);
						fclose(fn);
					}
					system("sync");
					fn=NULL;
					ret=0;
				}
			}
			qtk_mod_am60_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "GET_MICANS", 10) == 0)//检索麦克风噪声抑制功能的启用状态
		{
			wtk_debug("======>>>>GET_MICANS = %d\n", m->vboxebf->vebf4->denoise_enable);
			val = m->vboxebf->vebf4->denoise_enable;
			if(val==1)
			{//返回参数
				qtk_mod_am60_proc_write(m,"1",1);
			}else{
				qtk_mod_am60_proc_write(m,"-1",2);
			}
			// val = m->vboxebf->vebf4->cfg->qmmse.noise_suppress;
			// m->vboxebf->vebf4->cfg->agc_a;
			// m->vboxebf->vebf4->cfg->echo_rls.L;
		}else if (strncmp(buf, "SET_MICANS_ON", 13) == 0)//设置麦克风噪声抑制功能的启用状态
		{
			int ret=-2;
			wtk_vboxebf4_set_denoiseenable(m->vboxebf->vebf4,1);
			ret=0;
			qtk_mod_am60_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "SET_MICANS_OFF", 14) == 0)
		{
			int ret=-2;
			wtk_vboxebf4_set_denoiseenable(m->vboxebf->vebf4,0);
			ret=0;
			qtk_mod_am60_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "GET_MICAGC", 10) == 0)//检索麦克风自动增益控制功能的启用状态
		{
			wtk_debug("======>>>>GET_MICAGC = %d\n", m->vboxebf->vebf4->agc_enable);
			val = m->vboxebf->vebf4->agc_enable;
			if(val==1)
			{//返回参数
				qtk_mod_am60_proc_write(m,"1",1);
			}else{
				qtk_mod_am60_proc_write(m,"-1",2);
			}
		}else if (strncmp(buf, "SET_MICAGC_ON", 13) == 0)//设置麦克风自动增益控制功能的启用状态
		{
			int ret=-2;
			wtk_vboxebf4_set_agcenable(m->vboxebf->vebf4,1);
			ret=0;
			qtk_mod_am60_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "SET_MICAGC_OFF", 14) == 0)
		{
			int ret=-2;
			wtk_vboxebf4_set_agcenable(m->vboxebf->vebf4,0);
			ret=0;
			qtk_mod_am60_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "GET_MICAEC", 10) == 0)//检索麦克风回声消除功能的启用状态
		{
			wtk_debug("======>>>>GET_MICAEC = %d\n", m->vboxebf->vebf4->echo_enable);
			val = m->vboxebf->vebf4->echo_enable;
			if(val==1)
			{//返回参数
				qtk_mod_am60_proc_write(m,"1",1);
			}else{
				qtk_mod_am60_proc_write(m,"-1",2);
			}
		}else if (strncmp(buf, "SET_MICAEC_ON", 13) == 0)//设置麦克风回声消除功能的启动状态
		{
			int ret=-2;
			wtk_vboxebf4_set_echoenable(m->vboxebf->vebf4,1);
			ret=0;
			qtk_mod_am60_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "SET_MICAEC_OFF", 14) == 0)
		{
			int ret=-2;
			wtk_vboxebf4_set_echoenable(m->vboxebf->vebf4,0);
			ret=0;
			qtk_mod_am60_proc_write(m,wtk_itoa(ret),abs(ret));
		}
	}

	once = 0;
	wtk_thread_join(&mem_t);
	return 0;
}

int qtk_mod_am60_proc_write(qtk_mod_am60_t *m,char *data,int len)
{
	// 写入程序
	int ret=0;
	wtk_debug("input>[%.*s]\n",len,data);
	memset(mem_proc.shmaddr->buf, 0, MAX);
	memcpy(mem_proc.shmaddr->buf, data, len);
	// ret=kill(mem_proc.pid_server, SIGUSR2);
	union sigval tmp;
	tmp.sival_int = 100;
 // 给进程 pid，发送 SIGINT 信号，并把 tmp 传递过去
	sigqueue(mem_proc.pid_server, SIGRTMIN+2, tmp);

	if(ret<0)
	{
		perror("kill");
	}
	wtk_debug("mem_proc.shmaddr->buf>[%.*s]\n",len,mem_proc.shmaddr->buf);
	return 0;
}

// void myfunc(int sig)
static void myfunc(int sig, siginfo_t *info, void *ptr)
{
	once =1;
	wtk_thread_start(&mem_t);
    return;
}

static void myfunc2(int sig, siginfo_t *info, void *ptr)//绑定pid
{
	once2=1;
	mem_proc.pid_server = mem_proc.shmaddr->pid;
	wtk_debug("====>>>>cp.pid_server=%d   getpid=%d\n", mem_proc.pid_server, getpid());
	once2=0;
	return;
}

int qtk_mod_am60_proc_send_pid(qtk_mod_am60_t *m)
{
	while(1)
	{
		sleep(1);
		if (once2==0)
		{
			sleep(2);
			mem_proc.shmaddr->pid = getpid();
		}
	}
	return 0;
}

#endif
