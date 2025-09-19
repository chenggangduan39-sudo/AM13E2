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

#ifdef _cplusplus
extern "C"{
#endif

#if (defined  USE_AM32) || (defined  USE_802A) || (defined  USE_AM60)
#define UART_CFG_PATH "/oem/qdreamer/qsound/uart.cfg"
#else
#define UART_CFG_PATH "/oem/uart.cfg"
#endif
#define SIZE 64
#define MAX 48
#define REQUEST_FRAME_HEADER_0  0x81
#define REQUEST_FRAME_HEADER_1  0xAB
#define RESPONSE_FRAME_HEADER_0 0x90
#define RESPONSE_FRAME_HEADER_1 0x40
#define FRAME_FOOTER            0xFF
/* ========= 日志上报 ACK 事件码（按你的协议调整）========= */
/* 若对端回同码ACK（0x0120），也兼容处理；若协议定义了 0x8120 作为ACK，也支持 */
#ifndef EVT_LOG_UPLOAD_START_ACK
#define EVT_LOG_UPLOAD_START_ACK 0x8120
#endif
#ifndef EVT_LOG_UPLOAD_END_ACK
#define EVT_LOG_UPLOAD_END_ACK   0x8122
#endif

/* ========= 用于ACK队列的小节点 ========= */
typedef struct qtk_ack_node_s {
    wtk_queue_node_t q_n;
    uint16_t code;     // 收到的事件码
    uint8_t  status;   // 约定: data[0] == 0x00 表示OK，非0表示失败
} qtk_ack_node_t;

/* ========= 前置声明 ========= */


typedef struct shm
{
    pid_t pid;
    char buf[MAX];
	int audio_check_result;
}SHM;

#define MSG_DATA_HDR_LEN (128)
#define RECV_DATA_HDR (10)
#define RECV_FRAME_HEAD (9)

// #define USE_AM34
#ifdef USE_AM34
typedef struct msg_buffer {
    long msg_type;
    char msg_text[100];
}msg_buffer_t;
#endif

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
typedef struct qtk_uart_client qtk_uart_client_t;
struct qtk_uart_client{
	qtk_uart_client_cfg_t *cfg;
	qtk_uart_t *uart;
	wtk_json_parser_t *parser;
	wtk_strbuf_t *uart_buf;
	wtk_strbuf_t *uart_buf2;
	wtk_strbuf_t *mac;
	wtk_strbuf_t *version;

	wtk_thread_t trsn_thread;
	wtk_thread_t msg_thread;
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
    uint64_t last_watch_bytes; // 上次观察到的目录累计大小
    uint32_t upload_round; // 可用于会话编号（目前未上链路，仅本地记）
/* 新增：由 0x0119 触发的一次性立即上报标志 */
    unsigned int report_trigger_once:1;
	wtk_thread_t send_thread;    
    int send_run;                 
    wtk_blockqueue_t send_queue;  
    wtk_lock_t status_lock;       

#ifdef USE_AM34
	msg_buffer_t message;
	struct timeval current_time;
	msg_buffer_t message2;
	key_t key;
	int msg_id;
	struct msqid_ds msqid_buf;
#endif
	int lineout_pattern;
    int mic_shift2;
	int record_tm;
	int record_chn;
	char MD5_buf[33];
	unsigned int trsn_run:1;
	unsigned int msg_run:1;
	unsigned int spk_alarm:1;
};

typedef struct {
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	int statID;
	wtk_strbuf_t *buf;
}qtk_uart_client_msg_t;

qtk_uart_client_t *qtk_uart_client_new(qtk_uart_client_cfg_t *cfg, wtk_log_t *log);
int qtk_uart_client_delete(qtk_uart_client_t *fixbeam);
int qtk_uart_client_start(qtk_uart_client_t *fixbeam);
int qtk_uart_client_stop(qtk_uart_client_t *fixbeam);
int qtk_uart_client_feed(qtk_uart_client_t *fixbeam, char *data, int len);

static void ackq_push(qtk_uart_client_t *uc, uint16_t code, uint8_t status);
static int  wait_for_ack(qtk_uart_client_t *uc, uint16_t expect_code, int timeout_ms, uint8_t *status_out);
//计算校验和
unsigned short calculateModbusCRC(unsigned char *data, int length);
#ifdef _cplusplus
}
#endif
#endif
