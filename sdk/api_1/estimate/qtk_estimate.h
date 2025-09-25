#ifndef __QTK_API_ESTIMATE_H__
#define __QTK_API_ESTIMATE_H__
#include "qtk_estimate_cfg.h"
#include "wtk/bfio/ahs/estimate/qtk_rir_estimate2.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_estimate_notify_f)(void *ths, char *data, int len);

typedef struct qtk_estimate{
	qtk_estimate_cfg_t *cfg;
	qtk_rir_estimate2_t *rir_est;

	void *ths;
	qtk_estimate_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
}qtk_estimate_t;

qtk_estimate_t *qtk_estimate_new(qtk_estimate_cfg_t *cfg);
int qtk_estimate_delete(qtk_estimate_t *qform);
int qtk_estimate_start(qtk_estimate_t *qform);
int qtk_estimate_reset(qtk_estimate_t *qform);
int qtk_estimate_feed(qtk_estimate_t *qform, char *data, int len, int is_end);
float *qtk_estimate_code_generate(qtk_estimate_t *vb);
void qtk_estimate_set_notify(qtk_estimate_t *qform, void *ths, qtk_estimate_notify_f notify);
void qtk_estimate_set_notify2(qtk_estimate_t *qform, void *ths, qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
