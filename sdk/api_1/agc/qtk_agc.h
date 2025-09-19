#ifndef __QTK_API_AGC_H__
#define __QTK_API_AGC_H__
#include "wtk/bfio/agc/wtk_agc.h"
#include "qtk_agc_cfg.h"
#include "sdk/qtk_api.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_agc_notify_f)(void *ths, short **data, int len, int is_end);
typedef struct qtk_agc{
	qtk_agc_cfg_t *cfg;
	wtk_agc_t *agc;
	short **buf;
	void *ths;
	qtk_agc_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;

	int channel;
	int is_start;
	int winslen;
	char *zdata;
	wtk_strbuf_t *out_buf;

	wtk_wavfile_t *micwav;
	wtk_wavfile_t *echowav;
}qtk_agc_t;

qtk_agc_t *qtk_agc_new(qtk_agc_cfg_t *cfg);
int qtk_agc_delete(qtk_agc_t *qform);
int qtk_agc_start(qtk_agc_t *qform);
int qtk_agc_reset(qtk_agc_t *qform);
int qtk_agc_feed(qtk_agc_t *qform, char *data, int len, int is_end);
int qtk_agc_feed2(qtk_agc_t *qform, char *input, int in_bytes, char *output, int *out_bytes, int is_end);
void qtk_agc_set_notify(qtk_agc_t *qform, void *ths, qtk_agc_notify_f notify);
void qtk_agc_set_notify2(qtk_agc_t *qform, void *ths, qtk_engine_notify_f notify);
void qtk_agc_set_denoiseenable(qtk_agc_t *qform, int enable);
void qtk_agc_set_noisesupress(qtk_agc_t *qform, float noisesuppress);

#ifdef __cplusplus
};
#endif
#endif
