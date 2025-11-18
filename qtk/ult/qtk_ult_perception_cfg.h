#ifndef D39A13CA_2461_4863_BB42_D83DA7DA0FBB
#define D39A13CA_2461_4863_BB42_D83DA7DA0FBB

#include "wtk/core/cfg/wtk_local_cfg.h"

typedef struct qtk_ult_perception_cfg qtk_ult_perception_cfg_t;
struct qtk_ult_perception_cfg {
    int perception_1m_enter_trap;
    int perception_1m_leave_trap;
    int perception_5m_enter_trap;
    int perception_5m_leave_trap;

    float perception_1m_enter_ratio;
    float perception_5m_enter_ratio;
    float perception_1m_leave_ratio;
    float perception_5m_leave_ratio;
    float perception_5m_thresh;
    float perception_5m_speed_thresh;

    float sonicnet_vad_prob;
    int sonicnet_vad_enter_trap;
    int sonicnet_vad_leave_trap;

    unsigned use_vad : 1;
};

int qtk_ult_perception_cfg_init(qtk_ult_perception_cfg_t *cfg);
int qtk_ult_perception_cfg_clean(qtk_ult_perception_cfg_t *cfg);
int qtk_ult_perception_cfg_update_local(qtk_ult_perception_cfg_t *cfg,
                                        wtk_local_cfg_t *lc);
int qtk_ult_perception_cfg_update(qtk_ult_perception_cfg_t *cfg);

#endif /* D39A13CA_2461_4863_BB42_D83DA7DA0FBB */
