#ifndef __QTK_API_BEAMNET_H__
#define __QTK_API_BEAMNET_H__
#include "wtk/bfio/qform/beamnet/wtk_beamnet.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet2.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet3.h"
#include "qtk_beamnet_cfg.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_beamnet_notify_f)(void *ths, char *data, int len);
typedef struct qtk_beamnet{
	qtk_beamnet_cfg_t *cfg;
	wtk_beamnet_t *qform;
	wtk_beamnet2_t *qform2;
	wtk_beamnet3_t *qform3;
	short **buf;
	void *ths;
	qtk_beamnet_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
    wtk_strbuf_t *cache_buf;
}qtk_beamnet_t;

qtk_beamnet_t *qtk_beamnet_new(qtk_beamnet_cfg_t *cfg);
int qtk_beamnet_delete(qtk_beamnet_t *qform);
int qtk_beamnet_start(qtk_beamnet_t *qform);
int qtk_beamnet_reset(qtk_beamnet_t *qform);
int qtk_beamnet_feed(qtk_beamnet_t *qform, char *data, int len, int is_end);
void qtk_beamnet_set_notify(qtk_beamnet_t *qform, void *ths, qtk_beamnet_notify_f notify);
void qtk_beamnet_set_notify2(qtk_beamnet_t *qform, void *ths, qtk_engine_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
