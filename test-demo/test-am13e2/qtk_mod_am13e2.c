#include "qtk_mod_am13e2.h"
#include "sdk/qtk_api.h"
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_riff.h"
#include <stdio.h>
// #define LAOGANG

#ifdef LAOGANG
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <sys/stat.h>
#ifndef SYSFS_EQ_PATH
// 如果板不是 3 号 I2C 总线，请把 3-0069 改成实际的 *-0069
#define SYSFS_EQ_PATH "/sys/bus/i2c/devices/3-0069/eq"
#endif
#endif
#define USE_3308
static int once=0;
static int once2=0;
static wtk_thread_t mem_t;
static wtk_thread_t mem2_t;
static qtk_proc_t  mem_proc;
uint64_t rcd3_total_frames = 0;
double startdelay_tm=0.0;
static uint64_t play_total_frames = 0;
int audio_check_request =-1;  				// 0-无请求, 1-扬声器检测, 2-麦克风检测
int audio_check_duration_ms=1000; 			// 检测时长
int audio_check_result = -1;
uint64_t audio_check_start_time;        // 检测开始时间戳
int audio_check_running;      			// 检测状态标志
int mic_first;                                  // 检测时长开始时间标志
int spk_first;                                  // 检测时长开始时间标志
int current_time;
int get_volume_calue=0;  //get volume
int get_volume_first=0;  //get volume
float volume_get_result=-97;
int mic_check_result = -1;
int speak_check_result = -1;
static double last_aplay_time = 0.0;  
double rcd3_elapsed = 0;
double merge_all=0;
double gainnet_tm=0.0;
double time_rcd3 = 0.0;
static double audio_check_play_tm =0.0;
static double audio_check_rcd_tm = 0.0;

// static int start_play=0;
static double last_rcd_time = 0.0;
LowPassFilter lpf;
// FILE * low_filter=NULL;
// FILE * low_filter_befor=NULL;

int qtk_mod_am13e2_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_rcd2_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_rcd3_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
// int qtk_mod_am13e2_rcd4_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
// int qtk_mod_am13e2_denoise_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);//降噪算法
int qtk_mod_am13e2_merge_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_vbox_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_array_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_gainnet_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_gainnet2_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_gainnet3_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);

int qtk_mod_am13e2_usbaudio_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_lineout_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
// int qtk_mod_am13e2_spk_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);

int qtk_mod_am13e2_linein_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_linein_check_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);

