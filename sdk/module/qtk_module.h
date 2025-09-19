#ifndef SDK_MODULE_QTK_MODULE
#define SDK_MODULE_QTK_MODULE

#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/wtk_os.h"
#include "sdk/audio/recorder/qtk_recorder_module.h"
#include "sdk/audio/player/qtk_player_module.h"
#include "sdk/module/mqform/qtk_mqform_mod.h"
#include "sdk/module/mqform2/qtk_mqform2_mod.h"
#include "sdk/module/mgainnet/qtk_mgainnet_mod.h"
#include "sdk/module/qform/qtk_mqform.h"
#include "sdk/module/mult/qtk_mult_mod.h"
#include "qtk_api.h"
#include "sdk/session/qtk_session.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void*(*qtk_module_new_f)(qtk_session_t *session,void *cfg);
typedef void(*qtk_module_delete_f)(void *ths);
typedef void(*qtk_module_set_notify_f)(void *ths, void *user_data, qtk_engine_notify_f notify);

typedef int(*qtk_module_start_f)(void *ths);
typedef int(*qtk_module_stop_f)(void *ths);
typedef int(*qtk_module_feed_f)(void *ths,char *data,int bytes);

typedef int(*qtk_module_set_f)(void *ths, char *params);
typedef void(*qtk_module_set_audio_callback_f)(void *ths,
		void *user_data,
		qtk_recorder_start_func recorder_start,
		qtk_recorder_read_func  recorder_read,
		qtk_recorder_stop_func  recorder_stop,
		qtk_recorder_clean_func recorder_clean,
		qtk_player_start_func   player_start,
		qtk_player_write_func   player_write,
		qtk_player_stop_func    player_stop,
		qtk_player_clean_func   player_clean
		);

typedef int (*qtk_module_init_func)(qtk_module_t *m,char *cfg_fn);

typedef enum{
	QTK_MODULE_QFORM,
	QTK_MODULE_MQFORM,
	QTK_MODULE_MGAINNET,
	QTK_MODULE_ULTEVM,
	QTK_MODULE_MQFORM2,
	QTK_MODULE_MULT
}qtk_module_rlttype_t;

struct qtk_module
{
	qtk_session_t *session;
	wtk_str_hash_t *init_hash;
	wtk_main_cfg_t *main_cfg;
	qtk_module_new_f                new_f;
	qtk_module_delete_f             delete_f;
	qtk_module_start_f              start_f;
	qtk_module_stop_f               stop_f;
	qtk_module_feed_f				feed_f;
	qtk_module_set_f                set_f;
	qtk_module_set_notify_f         set_notify_f;
	qtk_module_set_audio_callback_f set_audio_callback_f;
	void*                           module_ths;
	qtk_module_rlttype_t rtype;
	int starttime;
	int overtime;
};

int qtk_module_set_audio_callback(qtk_module_t *m,
		void *user_data,
		qtk_recorder_start_func recorder_start,
		qtk_recorder_read_func  recorder_read,
		qtk_recorder_stop_func  recorder_stop,
		qtk_recorder_clean_func recorder_clean,
		qtk_player_start_func   player_start,
		qtk_player_write_func   player_write,
		qtk_player_stop_func    player_stop,
		qtk_player_clean_func   player_clean
		);

struct qtk_module_avspeech {
    qtk_session_t *session;
    wtk_cfg_file_t *params;
    void *handler;
};

#ifdef __cplusplus
};
#endif
#endif
