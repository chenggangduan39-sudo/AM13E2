#ifndef __QTK_TTS_PROSODY_H__
#define __QTK_TTS_PROSODY_H__

#include "qtk_tts_prosody_cfg.h"
#include "ncrf/qtk_tts_ncrf.h"

#ifdef __cplusplus
extern "C"{
#endif

typedef struct qtk_tts_prosody{
    qtk_tts_prosody_cfg_t *cfg;
    qtk_tts_ncrf_t *ncrf;

    wtk_mati_t **chn_symbols_id;
    wtk_veci_t **rhythm_vec;
    wtk_string_t **prosody_list;
    int nids;
}qtk_tts_prosody_t;

qtk_tts_prosody_t *qtk_tts_prosody_new(qtk_tts_prosody_cfg_t *cfg, wtk_rbin2_t* rbin);
int qtk_tts_prosody_process(qtk_tts_prosody_t *prosody,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
int qtk_tts_prosody_delete(qtk_tts_prosody_t *prosody);
int qtk_tts_prosody_print(qtk_tts_prosody_t *prosody);


#ifdef __cplusplus
};
#endif

#endif