void qtk_mod_am13e2_on_outresample(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf2(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf3(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf4(qtk_mod_am13e2_t *m, char *data, int len);

int qtk_mod_am13e2_mic_check_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_mic_check_play_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
void qtk_mod_am13e2_on_mic_check_rcd(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int nchn);
void qtk_mod_am13e2_on_mic_check_play(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int channenl);

void qtk_mod_am13e2_on_vboxebf(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_array_vboxebf(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_denoise_vboxebf(qtk_mod_am13e2_t *m, char *data, int len);

void qtk_mod_am13e2_set_cpu(qtk_mod_am13e2_t *m, wtk_thread_t *thread, int cpunum);
void qtk_mod_am13e2_log_wav_file_new(qtk_mod_am13e2_t *m);
void qtk_mod_am13e2_log_wav_file_delete(qtk_mod_am13e2_t *m);
void qtk_mod_am13e2_is_log_audio(qtk_mod_am13e2_t *m);
void _hh25c_a_frame_extr(wtk_strbuf_t *buf,int extr_n);
int16_t* _hh25c_a_try_interpolation_supplement(const int16_t *sourceData, int32_t sampleRate, uint32_t srcSize,
                  int32_t newSampleRate, uint32_t *dstSize);
void qtk_mod_am13e2_clean_queue(qtk_mod_am13e2_t *m, wtk_blockqueue_t *queue);
void qtk_mod_am13e2_player_mode(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_player2_mode(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_player3_mode(qtk_mod_am13e2_t *m, char *data, int len);

// void qtk_mod_on_usb(void *ths, qtk_usb_uevent_state_t state, int sample_rate);
void qtk_init_100hz_lpf(LowPassFilter *filter);
void qtk_process_100hz_lpf(short *data, int len, LowPassFilter *filter);
//音频检测函数
bool check_audio_data(const int16_t *data, int samples);
static int continuous_audio_check(qtk_mod_am13e2_t *m, const int16_t *data, int samples);
//音量值获取
static void continuous_get_volume(qtk_mod_am13e2_t *m, const int16_t *data, int samples);
#ifdef USE_3308
int qtk_mod_am13e2_proc_send_pid(qtk_mod_am13e2_t *m);
int qtk_mod_am13e2_proc_read(qtk_mod_am13e2_t *m);
int qtk_mod_am13e2_proc_write(qtk_mod_am13e2_t *m,char *data,int len);
//读取寄存器
int qtk_read_register(const char *reg_path, int *output_value) {
    int fd;
    int ret;
    char buf[32] = {0};

    fd = open(reg_path, O_RDONLY | O_SYNC);
    if (fd < 0) {
        perror("Open register for read failed");
        return -1;
    }

    ret = read(fd, buf, sizeof(buf) - 1);
    if (ret < 0) {
        perror("Read register failed");
        close(fd);
        return -1;
    }

    close(fd);
    *output_value = atoi(buf);
    return 0;
}
//音量值获取

float get_db(short *data, int len) {
	if(len == 0 || data == NULL)
		return -96.0;
    float sum = 0.0;
    for (int i = 0; i < len; i++) {
        sum += data[i] * data[i];
    }
    sum /= len;
    float rms = sqrt(sum);
    
    
    if (rms < 1) {
        return -96.0;
    }
    float db = 20 * log10(rms/32768.0);
	// wtk_debug("----------------------------->>>>>>>>>>>>>>dB: %f\n", db);
    return db;
}
void qtk_process_100hz_lpf(short *data, int len, LowPassFilter *filter) {
    for (int i = 0; i < len; i++) {
        // 将short转换为float（保持精度）
        float input = (float)data[i];
		// 应用滤波器
        float output = filter->b0 * input + filter->b1 * filter->x1 +
                       filter->b2 * filter->x2 - filter->a1 * filter->y1 -
                       filter->a2 * filter->y2;

        // 更新历史状态
        filter->x2 = filter->x1;
        filter->x1 = input;
        filter->y2 = filter->y1;
        filter->y1 = output;

        // 将结果写回（带饱和处理）
        if (output > 32767.0f) {
            data[i] = 32767;
        } else if (output < -32768.0f) {
            data[i] = -32768;
        } else {
            data[i] = (short)output;
        }
    }
}

void qtk_init_100hz_lpf(LowPassFilter *filter) {
    // 滤波器参数计算（Butterworth二阶低通）
    const float sample_rate = 48000.0f;
    const float cutoff_freq = 100.0f;
    const float Q = 0.7071f; // Butterworth Q值

    float w0 = 2 * 3.1415926535f * cutoff_freq / sample_rate;
    float alpha = sinf(w0) / (2 * Q);

    float cos_w0 = cosf(w0);
    float b0 = (1 - cos_w0) / 2;
    float b1 = 1 - cos_w0;
    float b2 = (1 - cos_w0) / 2;
    float a0 = 1 + alpha;
    float a1 = -2 * cos_w0;
    float a2 = 1 - alpha;

    // 归一化系数
    filter->b0 = b0 / a0;
    filter->b1 = b1 / a0;
    filter->b2 = b2 / a0;
    filter->a1 = a1 / a0;
    filter->a2 = a2 / a0;

    // 清零历史状态
    filter->x1 = filter->x2 = 0.0f;
    filter->y1 = filter->y2 = 0.0f;
}
// void myfunc(int sig);
static void myfunc(int sig, siginfo_t *info, void *ptr);
static void myfunc2(int sig, siginfo_t *info, void *ptr);
#endif
qtk_audio_port_t* create_port(int port_type, int port_id, const char* name, 
                             bool is_use, int gain_level, 
                             int audio_input_type, bool is_local_play)
{
    qtk_audio_port_t *port = (qtk_audio_port_t*)malloc(sizeof(qtk_audio_port_t));
    if (!port) {
        return NULL;
    }
    
    port->port_type = port_type;
    port->port_id = port_id;
    port->name = strdup(name);
    port->is_use = is_use;
    port->gain_level = gain_level;
    port->audio_input_type = audio_input_type;
    port->is_local_play = is_local_play;
    
    return port;
}
void free_port(qtk_audio_port_t *port)
{
    if (port) {
        if (port->name) {
            free(port->name);
        }
        free(port);
    }
}
bool check_audio_data(const int16_t *data, int samples)
{
    double sum = 0;
    int channel_samples = samples / 8;
    
    if (channel_samples <= 0) return false;
    for (int i = 0; i < samples; i += 8)
    {
        sum += abs(data[i]);
    }
    
    double average = sum / channel_samples;

    double db = 0;
    if (average > 0)
    {
        db = 20.0 * log10(average);
    }
    else
    {
        db = -96.0;
    }
    const double SILENCE_THRESHOLD_DB = -45.0;
    // wtk_debug("----------------------------<>>>db %.2f\n",db);
    return db > SILENCE_THRESHOLD_DB;
}
// bool check_audio_data(const int16_t *data, int samples)
// {
//     double sum = 0;
//     int channel_samples = samples / 8;
    
//     if (channel_samples <= 0) return false;

//     for (int i = 0; i < samples; i += 8)
//     {
//         sum += abs(data[i]);
//     }
    
//     sum = sum / channel_samples;

//     int db = 0;
//     if (sum > 0)
//     {
//         db = (int)(20.0 * log10(sum));
//     }

//     return db > 0;
// }
static int continuous_audio_check(qtk_mod_am13e2_t *m, const int16_t *data, int samples) {
    if (audio_check_running) {
        bool frame_result = check_audio_data(data, samples);
        if (frame_result == true) {
           return audio_check_result = 0;
        } 
        else if (frame_result == false) {
           return audio_check_result = 1;
        }
    }
}

static void continuous_get_volume(qtk_mod_am13e2_t *m, const int16_t *data, int samples){
	// double get_db_time = time_get_ms();
	double db_get_result= get_db((short *)data, samples);
	// wtk_debug("-------------------->>>>>>>get_db_time= %.3f\n",time_get_ms()-get_db_time);
	if(db_get_result < -47.00)
		volume_get_result =-96;
}
qtk_mod_am13e2_t *qtk_mod_am13e2_new(qtk_mod_am13e2_cfg_t *cfg)
{
	qtk_usb_uevent_t *qu=NULL;
	qtk_mod_am13e2_t *m;
	char tmp[64];
	int ret,volume,len,count_towrite=0;
	int opri,npri;
	char set_buf[128]={0};
	opri = getpriority(PRIO_PROCESS, getpid());
	wtk_debug("=================>>>>>>>>>>old:%d\n",opri);
	npri = -20;
	setpriority(PRIO_PROCESS, getpid(), npri);
	opri = getpriority(PRIO_PROCESS, getpid());
	wtk_debug("=================>>>>>>>>>>new:%d\n",opri);

	m = (qtk_mod_am13e2_t *)wtk_calloc(1, sizeof(*m));
	m->cfg = cfg;
	#if 1
	while(1){
		count_towrite++;
		if(access("/sys/bus/i2c/devices/3-0069/volume",F_OK) == 0 && access("/sys/bus/i2c/devices/3-0069/volume",W_OK) == 0)
		{
			char*p=file_read_buf("/oem/qdreamer/qsound/spk_volume.txt",&len);
			volume=atoi(p);
			sprintf(set_buf,"echo %d > /sys/bus/i2c/devices/3-0069/volume",volume);
			system(set_buf);
			wtk_debug("------->%s\n",set_buf);
			sprintf(set_buf,"echo %d > /sys/bus/i2c/devices/3-006d/volume",volume);
			system(set_buf);
			wtk_debug("------->%s\n",set_buf);
			break;
		}
		if(count_towrite > 3)
			break;
		usleep(10*1000);
	}
	#endif
#if 1//def USE_3308
	///////////////////////
	mem_proc.shmaddr=NULL;
	// 创建一个提取码

    if ((mem_proc.key = ftok("/oem/qdreamer/qsound/version.txt", 'z')) == -1)
    {
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
	wtk_thread_init(&mem_t,(thread_route_handler)qtk_mod_am13e2_proc_read, m);
	wtk_thread_init(&mem2_t,(thread_route_handler)qtk_mod_am13e2_proc_send_pid, m);
	signal(SIGUSR1,myfunc);
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

	m->usbaudio_run=0;
	m->lineout_run=0;
	// m->speaker_run=0;
	m->rcd_run=0;
	m->rcd2_run=0;
	m->rcd3_run=0;
	m->rcd4_run=0;
	m->merge_rcd_run=0;
	m->vbox_run=0;
	m->array_vbox_run=0;
	m->denoise_vbox_run=0;

	m->gainnet_run=0;
	m->gainnet2_run=0;
	m->gainnet3_run=0;
	m->linein_run=0;
	m->use_linein_out = 1;
	m->sttimer=NULL;
	m->mic_channel=m->cfg->rcd.channel - m->cfg->rcd.nskip;
	m->is_player_start=0;
	m->is_mic =1;
	m->is_use_uac = 0;
	m->audio_check_rcd_feed_end = 0;
	m->audio_check_play_feed_end = 0;
	//wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>>>>mic_channel=%d / %d ns=%d\n",m->mic_channel,m->cfg->rcd.channel,m->cfg->rcd.nskip);
	// wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");
	m->left_audiobuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(m->left_audiobuf);
	m->all_audiobuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(m->all_audiobuf);
	m->check_path_buf = wtk_strbuf_new(64,0);
	// m->mul_path = wtk_strbuf_new(64,0);
	// m->iis_path = wtk_strbuf_new(64,0);
	// m->play_path = wtk_strbuf_new(64, 0);
	

#if 1
	m->lineout_path = wtk_strbuf_new(64, 0);
	m->arraymul_path = wtk_strbuf_new(64,0);	
#endif
	// wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");s
	if(m->cfg->cache_path.len <= 0){
		wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");
		return ;
	}else{
		wtk_mkdir(m->cfg->cache_path.data);
		wtk_strbuf_push_f(m->check_path_buf, "%.*s/start_log_audio", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->check_path_buf, 0);
		// wtk_strbuf_push_f(m->mul_path, "%.*s/mul.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		// wtk_strbuf_push_c(m->mul_path, 0);
		// wtk_strbuf_push_f(m->iis_path, "%.*s/iis.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		// wtk_strbuf_push_c(m->iis_path, 0);
		
	#if 1
		wtk_strbuf_push_f(m->lineout_path, "%.*s/lineout.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->lineout_path, 0);
		
		wtk_strbuf_push_f(m->arraymul_path, "%.*s/arraymul.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->arraymul_path, 0);
	#endif
		// wtk_strbuf_push_f(m->play_path, "%.*s/play.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		// wtk_strbuf_push_c(m->play_path, 0);
		// wtk_strbuf_push_f(m->uac_path, "%.*s/uac.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		// wtk_strbuf_push_c(m->uac_path, 0);
		
	}
    qtk_init_100hz_lpf(&lpf);
#ifndef OFFLINE_TEST
	if(m->cfg->use_log && m->cfg->cache_path.len > 0){
		snprintf(tmp, 64, "%.*s/qvoice_rp.log", m->cfg->cache_path.len, m->cfg->cache_path.data);
		m->log = wtk_log_new(tmp);
	}
	if(m->cfg->use_log_wav){
		qtk_mod_am13e2_log_wav_file_new(m);
	}
#endif

	wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");
	if(m->cfg->use_out_resample){
		m->outresample = wtk_resample_new(256);
		wtk_resample_set_notify(m->outresample, m, (wtk_resample_notify_f)qtk_mod_am13e2_on_outresample);
	}

	wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");
	wtk_debug("=========>>>>>>>>use_array=%d\n",m->cfg->use_array);
	if(m->cfg->gainnetbf_cfg){
		if(m->cfg->use_array == 0){
			m->cfg->gainnetbf_cfg->join_channel = m->cfg->gainnetbf_cfg->join_channel - 1;
		}
		m->gainnetbf = qtk_gainnetbf_new(m->cfg->gainnetbf_cfg);
		if(!m->gainnetbf){
			wtk_debug("gainnetbf_rtjoin new failed!\n");
			m->gainnetbf = NULL;
		}
		qtk_gainnetbf_set_notify(m->gainnetbf, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf);

		m->gainnetbf2 = qtk_gainnetbf_new(m->cfg->gainnetbf_cfg);
		if(!m->gainnetbf2){
			wtk_debug("gainnetbf2_rtjoin new failed!\n");
			m->gainnetbf2 = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf2_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf2, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf2);

		m->gainnetbf3 = qtk_gainnetbf_new(m->cfg->gainnetbf_cfg);
		if(!m->gainnetbf3){
			wtk_debug("gainnetbf3_rtjoin new failed!\n");
			m->gainnetbf3 = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf3_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf3, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf3);

		m->gainnetbf4 = qtk_gainnetbf_new(m->cfg->gainnetbf_cfg);
		if(!m->gainnetbf4){
			wtk_debug("gainnetbf4_rtjoin new failed!\n");
			m->gainnetbf4 = NULL;
		}
			wtk_debug("----------------------->>>>>>>>>>gainnetbf4_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf4, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf4);
	}
	#if 1
	if(m->cfg->mic_check_rcd_cfg){
		wtk_debug("---------------------__>>>>>>>>>>>>>>>>>\n");
		m->mic_check_rcd = wtk_mic_check_new(m->cfg->mic_check_rcd_cfg);
		if(!m->mic_check_rcd){
			wtk_debug("mic_check_rcd new failed!\n");
			m->mic_check_rcd = NULL;
		}
		wtk_debug("---------------------__>>>>>>>>>>>>>>>>>\n");
		wtk_mic_check_set_notify(m->mic_check_rcd, m,(wtk_mic_check_notify_f)qtk_mod_am13e2_on_mic_check_rcd);
		wtk_debug("---------------------__>>>>>>>>>>>>>>>>>\n");
	}
	#endif
	#if 1
	if(m->cfg->mic_check_play_cfg){
		m->mic_check_play = wtk_mic_check_new(m->cfg->mic_check_play_cfg);
		if(!m->mic_check_play){
			wtk_debug("mic_check_play new failed!\n");
			m->mic_check_play = NULL;
		}
		wtk_mic_check_set_notify(m->mic_check_play, m,(wtk_mic_check_notify_f)qtk_mod_am13e2_on_mic_check_play);
	}
	#endif
	if(m->cfg->vboxebf_cfg){
		m->vboxebf = qtk_vboxebf_new(m->cfg->vboxebf_cfg);
		if(!m->vboxebf){
			wtk_debug("vboxebf new failed!\n");
			m->vboxebf = NULL;
		}
		qtk_vboxebf_set_notify(m->vboxebf, m, (qtk_vboxebf_notify_f)qtk_mod_am13e2_on_vboxebf);
	}

	if(m->cfg->use_array){
		if(m->cfg->avboxebf_cfg){
			m->avboxebf = qtk_vboxebf_new(m->cfg->avboxebf_cfg);
			if(!m->avboxebf){
				wtk_debug("array vboxebf new failed!\n");
				m->avboxebf = NULL;
			}
			qtk_vboxebf_set_notify(m->avboxebf, m, (qtk_vboxebf_notify_f)qtk_mod_am13e2_on_array_vboxebf);

		}
	}
	//linein_mic 降噪算法
	if(m->cfg->use_line_in){
		if(m->cfg->denoisebf_cfg){
			m->denoisebf = qtk_vboxebf_new(m->cfg->denoisebf_cfg);
			if(!m->denoisebf){
				wtk_debug("denoisebf new failed!\n");
				m->denoisebf = NULL;
			}
			qtk_vboxebf_set_notify(m->denoisebf, m, (qtk_vboxebf_notify_f)qtk_mod_am13e2_on_denoise_vboxebf);
		}
	}
	char buf[256];
	wtk_get_build_timestamp(buf);
	printf("BUILD AT %s\n",buf);
	wtk_log_log(m->log, "BUILD AT %s",buf);

#ifndef OFFLINE_TEST
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_init(&m->usbaudio_queue);
		wtk_thread_init(&m->usbaudio_t,(thread_route_handler)qtk_mod_am13e2_usbaudio_entry, m);
		m->usbaudio = qtk_play_new(&cfg->usbaudio);
	}
	if(m->cfg->use_line_in){
		wtk_blockqueue_init(&m->linein_queue);
		wtk_thread_init(&m->linein_t,(thread_route_handler)qtk_mod_am13e2_linein_entry, m);
	}
	wtk_debug("--------------------------->>>>>>>>\n");
	if(m->cfg->use_lineout){
		wtk_blockqueue_init(&m->lineout_queue);
		wtk_thread_init(&m->lineout_t,(thread_route_handler)qtk_mod_am13e2_lineout_entry, m);
		m->lineout = qtk_play_new(&cfg->lineout);
	}
	wtk_debug("--------------------------->>>>>>>>\n");
	// if(m->cfg->use_speaker){
	// 	wtk_blockqueue_init(&m->spk_queue);
	// 	wtk_thread_init(&m->speaker_t,(thread_route_handler)qtk_mod_am13e2_spk_entry, m);
	// 	wtk_debug("qtk_mod_am13e2_spk_entry is ok!!\n");
	// 	m->speaker = qtk_play_new(&cfg->speaker);
	// }
#endif

	m->rcd =qtk_record_new(&(m->cfg->rcd));
	if(!m->rcd){
		wtk_log_err0(m->log, "record fiald!");
		ret = -1;
		goto end;
	}
	wtk_debug("--------------------------->>>>>>>>\n");
	wtk_thread_init(&m->rcd_t, (thread_route_handler)qtk_mod_am13e2_rcd_entry, m);

	if(m->cfg->use_array){
		m->rcd2 =qtk_record_new(&(m->cfg->rcd2));
		if(!m->rcd2){
			wtk_log_err0(m->log, "record2 fiald!");
			ret = -1;
			goto end;
		}
		wtk_thread_init(&m->rcd2_t, (thread_route_handler)qtk_mod_am13e2_rcd2_entry, m);
		wtk_thread_init(&m->array_vbox_t, (thread_route_handler)qtk_mod_am13e2_array_entry, m);
		wtk_blockqueue_init(&m->array_vbox_queue);
	}
	wtk_debug("--------------------------->>>>>>>>\n");
	// if(m->cfg->use_linein_mic){
	// wtk_thread_init(&m->denoise_vbox_t, (thread_route_handler)qtk_mod_am13e2_denoise_entry, m);
	// wtk_blockqueue_init(&m->denoise_vbox_queue);
	// }
	m->rcd3 = qtk_record_new(&(m->cfg->rcd3));
	if(!m->rcd3){
		wtk_log_err0(m->log, "record3 failed!");
		ret = -1;
		goto end;
	}

	wtk_debug("--------------------------->>>>>>>>\n");
	wtk_thread_init(&m->rcd3_t, (thread_route_handler)qtk_mod_am13e2_rcd3_entry, m);
	// m->rcd4 = qtk_record_new(&(m->cfg->rcd4));
	// if(!m->rcd4){
	// 	wtk_log_err0(m->log, "record4 failed!");
	// 	ret = -1;
	// 	goto end;
	// }
	// wtk_thread_init(&m->rcd4_t, (thread_route_handler)qtk_mod_am13e2_rcd4_entry, m);
	wtk_thread_init(&m->merge_rcd_t, (thread_route_handler)qtk_mod_am13e2_merge_rcd_entry, m);
	wtk_blockqueue_init(&m->merge_rcd_queue);

	wtk_thread_init(&m->vbox_t, (thread_route_handler)qtk_mod_am13e2_vbox_entry, m);
	wtk_blockqueue_init(&m->vbox_queue);
	wtk_thread_init(&m->gainnet_t, (thread_route_handler)qtk_mod_am13e2_gainnet_entry, m);
	wtk_thread_init(&m->gainnet2_t, (thread_route_handler)qtk_mod_am13e2_gainnet2_entry, m);
	wtk_thread_init(&m->gainnet3_t, (thread_route_handler)qtk_mod_am13e2_gainnet3_entry, m);

	wtk_thread_init(&m->mic_check_rcd_t, (thread_route_handler)qtk_mod_am13e2_mic_check_rcd_entry, m);
	wtk_thread_init(&m->mic_check_play_t, (thread_route_handler)qtk_mod_am13e2_mic_check_play_entry, m);

	wtk_blockqueue_init(&m->gainnet_queue);
	wtk_blockqueue_init(&m->gainnet2_queue);
	wtk_blockqueue_init(&m->gainnet3_queue);


	if(m->cfg->use_linein_check){
		wtk_thread_init(&m->linein_check_t, (thread_route_handler)qtk_mod_am13e2_linein_check_entry, m);
	}

	m->sttimer = qtk_timer_new(m->log);
	m->msg = qtk_msg_new();
	
#if 0//def USE_3308
	///////////////////////
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
	wtk_debug("------------------------------------>>>>>\n");
    // 注册信号
	wtk_thread_init(&mem_t,(thread_route_handler)qtk_mod_am13e2_proc_read, m);
	wtk_debug("------------------------------------>>>>>\n");
	wtk_thread_init(&mem2_t,(thread_route_handler)qtk_mod_am13e2_proc_send_pid, m);
	wtk_debug("------------------------------------>>>>>\n");
	signal(SIGUSR1,myfunc);
	struct sigaction act, oact;
	act.sa_sigaction = myfunc; //指定信号处理回调函数
	wtk_debug("------------------------------------>>>>>\n");
	sigemptyset(&act.sa_mask); // 阻塞集为空
	act.sa_flags = SA_SIGINFO; // 指定调用 signal_handler
 // 注册信号 SIGINT
	sigaction(SIGRTMIN, &act, &oact);
	// signal(SIGUSR2,myfunc2);

	struct sigaction act2, oact2;
	wtk_debug("------------------------------------>>>>>\n");
	act2.sa_sigaction = myfunc2; //指定信号处理回调函数
	sigemptyset(&act2.sa_mask); // 阻塞集为空
	act2.sa_flags = SA_SIGINFO; // 指定调用 signal_handler
 // 注册信号 SIGINT
	wtk_debug("------------------------------------>>>>>\n");
	sigaction(SIGRTMIN+2, &act2, &oact2);
	wtk_debug("------------------------------------>>>>>\n");
	mem_proc.shmaddr->pid = getpid();
	wtk_debug("------------------------------------>>>>>\n");
#endif
	// if(access("/oem/qdreamer/qsound/spk_volume.txt",F_OK) == 0)
	// {
	// 	char*p=file_read_buf("/oem/qdreamer/qsound/spk_volume.txt",&len);
	// 	volume=atoi(p);
	// }
	// sprintf(set_buf,"echo %d > /sys/bus/i2c/devices/3-0069/volume",volume);
	// system(set_buf);
	// wtk_debug("------->%s\n",set_buf);
	// sprintf(set_buf,"echo %d > /sys/bus/i2c/devices/3-006d/volume",volume);
	// system(set_buf);
	// wtk_debug("------->%s\n",set_buf);
	// qu=qtk_usb_uevent_new();
	wtk_log_log0(m->log, "222222222222222222222222222!\n");
	// qtk_usb_uevent_set_notify(qu, NULL, (qtk_usb_uevent_notify_f)qtk_mod_on_usb);
	wtk_log_log0(m->log, "qtk_mod_am13e2 NEW OK!");
	wtk_debug("qtk_mod_am13e2 NEW OK!!!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_mod_am13e2_delete(m);
      m = NULL;
  	}
	return m;
}

void qtk_mod_am13e2_delete(qtk_mod_am13e2_t *m)
{
	if(m->rcd){
		qtk_record_delete(m->rcd);
	}
	if(m->cfg->use_array){
		if(m->rcd2){
			qtk_record_delete(m->rcd2);
		}
	}
	if(m->cfg->use_usbaudio){
		wtk_blockqueue_clean(&m->usbaudio_queue);
		if(m->usbaudio){
			qtk_play_delete(m->usbaudio);
		}
	}
	if(m->cfg->use_lineout){
		wtk_blockqueue_clean(&m->lineout_queue);
		if(m->lineout){
			qtk_play_delete(m->lineout);
		}
	}
	// if(m->cfg->use_speaker){
	// 	wtk_blockqueue_clean(&m->spk_queue);
	// 	if(m->speaker){
	// 		qtk_play_delete(m->speaker);
	// 	}
	// }
	wtk_blockqueue_clean(&m->gainnet_queue);
	wtk_blockqueue_clean(&m->gainnet2_queue);
	wtk_blockqueue_clean(&m->gainnet3_queue);
	wtk_blockqueue_clean(&m->vbox_queue);
	wtk_blockqueue_clean(&m->array_vbox_queue);
	wtk_blockqueue_clean(&m->merge_rcd_queue);
	wtk_blockqueue_clean(&m->denoise_vbox_queue);
	wtk_blockqueue_clean(&m->mic_check_rcd_queue);
	wtk_blockqueue_clean(&m->mic_check_play_queue);
	if(m->msg){
		qtk_msg_delete(m->msg);
	}
	if(m->cfg->use_log_wav){
		qtk_mod_am13e2_log_wav_file_delete(m);
	}
	if(m->check_path_buf){
		wtk_strbuf_delete(m->check_path_buf);
	}
	
	if(m->arraymul_path){
		wtk_strbuf_delete(m->arraymul_path);
	}
	if(m->lineout_path){
		wtk_strbuf_delete(m->lineout_path);
	}
#if 0
	if(m->iis_path){
		wtk_strbuf_delete(m->iis_path);
	}
	if(m->mul_path){
		wtk_strbuf_delete(m->mul_path);
	}
#endif
	// if(m->play_path){
	// 	wtk_strbuf_delete(m->play_path);
	// }
	if(m->vboxebf){
		qtk_vboxebf_delete(m->vboxebf);
	}
	if(m->avboxebf){
		qtk_vboxebf_delete(m->avboxebf);
	}
	if(m->gainnetbf){
		qtk_gainnetbf_delete(m->gainnetbf);
	}
	if(m->gainnetbf2){
		qtk_gainnetbf_delete(m->gainnetbf2);
	}
	if(m->gainnetbf3){
		qtk_gainnetbf_delete(m->gainnetbf3);
	}
	if(m->mic_check_rcd){
		wtk_mic_check_delete(m->mic_check_rcd);
	}
	if(m->mic_check_play){
		wtk_mic_check_delete(m->mic_check_play);
	}
	if(m->cfg->use_out_resample){
		if(m->outresample){
			wtk_resample_delete(m->outresample);
		}
	}
	if(m->sttimer){
		qtk_timer_delete(m->sttimer);
	}
	if(m->log){
		wtk_log_delete(m->log);
	}
	wtk_debug("======================>>>>>>>>>>>>>>.delete\n");
    wtk_free(m);
}

void qtk_mod_am13e2_record_start(qtk_mod_am13e2_t *m)
{
	if(m->cfg->debug){
		wtk_debug("=============>>>>>>>>>>>>rp record start\n");
		wtk_log_log0(m->log,"=============>>>>>>>>>>>>rp record start\n");
	}
	if(m->merge_rcd_run ==0){
		m->merge_rcd_run = 1;
		wtk_thread_start(&m->merge_rcd_t);
	}
	if(m->rcd_run ==0){
		int ret=-1;
		// ret = qtk_record_start(m->rcd);
		// wtk_debug("===================>>>>>>>>>>>>>>>>>>>>>>>>ret=%d\n",ret);
		// if(ret!=0){
		// 	wtk_log_err0(m->log, "record start fiald!");
		// }
		m->rcd_run = 1;
		m->is_mic = 1;
		wtk_thread_start(&m->rcd_t);
	}
	if(m->rcd2_run ==0 && m->cfg->use_array){
		int ret=-1;
		// ret = qtk_record_start(m->rcd2);
		// wtk_debug("===================>>>>>>>>>>>>>>>>>>>>>>>>ret=%d\n",ret);
		// if(ret!=0){
		// 	wtk_log_err0(m->log, "record2 start fiald!");
		// }
		m->rcd2_run = 1;
		wtk_thread_start(&m->rcd2_t);
	}
	if(m->rcd3_run ==0){
        m->rcd3_run = 1;
        wtk_thread_start(&m->rcd3_t);
    }
	if(m->rcd4_run ==0){
        m->rcd4_run = 1;
        wtk_thread_start(&m->rcd4_t);
    }
	return;
}

void qtk_mod_am13e2_record_stop(qtk_mod_am13e2_t *m)
{
	if(m->cfg->debug){
		wtk_debug("=============>>>>>>>>>>>>rp record stop\n");
		wtk_log_log0(m->log,"=============>>>>>>>>>>>>rp record stop\n");
	}
	if(m->rcd_run){
		m->rcd_run = 0;
		m->is_mic = 0;
		wtk_thread_join(&m->rcd_t);
		if(m->rcd){
			qtk_record_stop(m->rcd);
		}
		if(m->cfg->debug){
			wtk_debug("=============>>>>>>>>>>>>rp record stop\n");
			wtk_log_log0(m->log,"=============>>>>>>>>>>>>rp record stop\n");
		}
	}

	if(m->rcd2_run){
		m->rcd2_run = 0;
		wtk_thread_join(&m->rcd2_t);
		if(m->rcd2){
			qtk_record_stop(m->rcd2);
		}
		if(m->cfg->debug){
			wtk_debug("=============>>>>>>>>>>>>rp record2 stop\n");
			wtk_log_log0(m->log,"=============>>>>>>>>>>>>rp record2 stop\n");
		}
	}
	if(m->rcd3_run){
        m->rcd3_run = 0;
        wtk_thread_join(&m->rcd3_t);
        if(m->rcd3){
            qtk_record_stop(m->rcd3);
        }
		if(m->cfg->debug){
			wtk_debug("=============>>>>>>>>>>>>rp record3 stop\n");
			wtk_log_log0(m->log,"=============>>>>>>>>>>>>rp record3 stop\n");
		}
    }
	if(m->rcd4_run){
        m->rcd4_run = 0;
        wtk_thread_join(&m->rcd4_t);
        if(m->rcd4){
            qtk_record_stop(m->rcd4);
        }
		if(m->cfg->debug){
			wtk_debug("=============>>>>>>>>>>>>rp record3 stop\n");
			wtk_log_log0(m->log,"=============>>>>>>>>>>>>rp record3 stop\n");
		}
    }
	if(m->merge_rcd_run){
		m->merge_rcd_run = 0;
		wtk_thread_join(&m->merge_rcd_t);
	}
}

void qtk_mod_am13e2_output_start(qtk_mod_am13e2_t *m)
{
	if(m->cfg->use_usbaudio && m->usbaudio_run==0){
		qtk_play_start(m->usbaudio);
		m->usbaudio_run = 1;
		wtk_thread_start(&m->usbaudio_t);
	}
	if(m->cfg->use_lineout && m->lineout_run==0){
		qtk_play_start(m->lineout);
		m->lineout_run = 1;
		wtk_thread_start(&m->lineout_t);
	}
	// if(m->cfg->use_speaker && m->speaker_run==0){
	// 	qtk_play_start(m->speaker);
	// 	m->speaker_run = 1;
	// 	wtk_thread_start(&m->speaker_t);
	// }
	if(m->cfg->use_line_in && m->linein_run==0){
		m->linein_run = 1;
		wtk_thread_start(&m->linein_t);
	}
}

void qtk_mod_am13e2_output_stop(qtk_mod_am13e2_t *m)
{
	if(m->cfg->use_usbaudio && m->usbaudio_run){
		m->usbaudio_run = 0;
		wtk_blockqueue_wake(&m->usbaudio_queue);
		wtk_thread_join(&m->usbaudio_t);
	}
	if(m->cfg->use_lineout && m->lineout_run){
		m->lineout_run = 0;
		wtk_blockqueue_wake(&m->lineout_queue);
		wtk_thread_join(&m->lineout_t);
	}
	// if(m->cfg->use_speaker && m->speaker_run){
	// 	m->speaker_run = 0;
	// 	wtk_blockqueue_wake(&m->spk_queue);
	// 	wtk_thread_join(&m->speaker_t);
	// }
	if(m->cfg->use_line_in && m->linein_run){
		m->linein_run = 0;
		wtk_blockqueue_wake(&m->linein_queue);
		wtk_thread_join(&m->linein_t);
	}
	if(m->usbaudio_queue.length > 0){
		qtk_mod_am13e2_clean_queue(m, &m->usbaudio_queue);
	}
	if(m->lineout_queue.length > 0){
		qtk_mod_am13e2_clean_queue(m, &m->lineout_queue);
	}
	// if(m->spk_queue.length > 0){
	// 	qtk_mod_am13e2_clean_queue(m, &m->spk_queue);
	// }
	if(m->array_vbox_queue.length > 0){
		qtk_mod_am13e2_clean_queue(m, &m->array_vbox_queue);
	}
}

void qtk_mod_am13e2_start(qtk_mod_am13e2_t *m, int is_record)
{
	qtk_msg_node_t *msg_node;
	m->use_record = 1;
	m->is_output=1;
	m->is_outputstart=0;
	if(m->sttimer){
		qtk_timer_start(m->sttimer);
	}
	if(m->gainnetbf){
		m->gainnet_run=1;
		wtk_thread_start(&m->gainnet_t);
	}
	if(m->gainnetbf2){
		m->gainnet2_run=1;
		wtk_thread_start(&m->gainnet2_t);
	}
	if(m->gainnetbf3){
		m->gainnet3_run=1;
		wtk_thread_start(&m->gainnet3_t);
	}
	if(m->mic_check_rcd){
		wtk_mic_check_start(m->mic_check_rcd);
		m->mic_check_rcd_run = 1;
		wtk_thread_start(&m->mic_check_rcd_t);
	}
	if(m->mic_check_play){
		wtk_mic_check_start(m->mic_check_play);
		m->mic_check_play_run = 1;
		wtk_thread_start(&m->mic_check_play_t);
	}
	if(m->vboxebf){
		qtk_vboxebf_start(m->vboxebf);
		m->vbox_run=1;
		wtk_thread_start(&m->vbox_t);
	}
	if(m->cfg->use_array){
		qtk_vboxebf_start(m->avboxebf);
		m->array_vbox_run=1;
		wtk_thread_start(&m->array_vbox_t);
	}
	if(m->cfg->use_line_in && m->linein_run == 0){
		qtk_vboxebf_start(m->denoisebf);
		m->denoise_vbox_run=1;
		m->linein_run = 1;
		wtk_thread_start(&m->denoise_vbox_t);
		wtk_thread_start(&m->linein_t);
	}

#ifndef USE_3308
	if(m->cfg->use_out_resample && (m->cfg->usbaudio.sample_rate != 48000)){
		wtk_resample_start(m->outresample, 48000, m->cfg->usbaudio.sample_rate);
	}
#endif
	wtk_debug("----------------------------------\n");
	
	if(m->cfg->use_linein_check){
		m->linein_check_run=1;
		wtk_thread_start(&m->linein_check_t);
	}

	wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(m->is_output){
		if(m->cfg->use_usbaudio && m->usbaudio_run==0){
			qtk_play_start(m->usbaudio);
			m->usbaudio_run = 1;
			wtk_thread_start(&m->usbaudio_t);
			wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		}
		wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		if(m->cfg->use_lineout && m->lineout_run==0){
			qtk_play_start(m->lineout);
			m->lineout_run = 1;
			wtk_thread_start(&m->lineout_t);
			wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		}
		// if(m->cfg->use_speaker && m->speaker_run==0){
		// 	qtk_play_start(m->speaker);
		// 	m->speaker_run = 1;
		// 	wtk_thread_start(&m->speaker_t);
		// 	wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		// }
		m->is_output=0;
	}
	wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(m->use_record){
		if(m->cfg->debug){
			wtk_debug("==================# qtk_mod_am13e2_RECORD_START #=========>>>>\n");
		}
		wtk_log_log0(m->log,"==================# qtk_mod_am13e2_RECORD_START #=========>>>>");
		m->use_record=0;
		qtk_mod_am13e2_record_start(m);
	}
#if 0 //def USE_3308
	wtk_thread_start(&mem2_t);
#endif
	if(m->cfg->debug){
		wtk_debug("qtk_mod_am13e2 start\n");
	}
	wtk_log_log0(m->log,"qtk_mod_am13e2 start");
	return;
}

void qtk_mod_am13e2_stop(qtk_mod_am13e2_t *m)
{
	wtk_debug("======================>>>>>>>>>>>>>>.stop\n");

	if(m->linein_check_run = 1 && m->cfg->use_linein_check){
		m->linein_check_run = 0;
		wtk_thread_join(&m->linein_check_t);
	}

	if(m->is_output == 0){
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
		// if(m->cfg->use_speaker){
		// 	m->speaker_run = 0;
		// 	wtk_blockqueue_wake(&m->spk_queue);
		// 	wtk_thread_join(&m->speaker_t);
		// }
		m->is_output=1;
	}
	if(m->use_record == 0){
		if(m->cfg->debug){
			wtk_debug("==================#### qtk_mod_am13e2_RECORD_STOP ###=========>>>>\n");
		}
		wtk_log_log0(m->log,"==================#### qtk_mod_am13e2_RECORD_STOP ###=========>>>>");
		qtk_mod_am13e2_record_stop(m);
		m->use_record=1;
	}

#ifndef USE_3308
	if(m->cfg->use_out_resample){
		wtk_resample_close(m->outresample);
	}
#endif

	if(m->vbox_run == 1){
		m->vbox_run=0;
		wtk_blockqueue_wake(&m->vbox_queue);
		wtk_thread_join(&m->vbox_t);
	}

	if(m->cfg->use_array){
		if(m->array_vbox_run){
			m->array_vbox_run=0;
			wtk_blockqueue_wake(&m->array_vbox_queue);
			wtk_thread_join(&m->array_vbox_t);
		}
	}
	if(m->cfg->use_line_in){
		if(m->denoise_vbox_run){
			m->denoise_vbox_run=0;
			wtk_blockqueue_wake(&m->denoise_vbox_queue);
			wtk_thread_join(&m->denoise_vbox_t);
		}
	}
	if(m->gainnet_run == 1){
		m->gainnet_run=0;
		wtk_blockqueue_wake(&m->gainnet_queue);
		wtk_thread_join(&m->gainnet_t);
	}
	if(m->gainnet2_run == 1){
		m->gainnet2_run=0;
		wtk_blockqueue_wake(&m->gainnet2_queue);
		wtk_thread_join(&m->gainnet2_t);
	}
	if(m->gainnet3_run == 1){
		m->gainnet3_run=0;
		wtk_blockqueue_wake(&m->gainnet3_queue);
		wtk_thread_join(&m->gainnet3_t);
	}
	if(m->mic_check_rcd_run == 1){
		m->mic_check_rcd_run=0;
		wtk_blockqueue_wake(&m->mic_check_rcd_queue);
		wtk_thread_join(&m->mic_check_rcd_t);
	}
	if(m->mic_check_play_run == 1){
		m->mic_check_play_run=0;
		wtk_blockqueue_wake(&m->mic_check_play_queue);
		wtk_thread_join(&m->mic_check_play_t);
	}
	if(m->sttimer){
		qtk_timer_stop(m->sttimer);
	}
#if 0 //def USE_3308
	wtk_thread_join(&mem2_t);
#endif
	wtk_debug("======================>>>>>>>>>>>>>>.stop\n");
	return;
}

void qtk_mod_am13e2_start2(qtk_mod_am13e2_t *m, int sample_rate)
{
	if(sample_rate != 48000)
	{
		m->cfg->use_out_resample=1;
	}else{
		m->cfg->use_out_resample=0;
	}
	m->cfg->usbaudio.sample_rate=sample_rate;
	wtk_debug("================================>>>>>>>>>>>>>>>rate=%d\n",m->cfg->usbaudio.sample_rate);
	if(m->cfg->use_out_resample)
	{
		if(m->cfg->use_log_wav || m->log_audio)
		{
			if(m->playwav)
			{
				wtk_wavfile_close(m->playwav);
				wtk_wavfile_delete(m->playwav);
				m->playwav=NULL;
			}
			int channel;
			m->playwav = wtk_wavfile_new(sample_rate); 
			m->playwav->max_pend = 0;
			channel = m->cfg->usbaudio.channel;
			wtk_wavfile_set_channel2(m->playwav, channel, 2);
			wtk_wavfile_open(m->playwav, m->play_path->data);
		}
#if (defined USE_3308)
		wtk_resample_start(m->outresample, 48000, sample_rate);
#else
		wtk_resample_start(m->outresample, 48000, m->cfg->usbaudio.sample_rate);
#endif
	}
	m->player_run=1;
}

void qtk_mod_am13e2_stop2(qtk_mod_am13e2_t *m)
{
	m->player_run=0;
	if(m->cfg->use_out_resample)
	{
		wtk_resample_close(m->outresample);
	}
	m->is_player_start=0;
}

void qtk_mod_am13e2_clean_queue2(qtk_mod_am13e2_t *m, wtk_blockqueue_t *queue,int nx)
{
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	int len=queue->length;

	if(nx>0){
		while(queue->length>nx){
			qn= wtk_blockqueue_pop(queue, 0,NULL);
			if(!qn) {break;}
			msg_node = data_offset2(qn,qtk_msg_node_t,qn);
			qtk_msg_push_node(m->msg, msg_node);
		}
	}else{
		int i;
		for(i=0;i<len;++i)
		{
			qn= wtk_blockqueue_pop(queue, 1,NULL);
			if(!qn) {continue;}
			msg_node = data_offset2(qn,qtk_msg_node_t,qn);
			qtk_msg_push_node(m->msg, msg_node);
		}
	}
}

void qtk_mod_am13e2_clean_queue(qtk_mod_am13e2_t *m, wtk_blockqueue_t *queue)
{
	qtk_mod_am13e2_clean_queue2(m,queue,0);
}

void qtk_mod_am13e2_player_mode(qtk_mod_am13e2_t *m, char *data, int len)
{
	if(len <= 0){return;}
	int poslen=len>>1<<1;

	if(m->cfg->use_usbaudio && m->usbaudio_run){
		qtk_msg_node_t *msg_node;
		// wtk_debug("================================++>>>>>>>>>>>>>>>>>>>...\n");
		msg_node = qtk_msg_pop_node(m->msg);
		// wtk_debug("================================++>>>>>>>>>>>>>>>>>>>...\n");
		if(m->cfg->usbaudio.channel > 1){
			int i=0,j=0;
			while(i<poslen){
				j=0;
				for(j=0;j<m->cfg->usbaudio.channel;++j)
				{
					wtk_strbuf_push(msg_node->buf, data+i, 2);
				}
				i+=2;
			}
		}else{
			// wtk_debug("================================++>>>>>>>>>>>>>>>>>>>...\n");
			wtk_strbuf_push(msg_node->buf, data, poslen);
			// wtk_debug("================================++>>>>>>>>>>>>>>>>>>>...\n");
		}
		// wtk_debug("================================++>>>>>>>>>>>>>>>>>>>...\n");
		wtk_blockqueue_push(&m->usbaudio_queue, &msg_node->qn);
		// wtk_debug("------------==============>>>>>>>>>>m->usbaudio_queue.length = %d\n",m->usbaudio_queue.length);
	}
}

char* pv = NULL;

void qtk_mod_am13e2_player2_mode(qtk_mod_am13e2_t *m, char *data, int len) //lineout
{
	// wtk_debug("--------------------------------<>>>>>>>>>>>\n");
    if(pv == NULL){
		pv = (short *)wtk_malloc(sizeof(short) * 1 * len);
	}
	// qtk_msg_node_t *spk_check;
	// spk_check = qtk_msg_pop_node(m->msg);
	// wtk_strbuf_push(spk_check->buf, data, len);
	// wtk_blockqueue_push(&m->mic_check_play_queue, &spk_check->qn);
	qtk_msg_node_t *spk_check;
	spk_check = qtk_msg_pop_node(m->msg);
	spk_check->type = qtk_mod_am13e2_spk2;
	wtk_strbuf_push(spk_check->buf, data, len);
	wtk_blockqueue_push(&m->mic_check_play_queue, &spk_check->qn);
	memcpy(pv,data,len);
	int src_pos=0;
	if(len <= 0){return;}
	int poslen=len>>1<<1;
	// int poslen =len;
	if(m->cfg->use_lineout && m->lineout_run){
		// wtk_debug("===============>>>>>>>>>>len=%d poslen=%d\n",len,poslen);
		qtk_process_100hz_lpf((short *)pv,poslen>>1,&lpf);
		qtk_msg_node_t *msg_node;
		char zdata[32]={0};
		msg_node = qtk_msg_pop_node(m->msg);
		if(m->cfg->lineout.channel > 1){
			int i=0;
			while(i<poslen){
				// wtk_strbuf_push(msg_node->buf, zdata, 4);
				// wtk_strbuf_push(msg_node->buf, zdata, 4); 
				if (m->cfg->use_spkout == 1) {
					if(m->cfg->use_speaker_left){
						wtk_strbuf_push(msg_node->buf, data + i, 2);
					}else{
						wtk_strbuf_push(msg_node->buf, zdata, 2);
					}
					if(m->cfg->use_speaker_right){
						wtk_strbuf_push(msg_node->buf, data + i+2, 2);
					}else{
						wtk_strbuf_push(msg_node->buf, zdata, 2);
					}
					
				} else {
					wtk_strbuf_push(msg_node->buf, zdata, 4);
				}
				if (m->cfg->use_wooferout == 1) {
					wtk_strbuf_push(msg_node->buf, data + i, 2); 
					wtk_strbuf_push(msg_node->buf, data + i+2, 2); 
					// wtk_strbuf_push(msg_node->buf, data + i, 2); 
				} else {
					wtk_strbuf_push(msg_node->buf, zdata, 4); 
				}
				if (m->cfg->use_headset && m->cfg->use_wooflineout) {
					wtk_strbuf_push(msg_node->buf, pv + i, 2);
					wtk_strbuf_push(msg_node->buf, pv + i+2, 2);
				}else if(m->cfg->use_headset){
					wtk_strbuf_push(msg_node->buf, data + i, 2);
					wtk_strbuf_push(msg_node->buf, data + i+2, 2);
				}
				i += 4; 
			}
		}
		wtk_blockqueue_push(&m->lineout_queue, &msg_node->qn);
	}
}
void qtk_mod_am13e2_player3_mode(qtk_mod_am13e2_t *m, char *data, int len) //speaker
{
	// wtk_debug("------------------>>>>len=%d\n",len);
    // if(pv == NULL){
	// 	pv = (short *)wtk_malloc(sizeof(short) * 1 * len);
	// }
	// double player_start = time_get_ms();
	// memcpy(pv,data,len);
	qtk_msg_node_t *spk_check;
	spk_check = qtk_msg_pop_node(m->msg);
	spk_check->type = qtk_mod_am13e2_spk2;
	wtk_strbuf_push(spk_check->buf, data, len);
	wtk_blockqueue_push(&m->mic_check_play_queue, &spk_check->qn);
	int src_pos=0;
	if(len <= 0){return;}
	int poslen=len>>1<<1;
	// int poslen =len;
	// wtk_debug("--------------->>>use_lineout=%d,use_speaker=%d,use_lineout_queue.length=%d,use_spkout=%d,use_speaker_left=%d,use_speaker_right=%d,use_wooferout=%d,lineout.channel=%d\n",m->cfg->use_lineout,m->cfg->use_speaker,m->lineout_queue.length,m->cfg->use_spkout,m->cfg->use_speaker_left,m->cfg->use_speaker_right,m->cfg->use_wooferout,m->cfg->lineout.channel);
	if(m->cfg->use_lineout && m->cfg->use_speaker){
		// qtk_process_100hz_lpf((short *)pv,poslen>>1,&lpf);
		qtk_msg_node_t *msg_node;
		char zdata[32]={0};
		msg_node = qtk_msg_pop_node(m->msg);
		if(m->cfg->lineout.channel > 1){
			int i=0;
			while(i<poslen){
				if (m->cfg->use_spkout == 1) {
					if(m->cfg->use_speaker_left){
						wtk_strbuf_push(msg_node->buf, data + i, 2);
					}else{
						wtk_strbuf_push(msg_node->buf, zdata, 2);
					}
					if(m->cfg->use_speaker_right){
						wtk_strbuf_push(msg_node->buf, data + i+2, 2);
					}else{
						wtk_strbuf_push(msg_node->buf, zdata, 2);
					}
				} else {
					wtk_strbuf_push(msg_node->buf, zdata, 4);
				}
				if (m->cfg->use_wooferout == 1) {
					wtk_strbuf_push(msg_node->buf, data + i, 2); 
					wtk_strbuf_push(msg_node->buf, data + i+2, 2); 
					// wtk_strbuf_push(msg_node->buf, data + i, 2); 
				} else {
					wtk_strbuf_push(msg_node->buf, zdata, 4); 
				}
				wtk_strbuf_push(msg_node->buf, zdata, 4);
				i += 4; 
			}
		}
		wtk_blockqueue_push(&m->lineout_queue, &msg_node->qn);
		// double player_time = time_get_ms() - player_start;
		//  wtk_debug("---------------->>>>>>>play_time=%.3fms, lineout_queue_len=%d\n", player_time, m->lineout_queue.length);
	}
}
int qtk_mod_am13e2_gainnet_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	wtk_debug("-------------------------------------------------->>>>\n");
	if(m->cfg->use_out_mode == 2){
		qtk_mod_am13e2_set_cpu(m, t, 1);
	}else{
		qtk_mod_am13e2_set_cpu(m, t, 0);
	}
	double total_processing_time = 0.0;
    double total_audio_time = 0.0;
    int frame_count = 0;
    double frame_duration = (double)m->cfg->rcd.buf_time / 1000.0; // 音频帧时长(秒)
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *lineinbuf,*arraybuf,*tbuf,*lineintouac;
	int ret;
	int pos=0;//m->cfg->gainnetbf_cfg->mix_speech_cfg->wins;
	pos=m->cfg->rcd.buf_time*m->cfg->rcd.sample_rate*2/1000;
	pos = m->cfg->gainnetbf_cfg->rtjoin2_cfg->wins;
	int inlen;
	int outlen;
	int channel=2;
	char *out=(char *)wtk_malloc(pos*2);
	char *outresample=(char *)wtk_malloc(4096);
	memset(outresample, 0, 4096);
	// FILE * gainnet_fn;
	// gainnet_fn=fopen("/tmp/gainnet.pcm","wb");
	lineinbuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(lineinbuf);
	lineintouac = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(lineinbuf);
	tbuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(tbuf);
	arraybuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(arraybuf);
	qtk_gainnetbf_start(m->gainnetbf);
	int lineincnt=0;

	while(m->gainnet_run){
		double start_time = time_get_ms(); 
		qn= wtk_blockqueue_pop(&m->gainnet_queue,-1,NULL);
		// wtk_debug("----------------------------------\n");
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(m->cfg->debug){
			if(m->gainnet_queue.length > 10){
				wtk_debug("--------------------->>>>>>>>>gainnet_queue length=%d\n",m->gainnet_queue.length);
			}
		}
		switch (msg_node->type)
		{
		// case qtk_mod_am13e2_DATA_STUDENT_BF3A:
		// 	wtk_strbuf_push(sbuf3a, msg_node->buf->data, msg_node->buf->pos);
		// 	break;
		case qtk_mod_am13e2_DATA_ARRAY:
			// wtk_debug("----------------------------------->>>>>>>>>\n");
			wtk_strbuf_push(arraybuf, msg_node->buf->data, msg_node->buf->pos);
			break;
		case qtk_mod_am13e2_DATA_LINEIN_MIC_TOUAC:
			// wtk_debug("----------------------------------->>>>>>>>>\n");
			wtk_strbuf_push(lineinbuf, msg_node->buf->data, msg_node->buf->pos);
			break;
		case qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE_TOUAC:
			// wtk_debug("----------------------------------->>>>>>>>>\n");
			wtk_strbuf_push(lineintouac, msg_node->buf->data, msg_node->buf->pos);
			break;	
		default:
			break;
		}
		// wtk_debug("==>use_array=%d use_wx3a=%d sbuf3a->pos-%d\n tbuf3a=%d arraybuf=%d wxbuf=%d\n",m->cfg->use_array,m->cfg->use_wx3a,sbuf3a->pos, tbuf3a->pos, arraybuf->pos, wxbuf->pos);
		if(m->cfg->use_array == 1 && m->cfg->use_linein_mic && m->cfg->use_linein_courseware_touac == 0){
			// wtk_debug("-----------------------arraybuf->pos=%d ,lineinbuf->pos=%d\n",arraybuf->pos,lineinbuf->pos);
			if(arraybuf->pos >= pos && lineinbuf->pos >= pos){
				wtk_strbuf_reset(tbuf);
				int i=0;
				while(i<pos){
					wtk_strbuf_push(tbuf, arraybuf->data+i, 2);
					wtk_strbuf_push(tbuf, lineinbuf->data+i, 2);
					i+=2;
				}
				qtk_gainnetbf_feed(m->gainnetbf, tbuf->data, tbuf->pos, 0);

				wtk_strbuf_pop(lineinbuf, NULL, pos);
				wtk_strbuf_pop(arraybuf, NULL, pos);
			}
			if(lineintouac->pos > 0){
				wtk_strbuf_reset(lineintouac);
			}
		}else if(m->cfg->use_array == 1 && m->cfg->use_linein_mic == 0 && m->cfg->use_linein_courseware_touac == 0){
			if(arraybuf->pos >= pos){
				wtk_strbuf_reset(tbuf);
				int i=0,j=0;
				while(i<pos){
					for(j=0;j<2;++j)
					{
						wtk_strbuf_push(tbuf, arraybuf->data+i, 2);
					}
					i+=2;
				}
				qtk_gainnetbf_feed(m->gainnetbf, tbuf->data, tbuf->pos, 0);
				wtk_strbuf_pop(arraybuf, NULL, pos);
			}
		}else if(m->cfg->use_array == 1 && m->cfg->use_linein_mic == 0 && m->cfg->use_linein_courseware_touac == 1){
			if(arraybuf->pos >= pos && lineintouac->pos >=pos){
				wtk_strbuf_reset(tbuf);
				int i=0,j=0;
				while(i<pos){
					wtk_strbuf_push(tbuf, arraybuf->data+i, 2);
					wtk_strbuf_push(tbuf, lineintouac->data+i, 2);
					i+=2;
				}
				qtk_gainnetbf_feed(m->gainnetbf, tbuf->data, tbuf->pos, 0);
				
				wtk_strbuf_pop(arraybuf, NULL, pos);
				wtk_strbuf_pop(lineintouac, NULL, pos);
			}
		}
		qtk_msg_push_node(m->msg, msg_node);
	}
	// fclose(gainnet_fn);
	qtk_gainnetbf_feed(m->gainnetbf, NULL, 0, 1);
	qtk_gainnetbf_reset(m->gainnetbf);
	wtk_strbuf_delete(lineintouac);
	wtk_strbuf_delete(tbuf);
	wtk_strbuf_delete(lineinbuf);
	wtk_strbuf_delete(arraybuf);
	wtk_free(out);
	wtk_free(outresample);
	return 0;
}
int qtk_mod_am13e2_gainnet2_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	if(m->cfg->use_out_mode == 2){
		qtk_mod_am13e2_set_cpu(m, t, 1);
	}else{
		qtk_mod_am13e2_set_cpu(m, t, 0);
	}
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *i2sbuf,*uacbuf,*tbuf,*lineincourseware,*lineinmic,*ttbuf;
	int ret;
	int pos=0;//m->cfg->gainnetbf_cfg->mix_speech_cfg->wins;
	pos=m->cfg->rcd.buf_time*m->cfg->rcd.sample_rate*2/1000;
	pos = m->cfg->gainnetbf_cfg->rtjoin2_cfg->wins;
	int inlen;
	int outlen;
	
	int channel=2;
	char *out=(char *)wtk_malloc(pos*2);
	char *outresample=(char *)wtk_malloc(4096);
	memset(outresample, 0, 4096);

	i2sbuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(i2sbuf);
	tbuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(tbuf);
	ttbuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(ttbuf);
	uacbuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(uacbuf);
	lineincourseware = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(lineincourseware);
	lineinmic = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(lineinmic);
	qtk_gainnetbf_start(m->gainnetbf2);
	qtk_gainnetbf_start(m->gainnetbf4);
	int lineincnt=0;
	double wait_start_time = 0;
    int is_waiting = 0;
	char zdata[32]={0};
	// FILE * gainnet2_fn;
	// gainnet2_fn=fopen("/tmp/gainnet2.pcm","wb");
	// if(gainnet2_fn==NULL){
	// 	wtk_debug("fopen filed!!\n");
	// 	exit(1);
	// }
	while(m->gainnet2_run){
		qn= wtk_blockqueue_pop(&m->gainnet2_queue,-1,NULL);
		
		// wtk_debug("-----------------------m->cfg->use_linein_courseware =%d\n",m->cfg->use_linein_courseware);
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(!m->cfg->use_mainlineout && !m->cfg->use_wooflineout && !m->cfg->use_meetinglineout && !m->cfg->use_expandlineout){
			switch (msg_node->type)
			{
			
			case qtk_mod_am13e2_DATA_IIS:
				wtk_strbuf_push(i2sbuf, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE:
				wtk_strbuf_push(lineincourseware, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_UAC:
				wtk_strbuf_push(uacbuf, msg_node->buf->data, msg_node->buf->pos);
				break;
			default:
				break;
			}
			// wtk_debug("-------->>>>>i2sbuf.pos=%d,lineincourseware.pos=%d,uacbuf.pos=%d,m.rcd3=%d,linein_cource=%d\n",i2sbuf->pos,lineincourseware->pos,uacbuf->pos,m->cfg->use_rcd3,m->cfg->use_linein_courseware);
			if(m->cfg->use_rcd4 == 1 && m->cfg->use_linein_courseware && i2sbuf->pos == 0){
				// wtk_debug("--------------------------->>>>.lineincourseware->pos =%d,uacbuf->pos = %d\n",lineincourseware->pos,uacbuf->pos);
				if((lineincourseware->pos >= pos && uacbuf->pos >= pos) || (lineincourseware->pos > 2*pos)) {
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						if(uacbuf->pos > 0){
							wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						}else{
							wtk_strbuf_push(tbuf, zdata, 2);
						}
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						i+=2;
					}
					// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
					qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					wtk_strbuf_pop(lineincourseware, NULL, pos);
					if(uacbuf->pos >= pos) {
						wtk_strbuf_pop(uacbuf, NULL, pos);
					}
				}
			}
			else if (m->cfg->use_rcd3 == 1 && m->cfg->use_linein_courseware && uacbuf->pos == 0 && i2sbuf->pos !=0 ){
				if(i2sbuf->pos >= pos && lineincourseware->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						// wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						i+=2;
					}				
					qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
					wtk_strbuf_pop(i2sbuf, NULL, pos);
					wtk_strbuf_pop(lineincourseware, NULL, pos);
				}
			}
			else if (m->cfg->use_rcd3 == 1 && m->cfg->use_linein_courseware==0 && uacbuf->pos == 0){
				// wtk_debug("-------=========================>>>>>>>>>>>\n");
				if(i2sbuf->pos >= pos){
					wtk_strbuf_reset(tbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						// wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
						// wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
						// wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
						i+=4;
					}
					// gainnet_tm=time_get_ms();
					// double feed_start = time_get_ms();
					// wtk_debug("-------------------->>>>>>>>>>>>>>tbuf->pos=%d\n",tbuf->pos);
					qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);
					// qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					// qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
					// double feed_time = time_get_ms() - feed_start;
					// wtk_debug("----------gainnet2_feedtime=%.3f,tbuf->pos =%d \n",feed_time,tbuf->pos);
					wtk_strbuf_pop(i2sbuf, NULL, pos);
				}
			}
			else if (m->cfg->use_rcd4 == 1 && m->cfg->use_linein_courseware==0 && i2sbuf->pos == 0){
				// wtk_debug("------------------------------------->>>>>>>>>>>>>>.\n");
				if(uacbuf->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						i+=2;
					}
					qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					// wtk_debug("----------------------------------tbuf->pos =%d\n",tbuf->pos);
					wtk_strbuf_pop(uacbuf, NULL, pos);
				}
			}
			else if (m->cfg->use_rcd4 == 0 && m->cfg->use_linein_courseware==1){
				// wtk_debug("------------------------------------->>>>>>>>>>>>>>.\n");
				if(lineincourseware->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);

						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						i+=2;
					}
					qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);
					wtk_strbuf_pop(lineincourseware, NULL, pos);
				}
			}
		}
		qtk_msg_push_node(m->msg, msg_node);
		// fclose(gainnet2_fn);
	}
	qtk_gainnetbf_feed(m->gainnetbf2, NULL, 0, 1);
	qtk_gainnetbf_reset(m->gainnetbf2);
	
	wtk_strbuf_delete(tbuf);
	wtk_strbuf_delete(lineincourseware);
	wtk_strbuf_delete(i2sbuf);
	wtk_strbuf_delete(uacbuf);
	wtk_strbuf_delete(lineinmic);
	wtk_free(out);
	wtk_free(outresample);
    return 0;
}
int qtk_mod_am13e2_gainnet3_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	if(m->cfg->use_out_mode == 2){
		qtk_mod_am13e2_set_cpu(m, t, 1);
	}else{
		qtk_mod_am13e2_set_cpu(m, t, 0);
	}
	qtk_msg_node_t *msg_node,*msg_node2;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *i2sbuf,*uacbuf,*tbuf,*ttbuf,*lineincourseware,*lineinmic,*arraybuf;
	int ret;
	int pos=0;//m->cfg->gainnetbf_cfg->mix_speech_cfg->wins;
	pos=m->cfg->rcd.buf_time*m->cfg->rcd.sample_rate*2/1000;
	pos = m->cfg->gainnetbf_cfg->rtjoin2_cfg->wins;
	int inlen;
	int outlen;
	int channel=2;
	char *out=(char *)wtk_malloc(pos*2);
	char *outresample=(char *)wtk_malloc(4096);
	memset(outresample, 0, 4096);
	arraybuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(arraybuf);
	i2sbuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(i2sbuf);
	tbuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(tbuf);
	ttbuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(ttbuf);
	uacbuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(uacbuf);
	lineincourseware = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(lineincourseware);
	lineinmic = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(lineinmic);
	qtk_gainnetbf_start(m->gainnetbf3);
	int lineincnt=0;
	double wait_start_time = 0;
    int is_waiting = 0;
	char zdata[32]={0};
	while(m->gainnet3_run){
		qn= wtk_blockqueue_pop(&m->gainnet3_queue,-1,NULL);
		// wtk_debug("-----------------------==================================\n");
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		// wtk_debug("i2sbuf.pos=%d,lineincourseware.pos=%d,uabuf.pos=%d,lineinmic.pos=%d,araybuf.pos=%d\n",i2sbuf->pos,lineincourseware->pos,uacbuf->pos,lineinmic->pos,arraybuf->pos);
		// wtk_debug("m->cfg->use_mainlineout=%d\n",m->cfg->use_mainlineout);
		if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout){
			switch (msg_node->type)
			{
			case qtk_mod_am13e2_DATA_IIS:
				wtk_strbuf_push(i2sbuf, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_LINEIN_courseware_TOLINEOUT:
				wtk_strbuf_push(lineincourseware, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_UAC:
				wtk_strbuf_push(uacbuf, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_LINEIN_MIC_TOLINEOUT:
				wtk_strbuf_push(lineinmic, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_ARRAY_TOlINEOUT:
				wtk_strbuf_push(arraybuf, msg_node->buf->data, msg_node->buf->pos);
				break;
			default:
				break;
			}
			wtk_debug("-------->>>>>arraymul.pos=%d,i2sbuf.pos=%d,lineincourseware.pos=%d,uacbuf.pos=%d,linein_cource=%d\n",arraybuf->pos,i2sbuf->pos,lineincourseware->pos,uacbuf->pos,m->cfg->use_linein_courseware);

			if(m->cfg->use_meetinglineout){
				if(m->cfg->use_linein_courseware == 0 && m->cfg->use_linein_mic == 0){
					if(i2sbuf->pos >= pos && arraybuf->pos >= pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
							i+=2;
						}
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}
				else if(m->cfg->use_linein_courseware && i2sbuf->pos ==0){
					if((lineincourseware->pos >= pos && uacbuf->pos >= pos && arraybuf->pos >= pos) || (lineincourseware->pos > 2*pos)) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							if(uacbuf->pos > 0){
								wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
							}else{
								wtk_strbuf_push(tbuf, zdata, 2);
							}
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							i+=2;
						}
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						wtk_strbuf_pop(lineincourseware, NULL, pos);
						if(uacbuf->pos >= pos) {
							wtk_strbuf_pop(uacbuf, NULL, pos);
						}
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->cfg->use_linein_courseware && uacbuf->pos ==0){
					if((lineincourseware->pos >= pos && i2sbuf->pos >= pos && arraybuf->pos >= pos)) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
							i+=2;
						}
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(lineincourseware, NULL, pos);
						wtk_strbuf_pop(i2sbuf, NULL, pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->cfg->use_linein_mic&& i2sbuf->pos ==0){
					if((lineincourseware->pos >= pos && uacbuf->pos >= pos && arraybuf->pos >= pos) || (lineincourseware->pos > 2*pos)) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							if(uacbuf->pos > 0){
								wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
							}else{
								wtk_strbuf_push(tbuf, zdata, 2);
							}
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							i+=2;
						}
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						wtk_strbuf_pop(lineinmic, NULL, pos);
						if(uacbuf->pos > 0)
							wtk_strbuf_pop(uacbuf, NULL, pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->cfg->use_linein_mic && uacbuf->pos ==0){
					if((lineinmic->pos >= pos && i2sbuf->pos >= pos && arraybuf->pos >= pos)) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
							// wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(tbuf, lineinmic->data+i, 2);
							i+=2;
						}
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						wtk_strbuf_pop(lineinmic, NULL, pos);
						wtk_strbuf_pop(i2sbuf, NULL, pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}
			}
			else if(m->cfg->use_linein_courseware && i2sbuf->pos == 0){
				// wtk_debug("--------------------------->>>>.lineincourseware->pos =%d,uacbuf->pos = %d\n",lineincourseware->pos,uacbuf->pos);
				if((lineincourseware->pos >= pos && uacbuf->pos >= pos) || (lineincourseware->pos > 2*pos)) {
					
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						if(uacbuf->pos > 0){
							wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						}else{
							wtk_strbuf_push(tbuf, zdata, 2);
						}
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						i+=2;
					}
					
					// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
					qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
					// wtk_debug("----------------------------------tbuf->pos =%d\n",tbuf->pos);
					wtk_strbuf_pop(lineincourseware, NULL, pos);
					if(uacbuf->pos >= pos) {
						wtk_strbuf_pop(uacbuf, NULL, pos);
					}
				} 
			}
			else if ( m->cfg->use_linein_courseware && uacbuf->pos == 0){
				// wtk_debug("---------->>>>>>>>>>>>>>i2sbuf->pos=%d,lineincourseware->pos=%d,m->cfg->use_linein_courseware=%d\n",i2sbuf->pos,lineincourseware->pos,m->cfg->use_linein_courseware);
				if(i2sbuf->pos >= pos && lineincourseware->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						// wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						i+=4;
					}
					qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);

					wtk_strbuf_pop(i2sbuf, NULL, pos);
					wtk_strbuf_pop(lineincourseware, NULL, pos);
				}
			}
			else if ( m->cfg->use_linein_courseware==0 && uacbuf->pos == 0){
				// wtk_debug("---------->>>>>>>>>>>>>>i2sbuf->pos=%d,lineincourseware->pos=%d,m->cfg->use_linein_courseware=%d\n",i2sbuf->pos,lineincourseware->pos,m->cfg->use_linein_courseware);
				if(i2sbuf->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
						i+=4;
					}
					// wtk_debug("----------------------_>>>>>>\n");
					qtk_mod_am13e2_player2_mode(m,tbuf->data,tbuf->pos);
					// wtk_debug("-------=========================>>>>>>>>>>>\n");
					wtk_strbuf_pop(i2sbuf, NULL, pos);
				}
			}
			else if (m->cfg->use_linein_courseware==0 && i2sbuf->pos == 0){
				// wtk_debug("------------------------------------->>>>>>>>>>>>>>.\n");
				if(uacbuf->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						i+=2;
					}
					qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
					// wtk_debug("----------------------------------tbuf->pos =%d\n",tbuf->pos);
					wtk_strbuf_pop(uacbuf, NULL, pos);
				}
			}else if (m->cfg->use_rcd4 == 0 && m->cfg->use_linein_courseware==1){
				// wtk_debug("------------------------------------->>>>>>>>>>>>>>.\n");
				if(lineincourseware->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);

						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						i+=4;
					}
					qtk_mod_am13e2_player2_mode(m,tbuf->data,tbuf->pos);
					wtk_strbuf_pop(lineincourseware, NULL, pos);
			
				}	
			}
		}
		qtk_msg_push_node(m->msg, msg_node);	
	}
    qtk_gainnetbf_feed(m->gainnetbf3, NULL, 0, 1);
    qtk_gainnetbf_reset(m->gainnetbf3);
    
    wtk_strbuf_delete(tbuf);
    wtk_strbuf_delete(lineincourseware);
    wtk_strbuf_delete(i2sbuf);
    wtk_strbuf_delete(uacbuf);
    wtk_strbuf_delete(arraybuf);
    wtk_strbuf_delete(lineinmic);
    wtk_free(out);
    wtk_free(outresample);
    return 0;
}
int qtk_mod_am13e2_vbox_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	
	wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
	qtk_mod_am13e2_set_cpu(m, t, 3);
	qtk_msg_node_t *msg_node, *msg_node2;
	wtk_queue_node_t *qn;

	while(m->vbox_run){
		wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
		qn= wtk_blockqueue_pop(&m->vbox_queue,-1,NULL);
		wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		if(m->cfg->debug){
			if(m->vbox_queue.length > 10){
				wtk_debug("--------------------->>>>>>>>>vbox_queue length=%d\n",m->vbox_queue.length);
			}
		}

		if(m->cfg->use_3abfio){
			qtk_vboxebf_feed(m->vboxebf, msg_node->buf->data, msg_node->buf->pos, 0);
		}

		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_vboxebf_feed(m->vboxebf, NULL, 0, 1);
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
	qtk_vboxebf_reset(m->vboxebf);
	return 0;
}

