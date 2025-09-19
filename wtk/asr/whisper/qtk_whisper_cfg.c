#include "wtk/asr/whisper/qtk_whisper_cfg.h"
#include "qtk/serde/qtk_serde_np.h"
#include "wtk/core/json/wtk_json_parse.h"
#include "wtk/core/wtk_base64.h"

int qtk_whisper_cfg_init(qtk_whisper_cfg_t *cfg) {
    qtk_nnrt_cfg_init(&cfg->encoder_rt);
    qtk_nnrt_cfg_init(&cfg->decoder_rt);
    qtk_nnrt_cfg_init(&cfg->decoder_with_past_rt);
    qtk_stft_librosa_cfg_init(&cfg->stft);
    cfg->chunk_duration = 20;
    cfg->lang_start_token = 50259;
    cfg->decoder_start_token_id = 50258;
    cfg->task_transcribe_id = 50359;
    cfg->no_timestamps_token_id = 50363;
    cfg->mel_filter_fn = NULL;
    cfg->eos_id = -1;
    cfg->en_lang_id = -1;
    cfg->token2lang_fn = NULL;
    cfg->vocab_fn = NULL;
    cfg->vocab_en_fn = NULL;
    cfg->lang = NULL;
    cfg->max_decode_len = 50;
    cfg->K = 80;
    cfg->heap = wtk_heap_new(1024);
    cfg->vocab = wtk_array_new_h(cfg->heap, 10000, sizeof(wtk_string_t *));
    cfg->vocab_en = wtk_array_new_h(cfg->heap, 10000, sizeof(wtk_string_t *));
    cfg->mel_filter = NULL;
    return 0;
}

int qtk_whisper_cfg_clean(qtk_whisper_cfg_t *cfg) {
    qtk_nnrt_cfg_clean(&cfg->encoder_rt);
    qtk_nnrt_cfg_clean(&cfg->decoder_rt);
    qtk_nnrt_cfg_clean(&cfg->decoder_with_past_rt);
    qtk_stft_librosa_cfg_clean(&cfg->stft);
    if (cfg->lang) {
        for (int i = 0; i < cfg->nlang; i++) {
            wtk_string_delete(cfg->lang[i]);
        }
        wtk_free(cfg->lang);
    }
    wtk_heap_delete(cfg->heap);
    wtk_free(cfg->mel_filter);
    return 0;
}

static int read_vocab_(qtk_whisper_cfg_t *cfg, wtk_array_t *vocab,
                       const char *fn, int base64, wtk_strbuf_t *buf) {
    wtk_source_t s;
    wtk_source_init_file(&s, (char *)fn);
    wtk_strbuf_t *tmp_buf = wtk_strbuf_new(128, 1);
    int id = 0;
    while (1) {
        wtk_source_read_normal_string(&s, buf);
        if (buf->pos == 0) {
            break;
        }
        wtk_string_t **vocab_txt = wtk_array_push(vocab);
        wtk_source_read_normal_string(&s, buf);
        if (base64 && buf->data[0] != '<' && buf->data[0] != '=') {
            wtk_strbuf_push_c(buf, '\0');
            char *sdata = wtk_base64_decode(buf->data, buf->pos - 1);
            *vocab_txt = wtk_heap_dup_string(cfg->heap, sdata, strlen(sdata));
            wtk_free(sdata);
        } else {
            wtk_strbuf_replace(tmp_buf, buf->data, buf->pos, "\u0120", 2, " ",
                               1);
            *vocab_txt =
                wtk_heap_dup_string(cfg->heap, tmp_buf->data, tmp_buf->pos);
        }
        if (cfg->eos_id < 0 &&
            wtk_string_equal_s(*vocab_txt, "<|endoftext|>")) {
            cfg->eos_id = id;
        }
        if (cfg->en_lang_id < 0 && wtk_string_equal_s(*vocab_txt, "<|en|>")) {
            cfg->en_lang_id = id;
        }
        id++;
    }
    wtk_source_clean_file(&s);
    wtk_strbuf_delete(tmp_buf);
    return 0;
}

