#include "qtk_uart_client.h"

static double tm_s = 0;
static double tm_e = 0;
#define USE_3308
#define USE_LOG
#ifdef USE_3308
static int once=0;
static int once2=1;
static char buf[12]={0};
static int result=0;
static uint8_t result_data;
static struct{
		key_t key;
    	SHM *shmaddr;
    	int shmid;
    	pid_t pid_client;
}cp;
#endif
#define USE_AM13E2
#ifndef SYSFS_EQ_PATH
#define SYSFS_EQ_PATH "/sys/bus/i2c/devices/3-0069/eq"
#endif 
#ifndef SYSFS_EQ_PATH_BASS
#define SYSFS_EQ_PATH_BASS "/sys/bus/i2c/devices/3-006d/eq"
#endif
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
// #define chenggang
int send_count=0;
#define chenggang
#ifdef chenggang
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#ifndef QTK_EQ_JSON_PATH
#define QTK_EQ_JSON_PATH "/oem/qdreamer/qsound/eq.json"
#endif
#include <pthread.h>   // 互斥锁（若你不想加锁，可略过pthread相关两行）
/********** 主动上报事件码（按协议V1.8） **********/
#define EVT_LOG_UPLOAD_START 0x0120 // 开始包
#define EVT_LOG_UPLOAD_DATA 0x0121 // 数据包
#define EVT_LOG_UPLOAD_END 0x0122 // 结束包
/* #define EVT_LOG_UPLOAD_RETRANS 0x0123 // 重传（可选扩展） */

/********** 监控路径与阈值 **********/
#ifndef LOG_WATCH_DIR
#define LOG_WATCH_DIR "/oem/qdreamer/qsound" // 你要求监听的目录
#endif
#ifndef LOG_UPLOAD_THRESHOLD_BYTES
#define LOG_UPLOAD_THRESHOLD_BYTES (8*1024*1024ULL) // 达 8MB 就打包上报，可自行调整
#endif
#ifndef LOG_SCAN_INTERVAL_MS
#define LOG_SCAN_INTERVAL_MS 2000 // 每 2s 扫描一次
#endif

/* 单包最大载荷：协议上限 65535（这里要预留 2 字节序号） */
#define LOG_DATA_MAX_PER_FRAME (65535-2)

/* 数据包间隔节流，防止对端缓冲顶满（微秒） */
#define LOG_FRAME_GAP_US 2000
#endif
qtk_uart_recv_frame_t current_frame = {0};
int qdreamer_audio_check_request;  				// 0-无请求, 1-扬声器检测, 2-麦克风检测
int qdreamer_audio_check_result;   				//  0-正常, 1-静音, 2-爆音
uint64_t qdreamer_audio_check_start_time;        // 检测开始时间戳
int qdreamer_audio_check_running;      			// 检测状态标志
time_t startspk_time;
double recvdata_time = 0;
int isnot_spk_count=0;

qtk_uart_client_t *lc;

static int qtk_uart_client_netlink(qtk_uart_client_t *uc);
void qtk_uart_client_on_data(qtk_uart_client_t *uc, char *data, int bytes);
void qtk_uart_client_on_enrcheck(qtk_uart_client_t *uc, float enr_thresh, float enr, int on_run);
int qtk_uart_client_trsn_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
int qtk_uart_client_recorder_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
int qtk_uart_client_msg_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
int qtk_uart_client_send_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
static qtk_uart_client_msg_t* qtk_uart_client_msg_new(qtk_uart_client_t *uc);
static int qtk_uart_client_msg_delete(qtk_uart_client_msg_t *msg);
static qtk_uart_client_msg_t* qtk_uart_client_pop_msg(qtk_uart_client_t *uc);
static void qtk_uart_client_push_msg(qtk_uart_client_t *uc,qtk_uart_client_msg_t *msg);
static void qtk_uart_client_clean_q(qtk_uart_client_t *uc, wtk_blockqueue_t *queue);
void qtk_uart_client_update_params(qtk_uart_client_t *uc, char *data, int len);
int qtk_uart_client_uart_param(qtk_uart_client_t *uc, wtk_strbuf_t *buf);
static void qtk_uart_client_feed_notice(qtk_uart_client_t *uc, int notice);
static int qtk_uart_client_get_code_string(qtk_uart_client_t *uc, int statID, wtk_strbuf_t *buf);
void qtk_uart_client_on_uart(qtk_uart_client_t *uc, qtk_uart_type_t state, char *data, int len);
void handle_uart_frame(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *frame);
static void send_response(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *req_frame,uint8_t*data,uint16_t data_len);
static void send_active_response(qtk_uart_client_t *uc ,uint8_t event_code1, uint8_t event_code2,uint8_t *data, uint16_t data_len);
#ifdef chenggang
typedef void (*uart_callback_t)(qtk_uart_type_t type, void *userdata);
static int qtk_uart_client_report_run(qtk_uart_client_t *uc, wtk_thread_t *thread);
static int qtk_uart_client_send_active(qtk_uart_client_t *uc, uint16_t event_code,
const uint8_t *data, uint16_t data_len);
static uint64_t dir_total_size(const char *path);
static int make_tar_gz(qtk_uart_client_t *uc, const char *src_dir, char out_path[], size_t out_sz);
static int send_file_in_chunks(qtk_uart_client_t *uc, const char *file_path);
static void le16_write(uint8_t *p, uint16_t v);
static void le32_write(uint8_t *p, uint32_t v);
static void le64_write(uint8_t *p, uint64_t v);
static int sysfs_write_hex_u8(const char *path, unsigned code_hex);
static pthread_mutex_t g_amp_lock = PTHREAD_MUTEX_INITIALIZER;
static int map_gain_to_level(float g_db);
static void sanitize_json_payload(const uint8_t *in, size_t in_len,const char **out_json, size_t *out_len);

static int send_audio_status_frame(qtk_uart_client_t *uc,uint8_t event_code1, uint8_t event_code2 ,int type);
#endif
// —— 动态解析两个AMP的eq节点（避免硬编码3-0069/3-006d写错或节点不存在）——
static char g_eq_path_main[128] = "/sys/bus/i2c/devices/3-0069/eq";
static char g_eq_path_bass[128] = "/sys/bus/i2c/devices/3-006d/eq";
// 按你的频段规则把 json 文本直接写入 EQ（低频写 0x69+0x6d，中/高只写 0x69）
// 返回 0 表示至少成功写入主EQ一次；负数表示失败。
static int apply_eq_from_json_text_wtk(const char* json_in, int len_in)
{
    if (!json_in || len_in <= 0) {
        wtk_debug("[EQ][apply] invalid json buffer\n");
        return -1;
    }

    // A) 解析（传入的是 sanitize 后的切片就行，这里不再重复 sanitize）
    wtk_json_parser_t *jp = wtk_json_parser_new();
    if (!jp) { wtk_debug("[EQ][apply] parser_new failed\n"); return -2; }
    if (wtk_json_parser_parse(jp, json_in, len_in) != 0) {
        wtk_debug("[EQ][apply] parser_parse failed\n");
        wtk_json_parser_delete(jp);
        return -3;
    }

    wtk_json_item_t *root = (jp && jp->json) ? jp->json->main : NULL;
    if (!root) {
        wtk_debug("[EQ][apply] root null\n");
        wtk_json_parser_delete(jp);
        return -4;
    }

    // B) 拿到数组：根是数组；或根是对象时找常见数组字段
    wtk_json_item_t *arr = NULL;
    if (root->type == WTK_JSON_ARRAY) {
        arr = root;
    } else if (root->type == WTK_JSON_OBJECT) {
        static const char* keys[] = {"bands","eq","data","items","list"};
        for (int ki = 0; ki < (int)(sizeof(keys)/sizeof(keys[0])); ++ki) {
            wtk_json_item_t *tmp = wtk_json_obj_get(root, keys[ki], (int)strlen(keys[ki]));
            if (tmp && tmp->type == WTK_JSON_ARRAY) { arr = tmp; break; }
        }
    }
    if (!arr || arr->type != WTK_JSON_ARRAY) {
        wtk_debug("[EQ][apply] root not array\n");
        wtk_json_parser_delete(jp);
        return -5;
    }

    // C) 逐条应用
    int n = wtk_json_item_len(arr);
    int wrote_main = 0, wrote_bass = 0, total = 0;

    for (int i = 0; i < n; ++i) {
        wtk_json_item_t *obj = wtk_json_array_get(arr, i);
        if (!obj || obj->type != WTK_JSON_OBJECT) continue;

        // freq / gain：数字或字符串都支持
        float freq = -1.0f, gain = 0.0f;
        wtk_json_item_t *jf = wtk_json_obj_get(obj, "freq", 4);
        if (jf) {
            if (jf->type == WTK_JSON_NUMBER) freq = (float)jf->v.number;
            else if (jf->type == WTK_JSON_STRING && jf->v.str) freq = (float)atof(jf->v.str->data);
        }
        wtk_json_item_t *jg = wtk_json_obj_get(obj, "gain", 4);
        if (jg) {
            if (jg->type == WTK_JSON_NUMBER) gain = (float)jg->v.number;
            else if (jg->type == WTK_JSON_STRING && jg->v.str) gain = (float)atof(jg->v.str->data);
        }
        if (freq < 0.0f) { wtk_debug("[EQ][apply] skip item without valid freq\n"); continue; }

        int B = (freq <= 250.0f) ? 0 : ((freq >= 4000.0f) ? 2 : 1);
        int L = map_gain_to_level(gain);    // 0..6 → {+6,+4,+2,0,-2,-4,-6}
        unsigned char code = (unsigned char)(((B & 0x0F) << 4) | (L & 0x0F));

        // 主 EQ（0x69）
        int ret_main = -99, ret_bass = -99;
        if (g_eq_path_main[0] != '\0') {
            ret_main = sysfs_write_hex_u8(g_eq_path_main, code);
            wtk_debug("[EQ][apply] main write code=0x%02X ret=%d (freq=%.1f, B=%d, L=%d)\n",
                      code, ret_main, freq, B, L);
            if (ret_main == 0) wrote_main++;
        } else {
            wtk_debug("[EQ][apply] main path empty, skip\n");
        }

        // 低频 → 低音 AMP（0x6d）
        if (B == 0) {
            if (g_eq_path_bass[0] != '\0') {
                ret_bass = sysfs_write_hex_u8(g_eq_path_bass, code);
                wtk_debug("[EQ][apply] bass write code=0x%02X ret=%d (freq=%.1f)\n", code, ret_bass, freq);
                if (ret_bass == 0) wrote_bass++;
            } else {
                wtk_debug("[EQ][apply] bass path empty, skip\n");
            }
        }

        total++;
    }

    wtk_json_parser_delete(jp);

    wtk_debug("[EQ][apply] summary: total=%d, wrote_main=%d, wrote_bass=%d\n", total, wrote_main, wrote_bass);
    return (total > 0 && wrote_main > 0) ? 0 : -6;
}

static void resolve_eq_sysfs_paths(void) {
    // 主功放 0069
    for (int bus = 0; bus <= 7; ++bus) {
        char p[128];
        snprintf(p, sizeof(p), "/sys/bus/i2c/devices/%d-0069/eq", bus);
        if (access(p, W_OK) == 0) { strncpy(g_eq_path_main, p, sizeof(g_eq_path_main)); break; }
    }
    // 低音功放 006d
    for (int bus = 0; bus <= 7; ++bus) {
        char p[128];
        snprintf(p, sizeof(p), "/sys/bus/i2c/devices/%d-006d/eq", bus);
        if (access(p, W_OK) == 0) { strncpy(g_eq_path_bass, p, sizeof(g_eq_path_bass)); break; }
    }
    fprintf(stderr, "[EQ] main@%s  bass@%s\n", g_eq_path_main, g_eq_path_bass);
}
static int write_eq_bass_L_autoprobe(int L) {
    // 先尝试 0x0L / 0x1L / 0x2L
    unsigned cand[] = {
        ((0u&0x0F)<<4) | (unsigned)(L&0x0F),
        ((1u&0x0F)<<4) | (unsigned)(L&0x0F),
        ((2u&0x0F)<<4) | (unsigned)(L&0x0F),
    };
    for (int i=0;i<3;i++){
        if (sysfs_write_hex_u8(g_eq_path_bass, cand[i]) == 0) {
            fprintf(stderr, "[EQ][006d] accept 0x%02X at %s\n", cand[i]&0xFF, g_eq_path_bass);
            return 0;
        }
    }
    // 再试只写 L（十进制）
    {
        pthread_mutex_lock(&g_amp_lock);
        FILE *sf = fopen(g_eq_path_bass, "w");
        int ok = 0;
        if (sf) { if (fprintf(sf, "%d\n", (L&0x0F)) > 0) ok=1; fflush(sf); fclose(sf); }
        pthread_mutex_unlock(&g_amp_lock);
        if (ok) { fprintf(stderr, "[EQ][006d] accept dec L=%d at %s\n", L&0x0F, g_eq_path_bass); return 0; }
    }
    // 再试只写 L（十六进制）
    {
        pthread_mutex_lock(&g_amp_lock);
        FILE *sf = fopen(g_eq_path_bass, "w");
        int ok = 0;
        if (sf) { if (fprintf(sf, "0x%X\n", (L&0x0F)) > 0) ok=1; fflush(sf); fclose(sf); }
        pthread_mutex_unlock(&g_amp_lock);
        if (ok) { fprintf(stderr, "[EQ][006d] accept hex L=0x%X at %s\n", L&0x0F, g_eq_path_bass); return 0; }
    }
    fprintf(stderr, "[EQ][006d] all formats rejected for L=%d at %s\n", L, g_eq_path_bass);
    return -1;
}
#if 1
static int sysfs_write_hex_u8(const char *path, unsigned code_hex) {
    pthread_mutex_lock(&g_amp_lock);
    FILE *sf = fopen(path, "w");
    if (!sf) { pthread_mutex_unlock(&g_amp_lock); return -1; }
    int n = fprintf(sf, "0x%02X\n", (unsigned)(code_hex & 0xFF));  // 一次写一行，带换行
    fflush(sf);
    fclose(sf);
    pthread_mutex_unlock(&g_amp_lock);
    usleep(4000);  // 给I2C/AMP 3~5ms消化
    return (n > 0) ? 0 : -2;
}

