#include "qtk_semdlg.h"

static void qtk_semdlg_init(qtk_semdlg_t *semdlg) {
    wtk_queue_init(&semdlg->semdlg_q);
}

qtk_semdlg_t *qtk_semdlg_new(qtk_semdlg_cfg_t *cfg, qtk_session_t *session,
                             wtk_model_t *model) {
    qtk_semdlg_t *semdlg;
    qtk_isemdlg_t *i;
    qtk_isemdlg_cfg_t *isemdlg;
    wtk_queue_node_t *qn;
    int ret;

    semdlg = (qtk_semdlg_t *)wtk_malloc(sizeof(qtk_semdlg_t));
    qtk_semdlg_init(semdlg);

    for (qn = cfg->semdlg_q.pop; qn; qn = qn->next) {
        isemdlg = data_offset2(qn, qtk_isemdlg_cfg_t, q_n);
        i = qtk_isemdlg_new(isemdlg, session, model);
        if (!i) {
            ret = -1;
            goto end;
        }
        wtk_queue_push(&semdlg->semdlg_q, &i->q_n);
    }

    ret = 0;
end:
    if (ret != 0) {
        qtk_semdlg_delete(semdlg);
        semdlg = NULL;
    }
    return semdlg;
}

void qtk_semdlg_delete(qtk_semdlg_t *semdlg) {
    qtk_isemdlg_t *i;
    wtk_queue_node_t *qn;

    while (1) {
        qn = wtk_queue_pop(&semdlg->semdlg_q);
        if (!qn) {
            break;
        }
        i = data_offset2(qn, qtk_isemdlg_t, q_n);
        qtk_isemdlg_delete(i);
    }
    wtk_free(semdlg);
}

wtk_string_t qtk_semdlg_process(qtk_semdlg_t *semdlg, char *data, int bytes,
                                int use_json) {
    wtk_string_t v;
    qtk_isemdlg_t *i;
    wtk_queue_node_t *qn, *qn1;

    wtk_string_set(&v, 0, 0);

    for (qn = semdlg->semdlg_q.pop; qn; qn = qn->next) {
        i = data_offset2(qn, qtk_isemdlg_t, q_n);
        v = qtk_isemdlg_process(i, data, bytes, use_json);
        if (v.len > 0) {
            break;
        }
    }

    if (v.len > 0) {
        for (qn1 = semdlg->semdlg_q.pop; qn1; qn1 = qn1->next) {
            i = data_offset2(qn1, qtk_isemdlg_t, q_n);
            qn == qn1 ? qtk_isemdlg_set_syn(i, 0) : qtk_isemdlg_set_syn(i, 1);
        }
    }
    return v;
}

void qtk_semdlg_set_env(qtk_semdlg_t *semdlg, char *env, int bytes) {
    qtk_isemdlg_t *i;
    wtk_queue_node_t *qn;

    for (qn = semdlg->semdlg_q.pop; qn; qn = qn->next) {
        i = data_offset2(qn, qtk_isemdlg_t, q_n);
        qtk_isemdlg_set_env(i, env, bytes);
    }
}

void qtk_semdlg_set_coreType(qtk_semdlg_t *semdlg, char *data, int len) {
    qtk_isemdlg_t *i;
    wtk_queue_node_t *qn;

    for (qn = semdlg->semdlg_q.pop; qn; qn = qn->next) {
        i = data_offset2(qn, qtk_isemdlg_t, q_n);
        qtk_isemdlg_set_coreType(i, data, len);
    }
}
void qtk_semdlg_set_semRes(qtk_semdlg_t *semdlg, char *data, int len) {
    qtk_isemdlg_t *i;
    wtk_queue_node_t *qn;

    for (qn = semdlg->semdlg_q.pop; qn; qn = qn->next) {
        i = data_offset2(qn, qtk_isemdlg_t, q_n);
        qtk_isemdlg_set_semRes(i, data, len);
    }
}
