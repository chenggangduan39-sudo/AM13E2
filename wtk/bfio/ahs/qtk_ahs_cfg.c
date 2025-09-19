#include "wtk/bfio/ahs/qtk_ahs_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/math/wtk_math.h"

int qtk_ahs_cfg_init(qtk_ahs_cfg_t *cfg) {
    qtk_nnrt_cfg_init(&cfg->nnrt);
    cfg->rate = 16000;
    cfg->window_sz = 128;
    cfg->hop_sz = 64;
    cfg->chunk_sz = 8;
    cfg->window = NULL;
    cfg->synthesis_window = NULL;
    cfg->n_delay = 10;
    cfg->loop_delay = 10;
    cfg->pre_scale = 1.0;
    cfg->scale = 1.0;
    wtk_qmmse_cfg_init(&(cfg->qmmse));
    wtk_qmmse_cfg_init(&(cfg->qmmse2));
    cfg->use_out_qmmse=0;
    cfg->use_in_qmmse=0;
    cfg->use_nnrt = 0;
    cfg->use_nnrt2 = 0;
    cfg->use_lstm = 0;
    cfg->use_loop = 0;
    cfg->type = 0;
    cfg->gain = 2.0;
    cfg->rir_gain = 0.8;

    cfg->c0_idx = 3;
    cfg->h0_idx = 2;

    cfg->clip_s = 0;
    cfg->clip_e = 8000;
    cfg->use_clip = 0;

    wtk_equalizer_cfg_init(&(cfg->eq));
    cfg->use_eq=0;
    cfg->use_eq2=0;
    cfg->is_eq2_kalmanout=0;
    cfg->eq_gain_fn = NULL;

    wtk_covm_cfg_init(&(cfg->covm));
    wtk_bf_cfg_init(&(cfg->bf));
    cfg->use_mask = 0;
    cfg->use_fftsbf = 0;
    cfg->use_mask_sp = 1;
    cfg->wiener_thresh = 1.0;
    cfg->kalman_type = 0;
    qtk_ahs_kalman_cfg_init(&(cfg->km));
    qtk_ahs_kalman_cfg_init(&(cfg->km2));
    cfg->use_sp_check = 0;
    cfg->use_mt = 0;

    cfg->use_freq_shift = 0;
    qtk_ahs_freq_shift_cfg_init(&(cfg->freq_shift));

    cfg->use_gainnet = 0;
    qtk_ahs_erb_cfg_init(&(cfg->erb));
    wtk_bbonenet_cfg_init(&(cfg->gainnet));
    qtk_ahs_gain_controller_cfg_init(&(cfg->gain_controller));

    cfg->use_nnvad = 0;
    wtk_kvad_cfg_init(&(cfg->kvad));
    return 0;
}

int qtk_ahs_cfg_clean(qtk_ahs_cfg_t *cfg) {
    qtk_nnrt_cfg_clean(&cfg->nnrt);
    if (cfg->window) {
        wtk_free(cfg->window);
    }
    if (cfg->synthesis_window) {
        wtk_free(cfg->synthesis_window);
    }
    if(cfg->use_nnvad){
        wtk_kvad_cfg_clean(&(cfg->kvad));
    }
    wtk_bf_cfg_clean(&(cfg->bf));
    wtk_covm_cfg_clean(&(cfg->covm));
    wtk_qmmse_cfg_clean(&(cfg->qmmse));
    wtk_qmmse_cfg_clean(&(cfg->qmmse2));
    wtk_equalizer_cfg_clean(&(cfg->eq));
    qtk_ahs_freq_shift_cfg_clean(&(cfg->freq_shift));
    qtk_ahs_kalman_cfg_clean(&(cfg->km));
    qtk_ahs_kalman_cfg_clean(&(cfg->km2));
    qtk_ahs_erb_cfg_clean(&(cfg->erb));
    wtk_bbonenet_cfg_clean(&(cfg->gainnet));
    qtk_ahs_gain_controller_cfg_clean(&(cfg->gain_controller));
    return 0;
}