static int write_eq_code_to_sysfs(const char *path, unsigned code_hex) {
    for (int t = 0; t < 3; ++t) {
        FILE *sf = fopen(path, "w");
        if (!sf) { usleep(2000); continue; }
        // 驱动普遍接受带换行，稳一点：
        int n = fprintf(sf, "0x%02X\n", code_hex & 0xFF);
        fflush(sf);
        fclose(sf);
        if (n > 0) return 0;
        usleep(2000);
    }
    return -1;
}
/* 由 EQ 模式值映射到 “低频 dB” （来自Excel映射；如需后续调整，改这里即可） */
static int map_mode_to_low_db(unsigned char mode)
{
    switch (mode) {
        case 0x00: return 0;   // 标准: 低0dB
        case 0x01: return -2;  // 挂墙: 低-2dB
        case 0x02: return -4;  // 嵌墙: 低-4dB
        default:   return 0;   // 未知就取0
    }
}
static int send_audio_status_frame(qtk_uart_client_t *uc,uint8_t event_code1, uint8_t event_code2 ,int type) {
	qtk_uart_recv_frame_t frame;

	// 构建帧
	frame.frame_header[0] = RESPONSE_FRAME_HEADER_0; // 0x90
	frame.frame_header[1] = RESPONSE_FRAME_HEADER_1; // 0x40
	frame.event_code[0] = event_code1;
	frame.event_code[1] = event_code2;

	cJSON *port1 = cJSON_CreateObject();
	FILE*fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r");
	char buf[1024] = {0};
	int ret, s,is_use,is_lineinmic,pos=0;
	char *pv;
	ret = fread(buf, 1, sizeof(buf), fn);
	// pos += snprintf(final_json + pos, sizeof(final_json) - pos, "[\n");
	if(type == QTK_UART_STATUS_MIC) {
		pv = strstr(buf, "mic_shift2=");
		s = atoi(pv + 11);
		pv = strstr(buf, "VBOX3_MIC=");
		is_use = atoi(pv + 10);
		cJSON_AddNumberToObject(port1, "portType", 0);
		cJSON_AddNumberToObject(port1, "portId", 0xaa00000);
		cJSON_AddStringToObject(port1, "portName", "arrary_MIC");
		if(!is_use)
			cJSON_AddFalseToObject(port1, "isUse");
		else
			cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		cJSON_AddNumberToObject(port1, "audioInputType", 0);
		cJSON_AddFalseToObject(port1, "isLocalPlay");
	}
	else if(type == QTK_UART_STATUS_SPEAKER){
		pv = strstr(buf,"VBOX3_SPK=");
		is_use= atoi(pv+10);
		read_register("/sys/bus/i2c/devices/3-0069/volume", &s);
		cJSON_AddNumberToObject(port1, "portType", 4);
		cJSON_AddNumberToObject(port1, "portId", 0xbb00003);
		cJSON_AddStringToObject(port1, "portName", "SPK");
		if(!is_use)
			cJSON_AddFalseToObject(port1, "isUse");
		else
			cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		cJSON_AddNumberToObject(port1, "audioInputType", 0);
		cJSON_AddTrueToObject(port1, "isLocalPlay");
	}else if(type == QTK_UART_STATUS_LINEIN){
		pv = strstr(buf,"VBOX3_LINEIN=");
		is_use= atoi(pv+13);
		pv = strstr(buf, "vbox3_agc_level=");
		s = atoi(pv + 16);
		pv = strstr(buf,"USE_LINEIN_MIC=");
		is_lineinmic = atoi(pv + 15);
		cJSON_AddNumberToObject(port1, "portType", 1);
		cJSON_AddNumberToObject(port1, "portId", 0xaa00002);
		cJSON_AddStringToObject(port1, "portName", "Line_IN");
		if(!is_use)
			cJSON_AddFalseToObject(port1, "isUse");
		else
			cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		if(!is_lineinmic)
			cJSON_AddNumberToObject(port1, "audioInputType", 0);
		else
			cJSON_AddNumberToObject(port1, "audioInputType", 1);
		cJSON_AddFalseToObject(port1, "isLocalPlay");
	}else if(type == QTK_UART_STATUS_LINEOUT){
		// if(access("/oem/qdreamer/qsound/lineout_pattern.txt",F_OK) == 0)
		// {
		// 	pv=file_read_buf("//oem/qdreamer/qsound/lineout_pattern.txt",&ret);
		// 	is_use=atoi(pv);
		// }
		pv = strstr(buf, "vbox3_agc_level=");
		s = atoi(pv + 16);
		cJSON_AddNumberToObject(port1, "portType", 5);
		cJSON_AddNumberToObject(port1, "portId", 0xbb00003);
		cJSON_AddStringToObject(port1, "portName", "LINE_OUT");
		cJSON_AddTrueToObject(port1, "isUse");
		cJSON_AddNumberToObject(port1, "gainLevel", s);
		cJSON_AddNumberToObject(port1, "audioInputType", 0);
		cJSON_AddFalseToObject(port1, "isLocalPlay");
	}
	char *json_str = cJSON_Print(port1);
	send_response(uc,&frame,(uint8_t*)json_str,strlen(json_str));
	cJSON_Delete(port1);
}
#endif
//读取寄存器
int write_register(const char *reg_path, int value) {
    int fd;
    int ret;
    char buf[32];

    fd = open(reg_path, O_WRONLY | O_SYNC);
    if (fd < 0) {
        perror("Open register for write failed");
        return -1;
    }

    snprintf(buf, sizeof(buf), "%d\n", value); 
    ret = write(fd, buf, strlen(buf));
    if (ret < 0) {
        perror("Write register failed");
        close(fd);
        return -1;
    }

    close(fd); 
    return 0;
}

int read_register(const char *reg_path, int *output_value) {
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

// 将 gain(dB) 映射到驱动的 7 档索引：0..6 => {+6,+4,+2,0,-2,-4,-6}
static int map_gain_to_level(float g_db){
    static const float levels[7] = { 6.f, 4.f, 2.f, 0.f, -2.f, -4.f, -6.f };
    int best = 0; float bestd = 1e9f;
    for(int i=0;i<7;i++){
        float d = fabsf(g_db - levels[i]);
        if(d < bestd){ bestd = d; best = i; }
    }
    return best; // 0..6
}

// 读取 /oem/qdreamer/qsound/eq.json，扫描 "gain": <float>，离散成 7 档，逐 band 写 /sys/bus/i2c/devices/3-0069/eq
int eq_apply_from_json_file_fallback(const char *json_path)
{
    // 1) 读文件到内存
    FILE *f = fopen(json_path, "rb");
    if(!f){ syslog(LOG_ERR,"[EQ][fallback] open %s fail: %s", json_path, strerror(errno)); return -1; }
    if(fseek(f, 0, SEEK_END)!=0){ fclose(f); return -2; }
    long sz = ftell(f);
    if(sz<=0 || sz>64*1024){ fclose(f); return -3; }
    if(fseek(f, 0, SEEK_SET)!=0){ fclose(f); return -4; }
    char *buf = (char*)malloc((size_t)sz+1);
    if(!buf){ fclose(f); return -5; }
    long rd = (long)fread(buf,1,(size_t)sz,f); fclose(f);
    if(rd != sz){ free(buf); return -6; }
    buf[sz] = '\0';

    // 2) 解析 JSON（wtk_json_parser）
    wtk_json_parser_t *jp = wtk_json_parser_new();
    if(!jp){ free(buf); return -7; }
    if(wtk_json_parser_parse(jp, buf, (int)sz) != 0){
        syslog(LOG_ERR, "[EQ][fallback] parse %s failed", json_path);
        wtk_json_parser_delete(jp); free(buf); return -8;
    }

    wtk_json_item_t *root = (jp && jp->json) ? jp->json->main : NULL;
    if(!root || root->type != WTK_JSON_ARRAY){
        syslog(LOG_ERR, "[EQ][fallback] root not array");
        wtk_json_parser_delete(jp); free(buf); return -9;
    }

    // 3) 逐条应用：freq→B；gain→L；code=0xBL；低频再写 0x6d
    int n = wtk_json_item_len(root);
    int ok_main = 0, ok_bass = 0, total = 0;
    for(int i=0; i<n; ++i){
        wtk_json_item_t *obj = wtk_json_array_get(root, i);
        if(!obj || obj->type != WTK_JSON_OBJECT) continue;

        wtk_json_item_t *jf = wtk_json_obj_get(obj, "freq", 4);
        wtk_json_item_t *jg = wtk_json_obj_get(obj, "gain", 4);
        if(!jf || !jg || jf->type != WTK_JSON_NUMBER || jg->type != WTK_JSON_NUMBER) continue;

        float freq = (float)jf->v.number;
        float gain = (float)jg->v.number;

        int B;
        if (freq <= 250.0f)        B = 0;   // 低频：≤250 → 69 + 6d
        else if (freq >= 4000.0f)  B = 2;   // 高频：≥4000 → 69
        else                       B = 1;   // 中频：251~3999 → 69

        int L = map_gain_to_level(gain);           // 0..6 → {+6,+4,+2,0,-2,-4,-6}
        uint8_t code = (uint8_t)(((B & 0x0F) << 4) | (L & 0x0F));

        syslog(LOG_INFO, "[EQ][fallback] freq=%.1fHz, gain=%.1fdB -> B=%d, L=%d -> code=0x%02X",
               freq, gain, B, L, code);

        // 主 EQ（0x69）必写
        if (strlen(g_eq_path_main) > 0) {
            if (sysfs_write_hex_u8(g_eq_path_main, code) == 0) {
                ok_main++;
            } else {
                syslog(LOG_ERR, "[EQ][fallback] write 0x%02X to %s failed", code, g_eq_path_main);
            }
        }

        // 低频额外写 低音 AMP（0x6d）
        if (B == 0 && strlen(g_eq_path_bass) > 0) {
            if (sysfs_write_hex_u8(g_eq_path_bass, code) == 0) {
                ok_bass++;
            } else {
                syslog(LOG_ERR, "[EQ][fallback] write 0x%02X to %s failed", code, g_eq_path_bass);
            }
        }
        total++;
    }

    wtk_json_parser_delete(jp);
    free(buf);

    syslog(LOG_INFO, "[EQ][fallback] done: total=%d, ok_main=%d, ok_bass=%d", total, ok_main, ok_bass);
    return (total>0 && ok_main>0) ? 0 : -10;
}

/* 载荷净化：去掉 UTF‑8 BOM、可疑的 4 字节 u32 头（LE/BE），然后扫描到 '[' 或 '{' */
static void sanitize_json_payload(const uint8_t *in, size_t in_len,
                                  const char **out_json, size_t *out_len) {
    const unsigned char *p = in;
    size_t n = in_len;

    /* 跳过 UTF-8 BOM: EF BB BF */
    if (n >= 3 && p[0]==0xEF && p[1]==0xBB && p[2]==0xBF) { p += 3; n -= 3; }

    /* 尝试剥掉前置的 u32 头（例如 03 00 00 00 或 00 00 00 03） */
    if (n >= 4) {
        uint32_t le = (uint32_t)p[0] | ((uint32_t)p[1]<<8) | ((uint32_t)p[2]<<16) | ((uint32_t)p[3]<<24);
        uint32_t be = (uint32_t)p[3] | ((uint32_t)p[2]<<8) | ((uint32_t)p[1]<<16) | ((uint32_t)p[0]<<24);
        /* 经验规则：若该数值在 1..64 之间，且后续出现 JSON 起始，很可能是“误加计数头” */
        if ((le >= 1 && le <= 64) || (be >= 1 && be <= 64)) {
            const unsigned char *q = p + 4; size_t r = n - 4;
            while (r > 0 && q[0] <= 0x20) { q++; r--; }    // 跳过空白
            if (r > 0 && (q[0] == '[' || q[0] == '{')) {  // 符合预期才剥头
                p += 4; n -= 4;
            }
        }
    }

    /* 最终保险：一路扫到真正的 JSON 起点 */
    while (n > 0 && p[0] != '[' && p[0] != '{') { p++; n--; }

    *out_json = (const char*)p;
    *out_len  = n;
}
/* 最小文件写入封装（原工程没有 qtk_write_file，这里补一个） */
static int qtk_write_file(const char* path, const void* buf, size_t len){
	FILE *f = fopen(path,"wb");
    if(!f){ return -1; }
    size_t w = fwrite(buf,1,len,f);
    fflush(f);
    int fd = fileno(f);
    if(fd>=0){ fsync(fd); }
    fclose(f);
    chmod(path,0666);
    return (w==len)?0:-2;
}
static void ackq_push(qtk_uart_client_t *uc, uint16_t code, uint8_t status)
{
    qtk_ack_node_t *n = (qtk_ack_node_t*)wtk_malloc(sizeof(qtk_ack_node_t));
    if (!n) return;
    n->code = code;
    n->status = status;
    wtk_blockqueue_push(&uc->ack_q, &n->q_n);
}
/* 等待指定事件码的ACK（兼容 0xXXXX 与 0x8XXX），超时返回-1 */
static int wait_for_ack(qtk_uart_client_t *uc, uint16_t expect_code, int timeout_ms, uint8_t *status_out)
{
    uint64_t deadline_ms = time_get_ms() + (timeout_ms > 0 ? (uint64_t)timeout_ms : 0);
    for (;;) {
        int remain = (timeout_ms > 0) ? (int)(deadline_ms - time_get_ms()) : -1; // -1 表示一直等
        if (timeout_ms > 0 && remain <= 0) return -1;

        wtk_queue_node_t *qn = wtk_blockqueue_pop(&uc->ack_q, (timeout_ms > 0 ? remain : -1), NULL);
        if (!qn) { // 超时
            return -1;
        }
        qtk_ack_node_t *an = data_offset2(qn, qtk_ack_node_t, q_n);

        int match = 0;
        if (an->code == expect_code) match = 1;
        else if (an->code == (expect_code | 0x8000)) match = 1; // 兼容 ACK=0x8xxx 的写法

        if (match) {
            if (status_out) *status_out = an->status;
            wtk_free(an);
            return 0; // 命中ACK
        } else {
            // 不是我想要的ACK，丢弃（或按需缓存）；为简洁起见，直接丢弃
            wtk_free(an);
            // 继续等
        }
    }
}

void msleep(unsigned long mSec)
{
	struct timeval tv;
	
	tv.tv_sec=0;
	tv.tv_usec=mSec;
	select(0, NULL, NULL, NULL, &tv);
}

#ifdef USE_3308
void myfun(int sig, siginfo_t *info, void *ptr)
{
	// if (once2)
	// {
	// 	once=0;
    // 	cp.pid_client = cp.shmaddr->pid;
	// 	wtk_debug("====>>>>cp.pid_client=%d   getpid=%d\n",cp.pid_client,getpid());
	// 	once2=0;
	// }
	once=1;
	char buf[32] = {0};
	char tmpbuf[1024]={0};
	char buf2[24]={0};
	char buf3[128]={0};
	int val,val2;
	int set = 0;
	char *pv=NULL;
	memcpy(buf, cp.shmaddr->buf, MAX);
	wtk_debug("====>>>>>read buf=[%s]\n", buf);
#if 0
	if(strncmp (buf,"LINEIN_ON" ,9) == 0)
	{
		send_audio_status_frame(lc, 0x01 , 0x1B,QTK_UART_STATUS_LINEIN);
	}else if(strncmp (buf,"LINEOUT_ON" ,10) == 0)
	{
		send_audio_status_frame(lc,0x01 , 0x1B ,QTK_UART_STATUS_LINEOUT);
	}else if(strncmp (buf,"LINEIN_OFF" ,10) == 0)
	{
		send_audio_status_frame(lc,0x01 , 0x17 ,QTK_UART_STATUS_LINEIN);
	}else if(strncmp (buf,"LINEOUT_OFF" ,11) == 0)
	{
		send_audio_status_frame(lc,0x01 , 0x17 ,QTK_UART_STATUS_LINEOUT);
	}else if(strncmp (buf,"ALARM_MIC" ,9) == 0)
	{
		uint8_t mic_alarm[2] = {0x01,0x00};
		send_active_response(lc,0x01 ,0x15 ,(uint8_t*)&mic_alarm,2);
	}else if(strncmp (buf,"ALARM_SPK" ,9) == 0)
	{
		uint8_t spk_alarm[2] = {0x03,0x00};
		send_active_response(lc ,0x01 ,0x15 ,(uint8_t*)&spk_alarm,2);
	}
#endif
	// wtk_debug("=====>>>>>>>>>>>>>>>>>>>>>myfun=%d\n",sig);
    return;
}

int qtk_proc_read(qtk_uart_client_t *uc)
{
	double tm;
	// 读取程序
	// char buf[12]={0};
	wtk_debug("====>>>>qtk_proc_read start\n");
	tm = time_get_ms();
	while(1)
	{
		// sleep(1);
		msleep(100000);
		tm = time_get_ms() - tm;
		if(tm > 1500)
		{
			return -1;
		}
		if(once ==1)
			break;
	}
	once=0;
	wtk_debug("read:%s\n", cp.shmaddr->buf);
	result=atoi(cp.shmaddr->buf);
	wtk_debug("=====>>>> result=%d\n",result);
	return 0;
}

int qtk_proc_read2(qtk_uart_client_t *uc)
{
	// 读取程序
	wtk_debug("====>>>>qtk_proc_read2 start\n");
	while(1)
	{
		sleep(1);
		if(once ==1)
			break;
	}
	once=0;
	wtk_debug("read:%s\n", cp.shmaddr->buf);
	memset(buf,0,sizeof(buf));
	memcpy(buf,cp.shmaddr->buf,strlen(cp.shmaddr->buf));
	
	wtk_debug("====>>>>buf=[%s]\n",buf);
	return 0;
}


int qtk_proc_write(qtk_uart_client_t *uc,char *data,int len)
{
	// 写入程序
	wtk_debug("input>[%.*s]\n",len,data);
	memset(cp.shmaddr->buf,0,MAX);
	memcpy(cp.shmaddr->buf, data, len);
	// wtk_debug("input>[%.*s]\n",len,cp.shmaddr->buf);
	// kill(cp.pid_client, SIGUSR1);
	union sigval tmp;
	tmp.sival_int = 100;
 // 给进程 pid，发送 SIGINT 信号，并把 tmp 传递过去
	sigqueue(cp.pid_client, SIGRTMIN, tmp);
	wtk_debug("====>>>>qtk_proc_write end once=%d\n",once);
	return 0;
}
int qtk_uart_client_get_arg(qtk_uart_client_t *uc, qtk_uart_type_t state) 
{
	int s=-1;
    if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0)
	{
		FILE *fn;
		fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r");
		char buf[1024];
		char buf2[32]={0};
		char *pv;
		int ret, val;
		float scale;

		wtk_debug("==================>>>>>>>%d\n",sizeof(buf));
		ret = fread(buf, 1, sizeof(buf), fn);

		// wtk_debug("============>>>>>>ret=%d [%.*s]\n",ret,ret,buf);
		switch (state)
		{
			case QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH:
			     pv = strstr(buf,"VBOX3_ANS=");
				 s=atoi(pv + 10);
				break;
			case QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH:
				pv = strstr(buf,"VBOX3_AGC=");
				s=atoi(pv + 10);
			break;
			case QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH:
				pv = strstr(buf, "VBOX3_AEC=");
                if (pv) {
                    s = atoi(pv + 10);
                    if (s == 0) {
                        // s = -1; 
                    } 
                    else {
                        pv = strstr(buf, "vbox3_aec_level=");
                        if (pv) {
                            int level = atoi(pv + 16);
                            if (level >= 1 && level <= 3) {
                                s = level;
                            } else {
                                s = 0x00;
                            }
                        } else {
                            s = 0x00;
                        }
                    }
                }
			break;
			case QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE:
				
			break;
			case QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE:
				// wtk_debug("------------------_>>>>>>>>>>>>>\n");
				// FILE *src_file, *dst_file;
				// char buffer[256];
				// char *volume_value = NULL;
				// size_t len = 0;
				// src_file = fopen("/sys/bus/i2c/devices/3-0069/volume", "r");
				// if (fgets(buffer, sizeof(buffer), src_file) == NULL) {
				// 	printf("错误: 读取设备文件失败: %s\n", strerror(errno));
				// 	fclose(src_file);
				// 	return -1;
				// }
				// fclose(src_file);
				// buffer[strcspn(buffer, "\n")] = '\0';
				// volume_value = buffer;
				// wtk_debug("读取到的音量值: %s\n", volume_value);
				// dst_file = fopen("/oem/qdreamer/qsound/woofvolume.txt", "w");
				// if (fprintf(dst_file, "%s\n", volume_value) < 0) {
				// 	printf("错误: 写入文件失败: %s\n", strerror(errno));
				// 	fclose(dst_file);
				// 	return -1;
				// }
				// fclose(dst_file);
				// // printf("内容已成功写入文件\n");
			    // // system("cat /sys/bus/i2c/devices/3-0069/volume | tee /oem/qdreamer/qsound/woofvolume.txt");
			    // // system("cat /sys/bus/i2c/devices/3-0069/volume | tee /oem/qdreamer/qsound/tweetervolume.txt");
				// if(access("/oem/qdreamer/qsound/woofvolume.txt",F_OK) == 0)
				// {
				// 	pv=file_read_buf("/oem/qdreamer/qsound/woofvolume.txt",&val);
				// 	s=atoi(pv);
				// 	wtk_debug("-------------------%d\n",s);
				// }
			break;
			case QTK_UART_TYPE_RECV_GET_LINEOUT_MODE:
			    if(access("/oem/qdreamer/qsound/lineout_pattern.txt",F_OK) == 0)
				{
					pv=file_read_buf("//oem/qdreamer/qsound/lineout_pattern.txt",&val);
					s=atoi(pv);
				}
			default:
				break;
		}
		
		fclose(fn);
		fn = NULL;
	}
	return s;
}

