#ifndef __QTK_API_AEC_H__
#define __QTK_API_AEC_H__
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/bfio/aec/cmask_aec/wtk_cmask_aec.h"
#include "wtk/bfio/aec/cmask_aec/wtk_cmask_aec2.h"
#include "qtk_aec_cfg.h"
#include "sdk/qtk_api.h"
#include "sdk/codec/qtk_audio_conversion.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_aec_notify_f)(void *ths, short **data, int len, int is_end);
typedef struct qtk_aec{
	qtk_aec_cfg_t *cfg;
	wtk_aec_t *aec;
	wtk_cmask_aec_t *caec;
	wtk_cmask_aec2_t *caec2;
	short **buf;
	void *ths;
	qtk_aec_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;

	int channel;
	int is_start;
	int winslen;
	char *zdata;
	wtk_strbuf_t *out_buf;

	wtk_wavfile_t *micwav;
	wtk_wavfile_t *echowav;
}qtk_aec_t;

qtk_aec_t *qtk_aec_new(qtk_aec_cfg_t *cfg);
int qtk_aec_delete(qtk_aec_t *qform);
int qtk_aec_start(qtk_aec_t *qform);
int qtk_aec_reset(qtk_aec_t *qform);
int qtk_aec_feed(qtk_aec_t *qform, char *data, int len, int is_end);
void qtk_aec_set_notify(qtk_aec_t *qform, void *ths, qtk_aec_notify_f notify);
void qtk_aec_set_notify2(qtk_aec_t *qform, void *ths, qtk_engine_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
