#ifndef __QTK_PLAYER2_H__
#define __QTK_PLAYER2_H__
#include "qtk_player_cfg.h"
#if (defined __ANDROID__) || (defined USE_XDW)
#include "qtk_tinyalsa_player.h"
#else
#include "qtk_alsa_player.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_player2{
	qtk_player_cfg_t *cfg;
#if (defined __ANDROID__) || (defined USE_XDW)
    qtk_tinyalsa_player_t *alsa;
#else
    qtk_alsa_player_t *alsa;
#endif
}qtk_player2_t;

qtk_player2_t *qtk_player2_new(qtk_player_cfg_t *cfg);
int qtk_player2_start(qtk_player2_t *play);
int qtk_player2_stop(qtk_player2_t *play);
int qtk_player2_delete(qtk_player2_t *play);
int qtk_player2_write(qtk_player2_t *play, char *data, int len);
#ifdef __cplusplus
};
#endif
#endif