#endif

qtk_uart_client_t *qtk_uart_client_new(qtk_uart_client_cfg_t *cfg, wtk_log_t *log)
{
	qtk_uart_client_t *uc;
	char *data = NULL;
	int len;
	int ret;
	char tmpbuf[1024]={0};
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
	uc->msg_run=0;
	uc->trsn_run=0;
	uc->spk_alarm = 0;
	uc->report_run = 0;
    uc->report_busy = 0;
    uc->last_watch_bytes = 0;
    uc->upload_round = 0;
	uc->lineout_pattern = 0;
	resolve_eq_sysfs_paths();

#ifdef USE_LOG
	uc->log = wtk_log_new("/data/qtk_uart_client.log");
#endif

	wtk_blockqueue_init(&uc->input_q);
	wtk_blockqueue_init(&uc->msg_q);
	wtk_lockhoard_init(&uc->msg_hoard,offsetof(qtk_uart_client_msg_t,hoard_n),10,
			(wtk_new_handler_t)qtk_uart_client_msg_new,
			(wtk_delete_handler_t)qtk_uart_client_msg_delete,
			uc
			);
    wtk_thread_init(&uc->trsn_thread,(thread_route_handler)qtk_uart_client_trsn_run,(void*)uc);
#if 0
	wtk_thread_init(&uc->trsn_thread,(thread_route_handler)qtk_uart_client_send_run,(void*)uc);
#endif
	wtk_thread_set_name(&uc->trsn_thread,"trsn");

	wtk_thread_init(&uc->msg_thread,(thread_route_handler)qtk_uart_client_msg_run,(void *)uc);
	wtk_thread_set_name(&uc->msg_thread, "msg");
	// ACK队列初始化
    wtk_blockqueue_init(&uc->ack_q);

	if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
		FILE *fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
		ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
		pc = strstr(tmpbuf, "mic_shift2=");
		scale = wtk_str_atof(pc + 11, ret-(pc-tmpbuf)-12);				
		s=roundf((scale/6*100));
		uc->mic_shift2 = s;		
	}
	if(uc->cfg->use_uart){
		int i=0;
		while(i < 10)
		{
			lc->uart = qtk_uart_new(&(uc->cfg->uart));
			uc->uart = qtk_uart_new(&(uc->cfg->uart));
			if(uc->uart && lc->uart){break;}
			if(!uc->uart && lc->uart){sleep(1);}
			i++;
		}
		if(!uc->uart){
			wtk_debug("upload new failed.\n");
			wtk_log_warn0(uc->log, "upload new failed.");
			ret = -1;
			goto end;
		}

		uc->uart_buf = wtk_strbuf_new(3200, 1);
		uc->uart_buf2 = wtk_strbuf_new(256, 1);
	}
	uc->mac = wtk_strbuf_new(256,0);

#ifdef USE_3308
	///////////////////////
	cp.shmaddr=NULL;
	// 创建一个提取码
#ifdef USE_KTC3308
    if ((cp.key = ftok("/ktc/version.txt", 'z')) == -1)
#else
	if ((cp.key = ftok("/oem/qdreamer/qsound/version.txt", 'z')) == -1)
#endif
    {
        perror("ftok");
        return -1;
    }
    printf("===>>>key %d\n",cp.key);
    // 创建一个共享空间
    if ((cp.shmid = shmget(cp.key, SIZE, IPC_CREAT | 0666)) == -1)
    {
        perror("shmget");
        return -1;
    }
    printf("===>>>shmid %d\n",cp.shmid);
    // 设置共享空间地址
    if ((cp.shmaddr = shmat(cp.shmid, NULL, 0)) == (SHM *)-1)
    {
        perror("shmat");
        return -1;
    }

    // 注册信号
	cp.pid_client = cp.shmaddr->pid;
    cp.shmaddr->pid = getpid();
	wtk_debug("====>>>>cp.pid_client=%d   getpid=%d\n",cp.pid_client,getpid());
	// signal(SIGUSR2, myfun);
	// kill(cp.pid_client,SIGUSR2);
	struct sigaction act, oact;
	act.sa_sigaction = myfun; //指定信号处理回调函数
	sigemptyset(&act.sa_mask); // 阻塞集为空
	act.sa_flags = SA_SIGINFO; // 指定调用 signal_handler
 // 注册信号 SIGINT
	sigaction(SIGRTMIN+2, &act, &oact);

	union sigval tmp;
	tmp.sival_int = 100;
 // 给进程 pid，发送 SIGINT 信号，并把 tmp 传递过去
	sigqueue(cp.pid_client, SIGRTMIN+2, tmp);
#endif

#ifdef USE_AM34
// 生成唯一的key
    uc->key = ftok("/oem/uhan/qsound/is_sound.txt", 65);

    // 创建消息队列
    uc->msg_id = msgget(uc->key, 0666 | IPC_CREAT);
	if (uc->msg_id == -1)// 错误处理：msgget调用成功返回消息队列标识符，调用失败返回-1
	{
        wtk_debug("msgget failed with error: %d\n", errno);
    }
#endif
	/////////////////////

	wtk_debug("================>>>>>>>>>new ok!\n");
	ret = 0;
end:
	if(ret != 0){
		qtk_uart_client_delete(uc);
		uc = NULL;
	}
	return uc;
}

int qtk_uart_client_delete(qtk_uart_client_t *uc)
{
	wtk_debug("uc delete.\n");
	wtk_lockhoard_clean(&uc->msg_hoard);
	
	if(uc->uart){
		qtk_uart_delete(uc->uart);
	}

	if(uc->uart_buf){
		wtk_strbuf_delete(uc->uart_buf);
	}
	if(uc->uart_buf2){
		wtk_strbuf_delete(uc->uart_buf2);
	}

	if(uc->version) {
		wtk_strbuf_delete(uc->version);
	}
	if(uc->mac)
	{
		wtk_strbuf_delete(uc->mac);
	}
	if(uc->parser)
	{
		wtk_json_parser_delete(uc->parser);
	}
	wtk_queue_node_t *qn;
    while ((qn = wtk_blockqueue_pop(&uc->ack_q, 0, NULL)) != NULL)
	{
        qtk_ack_node_t *an = data_offset2(qn, qtk_ack_node_t, q_n);
        wtk_free(an);
    }

#ifdef USE_3308
	// 删除共享空间
    shmdt(cp.shmaddr);
    shmctl(cp.shmid, IPC_RMID, NULL);
#endif

#ifdef USE_AM34
	msgctl(uc->msg_id, IPC_RMID, NULL);
	perror("msgctl failed");
#endif
	wtk_free(uc);
	return 0;
}

