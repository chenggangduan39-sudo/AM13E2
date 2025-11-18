#include "qtk_player2.h"

qtk_player2_t *qtk_player2_new(qtk_player_cfg_t *cfg)
{
    qtk_player2_t *play;

    play = (qtk_player2_t *)calloc(1, sizeof(*play));
	play->cfg = cfg;
    return play;
    
}
int qtk_player2_delete(qtk_player2_t *play)
{
    free(play);
    return 0;
}

int qtk_player2_start(qtk_player2_t *play)
{
#if (defined __ANDROID__) || (defined USE_XDW)
	play->alsa = qtk_tinyalsa_player_start(NULL, play->cfg->snd_name, play->cfg->sample_rate, play->cfg->channel, play->cfg->bytes_per_sample, play->cfg->buf_time, play->cfg->period_time);
    if(!play->alsa){
        return -1;
    }
#else
	play->alsa = qtk_alsa_player_start(NULL, play->cfg->snd_name, play->cfg->sample_rate, play->cfg->channel, play->cfg->bytes_per_sample,
                        play->cfg->buf_time,
                        play->cfg->period_time,
                        play->cfg->start_time,
						play->cfg->stop_time,
						play->cfg->avail_time,
						play->cfg->silence_time,
                        play->cfg->use_uac);
    if(!play->alsa){
        return -1;
    }
#endif
    return 0;
}
int qtk_player2_stop(qtk_player2_t *play)
{
#if (defined __ANDROID__) || (defined USE_XDW)
    return qtk_tinyalsa_player_stop(NULL, play->alsa);
#else
    return qtk_alsa_player_stop(NULL, play->alsa);
#endif
}

int qtk_player2_write(qtk_player2_t *play, char *data, int len)
{
#if (defined __ANDROID__) || (defined USE_XDW)
    return qtk_tinyalsa_player_write(NULL,play->alsa, data, len);
#else
    return qtk_alsa_player_write(NULL,play->alsa, data, len);
#endif
}
