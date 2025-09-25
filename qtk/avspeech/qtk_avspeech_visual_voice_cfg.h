#ifndef G_48E6B84A63054B73BEB6D9368903B7AE
#define G_48E6B84A63054B73BEB6D9368903B7AE
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/signal/stft/qtk_stft_librosa_cfg.h"
typedef struct qtk_avspeech_visual_voice_cfg qtk_avspeech_visual_voice_cfg_t;

struct qtk_avspeech_visual_voice_cfg {
    qtk_nnrt_cfg_t av_sep;
    qtk_nnrt_cfg_t lip_embedding;
    qtk_stft_librosa_cfg_t stft;
    float audio_segment_duration;
    int num_frames;
    int lip_embedding_dim;
    int fps;
};

int qtk_avspeech_visual_voice_cfg_init(qtk_avspeech_visual_voice_cfg_t *cfg);
int qtk_avspeech_visual_voice_cfg_clean(qtk_avspeech_visual_voice_cfg_t *cfg);
int qtk_avspeech_visual_voice_cfg_update(qtk_avspeech_visual_voice_cfg_t *cfg);
int qtk_avspeech_visual_voice_cfg_update_local(
    qtk_avspeech_visual_voice_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_avspeech_visual_voice_cfg_update2(qtk_avspeech_visual_voice_cfg_t *cfg,
                                          wtk_source_loader_t *sl);

#endif
