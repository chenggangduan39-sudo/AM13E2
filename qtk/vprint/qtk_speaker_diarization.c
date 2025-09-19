#include "qtk/vprint/qtk_speaker_diarization.h"
#include "wtk/asr/vad/wtk_vframe.h"

static void vad_on_speech_start_(qtk_speaker_diarization_t *sd) {
    sd->vad_start_frame_idx = sd->vad_frame_idx - 1;
    wtk_kxparm_start(sd->kxparm);
}

static void vad_on_speech_(qtk_speaker_diarization_t *sd, wtk_vframe_t *vf) {
    wtk_kxparm_feed(sd->kxparm, vf->wav_data, vf->frame_step, 0);
}

static void add_segment_(qtk_speaker_diarization_t *sd) {
    qtk_speaker_diarization_segment_t seg;
    wtk_vecf_t *emb;
    wtk_nnet3_xvector_feed_feat(sd->xvector, (float *)sd->feat_mat->data,
                                sd->chunk_idx + 1, sd->cfg->feat_dim);
    emb = wtk_nnet3_xvector_compute_normalize(sd->xvector);
    qtk_clustering_spectral_add(sd->spec, emb->p);
    seg.start_ms =
        sd->vad_start_frame_idx * sd->cfg->vad_frame_ms +
        sd->cur_chunk * sd->cfg->feat_frame_ms * sd->cfg->sliding_step_frames;
    seg.end_ms = seg.start_ms + (sd->chunk_idx + 1) * sd->cfg->feat_frame_ms;
    wtk_strbuf_push(sd->segments, (const char *)&seg, sizeof(seg));
}

static void vad_on_speech_end_(qtk_speaker_diarization_t *sd) {
    wtk_kxparm_feed(sd->kxparm, NULL, 0, 1);
    wtk_kxparm_reset(sd->kxparm);

    if (sd->chunk_idx > 10) {
        add_segment_(sd);
    }

    qtk_slidingarray_reset(sd->feat);
    wtk_strbuf_reset(sd->feat_mat);
    wtk_nnet3_xvector_compute_reset(sd->xvector);
    sd->cur_chunk = 0;
    sd->chunk_idx = 0;
}

static int vad_postprocess_(qtk_speaker_diarization_t *sd) {
    wtk_queue_node_t *n, *next;
    wtk_vframe_t *vf;
    for (n = sd->kvad->output_q.pop; n; n = next) {
        vf = data_offset2(n, wtk_vframe_t, q_n);
        next = n->next;
        if (sd->sil) {
            if (vf->state == wtk_vframe_speech) {
                vad_on_speech_start_(sd);
                vad_on_speech_(sd, vf);
                sd->sil = 0;
            }
        } else {
            if (vf->state == wtk_vframe_speech) {
                vad_on_speech_(sd, vf);
            } else {
                vad_on_speech_end_(sd);
                sd->sil = 1;
            }
        }
        sd->vad_frame_idx++;
        wtk_kvad_push_frame(sd->kvad, vf);
    }
    wtk_queue_init(&sd->kvad->output_q);
    return 0;
}

static void on_kxparm_(qtk_speaker_diarization_t *sd, wtk_kfeat_t *feat) {
    qtk_slidingarray_push(sd->feat, feat->v);
}

static int on_feat_(qtk_speaker_diarization_t *sd, int idx0, int idx1,
                    void *elem) {
    if (idx0 != sd->cur_chunk) {
        if (sd->chunk_idx > 10) {
            add_segment_(sd);
        }
        wtk_strbuf_reset(sd->feat_mat);
        wtk_nnet3_xvector_compute_reset(sd->xvector);
        sd->cur_chunk = idx0;
    }
    sd->chunk_idx = idx1;
    wtk_strbuf_push(sd->feat_mat, elem, sd->feat->elem_sz);
    return 0;
}

