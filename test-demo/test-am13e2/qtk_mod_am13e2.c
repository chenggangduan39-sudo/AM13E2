#include "qtk_mod_am13e2.h"
#include "sdk/qtk_api.h"
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "wtk/core/wtk_os.h"
#include "wtk/core/wtk_riff.h"
#include <stdio.h>
#include <sys/file.h>
// #define WUTAOCESHI

#define USE_ATOMIC_FILE

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
#ifndef MIC_SHIFT
#define MIC_SHIFT powf(powf(10.0f, (12.0f / 20.0f)), 1.0f/50.0f)
#endif
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
static int debug_count=0;
static float uac_volume = 0.0f;
// static int start_play=0;
static double last_rcd_time = 0.0;
LowPassFilter lpf;
static int input_copunt = 0;
static int infor_count;
static double disconnect_time = 0.0;
static int usb_stata = 1;
qtk_mod_am13e2_t *lm;
#define RES_CFG_PATH "/oem/qdreamer/qsound/res/cfg"
#define UARTCFG_PATH "/oem/qdreamer/qsound/uart.cfg"

// FILE * low_filter=NULL;
// FILE * low_filter_befor=NULL;

int qtk_mod_am13e2_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_rcd2_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_rcd3_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_rcd4_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_merge_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_vbox_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_array_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_gainnet_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_gainnet2_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_gainnet3_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);

int qtk_mod_am13e2_usbaudio_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_lineout_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_spk_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);

int qtk_mod_am13e2_linein_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_linein_check_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);


