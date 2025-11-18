#include "qtk_uart_client.h"
#include "wtk/core/wtk_os.h"
static double tm_s = 0;
static double tm_e = 0;
// #define UART

#define USE_3308
// #define USE_LOG

// #define CESHI
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <syslog.h>
#include <limits.h>
#include <fcntl.h>
#include <pthread.h> // 互斥锁（若你不想加锁，可略过pthread相关两行）
int send_count = 0;
#ifndef MIC_SHIFT2
#define MIC_SHIFT2 powf(powf(10.0f, (12.0f / 20.0f)), 1/50)
#endif
void uart_log(qtk_uart_client_t *uc, int level, const char *fmt, ...)
{
	va_list ap;
	char buf[256];
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	if (uc && uc->log)
	{
		switch (level)
		{
		case LOG_ERR:
			wtk_log_err0(uc->log, buf);
			break;
		case LOG_WARN:
			wtk_log_warn0(uc->log, buf);
			break;
		default:
			wtk_log_log0(uc->log, buf);
			break;
		}
	}
	else
	{
		switch (level)
		{
		case LOG_ERR:
			wtk_debug("[UART][ERR] %s\n", buf);
			break;
		case LOG_WARN:
			wtk_debug("[UART][WARN] %s\n", buf);
			break;
		default:
			wtk_debug("[UART][DBG] %s\n", buf);
			break;
		}
	}
}

/********** 主动上报事件码（按协议V1.8） **********/
#define EVT_LOG_UPLOAD_START 0x0120 // 开始包
#define EVT_LOG_UPLOAD_DATA 0x0121	// 数据包
#define EVT_LOG_UPLOAD_END 0x0122	// 结束包
/* #define EVT_LOG_UPLOAD_RETRANS 0x0123 // 重传（可选扩展） */

/********** 监控路径与阈值 **********/
#ifndef LOG_WATCH_DIR
#define LOG_WATCH_DIR "/oem/qdreamer/qsound/ulog"
#endif

#ifndef LOG_PACKAGE_THRESHOLD_BYTES
#define LOG_PACKAGE_THRESHOLD_BYTES (3 * 1024ULL)
#endif

#ifndef LOG_PACKAGE_MAX_COUNT
#define LOG_PACKAGE_MAX_COUNT 10
#endif

#ifndef LOG_PACKAGE_MAX_BYTES
#define LOG_PACKAGE_MAX_BYTES (3 * 1024ULL)
#endif

#ifndef LOG_SCAN_INTERVAL_MS
#define LOG_SCAN_INTERVAL_MS 2000
#endif

/* 单包最大载荷：协议上限 65535（这里要预留 2 字节序号） */
#define LOG_DATA_MAX_PER_FRAME (65535 - 2)

/* 数据包间隔节流，防止对端缓冲顶满（微秒） */
#define LOG_FRAME_GAP_US 2000
// #endif
qtk_uart_recv_frame_t current_frame = {0};
int qdreamer_audio_check_request;		  // 0-无请求, 1-扬声器检测, 2-麦克风检测
int qdreamer_audio_check_result;		  //  0-正常, 1-静音, 2-爆音
uint64_t qdreamer_audio_check_start_time; // 检测开始时间戳
int qdreamer_audio_check_running;		  // 检测状态标志
time_t startspk_time;
double recvdata_time = 0;
int isnot_spk_count = 0;
int list_input = 0;
int list_output = 0;

qtk_uart_client_t *lc;

int qtk_uart_client_trsn_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
int qtk_uart_client_msg_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
int qtk_uart_client_send_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
static qtk_uart_client_msg_t *qtk_uart_client_msg_new(qtk_uart_client_t *uc);
static int qtk_uart_client_msg_delete(qtk_uart_client_msg_t *msg);
static qtk_uart_client_msg_t *qtk_uart_client_pop_msg(qtk_uart_client_t *uc);
static void qtk_uart_client_push_msg(qtk_uart_client_t *uc, qtk_uart_client_msg_t *msg);
static void qtk_uart_client_clean_q(qtk_uart_client_t *uc, wtk_blockqueue_t *queue);
int qtk_uart_client_uart_param(qtk_uart_client_t *uc, wtk_strbuf_t *buf);
static void qtk_uart_client_feed_notice(qtk_uart_client_t *uc, int notice);
void handle_uart_frame(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *frame, int len);
// void handle_uart_frame(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *frame);

static void send_response(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *req_frame, uint8_t *data, uint16_t data_len);
static void send_active_response(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, uint8_t *data, uint16_t data_len);
static pthread_mutex_t g_amp_lock = PTHREAD_MUTEX_INITIALIZER;
static int map_gain_to_level(float g_db);
static int sysfs_write_hex_u8(const char *path, unsigned code_hex);
static int send_audio_status_frame(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, int type, int is_exit);
void wtdebugCurrentTime();
static void log_hex_buffer(qtk_uart_client_t *uc, const char *tag, const uint8_t *buf, uint16_t len);
static void log_incoming_frame_hex(qtk_uart_client_t *uc, const qtk_uart_recv_frame_t *frame, uint16_t data_len);
static int qtk_uart_client_send_active(qtk_uart_client_t *uc, uint16_t event_code,
									   const uint8_t *data, uint16_t data_len);