int qtk_mod_am13e2_array_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t){
	qtk_mod_am13e2_set_cpu(m, t, 0);
	qtk_msg_node_t *msg_node, *msg_node2;
	wtk_queue_node_t *qn;
	while(m->array_vbox_run){
		qn= wtk_blockqueue_pop(&m->array_vbox_queue,-1,NULL);
		// wtk_debug("---------------============>>>>>>>>>m->array_vbox_queue.length= %d\n",m->array_vbox_queue.length);
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		if(m->cfg->debug){
			if(m->array_vbox_queue.length > 10){
				wtk_debug("--------------------->>>>>>>>>array_vbox_queue length=%d\n",m->array_vbox_queue.length);
			}
		}
		double delay_time=time_get_ms();
		qtk_vboxebf_feed(m->avboxebf, msg_node->buf->data, msg_node->buf->pos, 0);
		// wtk_debug("-------------------------->>>>>>>last_time-first_time = %f\n",time_get_ms()-delay_time);
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_vboxebf_feed(m->avboxebf, NULL, 0, 1);
	qtk_vboxebf_reset(m->avboxebf);
	return 0;
}
//mic 检测
int qtk_mod_am13e2_mic_check_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t){
	qtk_mod_am13e2_set_cpu(m, t, 0);
	qtk_msg_node_t *msg_node, *msg_node2;
	wtk_queue_node_t *qn;

	int len;
	float *play_vol = (float *)wtk_malloc(sizeof(float) * m->mic_check_rcd->cfg->nmicchannel);
    for (int i = 0; i < m->mic_check_rcd->cfg->nmicchannel; ++i) {
        play_vol[i] = 1.0;
    }
	int audio_first=1;
	// FILE * mic_check_rcd_mul;
	// mic_check_rcd_mul=fopen("/tmp/mic_check_rcd_mul.pcm","wb");
	// if(mic_check_rcd_mul==NULL){
	// 	wtk_debug("fopen filed!!\n");
	// 	exit(1);
	// }
	while(m->mic_check_rcd_run){
		qn= wtk_blockqueue_pop(&m->mic_check_rcd_queue,-1,NULL);
		// wtk_debug("---------------------->>>>>>>>>>>>>\n")
		// wtk_debug("---------------============>>>>>>>>>m->mic_check_rcd_queue.length= %d,m->mic_check_rcd->cfg->nmicchannel=%d\n",m->mic_check_rcd_queue.length,m->mic_check_rcd->cfg->nmicchannel);
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		if(audio_first)
		{
			audio_check_rcd_tm=time_get_ms();
			audio_first = 0;
		}
		len = msg_node->buf->pos / (sizeof(short) * m->mic_check_rcd->cfg->nmicchannel);
		
		// fwrite(msg_node->buf->data,len,1,mic_check_rcd_mul);
		wtk_mic_check_feed(m->mic_check_rcd, msg_node->buf->data, len, play_vol,0);
		// wtk_debug("------------------_>>>>>>>>>>>>>>len =%d\n",len);
		qtk_msg_push_node(m->msg, msg_node);
		// if(m->audio_check_rcd_feed_end)
		if(time_get_ms()-audio_check_rcd_tm > 6*1000)
		{
			wtk_debug("---------------------->>>>>>>>>>>>>\n");
			wtk_mic_check_feed(m->mic_check_rcd, NULL, 0, play_vol,1);
			wtk_mic_check_reset(m->mic_check_rcd);
			audio_check_rcd_tm=time_get_ms();
		}
	}
	// fclose(mic_check_rcd_mul);
	wtk_mic_check_feed(m->mic_check_rcd, NULL, 0, play_vol,1);
	wtk_mic_check_reset(m->mic_check_rcd);
	return 0;
}
//SPK 检测
int qtk_mod_am13e2_mic_check_play_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t){
	qtk_mod_am13e2_set_cpu(m, t, 0);
	qtk_msg_node_t *msg_node, *msg_node2;
	wtk_queue_node_t *qn;
	wtk_strbuf_t *spk2_buf=NULL;
	wtk_strbuf_t *sp2buf=NULL;
	wtk_strbuf_t *sk2_2spbuf=NULL;
	spk2_buf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(spk2_buf);
	sp2buf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(sp2buf);
	sk2_2spbuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(sk2_2spbuf);
	int len;
	int spk_first=1;
	int pos = m->mic_check_play->cfg->wins;
	float *play_vol = (float *)wtk_malloc(sizeof(float) * m->mic_check_play->cfg->nmicchannel);
    for (int i = 0; i < m->mic_check_play->cfg->nmicchannel; ++i) {
        play_vol[i] = 1.0;
    }
	while(m->mic_check_play_run){
		qn= wtk_blockqueue_pop(&m->mic_check_play_queue,-1,NULL);
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		switch (msg_node->type)
		{
			
			case qtk_mod_am13e2_spk2:
				wtk_strbuf_push(spk2_buf, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_sp2:
				wtk_strbuf_push(sp2buf, msg_node->buf->data, msg_node->buf->pos);
				break;
			default:
				break;
		}
		if(spk2_buf->pos >= pos && sp2buf->pos >= pos)
		{
			wtk_strbuf_reset(sk2_2spbuf);
			int i=0;
			while(i<pos){
				wtk_strbuf_push(sk2_2spbuf, spk2_buf->data+i, 2);
				wtk_strbuf_push(sk2_2spbuf, spk2_buf->data+i+2, 2);
				wtk_strbuf_push(sk2_2spbuf, sp2buf->data+i, 2);
				wtk_strbuf_push(sk2_2spbuf, sp2buf->data+i+2, 2);
				i+=4;
			}
		}
		if(spk_first)
		{
			audio_check_play_tm=time_get_ms();
			spk_first = 0;
		}
		// double delay_time=time_get_ms();
		// wtk_debug("->>>>>m->mic_check_rcd->cfg->nmicchannel=%d\n",m->mic_check_play->cfg->nmicchannel);
		len = sk2_2spbuf->pos / (sizeof(short) * m->mic_check_play->cfg->nmicchannel);
#if 1
		wtk_mic_check_feed(m->mic_check_play,sk2_2spbuf->data, len,play_vol, 0);
		wtk_strbuf_pop(sp2buf,NULL,pos*2); 
		wtk_strbuf_pop(spk2_buf,NULL,pos*2);
		// wtk_debug("-------------------------->>>>>>>last_time-first_time = %f\n",time_get_ms()-delay_time);
		qtk_msg_push_node(m->msg, msg_node);
		if(time_get_ms()-audio_check_play_tm > 6*1000)
		{
			wtk_debug("---------------------->>>>>>>>>>>>>\n");
			wtk_mic_check_feed(m->mic_check_play, NULL, 0, play_vol,1);
			wtk_mic_check_reset(m->mic_check_play);
			audio_check_play_tm=time_get_ms();
		}
#endif
	}
	wtk_mic_check_feed(m->mic_check_play, NULL, 0, play_vol,1);
	wtk_mic_check_reset(m->mic_check_play);
	wtk_strbuf_delete(sk2_2spbuf);
	wtk_strbuf_delete(sp2buf);
	wtk_strbuf_delete(spk2_buf);
	return 0;
}
//linein_mic 降噪算法
// int qtk_mod_am13e2_denoise_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t){
// 	qtk_mod_am13e2_set_cpu(m, t, 0);
// 	qtk_msg_node_t *msg_node, *msg_node2;
// 	wtk_queue_node_t *qn;
// 	while(m->denoise_vbox_run){
// 	wtk_debug("----------------------------------\n");
// 		qn= wtk_blockqueue_pop(&m->denoise_vbox_queue,-1,NULL);
// 		if(!qn) {continue;}
// 		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

// 		if(m->cfg->debug){
// 			if(m->denoise_vbox_queue.length > 10){
// 				wtk_debug("--------------------->>>>>>>>>denoise_vbox_queue length=%d\n",m->denoise_vbox_queue.length);
// 			}
// 		}
// 		double delay_time=time_get_ms();
// 		qtk_vboxebf_feed(m->denoisebf, msg_node->buf->data, msg_node->buf->pos, 0);
// 		qtk_msg_push_node(m->msg, msg_node);
// 	}
// 	qtk_vboxebf_feed(m->denoisebf, NULL, 0, 1);
// 	qtk_vboxebf_reset(m->denoisebf);
// 	return 0;
// }
//双线性插值 重采样
int16_t* _hh25c_a_try_interpolation_supplement(const int16_t *sourceData, int32_t sampleRate, uint32_t srcSize,
                  int32_t newSampleRate, uint32_t *dstSize) 
{
    uint32_t last_pos = srcSize - 1;
    *dstSize = (uint32_t) (srcSize * ((float) newSampleRate / sampleRate));
	int16_t *destinationData = wtk_malloc(*dstSize*2);
    for (uint32_t idx = 0; idx < *dstSize; idx++) {
        float index = ((float) idx * sampleRate) / (newSampleRate);
        uint32_t p1 = (uint32_t) index;
        float coef = index - p1;
        uint32_t p2 = (p1 == last_pos) ? last_pos : p1 + 1;
        destinationData[idx] = (int16_t) ((1.0f - coef) * sourceData[p1] + coef * sourceData[p2]);
    }
	return destinationData;
}

void _mod_hh25c_a_wake_ease_out(int rate,char *data,int len,int inlen)
{
	short *sd = (short*)data;
	int dl = len/2;
	int dlc = dl;
	int i = 0;
	float shlft = 0,t = 0;
	int eitn = 16*(rate/1000);
	int eutn = inlen;

	for(i = 0; i < dlc; ++i){
		if(eutn+i > eitn){
			shlft = 0.0f;
		}else{
			t = (eutn+i)*1.0f/eitn;
			shlft = 1.0f-t*t*t;
		}
		sd[0] *= shlft;
		sd += 1;
	}
	return;
}

int qtk_mod_am13e2_merge_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	wtk_debug("---------------->>>>>>>>>>>>>>>>>qtk_mod_am13e2_merge_rcd_entry\n");
	qtk_mod_am13e2_set_cpu(m, t, 1);
	qtk_msg_node_t *msg_node;
	wtk_queue_node_t *qn;
	qtk_msg_node_t *linein_node;
	qtk_msg_node_t *msg_node2;
	qtk_msg_node_t *msg_bf3anode;
	qtk_msg_node_t *msg_array_node;
	wtk_strbuf_t *sbf3abuf=NULL;
	wtk_strbuf_t *abf3abuf=NULL;
	wtk_strbuf_t **rcdbuf=NULL;
	wtk_strbuf_t *rcd2buf=NULL;
	wtk_strbuf_t *linein_buf=NULL;

	char zdata[64]={0};
	int i,j,pos;
	int ret;
	int channel1=m->mic_channel;
	int channel2=m->cfg->rcd2.channel - m->cfg->rcd2.nskip;
	int aspk_channel=m->cfg->aspk_channel;
	pos=m->cfg->rcd.buf_time*m->cfg->rcd.sample_rate*2/1000;
	wtk_debug("----------------------------------\n");
	sbf3abuf = wtk_strbuf_new(pos*3, 1.0f);
	abf3abuf = wtk_strbuf_new(pos*3, 1.0f);
	linein_buf = wtk_strbuf_new(pos * 2, 1.0f);
	rcdbuf =  (wtk_strbuf_t **)wtk_malloc(sizeof(wtk_strbuf_t *)*channel1);
	for(i=0;i<channel1;++i){
		rcdbuf[i] = wtk_strbuf_new(pos, 1.0f);
		wtk_strbuf_reset(rcdbuf[i]);
	}
	wtk_debug("----------------------------------\n");
	rcd2buf = wtk_strbuf_new(pos*channel2, 1.0f);
	wtk_strbuf_reset(rcd2buf);
	wtk_strbuf_reset(linein_buf);
	wtk_strbuf_reset(sbf3abuf);
	wtk_strbuf_reset(abf3abuf);
	
	while(m->merge_rcd_run){
		qn= wtk_blockqueue_pop(&m->merge_rcd_queue,-1,NULL);
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		// wtk_debug("----------------------->>>>>>>>use_line_in =%d\n",m->cfg->use_line_in);
		wtk_strbuf_reset(linein_buf);
		int src_pos = 0;
		int channel = m->cfg->linein_channel; 
		while(src_pos < msg_node->buf->pos) {
			wtk_strbuf_push(linein_buf, msg_node->buf->data + src_pos + channel*2, 2);
			wtk_strbuf_push(linein_buf, msg_node->buf->data + src_pos + channel*2+2, 2);
			src_pos += channel2 * 2;
		}
		// wtk_debug("---->>>use_linein_out=%d,use_linein_mic=%d,use_linein_courseware=%d\n",m->use_linein_out,m->cfg->use_linein_mic,m->cfg->use_linein_courseware);
		if(m->cfg->use_line_in && m->use_linein_out && msg_node->type == 2 && m->cfg->use_linein_mic==0){
			// wtk_debug("------------------_?>>>>>>>>>>>>>>>>>\n");
			// fwrite(linein_buf->data,linein_buf->pos,1,linein_fn);
			if(m->cfg->use_linein_courseware == 0 && m->cfg->use_linein_courseware_touac){
				if(linein_buf->pos > 0) {
					linein_node = qtk_msg_pop_node(m->msg);
					linein_node->type = qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE_TOUAC;
					wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
					wtk_blockqueue_push(&m->gainnet_queue, &linein_node->qn);
				}
			}else if (m->cfg->use_linein_courseware && m->cfg->use_linein_courseware_touac == 0){
				if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout){
					if(linein_buf->pos > 0){
						linein_node = qtk_msg_pop_node(m->msg);
						linein_node->type = qtk_mod_am13e2_DATA_LINEIN_courseware_TOLINEOUT;
						wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
						wtk_blockqueue_push(&m->gainnet3_queue, &linein_node->qn);
					//  wtk_debug("--------------------_>>>>>>>>>>linein_buff->pos = %d\n",linein_buf->pos);
					}	
				}
				else{
					if(linein_buf->pos > 0) {
						linein_node = qtk_msg_pop_node(m->msg);
						linein_node->type = qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE;
						wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
						wtk_blockqueue_push(&m->gainnet2_queue, &linein_node->qn);
						// wtk_debug("----------------->>>>>>>>>>>>>>>linein_buf->pos=%d\n",linein_buf->pos);
					}
				}
			}
		}else if(m->cfg->use_line_in && m->use_linein_out && msg_node->type == 2 && m->cfg->use_linein_mic) {
			// wtk_debug("------------------_?>>>>>>>>>>>>>>>>>\n");

			if(m->cfg->use_meetinglineout){
				if(linein_buf->pos > 0){
					linein_node = qtk_msg_pop_node(m->msg);
					linein_node->type = qtk_mod_am13e2_DATA_LINEIN_MIC_TOLINEOUT;
					wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
                	wtk_blockqueue_push(&m->gainnet3_queue, &linein_node->qn);
				}	
			}else{
				// fwrite(linein_buf->data,linein_buf->pos,1,linein_fn);
				
				if(linein_buf->pos > 0) {
					linein_node = qtk_msg_pop_node(m->msg);
					wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
					wtk_blockqueue_push(&m->linein_queue, &linein_node->qn);
					// wtk_debug("--------------->>>>>>>m->linein_queue.length=%d\n",m->linein_queue.length);
				}
			}
        }

		if(msg_node->type == 1){
			// wtk_strbuf_push(rcdbuf, msg_node->buf->data, msg_node->buf->pos);
			i=0;
			while(i < msg_node->buf->pos){
				for(j=0;j<channel1;++j){
					wtk_strbuf_push(rcdbuf[j], msg_node->buf->data+i, 2);
					i+=2;

				}
			}
		}else if(msg_node->type == 2){
			wtk_strbuf_push(rcd2buf, msg_node->buf->data, msg_node->buf->pos);
		}
		if(m->cfg->use_array == 1){
			if(rcdbuf[0]->pos >= pos && rcd2buf->pos >= pos*channel2){
				i=0;
				j=0;
				while(i < pos){
					for(int ch=0; ch<8; ch++) {
						wtk_strbuf_push(abf3abuf, rcdbuf[ch]->data+i, 2);
					}

					wtk_strbuf_push(abf3abuf, rcd2buf->data+j, 2);    // 通道0 低频通道
					wtk_strbuf_push(abf3abuf, rcd2buf->data+j+2, 2);  // 通道2 左右取一通道
					wtk_strbuf_push(abf3abuf, rcd2buf->data+j+4, 2);  // 通道7 line_out

					// i+=(channel1*2);
					i+=2;
					j+=(channel2*2);
				}
				// printf("  abf3abuf size after push: %d bytes\n", abf3abuf->pos);
				// msg_bf3anode = qtk_msg_pop_node(m->msg);
				// wtk_strbuf_push(msg_bf3anode->buf, sbf3abuf->data, sbf3abuf->pos);
				// wtk_blockqueue_push(&m->vbox_queue, &msg_bf3anode->qn);
				merge_all += abf3abuf->pos/11.0/96.0;
				msg_array_node = qtk_msg_pop_node(m->msg);
				wtk_strbuf_push(msg_array_node->buf, abf3abuf->data, abf3abuf->pos);
				wtk_blockqueue_push(&m->array_vbox_queue, &msg_array_node->qn);

				wtk_strbufs_pop(rcdbuf, channel1, pos);
				wtk_strbuf_pop(rcd2buf, NULL, pos*channel2);
				wtk_strbuf_pop(linein_buf, NULL, pos*2);
				wtk_strbuf_pop(sbf3abuf, NULL, pos*3);
				wtk_strbuf_pop(abf3abuf, NULL, pos*11);

				wtk_strbuf_reset(sbf3abuf);
				wtk_strbuf_reset(abf3abuf);
				wtk_strbuf_reset(linein_buf);
				// wtk_strbuf_reset(rcd2buf);
				// for(i=0;i<channel1;++i){
				// 	wtk_strbuf_reset(rcdbuf[i]);
				// }
				// wtk_debug("  sbf3abuf size: %d bytes\n", sbf3abuf->pos);
				// wtk_debug("  abf3abuf size: %d bytes\n", abf3abuf->pos);
				// wtk_debug("  lininbuf size: %d bytes\n", linein_buf->pos);
			}
		}
			// else if(m->cfg->use_array == 0 && m->use_linein_out == 0){
			// 	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
			// 	if(rcdbuf[0]->pos >= pos){
			// 		i=0;
			// 		j=0;
			// 		while(i < pos){
			// 			wtk_strbuf_push(sbf3abuf, rcdbuf[3]->data+i, 2);
			// 			wtk_strbuf_push(sbf3abuf, rcdbuf[7]->data+i, 2);

			// 			i+=2;
			// 			j+=(channel2*2);
			// 		}

			// 		msg_bf3anode = qtk_msg_pop_node(m->msg);
			// 		wtk_strbuf_push(msg_bf3anode->buf, sbf3abuf->data, sbf3abuf->pos);
			// 		wtk_blockqueue_push(&m->vbox_queue, &msg_bf3anode->qn);

			// 		wtk_strbufs_pop(rcdbuf, channel1, pos);
			// 		wtk_strbuf_reset(sbf3abuf);
			// 	}
			// }
			// else if(m->use_linein_out && rcdbuf[0]->pos >= pos){
			// 	wtk_debug("================================++>>>>>>>>>>>>>>>>>>>...\n");
			// 	qtk_mod_am13e2_player_mode(m, rcdbuf[m->cfg->linein_channel]->data, pos);

			// 	wtk_strbufs_pop(rcdbuf, channel1, pos);
			// }
			if(rcdbuf[0]->pos >= (pos << 1)){
				wtk_strbufs_pop(rcdbuf, channel1, pos);
			}
			if(rcd2buf->pos >= pos*channel2*2){
				wtk_strbuf_pop(rcd2buf, NULL, pos*channel2);
			}
			qtk_msg_push_node(m->msg, msg_node);
			
			// wtk_debug("  rcdbuf[0] size: %d bytes\n", rcdbuf[0]->pos);
			// wtk_debug("  rcd2buf size: %d bytes\n", rcd2buf->pos);
		}
	// }
	// fclose(linein_fn);
	wtk_strbuf_delete(sbf3abuf);
	wtk_strbuf_delete(abf3abuf);
	wtk_strbuf_delete(linein_buf);
	for(i=0;i<channel1;++i){
		wtk_strbuf_delete(rcdbuf[i]);
	}
	wtk_free(rcdbuf);
	wtk_strbuf_delete(rcd2buf);
}