int qtk_uart_client_start(qtk_uart_client_t *uc)
{
	if(0 == uc->msg_run)
	{
		uc->msg_run = 1;
		wtk_thread_start(&uc->msg_thread);
	}

    if(0 == uc->trsn_run)
    {
    	uc->trsn_run = 1;
        wtk_thread_start(&uc->trsn_thread);
    }
	// 启动日志上报线程
#ifdef chenggang
    if (0 == uc->report_run) 
    {
	uc->report_trigger_once = 0;   // 新增：清零一次性触发标志
        uc->report_run = 1;
        wtk_thread_init(&uc->report_thread, (thread_route_handler)qtk_uart_client_report_run, (void*)uc);
        wtk_thread_set_name(&uc->report_thread, "report");
        wtk_thread_start(&uc->report_thread);
    }
#endif
	return 0;
}

int qtk_uart_client_stop(qtk_uart_client_t *uc)
{
	if(1 == uc->trsn_run)
    {
    	uc->trsn_run = 0;
        wtk_blockqueue_wake(&uc->input_q);
        wtk_thread_join(&uc->trsn_thread);
		qtk_uart_client_clean_q(uc, &uc->input_q);
    }
	if(1 == uc->msg_run)
	{
		uc->msg_run = 0;
		wtk_blockqueue_wake(&uc->msg_q);
		wtk_thread_join(&uc->msg_hoard);
		qtk_uart_client_clean_q(uc, &uc->msg_q);
	}
	// 停止日志上报线程
#if 1
    if (1 == uc->report_run)
    {
        uc->report_run = 0;
        wtk_thread_join(&uc->report_thread);
    }
#endif
	return 0;
}

int qtk_uart_client_grep_start(char *data, int len, int *alen)
{
	
	if(len >= 4)
	{
		memcpy(alen, data, sizeof(int));
		wtk_debug("read data length, alen=%d\n", *alen);
		return 0; 
	}
	return -1;
}

int qtk_uart_client_msg_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
{
	qtk_uart_client_msg_t *msg;
	wtk_queue_node_t *qn;
	// qtk_uart_recv_frame_t *frame;
	while (uc->msg_run)
	{
		qn = wtk_blockqueue_pop(&uc->msg_q, -1, NULL);
		if(!qn){continue;}
		msg = data_offset2(qn, qtk_uart_client_msg_t, q_n);

		qtk_uart_client_on_uart(uc, msg->statID, msg->buf->data, msg->buf->pos);
		// handle_uart_frame(uc,frame);

		// if(msg)
		// {
		// 	qtk_uart_client_push_msg(uc, msg);
		// }
		if(&current_frame)
		{
			qtk_uart_client_push_msg(uc, &current_frame);
		}
	}
	
}
//新增：主动上报打帧工具（设备 → 主机的“主动”帧）
//沿用你现有响应帧头 `0x90 0x40` 作为设备上行帧头，CRC 使用你现有的 `calculateModbusCRC()`。
static int qtk_uart_client_send_active(qtk_uart_client_t *uc,uint16_t event_code,const uint8_t *data,uint16_t data_len)
{
    uint8_t hdr0 = RESPONSE_FRAME_HEADER_0; // 0x90
    uint8_t hdr1 = RESPONSE_FRAME_HEADER_1; // 0x40
    uint8_t ev[2] = { (uint8_t)(event_code & 0xFF), (uint8_t)(event_code >> 8) }; // 小端
    uint8_t len2[2] = { (uint8_t)(data_len & 0xFF), (uint8_t)(data_len >> 8) };
    uint8_t csum[2] = {0,0};


    int crc_data_len = 4 + data_len; // event(2)+len(2)+data
    uint8_t *crc_data = (uint8_t*)malloc(crc_data_len);
    if (!crc_data)
	    return -1;
    memcpy(crc_data, ev, 2);
    memcpy(crc_data+2, len2, 2);
    if (data_len && data)
	    memcpy(crc_data+4, data, data_len);
    uint16_t crc = calculateModbusCRC(crc_data, crc_data_len);
    free(crc_data);
    csum[0] = (uint8_t)(crc & 0xFF);
    csum[1] = (uint8_t)(crc >> 8);

    int frame_len = 2 + 2 + 2 + data_len + 2 + 1; // 9 + data_len
    uint8_t *send_buf = (uint8_t*)malloc(frame_len);
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
		memcpy(send_buf+pos, data, data_len); pos += data_len;
	}
    send_buf[pos++] = csum[0];
    send_buf[pos++] = csum[1];
    send_buf[pos++] = FRAME_FOOTER;
    int ret = qtk_uart_write2(uc->uart, (char*)send_buf, pos);
    free(send_buf);
    return (ret == pos) ? 0 : -3;
}
#ifdef chenggang
//目录体量统计 + 打包 `.tar.gz`
static uint64_t dir_total_size(const char *path)
{
    uint64_t total = 0;
    struct stat st;
    DIR *dir = opendir(path);
    if (!dir)
	    return 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL)
	{
        if (strcmp(ent->d_name, ".")==0 || strcmp(ent->d_name, "..")==0)
		    continue;
        char p[1024];
        snprintf(p, sizeof(p), "%s/%s", path, ent->d_name);
        if (lstat(p, &st) == 0)
		{
            if (S_ISREG(st.st_mode))
			    total += (uint64_t)st.st_size;
            else if (S_ISDIR(st.st_mode))
			    total += dir_total_size(p);
        }
    }
    closedir(dir);
    return total;
}
static int make_tar_gz(qtk_uart_client_t *uc, const char *src_dir, char out_path[], size_t out_sz)
{
    time_t t = time(NULL);
    struct tm tmv; localtime_r(&t, &tmv);
    char name[256];
    snprintf(name, sizeof(name), "uartlog_%04d%02d%02d_%02d%02d%02d.tar.gz",
    tmv.tm_year+1900, tmv.tm_mon+1, tmv.tm_mday, tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
    snprintf(out_path, out_sz, "/tmp/%s", name);
    char cmd[1024];
    // 打包目录下全部内容；若想只打 *.log 可改为 "-- * .log" 模式
    snprintf(cmd, sizeof(cmd), "tar -czf '%s' -C '%s' . 2>/dev/null", out_path, src_dir);
    int ret = system(cmd);
    if (ret != 0)
    {
        if (uc && uc->log) wtk_log_warn0(uc->log, "[report] tar build failed");
        return -1;
    }
    // 放宽权限，便于外部清理
    chmod(out_path, 0666);
    return 0;
}
#endif
//分包发送：开始包 / 数据包 / 结束包
#ifdef chenggang
static void le16_write(uint8_t *p, uint16_t v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; }
static void le32_write(uint8_t *p, uint32_t v){ p[0]=v&0xFF; p[1]=(v>>8)&0xFF; p[2]=(v>>16)&0xFF; p[3]=(v>>24)&0xFF; }
static void le64_write(uint8_t *p, uint64_t v){ for (int i=0;i<8;i++) p[i]=(uint8_t)(v>>(8*i)); }
static int send_file_in_chunks(qtk_uart_client_t *uc, const char *file_path)
{
    FILE *f = fopen(file_path, "rb");
    if (!f)
	    return -1;

    if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f); return -2;
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
    le16_write(start_payload+4, total_chunks);

    int start_ok = 0;
    for (int tries = 0; tries < 3 && !start_ok; ++tries) {
        if (qtk_uart_client_send_active(uc, EVT_LOG_UPLOAD_START, start_payload, sizeof(start_payload)) != 0) {
            if (uc && uc->log) wtk_log_warn0(uc->log, "[report] start frame send fail");
            usleep(200*1000);
            continue;
        }

        uint8_t st = 0xFF;
        if (wait_for_ack(uc, EVT_LOG_UPLOAD_START, 2000, &st) == 0 && st == 0x00) {
            start_ok = 1;   // 收到OK
        } else {
            if (uc && uc->log) wtk_log_warn0(uc->log, "[report] wait start-ack timeout or not OK, retry");
        }
    }

    if (!start_ok) {
        if (uc && uc->log) wtk_log_warn0(uc->log, "[report] abort: no start-ack");
        fclose(f);
        return -5;  // 中断上传
    }
    // 2) 数据包： [2B 序号][数据]
    uint8_t *buf = (uint8_t*)malloc(2 + LOG_DATA_MAX_PER_FRAME);
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
#endif
//日志上报线程体：轮询目录 → 达阈值 → 打包并上送
#ifdef chenggang
static int qtk_uart_client_report_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
{
    (void)thread;
    if (uc && uc->log)
	    wtk_log_log0(uc->log, "[report] thread start");
    uc->last_watch_bytes = dir_total_size(LOG_WATCH_DIR);
    while (uc->report_run)
	{
        uint64_t cur = dir_total_size(LOG_WATCH_DIR);
		/* 新增：检查一次性触发标志（被 0x0119 置位） */
        int trigger_now = 0;
        if (uc->report_trigger_once) 
		{
            trigger_now = 1;
            uc->report_trigger_once = 0;   // 消费掉本次触发
        }
        /* 触发条件：没有在忙，且（收到一次性触发 或 达到阈值且目录在变大） */
        if (!uc->report_busy && (trigger_now || (cur >= LOG_UPLOAD_THRESHOLD_BYTES && cur > uc->last_watch_bytes)))
        {
            uc->report_busy   = 1;
            uc->upload_round += 1;
            char tgz[512];
            if (make_tar_gz(uc, LOG_WATCH_DIR, tgz, sizeof(tgz)) == 0)
            {
                if (uc->log) 
				    wtk_log_log0(uc->log, "[report] upload start");
                int rc = send_file_in_chunks(uc, tgz);   // 里面会 wait_for_ack(...)
                if (uc->log) 
				    wtk_log_log0(uc->log, rc == 0 ? "[report] upload ok" : "[report] upload failed");
                unlink(tgz);
            }
            else
            {
                if (uc->log) 
			        wtk_log_log0(uc->log, "[report] tar.gz failed");
            }

            uc->report_busy = 0;
        }

        uc->last_watch_bytes = cur;
        usleep(LOG_SCAN_INTERVAL_MS*1000); 
    }
    if (uc && uc->log)
	    wtk_log_log0(uc->log, "[report] thread stop");
    return 0;
}
#endif
void qtk_uart_client_on_uart(qtk_uart_client_t *uc, qtk_uart_type_t state, char *data, int len)
{

}
int qtk_uart_client_trsn_run(qtk_uart_client_t *uc, wtk_thread_t *thread)
{
    uint8_t byte;
    int ret;
    uart_parse_state_t state = PARSE_STATE_HEADER1;

    qtk_uart_recv_frame_t current_frame = {0};
    uint16_t data_len = 0;
    uint16_t data_index = 0;
    uint16_t crc_expected = 0;
    uint8_t crc_data[256]; // 最大支持 256 字节数据
    int crc_pos = 0;
    int event_byte = 0;
    int len_byte = 0;
    int crc_byte = 0;
	int first=1;
    while (uc->trsn_run) {
		// wtk_debug("======>>>>>%p %p\n",uc,uc->uart);
        ret = qtk_uart_read(uc->uart, (char*)&byte, 1); // 逐字节读取
        if (ret <= 0) {
            usleep(10000); // 10ms
            continue;
        }
		if(first){
			wtk_debug("--------------->>>>>>>>>>>>>first_to_secendTime= %.2f\n",time_get_ms()-recvdata_time);
			recvdata_time = time_get_ms();
			first = 0;
		}
        // printf(" %02X ",byte); // 调试用
        switch (state) {
            case PARSE_STATE_HEADER1:
                if (byte == REQUEST_FRAME_HEADER_0) {
                    memset(&current_frame, 0, sizeof(current_frame));
                    current_frame.frame_header[0] = byte;
                    state = PARSE_STATE_HEADER2;
                }
                break;

            case PARSE_STATE_HEADER2:
                if (byte == REQUEST_FRAME_HEADER_1) {
                    current_frame.frame_header[1] = byte;
                    state = PARSE_STATE_EVENT_CODE;
                } else {
                    state = PARSE_STATE_HEADER1;
                }
                break;

            case PARSE_STATE_EVENT_CODE:
                current_frame.event_code[event_byte++] = byte;
                if (event_byte >= 2) {
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
                data_len =(current_frame.data_length[1] << 8) | current_frame.data_length[0];
                printf("RESP_STATE_DATA_LENGTH:date_len : %d\n", data_len);
                if (data_len > 0) {
                    current_frame.data = (uint8_t*)malloc(data_len);
                    if (!current_frame.data) {
                        state = PARSE_STATE_HEADER1;
                        break;
                    }
                }else{
					state = RESP_STATE_CHECKSUM;
					break;;
				}
                data_index = 0;
                state = RESP_STATE_DATA;
                break;
            case RESP_STATE_DATA:
                if (data_index < data_len) {
                    current_frame.data[data_index++] = byte;
                }
                printf("data_index : %d\n",data_index);
                if (data_index >= data_len) {
                    printf("RESP_STATE_DATA:date_len : %d\n",data_len);
                    state = RESP_STATE_CHECKSUM;
                }
                break;

            case RESP_STATE_CHECKSUM:
                current_frame.checksum[0] = byte;
                // printf("current_frame.checksum[0]: %02X\n",current_frame.checksum[0] ); 
                state = RESP_STATE_CHECKSUM_2;

                break;
            case RESP_STATE_CHECKSUM_2:
                current_frame.checksum[1] = byte;
                // printf("current_frame.checksum[1]: %02X\n",current_frame.checksum[1] ); 
                state = RESP_STATE_FOOTER;
                break;
            case RESP_STATE_FOOTER:
                if (byte == FRAME_FOOTER) {
                    current_frame.frame_footer = byte;
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
                    } else {
                        wtk_debug("CRC error: %04X vs %04X\n", recv_crc, calc_crc);
                    }
                }

                if (current_frame.data) {
                    free(current_frame.data);
                    current_frame.data = NULL;
                }
                state = PARSE_STATE_HEADER1;
				first=1;
                break;
        }
    }

    return 0;
}

void qtk_uart_client_update_params(qtk_uart_client_t *uc, char *data, int len)
{
	wtk_json_parser_t *parser = NULL;
	wtk_json_item_t *json = NULL;
	wtk_queue_node_t *qn;
	int reboot = 0;
	int reset_cache = 0;
	int ret;
	char sdate[24];
	char stime[24];

	if(len <= 0) {
		return;
	}

	parser = wtk_json_parser_new();
	if(!parser){
		return ;
	}

	wtk_debug("[%.*s]\n",len,data);
	ret = wtk_json_parser_parse(parser, data, len);
	if(ret == 0) {
		json = wtk_json_obj_get_s(parser->json->main, "record_tm");
		if(json && json->type == WTK_JSON_NUMBER) {
			uc->record_tm = json->v.number;
		}
		
		json = wtk_json_obj_get_s(parser->json->main, "channel");
		if(json && json->type == WTK_JSON_NUMBER) {
			uc->record_chn = json->v.number;
		}

	} else {
		wtk_debug("[WARN] uart recv params parser failed.\n");
		if(uc->log) {
			wtk_log_warn0(uc->log, "[WARN] *** device recv params parser failed, perhaps wrong json string");
		}
	}
	wtk_json_parser_delete(parser);
}

#ifdef USE_3308

#endif

int qtk_uart_client_set_volume(qtk_uart_client_t * uc, char *buf, int len)
{
    int isok = 0;
    if (access(UART_CFG_PATH, F_OK) == 0 && access(UART_CFG_PATH, W_OK) == 0)
    {
        FILE *uf;
        char tmpbuf[1024] = {0};
        char buf2[24] = {0};
        char *pv;
        int ret, val;
        int use_agc = -1;

        // 打开配置文件
        uf = fopen(UART_CFG_PATH, "r+");
        ret = fread(tmpbuf, sizeof(tmpbuf), 1, uf);
        wtk_debug("================fread=[%s]\n", tmpbuf);
        pv = strstr(tmpbuf, "VBOX3_AGC=");
        use_agc = atoi(pv + 10);
        wtk_debug("=====================>>>>>>use_agc=%d\n", use_agc);
        if (!use_agc)
        {
            if (len == 1)
            {
                float set;
                set = (15.5 - 1.2) * (buf[0] / 100.0) + 1.0;
                wtk_debug("==============>>>>SET_MICVOLUME=%f\n", set);
                pv = strstr(tmpbuf, "mic_shift2=");
                if (pv)
                {
                    wtk_debug("============>>>>>>>>old=[%s]\n", pv);
                    sprintf(buf2, "mic_shift2=%0.1f;", set);
                    memset(pv, 0, strlen(pv));
                    memcpy(pv, buf2, strlen(buf2));
                    fseek(uf, 0, SEEK_SET);
                    ret = fwrite(tmpbuf, strlen(tmpbuf), 1, uf);
                    wtk_debug("================fwrite=%d [%.*s]\n", ret, strlen(tmpbuf), tmpbuf);
                }
            }
        }
        else
        {
            isok = -2;
        }
        fflush(uf);
		{
			int fd=fileno(uf);
			if(fd>=0){fsync(fd);}
		}
        fclose(uf);
        // system("sync");
        uf = NULL;
    }
    else
    {
        isok = -2;  // 文件不可访问
    }

    return isok;
}

int qtk_uart_client_isRunning()
{
	int ret = 0;
	int count=0;
	FILE *fstream=NULL;

	char buff[1024] = {0};
	char input[1024] = {0};

	wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>\n");
#ifdef USE_AM60
	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_am60\" | wc -l");
#else
#ifdef USE_802A
	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_802\" | wc -l");