static int send_audio_status_frame(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, int type, int is_exit);
void qtk_uart_set_cpu(qtk_uart_client_t *m, wtk_thread_t *thread, int cpunum);
static int qtk_uart_client_report_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
static int send_file_in_chunks(qtk_uart_client_t *uc, const char *file_path);
static void le16_write(uint8_t *p, uint16_t v);
static void le32_write(uint8_t *p, uint32_t v);
static void le64_write(uint8_t *p, uint64_t v);
static int ensure_watch_directory(void);
static int package_pending_logs(qtk_uart_client_t *uc, int force_now);
static int enforce_package_queue_limit(qtk_uart_client_t *uc);
static int upload_ready_packages(qtk_uart_client_t *uc);
int qtk_uart_atomic_write(const char *filename, const void *data, size_t len) {
    char temp_template[1024];
    char temp_path[1024];
    int fd, ret;
    
    // 创建临时文件
    snprintf(temp_template, sizeof(temp_template), "%s.XXXXXX", filename);
    fd = mkstemp(temp_template);
    if (fd == -1) {
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
    if (fsync(fd) == -1) {
        close(fd);
        unlink(temp_path);
        return -1;
    }
    close(fd);
    // 原子性地重命名临时文件为目标文件
    if (rename(temp_path, filename) == -1) {
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
void wtdebugCurrentTime()
{
	// 获取当前时间戳
	time_t currentTime;
	time(&currentTime);

	// 转换为本地时间结构
	struct tm *localTime = localtime(&currentTime);

	// 检查时间获取是否成功
	if (localTime == NULL)
	{
		wtk_debug("can not get time\n");
		return;
	}

	// 提取时、分、秒
	int hour = localTime->tm_hour;	// 时 (0-23)
	int minute = localTime->tm_min; // 分 (0-59)
	int second = localTime->tm_sec; // 秒 (0-59)

	// 打印格式化的时间
	wtk_debug("NOW time: %02d:%02d:%02d\n", hour, minute, second);
}
#if 0
int qtk_uart_linein_lineout_check_entry(qtk_uart_client_t *uc, wtk_thread_t *t)
{
	int lvalue1=0;
	int lvalue2=0;
	int ovalue=0;

	int dlen=0;
	int offcount1=0;
	int offcount2=0;
	int oncount1=0;
	int oncount2=0;

	if(access(LINEIN_PIN_PATH,F_OK) == 0 ){
		read_register(LINEIN_PIN_PATH,&ovalue);
		if(ovalue < 20){
			wtk_debug("=========>>>>>linein off<<<<<<==========\n");
		}else if(ovalue > 20){
			wtk_debug("=========>>>>>linein on<<<<<<==========\n");
		}
	}
	if(access(LINEOUT_PIN_PATH,F_OK) == 0){
		read_register(LINEOUT_PIN_PATH,&ovalue);
		if(ovalue < 20){
			wtk_debug("=========>>>>>lineout 0ff<<<<<<==========\n");
		}else if(ovalue > 20){
			wtk_debug("=========>>>>>lineout on<<<<<<==========\n");
		}
	}
	while(uc->check_run){
		read_register(LINEIN_PIN_PATH,&lvalue1);
		read_register(LINEOUT_PIN_PATH,&lvalue2);

		if(lvalue1 < 20){
			if(uc->use_linein == 1){
				offcount1++;
			}
			if(offcount1 > 2){
				if(uc->use_linein == 1){
					wtk_debug("=========>>>>>linein off<<<<<<==========\n");
					wtdebugCurrentTime();
					send_audio_status_frame(uc ,0x01 , 0x17 ,QTK_UART_STATUS_LINEIN , 0);
				}
				uc->use_linein= 0;
				offcount1 = 0;
			}
			oncount1=0;
		}else if(lvalue1 > 20){
			if(uc->use_linein == 0){
				oncount1++;
			}
			if(oncount1 > 2){
				if(uc->use_linein == 0){
					wtk_debug("=========>>>>>linein on<<<<<<==========\n");
					wtdebugCurrentTime();
					send_audio_status_frame(uc ,0x01 , 0x1B ,QTK_UART_STATUS_LINEIN , 1);
				}
				uc->use_linein = 1;
				oncount1 = 0;
			}
			offcount1=0;
		}
		if(lvalue2 <20 ){
			if(uc->use_lineout == 1){
				offcount2++;
			}
			if(offcount2 > 2){
				if(uc->use_lineout  == 1){
					wtk_debug("=========>>>>>lineout off<<<<<<==========\n");
					wtdebugCurrentTime();

					send_audio_status_frame(uc,0x01 , 0x17 ,QTK_UART_STATUS_LINEOUT, 0);
				}
				uc->use_lineout = 0;
				offcount2 = 0;
			}
			oncount2=0;
		}else if(lvalue2 > 20){
			if(uc->use_lineout == 0){
				oncount2++;
			}
			if(oncount2 > 2){
				if(uc->use_lineout == 0){
					wtk_debug("=========>>>>>lineout on<<<<<<==========\n");
					wtdebugCurrentTime();

					send_audio_status_frame(uc,0x01 , 0x1B ,QTK_UART_STATUS_LINEOUT , 1);
				}
				uc->use_lineout = 1;
				oncount2 = 0;
			}
			offcount2=0;
		}

		usleep(500*1000);
	}
	return 0 ;
}
#endif

// —— 动态解析两个AMP的eq节点（避免硬编码3-0069/3-006d写错或节点不存在）——
static char g_eq_path_main[128] = "/sys/bus/i2c/devices/3-0069/eq";
static char g_eq_path_bass[128] = "/sys/bus/i2c/devices/3-006d/eq";
// 按你的频段规则把 json 文本直接写入 EQ（低频写 0x69+0x6d，中/高只写 0x69）
// 返回 0 表示至少成功写入主EQ一次；负数表示失败。
int apply_eq_from_json_text_wtk(const char *json_in, int len_in)
{
	if (!json_in || len_in <= 0)
	{
		wtk_debug("[EQ][apply] invalid json buffer\n");
		return -1;
	}

	// A) 解析（传入的是 sanitize 后的切片就行，这里不再重复 sanitize）
	wtk_json_parser_t *jp = wtk_json_parser_new();
	if (!jp)
	{
		wtk_debug("[EQ][apply] parser_new failed\n");
		return -2;
	}
	if (wtk_json_parser_parse(jp, json_in, len_in) != 0)
	{
		wtk_debug("[EQ][apply] parser_parse failed\n");
		wtk_json_parser_delete(jp);
		return -3;
	}

	wtk_json_item_t *root = (jp && jp->json) ? jp->json->main : NULL;
	if (!root)
	{
		wtk_debug("[EQ][apply] root null\n");
		wtk_json_parser_delete(jp);
		return -4;
	}

	// B) 拿到数组：根是数组；或根是对象时找常见数组字段
	wtk_json_item_t *arr = NULL;
	if (root->type == WTK_JSON_ARRAY)
	{
		arr = root;
	}
	else if (root->type == WTK_JSON_OBJECT)
	{
		static const char *keys[] = {"bands", "eq", "data", "items", "list"};
		for (int ki = 0; ki < (int)(sizeof(keys) / sizeof(keys[0])); ++ki)
		{
			wtk_json_item_t *tmp = wtk_json_obj_get(root, keys[ki], (int)strlen(keys[ki]));
			if (tmp && tmp->type == WTK_JSON_ARRAY)
			{
				arr = tmp;
				break;
			}
		}
	}
	if (!arr || arr->type != WTK_JSON_ARRAY)
	{
		wtk_debug("[EQ][apply] root not array\n");
		wtk_json_parser_delete(jp);
		return -5;
	}

	// C) 逐条应用
	int n = wtk_json_item_len(arr);
	int wrote_main = 0, wrote_bass = 0, total = 0;

	for (int i = 0; i < n; ++i)
	{
		wtk_json_item_t *obj = wtk_json_array_get(arr, i);
		if (!obj || obj->type != WTK_JSON_OBJECT)
			continue;

		// freq / gain：数字或字符串都支持
		float freq = -1.0f, gain = 0.0f;
		wtk_json_item_t *jf = wtk_json_obj_get(obj, "freq", 4);
		if (jf)
		{
			if (jf->type == WTK_JSON_NUMBER)
				freq = (float)jf->v.number;
			else if (jf->type == WTK_JSON_STRING && jf->v.str)
				freq = (float)atof(jf->v.str->data);
		}
		wtk_json_item_t *jg = wtk_json_obj_get(obj, "gain", 4);
		if (jg)
		{
			if (jg->type == WTK_JSON_NUMBER)
				gain = (float)jg->v.number;
			else if (jg->type == WTK_JSON_STRING && jg->v.str)
				gain = (float)atof(jg->v.str->data);
		}
		if (freq < 0.0f)
		{
			wtk_debug("[EQ][apply] skip item without valid freq\n");
			continue;
		}

		int B = (freq <= 250.0f) ? 0 : ((freq >= 4000.0f) ? 2 : 1);
		int L = map_gain_to_level(gain); // 0..6 → {+6,+4,+2,0,-2,-4,-6}
		unsigned char code = (unsigned char)(((B & 0x0F) << 4) | (L & 0x0F));

		// 主 EQ（0x69）
		int ret_main = -99, ret_bass = -99;
		if (g_eq_path_main[0] != '\0')
		{
			ret_main = sysfs_write_hex_u8(g_eq_path_main, code);
			wtk_debug("[EQ][apply] main write code=0x%02X ret=%d (freq=%.1f, B=%d, L=%d)\n",
					  code, ret_main, freq, B, L);
			if (ret_main == 0)
				wrote_main++;
		}
		else
		{
			wtk_debug("[EQ][apply] main path empty, skip\n");
		}

		// 低频 → 低音 AMP（0x6d）
		if (B == 0)
		{
			if (g_eq_path_bass[0] != '\0')
			{
				ret_bass = sysfs_write_hex_u8(g_eq_path_bass, code);
				wtk_debug("[EQ][apply] bass write code=0x%02X ret=%d (freq=%.1f)\n", code, ret_bass, freq);
				if (ret_bass == 0)
					wrote_bass++;
			}
			else
			{
				wtk_debug("[EQ][apply] bass path empty, skip\n");
			}
		}

		total++;
	}

	wtk_json_parser_delete(jp);

	wtk_debug("[EQ][apply] summary: total=%d, wrote_main=%d, wrote_bass=%d\n", total, wrote_main, wrote_bass);
	return (total > 0 && wrote_main > 0) ? 0 : -6;
}

static void resolve_eq_sysfs_paths(void)
{
	// 主功放 0069
	for (int bus = 0; bus <= 7; ++bus)
	{
		char p[128];
		snprintf(p, sizeof(p), "/sys/bus/i2c/devices/%d-0069/eq", bus);
		if (access(p, W_OK) == 0)
		{
			strncpy(g_eq_path_main, p, sizeof(g_eq_path_main));
			break;
		}
	}
	// 低音功放 006d
	for (int bus = 0; bus <= 7; ++bus)
	{
		char p[128];
		snprintf(p, sizeof(p), "/sys/bus/i2c/devices/%d-006d/eq", bus);
		if (access(p, W_OK) == 0)
		{
			strncpy(g_eq_path_bass, p, sizeof(g_eq_path_bass));
			break;
		}
	}
	fprintf(stderr, "[EQ] main@%s  bass@%s\n", g_eq_path_main, g_eq_path_bass);
}
static int write_eq_bass_L_autoprobe(int L)
{
	// 先尝试 0x0L / 0x1L / 0x2L
	unsigned cand[] = {
		((0u & 0x0F) << 4) | (unsigned)(L & 0x0F),
		((1u & 0x0F) << 4) | (unsigned)(L & 0x0F),
		((2u & 0x0F) << 4) | (unsigned)(L & 0x0F),
	};
	for (int i = 0; i < 3; i++)
	{
		if (sysfs_write_hex_u8(g_eq_path_bass, cand[i]) == 0)
		{
			fprintf(stderr, "[EQ][006d] accept 0x%02X at %s\n", cand[i] & 0xFF, g_eq_path_bass);
			return 0;
		}
	}
	// 再试只写 L（十进制）
	{
		pthread_mutex_lock(&g_amp_lock);
		FILE *sf = fopen(g_eq_path_bass, "w");
		int ok = 0;
		if (sf)
		{
			if (fprintf(sf, "%d\n", (L & 0x0F)) > 0)
				ok = 1;
			fflush(sf);
			fclose(sf);
		}
		pthread_mutex_unlock(&g_amp_lock);
		if (ok)
		{
			fprintf(stderr, "[EQ][006d] accept dec L=%d at %s\n", L & 0x0F, g_eq_path_bass);
			return 0;
		}
	}
	// 再试只写 L（十六进制）
	{
		pthread_mutex_lock(&g_amp_lock);
		FILE *sf = fopen(g_eq_path_bass, "w");
		int ok = 0;
		if (sf)
		{
			if (fprintf(sf, "0x%X\n", (L & 0x0F)) > 0)
				ok = 1;
			fflush(sf);
			fclose(sf);
		}
		pthread_mutex_unlock(&g_amp_lock);
		if (ok)
		{
			fprintf(stderr, "[EQ][006d] accept hex L=0x%X at %s\n", L & 0x0F, g_eq_path_bass);
			return 0;
		}
	}
	fprintf(stderr, "[EQ][006d] all formats rejected for L=%d at %s\n", L, g_eq_path_bass);
	return -1;
}
#if 1
static int sysfs_write_hex_u8(const char *path, unsigned code_hex)
{
	pthread_mutex_lock(&g_amp_lock);
	FILE *sf = fopen(path, "w");
	if (!sf)
	{
		pthread_mutex_unlock(&g_amp_lock);
		return -1;
	}
	int n = fprintf(sf, "0x%02X\n", (unsigned)(code_hex & 0xFF)); // 一次写一行，带换行
	fflush(sf);
	fclose(sf);
	pthread_mutex_unlock(&g_amp_lock);
	usleep(4000); // 给I2C/AMP 3~5ms消化
	return (n > 0) ? 0 : -2;
}

static int write_eq_code_to_sysfs(const char *path, unsigned code_hex)
{
	for (int t = 0; t < 3; ++t)
	{
		FILE *sf = fopen(path, "w");
		if (!sf)
		{
			usleep(2000);
			continue;
		}
		// 驱动普遍接受带换行，稳一点：
		int n = fprintf(sf, "0x%02X\n", code_hex & 0xFF);
		fflush(sf);
		fclose(sf);
		if (n > 0)
			return 0;
		usleep(2000);
	}
	return -1;
}
/* 由 EQ 模式值映射到 “低频 dB” （来自Excel映射；如需后续调整，改这里即可） */
static int map_mode_to_low_db(unsigned char mode)
{
	switch (mode)
	{
	case 0x00:
		return 0; // 标准: 低0dB
	case 0x01:
		return -2; // 挂墙: 低-2dB
	case 0x02:
		return -4; // 嵌墙: 低-4dB
	default:
		return 0; // 未知就取0
	}
}
static int send_audio_status_frame(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, int type, int is_exit)
{
	qtk_uart_recv_frame_t frame;
	int len;
	// 构建帧
	wtk_debug("-------------------------------->>>>>>\n");
	frame.frame_header[0] = RESPONSE_FRAME_HEADER_0; // 0x90
	frame.frame_header[1] = RESPONSE_FRAME_HEADER_1; // 0x40
	frame.event_code[0] = event_code1;
	frame.event_code[1] = event_code2;

	wtk_debug("-------------------------------->>>>>>\n");
	cJSON *port1 = cJSON_CreateObject();
	char *upresult=file_read_buf(UART_CFG_PATH,&len);
	
	char buf[4096] = {0};
	int ret, s, is_use, is_lineinmic, pos = 0;
	char *pv;
	
	// pos += snprintf(final_json + pos, sizeof(final_json) - pos, "[\n");
	wtk_debug("-------------------------------->>>>>>\n");
	if (type == QTK_UART_STATUS_MIC)
	{
		pv = strstr(upresult, "mic_shift2=");
		s = atoi(pv + 11);
		pv = strstr(upresult, "VBOX3_MIC=");
		is_use = atoi(pv + 10);
		cJSON_AddNumberToObject(port1, "portType", 0);
		cJSON_AddNumberToObject(port1, "portId", 0xaa00002);
		cJSON_AddStringToObject(port1, "portName", "arrary_MIC");
		if (!is_use)
			cJSON_AddFalseToObject(port1, "isUse");
		else
			cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		cJSON_AddNumberToObject(port1, "audioInputType", 0);
		cJSON_AddFalseToObject(port1, "isLocalPlay");
	}
	else if (type == QTK_UART_STATUS_SPEAKER)
	{
		pv = strstr(upresult, "VBOX3_SPK=");
		is_use = atoi(pv + 10);
		read_register("/sys/bus/i2c/devices/3-0069/volume", &s);
		cJSON_AddNumberToObject(port1, "portType", 4);
		cJSON_AddNumberToObject(port1, "portId", 0xbb00001);
		cJSON_AddStringToObject(port1, "portName", "SPK");
		if (!is_use)
			cJSON_AddFalseToObject(port1, "isUse");
		else
			cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		cJSON_AddNumberToObject(port1, "audioInputType", 0);
		cJSON_AddTrueToObject(port1, "isLocalPlay");
	}
	else if (type == QTK_UART_STATUS_LINEIN)
	{
		pv = strstr(upresult, "VBOX3_LINEIN=");
		is_use = atoi(pv + 13);
		pv = strstr(upresult, "vbox3_agc_level=");
		s = atoi(pv + 16);
		pv = strstr(upresult, "USE_LINEIN_MIC=");
		is_lineinmic = atoi(pv + 15);
		cJSON_AddNumberToObject(port1, "portType", 1);
		cJSON_AddNumberToObject(port1, "portId", 0xaa00000);
		cJSON_AddStringToObject(port1, "portName", "Line_IN");
		if (!is_exit)
			cJSON_AddFalseToObject(port1, "isUse");
		else
			cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		if (!is_lineinmic)
			cJSON_AddNumberToObject(port1, "audioInputType", 0);
		else
			cJSON_AddNumberToObject(port1, "audioInputType", 1);
		cJSON_AddFalseToObject(port1, "isLocalPlay");
	}
	else if (type == QTK_UART_STATUS_LINEOUT)
	{
		// if(access("/oem/qdreamer/qsound/lineout_pattern.txt",F_OK) == 0)
		// {
		// 	pv=file_read_buf("//oem/qdreamer/qsound/lineout_pattern.txt",&ret);
		// 	is_use=atoi(pv);
		// }
		wtk_debug("-------------------------------->>>>>>\n");
		pv = strstr(upresult, "vbox3_agc_level=");
		s = atoi(pv + 16);
		wtk_debug("-------------------------------->>>>>>\n");
		cJSON_AddNumberToObject(port1, "portType", 5);
		cJSON_AddNumberToObject(port1, "portId", 0xbb00000);
		cJSON_AddStringToObject(port1, "portName", "LINE_OUT");
		if (!is_exit)
			cJSON_AddFalseToObject(port1, "isUse");
		else
			cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		cJSON_AddNumberToObject(port1, "audioInputType", 0);
		cJSON_AddFalseToObject(port1, "isLocalPlay");
		wtk_debug("-------------------------------->>>>>>\n");
	}
	wtk_debug("-------------------------------->>>>>>\n");
	char *json_str = cJSON_Print(port1);
	send_response(uc, &frame, (uint8_t *)json_str, strlen(json_str));
	wtk_debug("-------------------------------->>>>>>\n");
	if (json_str)
	{
		free(json_str);
	}
	wtk_free(upresult);
	cJSON_Delete(port1);
	wtk_debug("-------------------------------->>>>>>\n");
}

int qtk_uart_client_send_audio_status_frame(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, int type, int is_exit)
{
	qtk_uart_client_msg_t *msg;
	
	msg = qtk_uart_client_pop_msg(uc);
	msg->statID = type;
	msg->buf->data[0]=event_code1;
	msg->buf->data[1]=event_code2;
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_reset(msg->sendbuf);
	wtk_blockqueue_push(&uc->send_queue, &msg->q_n);
	// return send_audio_status_frame(uc, event_code1, event_code2, type, is_exit);
	return 0;
}

#endif
// 读取寄存器
int write_register(const char *reg_path, int value)
{
	int fd;
	int ret;
	char buf[32];

	fd = open(reg_path, O_WRONLY | O_SYNC);
	if (fd < 0)
	{
		perror("Open register for write failed");
		return -1;
	}

	snprintf(buf, sizeof(buf), "%d\n", value);
	ret = write(fd, buf, strlen(buf));
	if (ret < 0)
	{
		perror("Write register failed");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int read_register(const char *reg_path, int *output_value)
{
	int fd;
	int ret;
	char buf[32] = {0};

	fd = open(reg_path, O_RDONLY | O_SYNC);
	if (fd < 0)
	{
		perror("Open register for read failed");
		return -1;
	}

	ret = read(fd, buf, sizeof(buf) - 1);
	if (ret < 0)
	{
		perror("Read register failed");
		close(fd);
		return -1;
	}

	close(fd);
	*output_value = atoi(buf);
	return 0;
}

// 将 gain(dB) 映射到驱动的 7 档索引：0..6 => {+6,+4,+2,0,-2,-4,-6}
static int map_gain_to_level(float g_db)
{
	static const float levels[7] = {6.f, 4.f, 2.f, 0.f, -2.f, -4.f, -6.f};
	int best = 0;
	float bestd = 1e9f;
	for (int i = 0; i < 7; i++)
	{
		float d = fabsf(g_db - levels[i]);
		if (d < bestd)
		{
			bestd = d;
			best = i;
		}
	}
	return best; // 0..6
}

// 读取 /oem/qdreamer/qsound/eq.json，扫描 "gain": <float>，离散成 7 档，逐 band 写 /sys/bus/i2c/devices/3-0069/eq
int eq_apply_from_json_file_fallback(const char *json_path)
{
	// 1) 读文件到内存
	FILE *f = fopen(json_path, "rb");
	if (!f)
	{
		syslog(LOG_ERR, "[EQ][fallback] open %s fail: %s", json_path, strerror(errno));
		return -1;
	}
	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return -2;
	}
	long sz = ftell(f);
	if (sz <= 0 || sz > 64 * 1024)
	{
		fclose(f);
		return -3;
	}
	if (fseek(f, 0, SEEK_SET) != 0)
	{
		fclose(f);
		return -4;
	}
	char *buf = (char *)malloc((size_t)sz + 1);
	if (!buf)
	{
		fclose(f);
		return -5;
	}
	long rd = (long)fread(buf, 1, (size_t)sz, f);
	fclose(f);
	if (rd != sz)
	{
		free(buf);
		return -6;
	}
	buf[sz] = '\0';

	// 2) 解析 JSON（wtk_json_parser）
	wtk_json_parser_t *jp = wtk_json_parser_new();
	if (!jp)
	{
		free(buf);
		return -7;
	}
	if (wtk_json_parser_parse(jp, buf, (int)sz) != 0)
	{
		syslog(LOG_ERR, "[EQ][fallback] parse %s failed", json_path);
		wtk_json_parser_delete(jp);
		free(buf);
		return -8;
	}

	wtk_json_item_t *root = (jp && jp->json) ? jp->json->main : NULL;
	if (!root || root->type != WTK_JSON_ARRAY)
	{
		syslog(LOG_ERR, "[EQ][fallback] root not array");
		wtk_json_parser_delete(jp);
		free(buf);
		return -9;
	}

	// 3) 逐条应用：freq→B；gain→L；code=0xBL；低频再写 0x6d
	int n = wtk_json_item_len(root);
	int ok_main = 0, ok_bass = 0, total = 0;
	for (int i = 0; i < n; ++i)
	{
		wtk_json_item_t *obj = wtk_json_array_get(root, i);
		if (!obj || obj->type != WTK_JSON_OBJECT)
			continue;

		wtk_json_item_t *jf = wtk_json_obj_get(obj, "freq", 4);
		wtk_json_item_t *jg = wtk_json_obj_get(obj, "gain", 4);
		if (!jf || !jg || jf->type != WTK_JSON_NUMBER || jg->type != WTK_JSON_NUMBER)
			continue;

		float freq = (float)jf->v.number;
		float gain = (float)jg->v.number;

		int B;
		if (freq <= 250.0f)
			B = 0; // 低频：≤250 → 69 + 6d
		else if (freq >= 4000.0f)
			B = 2; // 高频：≥4000 → 69
		else
			B = 1; // 中频：251~3999 → 69

		int L = map_gain_to_level(gain); // 0..6 → {+6,+4,+2,0,-2,-4,-6}
		uint8_t code = (uint8_t)(((B & 0x0F) << 4) | (L & 0x0F));

		syslog(LOG_INFO, "[EQ][fallback] freq=%.1fHz, gain=%.1fdB -> B=%d, L=%d -> code=0x%02X",
			   freq, gain, B, L, code);

		// 主 EQ（0x69）必写
		if (strlen(g_eq_path_main) > 0)
		{
			if (sysfs_write_hex_u8(g_eq_path_main, code) == 0)
			{
				ok_main++;
			}
			else
			{
				syslog(LOG_ERR, "[EQ][fallback] write 0x%02X to %s failed", code, g_eq_path_main);
			}
		}

		// 低频额外写 低音 AMP（0x6d）
		if (B == 0 && strlen(g_eq_path_bass) > 0)
		{
			if (sysfs_write_hex_u8(g_eq_path_bass, code) == 0)
			{
				ok_bass++;
			}
			else
			{
				syslog(LOG_ERR, "[EQ][fallback] write 0x%02X to %s failed", code, g_eq_path_bass);
			}
		}
		total++;
	}

	wtk_json_parser_delete(jp);
	free(buf);

	syslog(LOG_INFO, "[EQ][fallback] done: total=%d, ok_main=%d, ok_bass=%d", total, ok_main, ok_bass);
	return (total > 0 && ok_main > 0) ? 0 : -10;
}

/* 载荷净化：去掉 UTF‑8 BOM、可疑的 4 字节 u32 头（LE/BE），然后扫描到 '[' 或 '{' */
void sanitize_json_payload(const uint8_t *in, size_t in_len,
								  const char **out_json, size_t *out_len)
{
	const unsigned char *p = in;
	size_t n = in_len;

	/* 跳过 UTF-8 BOM: EF BB BF */
	if (n >= 3 && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF)
	{
		p += 3;
		n -= 3;
	}

	/* 尝试剥掉前置的 u32 头（例如 03 00 00 00 或 00 00 00 03） */
	if (n >= 4)
	{
		uint32_t le = (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
		uint32_t be = (uint32_t)p[3] | ((uint32_t)p[2] << 8) | ((uint32_t)p[1] << 16) | ((uint32_t)p[0] << 24);
		/* 经验规则：若该数值在 1..64 之间，且后续出现 JSON 起始，很可能是“误加计数头” */
		if ((le >= 1 && le <= 64) || (be >= 1 && be <= 64))
		{
			const unsigned char *q = p + 4;
			size_t r = n - 4;
			while (r > 0 && q[0] <= 0x20)
			{
				q++;
				r--;
			} // 跳过空白
			if (r > 0 && (q[0] == '[' || q[0] == '{'))
			{ // 符合预期才剥头
				p += 4;
				n -= 4;
			}
		}
	}

	/* 最终保险：一路扫到真正的 JSON 起点 */
	while (n > 0 && p[0] != '[' && p[0] != '{')
	{
		p++;
		n--;
	}

	*out_json = (const char *)p;
	*out_len = n;
}
/* 最小文件写入封装（原工程没有 qtk_write_file，这里补一个） */
int qtk_write_file(const char *path, const void *buf, size_t len)
{
	FILE *f = fopen(path, "wb");
	if (!f)
	{
		return -1;
	}
	size_t w = fwrite(buf, 1, len, f);
	fflush(f);
	int fd = fileno(f);
	if (fd >= 0)
	{
		fsync(fd);
	}
	fclose(f);
	chmod(path, 0666);
	return (w == len) ? 0 : -2;
}
void ackq_push(qtk_uart_client_t *uc, uint16_t code, uint8_t status)
{
	qtk_ack_node_t *n = (qtk_ack_node_t *)wtk_malloc(sizeof(qtk_ack_node_t));
	if (!n)
		return;
	n->code = code;
	n->status = status;
	wtk_blockqueue_push(&uc->ack_q, &n->q_n);
}
/* 等待指定事件码的ACK（兼容 0xXXXX 与 0x8XXX），超时返回-1 */
int wait_for_ack(qtk_uart_client_t *uc, uint16_t expect_code, int timeout_ms, uint8_t *status_out)
{
	uint64_t deadline_ms = time_get_ms() + (timeout_ms > 0 ? (uint64_t)timeout_ms : 0);
	for (;;)
	{
		int remain = (timeout_ms > 0) ? (int)(deadline_ms - time_get_ms()) : -1; // -1 表示一直等
		if (timeout_ms > 0 && remain <= 0)
			return -1;

		wtk_queue_node_t *qn = wtk_blockqueue_pop(&uc->ack_q, (timeout_ms > 0 ? remain : -1), NULL);
		if (!qn)
		{ // 超时
			return -1;
		}
		qtk_ack_node_t *an = data_offset2(qn, qtk_ack_node_t, q_n);

		int match = 0;
		if (an->code == expect_code)
			match = 1;
		else if (an->code == (expect_code | 0x8000))
			match = 1; // 兼容 ACK=0x8xxx 的写法

		if (match)
		{
			if (status_out)
				*status_out = an->status;
			wtk_free(an);
			return 0; // 命中ACK
		}
		else
		{
			// 不是我想要的ACK，丢弃（或按需缓存）；为简洁起见，直接丢弃
			wtk_free(an);
			// 继续等
		}
	}
}

void msleep(unsigned long mSec)
{
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = mSec;
	select(0, NULL, NULL, NULL, &tv);
}

qtk_uart_client_t *qtk_uart_client_new(qtk_uart_client_cfg_t *cfg, wtk_log_t *log)
{
	wtdebugCurrentTime();

	qtk_uart_client_t *uc;
	char *data = NULL;
	int len;
	int ret;
	char tmpbuf[1024] = {0};
	char *pc;
	int s;

	float scale;
	// int volume;
	// char set_buf[128]={0};
	signal(SIGPIPE, SIG_IGN);
	uc = (qtk_uart_client_t *)wtk_malloc(sizeof(qtk_uart_client_t));
	memset(uc, 0, sizeof(qtk_uart_client_t));
	lc = (qtk_uart_client_t *)wtk_malloc(sizeof(qtk_uart_client_t));
	memset(lc, 0, sizeof(qtk_uart_client_t));
	uc->cfg = cfg;
	uc->log = log;
	uc->version = wtk_strbuf_new(64, 1);
	uc->utype = QTK_UART_STATE_RECV_START;
	uc->parser = wtk_json_parser_new();
	uc->msg_run = 0;
	uc->trsn_run = 0;
	uc->send_run = 0;
	uc->check_run = 0;
	uc->spk_alarm = 0;
	uc->report_run = 0;
	uc->report_busy = 0;
	uc->upload_round = 0;
	uc->package_seq = 0;
	uc->report_trigger_once = 0;
	uc->lineout_pattern = 0;
	resolve_eq_sysfs_paths();
	uc->responsebuf = wtk_strbuf_new(1024, 1.0);
	uc->crcbuf = wtk_strbuf_new(1024, 1.0);

#ifdef USE_LOG
	uc->log = wtk_log_new("/data/qtk_uart_client.log");
#endif

	wtk_blockqueue_init(&uc->input_q);
	wtk_blockqueue_init(&uc->msg_q);
	wtk_lockhoard_init(&uc->msg_hoard, offsetof(qtk_uart_client_msg_t, hoard_n), 10,
					   (wtk_new_handler_t)qtk_uart_client_msg_new,
					   (wtk_delete_handler_t)qtk_uart_client_msg_delete,
					   uc);
	wtk_thread_init(&uc->trsn_thread, (thread_route_handler)qtk_uart_client_trsn_run, (void *)uc);
	wtk_thread_set_name(&uc->trsn_thread, "trsn");

	// wtk_thread_init(&uc->check_thread,(thread_route_handler)qtk_uart_linein_lineout_check_entry,(void*)uc);
	// wtk_thread_set_name(&uc->check_thread,"check");

	wtk_blockqueue_init(&uc->send_queue);
	wtk_thread_init(&uc->send_thread,(thread_route_handler)qtk_uart_client_send_run,(void*)uc);
	wtk_thread_set_name(&uc->send_thread, "send");

	wtk_thread_init(&uc->msg_thread, (thread_route_handler)qtk_uart_client_msg_run, (void *)uc);
	wtk_thread_set_name(&uc->msg_thread, "msg");
	// ACK队列初始化
	wtk_blockqueue_init(&uc->ack_q);

	if(access("/oem/qdreamer/qsound/lineout_pattern.txt", F_OK) == 0 && access("/oem/qdreamer/qsound/lineout_pattern.txt", W_OK) == 0)
	{
		char *upresult=file_read_buf("/oem/qdreamer/qsound/lineout_pattern.txt",&len);
		uc->lineout_pattern = atoi(upresult);
		wtk_free(upresult);
	}
	wtk_debug("===========================+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
	if(access("/oem/qdreamer/qsound/spk_volume.txt", F_OK) == 0 && access("/oem/qdreamer/qsound/spk_volume.txt", W_OK) == 0)
	{
		char *upresult=file_read_buf("/oem/qdreamer/qsound/spk_volume.txt",&len);
		uc->spk_volume = atoi(upresult);
		wtk_debug("---------------->>>>>>>>>>>>>spk_volume = %d\n",uc->spk_volume);
		wtk_free(upresult);
	}
	wtk_debug("===========================+>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");

	if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0)
	{
		char *upresult=file_read_buf(UART_CFG_PATH,&len);

		pc = strstr(upresult, "mic_shift2=");
		scale = wtk_str_atof(pc + 11, ret - (pc - upresult) - 12);

		float mic_shift_log = logf(MIC_SHIFT2); 
		float data4_value;

		if (fabsf(scale - 1.0f) < 0.0001f) {  // 处理浮点数精度
			data4_value = 50.0f;
		} else if (fabsf(scale) < 0.0001f) {  // 处理浮点数精度
			data4_value = 0.0f;
		} else if (scale < 1.0f) {
			data4_value = 50.0f - logf(1.0f / scale) / mic_shift_log;
		} else {
			data4_value = 50.0f + logf(scale) / mic_shift_log;
		}
		if (data4_value < 0.0f) data4_value = 0.0f;
		if (data4_value > 100.0f) data4_value = 100.0f;
		s = roundf(data4_value);
		uc->mic_shift2 = s;

		wtk_free(upresult);
	}
	if (uc->cfg->use_uart){
		int i = 0;
		while (i < 10){
			lc->uart = qtk_uart_new(&(uc->cfg->uart));
			uc->uart = qtk_uart_new(&(uc->cfg->uart));
			if (uc->uart && lc->uart){
				break;
			}
			if (!uc->uart && lc->uart){
				sleep(1);
			}
			i++;
		}
		if (!uc->uart){
			wtk_debug("upload new failed.\n");
			wtk_log_warn0(uc->log, "upload new failed.");
			ret = -1;
			goto end;
		}

		uc->uart_buf = wtk_strbuf_new(3200, 1);
		uc->uart_buf2 = wtk_strbuf_new(256, 1);
	}
	uc->mac = wtk_strbuf_new(256, 0);

	wtk_debug("================>>>>>>>>>new ok!\n");
	ret = 0;
end:
	if (ret != 0)
	{
		qtk_uart_client_delete(uc);
		uc = NULL;
	}
	return uc;
}

int qtk_uart_client_delete(qtk_uart_client_t *uc)
{
	wtk_debug("uc delete.\n");
	wtk_lockhoard_clean(&uc->msg_hoard);

	wtk_blockqueue_clean(&uc->send_queue);
	wtk_blockqueue_clean(&uc->msg_q);
	wtk_blockqueue_clean(&uc->input_q);
	wtk_blockqueue_clean(&uc->ack_q);

	if (uc->uart)
	{
		qtk_uart_delete(uc->uart);
	}

	if (uc->uart_buf)
	{
		wtk_strbuf_delete(uc->uart_buf);
	}
	if (uc->uart_buf2)
	{
		wtk_strbuf_delete(uc->uart_buf2);
	}

	if (uc->version)
	{
		wtk_strbuf_delete(uc->version);
	}
	if (uc->mac)
	{
		wtk_strbuf_delete(uc->mac);
	}
	if (uc->responsebuf)
	{
		wtk_strbuf_delete(uc->responsebuf);
	}
	if (uc->crcbuf)
	{
		wtk_strbuf_delete(uc->crcbuf);
	}
	if (uc->parser)
	{
		wtk_json_parser_delete(uc->parser);
	}

	if (lc){
		if (lc->uart){
			qtk_uart_delete(lc->uart);
			lc->uart = NULL;
		}
		wtk_free(lc);
		lc = NULL;
	}
	wtk_free(uc);
	return 0;
}

void qtk_uart_client_set_notify(qtk_uart_client_t *fixbeam, void *this, qtk_uart_recv_notify_f notify)
{
	fixbeam->recv_notify = notify;
	fixbeam->this = this;
}

int qtk_uart_client_start(qtk_uart_client_t *uc)
{
	if (0 == uc->msg_run)
	{
		uc->msg_run = 1;
		wtk_thread_start(&uc->msg_thread);
	}

	if (0 == uc->trsn_run)
	{
		uc->trsn_run = 1;
		wtk_thread_start(&uc->trsn_thread);
	}
#if 0
	 if(0 == uc->check_run)
    {
    	uc->check_run = 1;
        wtk_thread_start(&uc->check_thread);
    }
#endif
	// 启动日志上报线程
	if (0 == uc->report_run)
	{
		uc->report_trigger_once = 0;
		uc->report_run = 1;
		wtk_thread_init(&uc->report_thread, (thread_route_handler)qtk_uart_client_report_run, (void *)uc);
		wtk_thread_set_name(&uc->report_thread, "report");
		wtk_thread_start(&uc->report_thread);
	}
	wtk_debug("------------------send_active_response 0x0126\n");
	send_active_response(lc,0x01,0x26,NULL,0);
	if(uc->send_run == 0){
		uc->send_run = 1;
		wtk_thread_start(&uc->send_thread);
	}
	wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
	return 0;
}

int qtk_uart_client_stop(qtk_uart_client_t *uc)
{
	if (1 == uc->trsn_run)
	{
		uc->trsn_run = 0;
		wtk_blockqueue_wake(&uc->input_q);
		wtk_thread_join(&uc->trsn_thread);
		qtk_uart_client_clean_q(uc, &uc->input_q);
	}
	if (1 == uc->msg_run)
	{
		uc->msg_run = 0;
		wtk_blockqueue_wake(&uc->msg_q);
		// wtk_thread_join(&uc->msg_hoard);
		wtk_thread_join(&uc->msg_thread);
		qtk_uart_client_clean_q(uc, &uc->msg_q);
	}
	if(uc->send_run == 1){
		uc->send_run = 0;
		wtk_blockqueue_wake(&uc->send_queue);
		wtk_thread_join(&uc->send_thread);
		qtk_uart_client_clean_q(uc, &uc->send_queue);
	}
#if 0
	if(1 == uc->check_run)
	{
		uc->check_run = 0;
        wtk_thread_join(&uc->check_thread);

	}
#endif
	// 停止日志上报线程
	if (1 == uc->report_run)
	{
		uc->report_run = 0;
		wtk_thread_join(&uc->report_thread);
	}

	wtk_queue_node_t *qn;
	while ((qn = wtk_blockqueue_pop(&uc->ack_q, 0, NULL)) != NULL)
	{
		qtk_ack_node_t *an = data_offset2(qn, qtk_ack_node_t, q_n);
		wtk_free(an);
	}

	return 0;
}

int qtk_uart_client_msg_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
{
	qtk_uart_set_cpu(uc, thread, 2);
	qtk_uart_client_msg_t *msg;
	wtk_queue_node_t *qn;

	while (uc->msg_run)
	{
		qn = wtk_blockqueue_pop(&uc->msg_q, -1, NULL);
		if (!qn){continue;}
		msg = data_offset2(qn, qtk_uart_client_msg_t, q_n);

		if(uc->recv_notify){
			uc->recv_notify(uc->this, (qtk_uart_recv_frame_t *)msg->buf->data, msg->buf->pos);
		}

		if (msg){
			qtk_uart_client_push_msg(uc, msg);
		}
	}
}

int qtk_uart_client_send_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
{
	qtk_uart_set_cpu(uc, thread, 2);
	qtk_uart_client_msg_t *msg;
	wtk_queue_node_t *qn;

	while (uc->send_run)
	{
		qn = wtk_blockqueue_pop(&uc->send_queue, -1, NULL);
		if (!qn){continue;}
		msg = data_offset2(qn, qtk_uart_client_msg_t, q_n);

		switch (msg->statID)
		{
		case QTK_UART_SEND_STATE_LINEIN_ON:
			send_audio_status_frame(lc, 0x01, 0x1B, QTK_UART_STATUS_LINEIN, 1);
			break;
		case QTK_UART_SEND_STATE_LINEIN_OFF:
			send_audio_status_frame(lc, 0x01, 0x17, QTK_UART_STATUS_LINEIN, 0);
			break;
		case QTK_UART_SEND_STATE_LINEOUT_ON:
			send_audio_status_frame(lc, 0x01, 0x1B, QTK_UART_STATUS_LINEOUT, 1);
			break;
		case QTK_UART_SEND_STATE_LINEOUT_OFF:
			send_audio_status_frame(lc, 0x01, 0x17, QTK_UART_STATUS_LINEOUT, 0);
			break;
		case QTK_UART_SEND_STATE_NORMAL:
			send_response(uc, (qtk_uart_recv_frame_t *)(msg->buf->data), (uint8_t *)(msg->sendbuf->data), msg->sendbuf->pos);
			break;
		case QTK_UART_SEND_STATE_ACTIVE:
			send_active_response(uc, msg->buf->data[0], msg->buf->data[1], (uint8_t *)(msg->sendbuf->data), msg->sendbuf->pos);
			break;
		case QTK_UART_SEND_STATE_ACTIVE2:
			qtk_uart_client_send_active(uc, msg->buf->data[0], (uint8_t *)(msg->sendbuf->data), msg->sendbuf->pos);
			break;
		default:
			break;
		}

		if (msg){
			qtk_uart_client_push_msg(uc, msg);
		}
	}
}

// 新增：主动上报打帧工具（设备 → 主机的“主动”帧）
// 沿用你现有响应帧头 `0x90 0x40` 作为设备上行帧头，CRC 使用你现有的 `calculateModbusCRC()`。
static int qtk_uart_client_send_active(qtk_uart_client_t *uc, uint16_t event_code, const uint8_t *data, uint16_t data_len)
{
	uint8_t hdr0 = REQUEST_FRAME_HEADER_0;										// 0x81
	uint8_t hdr1 = REQUEST_FRAME_HEADER_1;										// 0xAB
	uint8_t ev[2] = {(uint8_t)(event_code >> 8), (uint8_t)(event_code & 0xFF)}; // 大端
	uint8_t len2[2] = {(uint8_t)(data_len & 0xFF), (uint8_t)(data_len >> 8)};
	uint8_t csum[2] = {0, 0};

	int crc_data_len = 4 + data_len; // event(2)+len(2)+data
	uint8_t *crc_data = (uint8_t *)malloc(crc_data_len);
	if (!crc_data)
		return -1;
	memcpy(crc_data, ev, 2);
	memcpy(crc_data + 2, len2, 2);
	if (data_len && data)
		memcpy(crc_data + 4, data, data_len);
	uint16_t crc = calculateModbusCRC(crc_data, crc_data_len);
	free(crc_data);
	csum[0] = (uint8_t)(crc & 0xFF);
	csum[1] = (uint8_t)(crc >> 8);

	int frame_len = 2 + 2 + 2 + data_len + 2 + 1; // 9 + data_len
	uint8_t *send_buf = (uint8_t *)malloc(frame_len);
	if (!send_buf)
		return -2;

	int pos = 0;
	send_buf[pos++] = hdr0;
	send_buf[pos++] = hdr1;
	send_buf[pos++] = ev[0];
	send_buf[pos++] = ev[1];
	send_buf[pos++] = len2[0];
	send_buf[pos++] = len2[1];
	if (data_len && data)
	{
		memcpy(send_buf + pos, data, data_len);
		pos += data_len;
	}
	send_buf[pos++] = csum[0];
	send_buf[pos++] = csum[1];
	send_buf[pos++] = FRAME_FOOTER;
	UART_LOG_DBG(uc, "send active evt=0x%04X payload=%u", event_code, data_len);
	log_hex_buffer(uc, "tx active payload", data, data_len);
	log_hex_buffer(uc, "tx active frame", send_buf, frame_len);
	int ret = qtk_uart_write2(uc->uart, (char *)send_buf, pos);
	free(send_buf);
	return (ret == pos) ? 0 : -3;
}
// 分包发送：开始包 / 数据包 / 结束包
static void le16_write(uint8_t *p, uint16_t v)
{
	p[0] = v & 0xFF;
	p[1] = (v >> 8) & 0xFF;
}
static void le32_write(uint8_t *p, uint32_t v)
{
	p[0] = v & 0xFF;
	p[1] = (v >> 8) & 0xFF;
	p[2] = (v >> 16) & 0xFF;
	p[3] = (v >> 24) & 0xFF;
}
static void le64_write(uint8_t *p, uint64_t v)
{
	for (int i = 0; i < 8; i++)
		p[i] = (uint8_t)(v >> (8 * i));
}
static int send_file_in_chunks(qtk_uart_client_t *uc, const char *file_path)
{
	FILE *f = fopen(file_path, "rb");
	if (!f)
		return -1;

	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return -2;
	}
	long long sz = ftell(f);
	if (sz < 0)
	{
		fclose(f);
		return -3;
	}
	if (fseek(f, 0, SEEK_SET) != 0)
	{
		fclose(f);
		return -4;
	}
	uint32_t total_bytes = (uint32_t)sz; // 协议开始包用 4 字节；>4GB 的情况此处不支持
	uint16_t total_chunks = (uint16_t)((sz + LOG_DATA_MAX_PER_FRAME - 1) / LOG_DATA_MAX_PER_FRAME);
	// 1) 开始包： [4B 总大小][2B 总包数]
	uint8_t start_payload[6];
	le32_write(start_payload, total_bytes);
	le16_write(start_payload + 4, total_chunks);

	int start_ok = 0;
	for (int tries = 0; tries < 3 && !start_ok; ++tries)
	{
		if (qtk_uart_client_send_active(uc, EVT_LOG_UPLOAD_START, start_payload, sizeof(start_payload)) != 0)
		{
			if (uc && uc->log)
				wtk_log_warn0(uc->log, "[report] start frame send fail");
			usleep(200 * 1000);
			continue;
		}

		uint8_t st = 0xFF;
		if (wait_for_ack(uc, EVT_LOG_UPLOAD_START, 2000, &st) == 0 && st == 0x00)
		{
			start_ok = 1; // 收到OK
		}
		else
		{
			if (uc && uc->log)
				wtk_log_warn0(uc->log, "[report] wait start-ack timeout or not OK, retry");
		}
	}

	if (!start_ok)
	{
		if (uc && uc->log)
			wtk_log_warn0(uc->log, "[report] abort: no start-ack");
		fclose(f);
		return -5; // 中断上传
	}
	// 2) 数据包： [2B 序号][数据]
	uint8_t *buf = (uint8_t *)malloc(2 + LOG_DATA_MAX_PER_FRAME);
	if (!buf)
	{
		fclose(f);
		return -6;
	}
	uint16_t seq = 0;
	while (seq < total_chunks)
	{
		size_t to_read = LOG_DATA_MAX_PER_FRAME;
		size_t n = fread(buf + 2, 1, to_read, f);
		if (n == 0 && ferror(f))
		{
			free(buf);
			fclose(f);
			return -7;
		}
		le16_write(buf, seq);
		if (qtk_uart_client_send_active(uc, EVT_LOG_UPLOAD_DATA, buf, (uint16_t)(2 + n)) != 0)
		{
			free(buf);
			fclose(f);
			return -8;
		}
		seq++;
		if (n < to_read)
			break; // 文件结束
		usleep(LOG_FRAME_GAP_US);
	}
	free(buf);
	fclose(f);
	// 3) 结束包（空载荷）
	if (qtk_uart_client_send_active(uc, EVT_LOG_UPLOAD_END, NULL, 0) != 0)
	{
		return -9;
	}
	if (uc && uc->log)
		wtk_log_log0(uc->log, "[report] upload OK");
	return 0;
}

typedef struct
{
	char name[NAME_MAX + 1];
	off_t size;
	time_t mtime;
} log_file_entry_t;

#define PKG_NAME_PREFIX "ulog_pkg_"
#define PKG_NAME_SUFFIX ".tar.gz"
#define STAGE_PREFIX "/tmp/ulog_stage_"

static void build_stage_path(const char *pkg_id, char *buf, size_t buf_sz)
{
	snprintf(buf, buf_sz, STAGE_PREFIX "%s", pkg_id);
}

static int move_file_crossfs(const char *src, const char *dst)
{
	if (rename(src, dst) == 0)
	{
		return 0;
	}
	if (errno != EXDEV)
	{
		return -1;
	}

	int in_fd = open(src, O_RDONLY);
	if (in_fd < 0)
	{
		return -1;
	}
	int out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (out_fd < 0)
	{
		close(in_fd);
		return -1;
	}

	uint8_t buf[4096];
	int ret = 0;
	for (;;)
	{
		ssize_t n = read(in_fd, buf, sizeof(buf));
		if (n == 0)
			break;
		if (n < 0)
		{
			ret = -1;
			break;
		}
		ssize_t off = 0;
		while (off < n)
		{
			ssize_t w = write(out_fd, buf + off, (size_t)n - (size_t)off);
			if (w <= 0)
			{
				ret = -1;
				goto copy_end;
			}
			off += w;
		}
	}
copy_end:
	if (ret == 0)
	{
		fsync(out_fd);
	}
	close(out_fd);
	close(in_fd);
	if (ret == 0)
	{
		if (unlink(src) != 0)
		{
			ret = -1;
		}
	}
	if (ret != 0)
	{
		unlink(dst);
	}
	return ret;
}

static int stage_path_from_package(const char *pkg_path, char *buf, size_t buf_sz)
{
	const char *base = strrchr(pkg_path, '/');
	base = base ? (base + 1) : pkg_path;
	size_t prefix_len = strlen(PKG_NAME_PREFIX);
	size_t suffix_len = strlen(PKG_NAME_SUFFIX);
	size_t base_len = strlen(base);
	if (base_len <= prefix_len + suffix_len)
		return -1;
	if (strncmp(base, PKG_NAME_PREFIX, prefix_len) != 0)
		return -1;
	if (strcmp(base + base_len - suffix_len, PKG_NAME_SUFFIX) != 0)
		return -1;
	size_t id_len = base_len - prefix_len - suffix_len;
	char id[128];
	if (id_len >= sizeof(id))
		return -1;
	memcpy(id, base + prefix_len, id_len);
	id[id_len] = '\0';
	build_stage_path(id, buf, buf_sz);
	return 0;
}

static int is_targz_file(const char *name)
{
	const char *suffix = ".tar.gz";
	size_t len = strlen(name);
	size_t sfx = strlen(suffix);
	return (len >= sfx && strcmp(name + len - sfx, suffix) == 0);
}

static int ensure_watch_directory(void)
{
	char path[PATH_MAX];
	size_t len;

	if (!LOG_WATCH_DIR || LOG_WATCH_DIR[0] == '\0')
		return -1;
	snprintf(path, sizeof(path), "%s", LOG_WATCH_DIR);
	len = strlen(path);
	if (len == 0 || len >= sizeof(path))
		return -1;

	for (char *p = path + 1; *p; ++p)
	{
		if (*p == '/')
		{
			*p = '\0';
			if (path[0] != '\0')
			{
				if (mkdir(path, 0775) != 0 && errno != EEXIST)
				{
					*p = '/';
					return -1;
				}
			}
			*p = '/';
		}
	}
	if (mkdir(path, 0775) != 0 && errno != EEXIST)
		return -1;
	return 0;
}

static void cleanup_directory(const char *path)
{
	DIR *dir = opendir(path);
	if (!dir)
	{
		rmdir(path);
		return;
	}
	struct dirent *ent;
	char full[PATH_MAX];
	while ((ent = readdir(dir)) != NULL)
	{
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;
		snprintf(full, sizeof(full), "%s/%s", path, ent->d_name);
		unlink(full);
	}
	closedir(dir);
	rmdir(path);
}

static int create_tar_from_dir(qtk_uart_client_t *uc, const char *src_dir, const char *dest_path)
{
	char cmd[1024];
	snprintf(cmd, sizeof(cmd), "tar -czf '%s' -C '%s' . 2>/dev/null", dest_path, src_dir);
	int ret = system(cmd);
	if (ret != 0)
	{
		if (uc && uc->log)
			wtk_log_warn0(uc->log, "[report] tar build failed");
		unlink(dest_path);
		return -1;
	}
	chmod(dest_path, 0666);
	return 0;
}

static int compare_by_mtime(const void *a, const void *b)
{
	const log_file_entry_t *fa = (const log_file_entry_t *)a;
	const log_file_entry_t *fb = (const log_file_entry_t *)b;
	if (fa->mtime == fb->mtime)
		return strcmp(fa->name, fb->name);
	return (fa->mtime < fb->mtime) ? -1 : 1;
}

static int append_entry(log_file_entry_t **arr, size_t *count, size_t *cap, const log_file_entry_t *entry)
{
	if (*count >= *cap)
	{
		size_t new_cap = (*cap == 0) ? 8 : (*cap * 2);
		log_file_entry_t *tmp = (log_file_entry_t *)realloc(*arr, new_cap * sizeof(log_file_entry_t));
		if (!tmp)
			return -1;
		*arr = tmp;
		*cap = new_cap;
	}
	(*arr)[*count] = *entry;
	(*count)++;
	return 0;
}

static int gather_watch_files(log_file_entry_t **raw_out, size_t *raw_count,
							  log_file_entry_t **pkg_out, size_t *pkg_count)
{
	DIR *dir = NULL;
	struct dirent *ent;
	struct stat st;
	char full[PATH_MAX];
	size_t raw_cap = 0;
	size_t pkg_cap = 0;

	*raw_out = NULL;
	*pkg_out = NULL;
	*raw_count = 0;
	*pkg_count = 0;

	if (ensure_watch_directory() != 0)
		return -1;

	dir = opendir(LOG_WATCH_DIR);
	if (!dir)
		return -1;

	while ((ent = readdir(dir)) != NULL)
	{
		if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0)
			continue;
		snprintf(full, sizeof(full), "%s/%s", LOG_WATCH_DIR, ent->d_name);
		if (lstat(full, &st) != 0 || !S_ISREG(st.st_mode))
			continue;

		log_file_entry_t entry;
		memset(&entry, 0, sizeof(entry));
		strncpy(entry.name, ent->d_name, sizeof(entry.name) - 1);
		entry.size = st.st_size;
		entry.mtime = st.st_mtime;

		if (is_targz_file(entry.name))
		{
			if (append_entry(pkg_out, pkg_count, &pkg_cap, &entry) != 0)
				goto fail;
		}
		else
		{
			if (append_entry(raw_out, raw_count, &raw_cap, &entry) != 0)
				goto fail;
		}
	}
	closedir(dir);
	return 0;
fail:
	if (dir)
		closedir(dir);
	free(*raw_out);
	free(*pkg_out);
	*raw_out = NULL;
	*pkg_out = NULL;
	*raw_count = 0;
	*pkg_count = 0;
	return -1;
}

static int package_fileset(qtk_uart_client_t *uc, log_file_entry_t *files, size_t file_count,
						   const char *pkg_id, char dest_path[], size_t dest_sz)
{
	if (file_count == 0)
		return 0;

	char stage_dir[PATH_MAX];
	build_stage_path(pkg_id, stage_dir, sizeof(stage_dir));
	cleanup_directory(stage_dir);
	if (mkdir(stage_dir, 0775) != 0)
	{
		UART_LOG_WARN(uc, "[report] mkdir %s failed: %s", stage_dir, strerror(errno));
		return -1;
	}

	size_t moved = 0;
	for (; moved < file_count; ++moved)
	{
		char src[PATH_MAX];
		char dst[PATH_MAX];
		snprintf(src, sizeof(src), "%s/%s", LOG_WATCH_DIR, files[moved].name);
		snprintf(dst, sizeof(dst), "%s/%s", stage_dir, files[moved].name);
		if (move_file_crossfs(src, dst) != 0)
		{
			UART_LOG_WARN(uc, "[report] move %s failed: %s", files[moved].name, strerror(errno));
			for (size_t j = 0; j < moved; ++j)
			{
				char back_src[PATH_MAX];
				char back_dst[PATH_MAX];
				snprintf(back_src, sizeof(back_src), "%s/%s", stage_dir, files[j].name);
				snprintf(back_dst, sizeof(back_dst), "%s/%s", LOG_WATCH_DIR, files[j].name);
				move_file_crossfs(back_src, back_dst);
			}
			cleanup_directory(stage_dir);
			return -1;
		}
	}

	if (!dest_path || dest_sz == 0)
	{
		UART_LOG_WARN(uc, "[report] invalid dest path buffer");
		for (size_t j = 0; j < file_count; ++j)
		{
			char back_src[PATH_MAX];
			char back_dst[PATH_MAX];
			snprintf(back_src, sizeof(back_src), "%s/%s", stage_dir, files[j].name);
			snprintf(back_dst, sizeof(back_dst), "%s/%s", LOG_WATCH_DIR, files[j].name);
			move_file_crossfs(back_src, back_dst);
		}
		cleanup_directory(stage_dir);
		return -1;
	}

	if (create_tar_from_dir(uc, stage_dir, dest_path) != 0)
	{
		for (size_t j = 0; j < file_count; ++j)
		{
			char back_src[PATH_MAX];
			char back_dst[PATH_MAX];
			snprintf(back_src, sizeof(back_src), "%s/%s", stage_dir, files[j].name);
			snprintf(back_dst, sizeof(back_dst), "%s/%s", LOG_WATCH_DIR, files[j].name);
			move_file_crossfs(back_src, back_dst);
		}
		cleanup_directory(stage_dir);
		return -1;
	}

	UART_LOG_DBG(uc, "[report] packaged %zu files into %s", file_count, dest_path);
	return 0;
}

static int package_pending_logs(qtk_uart_client_t *uc, int force_now)
{
	log_file_entry_t *raw = NULL;
	log_file_entry_t *pkgs = NULL;
	size_t raw_count = 0;
	size_t pkg_count = 0;
	if (gather_watch_files(&raw, &raw_count, &pkgs, &pkg_count) != 0)
		return -1;
	free(pkgs);
	if (raw_count == 0)
	{
		free(raw);
		return 0;
	}

	uint64_t total_raw_bytes = 0;
	for (size_t i = 0; i < raw_count; ++i)
	{
		total_raw_bytes += (uint64_t)raw[i].size;
	}
	if (!force_now && total_raw_bytes < LOG_PACKAGE_THRESHOLD_BYTES)
	{
		free(raw);
		return 0;
	}

	qsort(raw, raw_count, sizeof(log_file_entry_t), compare_by_mtime);
	size_t idx = 0;
	while (idx < raw_count)
	{
		size_t batch_end = idx;
		uint64_t batch_bytes = 0;
		while (batch_end < raw_count)
		{
			uint64_t next = (uint64_t)raw[batch_end].size;
			if (batch_bytes > 0 && batch_bytes + next > LOG_PACKAGE_THRESHOLD_BYTES)
				break;
			batch_bytes += next;
			batch_end++;
			if (batch_bytes >= LOG_PACKAGE_THRESHOLD_BYTES)
				break;
		}
		if (batch_end == idx)
		{
			batch_end = idx + 1;
		}
		time_t t = time(NULL);
		struct tm tmv;
		localtime_r(&t, &tmv);
		uint32_t seq = ++uc->package_seq;
		char pkg_id[64];
		snprintf(pkg_id, sizeof(pkg_id),
				 "%04d%02d%02d_%02d%02d%02d_%06u",
				 tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
				 tmv.tm_hour, tmv.tm_min, tmv.tm_sec, seq);
		char dest_path[PATH_MAX];
		snprintf(dest_path, sizeof(dest_path), "%s/%s%s%s",
				 LOG_WATCH_DIR, PKG_NAME_PREFIX, pkg_id, PKG_NAME_SUFFIX);
		if (package_fileset(uc, raw + idx, batch_end - idx, pkg_id, dest_path, sizeof(dest_path)) != 0)
		{
			break;
		}
		struct stat st;
		if (stat(dest_path, &st) == 0 && (uint64_t)st.st_size > LOG_PACKAGE_MAX_BYTES)
		{
			UART_LOG_WARN(uc, "[report] package %s size %lld > limit %u", dest_path,
						  (long long)st.st_size, (unsigned)LOG_PACKAGE_MAX_BYTES);
		}
		idx = batch_end;
	}
	free(raw);
	return 0;
}

static int enforce_package_queue_limit(qtk_uart_client_t *uc)
{
	log_file_entry_t *raw = NULL;
	log_file_entry_t *pkgs = NULL;
	size_t raw_count = 0;
	size_t pkg_count = 0;
	if (gather_watch_files(&raw, &raw_count, &pkgs, &pkg_count) != 0)
		return -1;
	free(raw);
	if (pkg_count <= LOG_PACKAGE_MAX_COUNT)
	{
		free(pkgs);
		return 0;
	}

	qsort(pkgs, pkg_count, sizeof(log_file_entry_t), compare_by_mtime);
	size_t to_remove = pkg_count - LOG_PACKAGE_MAX_COUNT;
	for (size_t i = 0; i < to_remove; ++i)
	{
		char path[PATH_MAX];
		snprintf(path, sizeof(path), "%s/%s", LOG_WATCH_DIR, pkgs[i].name);
		UART_LOG_WARN(uc, "[report] drop old package %s (queue full)", pkgs[i].name);
		unlink(path);
		char stage_path[PATH_MAX];
		if (stage_path_from_package(path, stage_path, sizeof(stage_path)) == 0)
		{
			cleanup_directory(stage_path);
		}
	}
	free(pkgs);
	return 0;
}

static int upload_ready_packages(qtk_uart_client_t *uc)
{
	log_file_entry_t *raw = NULL;
	log_file_entry_t *pkgs = NULL;
	size_t raw_count = 0;
	size_t pkg_count = 0;
	if (gather_watch_files(&raw, &raw_count, &pkgs, &pkg_count) != 0)
		return -1;
	free(raw);
	if (pkg_count == 0)
	{
		free(pkgs);
		return 0;
	}

	qsort(pkgs, pkg_count, sizeof(log_file_entry_t), compare_by_mtime);
	int processed = 0;
	int overall_rc = 0;
	for (size_t i = 0; i < pkg_count; ++i)
	{
		char path[PATH_MAX];
		snprintf(path, sizeof(path), "%s/%s", LOG_WATCH_DIR, pkgs[i].name);
		UART_LOG_DBG(uc, "[report] upload package %s", pkgs[i].name);
		int rc = send_file_in_chunks(uc, path);
		if (rc == 0)
		{
			unlink(path);
			char stage_path[PATH_MAX];
			if (stage_path_from_package(path, stage_path, sizeof(stage_path)) == 0)
			{
				cleanup_directory(stage_path);
			}
			processed++;
		}
		else
		{
			overall_rc = rc;
			UART_LOG_WARN(uc, "[report] upload %s failed rc=%d", pkgs[i].name, rc);
			break;
		}
	}
	free(pkgs);
	return (overall_rc != 0) ? overall_rc : processed;
}

static int qtk_uart_client_report_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
{
	qtk_uart_set_cpu(uc, thread, 2);
	(void)thread;
	ensure_watch_directory();
	if (uc && uc->log)
		wtk_log_log0(uc->log, "[report] thread start");
	while (uc->report_run)
	{
		ensure_watch_directory();
		int force_now = 0;
		if (uc->report_trigger_once)
		{
			force_now = 1;
			uc->report_trigger_once = 0;
		}
		package_pending_logs(uc, force_now);
		enforce_package_queue_limit(uc);

		if (!uc->report_busy)
		{
			int rc = 0;
			uc->report_busy = 1;
			int processed = upload_ready_packages(uc);
			if (processed > 0)
			{
				uc->upload_round += 1;
				UART_LOG_DBG(uc, "[report] upload trigger #%u processed=%d", uc->upload_round, processed);
				rc = 0;
			}
			else if (processed < 0)
			{
				rc = processed;
			}
			if (processed != 0 && uc && uc->log)
			{
				wtk_log_log0(uc->log, (processed > 0 && rc == 0) ? "[report] upload ok" : "[report] upload failed");
			}
			uc->report_busy = 0;
		}

		usleep(LOG_SCAN_INTERVAL_MS * 1000);
	}
	if (uc && uc->log)
		wtk_log_log0(uc->log, "[report] thread stop");
	return 0;
}

static void log_hex_buffer(qtk_uart_client_t *uc, const char *tag, const uint8_t *buf, uint16_t len)
{
	if (!uc || !tag)
	{
		return;
	}
	if (!buf || len == 0)
	{
		UART_LOG_DBG(uc, "%s len=0", tag);
		return;
	}

	uint32_t hex_buf_len = (uint32_t)len * 3 + 1;
	char *hex = (char *)malloc(hex_buf_len);
	if (!hex)
	{
		UART_LOG_WARN(uc, "%s dump skipped (len=%u no mem)", tag, len);
		return;
	}

	uint32_t offset = 0;
	for (uint16_t i = 0; i < len && offset + 3 <= hex_buf_len; ++i)
	{
		offset += snprintf(hex + offset, hex_buf_len - offset, "%02X%s", buf[i], (i + 1 < len) ? " " : "");
	}
	UART_LOG_DBG(uc, "%s len=%u: %s", tag, len, hex);
	free(hex);
}

static void log_incoming_frame_hex(qtk_uart_client_t *uc, const qtk_uart_recv_frame_t *frame, uint16_t data_len)
{
	if (!frame)
	{
		return;
	}

	uint16_t total_len = 2 + 2 + 2 + data_len + 2 + 1;
	uint8_t *raw = (uint8_t *)malloc(total_len);
	if (!raw)
	{
		return;
	}

	uint16_t pos = 0;
	raw[pos++] = frame->frame_header[0];
	raw[pos++] = frame->frame_header[1];
	raw[pos++] = frame->event_code[0];
	raw[pos++] = frame->event_code[1];
	raw[pos++] = frame->data_length[0];
	raw[pos++] = frame->data_length[1];
	if (data_len > 0 && frame->data)
	{
		memcpy(raw + pos, frame->data, data_len);
		pos += data_len;
	}
	raw[pos++] = frame->checksum[0];
	raw[pos++] = frame->checksum[1];
	raw[pos++] = frame->frame_footer;

	log_hex_buffer(uc, "rx frame raw", raw, total_len);
	free(raw);
}

int qtk_uart_client_trsn_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
{
	qtk_uart_set_cpu(uc, thread, 2);
	uint8_t byte;
	int ret;
	uart_parse_state_t state = PARSE_STATE_HEADER1;
	qtk_uart_client_msg_t *msg;
	qtk_uart_recv_frame_t current_frame = {0};
	uint16_t data_len = 0;
	uint16_t data_index = 0;
	uint16_t crc_expected = 0;
	uint8_t crc_data[256]; // 最大支持 256 字节数据
	int crc_pos = 0;
	int event_byte = 0;
	int len_byte = 0;
	int crc_byte = 0;
	int first = 1;
	while (uc->trsn_run)
	{
		// wtk_debug("======>>>>>%p %p\n",uc,uc->uart);
		ret = qtk_uart_read(uc->uart, (char *)&byte, 1); // 逐字节读取
		if (ret <= 0)
		{
			usleep(10000); // 10ms
			continue;
		}
		if (first)
		{
			wtk_debug("--------------->>>>>>>>>>>>>first_to_secendTime= %.2f\n", time_get_ms() - recvdata_time);
			recvdata_time = time_get_ms();
			first = 0;
		}
		printf(" %02X ", byte); // 调试用
		switch (state)
		{
		case PARSE_STATE_HEADER1:
			if (byte == REQUEST_FRAME_HEADER_0)
			{
				memset(&current_frame, 0, sizeof(current_frame));
				current_frame.frame_header[0] = byte;
				state = PARSE_STATE_HEADER2;
				
			}
			break;

		case PARSE_STATE_HEADER2:
			if (byte == REQUEST_FRAME_HEADER_1)
			{
				current_frame.frame_header[1] = byte;
				state = PARSE_STATE_EVENT_CODE;
			}
			else
			{
				state = PARSE_STATE_HEADER1;
			}
			break;

		case PARSE_STATE_EVENT_CODE:
			current_frame.event_code[event_byte++] = byte;
			if (event_byte >= 2)
			{
				event_byte = 0;
				state = RESP_STATE_DATA_LENGTH;
			}
			break;

		case RESP_STATE_DATA_LENGTH:
			current_frame.data_length[0] = byte;
			state = RESP_STATE_DATA_LENGTH_2;
			break;

		case RESP_STATE_DATA_LENGTH_2:
			current_frame.data_length[1] = byte;
			data_len = (current_frame.data_length[1] << 8) | current_frame.data_length[0];
			if (data_len > 4096)
			{
				UART_LOG_WARN(uc, "frame payload length %u bytes (evt=0x%02X%02X)", data_len, current_frame.event_code[1], current_frame.event_code[0]);
			}
			if (data_len > 0)
			{
				current_frame.data = (uint8_t *)malloc(data_len);
				if (!current_frame.data)
				{
					UART_LOG_ERR(uc, "malloc %u bytes for frame payload failed", data_len);
					state = PARSE_STATE_HEADER1;
					break;
				}
			}
			else
			{
				state = RESP_STATE_CHECKSUM;
				break;
			}
			data_index = 0;
			state = RESP_STATE_DATA;
			break;
		case RESP_STATE_DATA:
			if (data_index < data_len)
			{
				current_frame.data[data_index++] = byte;
			}
			printf("data_index : %d\n", data_index);
			if (data_index >= data_len)
			{
				printf("RESP_STATE_DATA:date_len : %d\n", data_len);
				state = RESP_STATE_CHECKSUM;
			}
			break;

		case RESP_STATE_CHECKSUM:
			current_frame.checksum[0] = byte;
			printf("current_frame.checksum[0]: %02X\n", current_frame.checksum[0]);
			state = RESP_STATE_CHECKSUM_2;

			break;
		case RESP_STATE_CHECKSUM_2:
			current_frame.checksum[1] = byte;
			printf("current_frame.checksum[1]: %02X\n", current_frame.checksum[1]);
			state = RESP_STATE_FOOTER;
			break;
		case RESP_STATE_FOOTER:
			if (byte == FRAME_FOOTER)
			{
				current_frame.frame_footer = byte;
#if 0
                    printf("Received Checksum: %02X %02X\n", 
                    current_frame.checksum[0], 
                    current_frame.checksum[1]);
                    // 构造 CRC 数据
                    crc_pos = 0;
                    crc_data[crc_pos++] = current_frame.event_code[0];
                    crc_data[crc_pos++] = current_frame.event_code[1];
                    crc_data[crc_pos++] = current_frame.data_length[0];
                    crc_data[crc_pos++] = current_frame.data_length[1];
                    for (int i = 0; i < data_len; i++) {
                        crc_data[crc_pos++] = current_frame.data[i];
                    }
					// wtk_debug("current_frame.data[]: %02X\n",current_frame.data[0]);
                    uint16_t calc_crc = calculateModbusCRC(crc_data, crc_pos);
                    uint16_t recv_crc = current_frame.checksum[0] | (current_frame.checksum[1] << 8);
					wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-recvdata_time);
                    if (calc_crc == recv_crc) {
                        handle_uart_frame(uc, &current_frame);
						// msg = qtk_uart_client_pop_msg(uc);
						// wtk_strbuf_push(msg->buf,(char*)&current_frame,sizeof(current_frame));
						
						// wtk_blockqueue_push(&uc->msg_q, &msg->q_n);
                    } else {
                        wtk_debug("CRC error: %04X vs %04X\n", recv_crc, calc_crc);
                    }
#else
				uint16_t event_code = (uint16_t)((current_frame.event_code[1] << 8) | current_frame.event_code[0]);
				uint16_t recv_crc = (uint16_t)(current_frame.checksum[0] | (current_frame.checksum[1] << 8));
				int crc_len = 4 + data_len;
				uint16_t calc_crc = 0;
				int crc_ready = 1;
				uint8_t *crc_buf = (uint8_t *)malloc(crc_len);
				if (!crc_buf)
				{
					// UART_LOG_ERR(uc, "alloc %d bytes for CRC buffer failed (evt=0x%04X)", crc_len, event_code);
					crc_ready = 0;
				}
				else
				{
					crc_buf[0] = current_frame.event_code[0];
					crc_buf[1] = current_frame.event_code[1];
					crc_buf[2] = current_frame.data_length[0];
					crc_buf[3] = current_frame.data_length[1];
					if (data_len > 0 && current_frame.data)
					{
						memcpy(crc_buf + 4, current_frame.data, data_len);
					}
					calc_crc = calculateModbusCRC(crc_buf, crc_len);
					free(crc_buf);
				}
				if (crc_ready && calc_crc == recv_crc)
				{
					double now_ms = time_get_ms();
					double delta_ms = now_ms - recvdata_time;
					UART_LOG_DBG(uc, "recv frame evt=0x%04X len=%u latency=%.2fms", event_code, data_len, delta_ms);
					log_incoming_frame_hex(uc, &current_frame, data_len);
					recvdata_time = now_ms;
					msg = qtk_uart_client_pop_msg(uc);
					if (msg)
					{
						size_t data_offset = 0;
						wtdebugCurrentTime();
						send_count++;
						wtk_debug("--------------->>>>>>>>>>>>>>recv_count = %d\n",send_count);
						wtk_strbuf_reset(msg->buf);
						wtk_strbuf_push(msg->buf, (char *)&current_frame, sizeof(current_frame));
						if (data_len > 0 && current_frame.data)
						{
							data_offset = msg->buf->pos;
							wtk_strbuf_push(msg->buf, (char *)current_frame.data, data_len);
						}
						qtk_uart_recv_frame_t *stored = (qtk_uart_recv_frame_t *)msg->buf->data;
						stored->data = (data_len > 0) ? (uint8_t *)(msg->buf->data + data_offset) : NULL;
						wtk_blockqueue_push(&uc->msg_q, &msg->q_n);
					}
					else
					{
						UART_LOG_WARN(uc, "msg pool exhausted while handling evt=0x%04X", event_code);
					}
				}
				else if (crc_ready)
				{
					UART_LOG_WARN(uc, "CRC mismatch evt=0x%04X len=%u calc=0x%04X recv=0x%04X", event_code, data_len, calc_crc, recv_crc);
				}
			}
			else
			{
				UART_LOG_WARN(uc, "frame footer mismatch: got 0x%02X", byte);
			}

#endif
			if (current_frame.data)
			{
				free(current_frame.data);
				current_frame.data = NULL;
			}
			state = PARSE_STATE_HEADER1;
			first = 1;
			break;
		}
	}

	return 0;
}

static void qtk_uart_client_clean_q(qtk_uart_client_t *uc, wtk_blockqueue_t *queue)
{
	qtk_uart_client_msg_t *msg;
	wtk_queue_node_t *qn;
	int len = queue->length;
	int i = 0;

	while (i < len)
	{
		qn = wtk_blockqueue_pop(queue, 0, NULL);
		if (!qn)
		{
			break;
		}
		msg = data_offset2(qn, qtk_uart_client_msg_t, q_n);
		qtk_uart_client_push_msg(uc, msg);
		i++;
	}
}

static qtk_uart_client_msg_t *qtk_uart_client_msg_new(qtk_uart_client_t *uc)
{
	qtk_uart_client_msg_t *msg;

	msg = (qtk_uart_client_msg_t *)wtk_malloc(sizeof(*msg));
	if (!msg){return NULL;}
	msg->buf = wtk_strbuf_new(2560, 1);
	if (!msg->buf){
		wtk_free(msg);
		return NULL;
	}
	msg->sendbuf = wtk_strbuf_new(2048, 1.0);

	return msg;
}

static int qtk_uart_client_msg_delete(qtk_uart_client_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_strbuf_delete(msg->sendbuf);
	wtk_free(msg);
	return 0;
}

static qtk_uart_client_msg_t *qtk_uart_client_pop_msg(qtk_uart_client_t *uc)
{
	qtk_uart_client_msg_t *msg;

	msg = wtk_lockhoard_pop(&uc->msg_hoard);
	if (!msg)
	{
		return NULL;
	}
	msg->statID = 0;
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_reset(msg->sendbuf);
	return msg;
}

static void qtk_uart_client_push_msg(qtk_uart_client_t *uc, qtk_uart_client_msg_t *msg)
{
	wtk_lockhoard_push(&uc->msg_hoard, msg);
}

int qtk_uart_client_feed(qtk_uart_client_t *uc, char *data, int len)
{
	return 0;
}

int qtk_uart_client_uart_param(qtk_uart_client_t *uc, wtk_strbuf_t *buf)
{
	wtk_json_item_t *item;
	wtk_json_t *json;
	wtk_strbuf_t *tmp = NULL;

	json = wtk_json_new();
	item = wtk_json_new_object(json);
	tmp = wtk_strbuf_new(256, 0);

	wtk_json_obj_add_ref_number_s(json, item, "dev_no", uc->cfg->dev_no);

	wtk_json_obj_add_str2_s(json, item, "mac", uc->mac->data, uc->mac->pos);
	wtk_json_obj_add_str2_s(json, item, "version", uc->version->data, uc->version->pos);

	tm_e = time_get_ms();
	wtk_json_obj_add_ref_number_s(json, item, "delay", (int)(tm_e - tm_s));
	tm_s = 0;
	tm_e = 0;
	// qtk_get_battery_file(tmp);
	// wtk_json_obj_add_str2_s(json, item, "batt", tmp->data, tmp->pos);
	// wtk_strbuf_reset(tmp);

	wtk_json_item_print(item, tmp);
	wtk_strbuf_push(buf, tmp->data, tmp->pos);
	wtk_strbuf_delete(tmp);
	wtk_json_delete(json);
	return 0;
}

static void qtk_uart_client_feed_notice(qtk_uart_client_t *uc, int notice)
{
	qtk_uart_client_msg_t *msg;

	msg = qtk_uart_client_pop_msg(uc);

	msg->statID = notice;

	wtk_blockqueue_push(&uc->input_q, &msg->q_n);
}
int parse_uart_frame(const uint8_t *data, int len, qtk_uart_recv_frame_t *frame)
{
	wtk_debug("parse_uart_frame\n");
	if (len < 9)
	{
		return -1; // 帧不完整
	}
	if (data[0] != REQUEST_FRAME_HEADER_0 ||
		data[1] != REQUEST_FRAME_HEADER_1)
	{
		return -2; // 帧头错误
	}
	if (data[len - 1] != FRAME_FOOTER)
	{
		return -3; // 帧尾错误
	}
	memcpy(frame->frame_header, data, 2);
	memcpy(frame->event_code, data + 2, 2);
	memcpy(frame->data_length, data + 4, 2);
	memcpy(frame->checksum, data + len - 3, 2);
	frame->frame_footer = data[len - 1];
	// 解析数据部分
	uint16_t data_len = frame->data_length[0] | (frame->data_length[1] << 8);
	if (data_len > 0)
	{
		// 检查数据长度是否匹配
		if (len != 9 + data_len)
		{
			return -4; // 长度不匹配
		}
		frame->data = (uint8_t *)malloc(data_len);
		if (!frame->data)
		{
			return -5; // 内存分配失败
		}
		memcpy(frame->data, data + 6, data_len);
	}
	else
	{
		frame->data = NULL;
	}

	return 0;
}
static void send_active_response(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, uint8_t *data, uint16_t data_len)
{
	qtk_uart_recv_frame_t resp = {
		.frame_header = {REQUEST_FRAME_HEADER_0, REQUEST_FRAME_HEADER_1},
		.event_code = {event_code1, event_code2},
		.data_length = {data_len & 0xFF, (data_len >> 8) & 0xFF},
		.data = (uint8_t *)data,
		.frame_footer = FRAME_FOOTER};

	wtk_debug("-------------__>>>>>>>>>>>>>>>>>data_len = %d\n",data_len);
	int crc_data_len = 4 + data_len;
	uint8_t *crc_data = (uint8_t *)malloc(crc_data_len);
	if (crc_data)
	{
		int pos = 0;
		memcpy(crc_data + pos, resp.event_code, 2);
		pos += 2;
		memcpy(crc_data + pos, resp.data_length, 2);
		pos += 2;
		if (data_len > 0)
		{
			memcpy(crc_data + pos, data, data_len);
		}

	wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
		uint16_t crc = calculateModbusCRC(crc_data, crc_data_len);
		resp.checksum[0] = crc & 0xFF;
		resp.checksum[1] = (crc >> 8) & 0xFF;

		free(crc_data);
	}
	else
	{
		resp.checksum[0] = 0x00;
		resp.checksum[1] = 0x00;
	}

	wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
	int frame_len = 9 + data_len; // 2头 + 2事件 + 2长度 + 数据 + 2CRC + 1尾
	uint8_t *send_buf = (uint8_t *)malloc(frame_len);
	wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
	if (send_buf)
	{
		int pos = 0;
		memcpy(send_buf + pos, resp.frame_header, 2);
		pos += 2;
		memcpy(send_buf + pos, resp.event_code, 2);
		pos += 2;
		memcpy(send_buf + pos, resp.data_length, 2);
		pos += 2;

		if (data_len > 0)
		{
			memcpy(send_buf + pos, data, data_len);
			pos += data_len;
		}

		memcpy(send_buf + pos, resp.checksum, 2);
		pos += 2;
		send_buf[pos++] = resp.frame_footer;
		int ret = qtk_uart_write2(uc->uart, (char *)send_buf, pos);
		free(send_buf);
	}
}

static void send_response(qtk_uart_client_t *uc,
						  qtk_uart_recv_frame_t *req_frame,
						  uint8_t *data,
						  uint16_t data_len)
{
	if (!uc || !req_frame)
	{
		return;
	}
#if 0
	qtk_uart_recv_frame_t resp = {
        .frame_header = {RESPONSE_FRAME_HEADER_0, RESPONSE_FRAME_HEADER_1},
        .event_code = {req_frame->event_code[0], req_frame->event_code[1]},
        .data_length = {data_len & 0xFF, (data_len >> 8) & 0xFF},
        .data = (data_len > 0) ? data : NULL,
		.checksum = {0},
        .frame_footer = FRAME_FOOTER
    };
	wtk_strbuf_reset(uc->responsebuf);
	wtk_strbuf_reset(uc->crcbuf);
	wtk_strbuf_push(uc->responsebuf, resp.frame_header, 2);
	wtk_strbuf_push(uc->crcbuf, resp.event_code, 2);
	wtk_strbuf_push(uc->crcbuf, resp.data_length, 2);
	if(resp.data_length > 0){
		wtk_strbuf_push(uc->crcbuf, resp.data, resp.data_length);
	}
	wtk_strbuf_push(uc->responsebuf, uc->crcbuf->data, uc->crcbuf->pos);
	uint16_t crc = calculateModbusCRC(uc->crcbuf->data, uc->crcbuf->pos);
	wtk_strbuf_push(uc->responsebuf, (char *)(&crc), 2);
	char footer=resp.frame_footer;
	wtk_strbuf_push(uc->responsebuf, &footer, 1);
	double send_start = time_get_ms();
    int ret = qtk_uart_write2(uc->uart, uc->responsebuf->data, uc->responsebuf->pos);
	uint16_t evt_code = (uint16_t)(*(resp.event_code));
    double send_end = time_get_ms();
    if (ret != uc->responsebuf->pos) {
        UART_LOG_WARN(uc, "send failed evt=0x%04X wrote=%d expected=%d errno=%d duration=%.2fms",
                      evt_code, ret, uc->responsebuf->pos, errno, send_end - send_start);
    } else {
        UART_LOG_DBG(uc, "send done evt=0x%04X bytes=%d duration=%.2fms",
                     evt_code, ret, send_end - send_start);
    }
#else
	uint16_t payload_len = data_len;
	uint8_t *payload = data;
	uint8_t inline_buf[8];
	if (payload_len > 0)
	{
		uintptr_t ptr_val = (uintptr_t)payload;
		if (ptr_val < 0x1000)
		{
			if (payload_len > sizeof(inline_buf))
			{
				// UART_LOG_WARN(uc, "send_response truncated inline payload len=%u to %zu (evt=0x%02X%02X)", payload_len, sizeof(inline_buf), req_frame->event_code[1], req_frame->event_code[0]);
				payload_len = (uint16_t)sizeof(inline_buf);
			}
			for (uint16_t i = 0; i < payload_len; ++i)
			{
				inline_buf[i] = (uint8_t)((ptr_val >> (i * 8)) & 0xFF);
			}
			payload = inline_buf;
			// UART_LOG_WARN(uc, "send_response treated address 0x%zx as inline payload (len=%u)", (size_t)ptr_val, payload_len);
		}
		else if (!payload)
		{
			// UART_LOG_ERR(uc, "send_response missing payload buffer for evt=0x%02X%02X, dropping payload", req_frame->event_code[1], req_frame->event_code[0]);
			payload_len = 0;
		}
	}

	qtk_uart_recv_frame_t resp = {
		.frame_header = {RESPONSE_FRAME_HEADER_0, RESPONSE_FRAME_HEADER_1},
		.event_code = {req_frame->event_code[0], req_frame->event_code[1]},
		.data_length = {payload_len & 0xFF, (payload_len >> 8) & 0xFF},
		.data = (payload_len > 0) ? payload : NULL,
		.frame_footer = FRAME_FOOTER};

	int crc_len = 4 + payload_len;
	uint8_t *crc_buf = (uint8_t *)malloc(crc_len);
	if (crc_buf)
	{
		crc_buf[0] = resp.event_code[0];
		crc_buf[1] = resp.event_code[1];
		crc_buf[2] = resp.data_length[0];
		crc_buf[3] = resp.data_length[1];
		if (payload_len > 0 && payload)
		{
			memcpy(crc_buf + 4, payload, payload_len);
		}
		uint16_t crc = calculateModbusCRC(crc_buf, crc_len);
		resp.checksum[0] = crc & 0xFF;
		resp.checksum[1] = (crc >> 8) & 0xFF;
		free(crc_buf);
	}
	else
	{
		resp.checksum[0] = 0x00;
		resp.checksum[1] = 0x00;
		// UART_LOG_ERR(uc, "alloc %d bytes for response CRC failed (evt=0x%02X%02X)", crc_len, resp.event_code[1], resp.event_code[0]);
	}

	int frame_len = 9 + payload_len;
	uint8_t *send_buf = (uint8_t *)malloc(frame_len);
	if (!send_buf)
	{
		// UART_LOG_ERR(uc, "alloc %d bytes for response frame failed (evt=0x%02X%02X)", frame_len, resp.event_code[1], resp.event_code[0]);
		return;
	}

	int pos = 0;
	memcpy(send_buf + pos, resp.frame_header, 2);
	pos += 2;
	memcpy(send_buf + pos, resp.event_code, 2);
	pos += 2;
	memcpy(send_buf + pos, resp.data_length, 2);
	pos += 2;
	if (payload_len > 0 && payload)
	{
		memcpy(send_buf + pos, payload, payload_len);
		pos += payload_len;
	}
	memcpy(send_buf + pos, resp.checksum, 2);
	pos += 2;
	send_buf[pos++] = resp.frame_footer;

	// log_hex_buffer(uc, "tx payload", payload, payload_len);
	// log_hex_buffer(uc, "tx frame", send_buf, frame_len);

	uint16_t evt_code = ((uint16_t)resp.event_code[0] << 8) | resp.event_code[1];
	double send_start = time_get_ms();
	// UART_LOG_DBG(uc, "send start evt=0x%04X payload=%u t=%.2fms", evt_code, payload_len, send_start);

	int ret = qtk_uart_write2(uc->uart, (char *)send_buf, frame_len);
	wtk_debug("Send log frame: len=%d\n", ret);
	wtk_debug("Header: %02X %02X\n", resp.frame_header[0], resp.frame_header[1]);
	wtk_debug("Event: %02X %02X\n", resp.event_code[0], resp.event_code[1]);
	wtk_debug("Length: %02X %02X (%d bytes)\n", resp.data_length[0], resp.data_length[1], payload_len);
	wtk_debug("CRC: %02X %02X\n", resp.checksum[0], resp.checksum[1]);
	wtk_debug("Footer: %02X\n", resp.frame_footer);
	free(send_buf);
#endif
}

void qtk_uart_client_send_response(qtk_uart_client_t *uc,
						  qtk_uart_recv_frame_t *req_frame,
						  uint8_t *data,
						  uint16_t data_len)
{
	// send_response(uc, req_frame, data, data_len);
	qtk_uart_client_msg_t *msg;
	
	msg = qtk_uart_client_pop_msg(uc);
	msg->statID = QTK_UART_SEND_STATE_NORMAL;
	wtk_strbuf_reset(msg->buf);
	wtk_strbuf_push(msg->buf, (char *)req_frame, sizeof(qtk_uart_recv_frame_t));
	uint16_t dlen;
	dlen = (req_frame->data_length[1] << 8) | req_frame->data_length[0];
	if (dlen > 0 && req_frame->data)
	{
		wtk_strbuf_push(msg->buf, (char *)req_frame->data, dlen);
	}
	wtk_strbuf_reset(msg->sendbuf);
	wtk_strbuf_push(msg->sendbuf, data, data_len);
	wtk_blockqueue_push(&uc->send_queue, &msg->q_n);
}

void qtk_uart_client_send_active_response(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, uint8_t *data, uint16_t data_len)
{
	send_active_response(uc, event_code1, event_code2, data, data_len);
}

unsigned short calculateModbusCRC(unsigned char *data, int length)
{
	unsigned short crc = 0xFFFF; // 初始化 CRC 值
	unsigned char b;
	for (int i = 0; i < length; i++)
	{
		crc ^= data[i];
		// 对每个字节进行 8 次右移操作
		for (int j = 0; j < 8; j++)
		{
			if (crc & 0x0001)
			{
				crc >>= 1;	   // 右移 1 位
				crc ^= 0xA001; // 异或常数 0xA001
			}
			else
			{
				crc >>= 1; // 右移 1 位
			}
		}
	}
	return crc;
}


void qtk_uart_set_cpu(qtk_uart_client_t *m, wtk_thread_t *thread, int cpunum)
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
