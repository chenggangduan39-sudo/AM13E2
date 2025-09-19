#include "qtk/ult/qtk_ult_perception.h"
#include "wtk/core/wtk_alloc.h"
#include "wtk/core/wtk_robin.h"

qtk_ult_perception_t *qtk_ult_perception_new(qtk_ult_perception_cfg_t *cfg) {
    int i;
    int vad_max_trap =
        max(cfg->sonicnet_vad_enter_trap, cfg->sonicnet_vad_leave_trap);
    qtk_ult_perception_t *m =
        wtk_malloc(sizeof(qtk_ult_perception_t) + sizeof(float) * vad_max_trap);
    m->cfg = cfg;
    m->perception_ctx_1m = wtk_robin_new(
        max(cfg->perception_1m_enter_trap, cfg->perception_1m_leave_trap));
    m->perception_ctx_5m = wtk_robin_new(
        max(cfg->perception_5m_enter_trap, cfg->perception_5m_leave_trap));
    m->vad_trap = wtk_robin_new(
        max(cfg->sonicnet_vad_enter_trap, cfg->sonicnet_vad_leave_trap));
    float *trap_data = (float *)(m + 1);
    for (i = 0; i < m->vad_trap->nslot; i++) {
        m->vad_trap->r[i] = trap_data++;
    }
    m->state_1m = m->state_5m = 0;
    m->has_p = QTK_ULT_PERCEPTION_HAS_P_INIT;
    return m;
}

void qtk_ult_perception_delete(qtk_ult_perception_t *m) {
    wtk_robin_delete(m->perception_ctx_1m);
    wtk_robin_delete(m->perception_ctx_5m);
    wtk_robin_delete(m->vad_trap);
    wtk_free(m);
}

static void app_judge_(qtk_ult_perception_t *m, wtk_robin_t *ctx, char *state,
                       int enter_trap, int leave_trap, float enter_ratio,
                       float leave_ratio) {
    int cnt = 0;
    if (*state) {
        if (ctx->used >= leave_trap) {
            for (int i = ctx->used - leave_trap; i < ctx->used; i++) {
                if ((uintptr_t)(wtk_robin_at(ctx, i)) == 0) {
                    cnt++;
                }
            }
            if ((float)cnt / leave_trap > leave_ratio) {
                *state = 0;
            }
        }
    } else {
        if (ctx->used >= enter_trap) {
            for (int i = ctx->used - enter_trap; i < ctx->used; i++) {
                if ((uintptr_t)(wtk_robin_at(ctx, i)) != 0) {
                    cnt++;
                }
            }
            if ((float)cnt / enter_trap > enter_ratio) {
                *state = 1;
            }
        }
    }
}

static void process_vad_(qtk_ult_perception_t *m) {
    int i;
    if (m->has_p == QTK_ULT_PERCEPTION_HAS_P_YES) {
        int has_p = 0;
        if (m->vad_trap->used >= m->cfg->sonicnet_vad_leave_trap) {
            for (i = m->vad_trap->used - m->cfg->sonicnet_vad_leave_trap;
                 i < m->vad_trap->used; i++) {
                if (*(float *)(wtk_robin_at(m->vad_trap, i)) >
                    m->cfg->sonicnet_vad_prob) {
                    has_p = 1;
                    break;
                }
            }
            if (!has_p) {
                m->has_p = QTK_ULT_PERCEPTION_HAS_P_NO;
            }
        }
    } else {
        int has_p = 1;
        if (m->vad_trap->used >= m->cfg->sonicnet_vad_enter_trap) {
            for (i = m->vad_trap->used - m->cfg->sonicnet_vad_enter_trap;
                 i < m->vad_trap->used; i++) {
                if (*(float *)(wtk_robin_at(m->vad_trap, i)) <
                    m->cfg->sonicnet_vad_prob) {
                    has_p = 0;
                    break;
                }
            }
            if (has_p) {
                m->has_p = QTK_ULT_PERCEPTION_HAS_P_YES;
            } else {
                m->has_p = QTK_ULT_PERCEPTION_HAS_P_NO;
            }
        }
    }
}

int qtk_ult_perception_feed(qtk_ult_perception_t *m,
                            qtk_ult_perception_input_t *in,
                            qtk_ult_perception_result_t *out) {
    uintptr_t *cur_1m;
    uintptr_t *cur_5m;
    int i;

    cur_1m = (uintptr_t *)wtk_robin_next_p(m->perception_ctx_1m);
    cur_5m = (uintptr_t *)wtk_robin_next_p(m->perception_ctx_5m);
    *cur_1m = *cur_5m = 0;
    for (i = 0; i < in->nobj; i++) {
        if (in->trk[i].r < 1.2) {
            *cur_1m = 1;
        } else if (in->trk[i].r < 5) {
            *cur_5m = 1;
        }
    }
    app_judge_(
        m, m->perception_ctx_1m, &m->state_1m, m->cfg->perception_1m_enter_trap,
        m->cfg->perception_1m_leave_trap, m->cfg->perception_1m_enter_ratio,
        m->cfg->perception_1m_leave_ratio);
    app_judge_(
        m, m->perception_ctx_5m, &m->state_5m, m->cfg->perception_5m_enter_trap,
        m->cfg->perception_5m_leave_trap, m->cfg->perception_5m_enter_ratio,
        m->cfg->perception_5m_leave_ratio);

    if (m->cfg->use_vad) {
        if (in->vad_prob >= 0) {
            *(float *)wtk_robin_next(m->vad_trap) = in->vad_prob;
        }
        process_vad_(m);

        if (m->state_1m || m->state_5m) {
            if (m->has_p == QTK_ULT_PERCEPTION_HAS_P_NO) {
                m->state_1m = m->state_5m = 0;
            }
        } else {
            if (m->has_p == QTK_ULT_PERCEPTION_HAS_P_YES) {
                m->state_5m = 1;
            }
        }
    }

    out->state_1m = m->state_1m;
    out->state_5m = m->state_5m;
    out->trusted = m->has_p != QTK_ULT_PERCEPTION_HAS_P_INIT;

    return 0;
}
