#ifndef G_DDFC16BCDA63418EB5BE857073AFD4BF
#define G_DDFC16BCDA63418EB5BE857073AFD4BF

#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/asr/whisper/qtk_whisper_cfg.h"
#include "wtk/signal/stft/qtk_stft_librosa.h"

typedef struct qtk_whisper qtk_whisper_t;

struct qtk_whisper {
    qtk_whisper_cfg_t *cfg;
    qtk_nnrt_t *encoder_rt;
    qtk_nnrt_t *decoder_rt;
    qtk_nnrt_t *decoder_with_past_rt;
    qtk_nnrt_value_t input_features;
    qtk_stft_librosa_t *stft;
    float *audio_chunk;
    void *feat_blob;
    float *magnitudes;
    void *cache_blob[24];
    qtk_nnrt_value_t cache_val[24];
    wtk_strbuf_t *result_buf;

    int feat_idx;
};

qtk_whisper_t *qtk_whisper_new(qtk_whisper_cfg_t *cfg);
void qtk_whisper_delete(qtk_whisper_t *w);
int qtk_whisper_transcribe(qtk_whisper_t *w, short *wav, int len,
                           wtk_string_t *result);

#endif