static int merge_adjacent_labels_(qtk_speaker_diarization_t *sd,
                                  qtk_speaker_diarization_segment_t *segments,
                                  int *labels, int N) {
    qtk_speaker_diarization_segment_t seg;
    int *result_lab = wtk_malloc(sizeof(int) * N);
    wtk_strbuf_t *result_seg =
        wtk_strbuf_new(sizeof(qtk_speaker_diarization_segment_t) * N * 0.75, 1);
    int result_lab_idx = 0;
    int pre_label;
    seg.start_ms = segments[0].start_ms;
    seg.end_ms = segments[0].end_ms;
    pre_label = labels[0];
    uint32_t middle;
    for (int i = 1; i < N; i++) {
        if (labels[i] == pre_label) {
            if (seg.end_ms > segments[i].start_ms ||
                segments[i].start_ms - seg.end_ms < 20) {
                seg.end_ms = segments[i].end_ms;
            } else {
                wtk_strbuf_push(result_seg, (const char *)&seg, sizeof(seg));
                seg = segments[i];
                result_lab[result_lab_idx++] = pre_label;
                pre_label = labels[i];
            }
        } else {
            if (seg.end_ms > segments[i].start_ms) {
                middle = seg.end_ms - (seg.end_ms - segments[i].start_ms) / 2;
                seg.end_ms = middle;
                wtk_strbuf_push(result_seg, (const char *)&seg, sizeof(seg));
                result_lab[result_lab_idx++] = pre_label;
                pre_label = labels[i];
                seg = segments[i];
                seg.start_ms = middle;
            } else {
                wtk_strbuf_push(result_seg, (const char *)&seg, sizeof(seg));
                result_lab[result_lab_idx++] = pre_label;
                seg = segments[i];
                pre_label = labels[i];
            }
        }
    }
    wtk_strbuf_push(result_seg, (const char *)&seg, sizeof(seg));
    result_lab[result_lab_idx++] = pre_label;
    memcpy(segments, result_seg->data,
           sizeof(qtk_speaker_diarization_segment_t) * result_lab_idx);
    memcpy(labels, result_lab, sizeof(int) * result_lab_idx);
    wtk_free(result_lab);
    wtk_strbuf_delete(result_seg);
    return result_lab_idx;
}

qtk_speaker_diarization_t *
qtk_speaker_diarization_new(qtk_speaker_diarization_cfg_t *cfg) {
    qtk_speaker_diarization_t *sd =
        wtk_malloc(sizeof(qtk_speaker_diarization_t));
    sd->cfg = cfg;
    sd->xvector = wtk_nnet3_xvector_compute_new2(&cfg->xvector);
    sd->kvad = wtk_kvad_new(&cfg->kvad);
    sd->spec = qtk_clustering_spectral_new(-1, cfg->emb_len, -1);
    sd->spec->pval = cfg->spec_pval;
    sd->kxparm = wtk_kxparm_new(&cfg->kxparm);
    sd->feat =
        qtk_slidingarray_new(cfg->sliding_win_frames, cfg->sliding_step_frames,
                             sizeof(float) * cfg->feat_dim);
    sd->sil = 1;
    sd->cur_chunk = 0;
    sd->chunk_idx = 0;
    sd->vad_frame_idx = 0;
    sd->feat_mat = wtk_strbuf_new(
        sizeof(float) * cfg->sliding_win_frames * cfg->feat_dim, 1);
    sd->segments =
        wtk_strbuf_new(sizeof(qtk_speaker_diarization_segment_t) * 1024, 1);

    wtk_kxparm_set_notify(sd->kxparm, sd, (wtk_kxparm_notify_f)on_kxparm_);
    qtk_slidingarray_set_handler(sd->feat, (qtk_slidingarray_handler_t)on_feat_,
                                 sd);

    wtk_kvad_start(sd->kvad);
    return sd;
}

void qtk_speaker_diarization_delete(qtk_speaker_diarization_t *sd) {
    wtk_nnet3_xvector_compute_delete(sd->xvector);
    wtk_kvad_delete(sd->kvad);
    qtk_clustering_spectral_delete(sd->spec);
    wtk_kxparm_delete(sd->kxparm);
    qtk_slidingarray_delete(sd->feat);
    wtk_strbuf_delete(sd->feat_mat);
    wtk_strbuf_delete(sd->segments);
    wtk_free(sd);
}

int qtk_speaker_diarization_feed(qtk_speaker_diarization_t *sd, short *audio,
                                 int len) {
    wtk_kvad_feed(sd->kvad, audio, len, 0);
    return vad_postprocess_(sd);
}

int qtk_speaker_diarization_feed_end(qtk_speaker_diarization_t *sd,
                                     qtk_speaker_diarization_segment_t **seg,
                                     int **labels, int *N) {
    int N_;
    wtk_kvad_feed(sd->kvad, NULL, 0, 1);
    vad_postprocess_(sd);
    int *labels_ = qtk_clustering_spectral_get_result(sd->spec);
    N_ = merge_adjacent_labels_(
        sd, (qtk_speaker_diarization_segment_t *)sd->segments->data, labels_,
        sd->spec->N);
    *seg = (qtk_speaker_diarization_segment_t *)sd->segments->data;
    *labels = labels_;
    *N = N_;
    return 0;
}

int qtk_speaker_diarization_reset(qtk_speaker_diarization_t *sd) {
    wtk_nnet3_xvector_compute_reset(sd->xvector);
    wtk_kvad_reset(sd->kvad);
    wtk_kvad_start(sd->kvad);
    wtk_kxparm_reset(sd->kxparm);
    qtk_clustering_spectral_reset(sd->spec);
    qtk_slidingarray_reset(sd->feat);
    wtk_strbuf_reset(sd->feat_mat);
    wtk_strbuf_reset(sd->segments);
    sd->cur_chunk = 0;
    sd->chunk_idx = 0;
    sd->vad_frame_idx = 0;
    sd->vad_start_frame_idx = 0;
    sd->sil = 1;
    return 0;
}