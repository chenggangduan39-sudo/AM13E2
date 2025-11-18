#ifdef WIN32
#elif __ANDROID__
#elif OPENAL
#elif __mips
#else

#ifndef SDK_AUDIO_RECORDER_QTK_ALSA_RECORDER
#define SDK_AUDIO_RECORDER_QTK_ALSA_RECORDER

#include <alsa/asoundlib.h>

#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_alsa_recorder qtk_alsa_recorder_t;
struct qtk_alsa_recorder
{
	snd_pcm_t *pcm;
	unsigned int rate;
	unsigned int channel;
	unsigned int bytes_per_sample;
};

qtk_alsa_recorder_t* qtk_alsa_recorder_start(void *h,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		);
int qtk_alsa_recorder_read(void *h,qtk_alsa_recorder_t *r,char *data,int bytes);
int qtk_alsa_recorder_stop(void *h,qtk_alsa_recorder_t *r);

#ifdef __cplusplus
};
#endif
#endif

#endif
