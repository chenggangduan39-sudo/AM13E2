#ifndef SDK_AUDIO_PLAYER_QTK_PLAYER_MODULE
#define SDK_AUDIO_PLAYER_QTK_PLAYER_MODULE

#include "wtk/core/wtk_type.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_player_module qtk_player_module_t;
typedef void (*qtk_player_module_notify_f)(void *ths,int is_err);

#if defined(__IPHONE_OS__) || defined(__APPLE__) || defined(__mips) || defined(OPENAL) || defined(USE_XDW) || defined(__ANDROID__) || defined(WIN32)
typedef void* (*qtk_player_start_func)(void *handler,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int peroid_time,
		int use_uac
		);
#else
typedef void* (*qtk_player_start_func)(void *handler,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int peroid_time,
		int start_time,
		int stop_time,
		int avail_time,
		int silence_time,
		int use_uac
		);
#endif
typedef int   (*qtk_player_stop_func) (void *handler,void *ths);
typedef int   (*qtk_player_write_func)(void *handler,void *ths,char *data,int bytes);
typedef int   (*qtk_player_clean_func)(void *handler,void *ths);

struct qtk_player_module
{
	void *handler;
	void *ths;
	qtk_player_start_func start_func;
	qtk_player_stop_func  stop_func;
	qtk_player_write_func write_func;
	qtk_player_clean_func clean_func;
};

void qtk_player_module_init(qtk_player_module_t *plyer_module);
void qtk_player_module_set_callback(qtk_player_module_t *plyer_module,
		void *handler,
		qtk_player_start_func start_func,
		qtk_player_stop_func  stop_func,
		qtk_player_write_func write_func,
		qtk_player_clean_func clean_func
		);

#ifdef __cplusplus
};
#endif
#endif
