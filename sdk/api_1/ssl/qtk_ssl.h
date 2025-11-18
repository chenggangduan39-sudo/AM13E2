#ifndef __QTK_API_SSL_H__
#define __QTK_API_SSL_H__
#include "qtk_ssl_cfg.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "sdk/qtk_api.h"
#include "wtk/bfio/ssl/wtk_gainnet_ssl.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct 
{
    int theta;
    int phi;
    float nspecsum;
}qtk_ssl_extp_t;

typedef void (*qtk_ssl_notify_f)(void *ths, qtk_ssl_extp_t *data);

typedef struct qtk_ssl{
	qtk_ssl_cfg_t *cfg;
	wtk_ssl_t *ssl;
	wtk_gainnet_ssl_t *gainnet_ssl;
	short **buf;
	void *ths;
	qtk_ssl_extp_t *extp;
	qtk_ssl_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
    wtk_strbuf_t *cache_buf;
	wtk_strbuf_t *out_buf;
}qtk_ssl_t;

qtk_ssl_t *qtk_ssl_new(qtk_ssl_cfg_t *cfg);
int qtk_ssl_delete(qtk_ssl_t *qform);
int qtk_ssl_start(qtk_ssl_t *qform);
int qtk_ssl_reset(qtk_ssl_t *qform);
int qtk_ssl_feed(qtk_ssl_t *qform, char *data, int len, int is_end);
void qtk_ssl_set_notify(qtk_ssl_t *qform, void *ths, qtk_ssl_notify_f notify);
void qtk_ssl_set_notify2(qtk_ssl_t *qform, void *ths, qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
