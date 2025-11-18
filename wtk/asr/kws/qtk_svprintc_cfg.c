#include "qtk_svprintc_cfg.h"

int qtk_svprintc_cfg_init(qtk_svprintc_cfg_t *cfg)
{
    memset(cfg,0,sizeof(qtk_svprintc_cfg_t));
    wtk_nnet3_xvector_compute_cfg_init(&cfg->xvector);
    wtk_ivector_plda_scoring_cfg_init(&cfg->plda);
    wtk_kvad_cfg_init(&cfg->kvad);

    return 0;
}

int qtk_svprintc_cfg_update_local(qtk_svprintc_cfg_t *cfg,wtk_local_cfg_t *main_lc)
{
    wtk_string_t *v = NULL;
    wtk_local_cfg_t *lc = NULL;
    wtk_local_cfg_update_cfg_str(main_lc,cfg,svprint_nn_fn,v);
    wtk_local_cfg_update_cfg_str(main_lc,cfg,pool_fn,v);
    wtk_local_cfg_update_cfg_i(main_lc,cfg,use_plda,v);
    lc = wtk_local_cfg_find_lc_s(main_lc, "xvector");
    if(lc){
        wtk_nnet3_xvector_compute_cfg_update_local(&cfg->xvector,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc, "vad");
    if(lc){
        wtk_kvad_cfg_update_local(&cfg->kvad,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_lc,"plda");
    if(lc){
        wtk_ivector_plda_scoring_cfg_update_local(&cfg->plda,lc);
    }
    return 0;
}

int qtk_svprintc_cfg_update(qtk_svprintc_cfg_t *cfg)
{
    wtk_nnet3_xvector_compute_cfg_update(&cfg->xvector);
    wtk_ivector_plda_scoring_cfg_update(&cfg->plda);
    wtk_kvad_cfg_update(&cfg->kvad);
    return 0;
}

int qtk_svprintc_cfg_clean(qtk_svprintc_cfg_t *cfg)
{
    wtk_nnet3_xvector_compute_cfg_clean(&cfg->xvector);
    wtk_ivector_plda_scoring_cfg_clean(&cfg->plda);
    wtk_kvad_cfg_clean(&cfg->kvad);

    return 0;
}