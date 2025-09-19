#ifndef QTK_AUDIO_RECORDER_QTK_ALSA_RECORDER
#define QTK_AUDIO_RECORDER_QTK_ALSA_RECORDER

#include <alsa/asoundlib.h>

#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_type.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_alsa_recorder qtk_alsa_recorder_t;
struct qtk_alsa_recorder {
    snd_pcm_t *pcm;
    unsigned int rate;
    unsigned int channel;
    unsigned int bytes_per_sample;
};

qtk_alsa_recorder_t *qtk_alsa_recorder_start(char *name, int sample_rate,
                                             int channel, int bytes_per_sample,
                                             int buf_time);
int qtk_alsa_recorder_read(qtk_alsa_recorder_t *r, char *data, int bytes);
int qtk_alsa_recorder_stop(qtk_alsa_recorder_t *r);

#ifdef __cplusplus
};
#endif
#endif
