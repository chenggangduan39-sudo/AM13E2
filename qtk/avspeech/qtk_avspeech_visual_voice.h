#ifndef G_6BDC7C3E34F449A9A9E35DD3B026D0F9
#define G_6BDC7C3E34F449A9A9E35DD3B026D0F9

#include "qtk/avspeech/qtk_avspeech_visual_voice_cfg.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/signal/stft/qtk_stft_librosa.h"

typedef struct qtk_avspeech_visual_voice qtk_avspeech_visual_voice_t;

struct qtk_avspeech_visual_voice {
    qtk_avspeech_visual_voice_cfg_t *cfg;
    qtk_nnrt_t *av_sep;
    qtk_nnrt_t *lip_embedding;
    qtk_stft_librosa_t *stft;
    float *input_freq_channel_first;
    short *output_ref;
    float *audio_segment;
    float *lip_segment;

    qtk_nnrt_value_t lip_embedding_input;
    qtk_nnrt_value_t av_sep_input1;

    uint16_t *lip_embedding_fp16;
    uint16_t *av_sep_input1_fp16;
    qtk_nnrt_value_t av_sep_input2;
};

qtk_avspeech_visual_voice_t *
qtk_avspeech_visual_voice_new(qtk_avspeech_visual_voice_cfg_t *cfg);
void qtk_avspeech_visual_voice_delete(qtk_avspeech_visual_voice_t *vv);
int qtk_avspeech_visual_voice_feed(qtk_avspeech_visual_voice_t *vv,
                                   wtk_complex_t *feat,
                                   const uint8_t *lip_patch);
float qtk_avspeech_visual_voice_get_audio_feature(
    qtk_avspeech_visual_voice_t *vv, const short *in_audio, wtk_complex_t *F);
int qtk_avspeech_visual_voice_synthesis_audio(qtk_avspeech_visual_voice_t *vv,
                                                short *out_audio,
                                                wtk_complex_t *freq,
                                                float normalizer);

#endif
