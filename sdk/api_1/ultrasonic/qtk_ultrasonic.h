#ifndef __QTK_API_ULTRASONIC_H__
#define __QTK_API_ULTRASONIC_H__
#include "qtk_ultrasonic_cfg.h"
#include "qtk/ult/evm2/qtk_ultevm2.h"
#include "qtk/ult/qtk_ult_track.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "sdk/qtk_api.h"
#include "wtk/core/wtk_wavfile.h"
#include "qtk/ult/qtk_ult_track_post.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_ultrasonic_notify_f)(void *ths, int type);

typedef struct qtk_ultrasonic{
	qtk_ultrasonic_cfg_t *cfg;
	qtk_ultevm2_t *ultevm;
	qtk_ult_track_t *ult_track;
	qtk_ult_track_post_cfg_t post_cfg;
	qtk_ult_track_post_t *post;
	short **buf;
	void *ths;
	qtk_ultrasonic_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
	
	wtk_wavfile_t *iwav;
	wtk_wavfile_t *owav;

	wtk_strbuf_t *feed_buf;
}qtk_ultrasonic_t;

qtk_ultrasonic_t *qtk_ultrasonic_new(qtk_ultrasonic_cfg_t *cfg);
int qtk_ultrasonic_delete(qtk_ultrasonic_t *qform);
int qtk_ultrasonic_start(qtk_ultrasonic_t *qform);
int qtk_ultrasonic_reset(qtk_ultrasonic_t *qform);
int qtk_ultrasonic_get_signal(qtk_ultrasonic_t *vb, short **data);
int qtk_ultrasonic_feed(qtk_ultrasonic_t *qform, char *data, int len, int is_end);
void qtk_ultrasonic_set_notify(qtk_ultrasonic_t *qform, void *ths, qtk_ultrasonic_notify_f notify);
void qtk_ultrasonic_set_notify2(qtk_ultrasonic_t *qform, void *ths, qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
