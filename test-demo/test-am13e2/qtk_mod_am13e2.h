#ifndef __qtk_mod_am13e2_H__
#define __qtk_mod_am13e2_H__
#include "qtk_mod_am13e2_cfg.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_log.h"
#include "sdk/codec/qtk_msg.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/os/wtk_sem.h"

#ifndef OFFLINE_TEST
#include "qtk/record/qtk_record.h"
#include "qtk/play/qtk_play.h"

#include "qtk/play/qtk_play2.h"
#endif
#include "wtk/core/wtk_jsonkv.h"

#include "sdk/qtk_comm.h"
#include "wtk/bfio/resample/wtk_resample.h"
#include "sdk/codec/qtk_usb_uevent.h"
#include "speex/speex_resampler.h"
#include "sdk/codec/timer/qtk_timer.h"
#include "sdk/api_1/gainnetbf/qtk_gainnetbf.h"
#include "sdk/api_1/vboxebf/qtk_vboxebf.h"
#include "wtk/os/wtk_lock.h"
#include <sys/ipc.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdbool.h>
#include "third/json/cJSON.h"
#include "wtk/bfio/consist/wtk_mic_check.h"
#ifdef __cplusplus
extern "C" {
#endif

#define SIZE 64
#define MAX 48

typedef struct shm
{
    pid_t pid;
    char buf[MAX];
}SHM;

typedef struct qtk_proc
{
	key_t key;
	SHM *shmaddr;
	int shmid;
	pid_t pid_server;
	// int audio_check_result;
}qtk_proc_t;

typedef enum{
	qtk_mod_am13e2_RECORD_START,
	qtk_mod_am13e2_RECORD_STOP,
	qtk_mod_am13e2_POWER_USB,
	qtk_mod_am13e2_POWER_48V,
	qtk_mod_am13e2_POWER_POE,
	qtk_mod_am13e2_OUTPUT_START,
	qtk_mod_am13e2_OUTPUT_STOP,
	qtk_mod_am13e2_MSG_PARSER,
	qtk_mod_am13e2_DATA_STUDENT_BF3A,
	qtk_mod_am13e2_DATA_OUTPUT,
	qtk_mod_am13e2_DATA_LINEIN_MIC_TOUAC,
	qtk_mod_am13e2_DATA_LINEIN_MIC_TOLINEOUT,
	qtk_mod_am13e2_DATA_LINEIN_courseware_TOLINEOUT,
	qtk_mod_am13e2_DATA_ARRAY,
	qtk_mod_am13e2_DATA_ARRAY_TOlINEOUT,
	qtk_mod_am13e2_STATE_MIC,
	qtk_mod_am13e2_STATE_SPK,
	qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE,
	qtk_mod_am13e2_DATA_LINEIN_COUURSEWARE_TOUAC,
	qtk_mod_am13e2_DATA_UAC,
	qtk_mod_am13e2_DATA_IIS_TOSPK,
	qtk_mod_am13e2_DATA_IIS_TOLINEOUT,

}qtk_mqform_mod_msg_t;
// 滤波器状态结构体（单通道）
typedef struct {
    float b0, b1, b2; // 分子系数
    float a1, a2;     // 分母系数
    float x1, x2;     // 输入历史
    float y1, y2;     // 输出历史
} LowPassFilter;
//串口通信扬声器以及麦克风检测
typedef enum {
    AM13E2_CHECK_NONE = 0,     // 无检测
    AM13E2_CHECK_SPEAKER,      // 扬声器检测
    AM13E2_CHECK_MIC           // 麦克风检测
} qtk_am13e2_check_type_t;
typedef enum {
    QTK_UART_TYPE_RECV_SPEAKER_JUDGMENT,
    QTK_UART_TYPE_RECV_MIC_JUDGMENT,
} qtk_uart_type_t;
typedef struct {
    int port_type;          // 端口类型
    int port_id;            // 端口ID
	char* name;
    bool is_use;             // 是否使用
    int gain_level;         // 增益级别
    int audio_input_type;   // 音频输入类型
    bool is_local_play;      // 是否本地播放
} qtk_audio_port_t;
typedef void (*audio_check_result_handler_t)(int result, void *userdata);
/// //////////////////////////////
typedef struct qtk_mod_am13e2{
	qtk_mod_am13e2_cfg_t *cfg;
#ifndef OFFLINE_TEST
	qtk_record_t *rcd;
	qtk_record_t *rcd2;
	qtk_record_t *rcd3;
	qtk_record_t *rcd4;
	qtk_play_t *usbaudio;
	qtk_play_t *lineout;
	qtk_play_t *speaker;

	wtk_thread_t usbaudio_t;
	wtk_thread_t lineout_t;
	wtk_thread_t speaker_t;
	wtk_thread_t linein_t;
	wtk_blockqueue_t usbaudio_queue;
	wtk_blockqueue_t lineout_queue;
	wtk_blockqueue_t linein_queue;
	wtk_blockqueue_t spk_queue;
	
	qtk_audio_port_t * input;
#endif
	wtk_resample_t *outresample;

	qtk_timer_t *sttimer;
	qtk_gainnetbf_t *gainnetbf;
	qtk_gainnetbf_t *gainnetbf2;
	qtk_gainnetbf_t *gainnetbf3;
	qtk_gainnetbf_t *gainnetbf4;
	qtk_gainnetbf_t *gainnetbf5;

	qtk_vboxebf_t *vboxebf;
	qtk_vboxebf_t *avboxebf;
	qtk_vboxebf_t *denoisebf;

	wtk_mic_check_t *mic_check_rcd;

	wtk_mic_check_t *mic_check_play;


	wtk_log_t *log;
	qtk_msg_t *msg;
	wtk_wavfile_t *mul;
	wtk_wavfile_t *arraymul;
	wtk_wavfile_t *iismul;
	wtk_wavfile_t *playwav;
	wtk_wavfile_t *jlmul;
	wtk_wavfile_t *uacmul;
	wtk_wavfile_t *lineoutml;
	wtk_strbuf_t *check_path_buf;
	wtk_strbuf_t *mul_path;
	wtk_strbuf_t *iis_path;
	wtk_strbuf_t *uac_path;
	wtk_strbuf_t *lineout_path;
	wtk_strbuf_t *arraymul_path;
	wtk_strbuf_t *play_path;

	wtk_strbuf_t *speaker_left_audiobuf;
	wtk_strbuf_t *lineout_left_audiobuf;	

	wtk_strbuf_t *speaker_all_audiobuf;
	wtk_strbuf_t *lineout_all_audiobuf;
	wtk_thread_t rcd_t;
	wtk_thread_t rcd2_t;
	wtk_thread_t rcd3_t;
	wtk_thread_t rcd4_t;
	wtk_thread_t merge_rcd_t;
	wtk_thread_t vbox_t;
	wtk_thread_t array_vbox_t;
	wtk_thread_t denoise_vbox_t;

	wtk_thread_t gainnet_t;
	wtk_thread_t gainnet2_t;
	wtk_thread_t gainnet3_t;
	
	wtk_thread_t mic_check_rcd_t;
	wtk_thread_t mic_check_play_t;

	wtk_thread_t linein_check_t;
	wtk_blockqueue_t vbox_queue;
	wtk_blockqueue_t array_vbox_queue;
	wtk_blockqueue_t denoise_vbox_queue; //linein_mic 降噪
	wtk_blockqueue_t gainnet_queue;
	wtk_blockqueue_t gainnet2_queue;
	wtk_blockqueue_t gainnet3_queue;

	wtk_blockqueue_t mic_check_rcd_queue;
	
	wtk_blockqueue_t mic_check_play_queue;
	
	wtk_blockqueue_t merge_rcd_queue;

	int mic_channel;
	double onplaytime;
	
	unsigned int rcd_run:1;
	unsigned int rcd2_run:1;
	unsigned int rcd3_run:1;
	unsigned int rcd4_run:1;
	unsigned int speaker_run:1;

	// unsigned int speaker:1;

	unsigned int use_spkout:1;
	// unsigned int use_wooferout:1;
	// unsigned int use_headset:1;
	unsigned int usbaudio_run:1;
	unsigned int lineout_run:1;
	unsigned int linein_run:1;
	unsigned int player_run:1;
	unsigned int vbox_run:1;
	unsigned int is_player_start:1;
	unsigned int array_vbox_run:1;
	unsigned int denoise_vbox_run:1;

	unsigned int gainnet_run:1;
	unsigned int gainnet2_run:1;
	unsigned int gainnet3_run:1;
	unsigned int mic_check_rcd_run:1;
	unsigned int mic_check_play_run:1;
	unsigned int merge_rcd_run:1;
	unsigned int linein_check_run:1;
	unsigned int use_record:1;
	unsigned int is_output:1;
	unsigned int is_outputstart:1;
	unsigned int use_linein_out:1;
	unsigned int use_lineout_out:1;

	volatile unsigned int play_on:1;
	unsigned int log_audio:1;
	unsigned int is_mic : 1;
	unsigned int is_use_uac:1;
	/// 串口通信模块
	unsigned int audio_check_requested:1;  // 异步检测标志
    qtk_am13e2_check_type_t check_type;
	audio_check_result_handler_t result_handler;
    void *handler_userdata;                      
}qtk_mod_am13e2_t;
typedef enum {
    QTK_MIC_CHECK_NORMAL = 0,
    QTK_MIC_CHECK_RCD_ERROR,
    QTK_MIC_CHECK_PLAY_ERROR,
}qtk_mic_check_err_type_t;

int qtk_mod_am13e2_bytes(qtk_mod_am13e2_t *m);
qtk_mod_am13e2_t *qtk_mod_am13e2_new(qtk_mod_am13e2_cfg_t *cfg);
void qtk_mod_am13e2_delete(qtk_mod_am13e2_t *m);
void qtk_mod_am13e2_start(qtk_mod_am13e2_t *m, int is_record);
void qtk_mod_am13e2_stop(qtk_mod_am13e2_t *m);
void qtk_mod_am13e2_start2(qtk_mod_am13e2_t *m, int sample_rate);
void qtk_mod_am13e2_stop2(qtk_mod_am13e2_t *m);

#ifdef __cplusplus
};
#endif
#endif
