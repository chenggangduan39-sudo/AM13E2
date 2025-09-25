#include "wtk/asr/whisper/qtk_whisper.h"
#include "qtk/linalg/qtk_gemm.h"
#include <float.h>

static int on_stft_(qtk_whisper_t *w, void *data,
                    qtk_stft_direction_t direction) {
    int nbin = w->cfg->stft.n_fft / 2 + 1;
    wtk_complex_t *cpx = data;
    if (w->feat_idx == w->cfg->T) {
        return 0;
    }
    for (int i = 0; i < nbin; i++) {
        w->magnitudes[i * w->cfg->T + w->feat_idx] =
            cpx[i].a * cpx[i].a + cpx[i].b * cpx[i].b;
    }
    w->feat_idx++;
    return 0;
}

qtk_whisper_t *qtk_whisper_new(qtk_whisper_cfg_t *cfg) {
    int nbin;
    qtk_whisper_t *w = malloc(sizeof(qtk_whisper_t));
    w->cfg = cfg;
    w->encoder_rt = qtk_nnrt_new(&cfg->encoder_rt);
    w->decoder_rt = qtk_nnrt_new(&cfg->decoder_rt);
    w->decoder_with_past_rt = qtk_nnrt_new(&cfg->decoder_with_past_rt);
    w->stft =
        qtk_stft_librosa_new(&cfg->stft, w, (qtk_stft_notifier_t)on_stft_);
    w->feat_blob = wtk_malloc(sizeof(float) * cfg->K * cfg->T);
    nbin = cfg->stft.n_fft / 2 + 1;
    w->magnitudes = wtk_malloc(sizeof(float) * cfg->T * nbin);
    w->audio_chunk = wtk_malloc(sizeof(float) * cfg->chunk_duration * 16000);

    {
        int64_t shape[3] = {1, cfg->K, cfg->T};
        w->input_features = qtk_nnrt_value_create_external(
            w->encoder_rt, QTK_NNRT_VALUE_ELEM_F32, shape, 3, w->feat_blob);
    }

    for (int i = 0; i < 6; i++) {
        int64_t shape[4] = {1, 8, 100, 64};
        w->cache_blob[i * 4 + 0] = wtk_malloc(sizeof(float) * 1 * 8 * 100 * 64);
        w->cache_blob[i * 4 + 1] = wtk_malloc(sizeof(float) * 1 * 8 * 100 * 64);
        w->cache_blob[i * 4 + 2] =
            wtk_malloc(sizeof(float) * 1 * 8 * 1000 * 64);
        w->cache_blob[i * 4 + 3] =
            wtk_malloc(sizeof(float) * 1 * 8 * 1000 * 64);
        w->cache_val[i * 4 + 0] = qtk_nnrt_value_create_external(
            w->decoder_with_past_rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4,
            w->cache_blob[i * 4 + 0]);
        w->cache_val[i * 4 + 1] = qtk_nnrt_value_create_external(
            w->decoder_with_past_rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4,
            w->cache_blob[i * 4 + 1]);
        shape[2] = 1000;
        w->cache_val[i * 4 + 2] = qtk_nnrt_value_create_external(
            w->decoder_with_past_rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4,
            w->cache_blob[i * 4 + 2]);
        w->cache_val[i * 4 + 3] = qtk_nnrt_value_create_external(
            w->decoder_with_past_rt, QTK_NNRT_VALUE_ELEM_F32, shape, 4,
            w->cache_blob[i * 4 + 3]);
    }

    w->result_buf = wtk_strbuf_new(1024, 1);

    return w;
}

void qtk_whisper_delete(qtk_whisper_t *w) {
    for (int i = 0; i < 24; i++) {
        wtk_free(w->cache_blob[i]);
        qtk_nnrt_value_release(w->decoder_with_past_rt, w->cache_val[i]);
    }
    qtk_nnrt_value_release(w->encoder_rt, w->input_features);
    qtk_nnrt_delete(w->encoder_rt);
    qtk_nnrt_delete(w->decoder_rt);
    qtk_nnrt_delete(w->decoder_with_past_rt);
    qtk_stft_librosa_delete(w->stft);
    wtk_free(w->feat_blob);
    wtk_free(w->magnitudes);
    wtk_strbuf_delete(w->result_buf);
    wtk_free(w->audio_chunk);
    wtk_free(w);
}

