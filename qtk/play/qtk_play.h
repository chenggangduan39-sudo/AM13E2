#ifndef __QTK_PLAY_H__
#define __QTK_PLAY_H__
#include "qtk_alsa_player.h"
#include "qtk_play_cfg.h"
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

#pragma pack(1)
typedef struct
{
	unsigned char	zero;		// 保留
	unsigned char	indx;		// 音频通道编号
	unsigned short	data;		// 有效音频数据
}audio_frame_t;
#pragma pack()
#define AUDIO_DATA_CHANNEL (8)

typedef struct qtk_play{
	qtk_play_cfg_t *cfg;
    qtk_alsa_player_t *alsa;
    audio_frame_t *frames;
    short *data_8ch;
    int frames_len;
    // int step;
}qtk_play_t;

qtk_play_t *qtk_play_new(qtk_play_cfg_t *cfg);
int qtk_play_start(qtk_play_t *play);
int qtk_play_stop(qtk_play_t *play);
int qtk_play_delete(qtk_play_t *play);
long qtk_play_write(qtk_play_t *play, char *data, int len, int channel);
int qtk_play_get_count(qtk_play_t *play);
int qtk_play_data_frame_fill(qtk_play_t *play, char *data, int len, int channel);
long qtk_play_write_on_write(qtk_play_t *play, char *data, int len);
int qtk_play_get_sound_card(char *card_name);
#ifdef __cplusplus
};
#endif
#endif
