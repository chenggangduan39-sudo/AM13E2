#include "wtk_qmmse_cfg.h"

int wtk_qmmse_cfg_init(wtk_qmmse_cfg_t *cfg)
{
	cfg->noise_suppress=-100;

    cfg->echo_suppress=-50;
    cfg->echo_suppress_active=-20;

	cfg->nbands=24;
    cfg->beta=0.03;
    cfg->init_sym=1e7;
    cfg->max_range=300;
    cfg->noise_prob=0.4f;

    cfg->agc_level=8000;
    cfg->max_gain=30;
    cfg->max_increase_step=12;
    cfg->max_decrease_step=40;
    cfg->max_out_gain=-1;
    cfg->agc_init_frame=0;
    cfg->max_init_increase_step=500;
    cfg->max_init_decrease_step=500;
    cfg->init_max_alpha=0.1f;
    cfg->loudness_pow=-0.2;

    cfg->use_logmmse=0;
    cfg->use_bank=1;
    cfg->use_imcra_org=0;
    cfg->use_agc=0;
    cfg->use_cnon=0;

    cfg->rate=16000;
    cfg->step=256;

    cfg->atcoe=1.0;

    cfg->echo_alpha=0.6f;
    cfg->use_echonoise=0;
    
    cfg->use_sed=0;
    cfg->use_sed2=0;
    cfg->sed_alpha=0.93;
    cfg->io_alpha=1.0;
    cfg->use_agc_smooth=0;
    cfg->init_tgt_eng=8e6;
    cfg->smooth_frame=200;
    cfg->smooth_scale=0.9;
    cfg->tgt_gain_scale=0.5;
    cfg->smooth_percent=0.7;
    cfg->max_smooth_gain=2.0;
    cfg->min_smooth_gain=0.5;

    cfg->use_down_agc=0;
    cfg->down_percent=0.3;
    cfg->down_frame=100;
    cfg->down_scale=0.1f;
    cfg->max_down_gain=5.0f;
    cfg->min_down_gain=0.1f;
    cfg->down_thresh=0.95f;
    cfg->down_cnt=20;

    cfg->use_agc_mask=0;
    cfg->agc_mask_thresh=0.1f;
    cfg->agc_mask_scale=1.0f;
    cfg->agc_mask_scale2=0;

    cfg->agc_pframe_thresh=0.1f;
    cfg->echo_agc_pframe_thresh=0.1f;

    cfg->loudness_thresh=-1;
    cfg->rate_scale=0.99f;
    cfg->loudness_frame=20;

    cfg->entropy_thresh=-1;
    cfg->b_agc_scale=1.0;
    cfg->loudness_thresh2=-1;
    cfg->loudness_frame2=20;

    cfg->agc_mean_mask_thresh=-1;
    cfg->echo_agc_mean_mask_thresh=-1;
    cfg->agc_mask_cnt=0;
    cfg->echo_agc_mask_cnt=0;
    return 0;
}


int wtk_qmmse_cfg_clean(wtk_qmmse_cfg_t *cfg)
{
    return 0;
}

int wtk_qmmse_cfg_update_local(wtk_qmmse_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_f(lc,cfg,atcoe,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,step,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,rate,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,beta,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,init_sym,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,max_range,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,noise_prob,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,echo_alpha,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_echonoise,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_cnon,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_agc,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_imcra_org,v);

    wtk_local_cfg_update_cfg_b(lc,cfg,use_logmmse,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_bank,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,noise_suppress,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,nbands,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,echo_suppress,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,echo_suppress_active,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,agc_level,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_increase_step,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_decrease_step,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_gain,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_out_gain,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,agc_init_frame,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_init_increase_step,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_init_decrease_step,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,init_max_alpha,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,loudness_pow,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sed,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_sed2,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,sed_alpha,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,io_alpha,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_down_agc,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,init_tgt_eng,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,smooth_frame,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,smooth_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,tgt_gain_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,smooth_percent,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_smooth_gain,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,min_smooth_gain,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_agc_smooth,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,down_percent,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,down_frame,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,down_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,max_down_gain,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,min_down_gain,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,down_thresh,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,down_cnt,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_agc_mask,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,agc_mask_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,agc_mask_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,agc_mask_scale2,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,agc_pframe_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,echo_agc_pframe_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,loudness_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,rate_scale,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,loudness_frame,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,entropy_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,b_agc_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,loudness_thresh2,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,loudness_frame2,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,agc_mean_mask_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,echo_agc_mean_mask_thresh,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,agc_mask_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,echo_agc_mask_cnt,v);
    if (cfg->agc_level<1)
    {
        cfg->agc_level=1;
    }
    // if (cfg->agc_level>32768)
    // {
    //     cfg->agc_level=32768;
    // }
    cfg->max_gain = expf(0.11513f * (cfg->max_gain));
    cfg->max_increase_step = expf(0.11513f *cfg->max_increase_step*cfg->step / cfg->rate);
    cfg->max_decrease_step = expf(-0.11513f *cfg->max_decrease_step*cfg->step / cfg->rate);
    cfg->max_init_increase_step = expf(0.11513f *cfg->max_init_increase_step*cfg->step / cfg->rate);
    cfg->max_init_decrease_step = expf(-0.11513f *cfg->max_init_decrease_step*cfg->step / cfg->rate);

    return 0;
}

int wtk_qmmse_cfg_update(wtk_qmmse_cfg_t *cfg)
{
    return 0;
}

int wtk_qmmse_cfg_update2(wtk_qmmse_cfg_t *cfg,wtk_source_loader_t *sl)
{
    return wtk_qmmse_cfg_update(cfg);
}


void wtk_qmmse_cfg_set_noise_suppress(wtk_qmmse_cfg_t *cfg,float noise_suppress)
{
    cfg->noise_suppress = - fabs(noise_suppress);
}