static int64_t detect_language_(qtk_whisper_t *w,
                                qtk_nnrt_value_t encoder_hidden_state) {
    int max_idx;
    qtk_nnrt_value_t logits;
    float *logits_data;
    int64_t decoder_input_ids[4] = {
        w->cfg->decoder_start_token_id, w->cfg->decoder_start_token_id,
        w->cfg->decoder_start_token_id, w->cfg->decoder_start_token_id};
    int64_t shape[2] = {1, 4};
    qtk_nnrt_value_t dii = qtk_nnrt_value_create_external(
        w->decoder_rt, QTK_NNRT_VALUE_ELEM_I64, shape, 2, decoder_input_ids);
    qtk_nnrt_feed(w->decoder_rt, dii, 0);
    qtk_nnrt_feed(w->decoder_rt, encoder_hidden_state, 1);
    qtk_nnrt_run(w->decoder_rt);
    qtk_nnrt_get_output(w->decoder_rt, &logits, 0);
    logits_data = qtk_nnrt_value_get_data(w->decoder_rt, logits);
    max_idx =
        wtk_float_argmax(logits_data + w->cfg->lang_start_token, w->cfg->nlang);
    qtk_nnrt_value_release(w->decoder_rt, dii);
    qtk_nnrt_value_release(w->decoder_rt, logits);
    return w->cfg->lang_start_token + max_idx;
}

static void logits_supress_(float *logits) {
    logits[50258] = FLT_MIN;
    logits[50358] = FLT_MIN;
    logits[50359] = FLT_MIN;
    logits[50360] = FLT_MIN;
    logits[50361] = FLT_MIN;
    logits[50362] = FLT_MIN;
}

static int append_hyp_(qtk_whisper_t *w, wtk_array_t *vocab, int id) {
    wtk_string_t *v = ((wtk_string_t **)vocab->slot)[id];
    wtk_strbuf_push(w->result_buf, v->data, v->len);
    return 0;
}

