#ifndef __QTK_API_QFORM_H__
#define __QTK_API_QFORM_H__
#include "wtk/bfio/qform/wtk_qform9.h"
#include "qtk_qform_cfg.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_qform_notify_f)(void *ths, char *data, int len);
typedef struct qtk_qform{
	qtk_qform_cfg_t *cfg;
	wtk_qform9_t *qform;
	short **buf;
	void *ths;
	qtk_qform_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
    wtk_strbuf_t *cache_buf;
}qtk_qform_t;

qtk_qform_t *qtk_qform_new(qtk_qform_cfg_t *cfg);
int qtk_qform_delete(qtk_qform_t *qform);
int qtk_qform_start(qtk_qform_t *qform);
int qtk_qform_reset(qtk_qform_t *qform);
int qtk_qform_feed(qtk_qform_t *qform, char *data, int len, int is_end);
void qtk_qform_set_notify(qtk_qform_t *qform, void *ths, qtk_qform_notify_f notify);
void qtk_qform_set_notify2(qtk_qform_t *qform, void *ths, qtk_engine_notify_f notify);
#ifdef __cplusplus
extern "C"{
};
#endif
#endif