static void ahs_local_update_(qtk_ahs_cfg_t *cfg) {
    int i;
    cfg->pad_sz = cfg->window_sz - cfg->hop_sz;
    cfg->nbin = cfg->window_sz / 2 + 1;
    switch (cfg->type)
    {
    case 0:
        cfg->feat_dim = cfg->nbin * 4;
        break;
    case 1:
        cfg->feat_dim = cfg->nbin * 2;
        break;
    case 2:
        cfg->feat_dim = cfg->nbin * 6;
        break;
    case 3:
        if(cfg->use_lstm){
            //cfg->feat_dim = cfg->nbin * 4;
            cfg->feat_dim = cfg->nbin * 3;
        }
        break;
    case 7:
        cfg->feat_dim = cfg->nbin * 3;
        break;
    default:
        break;
    }

    cfg->window = wtk_malloc(sizeof(float) * cfg->window_sz);
    for (i = 0; i < cfg->window_sz; i++) {
        cfg->window[i] = 0.5 * (1 - cos(WTK_TPI * i / (cfg->window_sz - 1)));
    }
    cfg->synthesis_window = wtk_malloc(sizeof(float) * cfg->window_sz);
    wtk_drft_init_synthesis_window(cfg->synthesis_window, cfg->window,
                                   cfg->window_sz);
}

int qtk_ahs_cfg_update(qtk_ahs_cfg_t *cfg) {
    qtk_nnrt_cfg_update(&cfg->nnrt);
    ahs_local_update_(cfg);
    wtk_qmmse_cfg_update(&(cfg->qmmse));
    wtk_qmmse_cfg_update(&(cfg->qmmse2));
    wtk_covm_cfg_update(&(cfg->covm));
    wtk_bf_cfg_update(&(cfg->bf));
    wtk_bf_cfg_set_channel(&(cfg->bf), 1);
    wtk_equalizer_cfg_update(&(cfg->eq));
    qtk_ahs_freq_shift_cfg_update(&(cfg->freq_shift));
    qtk_ahs_kalman_cfg_update(&(cfg->km));
    qtk_ahs_kalman_cfg_update(&(cfg->km2));
    if(cfg->use_gainnet){
        qtk_ahs_erb_cfg_update(&(cfg->erb));
        wtk_bbonenet_cfg_update(&(cfg->gainnet));
    }
    if(cfg->use_nnvad){
        wtk_kvad_cfg_update(&(cfg->kvad));
    }

    qtk_ahs_gain_controller_cfg_update(&(cfg->gain_controller));
    return 0;
}

int qtk_ahs_cfg_update_local(qtk_ahs_cfg_t *cfg, wtk_local_cfg_t *lc) {
    wtk_string_t *v;
    wtk_local_cfg_t *sub;
    sub = wtk_local_cfg_find_lc_s(lc, "nnrt");
    if (sub) {
        qtk_nnrt_cfg_update_local(&cfg->nnrt, sub);
    }
    wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, window_sz, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, hop_sz, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, chunk_sz, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, n_delay, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, loop_delay, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, gain, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, rir_gain, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, type, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, pre_scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, scale, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, wiener_thresh, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_out_qmmse,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_in_qmmse,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_loop,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_nnrt,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_nnrt2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_lstm,v);
    wtk_local_cfg_update_cfg_i(lc, cfg, c0_idx, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, h0_idx, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, kalman_type, v);

    wtk_local_cfg_update_cfg_str_local(lc, cfg, eq_gain_fn, v);

    wtk_local_cfg_update_cfg_i(lc, cfg, clip_s, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, clip_e, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_clip,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_mask,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_fftsbf,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_mask_sp,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sp_check,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_eq,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_eq2,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,is_eq2_kalmanout,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_mt,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_freq_shift,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_gainnet,v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_nnvad, v);

    cfg->clip_s = (cfg->clip_s*1.0*cfg->window_sz)/cfg->rate;
    cfg->clip_e = (cfg->clip_e*1.0*cfg->window_sz)/cfg->rate;

    sub=wtk_local_cfg_find_lc_s(lc,"qmmse");
    if(sub)
    {
        wtk_qmmse_cfg_update_local(&(cfg->qmmse),sub);
        cfg->qmmse.step=cfg->window_sz/2;
    }
    sub=wtk_local_cfg_find_lc_s(lc,"qmmse2");
    if(sub)
    {
        wtk_qmmse_cfg_update_local(&(cfg->qmmse2),sub);
        cfg->qmmse2.step=cfg->window_sz/2;
    }
    sub=wtk_local_cfg_find_lc_s(lc,"bf");
    if(sub)
    {
        wtk_bf_cfg_update_local(&(cfg->bf),sub);
    }
    sub=wtk_local_cfg_find_lc_s(lc,"covm");
    if(sub)
    {
        wtk_covm_cfg_update_local(&(cfg->covm),sub);
    }
    sub=wtk_local_cfg_find_lc_s(lc,"eq");
    if(sub)
    {
        wtk_equalizer_cfg_update_local(&(cfg->eq),sub);
    }
    sub=wtk_local_cfg_find_lc_s(lc,"freq_shift");
    if(sub)
    {
        qtk_ahs_freq_shift_cfg_update_local(&(cfg->freq_shift),sub);
    }
	sub=wtk_local_cfg_find_lc_s(lc,"km");
	if(sub)
	{
		qtk_ahs_kalman_cfg_update_local(&(cfg->km),sub);
	}
	sub=wtk_local_cfg_find_lc_s(lc,"km2");
	if(sub)
	{
		qtk_ahs_kalman_cfg_update_local(&(cfg->km2),sub);
	}

    if(cfg->use_gainnet){
        sub=wtk_local_cfg_find_lc_s(lc,"erb");
        if(sub)
        {
            qtk_ahs_erb_cfg_update_local(&(cfg->erb),sub);
        }
        sub=wtk_local_cfg_find_lc_s(lc,"gainnet");
        if(sub){
            wtk_bbonenet_cfg_update_local(&(cfg->gainnet),sub);
        }
    }

    if(cfg->use_nnvad){
        sub=wtk_local_cfg_find_lc_s(lc,"vad");
        if(sub)
        {
            wtk_kvad_cfg_update_local(&(cfg->kvad),sub);
        }
    }

    sub=wtk_local_cfg_find_lc_s(lc,"gc");
    if(sub){
        qtk_ahs_gain_controller_cfg_update_local(&(cfg->gain_controller),sub);
    }
    return 0;
}

