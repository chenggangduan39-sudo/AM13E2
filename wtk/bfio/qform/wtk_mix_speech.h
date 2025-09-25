#ifndef WTK_BFIO_QFORM_WTK_MIX_SPEECH
#define WTK_BFIO_QFORM_WTK_MIX_SPEECH
#include "wtk/core/wtk_strbuf.h"
#include "wtk_mix_speech_cfg.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_mix_speech wtk_mix_speech_t;
typedef void(*wtk_mix_speech_notify_f)(void *ths,short *output,int len);

struct wtk_mix_speech
{
	wtk_mix_speech_cfg_t *cfg;

	wtk_strbuf_t **mic;

	short *out;

	void *ths;
	wtk_mix_speech_notify_f notify;

	float *mic_scale;

    int mic_silcnt;
    unsigned mic_sil:1;
};

wtk_mix_speech_t* wtk_mix_speech_new(wtk_mix_speech_cfg_t *cfg);
void wtk_mix_speech_delete(wtk_mix_speech_t *mix_speech);
void wtk_mix_speech_start(wtk_mix_speech_t *mix_speech);
void wtk_mix_speech_reset(wtk_mix_speech_t *mix_speech);
void wtk_mix_speech_set_notify(wtk_mix_speech_t *mix_speech,void *ths,wtk_mix_speech_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_mix_speech_feed(wtk_mix_speech_t *mix_speech,short *data,int len,int is_end);
#ifdef __cplusplus
};
#endif
#endif
