#include "qtk_mic_check_cfg.h"

int qtk_mic_check_cfg_init(qtk_mic_check_cfg_t *cfg) {
    wtk_stft2_cfg_init(&cfg->stft2);
    cfg->thresh = 0.03;
    cfg->filterA_fn = 0;
    cfg->filterB_fn = 0;
    cfg->filt_a = 0;
    cfg->filt_b = 0;
    return 0;
}

int qtk_mic_check_cfg_clean(qtk_mic_check_cfg_t *cfg) {
    wtk_stft2_cfg_clean(&cfg->stft2);
    if(cfg->filt_a){
        wtk_free(cfg->filt_a);
    }
    if(cfg->filt_b){
        wtk_free(cfg->filt_b);
    }
    return 0;
}

int qtk_mic_check_cfg_update(qtk_mic_check_cfg_t *cfg) {
    wtk_source_loader_t sl;

    sl.hook=0;
    sl.vf=wtk_source_load_file_v;
    return qtk_mic_check_cfg_update2(cfg,&sl);
}

int qtk_mic_check_load_filter(double *val, wtk_source_t *src){
    int ret;
    ret = wtk_source_read_double_little(src, val, 16, 1);
    return ret;
}

int qtk_mic_check_cfg_update2(qtk_mic_check_cfg_t *cfg, wtk_source_loader_t *sl) {
    int ret = 0;
    if(cfg->filterA_fn){
        cfg->filt_a = (double*)wtk_malloc(sizeof(double)*16);
        ret = wtk_source_loader_load(sl,cfg->filt_a,(wtk_source_load_handler_t)qtk_mic_check_load_filter,cfg->filterA_fn);
    }

    if(cfg->filterB_fn){
        cfg->filt_b = (double*)wtk_malloc(sizeof(double)*16);
        ret = wtk_source_loader_load(sl,cfg->filt_b,(wtk_source_load_handler_t)qtk_mic_check_load_filter,cfg->filterB_fn);
    }

    wtk_stft2_cfg_update(&cfg->stft2);

    return ret; 
}

int qtk_mic_check_cfg_update_local(qtk_mic_check_cfg_t *cfg, wtk_local_cfg_t *main) {
    wtk_local_cfg_t *lc;
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_str(main,cfg,filterA_fn,v);
    wtk_local_cfg_update_cfg_str(main,cfg,filterB_fn,v);
    lc = wtk_local_cfg_find_lc_s(main, "stft2");
    if (lc) {
        if (wtk_stft2_cfg_update_local(&cfg->stft2, lc)) {
            goto err;
        }
    }
    return 0;
err:
    return -1;
    }