int qtk_mod_am13e2_linein_check_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	int lvalue1=0;
	int lvalue2=0;
	int ovalue=0;
	char *data1=NULL;
	char *data2=NULL;
	int dlen=0;
	int offcount=0;
	int oncount=0;

	if(wtk_file_exist(m->cfg->linein_check_path.data) == 0){
		data1 = file_read_buf(m->cfg->linein_check_path.data, &dlen);
		ovalue = atoi(data1);
		if(ovalue == 0){
			wtk_debug("=========>>>>>linein on<<<<<<==========\n");
			m->use_linein_out=1;
		}else if(ovalue > 0){
			wtk_debug("=========>>>>>linein off<<<<<<==========\n");
			m->use_linein_out=0;
		}
		wtk_free(data1);
		data1=NULL;
	}
	if(wtk_file_exist(m->cfg->lineout_check_path.data) == 0){
		data2 = file_read_buf(m->cfg->lineout_check_path.data, &dlen);
		ovalue = atoi(data2);
		if(ovalue == 0){
			wtk_debug("=========>>>>>lineout on<<<<<<==========\n");
			m->use_lineout_out=1;
		}else if(ovalue > 0){
			wtk_debug("=========>>>>>lineout off<<<<<<==========\n");
			m->use_lineout_out=0;
		}
		wtk_free(data2);
		data2=NULL;
	}
	while(m->linein_check_run){
		data1 = file_read_buf(m->cfg->linein_check_path.data, &dlen);
		data2 = file_read_buf(m->cfg->linein_check_path.data, &dlen);
		lvalue1 = atoi(data1);
		lvalue1 = atoi(data1);
		if(lvalue1 == 0){
			if(m->use_linein_out == 0){
				offcount++;
			}
			if(offcount > 2){
				if(m->use_linein_out == 0){
					wtk_debug("=========>>>>>linein on<<<<<<==========\n");
					qtk_mod_am13e2_proc_write(m,"LINEIN_ON",10);
				}
				m->use_linein_out = 1;
				offcount = 0;
			}
			oncount=0;
		}else if(lvalue1 > 0){
			if(m->use_linein_out == 1){
				oncount++;
			}
			if(oncount > 2){
				if(m->use_linein_out == 1){
					wtk_debug("=========>>>>>linein off<<<<<<==========\n");
					qtk_mod_am13e2_proc_write(m,"LINEIN_OFF",11);
				}
				m->use_linein_out = 0;
				oncount = 0;
			}
			offcount=0;
		}
		if(lvalue2 == 0){
			if(m->use_lineout_out == 0){
				offcount++;
			}
			if(offcount > 2){
				if(m->use_lineout_out == 0){
					wtk_debug("=========>>>>>lineout on<<<<<<==========\n");
					qtk_mod_am13e2_proc_write(m,"LINEOUT_ON",11);
				}
				m->use_lineout_out = 1;
				offcount = 0;
			}
			oncount=0;
		}else if(lvalue2 > 0){
			if(m->use_lineout_out == 1){
				oncount++;
			}
			if(oncount > 2){
				if(m->use_lineout_out == 1){
					wtk_debug("=========>>>>>linein off<<<<<<==========\n");
					qtk_mod_am13e2_proc_write(m,"LINEOUT_OFF",12);
				}
				m->use_lineout_out = 0;
				oncount = 0;
			}
			offcount=0;
		}
		wtk_free(data1);
		data1=NULL;
		wtk_free(data2);
		data2=NULL;
		sleep(1);
	}
}
int qtk_mod_am13e2_rcd3_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_mod_am13e2_set_cpu(m, t, 1);
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node;
	int is_first=1;

	wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry\n");
	wtk_log_log0(m->log,"------- recorde3 start");
	while(m->rcd3_run){
		
		rbuf = qtk_record_read(m->rcd3);
		if(rbuf->pos <=0){
			wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry\n");
			if(rbuf->pos == -19) usleep(1000*32);
			if(m->log) wtk_log_log(m->log, "record3 read error %d",rbuf->pos);
			continue;
		}
		if(is_first){
			wtk_debug("=================>>>>>>>>>>>>>>>>>>>record3 tm=%f\n",time_get_ms());
			is_first = 0;
		}
		// if(m->cfg->iis_shift != 1.0){
		// 	qtk_data_change_vol(rbuf->data, rbuf->pos, m->cfg->mic_shift);
		// }
		rcd3_total_frames += (rbuf->pos / (2 * 1));
		rcd3_elapsed = (rcd3_total_frames * 1000.0) / 48000;
		if((m->cfg->use_log_wav && m->iismul) || m->log_audio){
			wtk_wavfile_write(m->iismul, rbuf->data, rbuf->pos);
		}
		// wtk_debug("-------------------------->>>>>>>>>>>>>>>>>>>>>>>\n");
		if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout){
			wtk_debug("------------------>>>>>>>\n");
			qtk_msg_node_t *msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_am13e2_DATA_IIS;
			wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
			wtk_blockqueue_push(&m->gainnet3_queue, &msg_node->qn);
		}else{
			qtk_msg_node_t *msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_am13e2_DATA_IIS;
			wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
			wtk_blockqueue_push(&m->gainnet2_queue, &msg_node->qn);
		}
		// wtk_debug("-------->>>>rbuf->pos=%d, gainnet2_queue_len=%d\n",rbuf->pos, m->gainnet2_queue.length);
	}
	wtk_log_log0(m->log,"------- recorde3 end");
	return 0;
}
int qtk_mod_am13e2_rcd4_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	//  if(m->rcd_run || m->rcd2_run) {
    //     wtk_debug("Cannot start rcd4 while rcd1 or rcd2 is running\n");
    //     wtk_log_log0(m->log, "Cannot start rcd4 while rcd1 or rcd2 is running");
    //     // return -1;
    // }
	qtk_mod_am13e2_set_cpu(m, t, 1);
	wtk_strbuf_t *rbuf;
	wtk_strbuf_t *uacbuf=NULL;
	qtk_msg_node_t *msg_node;
	int is_first=1;
	// FILE * uac_fn;
	// uac_fn=fopen("/tmp/uac.pcm","wb");
	// if(uac_fn==NULL){
	// 	wtk_debug("fopen filed!!\n");
	// 	exit(1);
	// }
	wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry\n");
	wtk_log_log0(m->log,"------- recorde3 start");
	while(m->rcd4_run){
		rbuf = qtk_record_read(m->rcd4);
		if(rbuf->pos <=0){
			wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry\n");
			if(rbuf->pos == -19) usleep(1000*32);
			if(m->log) wtk_log_log(m->log, "record3 read error %d",rbuf->pos);
			continue;
		}
		if(is_first){
			wtk_debug("=================>>>>>>>>>>>>>>>>>>>record3 tm=%f\n",time_get_ms());
			is_first = 0;
		}

		// if(m->cfg->iis_shift != 1.0){
		// 	qtk_data_change_vol(rbuf->data, rbuf->pos, m->cfg->mic_shift);
		// }
		
		// fwrite(rbuf->data,rbuf->pos,1,uac_fn);
		if((m->cfg->use_log_wav && m->uacmul) || m->log_audio){
			wtk_wavfile_write(m->uacmul, rbuf->data, rbuf->pos);
		}
		if(m->is_use_uac)
		{
			msg_node = qtk_msg_pop_node(m->msg);
			// wtk_debug("---------------------------->>>>>>.rbuf->pos= %d\n",rbuf->pos);
			msg_node->type = qtk_mod_am13e2_DATA_UAC;
			// wtk_debug("-----------use_linein_courseware=%d--m->gainnetbf2_run = %d--uacrbuf.pos-= %d\n",m->cfg->use_linein_courseware,m->gainnet2_run,rbuf->pos);
			wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
			wtk_blockqueue_push(&m->gainnet2_queue, &msg_node->qn);
		}
	}
	// fclose(uac_fn);
	wtk_log_log0(m->log,"------- recorde4UAC end");
	return 0;
}
int qtk_mod_am13e2_rcd2_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_mod_am13e2_set_cpu(m, t, 1);
	wtk_strbuf_t *rbuf;
	wtk_strbuf_t *spk_check_buf=NULL;
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *spk_check_node;
	int is_first=1;
	spk_check_buf = wtk_strbuf_new(m->cfg->rcd.buf_time*32*2, 1.0f);

	wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry\n");
	wtk_log_log0(m->log,"------- recorde2 start");
	while(m->rcd2_run){
		rbuf = qtk_record_read(m->rcd2);
		if(rbuf->pos <=0){
			if(rbuf->pos == -19) usleep(1000*32);
			if(m->log) wtk_log_log(m->log, "record2 read error %d",rbuf->pos);
			continue;
		}
		if(is_first){
			wtk_debug("=================>>>>>>>>>>>>>>>>>>>record2 tm=%f\n",time_get_ms());
			is_first = 0;
		}

		// if(m->cfg->mic_shift != 1.0){
		// 	qtk_data_change_vol(rbuf->data, rbuf->pos, m->cfg->mic_shift);
		// }
		
		// wtk_debug("rrrrrrrrrrrrr222222222222222222=pos=%d\n",rbuf->pos);
		if((m->cfg->use_log_wav && m->arraymul) || m->log_audio){
			wtk_wavfile_write(m->arraymul, rbuf->data, rbuf->pos);
		}
		wtk_strbuf_reset(spk_check_buf);
		int src_pos = 0;
		while(src_pos < rbuf->pos) {
			wtk_strbuf_push(spk_check_buf,  rbuf->data+ src_pos + 6*2, 2);
			wtk_strbuf_push(spk_check_buf,  rbuf->data+ src_pos + 6*2+2, 2);
			src_pos += 8 * 2;
		}
		#if 1
		spk_check_node = qtk_msg_pop_node(m->msg);
		spk_check_node->type = qtk_mod_am13e2_sp2;
		wtk_strbuf_push(spk_check_node->buf, spk_check_buf->data, spk_check_buf->pos);
		wtk_blockqueue_push(&m->mic_check_play_queue, &spk_check_node->qn);
		#endif
		// wtk_debug("--------------_>>>>>>>>>>>>>>>>>\n");
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 2;
		// wtk_debug("rrrrrrrrrrrrr222222222222222222=pos=%d\n",rbuf->pos);
		wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
		wtk_blockqueue_push(&m->merge_rcd_queue, &msg_node->qn);
		wtk_strbuf_pop(spk_check_buf,NULL,spk_check_buf->pos);
		// wtk_debug("---------------===========>>>>>rcd2_rbuf->pos = %d\n",rbuf->pos);
		// wtk_debug("---------------===========>>>>>rcd2_merge_rcd_queue.length = %d\n",m->merge_rcd_queue.length);
	}
	wtk_log_log0(m->log,"------- recorde2 end");
	wtk_strbuf_delete(spk_check_buf);
	return 0;
}

