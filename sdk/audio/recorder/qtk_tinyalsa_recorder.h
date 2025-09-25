#if (defined __ANDROID__) || (defined USE_XDW)

#ifndef SDK_AUDIO_RECORDER_QTK_TINYALSA_RECORDER
#define SDK_AUDIO_RECORDER_QTK_TINYALSA_RECORDER

#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"

#include "tinyalsa/asoundlib.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_tinyalsa_recorder qtk_tinyalsa_recorder_t;
struct qtk_tinyalsa_recorder
{
	struct pcm_config config;
	struct pcm *pcm;
	int size;
	char *data;
	wtk_strbuf_t *readbuf;
};

qtk_tinyalsa_recorder_t* qtk_tinyalsa_recorder_start(void *h,
		char *pdev,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		);
int qtk_tinyalsa_recorder_read(void *h,qtk_tinyalsa_recorder_t* r,char *buf,int bytes);
int qtk_tinyalsa_recorder_stop(void*h,qtk_tinyalsa_recorder_t* r);
// int qtk_tinyalsa_recorder_get_bytes();


#ifdef __cplusplus
};
#endif
#endif

#endif
