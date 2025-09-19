#include "qtk/avspeech/qtk_avspeech_separator_cfg.h"

int qtk_avspeech_separator_cfg_init(qtk_avspeech_separator_cfg_t *cfg) {
    qtk_avspeech_lip_cfg_init(&cfg->lip);
    qtk_nnrt_cfg_init(&cfg->dnsmos);
    wtk_qform9_cfg_init(&cfg->qform9);
    wtk_agc_cfg_init(&cfg->agc);
    wtk_cmask_pse_cfg_init(&cfg->pse);
    qtk_avspeech_visual_voice_cfg_init(&cfg->visual_voice);
    cfg->sampling_rate = 16000;
    cfg->height = 480;
    cfg->width = 640;
    cfg->sliding_factor = 4;
    cfg->use_mpp = 0;
    cfg->image_max_free = 12;
    cfg->lip_result_max_free = 64;
    cfg->tracklet_max_free = 16;
    cfg->use_vad = 0;
    cfg->use_qform9 = 0;
    cfg->use_post = 0;
    cfg->thresh_low_conf = -15;
    cfg->thresh_high_conf = -10;
    cfg->mid_conf_reserve_factor = 0.8;
    cfg->reback_thresh = -15;
    cfg->sync_audio = 0;
    cfg->use_agc = 0;
    cfg->use_vpdenoise = 0;
    cfg->dnsmos_nsample = 144160;
    cfg->vp_extractor_vad_fn = NULL;
    cfg->save_video_path = "/sdcard/Android/data/com.qdreamer.hvs/files/output.h264";
    cfg->vp_extractor_duration_min_s = 3;
    cfg->vp_dnsmos_thresh = 2.6;
    return 0;
}

int qtk_avspeech_separator_cfg_clean(qtk_avspeech_separator_cfg_t *cfg) {
    qtk_avspeech_lip_cfg_clean(&cfg->lip);
    qtk_nnrt_cfg_clean(&cfg->dnsmos);
    wtk_qform9_cfg_clean(&cfg->qform9);
    wtk_agc_cfg_clean(&cfg->agc);
    wtk_cmask_pse_cfg_clean(&cfg->pse);
    qtk_avspeech_visual_voice_cfg_clean(&cfg->visual_voice);
    return 0;
}

static int local_update_(qtk_avspeech_separator_cfg_t *cfg) {
    int i;
    cfg->patch_bytes = sizeof(uint8_t) * 1 * cfg->lip.H * cfg->lip.W;
    cfg->audio_frame_bytes =
        sizeof(short) * (cfg->sampling_rate / cfg->visual_voice.fps);
    cfg->video_segment_bytes = cfg->patch_bytes * cfg->visual_voice.num_frames;
    if (cfg->visual_voice.audio_segment_duration > 0) {
        cfg->audio_segment_bytes = cfg->visual_voice.audio_segment_duration *
                                   cfg->sampling_rate * sizeof(short);
    } else {
        cfg->audio_segment_bytes =
            cfg->audio_frame_bytes * cfg->visual_voice.num_frames;
    }
    for (i = 0; i < 3; i++) {
        cfg->mean[i] *= 255;
        cfg->std[i] *= 255;
    }
    if (cfg->use_vad && cfg->sync_audio) {
        wtk_debug("Not Impl\n");
        return -1;
    }
    return 0;
}

int qtk_avspeech_separator_cfg_update(qtk_avspeech_separator_cfg_t *cfg) {
    qtk_avspeech_lip_cfg_update(&cfg->lip);
    qtk_nnrt_cfg_update(&cfg->dnsmos);
    wtk_qform9_cfg_update(&cfg->qform9);
    wtk_agc_cfg_update(&cfg->agc);
    wtk_cmask_pse_cfg_update(&cfg->pse);
    qtk_avspeech_visual_voice_cfg_update(&cfg->visual_voice);
    if (cfg->visual_voice.num_frames % cfg->sliding_factor != 0 ||
        (cfg->audio_segment_bytes / sizeof(short)) % cfg->sliding_factor != 0) {
        wtk_debug("wrong sliding factor %d\n", cfg->sliding_factor);
        return -1;
    }
    return local_update_(cfg);
}

int qtk_avspeech_separator_cfg_update_local(qtk_avspeech_separator_cfg_t *cfg,
                                            wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_array_t *a;
    wtk_local_cfg_t *sub;
    sub = wtk_local_cfg_find_lc_s(lc, "lip");
    if (sub) {
        qtk_avspeech_lip_cfg_update_local(&cfg->lip, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "visual_voice");
    if (sub) {
        qtk_avspeech_visual_voice_cfg_update_local(&cfg->visual_voice, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "dnsmos");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->dnsmos, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "qform9");
    if (sub) {
        wtk_qform9_cfg_update_local(&cfg->qform9, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "agc");
    if (sub) {
        wtk_agc_cfg_update_local(&cfg->agc, sub);
    }
    sub = wtk_local_cfg_find_lc_s(lc, "pse");
    if (sub) {
        wtk_cmask_pse_cfg_update_local(&cfg->pse, sub);
    }
    if ((a = wtk_local_cfg_find_float_array_s(lc, "mean"))) {
        cfg->mean = a->slot;
    }
    if ((a = wtk_local_cfg_find_float_array_s(lc, "std"))) {
        cfg->std = a->slot;
    }

    wtk_local_cfg_update_cfg_i(lc, cfg, height, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, width, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sampling_rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sliding_factor, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_mpp, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_post, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, sync_audio, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qform9, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vpdenoise, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, dnsmos_nsample, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, thresh_low_conf, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, thresh_high_conf, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, mid_conf_reserve_factor, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, reback_thresh, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, image_max_free, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, lip_result_max_free, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, tracklet_max_free, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, vad_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, save_video_path, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, vp_extractor_vad_fn, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, vp_extractor_duration_min_s, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, vp_dnsmos_thresh, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_agc, v);
    return 0;
}

int qtk_avspeech_separator_cfg_update2(qtk_avspeech_separator_cfg_t *cfg,
                                       wtk_source_loader_t *sl) {
    qtk_avspeech_lip_cfg_update2(&cfg->lip, sl);
    wtk_qform9_cfg_update2(&cfg->qform9, sl);
    wtk_agc_cfg_update2(&cfg->agc, sl);
    wtk_cmask_pse_cfg_update2(&cfg->pse, sl);
    qtk_avspeech_visual_voice_cfg_update2(&cfg->visual_voice, sl);

    return local_update_(cfg);
}