int qtk_mod_am13e2_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node;
	qtk_mod_am13e2_set_cpu(m, t, 1);
	int is_first=1;
	int count=0;
	int32_t tsample;
	short rsample=0;
	int achannel = m->cfg->rcd.channel - m->cfg->rcd.nskip;
	wtk_strbuf_t *tmpbuf=wtk_strbuf_new(m->cfg->rcd.buf_time*32*m->cfg->rcd.channel, 1.0);
	double tm=0.0;
	
	wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry tmpbuf->pos=%d\n",tmpbuf->pos);
	wtk_log_log0(m->log,"------- recorde start");
	while(m->rcd_run){
			rbuf = qtk_record_read(m->rcd);
			if(rbuf->pos <=0){
				if(rbuf->pos == -19) usleep(1000*32);
				if(m->log) wtk_log_log(m->log, "record read error %d",rbuf->pos);
				continue;
			}
			if(is_first){
				wtk_debug("=================>>>>>>>>>>>>>>>>>>>record tm=%f\n",time_get_ms());
				is_first = 0;
			}

			wtk_strbuf_reset(tmpbuf);
			if(m->cfg->rcd.bytes_per_sample == 4){
				int pos=0;
				while(pos < rbuf->pos){
					tsample = ((int32_t *)(rbuf->data+pos))[0];
					rsample = (short)(tsample >> 16);
					// wtk_debug("=============>>>>>>>>>>>>>pos=%d .tsample=%d rsample=%d\n",pos ,tsample, rsample);
					wtk_strbuf_push(tmpbuf, (char *)(&rsample), 2);
					pos+=4;
				}
			}else{
				wtk_strbuf_push(tmpbuf, rbuf->data, rbuf->pos);
			}
			// wtk_debug("==================>>>>>>>>>>>>>>>rbuf->pos=%d tmpbuf->pos=%d time=%f\n",rbuf->pos,tmpbuf->pos, time_get_ms() - tm);
			count++;
			if(count == 50 && m->cfg->use_log_wav==0){
				qtk_mod_am13e2_is_log_audio(m);
				count=0;
			}
			if((m->cfg->use_log_wav && m->mul) || m->log_audio){
				wtk_wavfile_write(m->mul, tmpbuf->data, tmpbuf->pos);
			}
			if(m->cfg->mic_shift != 1.0){
				qtk_data_change_vol(tmpbuf->data, tmpbuf->pos, m->cfg->mic_shift);
			}
		//	mic数据检测请求
			// if (audio_check_request == 2 && audio_check_running) {
			// 	if(mic_first)
			// 	{
			// 		audio_check_rcd_tm = time_get_ms();
			// 	}
			// 	if(time_get_ms()-audio_check_rcd_tm > 6*1000)
			// 	{
			// 		audio_check_running = 0;
			// 		mic_first = 0;
			// 		m->audio_check_rcd_feed_end = 1;
			// 		audio_check_rcd_tm = 0.0;
			// 	}
			// 	wtk_debug("------------------->>>>>>>>>>>>>>>>\n");
			// 	msg_node = qtk_msg_pop_node(m->msg);
			// 	wtk_strbuf_push(msg_node->buf, tmpbuf->data, tmpbuf->pos);
			// 	wtk_blockqueue_push(&m->mic_check_rcd_queue, &msg_node->qn);
			// 	wtk_debug("------------->>>>>>>mic_check_rcd_queue.length=%d,tmpbuf->pos=%d\n",m->mic_check_rcd_queue.length,tmpbuf->pos);
			// }
			msg_node = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node->buf, tmpbuf->data, tmpbuf->pos);
			wtk_blockqueue_push(&m->mic_check_rcd_queue, &msg_node->qn);
			//音量值获取
			if(get_volume_calue == 1){
				if(get_volume_first){
					audio_check_start_time=time_get_ms();
					get_volume_first=0;
				}
				if(time_get_ms()-audio_check_start_time<audio_check_duration_ms){
					continuous_get_volume(m,tmpbuf->data, tmpbuf->pos);
				}
				wtk_debug("---------->>>>>>>>>>>>>>volume_get_result = %f\n",volume_get_result);
				get_volume_calue=0;
			}
			// wtk_debug("rrrrrrrrrrrrr11111111111111111111111=pos=%d\n",rbuf->pos);
			if(m->is_mic){
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = 1;
			wtk_strbuf_push(msg_node->buf, tmpbuf->data, tmpbuf->pos);
			wtk_blockqueue_push(&m->merge_rcd_queue, &msg_node->qn);
			// wtk_debug("----------------------------------\n");
			}	
			// wtk_debug("merge_rcd_queue.length =%d,gainnet_queue.length=%d,gainnet2_queue.length=%d,gainnet3_queue.length=%d,denoise_vbox_queue.length=%d\n",m->merge_rcd_queue.length,m->gainnet_queue,m->gainnet2_queue.length,m->gainnet3_queue.length,m->denoise_vbox_queue.length);
			// wtk_debug("array_queue.length=%d,usbaudio_queue.length=%d,lineout_queue.length=%d,linein_queue.length=%d\n",m->array_vbox_queue.length,m->usbaudio_queue.length,m->lineout_queue.length,m->linein_queue.length);
	

	}
	wtk_log_log0(m->log,"------- recorde end");
	wtk_strbuf_delete(tmpbuf);
	return 0;
}
int qtk_mod_am13e2_linein_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
	wtk_strbuf_t *mic_buf;
	qtk_mod_am13e2_set_cpu(m, t, 3);
	qtk_msg_node_t *msg_node, *msg_node2;
	wtk_queue_node_t *qn;
	mic_buf = wtk_strbuf_new(2048, 1.0);

		while(m->linein_run){
			// wtk_debug("---------------------use_linein_mic=%d\n",m->cfg->use_linein_mic);
			qn = wtk_blockqueue_pop(&m->linein_queue, -1, NULL);
			// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
			if(!qn) {continue;}
			msg_node = data_offset2(qn,qtk_msg_node_t,qn);
			if(m->cfg->use_linein_mic){
				int linein_pos = 0;
				wtk_strbuf_reset(mic_buf);
				while(linein_pos < msg_node->buf->pos) {
					wtk_strbuf_push(mic_buf, msg_node->buf->data + linein_pos, 2);
					linein_pos += 4;
				}
				// wtk_debug("-----------_>>>>>>>>>>mic->pos=%d\n",mic_buf->pos);
				if(m->denoisebf) {
					qtk_vboxebf_feed(m->denoisebf, mic_buf->data, mic_buf->pos, 0);
					// qtk_vboxebf_feed(m->denoisebf, msg_node->buf->data, msg_node->buf->pos, 0);
				}
			qtk_msg_push_node(m->msg, msg_node);
			// wtk_debug("-----------_>>>>>>>>>>mic->pos=%d\n",mic_buf->pos);
			// wtk_strbuf_pop(mic_buf, NULL, msg_node->buf->pos);
			}
		}
		qtk_vboxebf_feed(m->denoisebf, NULL, 0, 1);
		wtk_strbuf_delete(mic_buf);
		// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
		qtk_vboxebf_reset(m->denoisebf);
	return 0;
}
#ifndef OFFLINE_TEST
int qtk_mod_am13e2_usbaudio_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_mod_am13e2_set_cpu(m, t, 2);
	qtk_msg_node_t *msg_node=NULL,*msg_node2=NULL;
	wtk_queue_node_t *qn;
	int first = 1;
	long ret;
	wtk_blockqueue_t *usbaudio_queue;
	int zlen=m->cfg->sil_time*m->cfg->usbaudio.channel*m->cfg->usbaudio.sample_rate/1000*2;
	int ucnt;
	double tm,tm2;
	char *zerodata = wtk_malloc(zlen);
	memset(zerodata, 0, zlen);
	usbaudio_queue=&(m->usbaudio_queue);

	// wtk_debug("-----------> usbaudio start entry  zlen=%d\n",zlen);
	// wtk_log_log0(m->log,"-----------> usbaudio start entry");

	m->play_on=1;

  	while(m->usbaudio_run){
		// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
		qn= wtk_blockqueue_pop(usbaudio_queue,-1,NULL);
		// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

#if (defined USE_3308)
		if(m->player_run)
#endif
		{
			if(first){
				wtk_debug("===================>>>>>>>>>>>>>>>>>>>>>>>>>player time=%f\n",time_get_ms());
				ret = qtk_play_write(m->usbaudio, zerodata, zlen, 1);
				if(ret){
					wtk_debug("play zero buf %ld\n",ret);
					wtk_log_log(m->log,"play zero buf %ld",ret);
				}
				m->is_player_start=1;
				first=0;
			}
			if(m->cfg->echo_shift != 1.0){
				qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
			}
			if((m->cfg->use_log_wav && m->playwav) || m->log_audio){
				wtk_wavfile_write(m->playwav, msg_node->buf->data, msg_node->buf->pos);
				// wtk_debug("---------------------->>>>>>>playway length=%d\n",msg_node->buf->pos);
			}
			tm2 = time_get_ms();
			// wtk_debug("=============================>>>>>>>>>>>>>>>>>>> [%d] tm=%f - %f = %f  pos=%d length=%d\n",ucnt++,tm2, tm, tm2 - tm,msg_node->buf->pos,m->usbaudio_queue.length);
			
			// wtk_debug("------------------------------------------------------------>>>>>>>>>>>>>>>>>>..\n");
			ret = qtk_play_write(m->usbaudio, msg_node->buf->data, msg_node->buf->pos, 1);
			// wtk_debug("---------------------->>>>>>>play_msg_mode length=%d\n",m->usbaudio_queue.length);
			// double current_time = time_get_ms();
			// if (last_aplay_time > 0) {
			// double frame_interval = current_time - last_aplay_time;
			// wtk_debug("aplay frame time: %.3fms\n", 
			// 		frame_interval);
			// }
    		// last_aplay_time = current_time;
			// play_total_frames += (msg_node->buf->pos / (2 * m->cfg->usbaudio.channel));
			// double play_elapsed = (play_total_frames * 1000.0) / 48000;
			if(ret < 0 && ret != -11){
				wtk_debug("=================>>>>>>>>>>>>write err=%ld\n",ret);
				wtk_log_log(m->log,"=================>>>>>>>>>>>>write err=%d",ret);
			}
		}

#if (defined USE_3308)
		if(m->is_player_start == 0 && first == 0)
		{
			qtk_play_stop(m->usbaudio);
			first=1;
		}
#endif
		tm = time_get_ms();
		if(msg_node){
			qtk_msg_push_node(m->msg, msg_node);
		}
		// wtk_debug("------------------------------------->>>>>>>>>> usbaudio_queue-length=%d array_vbox_queue->length=%d\n",m->usbaudio_queue.length,m->array_vbox_queue.length);
		// if(!start_play){
		// 	if(m->merge_rcd_queue.length > 0){
		// 		qtk_mod_am13e2_clean_queue(m, &m->merge_rcd_queue);
		// 	}
		// 	if(m->array_vbox_queue.length > 0){
		// 		qtk_mod_am13e2_clean_queue(m, &m->array_vbox_queue);
		// 	}
		// }
	}

	if(first==0){
		qtk_play_stop(m->usbaudio);
	}

	if(zerodata){
		wtk_free(zerodata);
	}
	wtk_debug("============> player stop %d\n",first);
	wtk_log_log(m->log,"============> player stop %d",first);
  	return 0;
}

