#ifndef __QTK_ALSA_RECORDER_H__
#define __QTK_ALSA_RECORDER_H__

#ifndef DEBUG_FILE
#include <alsa/asoundlib.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_alsa_recorder qtk_alsa_recorder_t;

#ifdef DEBUG_FILE
#include "wtk/core/wtk_riff.h"
struct qtk_alsa_recorder
{
	unsigned int rate;
	unsigned int channel;
	unsigned int bytes_per_sample;
	wtk_riff_t *riff;
};

#else
struct qtk_alsa_recorder
{
	snd_pcm_t *pcm;
	unsigned int rate;
	unsigned int channel;
	unsigned int bytes_per_sample;
};
#endif



qtk_alsa_recorder_t* qtk_alsa_recorder_start(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		);
qtk_alsa_recorder_t* qtk_alsa_recorder_start2(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		);
int qtk_alsa_recorder_read(qtk_alsa_recorder_t *r,char *data,int bytes);
int qtk_alsa_recorder_stop(qtk_alsa_recorder_t *r);

int setCapVal(int val);
int getCapVal(int val);
#ifdef __cplusplus
};
#endif
#endif
