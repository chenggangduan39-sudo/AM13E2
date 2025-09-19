#include "qtk/ult/qtk_ult_perception_cfg.h"

int qtk_ult_perception_cfg_init(qtk_ult_perception_cfg_t *cfg) {
    cfg->perception_1m_enter_trap = 10;
    cfg->perception_1m_leave_trap = 10;
    cfg->perception_5m_enter_trap = 10;
    cfg->perception_5m_leave_trap = 10;

    cfg->perception_1m_enter_ratio = 0.5;
    cfg->perception_5m_enter_ratio = 0.5;
    cfg->perception_1m_leave_ratio = 0.5;
    cfg->perception_5m_leave_ratio = 0.5;
    cfg->perception_5m_thresh = 0.5;
    cfg->perception_5m_speed_thresh = 0.2;
    cfg->sonicnet_vad_prob = 0.5;
    cfg->sonicnet_vad_enter_trap = 2;
    cfg->sonicnet_vad_leave_trap = 4;
    cfg->use_vad = 1;
    return 0;
}

int qtk_ult_perception_cfg_clean(qtk_ult_perception_cfg_t *cfg) { return 0; }

int qtk_ult_perception_cfg_update_local(qtk_ult_perception_cfg_t *cfg,
                                        wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    lc = main;

    wtk_local_cfg_update_cfg_i(lc, cfg, perception_1m_enter_trap, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, perception_1m_leave_trap, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, perception_5m_enter_trap, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, perception_5m_leave_trap, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, perception_1m_enter_ratio, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, perception_5m_enter_ratio, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, perception_1m_leave_ratio, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, perception_5m_leave_ratio, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, perception_5m_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, perception_5m_speed_thresh, v);

    wtk_local_cfg_update_cfg_f(lc, cfg, sonicnet_vad_prob, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sonicnet_vad_enter_trap, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sonicnet_vad_leave_trap, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_vad, v);

    return 0;
}

int qtk_ult_perception_cfg_update(qtk_ult_perception_cfg_t *cfg) { return 0; }
