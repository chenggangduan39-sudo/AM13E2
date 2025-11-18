#ifndef __SDK_UART_CLIENT_H__
#define __SDK_UART_CLIENT_H__

#include "qtk_uart_client_cfg.h"
#include "sdk/dev/uart/qtk_uart.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "wtk/os/wtk_log.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/core/wtk_jsonkv.h"
#include "qtk/record/qtk_alsa_recorder.h"
#include "wtk/core/wtk_type.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>
#include <stdio.h>
#include "third/json/cJSON.h"
#include <stdarg.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#ifdef _cplusplus
extern "C"{
#endif

#ifndef QTK_EQ_JSON_PATH
#define QTK_EQ_JSON_PATH "/oem/qdreamer/qsound/eq.json"
#endif
#ifndef SYSFS_EQ_PATH
#define SYSFS_EQ_PATH "/sys/bus/i2c/devices/3-0069/eq"
#endif

#define USE_AM13E2
#ifndef SYSFS_EQ_PATH
#define SYSFS_EQ_PATH "/sys/bus/i2c/devices/3-0069/eq"
#endif
#ifndef SYSFS_EQ_PATH_BASS
#define SYSFS_EQ_PATH_BASS "/sys/bus/i2c/devices/3-006d/eq"
#define UART_CFG_PATH "/oem/qdreamer/qsound/uart.cfg"
#define RESCFG_PATH "/oem/qdreamer/qsound/res/cfg"
#define LINEIN_PIN_PATH "/sys/bus/iio/devices/iio:device0/in_voltage5_raw"
#define LINEOUT_PIN_PATH "/sys/bus/iio/devices/iio:device0/in_voltage4_raw"
#endif

#define REQUEST_FRAME_HEADER_0  0x81
#define REQUEST_FRAME_HEADER_1  0xAB
#define RESPONSE_FRAME_HEADER_0 0x90
#define RESPONSE_FRAME_HEADER_1 0x40
#define FRAME_FOOTER            0xFF
/* ========= 日志上报事件码（按协议V1.8） ========= */
#ifndef EVT_LOG_UPLOAD_START
#define EVT_LOG_UPLOAD_START 0x0120
#endif
#ifndef EVT_LOG_UPLOAD_DATA
#define EVT_LOG_UPLOAD_DATA 0x0121
#endif
#ifndef EVT_LOG_UPLOAD_END
#define EVT_LOG_UPLOAD_END 0x0122
#endif

/* ========= 日志上报 ACK 事件码（按你的协议调整）========= */
/* 若对端回同码ACK（0x0120），也兼容处理；若协议定义了 0x8120 作为ACK，也支持 */
#ifndef EVT_LOG_UPLOAD_START_ACK
#define EVT_LOG_UPLOAD_START_ACK 0x8120
#endif
#ifndef EVT_LOG_UPLOAD_END_ACK
#define EVT_LOG_UPLOAD_END_ACK   0x8122
#endif

#define UART_LOG_DBG(uc, fmt, ...) uart_log((uc), LOG_NOTICE, fmt, ##__VA_ARGS__)
#define UART_LOG_WARN(uc, fmt, ...) uart_log((uc), LOG_WARN, fmt, ##__VA_ARGS__)
#define UART_LOG_ERR(uc, fmt, ...) uart_log((uc), LOG_ERR, fmt, ##__VA_ARGS__)

/* ========= 用于ACK队列的小节点 ========= */
typedef struct qtk_ack_node_s {
    wtk_queue_node_t q_n;
    uint16_t code;     // 收到的事件码
    uint8_t  status;   // 约定: data[0] == 0x00 表示OK，非0表示失败
} qtk_ack_node_t;

/* ========= 前置声明 ========= */

#define MSG_DATA_HDR_LEN (128)
#define RECV_DATA_HDR (10)
#define RECV_FRAME_HEAD (9)

typedef enum{
	QTK_UART_TYPE_RECV_SPEAKER_JUDGMENT,					    //扬声器音频检测判断
	QTK_UART_TYPE_RECV_MIC_JUDGMENT,							//mic音频检测判断
	QTK_UART_TYPE_RECV_MIC_JUDGMENT_OUTPUT_EQ_ADJUSTMENT,       //输出EQ设置
	QTK_UART_TYPE_RECV_SET_DENOISE_SWITCH,  					//智能降噪开关设置
	QTK_UART_TYPE_RECV_GET_DENOISE_SWITCH,   					//智能降噪开关获取
	QTK_UART_TYPE_RECV_SET_GAIN_CONTROL_SWITCH,					//自动增益开关设置
	QTK_UART_TYPE_RECV_GET_GAIN_CONTROL_SWITCH,					//自动增益开关获取
	QTK_UART_TYPE_RECV_SET_ECHO_INTENSITY_SWITCH,				//回声抑制强度设置
	QTK_UART_TYPE_RECV_GET_ECHO_INTENSITY_SWITCH,				//回声抑制强度获取
	QTK_UART_TYPE_RECV_GET_LIST_AUDIO_INPUT_PORTS,  			//获取音频输入口列表
	QTK_UART_TYPE_RECV_GET_LIST_AUDIO_OUTPUT_PORTS,				//获取音频输出口列表
	QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_MIC,					//启用禁用MIC
	QTK_UART_TYPE_RECV_ENDABLE_AND_DISABLE_SPK,					//启用禁用SPK
	QTK_UART_TYPE_RECV_LINE_IN_CONTROL,							//line in 本地输出控制(本地扩音)
	QTK_UART_TYPE_RECV_SET_INPUT_TYPE,							//设置输入口类型
	QTK_UART_TYPE_RECV_GET_VOLUME_VALUE,						//获取音量值
	QTK_UART_TYPE_RECV_GET_MICROPHONE_VOLUME_VALUE,				//读取麦克风音量档位
	QTK_UART_TYPE_RECV_SET_MICROPHONE_VOLUME_VALUE,				//设置麦克风音量档位
	QTK_UART_TYPE_RECV_GET_SPEKER_VOLUME_VALUE,					//读取扬声器音量档位
	QTK_UART_TYPE_RECV_SET_SPEKER_VOLUME_VALUE,					//设置扬声器音量档位
	QTK_UART_TYPE_RECV_LOG_REPORTING,							//日志上报
	QTK_UART_TYPE_RECV_ALARM_REPORTING,							//告警上报
	QTK_UART_TYPE_RECV_AUDIO_STATUS_CHANGE_NOTIFICATION,	    //音频状态变化通知
	QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_UNPLUGGING,//音频输入输出设备拔出通知
	QTK_UART_TYPE_RECV_SPEAKER_CONTROL,							//扬声器控制
	QTK_UART_TYPE_RECV_LOG_COLLECTION,							//日志收集
	QTK_UART_TYPE_RECV_AUDIO_INPUT_AND_OUTPUT_DEVICE_INSERTIOIN,//音频输入输出设备插入通知
	QTK_UART_TYPE_RECV_GET_OUTPUT_EQ_MODE,       				//获取输出EQ模式
	QTK_UART_TYPE_RECV_SET_OUTPUT_EQ_MODE,       				//设置输出EQ模式
	QTK_UART_TYPE_RECV_GET_LINEOUT_MODE,       					//获取lineout输出模式
	QTK_UART_TYPE_RECV_SET_LINEOUT_MODE,       					//设置lineout输出模式
}qtk_uart_type_t;
typedef struct {
    uint8_t frame_header[2];   // 帧头 2字节
    uint8_t event_code[2];     // 事件码 2字节
    uint8_t data_length[2];    // 数据长度 2字节
    uint8_t *data;             // 数据
    uint8_t checksum[2];       // 校验和 2字节
    uint8_t frame_footer;   // 帧尾 1字节
}qtk_uart_recv_frame_t;
typedef enum {
    PARSE_STATE_HEADER1,
    PARSE_STATE_HEADER2,
    PARSE_STATE_EVENT_CODE,
    RESP_STATE_DATA_LENGTH,   //数据长度
	RESP_STATE_DATA_LENGTH_2,
    RESP_STATE_DATA,          //数据内容
    RESP_STATE_CHECKSUM,
	RESP_STATE_CHECKSUM_2,      //校验和
    RESP_STATE_FOOTER  
} uart_parse_state_t;
typedef enum{
	QTK_UART_CLIENT_CONNECT_OK,
	QTK_UART_CLIENT_UPDATE_OK,
	QTK_UART_CLIENT_UPDATE_EQUAL,
	QTK_UART_CLIENT_UPDATE_VERSION,
	QTK_UART_CLIENT_UPDATE_UNZIP_FAILD,
	QTK_UART_CLIENT_UPDATE_RECORD_START,
	QTK_UART_CLIENT_UPDATE_RECORD_FILE,
	QTK_UART_CLIENT_SEND_CPUINFO,
	QTK_UART_CLIENT_UPDATE_MD5_CHEAK_FAILED=22,
}qtk_uart_client_data_type_t;

typedef enum{
	QTK_UART_SEND_GET_MICVOLUME=8,//检索当前麦克风音量级别
	QTK_UART_SEND_SET_MICVOLUME,//设置麦克风音量级别的所需值
	QTK_UART_SEND_GET_MICMUTE,//检索麦克风静音功能的启用状态
	QTK_UART_SEND_SET_MICMUTE_ON,//设置麦克风静音功能的启用状态
	QTK_UART_SEND_SET_MICMUTE_OFF,
	QTK_UART_SEND_GET_MICANS,//检索麦克风噪声抑制功能的启用状态
	QTK_UART_SEND_SET_MICANS_ON,//设置麦克风噪声抑制功能的启用状态
	QTK_UART_SEND_SET_MICANS_OFF,
	QTK_UART_SEND_GET_MICAGC,//检索麦克风自动增益控制功能的启用状态
	QTK_UART_SEND_SET_MICAGC_ON,//设置麦克风自动增益控制功能的启用状态
	QTK_UART_SEND_SET_MICAGC_OFF,
	QTK_UART_SEND_GET_MICAEC,//检索麦克风回声消除功能的启用状态
	QTK_UART_SEND_SET_MICAEC_ON,//设置麦克风回声消除功能的启动状态
	QTK_UART_SEND_SET_MICAEC_OFF,
}qtk_uart_send_data_type_t;

typedef enum{
	QTK_UART_STATE_RECV_START,
	QTK_UART_STATE_RECV_TYPE,
	QTK_UART_STATE_RECV_DATA,
	QTK_UART_STATE_RECV_END,
}qtk_uart_state_t;

typedef enum{
	QTK_UART_STATUS_MIC,
	QTK_UART_STATUS_SPEAKER,
	QTK_UART_STATUS_LINEIN,
	QTK_UART_STATUS_LINEOUT,
}qtk_state_t;

typedef enum{
	QTK_UART_SEND_STATE_LINEIN_ON,
	QTK_UART_SEND_STATE_LINEIN_OFF,
	QTK_UART_SEND_STATE_LINEOUT_ON,
	QTK_UART_SEND_STATE_LINEOUT_OFF,
	QTK_UART_SEND_STATE_NORMAL,
	QTK_UART_SEND_STATE_ACTIVE,
	QTK_UART_SEND_STATE_ACTIVE2,
}qtk_uart_send_state_t;

typedef void (*qtk_uart_recv_notify_f)(void *ths, qtk_uart_recv_frame_t *data, int len);

typedef struct qtk_uart_client qtk_uart_client_t;
struct qtk_uart_client{
	qtk_uart_client_cfg_t *cfg;
	qtk_uart_t *uart;
	wtk_json_parser_t *parser;
	wtk_strbuf_t *uart_buf;
	wtk_strbuf_t *uart_buf2;
	wtk_strbuf_t *mac;
	wtk_strbuf_t *version;
	wtk_strbuf_t *crcbuf;
	wtk_strbuf_t *responsebuf;

	wtk_thread_t trsn_thread;
	wtk_thread_t msg_thread;

	wtk_thread_t check_thread;

	wtk_blockqueue_t input_q;
	wtk_blockqueue_t msg_q;
	wtk_lockhoard_t msg_hoard;
	wtk_log_t *log;
	qtk_uart_state_t utype;
	// ===== 新增: ACK邮箱队列 =====
    wtk_blockqueue_t ack_q;
	// ---- 日志主动上报线程 ----
    wtk_thread_t report_thread;
    unsigned int report_run:1;
    unsigned int report_busy:1;
    uint32_t upload_round; // 会话编号
    uint32_t package_seq;
/* 由命令触发的立即上报标志 */
    unsigned int report_trigger_once:1;
	wtk_thread_t send_thread;    
    int send_run;                 
    wtk_blockqueue_t send_queue;  
    wtk_lock_t status_lock;
	qtk_uart_recv_notify_f recv_notify;
	void *this;

	int lineout_pattern;
    int mic_shift2;
	int spk_volume;
	int use_linein;
	int use_lineout;
	char MD5_buf[33];
	unsigned int trsn_run:1;
	unsigned int check_run:1;
	unsigned int msg_run:1;
	unsigned int spk_alarm:1;
	unsigned int list_input:1;
};

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	int statID;
	wtk_strbuf_t *buf;
	wtk_strbuf_t *sendbuf;
}qtk_uart_client_msg_t;

