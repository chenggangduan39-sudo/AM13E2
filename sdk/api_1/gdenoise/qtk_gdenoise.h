#ifndef __QTK_API_GAINNET_DENOISE_H__
#define __QTK_API_GAINNET_DENOISE_H__
#include "qtk_gdenoise_cfg.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/bfio/maskdenoise/wtk_gainnet_denoise.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_gdenoise_notify_f)(void *ths, char *data, int len);

typedef struct qtk_gdenoise{
	qtk_gdenoise_cfg_t *cfg;
	wtk_aec_t *aec;
	wtk_gainnet_denoise_t *gdenoise;
	short **buf;
	void *ths;
	qtk_gdenoise_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
    wtk_strbuf_t *cache_buf;
	wtk_strbuf_t *out_buf;
}qtk_gdenoise_t;

qtk_gdenoise_t *qtk_gdenoise_new(qtk_gdenoise_cfg_t *cfg);
int qtk_gdenoise_delete(qtk_gdenoise_t *qform);
int qtk_gdenoise_start(qtk_gdenoise_t *qform);
int qtk_gdenoise_reset(qtk_gdenoise_t *qform);
int qtk_gdenoise_feed(qtk_gdenoise_t *qform, char *data, int len, int is_end);
void qtk_gdenoise_set_notify(qtk_gdenoise_t *qform, void *ths, qtk_gdenoise_notify_f notify);
void qtk_gdenoise_set_notify2(qtk_gdenoise_t *qform, void *ths, qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