#else
#ifdef USE_BMC
	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_bmc\" | wc -l");
#else
	snprintf(input, 1024, "ps -ef | grep -w \"Qsound_3308\" | wc -l");
#endif
#endif
#endif
	if(NULL==(fstream=popen(input, "r")))
	{
		fprintf(stderr,"execute command failed: %s", strerror(errno));
		return -1;
	}

	while(NULL!=fgets(buff, sizeof(buff), fstream))
	{
		wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>\n");
		count = atoi(buff);
		wtk_debug("=========================>>>>>>>>>>>>>>>>>>>>count=%d\n",count);
		if(count > 2)
		{
			ret = 1;
		}
	}
	pclose(fstream);
	return ret;
}

#ifndef  KTC_K72A
int uart_skip_channels[7]={7,6,5,4,3,2,1};
#else
int uart_skip_channels[2]={10,11};
#endif
void handle_uart_frame(qtk_uart_client_t *uc, qtk_uart_recv_frame_t *frame)
{
	qtk_uart_client_msg_t *msg;
	wtk_queue_node_t *qn;
	qtk_uart_type_t data_type = -1;
	FILE *fn;
	uint8_t normal=0x00;
	uint8_t faile=0xFE;
	int ret;
	char tmpbuf[1024]={0};
	char *pc;
	int s;
	int getresult;
	int ucnt=0;
	char set_buf[128]={0};
	char *upresult;
	int ulen;
	int timeout_s = 5;
	int resp;
	int len;
	struct stat file_info;
	const char *in_filename = "/oem/qdreamer/qsound/audioinput_infor.txt";
	const char *out_filename = "/oem/qdreamer/qsound/audiooutput_infor.txt";
	uint8_t cehsi = 0x00;
	uint8_t response;
    uint16_t data_len = frame->data_length[0] | (frame->data_length[1] << 8);
    uint16_t event_code = frame->event_code[1] | (frame->event_code[0] << 8);
	double utime=time_get_ms();
    wtk_debug("Received event_code: 0x%04X\n", event_code);
	wtk_debug("-------=========================================---->>>>>\n");
    switch (event_code) {
#ifdef chenggang
		/* ===== 日志上报：开始包 ACK（兼容两种码）===== */
    case EVT_LOG_UPLOAD_START:           // 若对端回同码作为ACK
    case EVT_LOG_UPLOAD_START_ACK: {     // 若对端回 0x8120 作为ACK
        uint8_t st = 0x00;
        if (data_len >= 1 && frame->data) st = frame->data[0];
        ackq_push(uc, event_code, st);
        //  这里不要 send_response(...)，否则可能形成请求-响应死循环 ★★
        break;
    }

    /* （可选）结束包ACK；如果你的协议不要求，就可以不加 */
    case EVT_LOG_UPLOAD_END:
    case EVT_LOG_UPLOAD_END_ACK: {
        uint8_t st = 0x00;
        if (data_len >= 1 && frame->data) st = frame->data[0];
        ackq_push(uc, event_code, st);
        break;
    }
#endif
        case 0x0101:  // QTK_UART_TYPE_RECV_SPEAKER_JUDGMENT - 扬声器音频检测判断
		#ifndef CESHI
			// isnot_spk_count++;
			// wtk_debug("------------------_>>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// getresult = qtk_uart_client_isRunning();
			// wtk_debug("------------------------------------->>>>>\n");
			// if (getresult == 1) {
			// 	wtk_debug("------------------------------------->>>>>\n");
			// 	ret = 1;
			// 	ucnt = 0;
			// 	while (ret != 0) {
			// 		qtk_proc_write(uc,"SPK_AUDIO_CHECK",16);
			// 		// ret=qtk_proc_read(uc);
			// 		if(++ucnt > 0){break;}
			// 			wtk_debug("------------------------------------->>>>>\n");
			// 	}
			// }
			// startspk_time = time(NULL);
			// while (time(NULL) - startspk_time < timeout_s) {
				if (access("/oem/qdreamer/qsound/spkcheck_result.txt", F_OK) == 0){
					// 文件存在，读取文件内容
					//system("chmod 777 /oem/qdreamer/qsound/spkcheck_result.txt");
					chmod("/oem/qdreamer/qsound/spkcheck_result.txt",0777);
					upresult = file_read_buf("/oem/qdreamer/qsound/spkcheck_result.txt", &ulen);
					resp = atoi(upresult);
					// wtk_debug("------------------------resp = %d\n",resp);
					if (resp == 1) {
						response = 0x01;
						send_response(uc, frame, &response,1); // 静音
						wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					} else if (resp == 2) {
						response = 0x02;
						send_response(uc, frame, &response,1); // 爆音
						wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					} else {
						response = 0x00;
						send_response(uc, frame, &response,1); // 正常
						wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					}
					// unlink("/oem/qdreamer/qsound/spkcheck_result.txt");
					//system("rm /oem/qdreamer/qsound/spkcheck_result.txt");
					wtk_log_log0(uc->log,"SPK_AUDIO_CHECK finished!\n");
					wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
					break;
				}
				usleep(10000); // 10ms
			// }
			#else
				send_response(uc, frame, &cehsi,1); // 正常
			#endif
			// wtk_log_log0(uc->log,"ERROR:SPK_AUDIO_CHECK,timeout!\n");
			break;
        case 0x0102:  // QTK_UART_TYPE_RECV_MIC_JUDGMENT - mic音频检测判断
		#ifndef CESHI
			// isnot_spk_count++;
			// wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// getresult = qtk_uart_client_isRunning();
			// if (getresult == 1) {
			// 	ret = 1;
			// 	ucnt = 0;
			// 	while (ret != 0) {
			// 		// wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// 		qtk_proc_write(uc,"MIC_AUDIO_CHECK",16);
			// 		// wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// 		// ret=qtk_proc_read(uc);
			// 		if(++ucnt > 0){break;}
			// 	}
			// }
			// time_t startmic_time = time(NULL);
			// while (time(NULL) - startmic_time < timeout_s) {
			if (access("/oem/qdreamer/qsound/miccheck_result.txt", F_OK) == 0){
				chmod("/oem/qdreamer/qsound/miccheck_result.txt",0777);
				//system("chmod 777 /oem/qdreamer/qsound/miccheck_result.txt");
				upresult = file_read_buf("/oem/qdreamer/qsound/miccheck_result.txt", &ulen);
				resp = atoi(upresult);
				wtk_debug("------------------------resp = %d\n",resp);
				if (resp == 1) {
					response = 0x01;
					send_response(uc, frame, &response,1); // 静音
				} else if (resp == 2) {
					response = 0x02;
					send_response(uc, frame, &response,1); // 爆音
				} else {
					response = 0x00;
					send_response(uc, frame, &response,1); // 正常
				}
				// unlink("/oem/qdreamer/qsound/miccheck_result.txt");
				wtk_log_log0(uc->log,"MIC_AUDIO_CHECK finished!\n");
				// break;
			}else{
				send_response(uc, frame, &faile,1);
			}
			// }
			wtk_log_log0(uc->log,"ERROR:MIC_AUDIO_CHECK timeout!\n");
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
			#else
				send_response(uc, frame, &cehsi,1); // 正常
			#endif
			break;
		#ifndef SYSFS_EQ_PATH
        #define SYSFS_EQ_PATH "/sys/bus/i2c/devices/3-0069/eq"
        #endif

		case 0x0103:  /* QTK_UART_TYPE_RECV_MIC_JUDGMENT_OUTPUT_EQ_ADJUSTMENT - 输出EQ设置 */
{
    /* 1) 基本校验 */
    if (data_len == 0 || data_len > 4096 || frame->data == NULL){
        uint8_t fail = 0xFE;
        wtk_debug("[EQ][0103] invalid payload, len=%d\n", data_len);
        send_response(uc, frame, &fail, 1);
        break;
    }

    /* 2) 载荷净化 */
    const char *san_json = NULL;
    size_t      san_len  = 0;
    sanitize_json_payload((const uint8_t*)frame->data, (size_t)data_len, &san_json, &san_len);
    if (san_len == 0){
        uint8_t fail = 0xFE;
        wtk_debug("[EQ][0103] sanitize failed\n");
        send_response(uc, frame, &fail, 1);
        break;
    }

    /* 3) 落盘 eq.json（保留一份最新设置） */
    if (qtk_write_file(QTK_EQ_JSON_PATH, san_json, (int)san_len) != 0){
        uint8_t fail = 0xFE;
        wtk_debug("[EQ][0103] write %s failed\n", QTK_EQ_JSON_PATH);
        send_response(uc, frame, &fail, 1);
        break;
    }

    /* 4) 当前线程直接应用（不再通知后端 / 不再兜底） */
    int rc = apply_eq_from_json_text_wtk(san_json, (int)san_len);
    if (rc != 0) {
        uint8_t fail = 0xFE;
        wtk_debug("[EQ][0103] apply failed, rc=%d\n", rc);
        send_response(uc, frame, &fail, 1);
        break;
    }

    /* 5) 正常应答 */
    { uint8_t ok = 0x00; send_response(uc, frame, &ok, 1); }
    break;
}

        case 0x0105:  // QTK_UART_TYPE_RECV_SET_DENOISE_SWITCH - 智能降噪开关设置
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);	
			isnot_spk_count++;
			if(frame->data[0]==0x00){
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					wtk_debug("================fread=[%s]\n",tmpbuf);
					if(frame->data[0]==0x00){
						pc=strstr(tmpbuf,"VBOX3_ANS=");
					}
					s=pc-tmpbuf;
					tmpbuf[s+10]='0';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					fflush(fn);
					
				}
				getresult = qtk_uart_client_isRunning();
				if(getresult == 1){
					ret =1;
					ucnt=0;
					while(ret != 0){
						#if 1
						qtk_proc_write(uc,"SET_MICANC_OFF",15);
						#endif
						// ret=qtk_proc_read(uc);
						if(++ucnt > 0){break;}
					}
				}else{
					result=0;
				}
				if(ret == -1){
					result = -2;
				}
				wtk_log_log0(uc->log,"close anc!\n");
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			}else if (frame->data[0]==0x01){
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					wtk_debug("================fread=[%s]\n",tmpbuf);
					if(frame->data[0]==0x01){
						pc=strstr(tmpbuf,"VBOX3_ANS=");
					}
					s=pc-tmpbuf;
					tmpbuf[s+10]='1';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					fflush(fn);
				}
				getresult = qtk_uart_client_isRunning();
				if(getresult == 1){
					ret =1;
					ucnt=0;
					while(ret != 0){
						#if 1
						qtk_proc_write(uc,"SET_MICANC_ON",14);
						#endif
						// ret=qtk_proc_read(uc);
						if(++ucnt > 0){break;}
					}
				}else{
					result=0;
				}
				if(ret == -1){
					result = -2;
				}
				wtk_log_log0(uc->log,"open anc!\n");
			}
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_strbuf_reset(uc->uart_buf2);
			{
				int fd = fileno(fn);
				if (fd >= 0) { fsync(fd); }
			}
			fclose(fn);
			fn=NULL;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
            #else
				send_response(uc, frame, &cehsi,1); // 正常
			#endif
			break;
            
        case 0x0104:  // QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH - 智能降噪开关获取
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH);
            wtk_debug("anc status: %d\n", result);
            // wtk_strbuf_reset(uc->uart_buf2);
			if(result == 0){
				response = 0x00;
			}else if (result == 1){
				response = 0x01;
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            wtk_debug("anc status: 0x%02X\n", response);
            send_response(uc, frame, &response,1);
			response = 0x00;
			wtk_log_log0(uc->log,"Get AGC_switch success!\n");
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;
            
        case 0x0107:  // QTK_UART_TYPE_RECV_SET_GAIN_CONTROL_SWITCH - 自动增益开关设置
			#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);	
			isnot_spk_count++;
			if(frame->data[0]==0x00){
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					wtk_debug("================fread=[%s]\n",tmpbuf);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					if(frame->data[0]==0x00){
						pc=strstr(tmpbuf,"VBOX3_AGC=");
					}
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					s=pc-tmpbuf;
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					tmpbuf[s+10]='0';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					fflush(fn);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				}
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				getresult = qtk_uart_client_isRunning();
				if(getresult == 1){
					ret =1;
					ucnt=0;
					while(ret != 0){
						qtk_proc_write(uc,"SET_MICAGC_OFF",15);
						// ret=qtk_proc_read(uc);
						if(++ucnt > 0){break;}
					}
				}else{
					result=0;
				}
				if(ret == -1){
					result = -2;
				}
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				wtk_log_log0(uc->log,"close agc function!\n");
			}else if (frame->data[0]==0x01){
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					if(frame->data[0]==0x01){
						pc=strstr(tmpbuf,"VBOX3_AGC=");
					}
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					s=pc-tmpbuf;
					tmpbuf[s+10]='1';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					fflush(fn);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				}

				getresult = qtk_uart_client_isRunning();
				if(getresult == 1){
					ret =1;
					ucnt=0;
					while(ret != 0){
						qtk_proc_write(uc,"SET_MICAGC_ON",14);
						// ret=qtk_proc_read(uc);
						if(++ucnt > 0){break;}
					}
				}else{
					result=0;
				}
				if(ret == -1){
					result = -2;
				}
				wtk_log_log0(uc->log,"open agc function!\n");
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// system("sync");
			{
               int fd =fileno(fn);
				if(fd>=0){fsync(fd);}
			}
			wtk_strbuf_reset(uc->uart_buf2);
			fclose(fn);
			fn=NULL;
			// system("sync");
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;
            
        case 0x0106:  // QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH - 自动增益开关获取
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH);
            wtk_debug("agc status: %d\n", result);
            wtk_strbuf_reset(uc->uart_buf2);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            if(result == 0){
				response = 0x00;
			}else if (result == 1){
				response = 0x01;
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, &response,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			response = 0x00;
			wtk_log_log0(uc->log,"Get AGC_switch sucess!\n");
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
         #else		 
			send_response(uc, frame, &cehsi,1); // 正常
		 #endif
			break;     
        case 0x0109:  // QTK_UART_TYPE_RECV_SET_ECHO_INTENSITY_SWITCH - 回声抑制强度设置
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			if (frame->data[0] == 0x00) {
			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0) {
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r+");
				ret = fread(tmpbuf, sizeof(tmpbuf), 1, fn);
				wtk_debug("================fread=[%s]\n", tmpbuf);
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);

				pc = strstr(tmpbuf, "VBOX3_AEC=");
				s = pc - tmpbuf;
				tmpbuf[s + 10] = '0';
				fseek(fn, 0, SEEK_SET);
				ret = fwrite(tmpbuf, strlen(tmpbuf), 1, fn);
				wtk_debug("+========>>>>>>s=%d pc=%c\n", s, *pc);
				fflush(fn);
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// 发送关闭指令

			getresult = qtk_uart_client_isRunning();
			if (getresult == 1) {
				ret = 1;
				ucnt = 0;
				while (ret != 0) {
					qtk_proc_write(uc, "SET_MICAEC_OFF", 15);
					// ret = qtk_proc_read(uc);
					if (++ucnt > 0) break;
				}
			}
			wtk_log_log0(uc->log,"关闭回声抑制功能!\n");
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
		} 
		else if (frame->data[0] == 0x01 || frame->data[0] == 0x02 || frame->data[0] == 0x03) {
			uint8_t aec_level = frame->data[0]; // 0x01=低, 0x02=中, 0x03=高
			if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0) {
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				fn = fopen("/oem/qdreamer/qsound/uart.cfg", "r+");
				ret = fread(tmpbuf, sizeof(tmpbuf), 1, fn);
				wtk_debug("================fread=[%s]\n", tmpbuf);
				pc = strstr(tmpbuf, "VBOX3_AEC=");
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				if (pc) {
					s = pc - tmpbuf;
					tmpbuf[s + 10] = '1';
				}
				pc = strstr(tmpbuf, "vbox3_aec_level=");
				if (pc) {
					s = pc - tmpbuf;
					tmpbuf[s + 16] = '0' + (aec_level - 0x00); // 转换为字符'1'/'2'/'3'
				}
				fseek(fn, 0, SEEK_SET);
				ret = fwrite(tmpbuf, strlen(tmpbuf), 1, fn);
				fflush(fn);
				wtk_debug("已设置AEC等级: %d\n", aec_level);
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			getresult = qtk_uart_client_isRunning();
			if (getresult == 1) {
				ret = 1;
				ucnt = 0;
				while (ret != 0) {
					char cmd_buf[32];
					if (aec_level == 0x01){ 
						snprintf(cmd_buf, sizeof(cmd_buf), "SET_MICAEC_ON_LOW");
						wtk_log_log0(uc->log,"设置回声抑制强度：低!\n");
					}
					else if (aec_level == 0x02){
						snprintf(cmd_buf, sizeof(cmd_buf), "SET_MICAEC_ON_MID");
						wtk_log_log0(uc->log,"设置回声抑制强度：中!\n");
					}
					else if(aec_level == 0x03) {
						snprintf(cmd_buf, sizeof(cmd_buf), "SET_MICAEC_ON_HIGH");
						wtk_log_log0(uc->log,"设置回声抑制强度：高!\n");
					}
					qtk_proc_write(uc, cmd_buf, strlen(cmd_buf));
					// ret = qtk_proc_read(uc);
					if (++ucnt > 0) break;
				}
			}
		}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// system("sync");
			{
				int fd =fileno(fn);
				if(fd>=0){fsync(fd);}
			}
			wtk_strbuf_reset(uc->uart_buf2);
			fclose(fn);
			fn=NULL;
			// system("sync");
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;     
        case 0x0108:  // QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH - 回声抑制强度获取
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
         	result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH);
			// uint8_t strong_echo = (result == -1) ? 0x00 : (uint8_t)result;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("AEC Response:AEC level %d\n", result);
			if(result == 0){
				response = 0x00;
			}else if (result == 1){
				response = 0x01;
			}else if (result == 2){
				response = 0x02;
			}else if (result == 3){
				response = 0x03;
			}
            send_response(uc, frame, &response,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			response = 0x00;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;
            
        case 0x010A:  // QTK_UART_TYPE_RECV_GET_LIST_AUDIO_INPUT_PORTS - 获取音频输入口列表
		#ifndef CESHI
		wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
		#if 1
			isnot_spk_count++;
			ret=0;
			getresult = qtk_uart_client_isRunning();
			if(getresult == 1){
				ret =1;
				ucnt=0;
				while(ret != 0){
					qtk_proc_write(uc,"INPUT_INFOR",12);
					// ret=qtk_proc_read(uc);
					if(++ucnt > 0){break;}
				}
			}
			startspk_time = time(NULL);
			while (time(NULL) - startspk_time < timeout_s) {
		#endif
				if (access("/oem/qdreamer/qsound/audioinput_infor.txt", F_OK) == 0){
					if (stat(in_filename, &file_info) == -1) {
						perror("stat failed");
            			send_response(uc, frame, (uint8_t*)upresult,ulen);
					}
					if(file_info.st_size > 0){
						wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
						//system("chmod 777 /oem/qdreamer/qsound/audioinput_infor.txt");
						chmod("/oem/qdreamer/qsound/audioinput_infor.txt",0777);
						upresult = file_read_buf("/oem/qdreamer/qsound/audioinput_infor.txt", &ulen);
						remove("/oem/qdreamer/qsound/audioinput_infor.txt");
						//system("rm /oem/qdreamer/qsound/audioinput_infor.txt");
						// wtk_log_log0(uc->log,"SPK_AUDIO_CHECK finished!\n");
						ret=1;
						wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
						send_response(uc, frame, (uint8_t*)upresult,ulen);
						wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
						break;
					}
				}
	#if 1
				usleep(100); // 10ms
			}
			if(!ret){
				send_response(uc, frame, &faile,1);
			}
	#endif 
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
		
			break;  
        case 0x010B:  // QTK_UART_TYPE_RECV_GET_LIST_AUDIO_OUTPUT_PORTS - 获取音频输出口列表
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
		#if 1    
		isnot_spk_count++;
		ret=0;
			getresult = qtk_uart_client_isRunning();
			if(getresult == 1){
				ret =1;
				ucnt=0;
				while(ret != 0){
					qtk_proc_write(uc,"OUTPUT_INFOR",13);
					// ret=qtk_proc_read(uc);
					if(++ucnt > 0){break;}
				}
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			startspk_time = time(NULL);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			while (time(NULL) - startspk_time < timeout_s) {
	#endif	
				if (access("/oem/qdreamer/qsound/audiooutput_infor.txt", F_OK) == 0){
					if (stat(out_filename, &file_info) == -1) {
						perror("stat failed");
            			send_response(uc, frame, (uint8_t*)upresult,ulen);
					}
					if(file_info.st_size > 0){
						chmod("/oem/qdreamer/qsound/audiooutput_infor.txt",0777);
						//system("chmod 777 /oem/qdreamer/qsound/audiooutput_infor.txt");
						wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
						upresult = file_read_buf("/oem/qdreamer/qsound/audiooutput_infor.txt", &ulen);
						remove("/oem/qdreamer/qsound/audiooutput_infor.txt");
						// system("rm /oem/qdreamer/qsound/audiooutput_infor.txt");

						// wtk_log_log0(uc->log,"SPK_AUDIO_CHECK finished!\n");
						ret=1;
						wtk_debug("----------------------now_delay = %.2f  %f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
						send_response(uc, frame, (uint8_t*)upresult,ulen);
						wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
						break;
					}
				}
	#if 1
				usleep(100); // 10ms
			}
			if(!ret){
				send_response(uc, frame, &faile,1);
			}
	#endif
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
			#else

				send_response(uc, frame, &cehsi,1); // 正常
			#endif
            break;
            
        case 0x010C:  // QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_MIC - 启用禁用MIC
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			getresult = qtk_uart_client_isRunning();
			if(frame->data[4]==0x00)
			{
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					pc=strstr(tmpbuf,"VBOX3_MIC=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+10]='0';
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							qtk_proc_write(uc,"UNENABLE_MIC",13);
							// ret=qtk_proc_read(uc);
							if(++ucnt > 0){break;}
						}
					}
				}
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			}else if(frame->data[4] == 0x01){
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					pc=strstr(tmpbuf,"VBOX3_MIC=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+10]='1';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							qtk_proc_write(uc,"ENABLE_MIC",11);
							// ret=qtk_proc_read(uc);
							if(++ucnt > 0){break;}
						}
					}

				}

			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// system("sync");
			{
				int fd =fileno(fn);
				if(fd>=0){fsync(fd);}
			}
			fclose(fn);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// system("sync");
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
			#if 0
			send_audio_status_frame(uc,0x01, 0x16 ,QTK_UART_STATUS_MIC);
            #endif
			#else
				send_response(uc, frame, &cehsi,1); // 正常
			#endif
			break;
            
        case 0x010D:  // QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_SPK - 启用禁用SPK
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			getresult = qtk_uart_client_isRunning();
			if(frame->data[4]==0x00)
			{
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					pc=strstr(tmpbuf,"VBOX3_SPK=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+10]='0';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
							qtk_proc_write(uc,"UNENABLE_SPK",13);
							// ret=qtk_proc_read(uc);
							wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
							if(++ucnt > 0){break;}
						}
					}
				}
			}else if(frame->data[4] == 0x01){
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					pc=strstr(tmpbuf,"VBOX3_SPK=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+10]='1';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
							qtk_proc_write(uc,"ENABLE_SPK",11);
							// ret=qtk_proc_read(uc);
							wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
							if(++ucnt > 0){break;}
						}
					}
				}
			}
			
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// system("sync");
			{
				int fd =fileno(fn);
				if(fd>=0){fsync(fd);}
			}
			
			fclose(fn);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			// system("sync");
			isnot_spk_count++;
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
			#if 0
			send_audio_status_frame(uc,0x01, 0x16 ,QTK_UART_STATUS_SPEAKER);
			#endif
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif 
		    break;
            
        case 0x010E:  // QTK_UART_TYPE_RECV_LINE_IN_CONTROL - line in本地输出控制(本地扩音)
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			getresult = qtk_uart_client_isRunning();
			if(frame->data[4] == 0x00)
			{
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					pc=strstr(tmpbuf,"VBOX3_LINEIN=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+13]='0';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							qtk_proc_write(uc,"UNENABLE_LINEIN_KWS",20);
							// ret=qtk_proc_read(uc);
							if(++ucnt > 0){break;}
						}
					}
				}
			}else if(frame->data[4] == 0x01){
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					pc=strstr(tmpbuf,"VBOX3_LINEIN=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+13]='1';
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							wtk_debug("------------------------->>>>>>>>>>>>>>>>>\n");
							qtk_proc_write(uc,"ENABLE_LINEIN_KWS",18);
							// ret=qtk_proc_read(uc);
							if(++ucnt > 0){break;}
						}
					}
				}
			}
			wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);fclose(fn);
			wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
			#if 0
			send_audio_status_frame(uc,0x01, 0x16 ,QTK_UART_STATUS_LINEIN);
			#endif
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif    
			break;
            
        case 0x010F:  // QTK_UART_TYPE_RECV_SET_INPUT_TYPE - 设置输入口类型
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			getresult = qtk_uart_client_isRunning();
			if(frame->data[4] == 0x00)
			{
				wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					pc=strstr(tmpbuf,"USE_LINEIN_MIC=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+15]='1';
					pc=strstr(tmpbuf,"USE_LINEIN_SPK=");
					s=pc-tmpbuf;
					tmpbuf[s+15]='0';
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							qtk_proc_write(uc,"ENABLE_LINEIN_MIC",18);
							// ret=qtk_proc_read(uc);
							if(++ucnt > 0){break;}
						}
					}
				}
			}else if(frame->data[4] == 0x01){
				wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
				if (access("/oem/qdreamer/qsound/uart.cfg", F_OK) == 0 && access("/oem/qdreamer/qsound/uart.cfg", W_OK) == 0){
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fn=fopen("/oem/qdreamer/qsound/uart.cfg","r+");
					ret=fread(tmpbuf,sizeof(tmpbuf),1,fn);
					// wtk_debug("================fread=[%s]\n",tmpbuf);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					pc=strstr(tmpbuf,"USE_LINEIN_SPK=");
					s=pc-tmpbuf;
					wtk_debug("+========>>>>>>s=%d pc=%c\n",s,*pc);
					tmpbuf[s+15]='1';
					pc=strstr(tmpbuf,"USE_LINEIN_MIC=");
					s=pc-tmpbuf;
					tmpbuf[s+15]='0';
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					fseek(fn,0,SEEK_SET);
					ret=fwrite(tmpbuf,strlen(tmpbuf),1,fn);
					fflush(fn);
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					if (getresult == 1) {
						ret = 1;
						ucnt = 0;
						while (ret != 0) {
							qtk_proc_write(uc,"ENABLE_LINEIN_SPK",18);
							// ret=qtk_proc_read(uc);
							wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
							if(++ucnt > 0){break;}
						}
					}
				}
			}
			wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);fclose(fn);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
			#if 0
			send_audio_status_frame(uc,0x01, 0x16 ,QTK_UART_STATUS_LINEIN);
            #endif
		#else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif

			break;
            
        case 0x011A:  // QTK_UART_TYPE_RECV_GET_VOLUME_VALUE - 获取音量值
		#if 0
			isnot_spk_count++;
			wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
			getresult = qtk_uart_client_isRunning();
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			if (getresult == 1) {
				ret = 1;
				ucnt = 0;
				while (ret != 0) {
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					qtk_proc_write(uc,"GET_VOLUME_VALUE",17);
					// ret=qtk_proc_read(uc);
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					if(++ucnt > 0){break;}
				}
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			time_t startgetvolume = time(NULL);
			while (time(NULL) - startgetvolume < timeout_s) {
				wtk_debug("----------->>>>>>>now_delay = %f\n",time_get_ms()-utime);
				if (access("/oem/qdreamer/qsound/get_volume.txt", F_OK) == 0){
					// 文件存在，读取文件内容
					//system("chmod 777 /oem/qdreamer/qsound/get_volume.txt");
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					chmod("/oem/qdreamer/qsound/get_volume.txt",0777);
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					upresult = file_read_buf("/oem/qdreamer/qsound/get_volume.txt", &ulen);
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					resp = atoi(upresult)+96;
					wtk_debug("------------------------resp = %d\n",resp);
					//system("rm /oem/qdreamer/qsound/get_volume.txt");
					unlink("/oem/qdreamer/qsound/get_volume.txt");
					wtk_debug("------------------_>>>>>>>now_delay = %f\n",time_get_ms()-utime);
					wtk_log_log0(uc->log,"GET_VOLUME_VALUE finished!\n");
					break;
				}
				usleep(10000); 
			}
			wtk_debug("----------->>>>>>>now_delay = %f\n",time_get_ms()-utime);
			// wtk_log_log0(uc->log,"ERROR:SPK_AUDIO_CHECK,timeout!\n");
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, (uint8_t*)&resp,1);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
		#endif
			uint8_t get_volume=0x3C;
			send_response(uc, frame, (uint8_t*)&result,1); // 直接发送单字节
			break;
            
        case 0x0110:  // QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE - 读取麦克风音量档位
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
            // result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE);
            result = uc->mic_shift2;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("MIC volume_value: 0x%02X\n", (uint8_t)result);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			send_response(uc, frame, (uint8_t*)&result,1); // 直接发送单字节
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;
            
        case 0x0112:  // QTK_UART_TYPE_RECV_SET_MICROPHONE_VOLUME_VALUE - 设置麦克风音量档位
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			getresult = qtk_uart_client_isRunning();
			if(getresult == 1){
				ret =1;
				ucnt=0;
				while(ret != 0){
					ret=sprintf(set_buf,"SET_MICVOLUME%d",frame->data[4]);
					uc->mic_shift2 = frame->data[4];
					wtk_debug("--------------SET_MICVOLUME==%d\n",frame->data[4]);
					qtk_proc_write(uc,set_buf,ret);//先判断是否启用agc
					// ret=qtk_proc_read(uc);
					if(++ucnt > 0){break;}
				}
			}else{
				result = qtk_uart_client_set_volume(uc, (char*)&frame->data[4], 1); 
			}
			if(ret == -1){
				result = -2;
			}
			// sleep(1);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
			#if 0
			send_audio_status_frame(uc,0x01, 0x16 ,QTK_UART_STATUS_MIC);
			#endif
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif    
			break;
            
        case 0x0111:  // QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE - 读取扬声器音量档位
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			int read_val;
            // result = qtk_uart_client_get_arg(uc, QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE);
			// usleep(100);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			read_register("/sys/bus/i2c/devices/3-0069/volume", &read_val);
			wtk_debug("The value is: %d\n", read_val);
			// wtk_debug("SPK volume value: 0x%02X\n", (uint8_t)result);
			// result_data = (uint8_t)result;
			result_data = (uint8_t)read_val;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			send_response(uc, frame, &result_data,1); // 直接发送单字节
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif    
			break;
            
        case 0x0113:  // QTK_UART_TYPE_RECV_SET_SPEKER_VOLUME_VALUE - 设置扬声器音量档位
			#ifndef CESHI	
				wtk_debug("------------------>>>>>>%d\n",frame->data[4]);
				{
					FILE *fp = fopen("/sys/bus/i2c/devices/3-006d/volume", "w");
					if (fp) {
						fprintf(fp, "%d", frame->data[4]);
						fclose(fp);
					}
				}
				wtk_debug("------->echo %d > /sys/bus/i2c/devices/3-006d/volume\n", frame->data[4]);
				{
					FILE *fp = fopen("/sys/bus/i2c/devices/3-0069/volume", "w");
					if (fp) {
						fprintf(fp, "%d", frame->data[4]);
						fclose(fp);
					}
				}
				wtk_debug("------->echo %d > /sys/bus/i2c/devices/3-0069/volume\n", frame->data[4]);
            	send_response(uc, frame, &normal,1);
				wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				{
					FILE *fp = fopen("/oem/qdreamer/qsound/spk_volume.txt", "w");
					if (fp) {
						fprintf(fp, "%d", frame->data[4]);
						fclose(fp);
					}
				}
				// send_audio_status_frame(uc,0x01, 0x16 ,QTK_UART_STATUS_SPEAKER);
			#if 0
				if(frame->data[4] <= 20)
				{
					uc->spk_alarm = 1;
					uint8_t volume_alarm[2] = {0x06, 0x00};
					send_active_response(lc ,0x01 ,0x15 ,(uint8_t*)&volume_alarm,2);
				}
				if(frame->data[4] > 20 && uc->spk_alarm)
				{
					uc->spk_alarm = 0;
					uint8_t volume_alarm[2] = {0x06, 0x01};
					send_active_response(lc ,0x01 ,0x15 ,(uint8_t*)&volume_alarm,2);
				}
			#endif
            #else
				send_response(uc, frame, &cehsi,1); // 正常
			#endif
			break;
            
        case 0x0114:  // QTK_UART_TYPE_RECV_LOG_REPORTING - 日志上报
			system("tar -cvf /data/qtk_uart_client.log.tar /data/qtk_uart_client.log");
			FILE*fp=popen("tar -cvf /data/qtk_uart_client.log.tar /data/qtk_uart_client.log","r");
			if(fp)
			{
			fclose(fp);
			}
			char *data=file_read_buf("/data/qtk_uart_client.log.tar",&len);
            send_response(uc, frame, data,len);
            break;
            
        case 0x0115:  // QTK_UART_TYPE_RECV_ALARM_REPORTING - 告警上报
            send_response(uc, frame, 0x00,1);
            break;
            
        case 0x0116:  // QTK_UART_TYPE_RECV_AUDIO_STATUS_CHANGE_NOTIFICATION - 音频状态变化通知
            send_response(uc, frame, 0x00,1);
            break;
            
        case 0x0117:  // QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_UNPLUGGING - 音频输入输出设备拔出通知
            send_response(uc, frame, 0x00,1);
            break;
            
        case 0x0118:  // QTK_UART_TYPE_RECV_SPEAKER_CONTROL - 扬声器控制
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			getresult = qtk_uart_client_isRunning();
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			if(getresult == 1){
			ret =1;
			ucnt=0;
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			while(ret != 0){
				ret=sprintf(set_buf,"SPEAKER_CONRTOL%02x",frame->data[0]);
				wtk_debug("-----------------frame->data[1]:%d\n",frame->data[0]);
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				qtk_proc_write(uc,set_buf,ret);//先判断是否启用agc 
				// ret=qtk_proc_read(uc);
				wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
				if(++ucnt > 0){break;}
			}
		}
		// sleep(1);
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("----------------------------->>>>>>>isnot_spk_count=%d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif    
			break;
            
        case 0x0119:  // QTK_UART_TYPE_RECV_LOG_COLLECTION - 日志收集
#ifdef chenggang
            // ① 按协议立即回一个空载荷响应，保持请求-响应对称
            send_response(uc, frame, NULL, 0);   // 注意函数签名：最后一个参数是长度，传 0

            // ② 置位“一次性触发上报”标志，交给 report 线程去执行
            if (uc) 
			{
                uc->report_trigger_once = 1;
                if (uc->log) 
				    wtk_log_log0(uc->log, "[0119] trigger immediate log upload");
            }
            break;
#endif 
        case 0x011B:  // QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_INSERTIOIN - 音频输入输出设备插入通知
            send_response(uc, frame, 0x00,1);
            break;
            
        case 0x011C:  // QTK_UART_TYPE_RECV_GET_OUTPUT_EQ_MODE - 获取输出EQ模式
		#ifndef CESHI
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
            const char *MODE_PATH = "/oem/qdreamer/qsound/eq_mode";
            unsigned char mode = 0x00;  // 缺省：STANDARD_MODE
            FILE *mf = NULL;

            // 协议请求数据长度应为 0；为健壮起见，忽略载荷内容，直接读取持久化的模式
            (void)data_len;

            mf = fopen(MODE_PATH, "rb");
            if (mf) {
                char line[32] = {0};
                size_t n = fread(line, 1, sizeof(line)-1, mf);
                fclose(mf);
            if (n > 0) {
                // 允许 "0\n"、"1"、"2" 或 "0x00" 等写法
                char *endp = NULL;
                long v = strtol(line, &endp, 0);
            if (v >= 0 && v <= 2) {
                mode = (unsigned char)v;
            }
        }
    }
           // 响应：按协议返回 1 字节模式值
           // 0x00=STANDARD_MODE, 0x01=WALL_MOUNTED__MODE, 0x02=WALL_RECESSED__MODE
           send_response(uc, frame, &mode, 1);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;
            
          case 0x011D:  /* QTK_UART_TYPE_RECV_SET_OUTPUT_EQ_MODE - 设置输出EQ模式 */
{
#ifndef CESHI
    isnot_spk_count++;
    const char *preset_path = NULL;
    unsigned char eq_mode_val;
    FILE *fp = NULL, *mf = NULL;
    char *buf = NULL;
    long fsize = 0;

    /* 1) 载荷校验：1 字节模式值（0=标准，1=挂墙，2=嵌墙） */
    if (data_len != 1) { send_response(uc, frame, &faile, 0); break; }

    /* 2) 选择预置 JSON 文件路径 */
    eq_mode_val = frame->data[0];
    switch (eq_mode_val) {
        case 0x00: preset_path = "/oem/qdreamer/qsound/presets/eq_standard.json";  break;
        case 0x01: preset_path = "/oem/qdreamer/qsound/presets/eq_wall.json";      break;
        case 0x02: preset_path = "/oem/qdreamer/qsound/presets/eq_recessed.json";  break;
        default:   send_response(uc, frame, &faile, 0); break;
    }
    if (!preset_path) break;

    /* 3) 读预置 JSON 到内存，并备份到 eq.json */
    fp = fopen(preset_path, "rb");
    if (!fp) { wtk_debug("[EQ][011D] open %s failed: %s\n", preset_path, strerror(errno)); send_response(uc, frame, &faile, 0); break; }
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); send_response(uc, frame, &faile, 0); break; }
    fsize = ftell(fp);
    if (fsize <= 0 || fsize > 64*1024) { fclose(fp); send_response(uc, frame, &faile, 0); break; }
    if (fseek(fp, 0, SEEK_SET) != 0) { fclose(fp); send_response(uc, frame, &faile, 0); break; }

    buf = (char*)malloc((size_t)fsize + 1);
    if (!buf) { fclose(fp); send_response(uc, frame, &faile, 0); break; }
    if ((long)fread(buf, 1, (size_t)fsize, fp) != fsize) { fclose(fp); free(buf); send_response(uc, frame, &faile, 0); break; }
    fclose(fp);
    buf[fsize] = '\0';

    if (qtk_write_file(QTK_EQ_JSON_PATH, buf, (int)fsize) != 0) {
        wtk_debug("[EQ][011D] write %s failed\n", QTK_EQ_JSON_PATH);
        free(buf);
        send_response(uc, frame, &faile, 0);
        break;
    }

    /* 读完 buf/fsize 后，先净化再用 */
