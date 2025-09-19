#ifndef __QTK_PLAY2_H__
#define __QTK_PLAY2_H__
#include "qtk_alsa_player.h"
#include "qtk_play_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_play2{
	qtk_play_cfg_t *cfg;
    qtk_alsa_player_t *alsa;
}qtk_play2_t;

qtk_play2_t *qtk_play2_new(qtk_play_cfg_t *cfg);
int qtk_play2_start(qtk_play2_t *play);
int qtk_play2_stop(qtk_play2_t *play);
int qtk_play2_delete(qtk_play2_t *play);
int qtk_play2_write(qtk_play2_t *play, char *data, int len);
#ifdef __cplusplus
};
#endif
#endif
