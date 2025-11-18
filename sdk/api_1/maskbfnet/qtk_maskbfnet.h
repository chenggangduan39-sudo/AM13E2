#ifndef __QTK_API_MASKBFNET_H__
#define __QTK_API_MASKBFNET_H__
#include "wtk/bfio/maskbfnet/wtk_mask_bf_net.h"
#include "qtk_maskbfnet_cfg.h"
#include "sdk/qtk_api.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_maskbfnet_notify_f)(void *ths, short **data, int len, int is_end);
typedef struct qtk_maskbfnet{
	qtk_maskbfnet_cfg_t *cfg;
	wtk_mask_bf_net_t *maskbfnet;
	short **buf;
	void *ths;
	qtk_maskbfnet_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;

	int channel;
	int is_start;
	int winslen;
	char *zdata;
	wtk_strbuf_t *out_buf;

	wtk_wavfile_t *micwav;
	wtk_wavfile_t *echowav;
}qtk_maskbfnet_t;

qtk_maskbfnet_t *qtk_maskbfnet_new(qtk_maskbfnet_cfg_t *cfg);
int qtk_maskbfnet_delete(qtk_maskbfnet_t *qform);
int qtk_maskbfnet_start(qtk_maskbfnet_t *qform);
int qtk_maskbfnet_reset(qtk_maskbfnet_t *qform);
int qtk_maskbfnet_feed(qtk_maskbfnet_t *qform, char *data, int len, int is_end);
int qtk_maskbfnet_feed2(qtk_maskbfnet_t *qform, char *input, int in_bytes, char *output, int *out_bytes, int is_end);
void qtk_maskbfnet_set_notify(qtk_maskbfnet_t *qform, void *ths, qtk_maskbfnet_notify_f notify);
void qtk_maskbfnet_set_notify2(qtk_maskbfnet_t *qform, void *ths, qtk_engine_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