const char *san = NULL;
size_t      slen = 0;
sanitize_json_payload((const uint8_t*)buf, (size_t)fsize, &san, &slen);
if (slen == 0 || san == NULL) {
    wtk_debug("[EQ] sanitize failed\n");
    free(buf);
    send_response(uc, frame, &faile, 0);
    break;
}
/* 先把净化后的 JSON 写入 eq.json（保持与 0x0103 一致） */
if (qtk_write_file(QTK_EQ_JSON_PATH, san, (int)slen) != 0) {
    wtk_debug("[EQ][011D] write %s failed\n", QTK_EQ_JSON_PATH);
    free(buf);
    send_response(uc, frame, &faile, 0);
    break;
}

/* 然后应用（用净化后的切片） */
int rc = apply_eq_from_json_text_wtk(san, (int)slen);
free(buf);
if (rc != 0) {
    wtk_debug("[EQ][011D] apply failed, rc=%d\n", rc);
    send_response(uc, frame, &faile, 0);
    break;
}
/* === 新增：持久化当前模式，供 0x011C 读取 === */
{
    const char *MODE_PATH = "/oem/qdreamer/qsound/eq_mode";
    char tmp_path[128];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", MODE_PATH);

    /* 写临时文件，再 rename 原子替换，避免掉电/并发读到半文件 */
    FILE *mf = fopen(tmp_path, "wb");
    if (!mf) {
        wtk_debug("[EQ][011D] open %s failed: %s\n", tmp_path, strerror(errno));
        /* 不因持久化失败而判整个操作失败；仅告警 */
    } else {
        /* 存十进制或 0xXX 都可；你 0x011C 用 strtol(base=0) 兼容 */
        /* 这里写十进制更直观：0/1/2\n */
        fprintf(mf, "%u\n", (unsigned)eq_mode_val & 0xFF);
        fflush(mf);
        fsync(fileno(mf));   /* 可选：确保落盘（视系统支持） */
        fclose(mf);

        if (rename(tmp_path, MODE_PATH) != 0) {
            wtk_debug("[EQ][011D] rename(%s -> %s) failed: %s\n",
                      tmp_path, MODE_PATH, strerror(errno));
            /* 不中断主流程 */
        } else {
            wtk_debug("[EQ][011D] mode persisted: %u (0:STD,1:WALL,2:RECESSED)\n",
                      (unsigned)eq_mode_val);
        }
    }
}

