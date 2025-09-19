#ifndef __QTK_API_BFIO_H__
#define __QTK_API_BFIO_H__
#include "wtk/bfio/wtk_bfio.h"
#include "wtk/bfio/wtk_bfio5.h"
#include "qtk_bfio_cfg.h"
#include "sdk/qtk_api.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_bfio_notify_f)(void *ths, char *data, int len);
typedef struct qtk_bfio{
	qtk_bfio_cfg_t *cfg;
	wtk_bfio_t *qform;
	wtk_bfio5_t *bfio5;
	short **buf;
    wtk_strbuf_t *cache_buf;
	void *ths;
	qtk_bfio_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;

	int channel;
	int cancel_len;
}qtk_bfio_t;

qtk_bfio_t *qtk_bfio_new(qtk_bfio_cfg_t *cfg);
int qtk_bfio_delete(qtk_bfio_t *qform);
int qtk_bfio_start(qtk_bfio_t *qform);
int qtk_bfio_reset(qtk_bfio_t *qform);
int qtk_bfio_feed(qtk_bfio_t *qform, char *data, int len, int is_end);
void qtk_bfio_set_notify(qtk_bfio_t *qform, void *ths, qtk_bfio_notify_f notify);
void qtk_bfio_set_notify2(qtk_bfio_t *qform, void *ths, qtk_engine_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
