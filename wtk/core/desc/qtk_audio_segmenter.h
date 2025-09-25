#ifndef __WTK_CORE_DESC_QTK_AUDIO_SEGMENT_H__
#define __WTK_CORE_DESC_QTK_AUDIO_SEGMENT_H__
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_riff.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_audio_segmenter qtk_audio_segmenter_t;

struct qtk_audio_segmenter {
    char key[512];
    float offset;
    wtk_strbuf_t *buf;

    wtk_riff_t *riff;
    wtk_source_t seg_desc;
    wtk_string_t v;
};

int qtk_audio_segmenter_init(qtk_audio_segmenter_t *s, const char *audio_fn,
                             const char *seg_desc_fn, float offset);
int qtk_audio_segmenter_clean(qtk_audio_segmenter_t *s);
int qtk_audio_segmenter_next(qtk_audio_segmenter_t *s, wtk_string_t *k,
                             wtk_string_t *audio, float *fs, float *fe);

#ifdef __cplusplus
};
#endif
#endif
