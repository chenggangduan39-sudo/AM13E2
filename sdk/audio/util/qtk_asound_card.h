#ifndef SDK_AUDIO_QTK_ASOUND_CARD
#define SDK_AUDIO_QTK_ASOUND_CARD

#define QTK_ASOUND_CARD_PATH "/proc/asound/cards"
#define QTK_ASOUND_BUFSIZE 2048
#include "wtk/core/wtk_type.h"
#include "wtk/os/wtk_log.h"

#ifdef __cplusplus
extern "C" {
#endif

int qtk_asound_get_card();

#ifdef __cplusplus
};
#endif
#endif
