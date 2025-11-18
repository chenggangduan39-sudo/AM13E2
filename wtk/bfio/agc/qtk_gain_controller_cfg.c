#include "qtk_gain_controller_cfg.h"
int qtk_ahs_gain_controller_cfg_init(qtk_ahs_gain_controller_cfg_t *cfg){
    cfg->hop_size = 128;
    cfg->sample_rate = 16000;
    cfg->use_evad = 0;
    cfg->use_update_gali = 1;

    cfg->update_thresh = 1e-5f;
    cfg->alpha = 0.95;
    cfg->pworg = 0.5;
    cfg->pvorg = 0.05;
    cfg->beta = 0.9;

    cfg->Maximun_Gain = 0.0;
    cfg->Maximun_Atten = -30.0;

    cfg->static_size = 200;
    cfg->pvad_threshold = 0.8;
    cfg->low_energy_threshold = 1e-2;
    cfg->high_energy_threshold = 10;
    cfg->vad_delay = 40;
    cfg->vad_init_frame = 105;

    cfg->voice_init_cnt = 4;
    cfg->voice_frame = 40;
    cfg->voice_precent = 0.8;
    cfg->voice_alpha = 0.2;
    cfg->voice_alpha2 = 0.6;
    cfg->voice_prob = 0.7;
    cfg->voice_thresh = 10;
    cfg->use_voice_prob = 0;

    return 0;
}

int qtk_ahs_gain_controller_cfg_clean(qtk_ahs_gain_controller_cfg_t *cfg){
    return 0;
}

int qtk_ahs_gain_controller_cfg_update_local(qtk_ahs_gain_controller_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, hop_size, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sample_rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pworg, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pvorg, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, beta, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, update_thresh, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, Maximun_Gain, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, Maximun_Atten, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_evad, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_update_gali, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, pvad_threshold, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, low_energy_threshold, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, high_energy_threshold, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, static_size, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, vad_delay, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, vad_init_frame, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, voice_init_cnt, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, voice_frame, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, voice_precent, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, voice_alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, voice_alpha2, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, voice_prob, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, voice_thresh, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_voice_prob, v);

    return 0;
}

int qtk_ahs_gain_controller_cfg_update(qtk_ahs_gain_controller_cfg_t *cfg){
    return 0;
}

int qtk_ahs_gain_controller_cfg_update2(qtk_ahs_gain_controller_cfg_t *cfg, wtk_source_loader_t *sl){
    return 0;
}
