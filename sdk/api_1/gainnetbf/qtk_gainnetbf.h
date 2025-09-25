#ifndef __QTK_API_GAINNET_BF_H__
#define __QTK_API_GAINNET_BF_H__
#include "qtk_gainnetbf_cfg.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf3.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf4.h"
#include "wtk/bfio/maskform/wtk_gainnet_bf6.h"
#include "wtk/bfio/qform/wtk_rtjoin2.h"
#include "wtk/bfio/qform/wtk_mix_speech.h"
#include "sdk/codec/qtk_audio_conversion.h"
#include "sdk/qtk_api.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef void (*qtk_gainnetbf_notify_f)(void *ths, char *data, int len);

typedef struct qtk_gainnetbf{
	qtk_gainnetbf_cfg_t *cfg;
	wtk_gainnet_bf_t *gainnet_bf;
	wtk_gainnet_bf3_t *gainnet_bf3;
	wtk_gainnet_bf4_t *gainnet_bf4;
	wtk_gainnet_bf6_t *gainnet_bf6;
	wtk_rtjoin2_t *rtjoin2;
	wtk_mix_speech_t *mix_speech;
	short **buf;
	void *ths;
	qtk_gainnetbf_notify_f notify;
	void *eths;
	qtk_engine_notify_f enotify;
	int channel;
	
	char *zdata;
	float energy;
	int energy_cnt;
	wtk_wavfile_t *iwav;
	wtk_wavfile_t *owav;

	int *nbest_theta;
	int *nbest_phi;
	int *concount;

	wtk_strbuf_t *feed_buf;
    wtk_strbuf_t *cache_buf;
	wtk_strbuf_t *out_buf;
}qtk_gainnetbf_t;

qtk_gainnetbf_t *qtk_gainnetbf_new(qtk_gainnetbf_cfg_t *cfg);
int qtk_gainnetbf_delete(qtk_gainnetbf_t *qform);
int qtk_gainnetbf_start(qtk_gainnetbf_t *qform);
int qtk_gainnetbf_reset(qtk_gainnetbf_t *qform);
int qtk_gainnetbf_set_shift(qtk_gainnetbf_t *qform, float mic_shift, float echo_shift);
int qtk_gainnetbf_feed(qtk_gainnetbf_t *qform, char *data, int len, int is_end);
void qtk_gainnetbf_set_notify(qtk_gainnetbf_t *qform, void *ths, qtk_gainnetbf_notify_f notify);
void qtk_gainnetbf_set_notify2(qtk_gainnetbf_t *qform, void *ths, qtk_engine_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
