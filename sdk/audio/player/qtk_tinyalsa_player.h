#if (defined __ANDROID__) || (defined USE_XDW)

#ifndef SDK_AUDIO_PLAYER_QTK_TINYALSA_PLAYER
#define SDK_AUDIO_PLAYER_QTK_TINYALSA_PLAYER

#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_strbuf.h"

#include "tinyalsa/asoundlib.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_tinyalsa_player qtk_tinyalsa_player_t;
struct qtk_tinyalsa_player
{
	struct pcm_config config;
	struct pcm *pcm;
	int size;
	wtk_strbuf_t *writebuf;
};

qtk_tinyalsa_player_t* qtk_tinyalsa_player_start(void *h,
		char *pdev,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int period_time
		);
int qtk_tinyalsa_player_write(void *h,qtk_tinyalsa_player_t* r,char *buf,int bytes);
int qtk_tinyalsa_player_stop(void*h,qtk_tinyalsa_player_t* r);


#ifdef __cplusplus
};
#endif
#endif

#endif