int qtk_ahs_cfg_update2(qtk_ahs_cfg_t *cfg, wtk_source_loader_t *sl) {
    qtk_nnrt_cfg_update2(&cfg->nnrt, sl);
    ahs_local_update_(cfg);
    wtk_qmmse_cfg_update(&(cfg->qmmse));
    wtk_qmmse_cfg_update(&(cfg->qmmse2));
    wtk_covm_cfg_update(&(cfg->covm));
    wtk_bf_cfg_update(&(cfg->bf));
    wtk_bf_cfg_set_channel(&(cfg->bf), 1);
    wtk_equalizer_cfg_update(&(cfg->eq));
    qtk_ahs_freq_shift_cfg_update2(&(cfg->freq_shift),sl);
    qtk_ahs_kalman_cfg_update2(&cfg->km, sl);
    qtk_ahs_kalman_cfg_update2(&cfg->km2, sl);
    if(cfg->use_gainnet){
        qtk_ahs_erb_cfg_update2(&(cfg->erb),sl);
        wtk_bbonenet_cfg_update2(&(cfg->gainnet),sl);
    }
    if(cfg->use_nnvad){
        wtk_kvad_cfg_update2(&(cfg->kvad),sl);
    }
    qtk_ahs_gain_controller_cfg_update2(&(cfg->gain_controller),sl);
    return 0;
}


qtk_ahs_cfg_t* qtk_ahs_cfg_new(char *fn)
{
    wtk_main_cfg_t *main_cfg;
    qtk_ahs_cfg_t *cfg;

    main_cfg=wtk_main_cfg_new_type(qtk_ahs_cfg,fn);
    if(!main_cfg)
    {
        return NULL;
    }
    cfg=(qtk_ahs_cfg_t*)main_cfg->cfg;
    cfg->main_cfg = main_cfg;
    return cfg;
}

void qtk_ahs_cfg_delete(qtk_ahs_cfg_t *cfg)
{
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_ahs_cfg_t* qtk_ahs_cfg_new_bin(char *fn)
{
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_ahs_cfg_t *cfg;

    mbin_cfg=wtk_mbin_cfg_new_type(qtk_ahs_cfg,fn,"./cfg");
    if(!mbin_cfg)
    {
        return NULL;
    }
    cfg=(qtk_ahs_cfg_t*)mbin_cfg->cfg;
    cfg->mbin_cfg=mbin_cfg;
    return cfg;
}

void qtk_ahs_cfg_delete_bin(qtk_ahs_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

void qtk_ahs_cfg_set(qtk_ahs_cfg_t *cfg, float pre_scale, float n_delay){
    cfg->pre_scale = pre_scale;
    if(cfg->n_delay > 0){
        cfg->n_delay = n_delay;
    }
}