/* 按协议回成功（保持你现有做法） */
send_response(uc, frame, &normal, 1);  /* 若你的成功码是 0x00，就传 &sucess 或 &ok 对应变量 */
break;
#endif
}

        case 0x011E:  // QTK_UART_TYPE_RECV_GET_LINEOUT_MODE - 获取lineout输出模式
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			result = uc->lineout_pattern;
			send_response(uc, frame, (uint8_t*)&result,1); // 直接发送单字节
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;
            
        case 0x011F:  // QTK_UART_TYPE_RECV_SET_LINEOUT_MODE - 设置lineout输出模式
		#ifndef CESHI	
			wtk_debug("------------------_>>>>>>now_start_to_deal = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			isnot_spk_count++;
			getresult = qtk_uart_client_isRunning();
			if(getresult == 1){
				ret =1;
				ucnt=0;
				while(ret != 0){
					ret=sprintf(set_buf,"SET_LINEOUTPATTERN%d",frame->data[0]);
					wtk_debug("--------------SET_LINEOUTPATTERN=%d\n",frame->data[0]);
					uc->lineout_pattern = frame->data[0];
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					#if 1
					qtk_proc_write(uc,set_buf,ret);//先判断是否启用agc
					#endif
					// ret=qtk_proc_read(uc);
					wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
					if(++ucnt > 0){break;}
				}
			}else{
			}
			if(ret == -1){
				result = -2;
			}
			wtk_debug("------------------_>>>>>>>now_delay = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			usleep(100*1000);
			wtk_debug("----------------------now_delay = %.2f\n",time_get_ms()-recvdata_time);
            send_response(uc, frame, &normal,1);
			wtk_debug("------------------_>>>>>>>finish_send_response = %.2f,%.2f\n",time_get_ms()-recvdata_time,time_get_ms()-utime);
			wtk_debug("---------------------------------->>>>>>>>>isnot_spk_count = %d\n",isnot_spk_count);
        #else
			send_response(uc, frame, &cehsi,1); // 正常
		#endif
			break;
        	
        default:
            send_response(uc, frame, 0xFF,1); // 无效命令
            break;
    }
}
static void qtk_uart_client_clean_q(qtk_uart_client_t *uc, wtk_blockqueue_t *queue)
{
	qtk_uart_client_msg_t *msg;
	wtk_queue_node_t *qn;
	int len=queue->length;
	int i=0;

	while(i<len) {
		qn = wtk_blockqueue_pop(queue,0,NULL);
		if(!qn) {
			break;
		}
		msg = data_offset2(qn,qtk_uart_client_msg_t,q_n);
		qtk_uart_client_push_msg(uc,msg);
		i++;
	}
}

