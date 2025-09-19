#ifndef G_2AF0980F88254534AE66D8B67FDD83FD
#define G_2AF0980F88254534AE66D8B67FDD83FD
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/signal/stft/qtk_stft_librosa_cfg.h"

typedef struct qtk_whisper_cfg qtk_whisper_cfg_t;

struct qtk_whisper_cfg {
    qtk_nnrt_cfg_t encoder_rt;
    qtk_nnrt_cfg_t decoder_rt;
    qtk_nnrt_cfg_t decoder_with_past_rt;
    qtk_stft_librosa_cfg_t stft;
    float chunk_duration;
    int lang_start_token;
    int decoder_start_token_id;
    int task_transcribe_id;
    int no_timestamps_token_id;
    int zh_lang_id;
    int en_lang_id;
    int eos_id;
    char *token2lang_fn;
    char *vocab_fn;
    char *vocab_en_fn;
    char *mel_filter_fn;
    int max_decode_len;

    wtk_heap_t *heap;
    wtk_array_t *vocab;
    wtk_array_t *vocab_en;
    float *mel_filter;

    int T;
    int K;
    int nlang;
    wtk_string_t **lang;
};

int qtk_whisper_cfg_init(qtk_whisper_cfg_t *cfg);
int qtk_whisper_cfg_clean(qtk_whisper_cfg_t *cfg);
int qtk_whisper_cfg_update(qtk_whisper_cfg_t *cfg);
int qtk_whisper_cfg_update_local(qtk_whisper_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_whisper_cfg_update2(qtk_whisper_cfg_t *cfg, wtk_source_loader_t *sl);

#endif
