#ifndef __QTK_SOUNDSCREEN_H__
#define __QTK_SOUNDSCREEN_H__
#include "qtk_soundscreen_cfg.h"
#include "wtk/bfio/qform/wtk_qform9.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet2.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet3.h"
#include "wtk/bfio/qform/beamnet/wtk_beamnet4.h"
#include "wtk/bfio/qform/cmask_bfse/wtk_cmask_bfse.h"
#include "wtk/bfio/aec/wtk_aec.h"
#include "wtk/bfio/vbox/wtk_vboxebf3.h"
#include "wtk/os/wtk_thread.h"
#include "sdk/qtk_api.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef void(*qtk_soundscreen_notify_f)(void *ths, char *data, int len);
typedef struct qtk_soundscreen{
	qtk_soundscreen_cfg_t *cfg;
	wtk_qform9_t **qform;
	wtk_beamnet2_t **beamnet2;
	wtk_beamnet3_t **beamnet3;
	wtk_beamnet4_t **beamnet4;
	wtk_cmask_bfse_t *cmask_bfse;
	wtk_aec_t *aec;
	wtk_vboxebf3_t *vboxebf3;
	wtk_strbuf_t **bufs;
	wtk_strbuf_t *buf;
	wtk_strbuf_t *input;
	short **data;
	short **vboxdata;
	short *edata;
	int channel;

	void *ths;
	qtk_soundscreen_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
}qtk_soundscreen_t;

qtk_soundscreen_t *qtk_soundscreen_new(qtk_soundscreen_cfg_t *cfg);
void qtk_soundscreen_delete(qtk_soundscreen_t *sc);
int qtk_soundscreen_start(qtk_soundscreen_t *sc);
int qtk_soundscreen_reset(qtk_soundscreen_t *sc);
int qtk_soundscreen_feed(qtk_soundscreen_t *sc, char *data, int len, int is_end);
int qtk_soundscreen_feed2(qtk_soundscreen_t *sc, short **data, int len, int is_end);
void qtk_soundscreen_set_notify(qtk_soundscreen_t *sc, void *ths, qtk_soundscreen_notify_f notify);
void qtk_soundscreen_set_notify2(qtk_soundscreen_t *sc, void *eths, qtk_engine_notify_f enotify);
int qtk_soundscreen_set_denoiseenable(qtk_soundscreen_t *sc, int enable);
int qtk_soundscreen_set_noise_suppress(qtk_soundscreen_t *sc, float noise_suppress);
int qtk_soundscreen_set_out_scale(qtk_soundscreen_t *sc, float scale);
int qtk_soundscreen_set_agcenable(qtk_soundscreen_t *sc, int enable);

#ifdef __cplusplus
};
#endif
#endif
