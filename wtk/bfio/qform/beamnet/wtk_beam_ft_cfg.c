#include "wtk_beam_ft_cfg.h"

int wtk_beam_ft_cfg_init(wtk_beam_ft_cfg_t *cfg) {
    cfg->main_cfg = NULL;
    cfg->mbin_cfg = NULL;

    cfg->wins = 512;
    cfg->nbin = 257;
    cfg->rate = 16000;
    cfg->sv = 340;
    cfg->channel = 0;
    cfg->nmic = 0;
    cfg->mic_pos = NULL;

    cfg->ref_channel = 0;
    cfg->theta_step = 10;

    cfg->use_line = 0;
    cfg->use_csn = 0;
    return 0;
}
int wtk_beam_ft_cfg_clean(wtk_beam_ft_cfg_t *cfg) {
    int i;
    if (cfg->mic_pos) {
        for (i = 0; i < cfg->nmic; ++i) {
            wtk_free(cfg->mic_pos[i]);
        }
        wtk_free(cfg->mic_pos);
    }
    return 0;
}

int wtk_beam_ft_cfg_update_local(wtk_beam_ft_cfg_t *cfg, wtk_local_cfg_t *m) {
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret;
    wtk_array_t *a;
    int i;

    lc = m;
    wtk_local_cfg_update_cfg_i(lc, cfg, wins, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, sv, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, ref_channel, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, theta_step, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_line, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_csn, v);

    lc = wtk_local_cfg_find_lc_s(m, "mic");
    if (lc) {
        wtk_queue_node_t *qn;
        wtk_cfg_item_t *item;
        int i;

        cfg->mic_pos =
            (float **)wtk_malloc(sizeof(float *) * lc->cfg->queue.length);
        cfg->nmic = 0;
        for (qn = lc->cfg->queue.pop; qn; qn = qn->next) {
            item = data_offset2(qn, wtk_cfg_item_t, n);
            if (item->type != WTK_CFG_ARRAY || item->value.array->nslot != 3) {
                continue;
            }
            cfg->mic_pos[cfg->nmic] = (float *)wtk_malloc(sizeof(float) * 3);
            for (i = 0; i < 3; ++i) {
                v = ((wtk_string_t **)item->value.array->slot)[i];
                cfg->mic_pos[cfg->nmic][i] = wtk_str_atof(v->data, v->len);
                // wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
            }
            ++cfg->nmic;
        }
    }
    ret = 0;
end:
    return ret;
}

int wtk_beam_ft_cfg_update(wtk_beam_ft_cfg_t *cfg) {
    cfg->nbin = cfg->wins / 2 + 1;
    cfg->channel = cfg->nmic;
    cfg->out_channels = cfg->nmic * (cfg->nmic - 1) / 2;
    if (cfg->use_line) {
        cfg->min_theta = cfg->theta_step / 2;
        cfg->max_theta = 180 - cfg->theta_step / 2;
        cfg->n_theta = (int)((180 - cfg->min_theta) / cfg->theta_step) + 1;
    } else {
        cfg->min_theta = cfg->theta_step / 2;
        cfg->max_theta = 360 - cfg->theta_step / 2;
        cfg->n_theta = (int)((360 - cfg->min_theta) / cfg->theta_step) + 1;
    }
    return 0;
}
int wtk_beam_ft_cfg_update2(wtk_beam_ft_cfg_t *cfg, wtk_source_loader_t *sl) {
    return wtk_beam_ft_cfg_update(cfg);
}

wtk_beam_ft_cfg_t *wtk_beam_ft_cfg_new(char *fn) {
    wtk_main_cfg_t *main_cfg;
    wtk_beam_ft_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(wtk_beam_ft_cfg, fn);
    if (!main_cfg) {
        return NULL;
    }
    cfg = (wtk_beam_ft_cfg_t *)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void wtk_beam_ft_cfg_delete(wtk_beam_ft_cfg_t *cfg) {
    wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_beam_ft_cfg_t *wtk_beam_ft_cfg_new_bin(char *fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_beam_ft_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(wtk_beam_ft_cfg, fn, "./cfg");
    if (!mbin_cfg) {
        return NULL;
    }
    cfg = (wtk_beam_ft_cfg_t *)mbin_cfg->cfg;
    cfg->mbin_cfg = mbin_cfg;
    return cfg;
}

void wtk_beam_ft_cfg_delete_bin(wtk_beam_ft_cfg_t *cfg) {
    wtk_mbin_cfg_delete(cfg->mbin_cfg);
}