void qtk_mod_am13e2_on_outresample(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf2(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf3(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf4(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf5(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf_3ch(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf2_3ch(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf3_3ch(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf4_3ch(qtk_mod_am13e2_t *m, char *data, int len);

void qtk_mod_am13e2_on_gainnetbf_4ch(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf2_4ch(qtk_mod_am13e2_t *m, char *data, int len);

void qtk_mod_am13e2_on_gainnetbf_6ch(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_gainnetbf2_6ch(qtk_mod_am13e2_t *m, char *data, int len);

int qtk_mod_am13e2_mic_check_rcd_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
int qtk_mod_am13e2_mic_check_play_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t);
void qtk_mod_am13e2_on_mic_check_rcd(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int nchn);
void qtk_mod_am13e2_on_mic_check_play(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int channenl);

void qtk_mod_am13e2_on_vboxebf(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_array_vboxebf(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_am13e2_on_denoise_vboxebf(qtk_mod_am13e2_t *m, char *data, int len);
void qtk_mod_uart_on_recv(qtk_mod_am13e2_t *m, qtk_uart_recv_frame_t *frame, int len);

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
int qtk_mod_atomic_write(const char *filename, const void *data, size_t len);
void qtk_mod_uac_volume_callback(void *user_data,int type,int value);
void qtk_mod_usb_callback(void *user_data,int type);
void qtk_mod_am13e2_time_callback(void *user_data,int type);
void qtk_mod_am13e2_check_usb();
// void qtk_mod_on_usb(void *ths, qtk_usb_uevent_state_t state, int sample_rate);
void qtk_init_100hz_lpf(LowPassFilter *filter);
void qtk_process_100hz_lpf(short *data, int len, LowPassFilter *filter);
static void process_audio_with_limiter(char* data, int data_size); //应用限幅器
void wtdebugTime();

static void _limiter(float *out, int fsize)
{
    int max_out = 32700;
    float fv = max_out/32768.0;
    float thresh = 0.95;
    float alpha = fv * thresh;
    float alpha_1 = fv * (1 - thresh);
    int i;

    for (i = 0; i < fsize; i++) {
        double x = out[i]/32768.0;
        if (-alpha <= x && x <= alpha) {
            // 直通区间，无需修改
            continue;
        } else if (x > alpha) {
            // 正向压缩
            out[i] = alpha + alpha_1 * (1 - exp(-(x - alpha)));
        } else {
            // 负向压缩
            out[i] = -alpha - alpha_1 * (1 - exp(-(-x - alpha)));
        }
        out[i] *= 32768.0;
    }
}
// char* 转 float* 的转换函数
static void char_to_float(const char* char_data, float* float_data, int samples)
{
    const int16_t* int16_data = (const int16_t*)char_data;
    for (int i = 0; i < samples; i++) {
        float_data[i] = (float)int16_data[i];
    }
}

// float* 转 char* 的转换函数  
static void float_to_char(const float* float_data, char* char_data, int samples)
{
    int16_t* int16_data = (int16_t*)char_data;
    for (int i = 0; i < samples; i++) {
        // 限制在int16_t范围内并四舍五入
        float val = float_data[i];
        if (val > 32767.0f) val = 32767.0f;
        if (val < -32768.0f) val = -32768.0f;
        int16_data[i] = (int16_t)roundf(val);
    }
}

// 处理音频数据的完整函数
static void process_audio_with_limiter(char* data, int data_size)
{
    // 假设是16位PCM音频，计算样本数
    int samples = data_size / 2;  // 每个样本2字节
    
    float* float_buffer = (float*)malloc(samples * sizeof(float));
    if (!float_buffer) {
        return;  
    }
    
    char_to_float(data, float_buffer, samples);
    
    _limiter(float_buffer, samples);
    
    float_to_char(float_buffer, data, samples);
    
    free(float_buffer);
}

int qtk_mod_atomic_write(const char *filename, const void *data, size_t len) {
    char temp_template[1024];
    char temp_path[1024];
    int fd, ret;
    
    // 创建临时文件
    snprintf(temp_template, sizeof(temp_template), "%s.XXXXXX", filename);
    fd = mkstemp(temp_template);
    if (fd == -1 ) { //== -1 改为<0
        return -1;
    }
    strcpy(temp_path, temp_template);
    // 写入数据到临时文件
    ssize_t written = write(fd, data, len);
    if (written != len) {
        close(fd);
        unlink(temp_path);
        return -1;
    }
    // 确保数据刷到磁盘
    if (fsync(fd) == -1 ) {  //== -1 改为<0
        close(fd);
        unlink(temp_path);
        return -1;
    }
    close(fd);
    // 原子性地重命名临时文件为目标文件
    if (rename(temp_path, filename) == -1 ) { // == -1  改为<0
        unlink(temp_path);
        return -1;
    }
    // 同步目录，确保重命名操作持久化
    int dir_fd = open(".", O_RDONLY);
    if (dir_fd != -1) {
        fsync(dir_fd);
        close(dir_fd);
    }
    return 0;
}
//音频检测函数
bool check_audio_data(const int16_t *data, int samples);
static int continuous_audio_check(qtk_mod_am13e2_t *m, const int16_t *data, int samples);
//音量值获取
static void continuous_get_volume(qtk_mod_am13e2_t *m, const int16_t *data, int samples);
#ifdef USE_3308

void wtdebugTime() {
    // 获取当前时间戳
    time_t currentTime;
    time(&currentTime);
    
    // 转换为本地时间结构
    struct tm *localTime = localtime(&currentTime);
    
    // 检查时间获取是否成功
    if (localTime == NULL) {
        wtk_debug("can not get time\n");
        return;
    }
    
    // 提取时、分、秒
    int hour = localTime->tm_hour;   // 时 (0-23)
    int minute = localTime->tm_min;  // 分 (0-59)
    int second = localTime->tm_sec;  // 秒 (0-59)
    
    // 打印格式化的时间
    wtk_debug("now time: %02d:%02d:%02d\n", hour, minute, second);
}
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

static void qtk_mod_am13e2_check_file(qtk_mod_am13e2_t *m)
{
	int ret,volume,s,len,count_towrite=0;
	wtk_debug("--------------->>>>>>>\n");
	char * readbuf;
	char * p;
	wtk_debug("--------------->>>>>>>\n");
	char *pq;
	FILE *fp;
	
	// readbuf = file_read_buf(UARTCFG_PATH , &s);
	// p = strstr(readbuf , "pingsuan_run=");
	// s =atoi(p + 13);
	// wtk_free(readbuf);
	// readbuf=file_read_buf(RES_CFG_PATH,&s);
	// pq = strstr(readbuf, "volum_input_mute=");
	// ret = atoi(pq + 17);
	// lm->cfg->volum_input_mute = ret;
	// readbuf=file_read_buf(RES_CFG_PATH,&s);
	// pq = strstr(readbuf, "volum_output_mute=");
	// ret = atoi(pq + 18);
	// m->cfg->volum_output_mute = ret;
	// wtk_free(readbuf);

	wtk_debug("---------------->>>>>>>s = %d ,m->cfg->volum_output_mute = %d\n",s ,m->cfg->volum_output_mute);

	while(1){
		count_towrite++;
		if(m->cfg->volum_output_mute){
			wtk_debug("------------->>>>>>>>>\n");
			if(access("/sys/bus/i2c/devices/3-0069/volume",F_OK) == 0 && access("/sys/bus/i2c/devices/3-0069/volume",W_OK) == 0)
			{
				wtk_debug("------------->>>>>>>>>\n");
				fp = fopen("/sys/bus/i2c/devices/3-0069/volume", "w");
				if (fp) {
					wtk_debug("------------->>>>>>>>>\n");
					fprintf(fp, "%d", 0);
					fclose(fp);
					wtk_debug("----------->>>>>>>>...3-0069volume is %d\n",0);
				}
				fp = fopen("/sys/bus/i2c/devices/3-006d/volume", "w");
				if (fp) {
					wtk_debug("------------->>>>>>>>>\n");
					fprintf(fp, "%d", 0);
					fclose(fp);
					wtk_debug("----------->>>>>>>>...3-006dvolume is %d\n",0);
				}
				wtk_debug("------------->>>>>>>>>\n");
				break;
			}
		}else{
			if(access("/sys/bus/i2c/devices/3-0069/volume",F_OK) == 0 && access("/sys/bus/i2c/devices/3-0069/volume",W_OK) == 0)
			{
				char*datap=file_read_buf("/oem/qdreamer/qsound/spk_volume.txt",&len);
				volume=atoi(datap);
				fp = fopen("/sys/bus/i2c/devices/3-0069/volume", "w");
				if (fp) {
					fprintf(fp, "%d", volume);
					fclose(fp);
					wtk_debug("----------->>>>>>>>...3-0069volume is %d\n",volume);
				}
				fp = fopen("/sys/bus/i2c/devices/3-006d/volume", "w");
				if (fp) {
					fprintf(fp, "%d", volume);
					fclose(fp);
					wtk_debug("----------->>>>>>>>...3-006dvolume is %d\n",volume);
				}
				wtk_free(datap);
				break;
			}
		}
		if(count_towrite > 3)
			break;
		usleep(10*1000);
	}
}

static void continuous_get_volume(qtk_mod_am13e2_t *m, const int16_t *data, int samples){
	// double get_db_time = time_get_ms();
	double db_get_result= get_db((short *)data, samples);
	// wtk_debug("-------------------->>>>>>>get_db_time= %.3f\n",time_get_ms()-get_db_time);
	// wtk_debug("------------>>>>>>>>>>>>>>>>>db_get_result= %f\n",db_get_result);
	if(db_get_result < -44.5)
		db_get_result = -96.0;
	m->real_time = db_get_result + 96.0;
	// wtk_debug("------------>>>>>>>>>>>>>>>>>m->real_time = %f\n",m->real_time);
}
// FILE * gainnet2_fn;
qtk_mod_am13e2_t *qtk_mod_am13e2_new(qtk_mod_am13e2_cfg_t *cfg)
{
	// gainnet2_fn=fopen("/tmp/gainnet2.pcm","wb");
	// if(gainnet2_fn==NULL){
	// 	wtk_debug("fopen filed!!\n");
	// 	exit(1);
	// }
	wtdebugTime();
	qtk_usb_uevent_t *qu=NULL;
	qtk_mod_am13e2_t *m;
	char tmp[64];
	int ret;
	int opri,npri;
	char set_buf[128]={0};
	char res_buf[4096] = {0};
	opri = getpriority(PRIO_PROCESS, getpid());
	wtk_debug("=================>>>>>>>>>>old:%d\n",opri);
	npri = -20;
	setpriority(PRIO_PROCESS, getpid(), npri);
	opri = getpriority(PRIO_PROCESS, getpid());
	wtk_debug("=================>>>>>>>>>>new:%d\n",opri);
	FILE *fp=NULL;
	m = (qtk_mod_am13e2_t *)wtk_calloc(1, sizeof(*m));
	lm = m;
	m->cfg = cfg;

	qtk_mod_am13e2_check_file(m);

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
	m->sttimer=NULL;
	m->mic_channel=m->cfg->rcd.channel - m->cfg->rcd.nskip;
	m->is_player_start=0;
	m->is_mic =1;
	m->is_use_uac = 1;
	m->audio_check_rcd_feed_end = 0;
	m->audio_check_play_feed_end = 0;
	m->c5_pull_high =1;
	m->real_time = 0.0;
	//wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>>>>mic_channel=%d / %d ns=%d\n",m->mic_channel,m->cfg->rcd.channel,m->cfg->rcd.nskip);
	// wtk_debug("------------------------------>>>>>>>>>>>>>>>>\n");
	m->speaker_left_audiobuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(m->speaker_left_audiobuf);
	m->speaker_all_audiobuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(m->speaker_all_audiobuf);
	m->lineout_left_audiobuf = wtk_strbuf_new(2048, 1.0);
	wtk_strbuf_reset(m->lineout_left_audiobuf);
	m->lineout_all_audiobuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(m->lineout_all_audiobuf);
	m->check_path_buf = wtk_strbuf_new(64,0);
	m->mul_path = wtk_strbuf_new(64,0);
	m->iis_path = wtk_strbuf_new(64,0);
	m->spk_path = wtk_strbuf_new(64,0);
	m->play_path = wtk_strbuf_new(64, 0);
	m->uac_path = wtk_strbuf_new(64,0);

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
	#if 1
		wtk_strbuf_push_f(m->uac_path, "%.*s/uac.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->uac_path, 0);
	#endif
	#if 1
		wtk_strbuf_push_f(m->mul_path, "%.*s/mul.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->mul_path, 0);
	#endif 
	#if 1
		wtk_strbuf_push_f(m->arraymul_path, "%.*s/arraymul.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->arraymul_path, 0);
	#endif	
	#if 0
		wtk_strbuf_push_f(m->iis_path, "%.*s/iis.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->iis_path, 0);
	#endif
	#if 1
		wtk_strbuf_push_f(m->play_path, "%.*s/play.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->play_path, 0); 
	#endif
	#if 0
		wtk_strbuf_push_f(m->spk_path, "%.*s/spk.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->spk_path, 0);
	#endif
	#if 0
		wtk_strbuf_push_f(m->lineout_path, "%.*s/lineout.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->lineout_path, 0);
	#endif
	#if 0
		wtk_strbuf_push_f(m->uac_path, "%.*s/uac.wav", m->cfg->cache_path.len, m->cfg->cache_path.data);
		wtk_strbuf_push_c(m->uac_path, 0);
	#endif
	}
    qtk_init_100hz_lpf(&lpf);
#ifndef OFFLINE_TEST
	if(m->cfg->use_log && m->cfg->cache_path.len > 0){
		// snprintf(tmp, 64, "%.*s/qvoice_rp.log", m->cfg->cache_path.len, m->cfg->cache_path.data);
		// m->log = wtk_log_new(tmp);
		m->log = NULL;
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

		m->gainnetbf5 = qtk_gainnetbf_new(m->cfg->gainnetbf_cfg);
		if(!m->gainnetbf5){
			wtk_debug("gainnetbf5_rtjoin new failed!\n");
			m->gainnetbf5 = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf5_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf5, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf5);
	}
	if(m->cfg->gainnetbf2_cfg){
		m->gainnetbf_3ch = qtk_gainnetbf_new(m->cfg->gainnetbf2_cfg);
		if(!m->gainnetbf_3ch){
			wtk_debug("gainnetbf_3ch_rtjoin new failed!\n");
			m->gainnetbf_3ch = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf_3ch_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf_3ch, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf_3ch);

		m->gainnetbf2_3ch = qtk_gainnetbf_new(m->cfg->gainnetbf2_cfg);
		if(!m->gainnetbf2_3ch){
			wtk_debug("gainnetbf2_3ch_rtjoin new failed!\n");
			m->gainnetbf2_3ch = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf2_3ch_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf2_3ch, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf2_3ch);

		m->gainnetbf3_3ch = qtk_gainnetbf_new(m->cfg->gainnetbf2_cfg);
		if(!m->gainnetbf3_3ch){
			wtk_debug("gainnetbf3_3ch_rtjoin new failed!\n");
			m->gainnetbf3_3ch = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf3_3ch_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf3_3ch, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf3_3ch);

		m->gainnetbf4_3ch = qtk_gainnetbf_new(m->cfg->gainnetbf2_cfg);
		if(!m->gainnetbf4_3ch){
			wtk_debug("gainnetbf4_3ch_rtjoin new failed!\n");
			m->gainnetbf4_3ch = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf4_3ch_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf4_3ch, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf4_3ch);
	}
	if(m->cfg->gainnetbf3_cfg)
	{
		m->gainnetbf_4ch = qtk_gainnetbf_new(m->cfg->gainnetbf3_cfg);
		if(!m->gainnetbf_4ch){
			wtk_debug("gainnetbf_4ch_rtjoin new failed!\n");
			m->gainnetbf_4ch = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf_4ch_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf_4ch, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf_4ch);
		m->gainnetbf2_4ch = qtk_gainnetbf_new(m->cfg->gainnetbf3_cfg);
		if(!m->gainnetbf2_4ch){
			wtk_debug("gainnetbf2_4ch_rtjoin new failed!\n");
			m->gainnetbf2_4ch = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf2_4ch_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf2_4ch, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf2_4ch);
	}
	if(m->cfg->gainnetbf4_cfg)
	{
		m->gainnetbf_6ch = qtk_gainnetbf_new(m->cfg->gainnetbf4_cfg);
		if(!m->gainnetbf_6ch){
			wtk_debug("gainnetbf_6ch_rtjoin new failed!\n");
			m->gainnetbf_6ch = NULL;
		}
		wtk_debug("----------------------->>>>>>>>>>gainnetbf_6ch_rtjoin new success!\n");
		qtk_gainnetbf_set_notify(m->gainnetbf_6ch, m, (qtk_gainnetbf_notify_f)qtk_mod_am13e2_on_gainnetbf_6ch);
	}
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
	if(m->cfg->mic_check_play_cfg){
		m->mic_check_play = wtk_mic_check_new(m->cfg->mic_check_play_cfg);
		if(!m->mic_check_play){
			wtk_debug("mic_check_play new failed!\n");
			m->mic_check_play = NULL;
		}
		wtk_mic_check_set_notify(m->mic_check_play, m,(wtk_mic_check_notify_f)qtk_mod_am13e2_on_mic_check_play);
	}
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
	wtk_debug("=====>>>>>>>>>use_linein=%d,denoisebf_cfg=%p\n",m->cfg->use_line_in,m->cfg->denoisebf_cfg);
	if(m->cfg->use_line_in){
		if(m->cfg->denoisebf_cfg){
			wtk_debug("--------->>>>>>>>\n");
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
		wtk_debug("----------------->>>>>>>>>>.\n");
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
	if(m->cfg->use_speaker){
		wtk_blockqueue_init(&m->spk_queue);
		wtk_thread_init(&m->speaker_t,(thread_route_handler)qtk_mod_am13e2_spk_entry, m);
		wtk_debug("qtk_mod_am13e2_spk_entry is ok!!\n");
		m->speaker = qtk_play_new(&cfg->speaker);
	}
#endif
	wtk_debug("------------------->>>>>>>>>>>>>>>\n");
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

	m->rcd3 = qtk_record_new(&(m->cfg->rcd3));
	if(!m->rcd3){
		wtk_log_err0(m->log, "record3 failed!");
		ret = -1;
		goto end;
	}

	wtk_debug("--------------------------->>>>>>>>\n");
	wtk_thread_init(&m->rcd3_t, (thread_route_handler)qtk_mod_am13e2_rcd3_entry, m);
	m->rcd4 = qtk_record_new(&(m->cfg->rcd4));
	if(!m->rcd4){
		wtk_log_err0(m->log, "record4 failed!");
		ret = -1;
		goto end;
	}

	 m->dev_event = qtk_dev_event_new2(QTK_DEV_USB_EVENT);
     wtk_debug("-------------------------------------------------------------------\n");
    if(m->dev_event)
    {
        qtk_dev_event_set_value_notify(m->dev_event,qtk_mod_uac_volume_callback,m);
		qtk_dev_event_set_notify(m->dev_event ,qtk_mod_usb_callback,m );
        m->current_volume = 50;
        wtk_debug(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>UAC  volume control initialized\n");
    }

	if(m->cfg->use_uart){
		m->uart = qtk_uart_client_new(m->cfg->uart_cfg, m->log);
		qtk_uart_client_set_notify(m->uart, m, (qtk_uart_recv_notify_f)qtk_mod_uart_on_recv);
	}

	wtk_thread_init(&m->rcd4_t, (thread_route_handler)qtk_mod_am13e2_rcd4_entry, m);
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
	if(m->cfg->use_speaker){
		wtk_blockqueue_clean(&m->spk_queue);
		if(m->speaker){
			qtk_play_delete(m->speaker);
		}
	}
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
	if(m->uac_path){
		wtk_strbuf_delete(m->uac_path);
	}
	if(m->spk_path){
		wtk_strbuf_delete(m->spk_path);
	}
#if 1
	if(m->iis_path){
		wtk_strbuf_delete(m->iis_path);
	}
	if(m->mul_path){
		wtk_strbuf_delete(m->mul_path);
	}
#endif
	if(m->play_path){
		wtk_strbuf_delete(m->play_path);
	}
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
	if(m->cfg->use_uart && m->uart){
		qtk_uart_client_delete(m->uart);
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
		wtk_debug("---------------->>>>>>>>>>>>\n");
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
	if(m->cfg->use_speaker && m->speaker_run){
		m->speaker_run = 0;
		wtk_blockqueue_wake(&m->spk_queue);
		wtk_thread_join(&m->speaker_t);
	}
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
	if(m->spk_queue.length > 0){
		qtk_mod_am13e2_clean_queue(m, &m->spk_queue);
	}
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
	wtk_debug("------------->>>>>>>\n");
	if(m->gainnetbf){
		m->gainnet_run=1;
		wtk_thread_start(&m->gainnet_t);
	}
	wtk_debug("------------->>>>>>>\n");
    if(m->dev_event)
    {
        wtk_debug("-------------------------------------------------\n");
        qtk_dev_event_start2(m->dev_event);
    }
    wtk_debug("-------------------------------------------------\n");
	if(m->gainnetbf2){
		m->gainnet2_run=1;
		wtk_thread_start(&m->gainnet2_t);
	}
	wtk_debug("------------->>>>>>>\n");
	if(m->gainnetbf3){
		m->gainnet3_run=1;
		wtk_thread_start(&m->gainnet3_t);
	}
	wtk_debug("------------->>>>>>>\n");
	if(m->mic_check_rcd){
		wtk_mic_check_start(m->mic_check_rcd);
		m->mic_check_rcd_run = 1;
		wtk_thread_start(&m->mic_check_rcd_t);
	}
	wtk_debug("------------->>>>>>>\n");
	if(m->mic_check_play){
		wtk_mic_check_start(m->mic_check_play);
		m->mic_check_play_run = 1;
		wtk_thread_start(&m->mic_check_play_t);
	}
	wtk_debug("------------->>>>>>>\n");
	if(m->vboxebf){
		qtk_vboxebf_start(m->vboxebf);
		m->vbox_run=1;
		wtk_thread_start(&m->vbox_t);
	}
	wtk_debug("------------->>>>>>>\n");
	if(m->cfg->use_array){
		qtk_vboxebf_start(m->avboxebf);
		m->array_vbox_run=1;
		wtk_thread_start(&m->array_vbox_t);
	}
	wtk_debug("------------->>>>>>>\n");
	if(m->cfg->use_line_in && m->linein_run == 0){
		qtk_vboxebf_start(m->denoisebf);
		m->denoise_vbox_run=1;
		m->linein_run = 1;
		wtk_thread_start(&m->denoise_vbox_t);
		wtk_thread_start(&m->linein_t);
	}

// #ifndef USE_3308
// 	if(m->cfg->use_out_resample && (m->cfg->usbaudio.sample_rate != 48000)){
// 		wtk_resample_start(m->outresample, 48000, m->cfg->usbaudio.sample_rate);
// 	}
// #endif
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
		if(m->cfg->use_speaker && m->speaker_run==0){
			qtk_play_start(m->speaker);
			m->speaker_run = 1;
			wtk_thread_start(&m->speaker_t);
			wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>>>>>>\n");
		}
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
	if(m->cfg->use_uart){
		qtk_uart_client_start(m->uart);
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
		if(m->cfg->use_speaker){
			m->speaker_run = 0;
			wtk_blockqueue_wake(&m->spk_queue);
			wtk_thread_join(&m->speaker_t);
		}
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
	 if(m->dev_event)
    {
        qtk_dev_event_delete2(m->dev_event);
        m->dev_event = NULL;
    }
     if(m->dev_event)
    {
        qtk_dev_event_stop2(m->dev_event);
    }

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
	if(m->cfg->use_uart){
		qtk_uart_client_stop(m->uart);
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
			wtk_debug("---------------------->>>>>>>>>>>\n");
		}
		wtk_resample_start(m->outresample, 48000, sample_rate);
		wtk_debug("------------------->>>>>>>>>>..\n");
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
		msg_node = qtk_msg_pop_node(m->msg);
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
			wtk_strbuf_push(msg_node->buf, data, poslen);
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
		pv = (short *)wtk_malloc(sizeof(short) * 1 * 8192);
	}
	memcpy(pv,data,len);
	int src_pos=0;
	if(len <= 0){return;}
	int poslen=len>>1<<1;
	// int poslen =len;
	if(m->cfg->use_lineout && m->lineout_run && m->cfg->use_headset ){
		// wtk_debug("===============>>>>>>>>>>len=%d poslen=%d\n",len,poslen);
		qtk_process_100hz_lpf((short *)pv,poslen>>1,&lpf);
		qtk_msg_node_t *msg_node;
		char zdata[32]={0};
		msg_node = qtk_msg_pop_node(m->msg);
		if(m->cfg->lineout.channel > 1){
			int i=0;
			while(i<poslen){
				if (m->cfg->use_wooflineout) {
					wtk_strbuf_push(msg_node->buf, pv + i, 2);
					wtk_strbuf_push(msg_node->buf, pv + i+2, 2);
				}else{
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
	// wtk_debug("----------------->>>>>>>>>>\n");
	// wtk_debug("------------------>>>>len=%d\n",len);
    // if(pv == NULL){
	// 	pv = (short *)wtk_malloc(sizeof(short) * 1 * len);
	// }
	// double player_start = time_get_ms();
	// memcpy(pv,data,len);
	
#if 1
	qtk_msg_node_t *spk_check;
	spk_check = qtk_msg_pop_node(m->msg);
	spk_check->type = qtk_mod_am13e2_spk2;
	wtk_strbuf_push(spk_check->buf, data, len);
	wtk_blockqueue_push(&m->mic_check_play_queue, &spk_check->qn);
#endif
	int src_pos=0;
	if(len <= 0){return;}
	int poslen=len>>1<<1;
	// wtk_debug("--------------->>>use_lineout=%d,use_speaker=%d,use_lineout_queue.length=%d,use_spkout=%d,use_speaker_left=%d,use_speaker_right=%d,use_wooferout=%d,lineout.channel=%d\n",m->cfg->use_lineout,m->cfg->use_speaker,m->lineout_queue.length,m->cfg->use_spkout,m->cfg->use_speaker_left,m->cfg->use_speaker_right,m->cfg->use_wooferout,m->cfg->lineout.channel);
	if(m->cfg->use_lineout && m->cfg->use_speaker){
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
				i += 4; 
			}
		}
		// fwrite(msg_node->buf,msg_node->buf->pos,1,gainnet2_fn);
		// wtk_debug("--------->>>>>>>>>>>>>uac.pos = %d\n",msg_node->buf->pos);
		wtk_blockqueue_push(&m->spk_queue, &msg_node->qn);

		// double player_time = time_get_ms() - player_start;
		//  wtk_debug("---------------->>>>>>>play_time=%.3fms, lineout_queue_len=%d\n", player_time, m->lineout_queue.length);
	}
}
int qtk_mod_am13e2_gainnet_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	wtk_debug("-------------------------------------------------->>>>\n");
	qtk_mod_am13e2_set_cpu(m, t, 0);
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
	int lineintouac_pos=0;
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
			// wtk_debug("----------------------------------->>>>>>>>>\n");s
			wtk_strbuf_push(lineinbuf, msg_node->buf->data, msg_node->buf->pos);
			break;
		case qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE_TOUAC:
			// wtk_debug("----------------------------------->>>>>>>>>\n");
			wtk_strbuf_push(lineintouac, msg_node->buf->data,msg_node->buf->pos);
			break;	
		default:
			break;
		}
		if(debug_count == 100)
		{
			wtk_debug("use_linein_out=%d,arraybuf.pos=%d,lineinbuf.pos=%d,lineintouac.pos=%d\n",m->use_linein_out,arraybuf->pos,lineinbuf->pos,lineintouac->pos);
			debug_count = 0;
		}	
		// wtk_debug("---->>>m->cfg->use_linein_mic =%d,m->cfg->use_linein_courseware_touac=%d,arraybuf->pos=%d,lineinbuf->pos=%d,lineintouac->pos=%d\n",m->cfg->use_linein_mic,m->cfg->use_linein_courseware_touac,arraybuf->pos,lineinbuf->pos,lineintouac->pos);
		if(m->cfg->use_meetinglineout){
			if(arraybuf->pos >= pos){
					wtk_strbuf_reset(tbuf);
					if(m->cfg->use_out_resample){
						// wtk_debug("--------------------resample\n");
						wtk_resample_feed(m->outresample, arraybuf->data, arraybuf->pos, 0);
					}else{
						// wtk_debug("-------------------->>>>>>>>>>>>>>>>\n");
						qtk_mod_am13e2_player_mode(m, arraybuf->data, arraybuf->pos);
					}
					wtk_strbuf_pop(arraybuf, NULL, pos);
				}
		}else{
			if(m->cfg->use_array == 1 && m->cfg->use_linein_mic && m->use_linein_out){
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
			}else if(m->use_linein_out && m->cfg->use_linein_mic == 0 && m->cfg->use_linein_courseware_touac == 1 && m->cfg->use_linein_courseware){
				if(arraybuf->pos >= pos && lineintouac->pos >=2*pos){
					wtk_strbuf_reset(tbuf);
					int i=0,j=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, arraybuf->data+i, 2);
						wtk_strbuf_push(tbuf, lineintouac->data+j, 2);
						i+=2;
						j+=4;
					}
					qtk_gainnetbf_feed(m->gainnetbf, tbuf->data, tbuf->pos, 0);
					
					wtk_strbuf_pop(arraybuf, NULL, pos);
					wtk_strbuf_pop(lineintouac, NULL, 2*pos);
				}
			}else if(m->use_linein_out && m->cfg->use_array == 1 && m->cfg->use_linein_mic == 0 && m->cfg->use_linein_courseware == 1 && !m->cfg->use_linein_courseware_touac){
				if(arraybuf->pos >= pos){
					// wtk_debug("------------------>>>>>>>>>>>>>\n");
					wtk_strbuf_reset(tbuf);
					// int i=0,j=0;
					// while(i<pos){
					// 	for(j=0;j<2;++j)
					// 	{
					// 		wtk_strbuf_push(tbuf, arraybuf->data+i, 2);
					// 	}
					// 	i+=2;
					// }
					// qtk_gainnetbf_feed(m->gainnetbf, tbuf->data, tbuf->pos, 0);
					if(m->cfg->use_out_resample){
						// wtk_debug("--------------------resample\n");
						wtk_resample_feed(m->outresample, arraybuf->data, arraybuf->pos, 0);
					}else{
						// wtk_debug("-------------------->>>>>>>>>>>>>>>>\n");
						qtk_mod_am13e2_player_mode(m, arraybuf->data, arraybuf->pos);
					}
					wtk_strbuf_pop(arraybuf, NULL, pos);
				}
			}else if(arraybuf->pos >= pos){
				if(lineintouac->pos > 0)
					wtk_strbuf_pop(lineintouac, NULL, lineintouac->pos);
				if(lineinbuf->pos > 0)
					wtk_strbuf_pop(lineinbuf, NULL, lineinbuf->pos);
				wtk_strbuf_reset(tbuf);
					// wtk_debug("------------------>>>>>>>>>>>>>\n");
				// int i=0,j=0;
				// while(i<pos){
				// 	for(j=0;j<2;++j)
				// 	{
				// 		wtk_strbuf_push(tbuf, arraybuf->data+i, 2);
				// 	}
				// 	i+=2;
				// }
				// qtk_gainnetbf_feed(m->gainnetbf, tbuf->data, tbuf->pos, 0);
				if(m->cfg->use_out_resample){
					// wtk_debug("--------------------resample\n");
					wtk_resample_feed(m->outresample, arraybuf->data, arraybuf->pos, 0);
				}else{
					// wtk_debug("-------------------->>>>>>>>>>>>>>>>\n");
					qtk_mod_am13e2_player_mode(m, arraybuf->data, arraybuf->pos);
				}
					wtk_strbuf_pop(arraybuf, NULL, pos);
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
	
	qtk_mod_am13e2_set_cpu(m, t, 0);
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
	tbuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(tbuf);
	ttbuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(ttbuf);
	uacbuf = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(uacbuf);
	lineincourseware = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(lineincourseware);
	lineinmic = wtk_strbuf_new(4096, 1.0);
	wtk_strbuf_reset(lineinmic);
	qtk_gainnetbf_start(m->gainnetbf2);
	qtk_gainnetbf_start(m->gainnetbf4);
	qtk_gainnetbf_start(m->gainnetbf_3ch);
	qtk_gainnetbf_start(m->gainnetbf2_3ch);
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
	// FILE * gainnet3_fn;
	// gainnet3_fn=fopen("/tmp/gainnet3.pcm","wb");
	// if(gainnet3_fn==NULL){
	// 	wtk_debug("fopen filed!!\n");
	// 	exit(1);
	// }
	while(m->gainnet2_run){
		qn= wtk_blockqueue_pop(&m->gainnet2_queue,-1,NULL);
		
		// wtk_debug("-----------------------m->cfg->use_linein_courseware =%d\n",m->cfg->use_linein_courseware);
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		switch (msg_node->type)
		{
		
		case qtk_mod_am13e2_DATA_IIS_TOSPK:
			// wtk_debug("------------>>>>>>>>>>>>>>");
			wtk_strbuf_push(i2sbuf, msg_node->buf->data, msg_node->buf->pos);
			break;
		case qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE:
			wtk_strbuf_push(lineincourseware, msg_node->buf->data, msg_node->buf->pos);
			break;
		case qtk_mod_am13e2_DATA_UAC_TOSPK:
			wtk_strbuf_push(uacbuf, msg_node->buf->data, msg_node->buf->pos);
			break;
		default:
			break;
		}
		if(debug_count == 100)
		{
			wtk_debug("i2sbuf.pos=%d,lineincourseware.pos=%d,uacbuf.pos=%d,tbuf.pos=%d,ttbuf.pos=%d\n",i2sbuf->pos,lineincourseware->pos,uacbuf->pos,tbuf->pos,ttbuf->pos);
			debug_count =0;
		}
			// wtk_debug("-merge_rcd_queue.length=%d,linein_queue.length=%d,gainnet2_queue.length=%d\n",m->merge_rcd_queue.length,m->linein_queue.length,m->gainnet2_queue.length);
		// wtk_debug("-vbox_queue.length=%d,gainnet3_queue.length=%d,lineout_queue.length=%d,mic_check_rcd_queue.length=%d,mic_check_play_queue.length=%d\n",m->vbox_queue.length,m->gainnet3_queue.length,m->lineout_queue.length,m->mic_check_rcd_queue.length,m->mic_check_play_queue.length);
		// wtk_debug("-usbaudio_queue.length=%d,gainnet3_queue.length=%d,array_vbox_queue.length=%d,denoise_vbox_queue.length=%d\n",m->usbaudio_queue.length,m->gainnet3_queue.length,m->array_vbox_queue.length,m->denoise_vbox_queue.length);
		if(!m->cfg->use_mainlineout && !m->cfg->use_wooflineout && !m->cfg->use_expandlineout && !m->cfg->use_meetinglineout){
			if(m->use_linein_out && m->cfg->use_linein_courseware && i2sbuf->pos == 0 && uacbuf->pos != 0 &&!m->cfg->use_linein_courseware_touac){
				// wtk_debug("--------------------------->>>>.lineincourseware->pos =%d,uacbuf->pos = %d\n",lineincourseware->pos,uacbuf->pos);
				if(lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos) {
					// wtk_debug("--------------------->>>>>>>>>>>>>>>>\n");
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						i+=4;
					}
					// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
					qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
					wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
					wtk_strbuf_pop(uacbuf, NULL, 2*pos);
				}
			}else if (m->use_linein_out && m->cfg->use_linein_courseware && uacbuf->pos == 0 && i2sbuf->pos !=0 && !m->cfg->use_linein_courseware_touac){
				if(!m->cfg->use_onlylineinspk){
					if(i2sbuf->pos >= 3*pos && lineincourseware->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
					}
					if(i2sbuf->pos >=2*pos && lineincourseware->pos >=2*pos){
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<2*pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
							// wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
							i+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						#if 1
						qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
						#endif
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
					}
				}
				else{
					if(lineincourseware->pos >= pos){
						wtk_strbuf_reset(tbuf);
						int i=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+i+2, 2);
							i+=4;
						}
						qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);
						wtk_strbuf_pop(i2sbuf, NULL, pos);
						wtk_strbuf_pop(lineincourseware, NULL, pos);
					}
				}
			}else if(m->use_linein_out && m->cfg->use_linein_courseware && i2sbuf->pos != 0 && uacbuf->pos != 0 &&!m->cfg->use_linein_courseware_touac){
				// if((lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos) || (lineincourseware->pos > 2*pos)) {
					if(i2sbuf->pos >= 3*pos && lineincourseware->pos >= 2*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineincourseware, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}else if(i2sbuf->pos >= 3*pos && uacbuf->pos >=3*pos && lineincourseware->pos <= 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
					}
					if(lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos && i2sbuf->pos >= 2*pos) {	
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						wtk_strbuf_push(tbuf, i2sbuf->data+i,2);
						wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						wtk_strbuf_push(ttbuf, i2sbuf->data+i+2,2);
						i+=4;
					}
					// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
					qtk_gainnetbf_feed(m->gainnetbf_3ch, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf2_3ch, ttbuf->data, ttbuf->pos, 0);
					wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
					wtk_strbuf_pop(uacbuf, NULL, 2*pos);
					wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
				}
			}
			else if (m->cfg->use_rcd3 == 1 && uacbuf->pos == 0 && i2sbuf->pos != 0 ){
				if(lineincourseware->pos > pos)
				{
					wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
				}
				if(i2sbuf->pos >= pos ){
					wtk_strbuf_reset(tbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
						i+=4;
					}
					qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);
					wtk_strbuf_pop(i2sbuf, NULL, pos);
				}
			}
			else if (m->cfg->use_rcd3 == 1 && uacbuf->pos != 0 && i2sbuf->pos != 0){
				if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
				{
					wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
					wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
				}
				if(i2sbuf->pos >= 2*pos && uacbuf->pos >= 2*pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
						i+=4;
					}
					
					qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
					wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
					wtk_strbuf_pop(uacbuf, NULL, 2*pos);
				}
			}
			else if (m->cfg->use_rcd4 == 1 && i2sbuf->pos == 0){
				// wtk_debug("--------------------------->>>>>>>>>>>>>>>>>>>>>>>>\n");
				if(uacbuf->pos >= pos){
					wtk_strbuf_reset(tbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, uacbuf->data+i+2, 2);
						i+=4;
					}
					
					qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);

					// qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					// wtk_debug("----------------------------------tbuf->pos =%d\n",tbuf->pos);
					wtk_strbuf_pop(uacbuf, NULL, pos);
				}
			}
		}
		else{
			// if(m->cfg->use_mainlineout != 1 || m->use_lineout_out == 0)
			// {
				if(!m->use_lineout_out && m->use_linein_out && m->cfg->use_linein_courseware && uacbuf->pos == 0 && i2sbuf->pos !=0 && !m->cfg->use_linein_courseware_touac){
					// wtk_debug("--------------_>>>>>>>>>>>\n");
					if(i2sbuf->pos >=2*pos && lineincourseware->pos >=2*pos){
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<2*pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
							// wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
							i+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						#if 1
						qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
						#endif
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
					}
				}else if(!m->use_lineout_out && m->use_linein_out && m->cfg->use_linein_courseware && i2sbuf->pos != 0 && uacbuf->pos != 0 &&!m->cfg->use_linein_courseware_touac){
					// wtk_debug("--------------_>>>>>>>>>>>\n");
						if(i2sbuf->pos >= 3*pos && lineincourseware->pos >= 2*pos && uacbuf->pos < 2*pos)
						{
							wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
							wtk_strbuf_pop(lineincourseware, NULL, 3*pos);
							wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
						}else if(i2sbuf->pos >= 3*pos && uacbuf->pos >=3*pos && lineincourseware->pos <= 2*pos)
						{
							wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
							wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
						}
						if(lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos && i2sbuf->pos >= 2*pos) {	
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<2*pos){
							wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+i,2);
							wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
							wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+i+2,2);
							i+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
						qtk_gainnetbf_feed(m->gainnetbf_3ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf2_3ch, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
					}
				}
				else if (uacbuf->pos == 0 && i2sbuf->pos !=0){
					// wtk_debug("--------------_>>>>>>>>>>>\n");
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						// wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}
					if(i2sbuf->pos >= 2*pos){
						wtk_strbuf_reset(tbuf);
						int i=0;
						while(i<2*pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
							i+=4;
						}
						qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
					}
				}else if (m->cfg->use_rcd4 == 1 && i2sbuf->pos == 0){
					// wtk_debug("--------------------------->>>>>>>>>>>>>>>>>>>>>>>>\n");
					if(uacbuf->pos >= 2*pos){
						wtk_strbuf_reset(tbuf);
						int i=0;
						while(i<2*pos){
							wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+i+2, 2);
							i+=4;
						}

						// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);

						qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);
						// wtk_debug("------------->>>>>>>>>>>>>>>tbuf.pos=%d\n",tbuf->pos);
						// qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
						// wtk_debug("----------------------------------tbuf->pos =%d\n",tbuf->pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);
					}
				}else if (m->cfg->use_rcd3 == 1 && uacbuf->pos != 0 && i2sbuf->pos != 0){
					// wtk_debug("--------------_>>>>>>>>>>>\n");
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}
					if(i2sbuf->pos >= 2*pos && uacbuf->pos >= 2*pos){
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<2*pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
							i+=4;
						}
						#if 0
							fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
							fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						#endif
						qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);
					}
				}
			}
		// }
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

	qtk_mod_am13e2_set_cpu(m, t, 0);
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
	qtk_gainnetbf_start(m->gainnetbf5);

	qtk_gainnetbf_start(m->gainnetbf3_3ch);
	qtk_gainnetbf_start(m->gainnetbf4_3ch);

	qtk_gainnetbf_start(m->gainnetbf_4ch);
	qtk_gainnetbf_start(m->gainnetbf2_4ch);

	qtk_gainnetbf_start(m->gainnetbf_6ch);
	int lineincnt=0;
	double wait_start_time = 0;
    int is_waiting = 0;
	char zdata[32]={0};
	#if 0
	FILE * gainnet3ch_fn;
	gainnet3ch_fn=fopen("/tmp/gainnet3ch1.pcm","wb");
	if(gainnet3ch_fn==NULL){
		wtk_debug("fopen filed!!\n");
		exit(1);
	}
	FILE * gainnet3ch_2fn;
	gainnet3ch_2fn=fopen("/tmp/gainnet3ch2.pcm","wb");
	if(gainnet3ch_2fn==NULL){
		wtk_debug("fopen filed!!\n");
		exit(1);
	}
	#endif
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
			case qtk_mod_am13e2_DATA_IIS_TOLINEOUT:
				wtk_strbuf_push(i2sbuf, msg_node->buf->data, msg_node->buf->pos);
				// wtk_debug("------->>>>>>>i2sbuf->pos = %d\n",i2sbuf->pos);
				break;
			case qtk_mod_am13e2_DATA_LINEIN_courseware_TOLINEOUT:
				wtk_strbuf_push(lineincourseware, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_UAC_TOLINEOUT:
				wtk_strbuf_push(uacbuf, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_LINEIN_MIC_TOLINEOUT:
				wtk_strbuf_push(lineinmic, msg_node->buf->data, msg_node->buf->pos);
				break;
			case qtk_mod_am13e2_DATA_ARRAY_TOlINEOUT:
				wtk_strbuf_push(arraybuf, msg_node->buf->data, msg_node->buf->pos);
				// wtk_debug("---------->>>>>>>>>>>>>arraybuf->pos =%d\n",arraybuf->pos);
				break;
			default:
				break;
			}
			if(debug_count == 100)
			{
				wtk_debug("-use_lineout_out =%d ,use_linein_out =%d,uacbuf.pos =%d,arraymul.pos=%d,i2sbuf.pos=%d,lineincourseware.pos=%d,linein_cource=%d\n",m->use_lineout_out,m->use_linein_out,uacbuf->pos,arraybuf->pos,i2sbuf->pos,lineincourseware->pos,m->cfg->use_linein_courseware);
				debug_count = 0;
			}
				// wtk_debug("==========>>>>>pos = %d\n",pos);
			if(m->cfg->use_meetinglineout){
				if(m->use_linein_out && m->cfg->use_linein_courseware && uacbuf->pos ==0 && ! m->cfg->use_linein_courseware_touac){
					// wtk_debug("-------------------->>>>>>>>>>>>>>>>\n");
					if((m->use_linein_out && lineincourseware->pos >= 2*pos && i2sbuf->pos >= 2*pos && arraybuf->pos >= pos)) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0,j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+j, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(ttbuf, lineincourseware->data+j+2, 2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3ch_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3ch_2fn);
						qtk_gainnetbf_feed(m->gainnetbf3_3ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4_3ch, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->use_linein_out && m->cfg->use_linein_courseware && uacbuf->pos != 0 && ! m->cfg->use_linein_courseware_touac && i2sbuf->pos != 0){
					// wtk_debug("-------------------->>>>>>>>>>>>>>>>\n");
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}
					if(m->use_linein_out && lineincourseware->pos >= 2*pos && i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && uacbuf->pos >= 2*pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0,j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+j, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(ttbuf, lineincourseware->data+j+2, 2);
							wtk_strbuf_push(ttbuf, uacbuf->data+j+2, 2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3ch_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3ch_2fn);
						qtk_gainnetbf_feed(m->gainnetbf_4ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf2_4ch, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);
					}
				}else if(m->use_linein_out && m->cfg->use_linein_mic && uacbuf->pos ==0 && !m->cfg->use_linein_courseware){
					if(m->use_linein_out && lineinmic->pos >= pos && i2sbuf->pos >= pos && arraybuf->pos >= pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0,j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, lineinmic->data+i, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(ttbuf, lineinmic->data+i, 2);
							i+=2;
							j+=4;
						}
						qtk_gainnetbf_feed(m->gainnetbf3_3ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4_3ch, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(lineinmic, NULL, pos);
						wtk_strbuf_pop(i2sbuf, NULL, pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->use_linein_out && m->cfg->use_linein_mic && uacbuf->pos !=0 && !m->cfg->use_linein_courseware && i2sbuf->pos != 0){
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}
					if((lineinmic->pos >= pos && i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && uacbuf->pos >= 2*pos)) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0,j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, lineinmic->data+i, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(ttbuf, lineinmic->data+i, 2);
							wtk_strbuf_push(ttbuf, uacbuf->data+j, 2);
							i+=2;
							j+=4;
						}
						qtk_gainnetbf_feed(m->gainnetbf_4ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf2_4ch, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(lineinmic, NULL, pos);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);

					}
				}else if(!uacbuf->pos && i2sbuf->pos != 0){
				// wtk_debug("------------------__>>>>>>>>>>>>>>>>>>>>\n");
					if(lineinmic ->pos > pos || lineincourseware->pos > pos)
					{
						if(lineinmic->pos > pos)
							wtk_strbuf_pop(lineinmic, NULL, lineinmic->pos);
						if(lineincourseware->pos > pos)
							wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+j+2, 2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(uacbuf->pos!= 0 && i2sbuf->pos != 0){
				// wtk_debug("------------------__>>>>>>>>>>>>>>>>>>>>\n");
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && uacbuf->pos >= 2*pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf,arraybuf->data+i,2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(ttbuf, uacbuf->data+j+2, 2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf3_3ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf4_3ch, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}
			}else if(m->cfg->use_expandlineout)
			{
				// wtk_debug("----------------------__>>>>>>>>>>>>>>>>\n");
				if( m->use_linein_out && m->cfg->use_linein_courseware && !m->cfg->use_linein_mic && !m->cfg->use_linein_courseware_touac && !uacbuf->pos)
				{
					if(i2sbuf->pos >= 3*pos && lineincourseware->pos <= 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && lineincourseware->pos >= 2*pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+j, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+j+2, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
	 						wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf_4ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->use_linein_out && !m->cfg->use_linein_courseware && m->cfg->use_linein_mic && !uacbuf->pos)
				{
					if(i2sbuf->pos >= 3*pos && lineinmic->pos <= pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineinmic, NULL, lineinmic->pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && lineinmic->pos >= pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(tbuf, lineinmic->data+i, 2);
							wtk_strbuf_push(tbuf, lineinmic->data+i, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
	 						wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf_4ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(lineinmic, NULL, pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->use_linein_out && m->cfg->use_linein_courseware && !m->cfg->use_linein_mic && !m->cfg->use_linein_courseware_touac && uacbuf->pos != 0)
				{
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineincourseware, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}else if (i2sbuf->pos >= 3*pos && lineincourseware->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
						wtk_strbuf_pop(uacbuf, NULL, 3*pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+j, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+j+2, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j+2, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
	 						wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf_6ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);

						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(m->use_linein_out && !m->cfg->use_linein_courseware && m->cfg->use_linein_mic && uacbuf->pos != 0)
				{
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineinmic, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}else if(i2sbuf->pos >= 3*pos && lineinmic->pos < pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineinmic, NULL, lineinmic->pos);
						wtk_strbuf_pop(uacbuf, NULL, 3*pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && lineinmic->pos >= pos && uacbuf->pos >= 2*pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(tbuf, lineinmic->data+i, 2);
							wtk_strbuf_push(tbuf, lineinmic->data+i, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j+2, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
	 						wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf_6ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(lineinmic, NULL, pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);

						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(i2sbuf->pos != 0 && !uacbuf->pos){
				// wtk_debug("------------------__>>>>>>>>>>>>>>>>>>>>\n");
					if(lineinmic ->pos > pos || lineincourseware->pos > pos)
					{
						if(lineinmic->pos > pos)
							wtk_strbuf_pop(lineinmic, NULL, lineinmic->pos);
						if(lineincourseware->pos > pos)
							wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j+2, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
	 						wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}else if(uacbuf->pos!= 0 && i2sbuf->pos!= 0){
				// wtk_debug("------------------__>>>>>>>>>>>>>>>>>>>>\n");
					if(i2sbuf->pos >= 3*pos && uacbuf->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
					}
					if(i2sbuf->pos >= 2*pos && arraybuf->pos >= pos && uacbuf->pos >= 2*pos) {
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						int j=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+j, 2);
							wtk_strbuf_push(tbuf, i2sbuf->data+j+2, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j, 2);
							wtk_strbuf_push(tbuf, uacbuf->data+j+2, 2);

							wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
	 						wtk_strbuf_push(ttbuf,arraybuf->data+i,2);
							i+=2;
							j+=4;
						}
						// fwrite(tbuf->data,tbuf->pos,1,gainnet3_fn);
						// fwrite(ttbuf->data,ttbuf->pos,1,gainnet3_fn);
						qtk_gainnetbf_feed(m->gainnetbf_4ch, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(uacbuf, NULL, 2*pos);
						wtk_strbuf_pop(arraybuf, NULL, pos);
					}
				}
			}else if(m->use_linein_out  && m->cfg->use_linein_courseware && !i2sbuf->pos && !uacbuf->pos &&!m->cfg->use_linein_courseware_touac){
				// wtk_debug("iisbuf.pos =%d,lineincourseware->pos =%d,uacbuf->pos = %d\n",i2sbuf->pos,lineincourseware->pos,uacbuf->pos);
				if(lineincourseware->pos >= pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<pos){
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						wtk_strbuf_push(tbuf, lineincourseware->data+i+2, 2);
						i+=4;
					}
					// wtk_debug("----------------------_>>>>>>\n");
					qtk_mod_am13e2_player2_mode(m,tbuf->data,tbuf->pos);
					// wtk_debug("-------=========================>>>>>>>>>>>\n");
					wtk_strbuf_pop(lineincourseware, NULL, pos);
				}
			}
			else if(m->use_linein_out  && m->cfg->use_linein_courseware && i2sbuf->pos == 0 && uacbuf->pos != 0 &&!m->cfg->use_linein_courseware_touac){
				// wtk_debug("--------------------------->>>>.lineincourseware->pos =%d,uacbuf->pos = %d\n",lineincourseware->pos,uacbuf->pos);
				if(lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos) {
					// wtk_debug("--------------------->>>>>>>>>>>>>>>>\n");
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						i+=4;
					}
					// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
					qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
					wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
					wtk_strbuf_pop(uacbuf, NULL, 2*pos);
				}
			}
			else if (m->use_linein_out && m->cfg->use_linein_courseware && uacbuf->pos == 0 && i2sbuf->pos != 0 && !m->cfg->use_linein_courseware_touac){
				if(!m->cfg->use_onlylineinout){
					if(i2sbuf->pos >= 3*pos && lineincourseware->pos < 2*pos)
					{
						wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
						wtk_strbuf_pop(lineincourseware, NULL, lineincourseware->pos);
					}
					if(i2sbuf->pos >= 2*pos && lineincourseware->pos >= 2*pos){
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<2*pos){
							wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
							// wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
							wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
							i+=4;
						}
						qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
						qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);

						wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
						wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
					}
				}else{
					if(lineincourseware->pos >= pos){
						wtk_strbuf_reset(tbuf);
						wtk_strbuf_reset(ttbuf);
						int i=0;
						while(i<pos){
							wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
							wtk_strbuf_push(tbuf, lineincourseware->data+i+2, 2);
							i+=4;
						}
						// wtk_debug("----------------------_>>>>>>\n");
						qtk_mod_am13e2_player2_mode(m,tbuf->data,tbuf->pos);
						// wtk_debug("-------=========================>>>>>>>>>>>\n");
						wtk_strbuf_pop(lineincourseware, NULL, pos);
						wtk_strbuf_pop(i2sbuf, NULL, pos);
					}
				}
			}else if(m->use_linein_out && m->cfg->use_linein_courseware && i2sbuf->pos != 0 && uacbuf->pos != 0 &&!m->cfg->use_linein_courseware_touac){
				// wtk_debug("--------------------->>>>>>>>>>>>>>>>\n");
				// if((lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos) || (lineincourseware->pos > 2*pos)) {
				if(i2sbuf->pos >= 3*pos && lineincourseware->pos >= 2*pos && uacbuf->pos < 2*pos)
				{
					wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
					wtk_strbuf_pop(lineincourseware, NULL, 3*pos);
					wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
				}
				if(lineincourseware->pos >= 2*pos && uacbuf->pos >= 2*pos && i2sbuf->pos >= 2*pos) {
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);
						wtk_strbuf_push(tbuf, i2sbuf->data+i,2);
						wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
						wtk_strbuf_push(ttbuf, i2sbuf->data+i+2,2);
						i+=4;
					}
					// fwrite(tbuf->data,tbuf->pos,1,gainnet2_fn);
					qtk_gainnetbf_feed(m->gainnetbf3_3ch, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf4_3ch, ttbuf->data, ttbuf->pos, 0);
					wtk_strbuf_pop(lineincourseware, NULL, 2*pos);
					wtk_strbuf_pop(uacbuf, NULL, 2*pos);
					wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
				}
			}
			else if (uacbuf->pos == 0 && i2sbuf->pos != 0){
				if(lineincourseware->pos > 0){
					wtk_strbuf_pop(lineincourseware, NULL, 0);
				}
				// wtk_debug("---------->>>>>>>>>>>>>>i2sbuf->pos=%d,lineincourseware->pos=%d,m->cfg->use_linein_courseware=%d\n",i2sbuf->pos,lineincourseware->pos,m->cfg->use_linein_courseware);
				if(i2sbuf->pos >= 2*pos){
					wtk_strbuf_reset(tbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						wtk_strbuf_push(tbuf, i2sbuf->data+i+2, 2);
						i+=4;
					}
					// wtk_debug("----------------------_>>>>>>\n");
					//fwrite(tbuf->data,tbuf->pos,1,gainnet3ch_fn);
					qtk_mod_am13e2_player2_mode(m,tbuf->data,tbuf->pos);
					// wtk_debug("-------=========================>>>>>>>>>>>\n");
					wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
				}
			}
			else if (uacbuf->pos != 0 && i2sbuf->pos != 0){
				// wtk_debug("-------=========================>>>>>>>>>>>\n");
				if(i2sbuf->pos >= 2*pos && uacbuf->pos < 2*pos)
				{
					// wtk_strbuf_pop(i2sbuf, NULL, 3*pos);
					wtk_strbuf_pop(uacbuf, NULL, uacbuf->pos);
				}
				if(i2sbuf->pos >= 2*pos && uacbuf >= 2*pos){
					wtk_strbuf_reset(tbuf);
					wtk_strbuf_reset(ttbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, i2sbuf->data+i, 2);
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(ttbuf, i2sbuf->data+i+2, 2);
						wtk_strbuf_push(ttbuf, uacbuf->data+i+2, 2);
						i+=4;
					}
					qtk_gainnetbf_feed(m->gainnetbf3, tbuf->data, tbuf->pos, 0);
					qtk_gainnetbf_feed(m->gainnetbf5, ttbuf->data, ttbuf->pos, 0);
					wtk_strbuf_pop(i2sbuf, NULL, 2*pos);
					wtk_strbuf_pop(uacbuf, NULL, 2*pos);
				}
			}
			else if (i2sbuf->pos == 0 && uacbuf->pos != 0){
				// wtk_debug("------------------------------------->>>>>>>>>>>>>>.\n");
				if(uacbuf->pos >= 2*pos){
					wtk_strbuf_reset(tbuf);
					int i=0;
					while(i<2*pos){
						wtk_strbuf_push(tbuf, uacbuf->data+i, 2);
						wtk_strbuf_push(tbuf, uacbuf->data+i+2, 2);
						i+=4;
					}
					qtk_mod_am13e2_player3_mode(m,tbuf->data,tbuf->pos);

					// qtk_gainnetbf_feed(m->gainnetbf2, tbuf->data, tbuf->pos, 0);
					// wtk_debug("----------------------------------tbuf->pos =%d\n",tbuf->pos);
					wtk_strbuf_pop(uacbuf, NULL, 2*pos);
				}
			}
			// else if (m->cfg->use_rcd4 == 0 && m->cfg->use_linein_courseware==1){
			// 	// wtk_debug("------------------------------------->>>>>>>>>>>>>>.\n");
			// 	if(lineincourseware->pos >= pos){
			// 		wtk_strbuf_reset(tbuf);
			// 		wtk_strbuf_reset(ttbuf);
			// 		int i=0;
			// 		while(i<pos){
			// 			wtk_strbuf_push(tbuf, lineincourseware->data+i, 2);

			// 			wtk_strbuf_push(ttbuf, lineincourseware->data+i+2, 2);
			// 			i+=4;
			// 		}
			// 		qtk_mod_am13e2_player2_mode(m,tbuf->data,tbuf->pos);
			// 		wtk_strbuf_pop(lineincourseware, NULL, pos);
			
			// 	}	
			// }
		}
		qtk_msg_push_node(m->msg, msg_node);	
	}
	// fclose(gainnet3_fn);
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
	
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
	qtk_mod_am13e2_set_cpu(m, t, 3);
	qtk_msg_node_t *msg_node, *msg_node2;
	wtk_queue_node_t *qn;

	while(m->vbox_run){
		// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
		qn= wtk_blockqueue_pop(&m->vbox_queue,-1,NULL);
		// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		if(m->cfg->debug){
			if(m->vbox_queue.length > 10){
				// wtk_debug("--------------------->>>>>>>>>vbox_queue length=%d\n",m->vbox_queue.length);
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
	qtk_mod_am13e2_set_cpu(m, t, 3);
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
		// wtk_debug("----------------->>>>>>>>>>>>>>>>>time = %f\n",time_get_ms() - delay_time);
		qtk_vboxebf_feed(m->avboxebf, msg_node->buf->data, msg_node->buf->pos, 0);
		// wtk_debug("-------------------------->>>>>>>last_time-first_time = %f bufpos=%d queuelenth=%d\n",time_get_ms()-delay_time,msg_node->buf->pos,m->array_vbox_queue.length);
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
			// wtk_debug("---------------------->>>>>>>>>>>>>\n");
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
	int pos = 4096;
	float *play_vol = (float *)wtk_malloc(sizeof(float) * m->mic_check_play->cfg->nmicchannel);
    for (int i = 0; i < m->mic_check_play->cfg->nmicchannel; ++i) {
        play_vol[i] = 1.0;
    }
	while(m->mic_check_play_run){
		qn= wtk_blockqueue_pop(&m->mic_check_play_queue,-1,NULL);
		// wtk_debug("---------------============>>>>>>>>>m->array_vbox_queue.length= %d\n",m->array_vbox_queue.length);
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
		// wtk_debug("==================>>>>>>>>>>>>>>spk2buf.pos =%d,sp2buf.pos=%d\n",spk2_buf->pos,sp2buf->pos);
		if(spk2_buf->pos >= 2*pos && sp2buf->pos >= 2*pos)
		{
			wtk_strbuf_reset(sk2_2spbuf);
			int i=0;
			while(i<2*pos){
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
			// wtk_debug("---------------------->>>>>>>>>>>>>\n");
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
	// pos = 2048;
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
	
	double mergetm=0.0;

	while(m->merge_rcd_run){
		qn= wtk_blockqueue_pop(&m->merge_rcd_queue,-1,NULL);
		if(!qn) {continue;}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);
		// wtk_debug("----------------------->>>>>>>>use_line_in =%d\n",m->cfg->use_line_in);
		// wtk_debug("---->>>use_linein_out=%d,use_linein_mic=%d,use_linein_courseware=%d,use_linein_courseware_touac=%d\n",m->use_linein_out,m->cfg->use_linein_mic,m->cfg->use_linein_courseware,m->cfg->use_linein_courseware_touac);
		if(msg_node->type == 2){
			wtk_strbuf_push(rcd2buf, msg_node->buf->data, msg_node->buf->pos);
			#if 1
			wtk_strbuf_reset(linein_buf);
			int src_pos = 0;
			int channel = m->cfg->linein_channel; 
			while(src_pos < msg_node->buf->pos) {
				wtk_strbuf_push(linein_buf, msg_node->buf->data + src_pos + channel*2, 2);
				wtk_strbuf_push(linein_buf, msg_node->buf->data + src_pos + channel*2+2, 2);
				src_pos += channel2 * 2;
			}
			if(m->cfg->use_line_in && m->use_linein_out && msg_node->type == 2 && m->cfg->use_linein_mic==0){
				// wtk_debug("------------------_?>>>>>>>>>>>>>>>>>\n");
				if(m->cfg->use_linein_courseware && m->cfg->use_linein_courseware_touac){
					if(m->use_lineout_out && (m->cfg->use_meetinglineout || m->cfg->use_expandlineout)){
						if(linein_buf->pos > 0){
							if(m->cfg->in_inputtolineout_shift != 1.0){
								qtk_data_change_vol(linein_buf->data, linein_buf->pos, m->cfg->in_inputtolineout_shift);
							}
							linein_node = qtk_msg_pop_node(m->msg);
							linein_node->type = qtk_mod_am13e2_DATA_LINEIN_courseware_TOLINEOUT;
							wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
							wtk_blockqueue_push(&m->gainnet3_queue, &linein_node->qn);
						//  wtk_debug("--------------------_>>>>>>>>>>linein_buff->pos = %d\n",linein_buf->pos);
						}
					}	
					else if(linein_buf->pos > 0) {
					#if 1
						linein_node = qtk_msg_pop_node(m->msg);
						linein_node->type = qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE_TOUAC;
						wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
						wtk_blockqueue_push(&m->gainnet_queue, &linein_node->qn);
					#endif
					}
				}else if (m->cfg->use_linein_courseware && m->cfg->use_linein_courseware_touac == 0){
					if(m->use_lineout_out && (m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout)){	
						if(linein_buf->pos > 0){
							if(m->cfg->in_inputtolineout_shift != 1.0){
								qtk_data_change_vol(linein_buf->data, linein_buf->pos, m->cfg->in_inputtolineout_shift);
							}
							linein_node = qtk_msg_pop_node(m->msg);
							linein_node->type = qtk_mod_am13e2_DATA_LINEIN_courseware_TOLINEOUT;
							wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
							wtk_blockqueue_push(&m->gainnet3_queue, &linein_node->qn);
						//  wtk_debug("--------------------_>>>>>>>>>>linein_buff->pos = %d\n",linein_buf->pos);
						}
					}	
					else{
						if(linein_buf->pos > 0) {
							if(m->cfg->in_inputtospeaker_shift != 1.0){
								qtk_data_change_vol(linein_buf->data, linein_buf->pos, m->cfg->in_inputtospeaker_shift);
							}
							#if 1
								linein_node = qtk_msg_pop_node(m->msg);
								linein_node->type = qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE;
								wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
								wtk_blockqueue_push(&m->gainnet2_queue, &linein_node->qn);
							#endif
								// wtk_debug("----------------->>>>>>>>>>>>>>>linein_buf->pos=%d\n",linein_buf->pos);
						}
					}
				}
			}else if(m->cfg->use_line_in && m->use_linein_out && msg_node->type == 2 && m->cfg->use_linein_mic) {
			#if 1
				if(linein_buf->pos > 0) {
					linein_node = qtk_msg_pop_node(m->msg);
					wtk_strbuf_push(linein_node->buf, linein_buf->data, linein_buf->pos);
					wtk_blockqueue_push(&m->linein_queue, &linein_node->qn);
					// wtk_debug("--------------->>>>>>>m->linein_queue.length=%d\n",m->linein_queue.length);
				}
				// }
			#endif
			}
			#endif
		}
		if(msg_node->type == 1){
			mergetm = time_get_ms();
			// wtk_strbuf_push(rcdbuf, msg_node->buf->data, msg_node->buf->pos);
			i=0;
			while(i < msg_node->buf->pos){
				for(j=0;j<channel1;++j){
					wtk_strbuf_push(rcdbuf[j], msg_node->buf->data+i, 2);
					i+=2;
				}
			}
		}
		if(m->cfg->use_array == 1){
			wtk_strbuf_reset(abf3abuf);
			// wtk_debug("-------->>>>rcd_buf.pos =%d ,rcd2buf.pos=%d\n",rcdbuf[0]->pos,rcd2buf->pos);
			if(rcdbuf[0]->pos >= pos && rcd2buf->pos >= pos*channel2){
				i=0;
				j=0;
				while(i < pos){
					for(int ch=0; ch<8; ch++) {
						wtk_strbuf_push(abf3abuf, rcdbuf[ch]->data+i, 2);
					}

					// wtk_strbuf_push(abf3abuf, rcd2buf->data+j, 2);    // 通道0 低频通道
					// wtk_strbuf_push(abf3abuf, rcd2buf->data+j+4, 2);  // 通道2 line_out
					wtk_strbuf_push(abf3abuf, rcd2buf->data+j+14, 2); // 通道7 左右取一通道

					// i+=(channel1*2);
					i+=2;
					j+=(channel2*2);
				}
				// printf("  abf3abuf size after push: %d bytes\n", abf3abuf->pos);
				// msg_bf3anode = qtk_msg_pop_node(m->msg);
				// wtk_strbuf_push(msg_bf3anode->buf, sbf3abuf->data, sbf3abuf->pos);
				// wtk_blockqueue_push(&m->vbox_queue, &msg_bf3anode->qn);
				// merge_all += abf3abuf->pos/11.0/96.0;
				msg_array_node = qtk_msg_pop_node(m->msg);
				wtk_strbuf_push(msg_array_node->buf, abf3abuf->data, abf3abuf->pos);
				wtk_blockqueue_push(&m->array_vbox_queue, &msg_array_node->qn);
				// wtk_debug("------------------------------------->>>>>>>>>> usbaudio_queue-length=%d array_vbox_queue->length=%d\n",m->usbaudio_queue.length,m->array_vbox_queue.length);
				wtk_strbufs_pop(rcdbuf, channel1, pos);
				wtk_strbuf_pop(rcd2buf, NULL, pos*channel2);
				// wtk_strbuf_pop(linein_buf, NULL, pos*2);
				// wtk_strbuf_pop(sbf3abuf, NULL, pos*3);
				// wtk_strbuf_pop(abf3abuf, NULL, pos*11);

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
	qtk_mod_am13e2_set_cpu(m, t, 2);
	int lvalue1=0;
	int lvalue2=0;
	int ovalue=0;
	char *data1=NULL;
	char *data2=NULL;
	int dlen=0;
	int offcount1=0;
	int offcount2=0;
	int oncount1=0;
	int oncount2=0;
	int checkvalue=570;
	char *upresult;
	int ulen,s;
	char *pc;
	if(wtk_file_exist(m->cfg->linein_check_path.data) == 0){
		data1 = file_read_buf(m->cfg->linein_check_path.data, &dlen);
		ovalue = atoi(data1);
		if(ovalue < 100){
			wtk_debug("=========>>>>>linein off<<<<<<==========\n");
			m->use_linein_out=0;
			wtk_debug("----------------------->>>>>>>>>>>>>>>m->use_linein_out =%d\n",m->use_linein_out);
			wtk_debug("-------------------->>>>>>>>>>>>\n");
		}else{
			wtk_debug("=========>>>>>linein on<<<<<<==========\n");
			m->use_linein_out=1;
			wtk_debug("----------------------->>>>>>>>>>>>>>>m->use_linein_out =%d\n",m->use_linein_out);
		}
		wtk_free(data1);
		data1=NULL;
	}
	if(wtk_file_exist(m->cfg->lineout_check_path.data) == 0){
		wtk_debug("-------------------->>>>>>>>>>>>\n");
		data2 = file_read_buf(m->cfg->lineout_check_path.data, &dlen);
		wtk_debug("-------------------->>>>>>>>>>>>\n");
		ovalue = atoi(data2);
		wtk_debug("-------------------->>>>>>>>>>>>\n");
		if(ovalue < 100){
			wtk_debug("=========>>>>>lineout 0ff<<<<<<==========\n");
			m->use_lineout_out=0;
			lm->cfg->use_headset=0;
			lm->cfg->use_spkout = 1;
			lm->cfg->use_wooferout = 1;
			wtk_debug("-------->>>>>>>>>>>>>>m->use_headset=%d\n",m->cfg->use_headset);
			if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
				upresult=file_read_buf(RES_CFG_PATH,&ulen);
				pc = strstr(upresult, "use_headset=");
				s = pc - upresult;				
				upresult[s + 12] = '0';

				pc = strstr(upresult, "use_spkout=");
				s = pc - upresult;
				upresult[s + 11] = '1' ;

				pc = strstr(upresult, "use_wooferout=");
				s = pc - upresult;
				upresult[s + 14] = '1' ;
				qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
				wtk_free(upresult);
				
				// wtk_debug("================fwrite=%d\n",ret);
			}

			wtk_debug("----------------------->>>>>>>>>>>>>>>m->use_lineout_out =%d\n",m->use_lineout_out);
		}else {
			wtk_debug("=========>>>>>lineout on<<<<<<==========\n");
			m->use_lineout_out=1;
			lm->cfg->use_headset=1;
			lm->cfg->use_spkout = 0;
			lm->cfg->use_wooferout = 0;
			wtk_debug("-------->>>>>>>>>>>>>>m->use_headset=%d\n",m->cfg->use_headset);
			if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
				upresult=file_read_buf(RES_CFG_PATH,&ulen);
				pc = strstr(upresult, "use_headset=");
				s = pc - upresult;				
				upresult[s + 12] = '1';

				pc = strstr(upresult, "use_spkout=");
				s = pc - upresult;
				upresult[s + 11] = '0' ;

				pc = strstr(upresult, "use_wooferout=");
				s = pc - upresult;
				upresult[s + 14] = '0' ;

				qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
				wtk_free(upresult);
			}
			wtk_debug("----------------------->>>>>>>>>>>>>>>m->use_lineout_out =%d\n",m->use_lineout_out);
		}
		wtk_free(data2);
		data2=NULL;
	}
	while(m->linein_check_run){
		data1 = file_read_buf(m->cfg->linein_check_path.data, &dlen);
		data2 = file_read_buf(m->cfg->lineout_check_path.data, &dlen);
		lvalue1 = atoi(data1);
		lvalue2 = atoi(data2);
		if(lvalue1 < checkvalue){
			if(m->use_linein_out == 1){
				offcount1++;
			}
			if(offcount1 > 2){
				if(m->use_linein_out == 1){
					wtk_debug("=========>>>>>linein off<<<<<<==========\n");
					m->use_linein_out = 0;
					wtk_debug("----------------------->>>>>>>>>>>>>>>m->use_linein_out =%d\n",m->use_linein_out);
					// qtk_mod_am13e2_proc_write(m,"LINEIN_OFF",11);
					qtk_uart_client_send_audio_status_frame(m->uart, 0x01, 0x17, QTK_UART_SEND_STATE_LINEIN_OFF, 0);
				}
				offcount1 = 0;
			}
			oncount1=0;
		}else if(lvalue1 > checkvalue){
			if(m->use_linein_out == 0){
				oncount1++;
			}
			if(oncount1 > 2){
				if(m->use_linein_out == 0){
					wtk_debug("=========>>>>>linein on<<<<<<==========\n");
					m->use_linein_out = 1;
					wtk_debug("----------------------->>>>>>>>>>>>>>>m->use_linein_out =%d\n",m->use_linein_out);
					//qtk_mod_am13e2_proc_write(m,"LINEIN_ON",10);
					qtk_uart_client_send_audio_status_frame(m->uart, 0x01, 0x1B, QTK_UART_SEND_STATE_LINEIN_ON, 1);
				}
				oncount1 = 0;
			}
			offcount1=0;
		}
		if(lvalue2 < checkvalue){
			if(m->use_lineout_out == 1){
				offcount2++;
			}
			if(offcount2 > 2){
				if(m->use_lineout_out == 1){
					wtk_debug("=========>>>>>lineout off<<<<<<==========\n");
					// qtk_mod_am13e2_proc_write(m,"LINEOUT_OFF",12);
					qtk_uart_client_send_audio_status_frame(m->uart, 0x01, 0x17, QTK_UART_SEND_STATE_LINEOUT_OFF, 0);
				}
				m->use_lineout_out = 0;
				offcount2 = 0;
			}
			oncount2=0;
		}else if(lvalue2 > checkvalue){
			if(m->use_lineout_out == 0){
				oncount2++;
			}
			if(oncount2 > 2){
				if(m->use_lineout_out == 0){
					wtk_debug("=========>>>>>linein on<<<<<<==========\n");
					// qtk_mod_am13e2_proc_write(m,"LINEOUT_ON",11);
					qtk_uart_client_send_audio_status_frame(m->uart, 0x01, 0x1B, QTK_UART_SEND_STATE_LINEOUT_ON, 1);
				}
				m->use_lineout_out = 1;
				oncount2 = 0;
			}
			offcount2=0;
		}
		wtk_free(data1);
		data1=NULL;
		wtk_free(data2);
		data2=NULL;
		usleep(200*1000);
	}
	return 0 ;
}

int qtk_mod_am13e2_rcd3_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_mod_am13e2_set_cpu(m, t, 2);
	wtk_strbuf_t *rbuf;
	qtk_msg_node_t *msg_node;
	int is_first=1;

	wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry\n");
	wtk_log_log0(m->log,"------- recorde3 start");
	while(m->rcd3_run){
		#if 0
		continue;
		#endif
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
		rcd3_total_frames += (rbuf->pos / (2 * 1));
		rcd3_elapsed = (rcd3_total_frames * 1000.0) / 48000;
		if((m->cfg->use_log_wav && m->iismul) || m->log_audio){
			wtk_wavfile_write(m->iismul, rbuf->data, rbuf->pos);
		}
		// wtk_debug("-------------------------->>>>>>>>>>>>>>>>>>>>>>>\n");
		static double tm4= 0.0;
		// wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>rcd tm4 = %f\n",time_get_ms()-tm4);
		if(m->use_lineout_out){
			if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout){
				// wtk_debug("------------------>>>>>>>\n");
				msg_node = qtk_msg_pop_node(m->msg);
				msg_node->type = qtk_mod_am13e2_DATA_IIS_TOLINEOUT;
				wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
				wtk_blockqueue_push(&m->gainnet3_queue, &msg_node->qn);
			}
		}
		if(!m->cfg->use_onlylineout)
		{
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_am13e2_DATA_IIS_TOSPK;
			wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
			wtk_blockqueue_push(&m->gainnet2_queue, &msg_node->qn);
		}
		tm4 = time_get_ms();
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
	qtk_mod_am13e2_set_cpu(m, t, 2);
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
		// wtk_debug("---------->>>>>>>>>>>>>>\n");
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

		if(m->cfg->uac_shift != 1.0){
			qtk_data_change_vol(rbuf->data, rbuf->pos, m->cfg->uac_shift);
		}
		if((m->cfg->use_log_wav && m->uacmul) || m->log_audio){
			wtk_wavfile_write(m->uacmul, rbuf->data, rbuf->pos);
		}
		static double tm5= 0.0;
		// wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>rcd tm5 = %f\n",time_get_ms()-tm5);
		if(m->is_use_uac)
		{
			// wtk_debug("------------>>>>>>>>>>>>>>\n");
			#if 1
			msg_node = qtk_msg_pop_node(m->msg);
			// wtk_debug("---------------------------->>>>>>.rbuf->pos= %d\n",rbuf->pos);
			msg_node->type = qtk_mod_am13e2_DATA_UAC_TOSPK;
			// wtk_debug("-----------use_linein_courseware=%d--m->gainnetbf2_run = %d--uacrbuf.pos-= %d\n",m->cfg->use_linein_courseware,m->gainnet2_run,rbuf->pos);
			wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
			wtk_blockqueue_push(&m->gainnet2_queue, &msg_node->qn);
			#endif
			if(m->use_lineout_out){
				if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout){
					msg_node = qtk_msg_pop_node(m->msg);
					msg_node->type = qtk_mod_am13e2_DATA_UAC_TOLINEOUT;
					wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
					wtk_blockqueue_push(&m->gainnet3_queue, &msg_node->qn);
				}
			}
		}
		tm5 = time_get_ms();
	}
	// fclose(uac_fn);
	wtdebugTime();
	wtk_debug("=======================>>>>>>>>>>>>>>>>>>>>>\n");
	wtk_log_log0(m->log,"------- recorde4UAC end");
	return 0;
}
int qtk_mod_am13e2_rcd2_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_mod_am13e2_set_cpu(m, t, 0);
	wtk_strbuf_t *rbuf;
	wtk_strbuf_t *spk_check_buf=NULL;
	qtk_msg_node_t *msg_node;
	qtk_msg_node_t *spk_check_node;
	int is_first=1;
	spk_check_buf = wtk_strbuf_new(m->cfg->rcd.buf_time*32*2, 1.0f);

	wtk_debug("rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr entry\n");
	wtk_log_log0(m->log,"------- recorde2 start");
	while(m->rcd2_run){
		#if 0
		continue;
		#endif
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

#ifndef WUTAOCESHI
		if((m->cfg->use_log_wav && m->arraymul) || m->log_audio){
			wtk_wavfile_write(m->arraymul, rbuf->data, rbuf->pos);
		}
#else
		if((m->cfg->use_log_wav && m->arraymul) || m->log_audio){
			int pos =0;
			while(pos < rbuf->pos)
			{
				wtk_wavfile_write(m->arraymul, rbuf->data + pos + 7*2, 2);
				pos += 8*2;
			}
		}
#endif
		#if 1
		wtk_strbuf_reset(spk_check_buf);
		int src_pos = 0;
		while(src_pos < rbuf->pos) {
			wtk_strbuf_push(spk_check_buf,  rbuf->data+ src_pos + 6*2, 2);
			wtk_strbuf_push(spk_check_buf,  rbuf->data+ src_pos + 6*2+2, 2);
			src_pos += 8 * 2;
		}
		#endif
		#if 1
		spk_check_node = qtk_msg_pop_node(m->msg);
		spk_check_node->type = qtk_mod_am13e2_sp2;
		wtk_strbuf_push(spk_check_node->buf, spk_check_buf->data, spk_check_buf->pos);
		wtk_blockqueue_push(&m->mic_check_play_queue, &spk_check_node->qn);
		// wtk_debug("-------------->>>>>>>>>>>mic_chek_playqueue =%d\n",m->mic_check_play_queue.length);
		#endif
		static double tm_rcd2= 0.0;
		// wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>rcd tm3 = %f\n",time_get_ms()-tm_rcd2);
		
		// wtk_debug("rrrrrrrrrrrrr222222222222222222=pos=%d\n",rbuf->pos);
	#if 1
		msg_node = qtk_msg_pop_node(m->msg);
		msg_node->type = 2;
		wtk_strbuf_push(msg_node->buf, rbuf->data, rbuf->pos);
		wtk_blockqueue_push(&m->merge_rcd_queue, &msg_node->qn);
		wtk_strbuf_pop(spk_check_buf,NULL,spk_check_buf->pos);
	#endif
		tm_rcd2 = time_get_ms();
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
	qtk_mod_am13e2_set_cpu(m, t, 3);
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
		#if 0
		continue;
		#endif
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
			
		#ifndef WUTAOCESHI
			if((m->cfg->use_log_wav && m->mul) || m->log_audio){
				wtk_wavfile_write(m->mul, tmpbuf->data, tmpbuf->pos);
			}
		#else
			if((m->cfg->use_log_wav && m->mul) || m->log_audio){
				int i,pos=0;
				while(pos < tmpbuf->pos){
					wtk_wavfile_write(m->mul, tmpbuf->data + pos +7*12, 2);
					pos += 8*2;
				}
			}
		#endif

			#if 1
			msg_node = qtk_msg_pop_node(m->msg);
			wtk_strbuf_push(msg_node->buf, tmpbuf->data, tmpbuf->pos);
			wtk_blockqueue_push(&m->mic_check_rcd_queue, &msg_node->qn);
			#endif
			//音量值获取
			continuous_get_volume(m,tmpbuf->data, tmpbuf->pos);
			static double tm3= 0.0;
			// wtk_debug("==========================>>>>>>>>>>>>>>>>>>>>rcd tm3 = %f\n",time_get_ms()-tm3);
			if(m->is_mic){
				#if 1
				msg_node = qtk_msg_pop_node(m->msg);
				msg_node->type = 1;
				wtk_strbuf_push(msg_node->buf, tmpbuf->data, tmpbuf->pos);
				wtk_blockqueue_push(&m->merge_rcd_queue, &msg_node->qn);
				#endif
				// wtk_debug("-----------------------------tmpbuf->pos=%d\n",tmpbuf->pos);
			}
			tm3=time_get_ms();
			// wtk_debug("merge_rcd_queue.length =%d,gainnet_queue.length=%d,gainnet2_queue.length=%d,gainnet3_queue.length=%d,denoise_vbox_queue.length=%d\n",m->merge_rcd_queue.length,m->gainnet_queue,m->gainnet2_queue.length,m->gainnet3_queue.length,m->denoise_vbox_queue.length);
			// wtk_debug("array_queue.length=%d,usbaudio_queue.length=%d,lineout_queue.length=%d,linein_queue.length=%d\n",m->array_vbox_queue.length,m->usbaudio_queue.length,m->lineout_queue.length,m->linein_queue.length);
	

	}
	wtk_log_log0(m->log,"------- recorde end");
	wtk_strbuf_delete(tmpbuf);
	return 0;
}
int qtk_mod_am13e2_linein_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_mod_am13e2_set_cpu(m, t, 1);
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
	wtk_strbuf_t *mic_buf;
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
				// wtk_debug("-------------------_>>>>>>>>>>>>\n");
				// qtk_vboxebf_feed(m->denoisebf, msg_node->buf->data, msg_node->buf->pos, 0);
			}
			// wtk_debug("-----------_>>>>>>>>>>mic->pos=%d\n",mic_buf->pos);
			// wtk_strbuf_pop(mic_buf, NULL, msg_node->buf->pos);
		}
		qtk_msg_push_node(m->msg, msg_node);
	}
	qtk_vboxebf_feed(m->denoisebf, NULL, 0, 1);
	wtk_strbuf_delete(mic_buf);
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>..\n");
	qtk_vboxebf_reset(m->denoisebf);
	return 0;
}
#ifndef OFFLINE_TEST
int  qtk_mod_am13e2_usbaudio_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
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
		debug_count++;
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
			// if(m->cfg->echo_shift != 1.0){
			// 	qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, m->cfg->echo_shift);
			// }
			if(lm->cfg->volum_input_mute)
				lm->cfg->mic_shift = 0.0;
			if(lm->cfg->mic_shift != 1.0){
				qtk_data_change_vol(msg_node->buf->data,  msg_node->buf->pos, lm->cfg->mic_shift);
				process_audio_with_limiter(msg_node->buf->data, msg_node->buf->pos);
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
	qtk_mod_am13e2_set_cpu(m, t, 2);
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
		
	
		if(lm->cfg->volum_output_mute)
		{
			lm->cfg->echo_shift = 0.0;
		}
		if(lm->cfg->echo_shift != 1.0){
			qtk_data_change_vol(msg_node->buf->data, msg_node->buf->pos, lm->cfg->echo_shift);
		}
	    // wtk_debug("-------------->>>>>lineout.pos=%d\n",msg_node->buf->pos);
	#if 1
		if(m->cfg->use_log_wav && m->lineoutml)
		{
			wtk_wavfile_write(m->lineoutml, msg_node->buf->data, msg_node->buf->pos);
		}
	#endif
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
int qtk_mod_am13e2_spk_entry(qtk_mod_am13e2_t *m, wtk_thread_t *t)
{
	qtk_mod_am13e2_set_cpu(m, t, 2);
	qtk_msg_node_t *msg_node=NULL,*msg_node2=NULL;
	wtk_queue_node_t *qn;
	int first = 1;
	long ret;
	wtk_blockqueue_t *spk_queue;
	int zlen=m->cfg->sil_time*m->cfg->lineout.channel*m->cfg->lineout.sample_rate/1000*2;
	int ucnt;
	double tm,tm2;

	
	char *zerodata = wtk_malloc(zlen);
	memset(zerodata, 0, zlen);

	spk_queue=&(m->spk_queue);

	wtk_debug("-----------> speaker start entry zlen=%d\n",zlen);
	wtk_log_log0(m->log,"-----------> speaker start entry");

	m->play_on=1;

  	while(m->speaker_run){
		// wtk_debug("===================>>>>>>>>>>>>>>\n");
		qn= wtk_blockqueue_pop(spk_queue,-1,NULL);
		// wtk_debug("===================>>>>>>>>>>>>>>\n");
		if(!qn) {
			continue;
			// msg_node = NULL;
			// goto loopcontinue;
		}
		msg_node = data_offset2(qn,qtk_msg_node_t,qn);

		if(first){
			wtk_debug("===================>>>>>>>>>>>>>>>>>>>>>>>>>player time=%f\n",time_get_ms());
			ret = qtk_play_write(m->speaker, zerodata, zlen, 1);
			// qtk_play2_write(m->lineout,zerodata,zlen);    ///use_liwei
			wtk_debug("===================>>>>>>>>>>>>>>\n");
			if(ret){
				wtk_debug("play zero buf %ld\n",ret);
				wtk_log_log(m->log,"play zero buf %ld",ret);
			}
			first=0;
		}
	#if 1
		if((m->cfg->use_log_wav && m->spk_mul) || m->log_audio){
			wtk_wavfile_write(m->spk_mul, msg_node->buf->data, msg_node->buf->pos);
		}
	#endif
		ret = qtk_play_write(m->speaker, msg_node->buf->data, msg_node->buf->pos, 1);
		if(ret < 0){
			wtk_debug("=================>>>>>>>>>>>>write err=%ld\n",ret);
			wtk_log_log(m->log,"=================>>>>>>>>>>>>write err=%d",ret);
		}
		// qtk_play2_write(m->lineout, msg_node->buf->data, msg_nsode->buf->pos); ///use_liwei
		qtk_msg_push_node(m->msg, msg_node);
	}

	if(first==0){
		qtk_play_stop(m->speaker);
	}

	if(zerodata){
		wtk_free(zerodata);
	}
  	return 0;
}
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
	#if 1
		qtk_msg_node_t *msg_node;
		msg_node = qtk_msg_pop_node(m->msg);
		if(m->cfg->use_meetinglineout && m->use_lineout_out){
			msg_node->type = qtk_mod_am13e2_DATA_LINEIN_MIC_TOLINEOUT;
		}else{		
			msg_node->type = qtk_mod_am13e2_DATA_LINEIN_MIC_TOUAC;
		}
		wtk_strbuf_push(msg_node->buf, data, len);
		if(m->cfg->use_meetinglineout && m->use_lineout_out){
			wtk_blockqueue_push(&m->gainnet3_queue, &msg_node->qn);
		}else{
			wtk_blockqueue_push(&m->gainnet_queue, &msg_node->qn);
		}
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
void qtk_mod_am13e2_on_array_vboxebf(qtk_mod_am13e2_t *m, char *data, int len)
{
	static double tm=0.0;
	// wtk_debug("=======================>>>>>>>len=%d tm=%f\n",len,time_get_ms() - tm);
	if(m->cfg->gainnetbf_cfg){
		// wtk_debug("----------------->>>>>>>>>>>use_meetinglineout=%d,use_expandlineout=%d\n",m->cfg->use_meetinglineout,m->cfg->use_expandlineout);
		if(m->use_lineout_out){
			if(m->cfg->use_meetinglineout || m->cfg->use_expandlineout){
				qtk_msg_node_t *meetinglineout;
				meetinglineout = qtk_msg_pop_node(m->msg);
				meetinglineout->type = qtk_mod_am13e2_DATA_ARRAY_TOlINEOUT;
				wtk_strbuf_push(meetinglineout->buf, data, len);
				// wtk_debug("---------len = %d\n",len);
				wtk_blockqueue_push(&m->gainnet3_queue, &meetinglineout->qn);
			}
		}
		#if 1
			qtk_msg_node_t *msg_node;
			msg_node = qtk_msg_pop_node(m->msg);
			msg_node->type = qtk_mod_am13e2_DATA_ARRAY;
			wtk_strbuf_push(msg_node->buf, data, len);
			// wtk_debug("---------len = %d\n",len);
			wtk_blockqueue_push(&m->gainnet_queue, &msg_node->qn);
		#endif
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
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>>\n");
	if(m->cfg->use_out_resample){
		    wtk_debug("--------------------resample\n");
			wtk_resample_feed(m->outresample, data, len, 0);
	}else{
		qtk_mod_am13e2_player_mode(m, data, len);
	}
}
void qtk_mod_am13e2_on_gainnetbf2(qtk_mod_am13e2_t *m, char *data, int len) //speaker
{	
	// wtk_debug("==================>>>>>>>>>>>>\n");
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>>\n");
	wtk_strbuf_push(m->speaker_left_audiobuf, data, len);
}
void qtk_mod_am13e2_on_gainnetbf3(qtk_mod_am13e2_t *m, char *data, int len) //lineout
{
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>>\n");
	wtk_strbuf_push(m->lineout_left_audiobuf, data, len);
}
void qtk_mod_am13e2_on_gainnetbf4(qtk_mod_am13e2_t *m, char *data, int len) //speaker
{
	// wtk_debug("--------------->>>>>>>>>>>>>>>\n");
	// wtk_debug("--------------------->>>>>>>>>>>>>>>>>>>\n");
	int i=0;
	wtk_strbuf_reset(m->speaker_all_audiobuf);
	while(i<len){
		wtk_strbuf_push(m->speaker_all_audiobuf,m->speaker_left_audiobuf->data+i,2);
		wtk_strbuf_push(m->speaker_all_audiobuf,data+i,2);
		i+=2;
	}
	// wtk_debug("-------------------____>>>>>>>>>>>>\n");
	qtk_mod_am13e2_player3_mode(m, m->speaker_all_audiobuf->data, m->speaker_all_audiobuf->pos); 
	wtk_strbuf_pop(m->speaker_left_audiobuf, NULL, len);

}
void qtk_mod_am13e2_on_gainnetbf5(qtk_mod_am13e2_t *m, char *data, int len) //lineout
{
	// wtk_debug("-------------------_>>>>>>>>>>>>>");
	int i=0;
	wtk_strbuf_reset(m->lineout_all_audiobuf);
	while(i<len){
		wtk_strbuf_push(m->lineout_all_audiobuf,m->lineout_left_audiobuf->data+i,2);
		wtk_strbuf_push(m->lineout_all_audiobuf,data+i,2);
		i+=2;
	}
	if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout)
		qtk_mod_am13e2_player2_mode(m, m->lineout_all_audiobuf->data, m->lineout_all_audiobuf->pos); 
	wtk_strbuf_pop(m->lineout_left_audiobuf, NULL, len);

}
void qtk_mod_am13e2_on_gainnetbf_3ch(qtk_mod_am13e2_t *m, char *data, int len) //speaker_3ch
{
	wtk_strbuf_push(m->speaker_left_audiobuf, data, len);
}
void qtk_mod_am13e2_on_gainnetbf2_3ch(qtk_mod_am13e2_t *m, char *data, int len)  //speaker_3ch
{
	int i=0;
	wtk_strbuf_reset(m->speaker_all_audiobuf);
	while(i<len){
		wtk_strbuf_push(m->speaker_all_audiobuf,m->speaker_left_audiobuf->data+i,2);
		wtk_strbuf_push(m->speaker_all_audiobuf,data+i,2);
		i+=2;
	}
	// wtk_debug("-------------------____>>>>>>>>>>>>\n");
	qtk_mod_am13e2_player3_mode(m, m->speaker_all_audiobuf->data, m->speaker_all_audiobuf->pos); 
	wtk_strbuf_pop(m->speaker_left_audiobuf, NULL, len);
}
void qtk_mod_am13e2_on_gainnetbf3_3ch(qtk_mod_am13e2_t *m, char *data, int len) //lineout_3ch
{
	wtk_strbuf_push(m->lineout_left_audiobuf, data, len);
}
void qtk_mod_am13e2_on_gainnetbf4_3ch(qtk_mod_am13e2_t *m, char *data, int len) //lineout_3ch
{
	int i=0;
	wtk_strbuf_reset(m->lineout_all_audiobuf);
	while(i<len){
		wtk_strbuf_push(m->lineout_all_audiobuf,m->lineout_left_audiobuf->data+i,2);
		wtk_strbuf_push(m->lineout_all_audiobuf,data+i,2);
		i+=2;
	}
	if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout)
		qtk_mod_am13e2_player2_mode(m, m->lineout_all_audiobuf->data, m->lineout_all_audiobuf->pos); 
	wtk_strbuf_pop(m->lineout_left_audiobuf, NULL, len);
}
void qtk_mod_am13e2_on_gainnetbf_4ch(qtk_mod_am13e2_t *m, char *data, int len) //lineout_4ch
{
	wtk_strbuf_push(m->lineout_left_audiobuf, data, len);
}
void qtk_mod_am13e2_on_gainnetbf2_4ch(qtk_mod_am13e2_t *m, char *data, int len) //lineout_4ch
{
	int i=0;
	wtk_strbuf_reset(m->lineout_all_audiobuf);
	while(i<len){
		wtk_strbuf_push(m->lineout_all_audiobuf,m->lineout_left_audiobuf->data+i,2);
		wtk_strbuf_push(m->lineout_all_audiobuf,data+i,2);
		i+=2;
	}
	if(m->cfg->use_mainlineout || m->cfg->use_wooflineout || m->cfg->use_meetinglineout || m->cfg->use_expandlineout)
		qtk_mod_am13e2_player2_mode(m, m->lineout_all_audiobuf->data, m->lineout_all_audiobuf->pos); 
	wtk_strbuf_pop(m->lineout_left_audiobuf, NULL, len);
}
void qtk_mod_am13e2_on_gainnetbf_6ch(qtk_mod_am13e2_t *m, char *data, int len) //lineout_6ch
{
	wtk_strbuf_push(m->lineout_left_audiobuf, data, len);
}

void qtk_mod_uac_volume_callback(void *user_data,int type,int value)
{
    qtk_mod_am13e2_t *m = (qtk_mod_am13e2_t *)user_data;
    wtk_debug("=================----------->>>>>>>>>>>=========   %d\n",value);
    float set,rett;
	int ret;
	int readbuf_len;
	char buf2[4096] ={0};
	char *pl;
	char *file_readbuf;
    switch (type)
    {
    case VOLUME_CHANGED:
        m->current_volume = value;
        wtk_debug("===========================>>>>>>>>%d%%",value);
        // 确保音量值在有效范围内(0-100)
        int volume_percentage = value;
        if (volume_percentage <= 1) volume_percentage = 0;
        if (volume_percentage >= 99) volume_percentage = 100;

        // set_volume_level(value);
		set = 1 * (volume_percentage / 100.0) ;
		m->cfg->uac_shift=set;
		uac_volume = set;
	#if 1
		file_readbuf=file_read_buf(RES_CFG_PATH,&readbuf_len);
				
		pl = strstr(file_readbuf, "uac_shift=");
		if(pl)
		{
			sprintf(buf2,"uac_shift=%0.2f;",set);
			memcpy(pl,buf2,strlen(buf2));
			ret = qtk_mod_atomic_write(RES_CFG_PATH,file_readbuf,readbuf_len);

		}
		wtk_free(file_readbuf);
	#endif
        break;
    case EUSB_PLAY_MUTE:
		if(value == 1)
		{
			m->cfg->uac_shift = 0.0;
		}else
		{
		#if 1
			file_readbuf=file_read_buf(RES_CFG_PATH,&readbuf_len);
			pl = strstr(file_readbuf, "uac_shift=");
			set = wtk_str_atof(pl + 11, ret - (pl - file_readbuf) - 12);
			m->cfg->uac_shift = set;
		#endif
			m->cfg->uac_shift = uac_volume;
		#if 1
			wtk_free(file_readbuf);
		#endif
		}
		break;
	default:
        break;
    }
}
void qtk_mod_usb_callback(void *user_data,int type)
{
	qtk_mod_am13e2_t *m = (qtk_mod_am13e2_t *)user_data;
    float set,rett;
	int ret;
	int readbuf_len;
	char buf2[4096] ={0};
	char *pl;
	char *file_readbuf;
	switch (type)
    {
	case DISCONNECTED_E:
		wtk_debug("-------------------------------DISCONNECTED_E--------------------->>>>>>>>>>>>>\n");
		wtdebugTime();	
		wtk_debug("========================================.>>>>>>>>>>>>>>>>>>>>>>>>>>>");
		usb_stata = 0;
		wtk_debug("-------------_>>>>>>>>>>>>>>>DISCONNECTED_E\n");
		disconnect_time = time_get_ms();
		
		break;
	case CONFIGURED_E:
		break;
	default:
        break;
    }
}
void qtk_mod_am13e2_on_mic_check_rcd(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int nchn)
{
	char set_buf[128]={0};
	// wtk_debug("------------------------------__>>>>>>>>>>>>>>>>>>>>\n");
	int i,mic_check_rcd_result=0,ret=0;
    for (i = 0; i < nchn; ++i) {
        // printf("%d:%d\n", i, type[i]);
		if(type[i] != 0)
		{
			mic_check_rcd_result = type[i];
		}
    }
	m->mic_check_result = mic_check_rcd_result;
	// FILE *file = fopen("/oem/qdreamer/qsound/miccheck_result.txt", "w");
	// if (file != NULL) {
	// 	fprintf(file, "%d\n", m->mic_check_result);
	// 	// wtk_debug("---------------------mic_check_result = %d\n", mic_check_result);
	// 	fflush(file);
	// 	{
	// 	int fd=fileno(file);
	// 	if(fd>=0){fsync(fd);}
	// 	}
	// 	fclose(file);
	// } else {
	// 	printf("无法打开文件进行写入。\n");
	// }
}
void qtk_mod_am13e2_on_mic_check_play(qtk_mod_am13e2_t *m, wtk_mic_check_err_type_t *type, int channenl)
{
	char set_buf[128]={0};
	// wtk_debug("------------------------------__>>>>>>>>>>>>>>>>>>>>\n");
	int i,mic_check_play_result=0,ret=0;
    for (i = 0; i < channenl; ++i) {
        // printf("%d:%d\n", i, type[i]);
		if(type[i] != 0)
		{
			mic_check_play_result = type[i];
		}
    }
	m->speak_check_result = mic_check_play_result;

	// FILE *file = fopen("/oem/qdreamer/qsound/spkcheck_result.txt", "w");
	// if (file != NULL) {
	// 	fprintf(file, "%d\n", m->speak_check_result);
	// 	// wtk_debug("---------------------spkcheck_result = %d\n", speak_check_result);
	// 	fflush(file);
	// 	{
	// 	int fd=fileno(file);
	// 	if(fd>=0){fsync(fd);}
	// 	}
	// 	fclose(file);
	// } else {
	// 	printf("无法打开文件进行写入。\n");
	// }
}
void qtk_mod_am13e2_on_outresample(qtk_mod_am13e2_t *m, char *data, int len)
{
	// wtk_debug("----------------------------->>>>>>>>>>>>>>>\n");
	qtk_mod_am13e2_player_mode(m, data, len);
}
void qtk_mod_am13e2_log_wav_file_new(qtk_mod_am13e2_t *m)
{
	int channel = m->mic_channel;
	int bytes_per_sample = 2;
	// int hh25c_a_sample_rate = m->cfg->rcd.sample_rate;
	
	// wtk_debug("============>>>>>>>sample_rate=%d channel=%d bytes_per_sample=%d\n",m->cfg->rcd.sample_rate,channel,bytes_per_sample);
#if 0  //lineout
	channel =2;
	m->lineoutml = wtk_wavfile_new(m->cfg->lineout.sample_rate);
	m->lineoutml->max_pend = 0;
	wtk_wavfile_set_channel2(m->lineoutml,m->cfg->lineout.channel,bytes_per_sample);
	wtk_wavfile_open(m->lineoutml, m->lineout_path->data);
#endif
#if 0  //spk
	channel =4;
	m->spk_mul = wtk_wavfile_new(m->cfg->speaker.sample_rate);
	m->spk_mul->max_pend = 0;
	wtk_wavfile_set_channel2(m->spk_mul,m->cfg->speaker.channel,bytes_per_sample);
	wtk_wavfile_open(m->spk_mul, m->spk_path->data);
#endif
#if 0  //iis
	m->iismul = wtk_wavfile_new(m->cfg->rcd3.sample_rate); 
	m->iismul->max_pend = 0;
	channel = m->cfg->rcd3.channel - m->cfg->rcd3.nskip;
	wtk_wavfile_set_channel2(m->iismul,channel,bytes_per_sample);
	wtk_wavfile_open(m->iismul, m->iis_path->data);
#endif
#if 0  //uac
	m->uacmul = wtk_wavfile_new(m->cfg->rcd3.sample_rate); 
	m->uacmul->max_pend = 0;
	channel = m->cfg->rcd4.channel - m->cfg->rcd4.nskip;
	wtk_wavfile_set_channel2(m->uacmul,channel,bytes_per_sample);
	wtk_wavfile_open(m->uacmul, m->uac_path->data);
#endif
#if 1
	#ifndef WUTAOCESHI
		m->mul = wtk_wavfile_new(m->cfg->rcd.sample_rate); 
		m->mul->max_pend = 0;
		channel = 8;
		wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);
		wtk_wavfile_open(m->mul, m->mul_path->data);
	#else
		m->mul = wtk_wavfile_new(m->cfg->rcd.sample_rate); 
		m->mul->max_pend = 0;
		channel = 1;
		wtk_wavfile_set_channel2(m->mul,channel,bytes_per_sample);
		wtk_wavfile_open(m->mul, m->mul_path->data);
	#endif
#endif
#if 1
	#ifndef WUTAOCESHI
		m->arraymul = wtk_wavfile_new(m->cfg->rcd2.sample_rate); 
		m->arraymul->max_pend = 0;
		channel = m->cfg->rcd2.channel - m->cfg->rcd2.nskip;
		wtk_wavfile_set_channel2(m->arraymul,channel,bytes_per_sample);
		wtk_wavfile_open(m->arraymul, m->arraymul_path->data);
	#else
		m->arraymul = wtk_wavfile_new(m->cfg->rcd2.sample_rate); 
		m->arraymul->max_pend = 0;
		channel = 1;
		wtk_wavfile_set_channel2(m->arraymul,channel,bytes_per_sample);
		wtk_wavfile_open(m->arraymul, m->arraymul_path->data);
	#endif
#endif
#if 1
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
#if 1
	wtk_wavfile_close(m->mul);
	wtk_wavfile_delete(m->mul);
	m->mul = NULL;
	wtk_wavfile_close(m->arraymul);
	wtk_wavfile_delete(m->arraymul);
	m->arraymul = NULL;
#endif

#if 1
	wtk_wavfile_close(m->iismul);
	wtk_wavfile_delete(m->iismul);
	m->iismul = NULL;
	
	wtk_wavfile_close(m->uacmul);
	wtk_wavfile_delete(m->uacmul);
	m->uacmul = NULL;

#endif
#if 1
	wtk_wavfile_close(m->spk_mul);
	wtk_wavfile_delete(m->spk_mul);
	m->spk_mul = NULL;
#endif
#if 1
	wtk_wavfile_close(m->lineoutml);
	wtk_wavfile_delete(m->lineoutml);
	m->lineoutml = NULL;
#endif
// 	wtk_wavfile_close(m->jlmul);
// 	wtk_wavfile_delete(m->jlmul);
// 	m->jlmul = NULL;

	wtk_wavfile_close(m->playwav);
	wtk_wavfile_delete(m->playwav);
	m->playwav = NULL;
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

void qtk_mod_am13e2_write_file(char *path, char *key, int keylen, int is_on)
{
	if (access(path, F_OK) == 0 && access(path, W_OK) == 0)
	{
		int ulen,s;
		char * upresult=file_read_buf(path,&ulen);
		char *pc;
		
		pc = strstr(upresult, key);
		s = pc - upresult;
		if (is_on == 0x00){
			upresult[s + keylen] = '0';
		}else if (is_on == 0x01){
			upresult[s + keylen] = '1';
		}
		qtk_mod_atomic_write(path,upresult,ulen);
		wtk_free(upresult);
	}
}

static void handle_uart_frame(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *frame, int len)
{
	FILE *fp = NULL;
	FILE *lp = NULL;
	FILE *mf = NULL;
	uint8_t normal = 0x00;
	uint8_t faile = 0xFE;
	int ret=0;
	int result=-1;
	char *pc;
	int s;
	int getresult;
	int ucnt = 0;
	char set_buf[128] = {0};
	char *upresult;
	int ulen;
	int timeout_s = 5;
	int resp;
	len = 0;
	uint8_t cehsi = 0x00;
	uint8_t response;
	uint16_t data_len = frame->data_length[0] | (frame->data_length[1] << 8);
	// uint16_t event_code = (frame->event_code[0] << 8) | frame->event_code[1];
	uint16_t event_code = frame->event_code[1] | (frame->event_code[0] << 8);
	cJSON *port_list;
	cJSON *port1;
	cJSON *port2;
	char msgdata1[256], msgdata2[256];
	char final_json[512];
	char *pv;
	int pos;
	int rc;
	int issend=1;
	char *json_str;

	double utime = time_get_ms();
	double event_start_ms = utime;
	wtk_debug("Received event_code: 0x%04X\n", event_code);
	infor_count++;
	switch (event_code)
	{
#ifdef chenggang
		/* ===== 日志上报：开始包 ACK（兼容两种码）===== */
	case EVT_LOG_UPLOAD_START: // 若对端回同码作为ACK
	case EVT_LOG_UPLOAD_START_ACK:
	{
	 // 若对端回 0x8120 作为ACK
		uint8_t st = 0x00;
		if (data_len >= 1 && frame->data)
			st = frame->data[0];
		ackq_push(uc, event_code, st);
		//  这里不要 qtk_uart_client_send_response(...)，否则可能形成请求-响应死循环 ★★
		break;
	}
	/* （可选）结束包ACK；如果你的协议不要求，就可以不加 */
	case EVT_LOG_UPLOAD_END:
	case EVT_LOG_UPLOAD_END_ACK:
	{
		uint8_t st = 0x00;
		if (data_len >= 1 && frame->data)
			st = frame->data[0];
		ackq_push(uc, event_code, st);
		break;
	}
#endif
	case 0x0101: // QTK_UART_TYPE_RECV_SPEAKER_JUDGMENT - 扬声器音频检测判断
		resp = lm->speak_check_result;
		if (resp == 1){
			normal = 0x01;// 静音
		}else if (resp == 2){
			normal = 0x02;// 爆音
		}else{
			normal = 0x00;// 正常
		}
		break;
	case 0x0102: // QTK_UART_TYPE_RECV_MIC_JUDGMENT - mic音频检测判断
		resp = lm->mic_check_result;
		wtk_debug("------------------------resp = %d\n", resp);
		if (resp == 1){
			normal = 0x01; // 静音
		}else if (resp == 2){
			normal = 0x02; // 爆音
		}else{
			normal = 0x00; // 正常
		}
		break;
	case 0x0103: /* QTK_UART_TYPE_RECV_MIC_JUDGMENT_OUTPUT_EQ_ADJUSTMENT - 输出EQ设置 */
		/* 1) 基本校验 */
		if (data_len == 0 || data_len > 4096 || frame->data == NULL){
			normal = 0xFE;
			wtk_debug("[EQ][0103] invalid payload, len=%d\n", data_len);
			break;
		}
		/* 2) 载荷净化 */
		const char *san_json = NULL;
		size_t san_len = 0;
		sanitize_json_payload((const uint8_t *)frame->data, (size_t)data_len, &san_json, &san_len);
		if (san_len == 0){
			normal = 0xFE;
			wtk_debug("[EQ][0103] sanitize failed\n");
			break;
		}
		/* 3) 落盘 eq.json（保留一份最新设置） */
		if (qtk_write_file(QTK_EQ_JSON_PATH, san_json, (int)san_len) != 0){
			normal = 0xFE;
			wtk_debug("[EQ][0103] write %s failed\n", QTK_EQ_JSON_PATH);
			break;
		}
		/* 4) 当前线程直接应用（不再通知后端 / 不再兜底） */
		rc = apply_eq_from_json_text_wtk(san_json, (int)san_len);
		if (rc != 0){
			wtk_debug("[EQ][0103] apply failed, rc=%d\n", rc);
			normal=0xFE;
			break;
		}
		/* 5) 正常应答 */
		break;
	case 0x0105: // QTK_UART_TYPE_RECV_SET_DENOISE_SWITCH - 智能降噪开关设置
		if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
			upresult=file_read_buf(UART_CFG_PATH,&ulen);
			pc = strstr(upresult, "VBOX3_ANS=");
			s = pc - upresult;
			if (frame->data[0] == 0x00){
				upresult[s + 10] = '0';
			}else if(frame->data[0] == 0x01){
				upresult[s + 10] = '1';
			}
			qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
		if (frame->data[0] == 0x00){
			qtk_vboxebf_set_denoiseenable(lm->avboxebf, 0);
		}else if (frame->data[0] == 0x01){
			qtk_vboxebf_set_denoiseenable(lm->avboxebf, 1);
		}
		wtk_strbuf_reset(uc->uart_buf2);
		break;
	case 0x0104: // QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH - 智能降噪开关获取
		result=qtk_vboxebf_get_denoisestate(lm->avboxebf);
		wtk_debug("anc status: %d\n", result);
		if (result == 0){
			response = 0x00;
		}else if (result == 1){
			response = 0x01;
		}
		wtk_debug("anc status: 0x%02X\n", response);
		normal=response;
		response = 0x00;
		break;
	case 0x0107: // QTK_UART_TYPE_RECV_SET_GAIN_CONTROL_SWITCH - 自动增益开关设置
		if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
			upresult=file_read_buf(UART_CFG_PATH,&ulen);
			pc = strstr(upresult, "VBOX3_AGC=");
			s = pc - upresult;
			if (frame->data[0] == 0x00){
				upresult[s + 10] = '0';
			}else if(frame->data[0] == 0x01){
				upresult[s + 10] = '1';
			}
			qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
		if (frame->data[0] == 0x00){
			qtk_vboxebf_set_agcenable(lm->avboxebf, 0);
		}else if (frame->data[0] == 0x01){
			qtk_vboxebf_set_agcenable(lm->avboxebf, 1);
		}
		wtk_strbuf_reset(uc->uart_buf2);
		break;
	case 0x0106: // QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH - 自动增益开关获取
		wtk_debug("agc status: %d\n", result);
		wtk_strbuf_reset(uc->uart_buf2);
		result = qtk_vboxebf_get_agcstate(lm->avboxebf);
		if (result == 0){
			response = 0x00;
		}else if (result == 1){
			response = 0x01;
		}
		normal=response;
		response = 0x00;
		wtk_log_log0(uc->log, "Get AGC_switch sucess!\n");
		break;
	case 0x0109: // QTK_UART_TYPE_RECV_SET_ECHO_INTENSITY_SWITCH - 回声抑制强度设置
		if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
		{
			upresult=file_read_buf(UART_CFG_PATH,&ulen);
			pc = strstr(upresult, "VBOX3_AEC=");
			s = pc - upresult;
			if (frame->data[0] == 0x00){
				upresult[s + 10] = '0';
			}else if (frame->data[0] == 0x01 || frame->data[0] == 0x02 || frame->data[0] == 0x03){
				upresult[s + 10] = '1';
				uint8_t aec_level = frame->data[0];

				pc = strstr(upresult, "vbox3_aec_level=");
				if (pc){
					s = pc - upresult;
					upresult[s + 16] = '0' + (aec_level - 0x00); // 转换为字符'1'/'2'/'3'
				}
			}
			qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
		if (frame->data[0] == 0x00){
			qtk_vboxebf_set_echoenable(lm->avboxebf, 0);
		}else if (frame->data[0] == 0x01 || frame->data[0] == 0x02 || frame->data[0] == 0x03){
			qtk_vboxebf_set_echoenable(lm->avboxebf, 1);
			uint8_t aec_level = frame->data[0];
			lm->cfg->avboxebf_cfg->aec_level = aec_level;
		}
		wtk_strbuf_reset(uc->uart_buf2);
		break;
	case 0x0108: // QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH - 回声抑制强度获取
		result = qtk_vboxebf_get_echostate(lm->avboxebf);
		if(result == 1){
			result = lm->cfg->avboxebf_cfg->aec_level;
		}
		wtk_debug("AEC Response:AEC level %d\n", result);
		if (result == 0){
			response = 0x00;
		}else if (result == 1){
			response = 0x01;
		}else if (result == 2){
			response = 0x02;
		}else if (result == 3){
			response = 0x03;
		}
		normal=response;
		response = 0x00; 
		break;
	case 0x010A: // QTK_UART_TYPE_RECV_GET_LIST_AUDIO_INPUT_PORTS - 获取音频输入口列表
		input_copunt++;
		wtdebugTime();
		wtk_debug("======================>>>>>>>>>>>>>>>>>>.input_count=%d\n",input_copunt);
		port_list = cJSON_CreateArray();
		port1 = cJSON_CreateObject();
		port2 = cJSON_CreateObject();
		wtk_debug("---------------------->>>>>>>>>>>>>>>>>>>\n");
		if(lm->rcd && lm->rcd_run){
			cJSON_AddNumberToObject(port1, "portType", 0);
			cJSON_AddNumberToObject(port1, "portId", 0xaa00002);
			cJSON_AddStringToObject(port1, "portName", "arrary_MIC");
			if(!lm->is_mic)
				cJSON_AddFalseToObject(port1, "isUse");
			else
				cJSON_AddTrueToObject(port1, "isUse");
			cJSON_AddNumberToObject(port1, "gainLevel", 1);
			cJSON_AddNumberToObject(port1, "audioInputType", 0);
			cJSON_AddFalseToObject(port1, "isLocalPlay");
			cJSON_AddItemToArray(port_list, port1);	
		}
		wtk_debug("---------------------->>>>>>>>>>>>>>>>>>>\n");
		if(lm->use_linein_out){
			cJSON_AddNumberToObject(port2, "portType", 1);
			cJSON_AddNumberToObject(port2, "portId", 0xaa00000);
			cJSON_AddStringToObject(port2, "portName", "Line_IN");
			if(!lm->use_linein_out)
				cJSON_AddFalseToObject(port2, "isUse");
			else
				cJSON_AddTrueToObject(port2, "isUse");
			cJSON_AddNumberToObject(port2, "gainLevel", 1);
			cJSON_AddNumberToObject(port2, "audioInputType", 0);
			cJSON_AddFalseToObject(port2, "isLocalPlay");
			cJSON_AddItemToArray(port_list, port2);
		}
		wtk_debug("---------------------->>>>>>>>>>>>>>>>>>>\n");
		json_str = cJSON_Print(port_list);
		qtk_uart_client_send_response(uc, frame, (uint8_t *)json_str, strlen(json_str));
		wtk_debug("udioinput_infor.txt JSON_infor: ulen=%d,[[[%.*s]]]\n",strlen(json_str),strlen(json_str), json_str);
		issend=0;
		cJSON_Delete(port_list);
		wtk_free(json_str);
		break;
	case 0x010B: // QTK_UART_TYPE_RECV_GET_LIST_AUDIO_OUTPUT_PORTS - 获取音频输出口列表
		port_list = cJSON_CreateArray();
		port1 = cJSON_CreateObject();
		port2 = cJSON_CreateObject();
		
		qtk_read_register("/sys/bus/i2c/devices/3-0069/volume", &s);
		if(lm->cfg->use_speaker){
			cJSON_AddNumberToObject(port1, "portType", 4);
			cJSON_AddNumberToObject(port1, "portId", 0xbb00001);
			cJSON_AddStringToObject(port1, "portName", "SPK");
			cJSON_AddTrueToObject(port1, "isUse");
			cJSON_AddNumberToObject(port1, "gainLevel", s);
			cJSON_AddNumberToObject(port1, "audioInputType", 0);
			cJSON_AddTrueToObject(port1, "isLocalPlay");
		} else {
			cJSON_AddNumberToObject(port1, "portType", 4);
			cJSON_AddNumberToObject(port1, "portId", 0xbb00001);
			cJSON_AddStringToObject(port1, "portName", "SPK");
			cJSON_AddFalseToObject(port1, "isUse");
			cJSON_AddNumberToObject(port1, "gainLevel", s);
			cJSON_AddNumberToObject(port1, "audioInputType", 0);
			cJSON_AddFalseToObject(port1, "isLocalPlay");
			// ret = snprintf(msgdata1, 256, "{\"portType\":0xbb000004,\"portId\":3,\"portName\":\"SPK\",\"isUse\":false,\"gainLevel\":%d,\"audioInputType\":0,\"isLocalPlay\":false}",s);
		}
		cJSON_AddItemToArray(port_list, port1);				
		if(lm->use_lineout_out){
			if(lm->cfg->use_headset){
				cJSON_AddNumberToObject(port2, "portType", 5);
				cJSON_AddNumberToObject(port2, "portId", 0xbb00000);
				cJSON_AddStringToObject(port2, "portName", "LINE_OUT");
				if(!lm->use_lineout_out)
					cJSON_AddFalseToObject(port2, "isUse");
				else
					cJSON_AddTrueToObject(port2, "isUse");
				cJSON_AddNumberToObject(port2, "gainLevel", s);
				cJSON_AddNumberToObject(port2, "audioInputType", 0);
				cJSON_AddFalseToObject(port2, "isLocalPlay");
			} else {
				cJSON_AddNumberToObject(port2, "portType", 5);
				cJSON_AddNumberToObject(port2, "portId", 0xbb00000);
				cJSON_AddStringToObject(port2, "portName", "LINE_OUT");
				if(!lm->use_lineout_out)
					cJSON_AddFalseToObject(port2, "isUse");
				else
					cJSON_AddTrueToObject(port2, "isUse");
				cJSON_AddNumberToObject(port2, "gainLevel", 1);
				cJSON_AddNumberToObject(port2, "audioInputType", 0);
				cJSON_AddFalseToObject(port2, "isLocalPlay");
			}
			cJSON_AddItemToArray(port_list, port2);
		}
		json_str = cJSON_Print(port_list);
		qtk_uart_client_send_response(uc, frame, (uint8_t *)json_str, strlen(json_str));
		wtk_debug("udioinput_infor.txt JSON_infor: ulen=%d,[[[%.*s]]]\n",strlen(json_str),strlen(json_str), json_str);
		issend=0;
		cJSON_Delete(port_list);

		wtk_free(json_str);
		break;
	case 0x010C: // QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_MIC - 启用禁用MIC
		if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
		{
			upresult=file_read_buf(UART_CFG_PATH,&ulen);
			
			pc = strstr(upresult, "VBOX3_MIC=");
			s = pc - upresult;
			wtk_debug("+========>>>>>>s=%d pc=%c\n", s, *pc);
			if (frame->data[4] == 0x00){
				upresult[s + 10] = '0';
			}else if (frame->data[4] == 0x01){
				upresult[s + 10] = '1';
			}
			qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
		if (frame->data[4] == 0x00){
			lm->is_mic = 0;
		}else if (frame->data[4] == 0x01){
			lm->is_mic = 1;
		}
		break;
	case 0x010D: // QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_SPK - 启用禁用SPK
		if (frame->data[0] == 0x01)
		{
			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
			{
				upresult=file_read_buf(UART_CFG_PATH,&ulen);
				pc = strstr(upresult, "VBOX3_SPK=");
				s = pc - upresult;
				wtk_debug("+========>>>>>>s=%d pc=%c\n", s, *pc);
				if (frame->data[4] == 0x00){
					upresult[s + 10] = '0';
				}else if (frame->data[4] == 0x01){
					upresult[s + 10] = '1';
				}
				qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
				wtk_free(upresult);
			}

			if (frame->data[4] == 0x00){
				lm->cfg->use_spkout = 0;
				lm->cfg->use_wooferout = 0;
			}else if (frame->data[4] == 0x01){
				lm->cfg->use_spkout = 1;
				lm->cfg->use_wooferout = 1;
			}
			if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
				upresult=file_read_buf(RES_CFG_PATH,&ulen);

				pc = strstr(upresult, "use_spkout=");
				s = pc - upresult;
				if (frame->data[4] == 0x00){
					upresult[s + 11] = '0' ;
				}else if (frame->data[4] == 0x01){
					upresult[s + 11] = '1' ;
				}

				pc = strstr(upresult, "use_wooferout=");
				s = pc - upresult;
				if (frame->data[4] == 0x00){
					upresult[s + 14] = '0' ;
				}else if (frame->data[4] == 0x01){
					upresult[s + 14] = '1' ;
				}
				qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
				wtk_free(upresult);
				// wtk_debug("================fwrite=%d\n",ret);
			}
		}else if (frame->data[0] == 0x00){
			if (frame->data[4] == 0x00){
				lm->cfg->use_headset=0;
			}else if (frame->data[4] == 0x01){
				lm->cfg->use_headset=1;
			}
			wtk_debug("-------->>>>>>>>>>>>>>m->use_headset=%d\n",lm->cfg->use_headset);
			if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
				upresult=file_read_buf(RES_CFG_PATH,&ulen);
				pc = strstr(upresult, "use_headset=");
				s = pc - upresult;
				if (frame->data[4] == 0x00){
					upresult[s + 12] = '0';
				}else if (frame->data[4] == 0x00){
					upresult[s + 12] = '1';
				}
				qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
				wtk_free(upresult);
				// wtk_debug("================fwrite=%d\n",ret);
			}
		}
		break;
	case 0x010E: // QTK_UART_TYPE_RECV_LINE_IN_CONTROL - line in本地输出控制(本地扩音)
		if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
		{
			upresult=file_read_buf(UART_CFG_PATH,&ulen);
			
			pc = strstr(upresult, "VBOX3_LINEIN=");
			s = pc - upresult;
			wtk_debug("+========>>>>>>s=%d pc=%c\n", s, *pc);
			if (frame->data[4] == 0x00){
				upresult[s + 13] = '0';
			}else if (frame->data[4] == 0x01){
				upresult[s + 13] = '1';
			}
			qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
		if (frame->data[4] == 0x00){
			lm->cfg->use_linein_courseware_touac = 1;
		}if (frame->data[4] == 0x01){
			lm->cfg->use_linein_courseware_touac = 0;
		}
		break;
	case 0x010F: // QTK_UART_TYPE_RECV_SET_INPUT_TYPE - 设置输入口类型
		if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
		{
			upresult=file_read_buf(UART_CFG_PATH,&ulen);
			
			pc = strstr(upresult, "USE_LINEIN_MIC=");
			s = pc - upresult;
			wtk_debug("+========>>>>>>s=%d pc=%c\n", s, *pc);
			if (frame->data[4] == 0x00){
				upresult[s + 15] = '1';
			}else if (frame->data[4] == 0x01){
				upresult[s + 15] = '0';
			}
			pc = strstr(upresult, "USE_LINEIN_SPK=");
			s = pc - upresult;
			if (frame->data[4] == 0x00){
				upresult[s + 15] = '0';
			}else if (frame->data[4] == 0x00){
				upresult[s + 15] = '1';
			}
			qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
		if(frame->data[4] == 0x00){
			lm->cfg->use_linein_mic = 1;
			lm->cfg->use_linein_courseware = 0;
			if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
				upresult=file_read_buf(RES_CFG_PATH,&ulen);
				
				pc = strstr(upresult, "use_linein_mic=");
				s = pc - upresult;
				upresult[s + 15]= '1' ;

				pc = strstr(upresult, "use_linein_courseware=");
				s = pc - upresult;
				upresult[s + 22] = '0' ;

				qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
				wtk_free(upresult);

			}
		}else if(frame->data[4] == 0x01){
			lm->cfg->use_linein_mic = 0;
			lm->cfg->use_linein_courseware = 1;
			if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
				upresult=file_read_buf(RES_CFG_PATH,&ulen);
				
				pc = strstr(upresult, "use_linein_mic=");
				s = pc - upresult;
				upresult[s + 15]= '0' ;

				pc = strstr(upresult, "use_linein_courseware=");
				s = pc - upresult;
				upresult[s + 22] = '1' ;

				qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
				wtk_free(upresult);

			}
		}
		break;
	case 0x011A: // QTK_UART_TYPE_RECV_GET_VOLUME_VALUE - 获取音量值
		wtk_debug("-------------->>>>>>>>>>>>>>>m->real_time = %f\n",lm->real_time);
		uint8_t get_volume = (uint8_t)lm->real_time;
		normal = get_volume;
		break;
	case 0x0110: // QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE - 读取麦克风音量档位
		upresult=file_read_buf(RES_CFG_PATH,&ulen);
		pv = strstr(upresult , "volum_input_mute=");
		ret = atoi(pv + 17);
		result = uc->mic_shift2;
		if(ret == 1)
			result = -result;			
		wtk_debug("MIC volume_value: 0x%02X\n", (uint8_t)result);
		normal = (uint8_t)result;
		wtk_free(upresult);
		break;
	case 0x0112: // QTK_UART_TYPE_RECV_SET_MICROPHONE_VOLUME_VALUE - 设置麦克风音量档位
		wtk_debug("----->>>>>>>>>>>>>>>>frame.data[4]======%d\n",frame->data[4]);
		float set;
		if(frame->data[4] == 255)
		{
			lm->cfg->mic_shift = 0.0;
			lm->cfg->volum_input_mute = 1;
		}else
		{
			lm->cfg->volum_input_mute = 0;
			uc->mic_shift2 = frame->data[4];
			wtk_debug("-------------->>>>>>>>>>>>>uc->mic_shift2 = %d\n",uc->mic_shift2);
			wtk_debug("==========>>>>>>>>>>>handle_time = %.2f\n",time_get_ms() - event_start_ms);
			if(frame->data[4] == 50)
			{
				set = 1.0f ;
			}else if(frame->data[4] == 0)
			{
				set = 0.25f;
			}
			else if(0<frame->data[4]<50)
			{
				set = 1.0f / powf(MIC_SHIFT ,50 - frame->data[4]);
				wtk_debug("=====>>>>>>>set = %.2f\n",set);
			}else if(50<frame->data[4]<= 100)
			{
				set = 1.0f * powf(MIC_SHIFT ,frame->data[4] - 50);
				wtk_debug("=====>>>>>>>set = %.2f\n",set);
			}
			lm->cfg->mic_shift =set;
			wtk_debug("==========>>>>>>>>>>>handle_time = %.2f\n",time_get_ms() - event_start_ms);
			wtk_debug("=====>>>>>>>set = %.2f\n",set);
			// if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
			// {
			// 	upresult=file_read_buf(UARTCFG_PATH,&ulen);
				
			// 	pv = strstr(upresult, "mic_shift2=");
			// 	if(pv)
			// 	{
			// 		char buf2[1024]={0};
			// 		sprintf(buf2,"mic_shift2=%0.2f;",set);
			// 		memcpy(pv,buf2,strlen(buf2));
			// 		qtk_mod_atomic_write(UARTCFG_PATH,upresult,ulen);
			// 	}
			// 	wtk_free(upresult);
			// }
			wtk_debug("==========>>>>>>>>>>>handle_time = %.2f\n",time_get_ms() - event_start_ms);
			
		}
		wtk_debug("==========>>>>>>>>>>>handle_time = %.2f\n",time_get_ms() - event_start_ms);
		if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
			upresult=file_read_buf(RES_CFG_PATH,&ulen);

			pv = strstr(upresult, "volum_input_mute=");
			s = pv - upresult;
			if(lm->cfg->volum_input_mute){
				upresult[s + 17] = '1' ;
			}else{
				upresult[s + 17] = '0' ;
			}

			pv = strstr(upresult, "mic_shift=");
			if(pv)
			{
				char buf2[1024]={0};
				sprintf(buf2,"mic_shift=%0.2f;",set);
				memcpy(pv,buf2,strlen(buf2));
				qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
			}
			wtk_free(upresult);
		}
		wtk_debug("==========>>>>>>>>>>>handle_time = %.2f\n",time_get_ms() - event_start_ms);
	#if 0
		if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
			upresult=file_read_buf(RES_CFG_PATH,&ulen);
			pv = strstr(upresult, "volum_input_mute=");
			s = pv - upresult;
			if(lm->cfg->volum_input_mute){
				upresult[s + 17] = '1' ;
			}else{
				upresult[s + 17] = '0' ;
			}
			qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
		wtk_debug("==========>>>>>>>>>>>handle_time = %.2f\n",time_get_ms() - event_start_ms);
	#endif
		break;
	case 0x0111: // QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE - 读取扬声器音量档位
		int8_t val2;
		val2 = uc->spk_volume;
		if(lm->cfg->volum_output_mute){
			val2 = -val2;
		}
		wtk_debug("The value is: %d,uc-<spk_volume=%d\n", val2,uc->spk_volume);
		normal =  (uint8_t)val2;
		break;
	case 0x0113: // QTK_UART_TYPE_RECV_SET_SPEKER_VOLUME_VALUE - 设置扬声器音量档位
		ret = frame->data[4];
		wtk_debug("==========SET_VOLUMOUTPUT===========>>>>>>>>>>>>>ret=%d\n",ret);
		upresult=file_read_buf(RES_CFG_PATH,&ulen);
		if(ret == 255){
			lm->cfg->echo_shift = 0.0f;
			lm->cfg->volum_output_mute = 1;
			ret=0;
		}else{
			uc->spk_volume = ret;
			lm->cfg->volum_output_mute = 0;
			wtk_debug("ret = %d\n",ret);
			if(uc->cfg->pingsuan_run==0){
				float lineout_val = 1 * (ret / 100.0) ;
				lm->cfg->echo_shift=lineout_val;
				printf("Mapped volume: %.2f,m->cfg->echo_shift:%.2f\n", lineout_val,lm->cfg->echo_shift);
				if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
					upresult=file_read_buf(RES_CFG_PATH,&ulen);
					pv = strstr(upresult, "echo_shift=");
					if(pv){
						char buf2[1024]={0};
						sprintf(buf2,"echo_shift=%0.2f;",lineout_val);
						memcpy(pv,buf2,strlen(buf2));
						// qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
					}
					// wtk_free(upresult);
				}
			}else{
				ret=100;
				pv = strstr(upresult, "echo_shift=");
				float scale;
				if (sscanf(pv, "echo_shift=%f", &scale) == 1) {
					lm->cfg->echo_shift = scale;
					wtk_debug("=====>>>>>>>>>>>>>echo_shift = %.2f\n", scale);
				}
				// wtk_free(upresult);

				// pv = strstr(upresult, "echo_shift=");
				// wtk_debug("-------------->>>>>>\n");
				// float scale = wtk_str_atof(pv + 11, ret - (pv - upresult) - 12);
				// wtk_debug("-------------->>>>>>\n");
				// lm->cfg->echo_shift = scale;
				// wtk_debug("-------------->>>>>>scale = %.2f\n",scale);
			}
		}
		fp = fopen("/sys/bus/i2c/devices/3-006d/volume", "w");
		if (fp){
			fprintf(fp, "%d", ret);
			fclose(fp);
			fp=NULL;
		}
		wtk_debug("------->echo %d > /sys/bus/i2c/devices/3-006d/volume\n", ret);
		lp = fopen("/sys/bus/i2c/devices/3-0069/volume", "w");
		if (lp){
			fprintf(lp, "%d", ret);
			fclose(lp);
			lp=NULL;
		}
		wtk_debug("------->echo %d > /sys/bus/i2c/devices/3-0069/volume\n", ret);
		// if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
		// upresult=file_read_buf(RES_CFG_PATH,&ulen);
		pv = strstr(upresult, "volum_output_mute=");
		wtk_debug("------------------>>>>>>>>>>>>>>>>>>>\n");
		s = pv - upresult;
		if(lm->cfg->volum_output_mute){
			upresult[s + 18] = '1' ;
		}else{
			upresult[s + 18] = '0' ;
		}
		wtk_debug("------------------>>>>>>>>>>>>>>>>>>>\n");
		qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
		wtk_free(upresult);
		// }
		wtk_debug("------------------>>>>>>>>>>>>>>>>>>>\n");
		if(frame->data[4] != 255 && frame->data != -1){
			#if 1
				fp = fopen("/oem/qdreamer/qsound/spk_volume.txt", "w");
				if (!fp) {
					wtk_debug("Failed to open : %s\n", strerror(errno));
					fclose(fp);
				}
				if (fp){
					fprintf(fp, "%d", frame->data[4]);
					fflush(fp);
					int fd = fileno(fp);
					if (fd >= 0){
						fsync(fd);
					}
					fclose(fp);
				}
			#else
				int vol = frame->data[4];
				qtk_mod_atomic_write("/oem/qdreamer/qsound/spk_volume.txt",vol,strlen(vol));
			#endif
		}
		wtk_debug("------------------>>>>>>>>>>>>>>>>>>>\n");
		wtk_debug("no handle----pingsuan_run = %d\n",uc->cfg->pingsuan_run);
		break;
	case 0x0114: // QTK_UART_TYPE_RECV_LOG_REPORTING - 日志上报
		system("tar -cvf /tmp/qtk_uart_client.log.tar /tmp/qtk_uart_client.log");
		fp = popen("tar -cvf /tmp/qtk_uart_client.log.tar /tmp/qtk_uart_client.log", "r");
		if (fp){
			fclose(fp);
		}
		char *data = file_read_buf("/tmp/qtk_uart_client.log.tar", &len);
		qtk_uart_client_send_response(uc, frame, data, len);
		issend=0;
		if (data){
			wtk_free(data);
			data = NULL;
		}
		break;
	case 0x0115: // QTK_UART_TYPE_RECV_ALARM_REPORTING - 告警上报
		break;
	case 0x0116: // QTK_UART_TYPE_RECV_AUDIO_STATUS_CHANGE_NOTIFICATION - 音频状态变化通知
		break;
	case 0x0117: // QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_UNPLUGGING - 音频输入输出设备拔出通知
		break;
	case 0x0118: // QTK_UART_TYPE_RECV_SPEAKER_CONTROL - 扬声器控制
		ret = frame->data[0];
		if(ret == 0){
			lm->cfg->use_speaker_left=0;
			lm->cfg->use_speaker_right=0;
			lm->cfg->use_wooferout = 0;
		}
		if(ret == 1){
			lm->cfg->use_speaker_left=1;
			lm->cfg->use_speaker_right=0;
			lm->cfg->use_wooferout = 0;
		}
		if(ret == 2){
			lm->cfg->use_speaker_left=0;
			lm->cfg->use_speaker_right=1;
			lm->cfg->use_wooferout = 0;
		}
		if(ret == 3){
			lm->cfg->use_speaker_left=0;
			lm->cfg->use_speaker_right=0;
			lm->cfg->use_wooferout = 1;
		}
		if(ret == 4){
			lm->cfg->use_speaker_left=1;
			lm->cfg->use_speaker_right=1;
			lm->cfg->use_wooferout = 1;
		}
		break;

	case 0x0119: // QTK_UART_TYPE_RECV_LOG_COLLECTION - 日志收集
#ifdef chenggang
				 // ① 按协议立即回一个空载荷响应，保持请求-响应对称
		qtk_uart_client_send_response(uc, frame, NULL, 0); // 注意函数签名：最后一个参数是长度，传 0
		// ② 置位“一次性触发上报”标志，交给 report 线程去执行
		issend=0;
		if (uc)
		{
			uc->report_trigger_once = 1;
			if (uc->log)
				wtk_log_log0(uc->log, "[0119] trigger immediate log upload");
		}
		break;
#endif
	case 0x011B: // QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_INSERTIOIN - 音频输入输出设备插入通知
		break;
	case 0x011C: // QTK_UART_TYPE_RECV_GET_OUTPUT_EQ_MODE - 获取输出EQ模式
		const char *MODE_PATH = "/oem/qdreamer/qsound/eq_mode";
		unsigned char mode = 0x00; // 缺省：STANDARD_MODE
		// 协议请求数据长度应为 0；为健壮起见，忽略载荷内容，直接读取持久化的模式
		(void)data_len;
		mf = fopen(MODE_PATH, "rb");
		if (mf){
			char line[32] = {0};
			size_t n = fread(line, 1, sizeof(line) - 1, mf);
			fclose(mf);
			if (n > 0){
				// 允许 "0\n"、"1"、"2" 或 "0x00" 等写法
				char *endp = NULL;
				long v = strtol(line, &endp, 0);
				if (v >= 0 && v <= 2){
					mode = (unsigned char)v;
				}
			}
		}
		// 响应：按协议返回 1 字节模式值
		// 0x00=STANDARD_MODE, 0x01=WALL_MOUNTED__MODE, 0x02=WALL_RECESSED__MODE
		normal = mode;
		UART_LOG_DBG(uc, "evt=0x011C done mode=%u latency=%.2fms", mode, time_get_ms() - event_start_ms);
		break;
	case 0x011D: /* QTK_UART_TYPE_RECV_SET_OUTPUT_EQ_MODE - 设置输出EQ模式 */
		
		const char *preset_path = NULL;
		unsigned char eq_mode_val;
		char *buf = NULL;
		long fsize = 0;

		/* 1) 载荷校验：1 字节模式值（0=标准，1=挂墙，2=嵌墙） */
		if (data_len != 1){
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D invalid payload len=%u latency=%.2fms", data_len, time_get_ms() - event_start_ms);
			break;
		}

		/* 2) 选择预置 JSON 文件路径 */
		eq_mode_val = frame->data[0];
		switch (eq_mode_val)
		{
		case 0x00:
			preset_path = "/oem/qdreamer/qsound/presets/eq_standard.json";
			break;
		case 0x01:
			preset_path = "/oem/qdreamer/qsound/presets/eq_wall.json";
			break;
		case 0x02:
			preset_path = "/oem/qdreamer/qsound/presets/eq_recessed.json";
			break;
		default:
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D invalid mode=0x%02X latency=%.2fms", eq_mode_val, time_get_ms() - event_start_ms);
			break;
		}
		if (!preset_path)
			break;

		/* 3) 读预置 JSON 到内存，并备份到 eq.json */
		fp = fopen(preset_path, "rb");
		if (!fp){
			wtk_debug("[EQ][011D] open %s failed: %s\n", preset_path, strerror(errno));
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D preset open failed latency=%.2fms", time_get_ms() - event_start_ms);
			break;
		}
		if (fseek(fp, 0, SEEK_END) != 0){
			fclose(fp);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D preset seek end failed latency=%.2fms", time_get_ms() - event_start_ms);
			break;
		}
		fsize = ftell(fp);
		if (fsize <= 0 || fsize > 64 * 1024){
			fclose(fp);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D preset size invalid=%ld latency=%.2fms", fsize, time_get_ms() - event_start_ms);
			break;
		}
		if (fseek(fp, 0, SEEK_SET) != 0){
			fclose(fp);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D preset seek set failed latency=%.2fms", time_get_ms() - event_start_ms);
			break;
		}

		buf = (char *)malloc((size_t)fsize + 1);
		if (!buf){
			fclose(fp);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D malloc %ld failed latency=%.2fms", fsize, time_get_ms() - event_start_ms);
			break;
		}
		if ((long)fread(buf, 1, (size_t)fsize, fp) != fsize){
			fclose(fp);
			free(buf);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D fread short latency=%.2fms", time_get_ms() - event_start_ms);
			break;
		}
		fclose(fp);
		buf[fsize] = '\0';
		if (qtk_write_file(QTK_EQ_JSON_PATH, buf, (int)fsize) != 0){
			wtk_debug("[EQ][011D] write %s failed\n", QTK_EQ_JSON_PATH);
			free(buf);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D write eq.json failed latency=%.2fms", time_get_ms() - event_start_ms);
			break;
		}
		/* 读完 buf/fsize 后，先净化再用 */
		const char *san = NULL;
		size_t slen = 0;
		sanitize_json_payload((const uint8_t *)buf, (size_t)fsize, &san, &slen);
		if (slen == 0 || san == NULL){
			wtk_debug("[EQ] sanitize failed\n");
			free(buf);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D sanitize failed latency=%.2fms", time_get_ms() - event_start_ms);
			break;
		}
		/* 先把净化后的 JSON 写入 eq.json（保持与 0x0103 一致） */
		if (qtk_write_file(QTK_EQ_JSON_PATH, san, (int)slen) != 0){
			wtk_debug("[EQ][011D] write %s failed\n", QTK_EQ_JSON_PATH);
			free(buf);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D write sanitized eq.json failed latency=%.2fms", time_get_ms() - event_start_ms);
			break;
		}

		/* 然后应用（用净化后的切片） */
		rc = apply_eq_from_json_text_wtk(san, (int)slen);
		free(buf);
		if (rc != 0){
			wtk_debug("[EQ][011D] apply failed, rc=%d\n", rc);
			normal = faile;
			UART_LOG_WARN(uc, "evt=0x011D apply failed rc=%d latency=%.2fms", rc, time_get_ms() - event_start_ms);
			break;
		}
		/* === 新增：持久化当前模式，供 0x011C 读取 === */
		{
			const char *MODE_PATH = "/oem/qdreamer/qsound/eq_mode";
			char tmp_path[128];
			snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", MODE_PATH);

			/* 写临时文件，再 rename 原子替换，避免掉电/并发读到半文件 */
			mf = fopen(tmp_path, "wb");
			if (!mf){
				wtk_debug("[EQ][011D] open %s failed: %s\n", tmp_path, strerror(errno));
				/* 不因持久化失败而判整个操作失败；仅告警 */
			}else{
				/* 存十进制或 0xXX 都可；你 0x011C 用 strtol(base=0) 兼容 */
				/* 这里写十进制更直观：0/1/2\n */
				fprintf(mf, "%u\n", (unsigned)eq_mode_val & 0xFF);
				fflush(mf);
				fsync(fileno(mf)); /* 可选：确保落盘（视系统支持） */
				fclose(mf);

				if (rename(tmp_path, MODE_PATH) != 0){
					wtk_debug("[EQ][011D] rename(%s -> %s) failed: %s\n",
							  tmp_path, MODE_PATH, strerror(errno));
				}else{
					wtk_debug("[EQ][011D] mode persisted: %u (0:STD,1:WALL,2:RECESSED)\n",
							  (unsigned)eq_mode_val);
				}
			}
		}
		
		UART_LOG_DBG(uc, "evt=0x011D done mode=%u latency=%.2fms", eq_mode_val, time_get_ms() - event_start_ms);
		break;
	case 0x011E: // QTK_UART_TYPE_RECV_GET_LINEOUT_MODE - 获取lineout输出模式
		result = uc->lineout_pattern;
		normal = (uint8_t)result;
		break;
	case 0x011F: // QTK_UART_TYPE_RECV_SET_LINEOUT_MODE - 设置lineout输出模式
		ret = frame->data[0];
		uc->lineout_pattern=ret;
		wtk_debug("---------------->>>>>>>>>>>>>>>ret=%d\n",ret);
		if(ret == 0){
			lm->cfg->use_mainlineout=1;
			lm->cfg->use_wooflineout=0;
			lm->cfg->use_meetinglineout=0;
			lm->cfg->use_expandlineout=0;
		}
		if(ret == 1){
			lm->cfg->use_mainlineout=0;
			lm->cfg->use_wooflineout=1;
			lm->cfg->use_meetinglineout=0;
			lm->cfg->use_expandlineout=0;
		}
		if(ret == 2){
			lm->cfg->use_mainlineout=0;
			lm->cfg->use_wooflineout=0;
			lm->cfg->use_meetinglineout=1;
			lm->cfg->use_expandlineout=0;
		}
		if(ret == 3){
			lm->cfg->use_mainlineout=0;
			lm->cfg->use_wooflineout=0;
			lm->cfg->use_meetinglineout=0;
			lm->cfg->use_expandlineout=1;
		}
	#if 1
		if (access("/oem/qdreamer/qsound/res/cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/res/cfg", W_OK) == 0){
			upresult=file_read_buf(RES_CFG_PATH,&ulen);
			
			pv = strstr(upresult, "use_mainlineout=");
			s = pv - upresult;
			if(ret==0){
				upresult[s + 16] = '1' ;
			}else{
				upresult[s + 16] = '0' ;
			}
			pv = strstr(upresult, "use_wooflineout=");
			s = pv - upresult;
			if(ret == 1){
				upresult[s + 16] = '1' ;
			}else{
				upresult[s + 16] = '0' ;
			}
			pv = strstr(upresult, "use_meetinglineout=");
			s = pv - upresult;
			if(ret == 2){
				upresult[s + 19] = '1' ;
			}else{
				upresult[s + 19] = '0' ;
			}
			pv = strstr(upresult, "use_expandlineout=");
			s = pv - upresult;
			if(ret==3){
				upresult[s + 18] = '1' ;
			}else{
				upresult[s + 18] = '0' ;
			}
			qtk_mod_atomic_write(RES_CFG_PATH,upresult,ulen);
			wtk_free(upresult);
		}
	#endif
		wtk_debug("-------------->>>>>>>>>>>>>>\n");
		char buf3[128]={0};
		int sret=0;
#if 0 //def USE_ATOMIC_FILE
		memset(buf3, 0, 128);
		sret = sprintf(buf3, "%d", ret);
		qtk_mod_atomic_write("/oem/qdreamer/qsound/lineout_pattern.txt", buf3, sret);
		if(lm->c5_pull_high){
			qtk_mod_atomic_write("/sys/class/gpio/export", "53", strlen("53"));
			qtk_mod_atomic_write("/sys/class/gpio/gpio53/direction", "out", strlen("out"));
			qtk_mod_atomic_write("/sys/class/gpio/gpio53/value", "1", strlen("1"));
			lm->c5_pull_high = 0;
		}
#else
		memset(buf3, 0, 128);
		sret = sprintf(buf3, "%d", ret);
		qtk_mod_atomic_write("/oem/qdreamer/qsound/lineout_pattern.txt", buf3, sret);
		// sprintf(buf3,"echo %d > /oem/qdreamer/qsound/lineout_pattern.txt",ret);
		// system(buf3);
		if(lm->c5_pull_high){
			// usleep(50*1000);
			fp = fopen("/sys/class/gpio/export", "w");
			if (fp) {
				fprintf(fp, "%d", 53);
				fclose(fp);
			}
			fp = fopen("/sys/class/gpio/gpio53/direction", "w");
			if (fp) {
				fprintf(fp, "%s", "out");
				fclose(fp);
			}
			fp = fopen("/sys/class/gpio/gpio53/value", "w");
			if (fp) {
				fprintf(fp, "%d", 1);
				fclose(fp);
			}
			lm->c5_pull_high = 0;
		}
#endif
		break;
	case 0x0124:
		lm->cfg->mic_shift = 1.0;
		lm->cfg->echo_shift = 1.0;
		lm->cfg->uac_shift = 1.0;
		lm->cfg->in_inputtolineout_shift = 2.12;
		lm->cfg->in_inputtospeaker_shift = 2.12;
		lm->cfg->use_speaker_left =1 ;
		lm->cfg->use_speaker_right = 1;
		if(lm->use_lineout_out)
		{
			lm->cfg->use_spkout = 0;
			lm->cfg->use_wooferout =0;
			lm->cfg->use_headset = 1;
		}else{
			lm->cfg->use_spkout = 1;
			lm->cfg->use_wooferout =1;
			lm->cfg->use_headset = 0;
		}
		if(lm->use_linein_out)
		{
			lm->cfg->use_linein_courseware = 1;
		}else{
			lm->cfg->use_linein_courseware = 0;
		}
		lm->cfg->use_linein_mic = 0;
		lm->cfg->use_linein_courseware_touac = 1;
		lm->cfg->use_mainlineout = 1;
		lm->cfg->use_wooflineout = 0;
		lm->cfg->use_meetinglineout = 0;
		lm->cfg->use_expandlineout = 0;
		lm->cfg->volum_input_mute = 0;
		lm->cfg->volum_output_mute = 0;
#ifdef USE_ATOMIC_FILE
		upresult=file_read_buf("/oem/qdreamer/qsound/res/cfg_initial", &ulen);
		pv = strstr(upresult, "use_spkout=");
		s = pv - upresult;
		if(lm->cfg->use_spkout){
			upresult[s + 11] = '1' ;
		}else{
			upresult[s + 11] = '0' ;
		}

		pv = strstr(upresult, "use_wooferout=");
		s = pv - upresult;
		if(lm->cfg->use_wooferout){
			upresult[s + 14] = '1' ;
		}else{
			upresult[s + 14] = '0' ;
		}

		pv = strstr(upresult, "use_headset=");
		s = pv - upresult;
		if(lm->cfg->use_headset){
			upresult[s + 12] = '1' ;
		}else{
			upresult[s + 12] = '0' ;
		}
		
		pv = strstr(upresult, "use_linein_courseware=");
		s = pv - upresult;
		if(lm->cfg->use_linein_courseware){
			upresult[s + 22] = '1' ;
		}else{
			upresult[s + 22] = '0' ;
		}
		qtk_mod_atomic_write("/oem/qdreamer/qsound/res/cfg", upresult, ulen);
		wtk_free(upresult);
#else
		system("cp /oem/qdreamer/qsound/res/cfg_initial /oem/qdreamer/qsound/res/cfg");
#endif
		wtk_debug("--------->>>>>>>>.Start reply initialization\n");
		break;
	case 0x0125:
		wtk_debug("----------->>>>>>>>>>>.0x0125  frame->data[0]=%d\n",frame->data[0]);
		ret=0;
		if(frame->data[0] == 0x01)
		{
			uc->cfg->pingsuan_run = 1;
			wtk_debug("----pingsuan_run = %d\n",uc->cfg->pingsuan_run);
			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
			{
				upresult=file_read_buf(UART_CFG_PATH,&ulen);
				pc = strstr(upresult, "pingsuan_run=");
				s = pc - upresult;
				upresult[s + 13] = '1';
				qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
				wtk_free(upresult);
			}

			if(lm->cfg->volum_output_mute == 0)
			{
				ret=100;
			}
		}else if (frame->data[0] == 0x00)
		{
			uc->cfg->pingsuan_run= 0;
			wtk_debug("----pingsuan_run = %d\n",uc->cfg->pingsuan_run);
			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
			{
				upresult=file_read_buf(UART_CFG_PATH,&ulen);
				pc = strstr(upresult, "pingsuan_run=");
				s = pc - upresult;
				upresult[s + 13] = '0';
				qtk_mod_atomic_write(UART_CFG_PATH,upresult,ulen);
				wtk_free(upresult);
			}
			if(lm->cfg->volum_output_mute == 0){
				char*p=file_read_buf("/oem/qdreamer/qsound/spk_volume.txt",&len);
				ret=atoi(p);
				wtk_free(p);
			}
		}
		fp = fopen("/sys/bus/i2c/devices/3-006d/volume", "w");
		if (fp)
		{
			fprintf(fp, "%d", ret);
			fclose(fp);
		}
		wtk_debug("------->echo %d > /sys/bus/i2c/devices/3-006d/volume\n", ret);
		lp = fopen("/sys/bus/i2c/devices/3-0069/volume", "w");
		if (lp)
		{
			fprintf(lp, "%d", ret);
			fclose(lp);
		}
		break;
	case 0x0127:
		issend=0;
		qtk_uart_client_send_response(uc, frame, &normal, 1);
		if(lm && lm->cfg && lm->cfg->use_usbaudio){
			// When working as a USB audio device we must not force a soft disconnect,
			// otherwise Android re-enumerates us, resets its own volume slider and
			// shows the system volume UI after resume.
			wtk_debug("skip USB reset for 0x0127 because use_usbaudio is enabled\n");
			break;
		}
		if(lm->rcd4_run){
			lm->rcd4_run = 0;
			wtk_thread_join(&lm->rcd4_t);
			if(lm->rcd4){
				qtk_record_delete(lm->rcd4);
			}
		}
		if(lm->cfg->use_usbaudio){
			lm->usbaudio_run = 0;
			wtk_blockqueue_wake(&lm->usbaudio_queue);
			wtk_thread_join(&lm->usbaudio_t);
		}
		system("killall uart_ota");
		qtk_mod_am13e2_check_usb();
		break;
	default:
		normal = 0XFF;
		break;
	}
	if(issend){
		qtk_uart_client_send_response(uc, frame, &normal, 1);
	}
	wtk_debug("==========>>>>>>>>>>>now count = %d , handle_time = %.2f\n",infor_count, time_get_ms() - event_start_ms);
}

void qtk_mod_uart_on_recv(qtk_mod_am13e2_t *m, qtk_uart_recv_frame_t *frame, int len)
{
	handle_uart_frame(m->uart, frame, len);
}
void qtk_mod_am13e2_check_usb()
{
	// system("echo '[$(cat /proc/uptime | cut -d'\\'' ' -f1)s] echo  disconnect'");
	// usleep(100*1000);

	// system("echo '[$(cat /proc/uptime | cut -d'\\'' ' -f1)s] echo connected'");
	// system("echo connect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
	// system("echo connect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
	// system("sync");
	wtk_debug("------------------------qtk_mod_am13e2_check_usb-----0x0127----------------------->>>>>>>>>>>>>\n");
	wtdebugTime();
	qtk_timer_remove(lm->sttimer,lm,qtk_mod_am13e2_time_callback);
	wtk_debug("------------------>>>>>>>>>>>>>>>>>>>>>usb_stata = %d\n",usb_stata);
	// usb_stata = 1;
	wtk_debug("------------------>>>>>>>>>>>>>>>>>>>>>usb_stata = %d\n",usb_stata);
	
	qtk_timer_add(lm->sttimer,6*1000,lm,qtk_mod_am13e2_time_callback);	
	wtk_debug("========================================.>>>>>>>>>>>>>>>>>>>>>>>>>>>");

}

void qtk_mod_am13e2_time_callback(void *user_data,int type)
{
	wtdebugTime();
	wtk_debug("------------------>>>>>>>>>>>>>>>>>>>>>usb_stata = %d\n",usb_stata);
	if(usb_stata != 0 )
	{
	 	system("echo disconnect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
		// system("echo disconnect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect");
		wtk_debug("==============echo disconnect >/sys/devices/platform/ff400000.usb/udc/ff400000.usb/soft_connect!!!!\n");
		usleep(400*1000);
	}
	usb_stata = 1;
	qtk_timer_remove(lm->sttimer,user_data,qtk_mod_am13e2_time_callback);
}
