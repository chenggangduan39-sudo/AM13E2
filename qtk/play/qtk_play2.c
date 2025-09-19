#include "qtk_play2.h"

qtk_play2_t *qtk_play2_new(qtk_play_cfg_t *cfg)
{
    qtk_play2_t *play;

    play = (qtk_play2_t *)calloc(1, sizeof(*play));
	play->cfg = cfg;
    return play;
    
}
int qtk_play2_delete(qtk_play2_t *play)
{
    free(play);
    return 0;
}

int qtk_play2_start(qtk_play2_t *play)
{
	//printf("%.*s\n", play->cfg->snd_name.len, play->cfg->snd_name.data);
	play->alsa = qtk_alsa_player_start(play->cfg->snd_name.data, play->cfg->sample_rate, play->cfg->channel, play->cfg->bytes_per_sample, play->cfg->buf_time, play->cfg->period_time, play->cfg->use_uac);
    if(!play->alsa){
        return -1;
    }
    return 0;
}
int qtk_play2_stop(qtk_play2_t *play)
{
    return qtk_alsa_player_stop(play->alsa);
}

int qtk_play2_write(qtk_play2_t *play, char *data, int len)
{
    return qtk_alsa_player_write(play->alsa, data, len);
}