int qtk_mod_am13e2_lineout_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_msg_node_t *msg_node=NULL,*msg_node2=NULL;
	wtk_queue_node_t *qn;
	int first = 1;
	long ret;
	wtk_blockqueue_t *lineout_queue;
	int zlen=m->cfg->sil_time*m->cfg->lineout.channel*m->cfg->lineout.sample_rate/1000*2;
	int ucnt,count_towrite=0;
	double tm,tm2;
	int len;
	int volume;
	char set_buf[128]={0};
	qtk_mod_am13e2_set_cpu(m, t, 2);
	
	char *zerodata = wtk_malloc(zlen);
	memset(zerodata, 0, zlen);

	lineout_queue=&(m->lineout_queue);

	wtk_debug("-----------> lineout start entry zlen=%d\n",zlen);
	wtk_log_log0(m->log,"-----------> lineout start entry");

	m->play_on=1;

  	while(m->lineout_run){
		// wtk_debug("===================>>>>>>>>>>>>>>\n");
		qn= wtk_blockqueue_pop(lineout_queue,-1,NULL);
		// wtk_debug("===================>>>>>>>>>>>>>>\n");
		if(!qn) {
			continue;
			// msg_node = NULL;
			// goto loopcontinue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		if(first){
			wtk_debug("===================>>>>>>>>>>>>>>>>>>>>>>>>>player time=%f\n",time_get_ms());
			ret = qtk_play_write(m->lineout, zerodata, zlen, 6);
			// qtk_play2_write(m->lineout,zerodata,zlen);    ///use_liwei
			wtk_debug("===================>>>>>>>>>>>>>>\n");
			if(ret){
				wtk_debug("play zero buf %ld\n",ret);
				wtk_log_log(m->log,"play zero buf %ld",ret);
			}
			first=0;
		}
	    // wtk_debug("-------------->>>>>>>>>>>>>>>>>>>>>\n");
		// if(m->cfg->echo_shift != 1.0){
		// 	qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
		// }
	#if 1
		if(m->cfg->use_log_wav && m->lineoutml)
		{
			wtk_wavfile_write(m->lineoutml, msg_node->buf->data, msg_node->buf->pos);
		}
	#endif
	    // wtk_debug("-------------->>>>>lineout.pos=%d\n",msg_node->buf->pos);
		ret = qtk_play_write(m->lineout, msg_node->buf->data, msg_node->buf->pos, 1);
		// play_total_frames += (msg_node->buf->pos / (2 * 6));
		// double play_elapsed = (play_total_frames * 1000.0) / 48000;
		// wtk_debug("------------------------------------->>>>>>>>>>..delay time =%.3f play->pos = %d\n",rcd3_elapsed-play_elapsed,msg_node->buf->pos);
		if(ret < 0){
			wtk_debug("=================>>>>>>>>>>>>write err=%ld\n",ret);
			wtk_log_log(m->log,"=================>>>>>>>>>>>>write err=%d",ret);
		}
		// qtk_play2_write(m->lineout, msg_node->buf->data, msg_node->buf->pos); ///use_liwei
		if(msg_node)
			qtk_msg_push_node(m->msg, msg_node);
		// wtk_debug("------------------------------------->>>>>>>>>>..delay time =%.3f,lineout_queue.length= %d\n",rcd_elapsed-play_elapsed,m->lineout_queue.length);
	}

	if(first==0){
		qtk_play_stop(m->lineout);
	}

	if(zerodata){
		wtk_free(zerodata);
	}
  	return 0;
}
#endif
// int qtk_mod_am13e2_spk_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
// {
// 	qtk_msg_node_t *msg_node=NULL,*msg_node2=NULL;
// 	wtk_queue_node_t *qn;
// 	int first = 1;
// 	long ret;
// 	wtk_blockqueue_t *spk_queue;
// 	int zlen=m->cfg->sil_time*m->cfg->lineout.channel*m->cfg->lineout.sample_rate/1000*2;
// 	int ucnt;
// 	double tm,tm2;