static int file_read_(FILE *f, char *buf, int l) { return fread(buf, 1, l, f); }

static int local_update_(qtk_whisper_cfg_t *cfg) {
    wtk_queue_node_t *node;
    wtk_json_parser_t *jp = wtk_json_parser_new();
    wtk_strbuf_t *buf = wtk_strbuf_new(128, 1);
    qtk_serde_np_t np;
    cfg->T = (cfg->chunk_duration * 16000) / cfg->stft.hop_length;
    wtk_json_parser_parse_file(jp, cfg->token2lang_fn);
    if (jp->json->main->type != WTK_JSON_OBJECT) {
        wtk_debug(">>\n");
        goto err;
    }
    cfg->nlang = jp->json->main->v.object->length;
    cfg->lang = wtk_malloc(cfg->nlang * sizeof(cfg->lang[0]));
    for (node = jp->json->main->v.object->pop; node; node = node->next) {
        wtk_json_obj_item_t *item =
            data_offset2(node, wtk_json_obj_item_t, q_n);
        int k = wtk_str_atoi(item->k.data, item->k.len);
        int idx = k - cfg->lang_start_token;
        if (idx < 0 || idx >= cfg->nlang ||
            item->item->type != WTK_JSON_STRING) {
            goto err;
        }
        cfg->lang[idx] = wtk_string_dup_data(item->item->v.str->data,
                                             item->item->v.str->len);
        if (cfg->lang[idx]->len == 2 && cfg->lang[idx]->data[0] == 'z' &&
            cfg->lang[idx]->data[1] == 'h') {
            cfg->zh_lang_id = k;
        }
    }
    wtk_json_parser_reset(jp);
    wtk_json_parser_delete(jp);

    read_vocab_(cfg, cfg->vocab, cfg->vocab_fn, 1, buf);
    read_vocab_(cfg, cfg->vocab_en, cfg->vocab_en_fn, 0, buf);

    wtk_strbuf_delete(buf);

    qtk_serde_np_init(&np);
    FILE *f = fopen(cfg->mel_filter_fn, "rb");
    cfg->mel_filter = qtk_serde_np_load(&np, (qtk_io_reader)file_read_, f);
    fclose(f);
    qtk_serde_np_clean(&np);
    return 0;
err:
    wtk_debug("!!!!!!!!!!!! whisper cfg update failed\n");
    return -1;
}

int qtk_whisper_cfg_update(qtk_whisper_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->encoder_rt);
    qtk_nnrt_cfg_update(&cfg->decoder_rt);
    qtk_nnrt_cfg_update(&cfg->decoder_with_past_rt);
    qtk_stft_librosa_cfg_update(&cfg->stft);
    local_update_(cfg);
    return 0;
}

int qtk_whisper_cfg_update_local(qtk_whisper_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_t *sub;
    sub = wtk_local_cfg_find_lc_s(lc, "encoder_rt");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->encoder_rt, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "decoder_rt");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->decoder_rt, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "decoder_with_past_rt");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->decoder_with_past_rt, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "stft");
    if (sub) {
        qtk_stft_librosa_cfg_update_local(&cfg->stft, sub);
    }
    wtk_local_cfg_update_cfg_f(lc, cfg, chunk_duration, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, lang_start_token, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, decoder_start_token_id, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, task_transcribe_id, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, no_timestamps_token_id, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, max_decode_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, K, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, token2lang_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, vocab_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, vocab_en_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, mel_filter_fn, v);
    return 0;
}

int qtk_whisper_cfg_update2(qtk_whisper_cfg_t *cfg, wtk_source_loader_t *sl) {
    qtk_nnrt_cfg_update2(&cfg->encoder_rt, sl);
    qtk_nnrt_cfg_update2(&cfg->decoder_rt, sl);
    qtk_nnrt_cfg_update2(&cfg->decoder_with_past_rt, sl);
    qtk_stft_librosa_cfg_update2(&cfg->stft, sl);
    local_update_(cfg);
    return 0;
}
