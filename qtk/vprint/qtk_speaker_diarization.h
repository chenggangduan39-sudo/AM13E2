#ifndef G_57SHFQDW_WED1_NZDM_PNLT_FMBS0PT9A5DS
#define G_57SHFQDW_WED1_NZDM_PNLT_FMBS0PT9A5DS
#pragma once
#include "qtk/core/qtk_slidingarray.h"
#include "qtk/sci/clustering/qtk_clustering_spectral.h"
#include "qtk/vprint/qtk_speaker_diarization_cfg.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t start_ms;
    uint32_t end_ms;
} qtk_speaker_diarization_segment_t;

typedef struct qtk_speaker_diarization qtk_speaker_diarization_t;

struct qtk_speaker_diarization {
    wtk_nnet3_xvector_compute_t *xvector;
    wtk_kvad_t *kvad;
    qtk_speaker_diarization_cfg_t *cfg;
    wtk_kxparm_t *kxparm;
    qtk_clustering_spectral_t *spec;
    qtk_slidingarray_t *feat;
    wtk_strbuf_t *feat_mat;
    wtk_strbuf_t *segments;
    int cur_chunk;
    int chunk_idx;
    uint32_t vad_frame_idx;
    uint32_t vad_start_frame_idx;
    unsigned sil : 1;
};

qtk_speaker_diarization_t *
qtk_speaker_diarization_new(qtk_speaker_diarization_cfg_t *cfg);
void qtk_speaker_diarization_delete(qtk_speaker_diarization_t *sd);
int qtk_speaker_diarization_feed(qtk_speaker_diarization_t *sd, short *audio,
                                 int len);
int qtk_speaker_diarization_feed_end(qtk_speaker_diarization_t *sd,
                                     qtk_speaker_diarization_segment_t **seg,
                                     int **labels, int *N);
int qtk_speaker_diarization_reset(qtk_speaker_diarization_t *sd);

#ifdef __cplusplus
};
#endif
#endif /* G_57SHFQDW_WED1_NZDM_PNLT_FMBS0PT9A5DS */