void uart_log(qtk_uart_client_t *uc, int level, const char *fmt, ...);

qtk_uart_client_t *qtk_uart_client_new(qtk_uart_client_cfg_t *cfg, wtk_log_t *log);
void qtk_uart_client_set_notify(qtk_uart_client_t *fixbeam, void *this, qtk_uart_recv_notify_f notify);
int qtk_uart_client_delete(qtk_uart_client_t *fixbeam);
int qtk_uart_client_start(qtk_uart_client_t *fixbeam);
int qtk_uart_client_stop(qtk_uart_client_t *fixbeam);
int qtk_uart_client_feed(qtk_uart_client_t *fixbeam, char *data, int len);
void qtk_uart_client_send_response(qtk_uart_client_t *uc,
						  qtk_uart_recv_frame_t *req_frame,
						  uint8_t *data,
						  uint16_t data_len);
void qtk_uart_client_send_active_response(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, uint8_t *data, uint16_t data_len);
int qtk_uart_client_send_audio_status_frame(qtk_uart_client_t *uc, uint8_t event_code1, uint8_t event_code2, int type, int is_exit);

void ackq_push(qtk_uart_client_t *uc, uint16_t code, uint8_t status);
int  wait_for_ack(qtk_uart_client_t *uc, uint16_t expect_code, int timeout_ms, uint8_t *status_out);
//计算校验和
unsigned short calculateModbusCRC(unsigned char *data, int length);
int apply_eq_from_json_text_wtk(const char *json_in, int len_in);
int qtk_write_file(const char *path, const void *buf, size_t len);
void sanitize_json_payload(const uint8_t *in, size_t in_len, const char **out_json, size_t *out_len);
#ifdef _cplusplus
}
#endif
#endif
