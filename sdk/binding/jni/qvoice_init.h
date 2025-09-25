#include <jni.h>

#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/core/wtk_wavfile.h"

#include "sdk/qtk_api.h"
#include "sdk/audio/qtk_audio.h"
#include "sdk/engine/qtk_engine.h"
#include "sdk/session/qtk_session.h"
#include "sdk/mulsv/qtk_mulsv_api.h"
// #include "sdk/module/qtk_module.h"
#include "sdk/codec/oggdec/qtk_oggdec.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
// #include "qtk/qtk_vprint_wake.h"

extern JavaVM *gJvm;
extern jobject gClassLoader;
extern jmethodID gFindClassMethod;
extern jclass *session_cls;
extern jclass *module_cls;
extern jclass *engine_cls;
extern jclass *audio_cls;
extern jclass *ogg_cls;
// extern jclass *vw_cls;

typedef enum {
	QTK_JNI_SPEECH_START  = 0,
	QTK_JNI_SPEECH_DATA_OGG   = 1,
	QTK_JNI_SPEECH_DATA_PCM   = 2,
	QTK_JNI_SPEECH_END    = 3,
	QTK_JNI_AEC_WAKE      = 4,
	QTK_JNI_AEC_DIRECTION = 5,
	QTK_JNI_AEC_WAKE_INFO = 6,
	QTK_JNI_AEC_SLEEP	  = 7,
	QTK_JNI_AEC_CANCEL    = 8,
	QTK_JNI_AEC_CANCEL_DATA = 9,
	QTK_JNI_AEC_WAKE_ONESHOT =10,
	QTK_JNI_AEC_WAKE_NORMAL = 11,
	QTK_JNI_AEC_THETA_HINT = 12,
	QTK_JNI_AEC_THETA_BF_BG = 13,
	QTK_JNI_AEC_THETA_BG = 14,
	QTK_JNI_SINGLE_WAKEUP=15,//单独唤醒
	QTK_JNI_ASR_ENERGY=16,
	QTK_JNI_WAKEUP_BUF=17,

	QTK_JNI_ASR_DATA      = 20,
	QTK_JNI_ASR_HINT	  = 21,
	QTK_JNI_ASR_HOTWORD   = 22,

	QTK_JNI_TTS_START     = 30,
	QTK_JNI_TTS_DATA      = 31,
	QTK_JNI_TTS_END       = 32,

	QTK_JNI_SEMDLG_DATA   = 40,

	QTK_JNI_EVAL_DATA     = 50,


	QTK_JNI_NORESPONSE    = 60,
	QTK_JNI_SOURCE_AUDIO = 61,

	QTK_JNI_AUDIO_LEFT   = 70,
	QTK_JNI_AUDIO_ARRIVE = 71,
	QTK_JNI_AUDIO_ERROR  = 72,

	QTK_JNI_CONSIST_MICERR_NIL = 73,
    QTK_JNI_CONSIST_MICERR_ALIGN = 74,
    QTK_JNI_CONSIST_MICERR_MAX = 75,
    QTK_JNI_CONSIST_MICERR_CORR = 76,
    QTK_JNI_CONSIST_MICERR_ENERGY = 77,
    QTK_JNI_CONSIST_SPKERR_NIL = 78,
    QTK_JNI_CONSIST_SPKERR_ALIGN = 79,
    QTK_JNI_CONSIST_SPKERR_MAX = 80,
    QTK_JNI_CONSIST_SPKERR_CORR = 81,
    QTK_JNI_CONSIST_SPKERR_ENERGY = 82,

	QTK_JNI_ULTGESTURE_TYPE = 90,
	QTK_JNI_AUDIO_RANGE_IDX = 91,
	QTK_JNI_ULTEVM_TYPE=95,
	QTK_JNI_VPRINT_ATTRIBUTE = 96,
	QTK_JNI_ERRCODE =100,
	QTK_JNI_ASR_SIL_END = 105,
	QTK_JNI_ASR_SPEECH_END=110,
	QTK_JNI_GET_WAKE_ASR=115,
	QTK_JNI_GET_WAKE_KEY=116,
	QTK_JNI_GET_VOICE_AGE_GENDER=119,
}qtk_jni_type_t;

typedef struct{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_strbuf_t*    buf;
}qtk_jni_msg_t;

typedef struct {
	qtk_session_t *session;
	wtk_blockqueue_t rlt_q;
	wtk_lockhoard_t msg_hoard;
	jclass cls;
	JavaVM *jvm;
}qtk_jni_session_t;

typedef struct {
	wtk_main_cfg_t *cfg;
        qtk_audio_t    *audio;
        wtk_blockqueue_t audio_q;
	wtk_blockqueue_t cache_q;
	int cache_times;
	int cache_time;
	wtk_sem_t play_sem;
	int syn;
	wtk_wavfile_t* linux_wav;//从linux驱动拿到的数据
	wtk_wavfile_t* android_wav;//安卓读取的数据

	jclass   cls;
	JavaVM * jvm;

	JNIEnv *   rcd_env;
	jclass     rcd_cls;
	jmethodID  rcd_start;
	jmethodID  rcd_stop;
	jmethodID  rcd_read;
	jbyteArray rcd_byte;

	JNIEnv *   ply_env;
	jclass     ply_cls;
	jmethodID  ply_start;
	jmethodID  ply_stop;
	jmethodID  ply_write;
	jbyteArray ply_byte;
}qtk_jni_audio_t;

typedef struct {
	qtk_engine_t *e;
	wtk_blockqueue_t rlt_q;
	wtk_lockhoard_t msg_hoard;
	int step;
	int cache;
}qtk_jni_engine_t;

typedef struct {
	qtk_oggdec_t *oggdec;
	wtk_blockqueue_t rlt_q;
	wtk_lockhoard_t msg_hoard;
}qtk_jni_oggdec_t;

typedef struct {
	qtk_module_t *module;
	wtk_log_t *log;
	wtk_blockqueue_t rlt_q;
	wtk_lockhoard_t msg_hoard;
	int step;
	int cache;

	jclass   cls;
	JavaVM * jvm;

	JNIEnv *   rcd_env;
	jclass     rcd_cls;
	jmethodID  rcd_start;
	jmethodID  rcd_stop;
	jmethodID  rcd_read;
	jbyteArray rcd_byte;

	JNIEnv *   ply_env;
	jclass     ply_cls;
	jmethodID  ply_start;
	jmethodID  ply_stop;
	jmethodID  ply_write;
	jbyteArray ply_byte;
}qtk_jni_module_t;

typedef enum{
	QTK_JNI_WAKE = 0,//wake
	QTK_JNI_WAKE_PRE,
}vw_jni_type_t;

typedef struct 
{
	int step;
	int cache;
	qtk_mulsv_api_t *m;
	wtk_lockhoard_t msg_hoard;
}qtk_jni_mul_t;


typedef struct{
	wtk_queue_node_t hoard_n;
	wtk_queue_node_t q_n;
	wtk_strbuf_t*    buf;
}vw_jni_msg_t;

typedef struct {
    // qtk_vprint_wake_t  *vw;
	wtk_blockqueue_t rlt_q;
	wtk_lockhoard_t msg_hoard;
}vw_jni_t;

/*
 * JNI_OnLoad
 */
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *pjvm, void *reserved);
JNIEXPORT void JNICALL JNI_OnUnload(JavaVM *pjvm, void *reserved);
