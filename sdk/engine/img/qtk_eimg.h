#ifndef __QTK_EIMG_H__
#define __QTK_EIMG_H__

#include "sdk/qtk_api.h"
#include "sdk/engine/qtk_engine.h"
#include "sdk/session/qtk_session.h"
#include "sdk/engine/comm/qtk_engine_param.h"
#include "sdk/engine/comm/qtk_engine_thread.h"
#include "sdk/api_1/img/qtk_api_img.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*qtk_eimg_notify_f)(void *ths,char *data,int bytes);
typedef struct qtk_eimg qtk_eimg_t;
struct qtk_eimg
{
	qtk_engine_param_t param;
	qtk_session_t *session;
	qtk_api_img_cfg_t *cfg;
	qtk_api_img_t *img;
	wtk_log_t *log;
	qtk_engine_notify_f notify;
	void *notify_ths;
	qtk_engine_thread_t *thread;
	qtk_engine_callback_t *callback;
};

qtk_eimg_t * qtk_eimg_new(qtk_session_t *s,wtk_local_cfg_t *params);
int qtk_eimg_start(qtk_eimg_t *e);
void qtk_eimg_delete(qtk_eimg_t *e);
void qtk_eimg_reset(qtk_eimg_t *e);
int qtk_eimg_feed(qtk_eimg_t *e,char *data,int bytes,int is_end);
void qtk_eimg_set_notify(qtk_eimg_t *e,void *ths,qtk_engine_notify_f notify);

int qtk_eimg_set(qtk_eimg_t *e,char *data,int bytes);

#ifdef __cplusplus
};
#endif
#endif
