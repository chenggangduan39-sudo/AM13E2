#ifndef __QTK_API_EQFORM_H__
#define __QTK_API_EQFORM_H__
#include "wtk/bfio/wtk_eqform.h"
#include "qtk_eqform_cfg.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_eqform_notify_f)(void *ths, char *data, int len);
typedef struct qtk_eqform{
	qtk_eqform_cfg_t *cfg;
	wtk_eqform_t *eqform;
	short **buf;
	void *ths;
	qtk_eqform_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
	int inchannel;
    wtk_strbuf_t *cache_buf;
}qtk_eqform_t;

qtk_eqform_t *qtk_eqform_new(qtk_eqform_cfg_t *cfg);
int qtk_eqform_delete(qtk_eqform_t *qform);
int qtk_eqform_start(qtk_eqform_t *qform);
int qtk_eqform_reset(qtk_eqform_t *qform);
int qtk_eqform_feed(qtk_eqform_t *qform, char *data, int len, int is_end);
void qtk_eqform_set_notify(qtk_eqform_t *qform, void *ths, qtk_eqform_notify_f notify);
void qtk_eqform_set_notify2(qtk_eqform_t *qform, void *ths, qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
