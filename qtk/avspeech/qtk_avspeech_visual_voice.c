#include "qtk/avspeech/qtk_avspeech_visual_voice.h"
#include "qtk/numeric/qtk_numeric_type.h"
#include "wtk/signal/qtk_normalize.h"

qtk_avspeech_visual_voice_t *
qtk_avspeech_visual_voice_new(qtk_avspeech_visual_voice_cfg_t *cfg) {
    qtk_avspeech_visual_voice_t *vv =
        wtk_malloc(sizeof(qtk_avspeech_visual_voice_t));
    int nsample = cfg->audio_segment_duration * 16000;
    int64_t shape[5] = {1, 1, cfg->num_frames, 88, 88};
    vv->cfg = cfg;
    vv->av_sep = qtk_nnrt_new(&cfg->av_sep);
    vv->lip_embedding = qtk_nnrt_new(&cfg->lip_embedding);
    vv->stft = qtk_stft_librosa_new(&cfg->stft, NULL, NULL);
    vv->input_freq_channel_first =
        wtk_malloc(sizeof(float) * 1 * 2 * (cfg->stft.n_fft / 2 + 1) *
                   (nsample / cfg->stft.hop_length + 1));
    vv->audio_segment = wtk_malloc(sizeof(float) * nsample);
    vv->lip_segment = wtk_malloc(sizeof(float) * 88 * 88 * cfg->num_frames);
    vv->lip_embedding_input = qtk_nnrt_value_create_external(
        vv->lip_embedding, QTK_NNRT_VALUE_ELEM_F32, shape, 5, vv->lip_segment);

    shape[0] = 1;
    shape[1] = 2;
    shape[2] = cfg->stft.n_fft / 2 + 1;
    shape[3] = nsample / cfg->stft.hop_length + 1;

    if (cfg->av_sep.use_rknpu) {
        vv->lip_embedding_fp16 = wtk_malloc(
            sizeof(uint16_t) * cfg->lip_embedding_dim * cfg->num_frames);
        vv->av_sep_input1_fp16 =
            wtk_malloc(sizeof(uint16_t) * shape[1] * shape[2] * shape[3]);

        vv->av_sep_input1 =
            qtk_nnrt_value_create_external(vv->av_sep, QTK_NNRT_VALUE_ELEM_F16,
                                           shape, 4, vv->av_sep_input1_fp16);
        vv->av_sep_input2 =
            qtk_nnrt_value_create_external(vv->av_sep, QTK_NNRT_VALUE_ELEM_F16,
                                           NULL, 0, vv->lip_embedding_fp16);
    } else {
        vv->av_sep_input1 = qtk_nnrt_value_create_external(
            vv->av_sep, QTK_NNRT_VALUE_ELEM_F32, shape, 4,
            vv->input_freq_channel_first);
    }

    return vv;
}

void qtk_avspeech_visual_voice_delete(qtk_avspeech_visual_voice_t *vv) {
    qtk_nnrt_value_release(vv->lip_embedding, vv->lip_embedding_input);
    qtk_nnrt_delete(vv->lip_embedding);
    qtk_stft_librosa_delete(vv->stft);
    wtk_free(vv->input_freq_channel_first);
    wtk_free(vv->audio_segment);
    wtk_free(vv->lip_segment);
    qtk_nnrt_value_release(vv->av_sep, vv->av_sep_input1);
    if (vv->cfg->av_sep.use_rknpu) {
        qtk_nnrt_value_release(vv->av_sep, vv->av_sep_input2);
        wtk_free(vv->lip_embedding_fp16);
        wtk_free(vv->av_sep_input1_fp16);
    }
    qtk_nnrt_delete(vv->av_sep);
    wtk_free(vv);
}

static void av_tracklet_meta_post_(qtk_avspeech_visual_voice_t *vv,
                                   qtk_nnrt_value_t out, wtk_complex_t *freq) {
    int i, k;
    // FIXME
    float scaler = 5;
    float *mask = qtk_nnrt_value_get_data(vv->av_sep, out);
    float *input_freq = vv->input_freq_channel_first;
    int nsample = vv->cfg->audio_segment_duration * 16000;
    int T = nsample / vv->cfg->stft.hop_length + 1;
    int nbin = vv->cfg->stft.n_fft / 2 + 1;
    int nbin_minus1 = nbin - 1;
    float reserve_factor = 0;
    for (i = 0; i < T; i++, freq += nbin) {
        for (k = 0; k < nbin_minus1; k++) {
            float a = mask[0 * (nbin_minus1 * T) + k * T + i];
            float b = mask[1 * (nbin_minus1 * T) + k * T + i];
            if (a < -1) {
                a = -1;
            }
            if (b < -1) {
                b = -1;
            }
            if (a > 1) {
                a = 1;
            }
            if (b > 1) {
                b = 1;
            }

            float c = input_freq[0 * (nbin * T) + k * T + i];
            float d = input_freq[1 * (nbin * T) + k * T + i];

            a *= scaler;
            b *= scaler;
            float real = a * c - b * d;
            float imag = a * d + b * c;
            freq[k].a = (1 - reserve_factor) * real + reserve_factor * c;
            freq[k].b = (1 - reserve_factor) * imag + reserve_factor * d;
        }
    }
}