// 	qtk_mod_am13e2_set_cpu(m, t, 2);
	
// 	char *zerodata = wtk_malloc(zlen);
// 	memset(zerodata, 0, zlen);

// 	spk_queue=&(m->spk_queue);

// 	wtk_debug("-----------> speaker start entry zlen=%d\n",zlen);
// 	wtk_log_log0(m->log,"-----------> speaker start entry");

// 	m->play_on=1;

//   	while(m->speaker_run){
// 		// wtk_debug("===================>>>>>>>>>>>>>>\n");
// 		qn= wtk_blockqueue_pop(spk_queue,-1,NULL);
// 		// wtk_debug("===================>>>>>>>>>>>>>>\n");
// 		if(!qn) {
// 			continue;
// 			// msg_node = NULL;
// 			// goto loopcontinue;
// 		}
// 		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

// 		if(first){
// 			wtk_debug("===================>>>>>>>>>>>>>>>>>>>>>>>>>player time=%f\n",time_get_ms());
// 			ret = qtk_play_write(m->speaker, zerodata, zlen, 1);
// 			// qtk_play2_write(m->lineout,zerodata,zlen);    ///use_liwei
// 			wtk_debug("===================>>>>>>>>>>>>>>\n");
// 			if(ret){
// 				wtk_debug("play zero buf %ld\n",ret);
// 				wtk_log_log(m->log,"play zero buf %ld",ret);
// 			}
// 			first=0;
// 		}
		
// 		ret = qtk_play_write(m->speaker, msg_node->buf->data, msg_node->buf->pos, 1);
// 		if(ret < 0){
// 			wtk_debug("=================>>>>>>>>>>>>write err=%ld\n",ret);
// 			wtk_log_log(m->log,"=================>>>>>>>>>>>>write err=%d",ret);
// 		}
// 		// qtk_play2_write(m->lineout, msg_node->buf->data, msg_nsode->buf->pos); ///use_liwei
// 		qtk_msg_push_node(m->msg, msg_node);
// 	}

// 	if(first==0){
// 		qtk_play_stop(m->speaker);
// 	}

// 	if(zerodata){
// 		wtk_free(zerodata);
// 	}
//   	return 0;
// }
void qtk_mod_am13e2_on_vboxebf(qtk_mod_am13e2_t *m, char *data, int len)
{
	// qtk_mod_am13e2_player_mode(m, data, len);
	// wtk_debug("--------------------------->>>>>>>>>>>>>>.gainnetbf_cfg = %d\n",m->cfg->gainnetbf_cfg);
	// wtk_debug("--------------------------->>>>>>>>>>>>>>.use_out_resample = %d\n",m->cfg->use_out_resample);
	if(m->cfg->gainnetbf_cfg){
		wtk_debug("----------------------------------\n");
		qtk_msg_node_t *msg_node;
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = qtk_mod_am13e2_DATA_STUDENT_BF3A;
		
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->gainnet_queue, &msg_node->qn);
	}else{
		wtk_debug("----------------------------------\n");
		if(m->cfg->use_out_resample){
			wtk_debug("================================++>>>>>>>>>>>>>>>>>>>...\n");
			wtk_resample_feed(m->outresample, data, len, 0);
		}else{
			qtk_mod_am13e2_player_mode(m, data, len);
		}
	}
}
void qtk_mod_am13e2_on_denoise_vboxebf(qtk_mod_am13e2_t *m, char *data, int len)
{
	static double tm=0.0;
	// wtk_debug("=======================>>>>>>>len=%d tm=%f\n",len,time_get_ms() - tm);
	if(m->cfg->gainnetbf_cfg){
		qtk_msg_node_t *msg_node;
		msg_node = qtk_msg_pop_node(m->msg);		
		msg_node->type = qtk_mod_am13e2_DATA_LINEIN_MIC_TOUAC;
		wtk_strbuf_push(msg_node->buf, data, len);
		wtk_blockqueue_push(&m->gainnet_queue, &msg_node->qn);
	}else{
		if(m->cfg->use_out_resample){
			wtk_resample_feed(m->outresample, data, len, 0);
		}else{
			qtk_mod_am13e2_player_mode(m, data, len);
		}
	}
	tm = time_get_ms();
}
void qtk_mod_am13e2_on_array_vboxebf(qtk_mod_am13e2_t *m, char *data, int len)
{
	static double tm=0.0;
	// wtk_debug("=======================>>>>>>>len=%d tm=%f\n",len,time_get_ms() - tm);
	if(m->cfg->gainnetbf_cfg){
		if(m->cfg->use_meetinglineout){
			qtk_msg_node_t *meetinglineout;
			meetinglineout = qtk_msg_pop_node(m->msg);
			meetinglineout->type = qtk_mod_am13e2_DATA_ARRAY_TOlINEOUT;
			wtk_strbuf_push(meetinglineout->buf, data, len);
			wtk_blockqueue_push(&m->gainnet3_queue, &meetinglineout->qn);
		}else{
			qtk_msg_node_t *msg_node;
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_am13e2_DATA_ARRAY;
			wtk_strbuf_push(msg_node->buf, data, len);
			wtk_blockqueue_push(&m->gainnet_queue, &msg_node->qn);
		}
	#if 0
		wtk_debug("-----------<>>>>>use_meetinglineout=%d\n",m->cfg->use_meetinglineout);
	#endif
	}else{
		if(m->cfg->use_out_resample){
			wtk_resample_feed(m->outresample, data, len, 0);
		}else{
			qtk_mod_am13e2_player_mode(m, data, len);
		}
	}
	tm = time_get_ms();
}

void qtk_mod_am13e2_on_gainnetbf(qtk_mod_am13e2_t *m, char *data, int len)
{	
	
	qtk_mod_am13e2_player_mode(m, data, len);
}
void qtk_mod_am13e2_on_gainnetbf2(qtk_mod_am13e2_t *m, char *data, int len) //speaker
{	
	wtk_strbuf_push(m->left_audiobuf, data, len);
}
void qtk_mod_am13e2_on_gainnetbf3(qtk_mod_am13e2_t *m, char *data, int len) //lineout
{
	// qtk_mod_am13e2_player2_mode(m, data, len);
	wtk_strbuf_push(m->left_audiobuf, data, len);
}
void qtk_mod_am13e2_on_gainnetbf4(qtk_mod_am13e2_t *m, char *data, int len) 
{
	int i=0;
	wtk_strbuf_reset(m->all_audiobuf);
	while(i<len){
		wtk_strbuf_push(m->all_audiobuf,m->left_audiobuf->data+i,2);
		wtk_strbuf_push(m->all_audiobuf,data+i,2);
		i+=4;
	}
	if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout)
		qtk_mod_am13e2_player2_mode(m, m->all_audiobuf->data, m->all_audiobuf->pos); 
	else
		qtk_mod_am13e2_player3_mode(m, m->all_audiobuf->data, m->all_audiobuf->pos); 
	wtk_strbuf_pop(m->left_audiobuf, NULL, len);

}
void qtk_mod_am13e2_on_mic_check_rcd(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int nchn)
{
	char set_buf[128]={0};
	wtk_debug("------------------------------__>>>>>>>>>>>>>>>>>>>>\n");
	int i,mic_check_rcd_result=0,ret=0;
    for (i = 0; i < nchn; ++i) {
        // printf("%d:%d\n", i, type[i]);
		if(type[i] != 0)
		{
			mic_check_rcd_result = type[i];
		}
    }
	mic_check_result = mic_check_rcd_result;
	FILE *file = fopen("/oem/qdreamer/qsound/miccheck_result.txt", "w");
	if (file != NULL) {
		fprintf(file, "%d\n", mic_check_result);
		wtk_debug("---------------------mic_check_result = %d\n", mic_check_result);
		fflush(file);
		{
		int fd=fileno(file);
		if(fd>=0){fsync(fd);}
		}
		fclose(file);
	} else {
		printf("无法打开文件进行写入。\n");
	}
	
}
void qtk_mod_am13e2_on_mic_check_play(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int channenl)
{
	char set_buf[128]={0};
	wtk_debug("------------------------------__>>>>>>>>>>>>>>>>>>>>\n");
	int i,mic_check_play_result=0,ret=0;
    for (i = 0; i < channenl; ++i) {
        printf("%d:%d\n", i, type[i]);
		if(type[i] != 0)
		{
			mic_check_play_result = type[i];
		}
    }
	speak_check_result = mic_check_play_result;

	FILE *file = fopen("/oem/qdreamer/qsound/spkcheck_result.txt", "w");
	if (file != NULL) {
		fprintf(file, "%d\n", speak_check_result);
		wtk_debug("---------------------spkcheck_result = %d\n", speak_check_result);
		fflush(file);
		{
		int fd=fileno(file);
		if(fd>=0){fsync(fd);}
		}
		fclose(file);
	} else {
		printf("无法打开文件进行写入。\n");
	}
}
void qtk_mod_am13e2_on_outresample(qtk_mod_am13e2_t *m, char *data, int len)
{
	qtk_mod_am13e2_player_mode(m, data, len);
}

void qtk_mod_am13e2_log_wav_file_new(qtk_mod_am13e2_t *m)
{
	int channel = m->mic_channel;
	int bytes_per_sample = 2;
	// int hh25c_a_sample_rate = m->cfg->rcd.sample_rate;
	
	// wtk_debug("============>>>>>>>sample_rate=%d channel=%d bytes_per_sample=%d\n",m->cfg->rcd.sample_rate,channel,bytes_per_sample);
	
#if 1
	m->arraymul = wtk_wavfile_new(m->cfg->rcd2.sample_rate); 
	m->arraymul->max_pend = 0;
	channel = m->cfg->rcd2.channel - m->cfg->rcd2.nskip;
	wtk_wavfile_set_channel2(m->arraymul,channel,bytes_per_sample);
	wtk_wavfile_open(m->arraymul, m->arraymul_path->data);
	channel =6;
	m->lineoutml = wtk_wavfile_new(m->cfg->lineout.sample_rate);
	m->lineoutml->max_pend = 0;
	wtk_wavfile_set_channel2(m->lineoutml,m->cfg->lineout.channel,bytes_per_sample);
	wtk_wavfile_open(m->lineoutml, m->lineout_path->data);
#endif
#if 0
	m->mul = wtk_wavfile_new(m->cfg->rcd.sample_rate); 
	m->mul->max_pend = 0;
	// channel = 1;
	wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);
	wtk_wavfile_open(m->mul, m->mul_path->data);

	m->iismul = wtk_wavfile_new(m->cfg->rcd3.sample_rate); 
	m->iismul->max_pend = 0;
	channel = m->cfg->rcd3.channel - m->cfg->rcd3.nskip;
	wtk_wavfile_set_channel2(m->iismul,channel,bytes_per_sample);
	wtk_wavfile_open(m->iismul, m->iis_path->data);
#endif
	// m->uacmul = wtk_wavfile_new(m->cfg->rcd4.sample_rate); 
	// m->uacmul->max_pend = 0;
	// channel = m->cfg->rcd4.channel - m->cfg->rcd4.nskip;
	// wtk_wavfile_set_channel2(m->uacmul,channel,bytes_per_sample);
	// wtk_wavfile_open(m->uacmul, m->uac_path->data);
	// m->jlmul = wtk_wavfile_new(48000); 
	// m->jlmul->max_pend = 0;	
	// channel = 1;
	// wtk_debug("===>jlchannel=%d\n",channel);
	// wtk_wavfile_set_channel2(m->jlmul, channel, bytes_per_sample);
	// wtk_wavfile_open(m->jlmul, "/data/qdreamer/mix.wav");
#if 0
	m->playwav = wtk_wavfile_new(m->cfg->usbaudio.sample_rate);
	// m->playwav = wtk_wavfile_new(16000);
	m->playwav->max_pend = 0;
	// wtk_wavfile_set_channel2(m->playwav,1,bytes_per_sample);
	wtk_wavfile_set_channel2(m->playwav,m->cfg->usbaudio.channel,bytes_per_sample);
	wtk_wavfile_open(m->playwav, m->play_path->data);
#endif

}

void qtk_mod_am13e2_log_wav_file_delete(qtk_mod_am13e2_t *m)
{
#if 0
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);
	m->mul = NULL;
	wtk_wavfile_close(m->iismul);
	wtk_wavfile_delete(m->iismul);
	m->iismul = NULL;
#endif

#if 1
	wtk_wavfile_close(m->arraymul);
	wtk_wavfile_delete(m->arraymul);
	m->arraymul = NULL;
	wtk_wavfile_close(m->lineoutml);
	wtk_wavfile_delete(m->lineoutml);
	m->lineoutml = NULL;
#endif
// wtk_wavfile_close(m->uacmul);
// 	wtk_wavfile_delete(m->uacmul);
// 	m->uacmul = NULL;

// 	wtk_wavfile_close(m->jlmul);
// 	wtk_wavfile_delete(m->jlmul);
// 	m->jlmul = NULL;

// 	wtk_wavfile_close(m->playwav);
// 	wtk_wavfile_delete(m->playwav);
// 	m->playwav = NULL;
}

void qtk_mod_am13e2_is_log_audio(qtk_mod_am13e2_t *m)
{
	if(m->cfg->cache_path.len <= 0){
		wtk_log_log0(glb_log, "cfg->cache_path = NULL\n");
		wtk_log_log0(glb_log, "cfg->cache_path = NULL\n");
		return ;
	}
	if(m->log_audio == 0 && access(m->check_path_buf->data, F_OK)==0){
		qtk_mod_am13e2_log_wav_file_new(m);
		m->log_audio = 1;
	}else if(m->log_audio == 1 && access(m->check_path_buf->data, F_OK)){
		m->log_audio = 0;
		qtk_mod_am13e2_log_wav_file_delete(m);
	}
}

void qtk_mod_am13e2_set_cpu(qtk_mod_am13e2_t *m, wtk_thread_t *thread, int cpunum)
{
#ifndef DEBUG_FILE //def USE_SLB
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
    if (ret != 0){
        wtk_log_err(m->log, "pthread_setaffinity_np error %d!\n",ret);
	}
	ret = pthread_getaffinity_np(thread->handler, sizeof(cpu_set_t), &cpuset);
    if (ret != 0){
        wtk_log_err(m->log, "pthread_getaffinity_np error %d!\n",ret);
	}

	wtk_log_log0(m->log, "Set returned by pthread_getaffinity_np() contained:\n");
    printf("Set returned by pthread_getaffinity_np() contained:\n");
	int j;
    for (j = 0; j < 4; j++)
	{
		// if(CPU_ISSET(j, &cpuset))
        if (__CPU_ISSET_S(j, sizeof(cpu_set_t), &cpuset)){
			wtk_log_log(m->log, "    CPU %d\n", j);
            printf("    CPU %d\n", j);
		}
	}
#endif
}

void _hh25c_a_frame_extr(wtk_strbuf_t *buf,int extr_n)
{
	int n = buf->pos;
	char *data = buf->data;
	int i = 0;
	wtk_strbuf_t *tmp = wtk_strbuf_new(buf->pos,1.0f);
	int p = n/extr_n;
	int np = n%extr_n;

	if(buf->pos <= (extr_n*2)){return;}

	for(i = 0; i < extr_n; ++i){
		wtk_strbuf_push(tmp,data+i*p,p-2);
	}
	if(np) wtk_strbuf_push(tmp,data+buf->pos-np,np);
	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,tmp->data,tmp->pos);
	wtk_strbuf_delete(tmp);
	return;
}

