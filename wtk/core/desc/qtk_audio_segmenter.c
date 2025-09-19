#include "wtk/core/desc/qtk_audio_segmenter.h"

int qtk_audio_segmenter_init(qtk_audio_segmenter_t *s, const char *audio_fn,
                             const char *seg_desc_fn, float offset) {
    s->riff = wtk_riff_new();
    wtk_riff_open(s->riff, cast(char *, audio_fn));
    wtk_source_init_file(&s->seg_desc, cast(char *, seg_desc_fn));
    s->buf = wtk_strbuf_new(1024, 1);
    s->offset = offset;
    return 0;
}

int qtk_audio_segmenter_clean(qtk_audio_segmenter_t *s) {
    wtk_riff_delete(s->riff);
    wtk_source_clean_file(&s->seg_desc);
    wtk_strbuf_delete(s->buf);
    return 0;
}

int qtk_audio_segmenter_next(qtk_audio_segmenter_t *s, wtk_string_t *k,
                             wtk_string_t *audio, float *fs, float *fe) {
    uint32_t start, end, length, ret;
    if (wtk_source_read_string(&s->seg_desc, s->buf) ||
        s->buf->pos > sizeof(s->key) - 1) {
        goto err;
    }

    memcpy(s->key, s->buf->data, s->buf->pos);
    s->key[s->buf->pos] = '\0';
    *k = (wtk_string_t){s->key, s->buf->pos};

    if (wtk_source_read_float(&s->seg_desc, fs, 1, 0) ||
        wtk_source_read_float(&s->seg_desc, fe, 1, 0)) {
        goto err;
    }

    if (*fe <= *fs) {
        goto err;
    }

    *fs += s->offset;
    *fe += s->offset;

    start = s->riff->fmt.sample_rate * s->riff->fmt.channels * *fs *
            (s->riff->fmt.bit_per_sample / 8);
    end = s->riff->fmt.sample_rate * s->riff->fmt.channels * *fe *
          (s->riff->fmt.bit_per_sample / 8);
    length = end - start;

    fseek(s->riff->file, start, SEEK_SET);

    wtk_strbuf_reset(s->buf);
    wtk_strbuf_expand(s->buf, length);
    ret = wtk_riff_read(s->riff, s->buf->data, length);
    if (ret == 0) {
        goto err;
    }

    *audio = (wtk_string_t){s->buf->data, ret};

    return 1;
err:
    return 0;
}
