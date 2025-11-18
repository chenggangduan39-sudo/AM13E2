#ifndef QTK_QTK_COMMON_API
#define QTK_QTK_COMMON_API

#ifdef __cplusplus
extern "C" {
#endif

#if defined(WIN32) || defined(_WIN32)
#ifndef DLL_API
#define DLL_API __declspec(dllexport)
#endif
#else
#ifndef DLL_API
#define DLL_API __attribute ((visibility("default")))
#endif
#endif

typedef struct qtk_session qtk_session_t;

typedef enum {
	QTK_DEBUG,
	QTK_WARN,
	QTK_ERROR,
}qtk_errcode_level_t;

typedef void(*qtk_errcode_handler)(void *ths,int errcode,char* errstr);

DLL_API qtk_session_t* qtk_session_init(char *params,qtk_errcode_level_t level,void *ths,qtk_errcode_handler errhandler);
DLL_API void qtk_session_exit(qtk_session_t *session);

typedef enum{
	QTK_FEED_DATA = 0,
	QTK_FEED_END  = 1,
}qtk_feed_type_t;

typedef enum {
	QTK_SPEECH_START,
	QTK_SPEECH_DATA_OGG,//ogg
	QTK_SPEECH_DATA_PCM,//pcm
	QTK_SPEECH_END,
	QTK_AEC_WAKE,
	QTK_AEC_DIRECTION,
	QTK_AEC_WAKE_INFO,
	QTK_AEC_SLEEP,
	QTK_AEC_CANCEL,
	QTK_AEC_CANCEL_DATA,
	QTK_AEC_WAKE_ONESHOT,
	QTK_AEC_WAKE_NORMAL,
	QTK_AEC_THETA_HINT,
	QTK_AEC_THETA_BF_BG,
	QTK_AEC_THETA_BG,
	QTK_TTS_START,
	QTK_TTS_DATA,
	QTK_TTS_END,
	QTK_ASR_TEXT,
	QTK_ASR_HINT,
	QTK_ASR_HOTWORD,
	QTK_EVAL_TEXT,
	QTK_SEMDLG_TEXT,
	QTK_VAR_ERR,
	QTK_VAR_SOURCE_AUDIO,
	QTK_AUDIO_LEFT,
	QTK_AUDIO_ARRIVE,
	QTK_AUDIO_ERROR,
	QTK_CONSIST_MICERR_NIL,
    QTK_CONSIST_MICERR_ALIGN,
    QTK_CONSIST_MICERR_MAX,
    QTK_CONSIST_MICERR_CORR,
    QTK_CONSIST_MICERR_ENERGY,
    QTK_CONSIST_SPKERR_NIL,
    QTK_CONSIST_SPKERR_ALIGN,
    QTK_CONSIST_SPKERR_MAX,
    QTK_CONSIST_SPKERR_CORR,
    QTK_CONSIST_SPKERR_ENERGY,
	QTK_ULTGESTURE_TYPE,
	QTK_ULTEVM_TYPE,
	QTK_AUDIO_ENERGY,
	QTK_UNKOWN,
}qtk_var_type_t;

typedef struct
{
	qtk_var_type_t type;
	union{
		int i;
		float f;
		short s;
		struct{
			int nbest;
			int phi;
			int theta;
			float nspecsum;
		}ii;
		struct{
			char *data;
			int len;
		}str;
		struct {
			float theta;
			int on;
		}fi;
		struct{
			float energy;
			float snr;
		}ff;
	}v;
}qtk_var_t;

typedef void(*qtk_engine_notify_f)(void *ths, qtk_var_t *data);
typedef struct qtk_engine qtk_engine_t;

DLL_API qtk_engine_t* qtk_engine_new(qtk_session_t *session, char *params);
DLL_API int qtk_engine_delete(qtk_engine_t *e);
DLL_API int qtk_engine_start(qtk_engine_t *e);
DLL_API int qtk_engine_reset(qtk_engine_t *e);
DLL_API int qtk_engine_feed(qtk_engine_t *e, char *data, int bytes, qtk_feed_type_t is_end);
DLL_API int qtk_engine_cancel(qtk_engine_t *e);
DLL_API int qtk_engine_set(qtk_engine_t *e, char *params);
DLL_API int qtk_engine_set_xbnf(qtk_engine_t *e, char *data, int len);
DLL_API int qtk_engine_set_notify(qtk_engine_t *e, void *ths, qtk_engine_notify_f notify_f);
#ifdef USE_ASR
//语法识别专用接口
DLL_API int qtk_engine_update_cmds(qtk_engine_t *e,char* words);
#endif
DLL_API char *qtk_engine_get_fn(qtk_engine_t *e);
DLL_API float qtk_engine_get_prob(qtk_engine_t *e);
DLL_API void qtk_engine_get_result(qtk_engine_t *e, qtk_var_t *var);

typedef struct qtk_module qtk_module_t;
DLL_API qtk_module_t* qtk_module_new(qtk_session_t *session,char *res,char *cfg_fn);
DLL_API void qtk_module_delete(qtk_module_t *m);
DLL_API int qtk_module_start(qtk_module_t *m);
DLL_API int qtk_module_stop(qtk_module_t *m);
DLL_API int qtk_module_feed(qtk_module_t *m,char *data,int bytes);
DLL_API int qtk_module_set(qtk_module_t *m,char *params);
DLL_API int qtk_module_set_notify(qtk_module_t *m,void *user_data,qtk_engine_notify_f notify);


//utils
//queue

typedef struct{
	void* q;
	int isblock;
}qtk_test_queue_t;
DLL_API qtk_test_queue_t* qtk_test_queue_new(int isblock);
DLL_API void qtk_test_queue_delete(qtk_test_queue_t* q);
DLL_API int qtk_test_queue_push(qtk_test_queue_t* q,void *n);
DLL_API void* qtk_test_queue_pop(qtk_test_queue_t *q,int ms,int *is_timeout);

#ifdef __cplusplus
};
#endif
#endif
