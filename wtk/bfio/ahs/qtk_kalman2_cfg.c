#include "qtk_kalman2_cfg.h"
int qtk_ahs_kalman2_cfg_init(qtk_ahs_kalman2_cfg_t *cfg){
    qtk_ahs_freq_shift_cfg_init(&cfg->freq_shift);
    cfg->B = 20;
    cfg->M = 256;
    cfg->L = 128;
    cfg->alpha = 0.999;
    cfg->Phi_SS_smooth_factor = 0.7;
    cfg->p_initial = 1;
    cfg->use_res = 1;
    cfg->use_fs = 0;
    cfg->use_se = 0;
    cfg->wb_fn = NULL;
    cfg->wb_buf = NULL;
    return 0;
}

int qtk_ahs_kalman2_cfg_clean(qtk_ahs_kalman2_cfg_t *cfg){
    if(cfg->wb_buf){
        wtk_strbuf_delete(cfg->wb_buf);
    }
    qtk_ahs_freq_shift_cfg_clean(&cfg->freq_shift);
    return 0;
}

int qtk_ahs_kalman2_cfg_update_local(qtk_ahs_kalman2_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;
    wtk_local_cfg_t *sub;

    wtk_local_cfg_update_cfg_i(lc, cfg, B, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, L, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, M, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_res, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_se, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_fs, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, alpha, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, Phi_SS_smooth_factor, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, wb_fn, v);

    if(cfg->use_fs){
        sub = wtk_local_cfg_find_lc_s(lc, "fs");
        if (sub) {
            qtk_ahs_freq_shift_cfg_update_local(&cfg->freq_shift, sub);
        }
    }

    return 0;
}

int qtk_ahs_kalman2_cfg_wb_load(wtk_strbuf_t *buf,wtk_source_t *src){
    if(buf){
        wtk_source_read_file2(src,buf);
        return 0;
    }
    return -1;
}

int qtk_ahs_kalman2_cfg_update(qtk_ahs_kalman2_cfg_t *cfg){
    wtk_source_loader_t sl;
    sl.vf = wtk_source_load_file_v;
    sl.hook = 0;
    qtk_ahs_kalman2_cfg_update2(cfg,&sl);
    return 0;
}

int qtk_ahs_kalman2_cfg_update2(qtk_ahs_kalman2_cfg_t *cfg, wtk_source_loader_t *sl){
    if(cfg->wb_fn){
        cfg->wb_buf = wtk_strbuf_new(1024,1);
        wtk_source_loader_load(sl,cfg->wb_buf,(wtk_source_load_handler_t)qtk_ahs_kalman2_cfg_wb_load,cfg->wb_fn);
    }

    if(cfg->use_fs){
        qtk_ahs_freq_shift_cfg_update2(&(cfg->freq_shift),sl);
    }
    return 0;
}