#ifdef USE_3308
int qtk_mod_am13e2_proc_read(qtk_mod_am13e2_t *m)
{
	// 读取程序
	char buf[32] = {0};
	char tmpbuf[1024]={0};
	char buf2[24]={0};
	char buf3[128]={0};
	int val,val2;
	char *pv=NULL;
	if (once == 1)
	{
		wtk_debug("=====>>>> mem_proc.pid_server = %d\n", mem_proc.pid_server);
		// wtk_debug("read:[%s]\n", mem_proc.shmaddr->buf);
		memcpy(buf, mem_proc.shmaddr->buf, MAX);
		wtk_debug("====>>>>>read buf=[%s]\n", buf); // 读取到sound_uart发送的请求
		if (strncmp(buf, "MIC_AUDIO_CHECK", 15) == 0)//检索mic 音频是否正常
		{
			// wtk_debug("------------------------------------------------------------<<<<<<\n");
			// audio_check_request=2;
			// audio_check_running=1;
			// mic_first = 1;
			// FILE *file = fopen("/oem/qdreamer/qsound/miccheck_result.txt", "w");
			// if (file != NULL) {
			// 	fprintf(file, "%d\n", mic_check_result);
			// 	wtk_debug("---------------------mic_check_result = %d\n", mic_check_result);
			// 	mic_check_result = -1;
			// 	fflush(file);
			// 	{
			// 	int fd=fileno(file);
			// 	if(fd>=0){fsync(fd);}
			// 	}
			// 	fclose(file);
			// 	//system("sync");
			// } else {
			// 	printf("无法打开文件进行写入。\n");
			// }
		}
	#if 1
		else if (strncmp(buf, "OUTPUT_INFOR",12) == 0) //音频输出列表
		{
			double outinfor_time=time_get_ms();
			cJSON *port_list = cJSON_CreateArray();
			cJSON *port1 = cJSON_CreateObject();
			cJSON *port2 = cJSON_CreateObject();
			char msgdata1[256], msgdata2[256];
			char final_json[512];
			FILE *fn;
			char *pv;
			int s,pos,ret;
			qtk_read_register("/sys/bus/i2c/devices/3-0069/volume", &s);
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-outinfor_time);
			if(m->cfg->use_speaker){
				cJSON_AddNumberToObject(port1, "portType", 4);
				cJSON_AddNumberToObject(port1, "portId", 0xbb00003);
				cJSON_AddStringToObject(port1, "portName", "SPK");
				cJSON_AddTrueToObject(port1, "isUse");
				cJSON_AddNumberToObject(port1, "gainLevel", s);
				cJSON_AddNumberToObject(port1, "audioInputType", 0);
				cJSON_AddTrueToObject(port1, "isLocalPlay");
			} else {
				cJSON_AddNumberToObject(port1, "portType", 4);
				cJSON_AddNumberToObject(port1, "portId", 0xbb00003);
				cJSON_AddStringToObject(port1, "portName", "SPK");
				cJSON_AddFalseToObject(port1, "isUse");
				cJSON_AddNumberToObject(port1, "gainLevel", s);
				cJSON_AddNumberToObject(port1, "audioInputType", 0);
				cJSON_AddFalseToObject(port1, "isLocalPlay");
				ret = snprintf(msgdata1, 256, "{\"portType\":0xbb000004,\"portId\":3,\"portName\":\"SPK\",\"isUse\":false,\"gainLevel\":%d,\"audioInputType\":0,\"isLocalPlay\":false}",s);
			}
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-outinfor_time);
			cJSON_AddItemToArray(port_list, port1);
			if(m->cfg->use_headset){
				cJSON_AddNumberToObject(port2, "portType", 5);
				cJSON_AddNumberToObject(port2, "portId", 0xbb00003);
				cJSON_AddStringToObject(port2, "portName", "LINE_OUT");
				cJSON_AddTrueToObject(port2, "isUse");
				cJSON_AddNumberToObject(port2, "gainLevel", s);
				cJSON_AddNumberToObject(port2, "audioInputType", 0);
				cJSON_AddFalseToObject(port2, "isLocalPlay");
			} else {
				cJSON_AddNumberToObject(port2, "portType", 5);
				cJSON_AddNumberToObject(port2, "portId", 0xbb00003);
				cJSON_AddStringToObject(port2, "portName", "LINE_OUT");
				cJSON_AddFalseToObject(port2, "isUse");
				cJSON_AddNumberToObject(port2, "gainLevel", s);
				cJSON_AddNumberToObject(port2, "audioInputType", 0);
				cJSON_AddFalseToObject(port2, "isLocalPlay");
			}
			cJSON_AddItemToArray(port_list, port2);
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-outinfor_time);
			char *json_str = cJSON_Print(port_list);
			FILE *output_file = fopen("/oem/qdreamer/qsound/audiooutput_infor.txt", "w");
			if (output_file) {
				// fwrite(final_json, 1, pos, output_file);
				fwrite(json_str, 1, strlen(json_str), output_file);
				fclose(output_file);
			} else {
				wtk_debug("---------------------_>>>>>>>>>>>>failed to open output file\n");
			}
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-outinfor_time);
			cJSON_Delete(port_list);
		}else if (strncmp(buf, "INPUT_INFOR",11) == 0) //音频输入列表
		{ 
			double ininfor_time=time_get_ms();
			cJSON *port_list = cJSON_CreateArray();
			cJSON *port1 = cJSON_CreateObject();
			cJSON *port2 = cJSON_CreateObject();
			// char msgdata1[256]={0}, msgdata2[256]={0};
			// char final_json[512]={0};
			FILE *fn;
			fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r");
			char buf[1024] = {0};
			int ret, s,pos=0;
			char *pv;
			ret = fread(buf, 1, sizeof(buf), fn);
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-ininfor_time);
			if(m->rcd && m->rcd_run){
				pv = strstr(buf, "mic_shift2=");
				// pos += snprintf(final_json + pos, sizeof(final_json) - pos, "[\n");
				if (pv) {
					s = atoi(pv + 11);
					cJSON_AddNumberToObject(port1, "portType", 0);
					cJSON_AddNumberToObject(port1, "portId", 0xaa00000);
					cJSON_AddStringToObject(port1, "portName", "arrary_MIC");
					cJSON_AddTrueToObject(port1, "isUse");
					cJSON_AddNumberToObject(port1, "gainLevel", s);
					cJSON_AddNumberToObject(port1, "audioInputType", 0);
					cJSON_AddFalseToObject(port1, "isLocalPlay");
				} else {
					cJSON_AddNumberToObject(port1, "portType", 0);
					cJSON_AddNumberToObject(port1, "portId", 0xaa00000);
					cJSON_AddStringToObject(port1, "portName", "arrary_MIC");
					cJSON_AddTrueToObject(port1, "isUse");
					cJSON_AddNumberToObject(port1, "gainLevel", 0);
					cJSON_AddNumberToObject(port1, "audioInputType", 0);
					cJSON_AddFalseToObject(port1, "isLocalPlay");
				}
			} else {
					cJSON_AddNumberToObject(port1, "portType", 0);
					cJSON_AddNumberToObject(port1, "portId", 0xaa00000);
					cJSON_AddStringToObject(port1, "portName", "arrary_MIC");
					cJSON_AddFalseToObject(port1, "isUse");
					cJSON_AddNumberToObject(port1, "gainLevel", 0);
					cJSON_AddNumberToObject(port1, "audioInputType", 0);
					cJSON_AddFalseToObject(port1, "isLocalPlay");
			}
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-ininfor_time);
			 cJSON_AddItemToArray(port_list, port1);
			if(m->cfg->use_line_in){
				pv = strstr(buf, "vbox3_agc_level=");
				if (pv) {
					s = atoi(pv + 16);
					cJSON_AddNumberToObject(port2, "portType", 1);
					cJSON_AddNumberToObject(port2, "portId", 0xaa00002);
					cJSON_AddStringToObject(port2, "portName", "Line_IN");
					cJSON_AddTrueToObject(port2, "isUse");
					cJSON_AddNumberToObject(port2, "gainLevel", s);
					cJSON_AddNumberToObject(port2, "audioInputType", 0);
					cJSON_AddFalseToObject(port2, "isLocalPlay");
				} else {
					cJSON_AddNumberToObject(port2, "portType", 1);
					cJSON_AddNumberToObject(port2, "portId", 0xaa00002);
					cJSON_AddStringToObject(port2, "portName", "arrary_MIC");
					cJSON_AddTrueToObject(port2, "isUse");
					cJSON_AddNumberToObject(port2, "gainLevel", 0);
					cJSON_AddNumberToObject(port2, "audioInputType", 0);
					cJSON_AddFalseToObject(port2, "isLocalPlay");
				}
			} else {
					cJSON_AddNumberToObject(port2, "portType", 1);
					cJSON_AddNumberToObject(port2, "portId", 0xaa00002);
					cJSON_AddStringToObject(port2, "portName", "arrary_MIC");
					cJSON_AddFalseToObject(port2, "isUse");
					cJSON_AddNumberToObject(port2, "gainLevel", 0);
					cJSON_AddNumberToObject(port2, "audioInputType", 0);
					cJSON_AddFalseToObject(port2, "isLocalPlay");
			}
			cJSON_AddItemToArray(port_list, port2);
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-ininfor_time);
			char *json_str = cJSON_Print(port_list);
			// wtk_debug("------------->>>>--json_str-%d\n",strlen(json_str));
			FILE *output_file = fopen("/oem/qdreamer/qsound/audioinput_infor.txt", "w");
			if (output_file) {
				fwrite(json_str, 1, strlen(json_str), output_file);
				fclose(output_file);
			} else {
				wtk_debug("---------------------_>>>>>>>>>>>>failed to open output file\n");
			}
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-ininfor_time);
			cJSON_Delete(port_list);
		}
		#endif
		else if (strncmp(buf, "SET_MICVOLUME", 13) == 0)//设置当前麦克风音量级别
		{
			wtk_debug("----------------------->>>>>>>>\n");
			int ret=-2;
			float set;
			FILE*fn;
			if (sscanf(buf, "SET_MICVOLUME%d", &val)== 1) {
				set = 6 * (val / 100.0) ;
				m->cfg->mic_shift=set;
				printf("Mapped volume: %.2f,m->cfg->mic_shift:%.2f\n", set,m->cfg->mic_shift);
			} else {
				printf("Invalid input format\n");
			}
			wtk_debug("----------------------->>>>>>>>\n");
			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
			{
				fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r+");
				ret = fread(tmpbuf, sizeof(tmpbuf), 1, fn);
				wtk_debug("================ret=%d fread=[%s]\n", ret,tmpbuf);
				pv = strstr(tmpbuf, "mic_shift2=");
				if(pv)
				{
					sprintf(buf2,"mic_shift2=%0.2f;",set);
					memcpy(pv,buf2,strlen(buf2));
					fseek(fn, 0, SEEK_SET);
					ret = fwrite(tmpbuf, strlen(tmpbuf), 1, fn);
					wtk_debug("================fwrite=%d\n", ret);
				}
			}
			fflush(fn);
			{
				int fd=fileno(fn);
				if(fd>=0){fsync(fd);}
			}
			fclose(fn);
			//system("sync");
			wtk_debug("----------------------->>>>>>>>\n");
			//system("sync");
			fn=NULL;
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),1);
		}else if (strncmp(buf,"GET_VOLUME_VALUE",16) == 0)
		{
			double start_time = time_get_ms();
			wtk_debug("------------------------------------------------------------<<<<<<\n");
			wtk_debug("------------------_>>>>>>>start_time = %.2f\n", time_get_ms() - start_time);
			get_volume_calue=1;
			get_volume_first=1;
			wtk_debug("------------------_>>>>>>>after_set_flags = %.2f\n", time_get_ms() - start_time);
			while (volume_get_result == -97) {
				usleep(1000);
			}
			wtk_debug("------------------_>>>>>>>after_while_loop = %.2f\n", time_get_ms() - start_time);
			FILE *file = fopen("/oem/qdreamer/qsound/get_volume.txt", "w");
			wtk_debug("------------------_>>>>>>>after_fopen = %.2f\n", time_get_ms() - start_time);
			if (file != NULL) {
				fprintf(file, "%.2f\n", volume_get_result);
				wtk_debug("---------------------get_volume = %f\n", volume_get_result);
				wtk_debug("------------------_>>>>>>>after_fprintf = %.2f\n", time_get_ms() - start_time);
				volume_get_result = -1;
				fflush(file);
				wtk_debug("------------------_>>>>>>>after_fflush = %.2f\n", time_get_ms() - start_time);
				{
				int fd=fileno(file);
				if(fd>=0){fsync(fd);}
				}
				wtk_debug("------------------_>>>>>>>after_fsync = %.2f\n", time_get_ms() - start_time);
				fclose(file);
				wtk_debug("------------------_>>>>>>>after_fclose = %.2f\n", time_get_ms() - start_time);
				//system("sync");
			} else {
				printf("无法打开文件进行写入。\n");
			}
			wtk_debug("------------------_>>>>>>>total_time = %.2f\n", time_get_ms() - start_time);
		}
		else if (strncmp(buf, "SPK_AUDIO_CHECK", 15) == 0)//检索spk 音频是否正常
		{
			// wtk_debug("------------------------------------------------------------<<<<<<\n");
			// int ret =0;
			// if(!m->cfg->use_speaker|| !m->cfg->use_spkout){
			// 	ret = 1;
			// 	m->cfg->use_speaker = 1;
			// 	m->cfg->use_spkout = 1;
			// }
			// audio_check_request=1;
			// audio_check_running=1;
			// spk_first = 1;
			// wtk_debug("------------------------------------------------------------<<<<<<\n");
			//  while (speak_check_result == -1) {
			// 	usleep(100);
			// }
			// wtk_debug("------------------------------------------------------------<<<<<<\n");
			// FILE *file = fopen("/oem/qdreamer/qsound/spkcheck_result.txt", "w");
			// if (file != NULL) {
			// 	fprintf(file, "%d\n", speak_check_result);
			// 	wtk_debug("---------------------speak_check_result = %d\n", speak_check_result);
			// 	speak_check_result = -1;
			// 	fflush(file);
			// 	{
			// 	int fd=fileno(file);
			// 	if(fd>=0){fsync(fd);}
			// 	}
			// 	fclose(file);
			// 	if(ret == 1){
			// 		m->cfg->use_speaker = 0;
			// 	}
			// 	//system("sync");
			// } else {
			// 	printf("无法打开文件进行写入。\n");
			// }
			// wtk_debug("------------------------------------------------------------<<<<<<\n");
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),1);
		}else if (strncmp(buf, "SET_LINEOUTPATTERN", 18) == 0)//设置 lineout模式
		{
			// wtk_debug("-------->>>>>>>>>>>>>>>>>>>>>>SET_LINEOUTPATTERN\n");
			double set_lineout_time=time_get_ms();
			int ret;
			if (sscanf(buf, "SET_LINEOUTPATTERN%d", &ret)== 1){
				if(ret == 0){
					m->cfg->use_mainlineout=1;
					m->cfg->use_wooflineout=0;
					m->cfg->use_meetinglineout=0;
					m->cfg->use_expandlineout=0;

				}
				if(ret == 1){
					m->cfg->use_mainlineout=0;
					m->cfg->use_wooflineout=1;
					m->cfg->use_meetinglineout=0;
					m->cfg->use_expandlineout=0;
				}
				if(ret == 2){
				#if 0
					m->cfg->use_mainlineout=0;
					m->cfg->use_wooflineout=0;
					m->cfg->use_meetinglineout=1;
					m->cfg->use_expandlineout=0;
				#endif
				}
				if(ret == 3){
				#if 0
					m->cfg->use_mainlineout=0;
					m->cfg->use_wooflineout=0;
					m->cfg->use_meetinglineout=0;
					m->cfg->use_expandlineout=1;
				#endif
				}
			}
			sprintf(buf3,"echo %d > /oem/qdreamer/qsound/lineout_pattern.txt",ret);
			system(buf3);
			// wtk_debug("------------------------------------>>>>>now_delay= %.3f",time_get_ms()-set_lineout_time);
			// wtk_debug("=================m->cfg->use_mainlineout = %d\n",m->cfg->use_mainlineout);
			// wtk_debug("=================m->cfg->use_wooflineout = %d\n",m->cfg->use_wooflineout);
			// wtk_debug("=================m->cfg->use_meetinglineout = %d\n",m->cfg->use_meetinglineout);
			// wtk_debug("=================m->cfg->use_expandlineout = %d\n",m->cfg->use_expandlineout);
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
			ret = 0;

		}else if (strncmp(buf, "UNENABLE_MIC", 12) == 0)//禁用mic
		{
			int ret=-2;
			m->is_mic = 0;
			wtk_debug("-------->>>>>>>>>>>>>>m->is_mic=%d\n",m->is_mic);
			ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "ENABLE_MIC", 10) == 0)//启用mic
		{
			int ret=-2;
			m->is_mic=1;
			wtk_debug("-------->>>>>>>>>>>>>>mm->is_mic=%d\n",m->is_mic);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "UNENABLE_SPK", 12) == 0)//禁用spk
		{
			int ret=-2;
			m->cfg->use_speaker=0;
			wtk_debug("-------->>>>>>>>>>>>>>m->speaker_run=%d\n",m->cfg->use_speaker);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
			
		}else if (strncmp(buf, "ENABLE_SPK", 10) == 0)//启用SPK
		{
			int ret=-2;
			m->cfg->use_speaker=1;
			wtk_debug("-------->>>>>>>>>>>>>>m->speaker_run=%d\n",m->cfg->use_speaker);
			ret =0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "USE_UNENABLE_LINEIN", 19) == 0)//禁用LINEIN
		{
			int ret=-2;
			m->cfg->use_linein_mic=0;
			m->cfg->use_linein_courseware=0;
			m->linein_run=0;

			//system("sync");
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_mic=%d\n",m->cfg->use_linein_mic);
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_courseware=%d\n",m->cfg->use_linein_courseware);
			wtk_debug("-------->>>>>>>>>>>>>>m->linein_run=%d\n",m->linein_run);
			ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
			
		}else if (strncmp(buf, "USE_ENABLE_LINEIN", 17) == 0)//启用LINEIN
		{

			int ret=-2;
			m->cfg->use_line_in=1;
			// m->cfg->use_linein_mic=1;
			// m->cfg->use_linein_courseware=1;
			//system("sync");
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_mic=%d\n",m->cfg->use_linein_mic);
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_courseware=%d\n",m->cfg->use_linein_courseware);
			wtk_debug("-------->>>>>>>>>>>>>>m->linein_run=%d\n",m->linein_run);
			ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "ENABLE_LINEIN_MIC", 17) == 0)//启用LINEIN_MIC
		{
			int ret=-2;
			m->cfg->use_linein_mic = 1;
			m->cfg->use_linein_courseware = 0;
			// m->linein_run=1;
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_mic=%d\n",m->cfg->use_linein_mic);
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_courseware=%d\n",m->cfg->use_linein_courseware);
			wtk_debug("-------->>>>>>>>>>>>>>m->linein_run=%d\n",m->linein_run);
			ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "ENABLE_LINEIN_SPK", 17) == 0)//启用LINEIN_SPK
		{

			int ret=-2;
			m->cfg->use_linein_mic=0;
			m->cfg->use_linein_courseware=1;
			// m->linein_run=0;
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_mic=%d\n",m->cfg->use_linein_mic);
			wtk_debug("-------->>>>>>>>>>>>>>m->cfg->use_linein_courseware=%d\n",m->cfg->use_linein_courseware);
			wtk_debug("-------->>>>>>>>>>>>>>m->linein_run=%d\n",m->linein_run);
			ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "GET_MICVOLUME", 13) == 0)//检索当前麦克风音量级别
		{
			int ret;
			wtk_debug("======>>>>GET_MICVOLUME = %d\n", m->vboxebf->vebf3->agc_enable);
			val = m->vboxebf->vebf3->agc_enable;
			if(val==0)
			{
				float s;
				
				wtk_debug("mic_shift=%0.2f\n",m->vboxebf->vebf4->inmic_scale);
				s=((m->vboxebf->vebf4->inmic_scale-3.5)/(11-3.5))*100;
				ret=sprintf(buf2,"%d%%",(int)s);
				wtk_debug("GET_MICVOLUME=%s\n",buf2);
				// qtk_mod_am13e2_proc_write(m,buf2,strlen(buf2));
			}else{
				float s;
				wtk_debug("+=====>>>>>>>agc_a=%f\n",m->vboxebf->vebf3->cfg->agc_a);
				s =( 1- ( ( m->vboxebf->vebf4->cfg->agc_a - 0.19) / (0.69 - 0.19) ) )*100;
				ret=sprintf(buf2,"%d%%",(int)s);//0.19-0.69
				wtk_debug("GET_MICVOLUME=%s\n",buf2);
				// qtk_mod_am13e2_proc_write(m,buf2,strlen(buf2));
				// qtk_mod_proc_write(m,"-1",2);
			}
			ret = 0;
			// if(val==1)//开启agc
			// {
			// 	float s;
			// 	wtk_debug("+=====>>>>>>>agc_a=%f\n",m->vboxebf->vebf4->cfg->agc_a);
			// 	s =( 1- ( ( m->vboxebf->vebf4->cfg->agc_a - 0.19) / (0.69 - 0.19) ) )*100;
			// 	ret=sprintf(buf2,"%d%%",(int)s);//0.19-0.69
			// 	wtk_debug("GET_MICVOLUME=%s\n",buf2);
			// 	qtk_mod_proc_write(m,buf2,strlen(buf2));
			// }else{
			// 	float s;
				
			// 	// wtk_debug()
			// 	qtk_mod_proc_write(m,"-1",2);
			// }
		}else if (strncmp(buf, "SET_SPKVOLUME", 13) == 0)//控制扬声器
		{
			

			// qtk_mod_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "SPEAKER_CONRTOL", 13) == 0)
		{
			wtk_debug("-------->>>>>>>>>>>>>>>>>>>>>>SPEAKER_CONRTOL\n");
			int ret;
			if (sscanf(buf, "SPEAKER_CONRTOL%d", &ret)== 1){
				if(ret == 0){
					m->cfg->use_speaker_left=0;
					m->cfg->use_speaker_right=0;
					m->cfg->use_wooferout = 0;
				}
				if(ret == 1){
					m->cfg->use_speaker_left=1;
					m->cfg->use_speaker_right=0;
					m->cfg->use_wooferout = 0;
				}
				if(ret == 2){
					m->cfg->use_speaker_left=0;
					m->cfg->use_speaker_right=1;
					m->cfg->use_wooferout = 0;
				}
				if(ret == 3){
					m->cfg->use_speaker_left=0;
					m->cfg->use_speaker_right=0;
					m->cfg->use_wooferout = 1;
				}
				if(ret == 4){
					m->cfg->use_speaker_left=1;
					m->cfg->use_speaker_right=1;
					m->cfg->use_wooferout = 1;
				}
			}
			wtk_debug("=================m->cfg->use_speaker_left = %d\n",m->cfg->use_speaker_left);
			wtk_debug("=================m->cfg->use_speaker_right = %d\n",m->cfg->use_speaker_right);
			wtk_debug("=================m->cfg->use_wooferout = %d\n",m->cfg->use_wooferout);
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
			ret = 0;
		}else if (strncmp(buf, "SET_MICANC_ON", 13) == 0)//设置麦克风噪声抑制功能的启用状态
		{
			int ret=-2;
			wtk_mask_bf_net_set_denoiseenable(m->avboxebf->mask_bf_net,1);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
			
		}else if (strncmp(buf, "SET_MICANC_OFF", 14) == 0)
		{
			int ret=-2;
			wtk_mask_bf_net_set_denoiseenable(m->avboxebf->mask_bf_net,0);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "SET_MICAGC_ON", 13) == 0)//设置麦克风自动增益控制功能的启用状态
		{
			int ret=-2;
			wtk_mask_bf_net_set_agcenable(m->avboxebf->mask_bf_net,1);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));

		}else if (strncmp(buf, "SET_MICAGC_OFF", 14) == 0)
		{
			int ret=-2;
			wtk_mask_bf_net_set_agcenable(m->avboxebf->mask_bf_net,0);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "SET_MICAEC_ON", 13) == 0)//设置麦克风回声消除功能的启动状态
		{
			int ret=-2;
			wtk_mask_bf_net_set_echoenable(m->avboxebf->mask_bf_net,1);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}else if (strncmp(buf, "SET_MICAEC_OFF", 14) == 0)
		{
			int ret=-2;
			wtk_mask_bf_net_set_echoenable(m->avboxebf->mask_bf_net,0);
			// ret=0;
			// qtk_mod_am13e2_proc_write(m,wtk_itoa(ret),abs(ret));
		}

		
	}

	once = 0;
	wtk_thread_join(&mem_t);
	return 0;
}

int qtk_mod_am13e2_proc_write(qtk_mod_am13e2_t *m,char *data,int len)
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

int qtk_mod_am13e2_proc_send_pid(qtk_mod_am13e2_t *m)
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