static qtk_uart_client_msg_t* qtk_uart_client_msg_new(qtk_uart_client_t *uc)
{
	qtk_uart_client_msg_t *msg;

	msg = (qtk_uart_client_msg_t*)wtk_malloc(sizeof(*msg));
	if(!msg) {
		return NULL;
	}
	msg->buf = wtk_strbuf_new(2560,1);
	if(!msg->buf) {
		wtk_free(msg);
		return NULL;
	}

	return msg;
}

static int qtk_uart_client_msg_delete(qtk_uart_client_msg_t *msg)
{
	wtk_strbuf_delete(msg->buf);
	wtk_free(msg);
	return 0;
}

static qtk_uart_client_msg_t* qtk_uart_client_pop_msg(qtk_uart_client_t *uc)
{
	qtk_uart_client_msg_t *msg;

	msg = wtk_lockhoard_pop(&uc->msg_hoard);
	if(!msg) {
		return NULL;
	}
	msg->statID = 0;
	wtk_strbuf_reset(msg->buf);
	return msg;
}

static void qtk_uart_client_push_msg(qtk_uart_client_t *uc,qtk_uart_client_msg_t *msg)
{
	wtk_lockhoard_push(&uc->msg_hoard,msg);
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
	wtk_json_obj_add_ref_number_s(json, item, "delay", (int)(tm_e-tm_s));
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
    if (len < 9) {
        return -1; // 帧不完整
    }
    if (data[0] != REQUEST_FRAME_HEADER_0 || 
        data[1] != REQUEST_FRAME_HEADER_1) {
        return -2; // 帧头错误
    }
    if (data[len-1] != FRAME_FOOTER) {
        return -3; // 帧尾错误
    }
    memcpy(frame->frame_header, data, 2);
    memcpy(frame->event_code, data + 2, 2);
    memcpy(frame->data_length, data + 4, 2);
    memcpy(frame->checksum, data + len - 3, 2); 
    frame->frame_footer = data[len-1];
    // 解析数据部分
    uint16_t data_len =  frame->data_length[0]| (frame->data_length[1] << 8);
    if (data_len > 0) {
        // 检查数据长度是否匹配
        if (len != 9 + data_len) {
            return -4; // 长度不匹配
        }
        frame->data = (uint8_t *)malloc(data_len);
        if (!frame->data) {
            return -5; // 内存分配失败
        }
        memcpy(frame->data, data + 6, data_len);
    } else {
        frame->data = NULL;
    }
    
    return 0;
}
static void send_active_response(qtk_uart_client_t *uc ,uint8_t event_code1, uint8_t event_code2,uint8_t *data, uint16_t data_len)
{
    wtk_debug("------------------------------>>>>>>>>>>>>>\n");
    qtk_uart_recv_frame_t resp = {
        .frame_header = {RESPONSE_FRAME_HEADER_0, RESPONSE_FRAME_HEADER_1},
        .event_code = {event_code1, event_code2},
        .data_length = {data_len & 0xFF, (data_len >> 8) & 0xFF},
        .data = (uint8_t *)data,
        .frame_footer = FRAME_FOOTER
    };
    
    wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
    int crc_data_len = 4 + data_len;
    uint8_t *crc_data = (uint8_t *)malloc(crc_data_len);
    if (crc_data) {
        int pos = 0;
        memcpy(crc_data + pos, resp.event_code, 2);
        pos += 2;
        memcpy(crc_data + pos, resp.data_length, 2);
        pos += 2;
        if (data_len > 0) {
            memcpy(crc_data + pos, data, data_len);
        }
        
        uint16_t crc = calculateModbusCRC(crc_data, crc_data_len);
        resp.checksum[0] = crc & 0xFF;
        resp.checksum[1] = (crc >> 8) & 0xFF;
        
        free(crc_data);
    } else {
        resp.checksum[0] = 0x00;
        resp.checksum[1] = 0x00;
    }
    
    wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
    int frame_len = 9 + data_len; // 2头 + 2事件 + 2长度 + 数据 + 2CRC + 1尾
    uint8_t *send_buf = (uint8_t *)malloc(frame_len);
    wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
    if (send_buf) {
        int pos = 0;
        memcpy(send_buf + pos, resp.frame_header, 2);
        pos += 2;
        memcpy(send_buf + pos, resp.event_code, 2);
        pos += 2;
        memcpy(send_buf + pos, resp.data_length, 2);
        pos += 2;
        
        if (data_len > 0) {
            memcpy(send_buf + pos, data, data_len);
            pos += data_len;
        }
        
        memcpy(send_buf + pos, resp.checksum, 2);
        pos += 2;
        send_buf[pos++] = resp.frame_footer;
    	wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
        int ret = qtk_uart_write2(uc->uart, (char*)send_buf, pos);
    	wtk_debug("-----------------------------__>>>>>>>>>>>>>>>>>\n");
        free(send_buf);
    }
}
static void send_response(qtk_uart_client_t *uc, 
                                qtk_uart_recv_frame_t *req_frame,
                                uint8_t *data,
							    uint16_t data_len)
{
	wtk_debug("------------------------------>>>>>>>>>>>>>\n");
	qtk_uart_recv_frame_t resp = {
        .frame_header = {RESPONSE_FRAME_HEADER_0, RESPONSE_FRAME_HEADER_1},
        .event_code = {req_frame->event_code[0], req_frame->event_code[1]},
        .data_length = {data_len & 0xFF,(data_len >> 8) & 0xFF},
        .data = (uint8_t *)data,
        .frame_footer = FRAME_FOOTER
    };
    int crc_data_len = 4 + data_len;
    uint8_t *crc_data = (uint8_t *)malloc(crc_data_len);
    if (crc_data) {
        int pos = 0;
        memcpy(crc_data + pos, resp.event_code, 2);
        pos += 2;
        memcpy(crc_data + pos, resp.data_length, 2);
        pos += 2;
        if (data_len > 0) {
            memcpy(crc_data + pos, data, data_len);
        }
        uint16_t crc = calculateModbusCRC(crc_data, crc_data_len);
        resp.checksum[0] = crc & 0xFF;
        resp.checksum[1] = (crc >> 8) & 0xFF;
        
        free(crc_data);
    } else {
        resp.checksum[0] = 0x00;
        resp.checksum[1] = 0x00;
    }
    int frame_len = 9 + data_len; // 2头 + 2事件 + 2长度 + 数据 + 2CRC + 1尾
    uint8_t *send_buf = (uint8_t *)malloc(frame_len);
    if (send_buf) {
        int pos = 0;
        memcpy(send_buf + pos, resp.frame_header, 2);
        pos += 2;
        memcpy(send_buf + pos, resp.event_code, 2);
        pos += 2;
        memcpy(send_buf + pos, resp.data_length, 2);
        pos += 2;
        if (data_len > 0) {
            memcpy(send_buf + pos, data, data_len);
            pos += data_len;
        }
		wtk_debug("------------------------------>>>>>>>>>>>>>\n");
        memcpy(send_buf + pos, resp.checksum, 2);
        pos += 2;
        send_buf[pos++] = resp.frame_footer;
        int ret = qtk_uart_write2(uc->uart, (char*)send_buf, pos);
        wtk_debug("Send log frame: len=%d\n", ret);
        wtk_debug("Header: %02X %02X\n", resp.frame_header[0], resp.frame_header[1]);
        wtk_debug("Event: %02X %02X\n", resp.event_code[0], resp.event_code[1]);
        wtk_debug("Length: %02X %02X (%d bytes)\n", 
                 resp.data_length[0], resp.data_length[1], data_len);
        if (data_len ==1) {
            // wtk_debug("Data:%02X\n“,re);
			wtk_debug("value: 0x%02X\n", *data);
        }else{
			// for (size_t i = 0; i < data_len; i++) {
			// 	printf("0x%02X ", data[i]);
			// }
			// // printf("\n"); // 最后换行
		}
        wtk_debug("CRC: %02X %02X\n", resp.checksum[0], resp.checksum[1]);
        wtk_debug("Footer: %02X\n", resp.frame_footer);
        free(send_buf);
	}
}
unsigned short calculateModbusCRC(unsigned char *data, int length) {
	unsigned short crc = 0xFFFF;// 初始化 CRC 值
    unsigned char b;
    for (int i = 0; i < length; i++) {
        crc ^= data[i];
        // 对每个字节进行 8 次右移操作
        for (int j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc >>= 1;  // 右移 1 位
                crc ^= 0xA001;  // 异或常数 0xA001
            } else {
                crc >>= 1;  // 右移 1 位
            }
        }
    }
    return crc; 
}
