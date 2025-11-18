#ifndef QTK_AUDIO_PLAYER_QTK_ALSA_PLAYER
#define QTK_AUDIO_PLAYER_QTK_ALSA_PLAYER
#ifdef WIN32
#elif __ANDROID__
#elif OPENAL
#elif __mips
#else

#ifndef DEBUG_FILE
#include <alsa/asoundlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_alsa_player qtk_alsa_player_t;

#ifdef DEBUG_FILE
struct qtk_alsa_player
{
	int channel;
	int bytes_per_sample;
	char *zerobuf;
};
#else
struct qtk_alsa_player
{
	char *snd_name;
	snd_pcm_t *pcm;
	int channel;
	unsigned int rate;
	int buf_time;
	int period_time;
	int start_time;
	int stop_time;
	int avail_time;
	int silence_time;
	int bytes_per_sample;
	int bc;
	int on;
	int err;
	int use_uac;
	char *zerobuf;
};
#endif

qtk_alsa_player_t* qtk_alsa_player_new(
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time
		);

void qtk_alsa_player_delete(qtk_alsa_player_t *player);

int qtk_alsa_player_stop2(qtk_alsa_player_t *player);

qtk_alsa_player_t* qtk_alsa_player_start2(
		void *h,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int period_time,
		int use_uac
		);
qtk_alsa_player_t* qtk_alsa_player_start(
		void *h,
		char *name,
		int sample_rate,
		int channel,
		int bytes_per_sample,
		int buf_time,
		int period_time,
		int start_time,
		int stop_time,
		int avail_min_time,
		int silence_time,
		int use_uac
		);
int qtk_alsa_player_stop(void *h, qtk_alsa_player_t *player);

long qtk_alsa_player_write(void *h, qtk_alsa_player_t *player,char *data,int bytes);
int qtk_alsa_player_get_count(qtk_alsa_player_t *player);

#ifdef __cplusplus
};
#endif
#endif
#endif
