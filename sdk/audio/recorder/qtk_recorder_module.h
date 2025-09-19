#ifndef SDK_AUDIO_RECORDER_QTK_RECORDER_MODULE
#define SDK_AUDIO_RECORDER_QTK_RECORDER_MODULE

#include "wtk/core/wtk_type.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*qtk_recorder_module_notify_f)(void *ths,int is_err);
typedef void* (*qtk_recorder_start_func)(void *handler,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		);
typedef int (*qtk_recorder_stop_func) (void *handler,void *ths);
typedef int (*qtk_recorder_read_func) (void *handler,void *ths,char *buf,int bytes);
typedef int (*qtk_recorder_clean_func)(void *handler,void *ths);


typedef struct {
	qtk_recorder_start_func start_func;
	qtk_recorder_stop_func  stop_func;
	qtk_recorder_read_func  read_func;
	qtk_recorder_clean_func clean_func;
	void *handler;
	void *ths;
}qtk_recorder_module_t;

void qtk_recorder_module_init(qtk_recorder_module_t *rcder_module);
void qtk_recorder_module_set_callback(qtk_recorder_module_t *rcder_module,
		void *handler,
		qtk_recorder_start_func start_func,
		qtk_recorder_stop_func  stop_func,
		qtk_recorder_read_func  read_func,
		qtk_recorder_clean_func clean_func
		);

#ifdef __cplusplus
};
#endif
#endif