int qtk_avspeech_visual_voice_feed(qtk_avspeech_visual_voice_t *vv,
                                   wtk_complex_t *feat,
                                   const uint8_t *lip_patch) {
    qtk_nnrt_value_t lip_embedding, audio_mask;
    int i, j;
    int64_t shape[5] = {1, 1, vv->cfg->num_frames, 88, 88};
    int nsample = vv->cfg->audio_segment_duration * 16000;
    float *data = vv->input_freq_channel_first;
    int T = nsample / vv->cfg->stft.hop_length + 1;
    int K = vv->cfg->stft.n_fft / 2 + 1;
    int t, k;

    for (t = 0; t < T; t++) {
        for (k = 0; k < K; k++) {
            data[0 * K * T + k * T + t] = feat[t * K + k].a;
            data[1 * K * T + k * T + t] = feat[t * K + k].b;
        }
    }

    if (vv->cfg->av_sep.use_rknpu) {
        for (t = 0; t < T; t++) {
            for (k = 0; k < K; k++) {
                vv->av_sep_input1_fp16[k * T * 2 + t * 2 + 0] =
                    float_to_half(feat[t * K + k].a);
                vv->av_sep_input1_fp16[k * T * 2 + t * 2 + 1] =
                    float_to_half(feat[t * K + k].b);
            }
        }
    }

    for (i = 0; i < shape[2] * shape[3] * shape[4]; i++) {
        vv->lip_segment[i] = (lip_patch[i] / 255.0 - 0.421) / 0.165;
    }
    qtk_nnrt_feed(vv->lip_embedding, vv->lip_embedding_input, 0);
    qtk_nnrt_run(vv->lip_embedding);
    qtk_nnrt_get_output(vv->lip_embedding, &lip_embedding, 0);

    qtk_nnrt_feed(vv->av_sep, vv->av_sep_input1, 0);
    if (vv->cfg->av_sep.use_rknpu) {
        float *embedding_float =
            qtk_nnrt_value_get_data(vv->lip_embedding, lip_embedding);
        for (i = 0; i < vv->cfg->lip_embedding_dim; i++) {
            for (j = 0; j < vv->cfg->num_frames; j++) {
                vv->lip_embedding_fp16[j * vv->cfg->lip_embedding_dim + i] =
                    float_to_half(*embedding_float++);
            }
        }
        qtk_nnrt_feed(vv->av_sep, vv->av_sep_input2, 1);
    } else {
        qtk_nnrt_feed(vv->av_sep, lip_embedding, 1);
    }

    qtk_nnrt_run(vv->av_sep);

    qtk_nnrt_get_output(vv->av_sep, &audio_mask, 0);

    av_tracklet_meta_post_(vv, audio_mask, feat);

    qtk_stft_librosa_reset(vv->stft);
    qtk_nnrt_value_release(vv->lip_embedding, lip_embedding);
    qtk_nnrt_value_release(vv->av_sep, audio_mask);
    qtk_nnrt_reset(vv->av_sep);
    qtk_nnrt_reset(vv->lip_embedding);

    return 0;
}

struct stft_forward_upval {
    wtk_complex_t *F;
    qtk_stft_librosa_t *lr;
};

struct stft_backward_upval {
    short *audio;
    qtk_stft_librosa_t *lr;
    float normalizer;
};

static int on_stft_forward_(struct stft_forward_upval *v, void *data,
                            qtk_stft_direction_t _) {
    int K = v->lr->cfg->n_fft / 2 + 1;
    memcpy(v->F, data, sizeof(wtk_complex_t) * K);
    v->F += K;
    return 0;
}

static int on_stft_backward_(struct stft_backward_upval *v, void *data,
                             qtk_stft_direction_t _) {
    qtk_stft_synthesis_sample_t *sample = data;
    int i;
    for (i = 0; i < sample->len; i++) {
        *v->audio++ = v->normalizer * sample->data[i];
    }
    return 0;
}

float qtk_avspeech_visual_voice_get_audio_feature(
    qtk_avspeech_visual_voice_t *vv, const short *in_audio, wtk_complex_t *F) {
    float normalizer;
    int nsample = vv->cfg->audio_segment_duration * 16000;
    int i;
    struct stft_forward_upval upval = {F, vv->stft};
    for (i = 0; i < nsample; i++) {
        vv->audio_segment[i] = in_audio[i] / (32768.0 * 2);
    }
    qtk_signal_normalize(vv->audio_segment, nsample, 0.07, &normalizer);
    normalizer *= 32768;
    qtk_stft_librosa_set_notifier(vv->stft, &upval,
                                  (qtk_stft_notifier_t)on_stft_forward_);
    qtk_stft_librosa_forward(vv->stft, vv->audio_segment, nsample);
    qtk_stft_librosa_forward_end(vv->stft);
    qtk_stft_librosa_reset(vv->stft);
    return normalizer;
}

int qtk_avspeech_visual_voice_synthesis_audio(qtk_avspeech_visual_voice_t *vv,
                                                short *out_audio,
                                                wtk_complex_t *freq,
                                                float nomralizer) {
    int nsample = vv->cfg->audio_segment_duration * 16000;
    int T = nsample / vv->cfg->stft.hop_length + 1;
    int K = vv->cfg->stft.n_fft / 2 + 1;
    int i;
    struct stft_backward_upval upval = {out_audio, vv->stft, nomralizer};
    qtk_stft_librosa_set_notifier(vv->stft, &upval,
                                  (qtk_stft_notifier_t)on_stft_backward_);
    for (i = 0; i < T; i++, freq += K) {
        qtk_stft_librosa_backward(vv->stft, freq);
    }
    qtk_stft_librosa_backward_end(vv->stft);
    qtk_stft_librosa_reset(vv->stft);
    return 0;
}