int qtk_whisper_transcribe(qtk_whisper_t *w, short *wav, int len,
                           wtk_string_t *result) {
    int feed_len;
    int idx;
    int chunk_len = w->cfg->chunk_duration * 16000;
    int64_t lang_token;
    qtk_nnrt_value_t last_hidden_state;
    feed_len = min(len, chunk_len);
    for (int i = 0; i < feed_len; i++) {
        w->audio_chunk[i] = wav[i] / 32768.0f;
    }
    if (feed_len < chunk_len) {
        memset(w->audio_chunk + feed_len, 0,
               sizeof(float) * (chunk_len - feed_len));
    }

    int nbin = w->cfg->stft.n_fft / 2 + 1;
    w->feat_idx = 0;
    qtk_stft_librosa_forward(w->stft, w->audio_chunk, chunk_len);
    qtk_stft_librosa_forward_end(w->stft);
    qtk_stft_librosa_reset(w->stft);

    qtk_linalg_sgemm(w->cfg->mel_filter, w->magnitudes, w->feat_blob, w->cfg->K,
                     nbin, w->cfg->T);
    float max_log = FLT_MIN;
    for (int i = 0; i < w->cfg->K * w->cfg->T; i++) {
        float x = ((float *)w->feat_blob)[i];
        x = x < 1e-10 ? log10f(1e-10) : log10f(x);
        ((float *)w->feat_blob)[i] = x;
        if (x > max_log) {
            max_log = x;
        }
    }
    for (int i = 0; i < w->cfg->K * w->cfg->T; i++) {
        float x = ((float *)w->feat_blob)[i];
        if (x < max_log - 8.0) {
            x = max_log - 8.0;
        }
        x = (x + 4.0) / 4.0;
        ((float *)w->feat_blob)[i] = x;
    }

    wtk_strbuf_reset(w->result_buf);
    qtk_nnrt_feed(w->encoder_rt, w->input_features, 0);
    qtk_nnrt_run(w->encoder_rt);
    qtk_nnrt_get_output(w->encoder_rt, &last_hidden_state, 0);

    lang_token = detect_language_(w, last_hidden_state);
    wtk_array_t *vocab =
        lang_token == w->cfg->en_lang_id ? w->cfg->vocab_en : w->cfg->vocab;
    int64_t shape[2] = {1, 4};
    int64_t input_ids[4] = {w->cfg->decoder_start_token_id, lang_token,
                            w->cfg->task_transcribe_id,
                            w->cfg->no_timestamps_token_id};
    qtk_nnrt_value_t dii = qtk_nnrt_value_create_external(
        w->decoder_rt, QTK_NNRT_VALUE_ELEM_I64, shape, 2, input_ids);
    qtk_nnrt_feed(w->decoder_rt, dii, 0);
    qtk_nnrt_feed(w->decoder_rt, last_hidden_state, 1);
    qtk_nnrt_run(w->decoder_rt);

    int64_t best_hyp;
    qtk_nnrt_value_t logits;
    float *logits_data;
    qtk_nnrt_get_output(w->decoder_rt, &logits, 0);
    logits_data = qtk_nnrt_value_get_data(w->decoder_rt, logits);
    logits_data += 3 * w->cfg->vocab->nslot;
    logits_data[220] = logits_data[50257] = FLT_MIN;
    logits_supress_(logits_data);
    best_hyp = wtk_float_argmax(logits_data, w->cfg->vocab->nslot);
    append_hyp_(w, vocab, best_hyp);
    qtk_nnrt_value_release(w->decoder_rt, logits);
    qtk_nnrt_value_release(w->decoder_rt, dii);

    for (int i = 0; i < 6; i++) {
        qtk_nnrt_value_t val1, val2, val3, val4;
        float *data1, *data2, *data3, *data4;
        qtk_nnrt_get_output(w->decoder_rt, &val1, 1 + 4 * i + 0);
        qtk_nnrt_get_output(w->decoder_rt, &val2, 1 + 4 * i + 1);
        qtk_nnrt_get_output(w->decoder_rt, &val3, 1 + 4 * i + 2);
        qtk_nnrt_get_output(w->decoder_rt, &val4, 1 + 4 * i + 3);

        data1 = qtk_nnrt_value_get_data(w->decoder_rt, val1);
        data2 = qtk_nnrt_value_get_data(w->decoder_rt, val2);
        data3 = qtk_nnrt_value_get_data(w->decoder_rt, val3);
        data4 = qtk_nnrt_value_get_data(w->decoder_rt, val4);

        for (int j = 0; j < 8; j++) {
            memcpy((float *)(w->cache_blob[4 * i + 0]) + j * 100 * 64,
                   data1 + j * 4 * 64, sizeof(float) * 4 * 64);
            memcpy((float *)(w->cache_blob[4 * i + 1]) + j * 100 * 64,
                   data2 + j * 4 * 64, sizeof(float) * 4 * 64);
        }

        memcpy(w->cache_blob[4 * i + 2], data3,
               sizeof(float) * 1 * 8 * 1000 * 64);
        memcpy(w->cache_blob[4 * i + 3], data4,
               sizeof(float) * 1 * 8 * 1000 * 64);

        qtk_nnrt_value_release(w->decoder_rt, val1);
        qtk_nnrt_value_release(w->decoder_rt, val2);
        qtk_nnrt_value_release(w->decoder_rt, val3);
        qtk_nnrt_value_release(w->decoder_rt, val4);
    }

    int64_t cache_postion;
    int64_t decoder_attension_mask[101];
    qtk_nnrt_value_t best_hyp_val, decoder_attension_mask_val,
        cache_postion_val;
    shape[1] = 1;
    best_hyp_val = qtk_nnrt_value_create_external(
        w->decoder_with_past_rt, QTK_NNRT_VALUE_ELEM_I64, shape, 2, &best_hyp);
    cache_postion_val = qtk_nnrt_value_create_external(
        w->decoder_with_past_rt, QTK_NNRT_VALUE_ELEM_I64, shape, 1,
        &cache_postion);
    shape[1] = 101;
    decoder_attension_mask_val = qtk_nnrt_value_create_external(
        w->decoder_with_past_rt, QTK_NNRT_VALUE_ELEM_I64, shape, 2,
        decoder_attension_mask);
    for (idx = 1; idx < w->cfg->max_decode_len; idx++) {
        memset(decoder_attension_mask, 0, sizeof(decoder_attension_mask));
        decoder_attension_mask[0] = 1;
        decoder_attension_mask[1] = 1;
        decoder_attension_mask[2] = 1;
        decoder_attension_mask[3] = 1;
        for (int i = 0; i < idx; i++) {
            decoder_attension_mask[100 - i] = 1;
        }
        cache_postion = idx + 3;
        qtk_nnrt_feed(w->decoder_with_past_rt, best_hyp_val, 0);
        qtk_nnrt_feed(w->decoder_with_past_rt, decoder_attension_mask_val, 1);
        for (int i = 0; i < 24; i++) {
            qtk_nnrt_feed(w->decoder_with_past_rt, w->cache_val[i], 2 + i);
        }
        qtk_nnrt_feed(w->decoder_with_past_rt, cache_postion_val, 2 + 24);
        qtk_nnrt_run(w->decoder_with_past_rt);
        qtk_nnrt_get_output(w->decoder_with_past_rt, &logits, 0);
        logits_data = qtk_nnrt_value_get_data(w->decoder_with_past_rt, logits);
        logits_supress_(logits_data);
        best_hyp = wtk_float_argmax(logits_data, w->cfg->vocab->nslot);
        qtk_nnrt_value_release(w->decoder_with_past_rt, logits);
        if (best_hyp == w->cfg->eos_id) {
            break;
        }
        append_hyp_(w, vocab, best_hyp);
        for (int i = 0; i < 6; i++) {
            qtk_nnrt_value_t decoder_key_out, decoder_val_out;
            qtk_nnrt_get_output(w->decoder_with_past_rt, &decoder_key_out,
                                1 + i * 2 + 0);
            qtk_nnrt_get_output(w->decoder_with_past_rt, &decoder_val_out,
                                1 + i * 2 + 1);
            float *decoder_key_out_data = qtk_nnrt_value_get_data(
                w->decoder_with_past_rt, decoder_key_out);
            float *decoder_val_out_data = qtk_nnrt_value_get_data(
                w->decoder_with_past_rt, decoder_val_out);
            float *decoder_key_in_data = w->cache_blob[i * 4 + 0];
            float *decoder_val_in_data = w->cache_blob[i * 4 + 1];
            int N1 = 100 - idx;
            for (int j = 0; j < 8; j++) {
                memcpy(decoder_key_in_data + j * 100 * 64,
                       decoder_key_out_data + j * 101 * 64,
                       N1 * 64 * sizeof(float));
                memcpy(decoder_key_in_data + j * 100 * 64 + N1 * 64,
                       decoder_key_out_data + j * 101 * 64 + (101 - idx) * 64,
                       idx * 64 * sizeof(float));
                memcpy(decoder_val_in_data + j * 100 * 64,
                       decoder_val_out_data + j * 101 * 64,
                       N1 * 64 * sizeof(float));
                memcpy(decoder_val_in_data + j * 100 * 64 + N1 * 64,
                       decoder_val_out_data + j * 101 * 64 + (101 - idx) * 64,
                       idx * 64 * sizeof(float));
            }
            qtk_nnrt_value_release(w->decoder_with_past_rt, decoder_key_out);
            qtk_nnrt_value_release(w->decoder_with_past_rt, decoder_val_out);
        }
    }

    qtk_nnrt_value_release(w->decoder_with_past_rt, best_hyp_val);
    qtk_nnrt_value_release(w->decoder_with_past_rt, decoder_attension_mask_val);
    qtk_nnrt_value_release(w->decoder_with_past_rt, cache_postion_val);
    qtk_nnrt_value_release(w->encoder_rt, last_hidden_state);

    result->data = w->result_buf->data;
    result->len = w->result_buf->pos;
    return 0;
}
