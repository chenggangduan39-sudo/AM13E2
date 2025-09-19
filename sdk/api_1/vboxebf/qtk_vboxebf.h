#ifndef __QTK_API_VBOXEBF_H__
#define __QTK_API_VBOXEBF_H__
#include "qtk_vboxebf_cfg.h"
#include "wtk/bfio/vbox/wtk_vboxebf3.h"
#include "wtk/bfio/vbox/wtk_vboxebf4.h"
#include "wtk/bfio/vbox/wtk_vboxebf6.h"
#include "wtk/bfio/maskbfnet/wtk_mask_bf_net.h"
#ifdef USE_AHS
#include "wtk/bfio/ahs/qtk_ahs.h"
#endif
#include "sdk/codec/qtk_audio_conversion.h"
#include "sdk/qtk_api.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/os/wtk_thread.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_vboxebf_notify_f)(void *ths, char *data, int len);

typedef struct qtk_vboxebf{
	qtk_vboxebf_cfg_t *cfg;
	wtk_vboxebf3_t *vebf3;
	wtk_vboxebf4_t *vebf4;
	wtk_vboxebf6_t *vebf6;
	wtk_mask_bf_net_t *mask_bf_net;
#ifdef USE_AHS
	qtk_ahs_t *ahs;
#endif
	short **buf;
	void *ths;
	qtk_vboxebf_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	wtk_thread_t ssl_delay_t;
	
	int *nbest_theta;
	int *nbest_phi;
	int *concount;
	char *zdata;
	
	int channel;
	int winslen;
	int use_line;
	int is_start;
    wtk_strbuf_t *cache_buf;
	wtk_strbuf_t *out_buf;
	wtk_strbuf_t *sample_bytes_in;
	wtk_strbuf_t *sample_bytes_out;

	wtk_wavfile_t *micwav;
	wtk_wavfile_t *echowav;
}qtk_vboxebf_t;

qtk_vboxebf_t *qtk_vboxebf_new(qtk_vboxebf_cfg_t *cfg);
void qtk_vboxebf_ssl_delay_new(qtk_vboxebf_t *vb);
int qtk_vboxebf_delete(qtk_vboxebf_t *qform);
int qtk_vboxebf_start(qtk_vboxebf_t *qform);
int qtk_vboxebf_reset(qtk_vboxebf_t *qform);
int qtk_vboxebf_feed(qtk_vboxebf_t *qform, char *data, int len, int is_end);
int qtk_vboxebf_feed2(qtk_vboxebf_t *vb, char *intput, int in_bytes, char *output, int *out_bytes, int is_end);
void qtk_vboxebf_set_notify(qtk_vboxebf_t *qform, void *ths, qtk_vboxebf_notify_f notify);
void qtk_vboxebf_set_notify2(qtk_vboxebf_t *qform, void *ths, qtk_engine_notify_f notify);
void qtk_vboxebf_set_agcenable(qtk_vboxebf_t *vb, int enable);
void qtk_vboxebf_set_echoenable(qtk_vboxebf_t *vb, int enable);
void qtk_vboxebf_set_denoiseenable(qtk_vboxebf_t *vb, int enable);
void qtk_vboxebf_set_ssl_enable(qtk_vboxebf_t *vb, int enable);

#ifdef __cplusplus
};
#endif
#endif
