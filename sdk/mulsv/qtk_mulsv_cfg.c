#include "qtk_mulsv_cfg.h"

int qtk_mulsv_cfg_init(qtk_mulsv_cfg_t *cfg)
{
    wtk_bfio_cfg_init(&cfg->bfio);
    wtk_aec_cfg_init(&(cfg->aec));
    qtk_img_cfg_init(&(cfg->img));
    qtk_img_thresh_cfg_init(&(cfg->img1st));
    wtk_svprint_cfg_init(&(cfg->svprint));

    cfg->use_1st = 0;
    cfg->use_padding=0;
    cfg->use_oppo_log = 0;
    cfg->use_ori_enroll = 1;
    cfg->use_1st_notify_wav = 0;
    cfg->use_aec=0;
    cfg->use_total_log = 0;
    // cfg->bfio.use_aec=0;
    cfg->frame_samp=0.08;
    cfg->cache_tm=5000; //5s
    cfg->wkd_tm=2000;  //2s
    cfg->vprint_feed_len=cfg->wkd_tm*1*32;
    cfg->wake1st_channel=2; //1st wakeup input data channel
    cfg->wake2nd_channel=2; //2nd input channel
    cfg->use_2nd_pad_sp=1;
    cfg->wake1st_right_tm=0.3;
    cfg->vprint_enroll_loff = -0.6;
    cfg->vprint_test_loff = -0.6;

    return 0;
}

int qtk_mulsv_cfg_clean(qtk_mulsv_cfg_t *cfg)
{
    wtk_bfio_cfg_clean(&(cfg->bfio));
    qtk_img_cfg_clean(&(cfg->img));
    wtk_aec_cfg_clean(&(cfg->aec));
    wtk_svprint_cfg_clean(&(cfg->svprint));

    return 0;
}

int qtk_mulsv_cfg_update_local(qtk_mulsv_cfg_t *cfg, wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_local_cfg_t *m;
    int ret;

    wtk_local_cfg_update_cfg_i(lc, cfg, wake1st_channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, wake2nd_channel, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, cache_tm, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, wkd_tm, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, frame_samp, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, wake1st_right_tm, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, vprint_enroll_loff, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, vprint_test_loff, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_1st, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_padding, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_oppo_log, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_ori_enroll, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_1st_notify_wav, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_aec, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_total_log, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_2nd_pad_sp, v);

    if(cfg->use_1st)
    {
        if (cfg->use_aec) 
        {
            m = wtk_local_cfg_find_lc_s(lc, "aec");
            if (m) 
            {
                ret = wtk_aec_cfg_update_local(&(cfg->aec), m);
                if (ret != 0) 
                {
                    goto end;
                }
            }
        }

        m = wtk_local_cfg_find_lc_s(lc, "img");
        if (m) 
        {
            qtk_img_cfg_update_local(&(cfg->img), m);
            cfg->img.min_vad_thresh=0;
            cfg->img.avg_vad_thresh=0;
            cfg->img.max_vad_thresh=0;
            cfg->img.min_noise_dur=0;
        }

        m = wtk_local_cfg_find_lc_s(lc, "img1st");
        if (m) 
        {
            ret = qtk_img_thresh_cfg_update_local(&(cfg->img1st), m);
            if (ret != 0) 
            {
                goto end;
            }
        }
    }

    m = wtk_local_cfg_find_lc_s(lc, "bfio");
    if(m)
    {
        ret = wtk_bfio_cfg_update_local(&(cfg->bfio), m);
        if (ret != 0) 
        {
            goto end;
        }
    }

 
    m = wtk_local_cfg_find_lc_s(lc, "svprint");
    if(m)
    {
        ret=wtk_svprint_cfg_update_local(&(cfg->svprint),m);
        if (ret != 0) 
        {
            goto end;
        }
    }



    ret=0;
end:
    return ret;
}

int qtk_mulsv_cfg_update(qtk_mulsv_cfg_t *cfg)
{
    int ret;

    if(cfg->use_1st)
    {
        if (cfg->use_aec) 
        {
            ret = wtk_aec_cfg_update(&(cfg->aec));
            if (ret != 0) 
            {
                goto end;
            }
        }
        qtk_img_cfg_update(&(cfg->img));
    }

    wtk_bfio_cfg_update(&(cfg->bfio));

    wtk_svprint_cfg_update(&(cfg->svprint));

    ret=0;
end:
    return 0;
}

int qtk_mulsv_cfg_update2(qtk_mulsv_cfg_t *cfg, wtk_source_loader_t *sl)
{
    int ret = 0;
    if (cfg->use_1st) {
        if (cfg->use_aec) {
            ret = wtk_aec_cfg_update2(&(cfg->aec), sl);
            if (ret != 0) {
                goto end;
            }
        }
        qtk_img_cfg_update2(&(cfg->img), sl);
    }

    wtk_bfio_cfg_update2(&(cfg->bfio), sl);

    wtk_svprint_cfg_update2(&(cfg->svprint), sl);

end:
    return ret;
}

qtk_mulsv_cfg_t *qtk_mulsv_cfg_new(char *cfg_fn) 
{
    wtk_main_cfg_t *main_cfg;
    qtk_mulsv_cfg_t *cfg;

    main_cfg = wtk_main_cfg_new_type(qtk_mulsv_cfg, cfg_fn);
    cfg = (qtk_mulsv_cfg_t *)main_cfg->cfg;
    cfg->hook = main_cfg;
    return cfg;
}

qtk_mulsv_cfg_t *qtk_mulsv_cfg_new_bin(char *bin_fn) {
    wtk_mbin_cfg_t *mbin_cfg;
    qtk_mulsv_cfg_t *cfg;

    mbin_cfg = wtk_mbin_cfg_new_type(qtk_mulsv_cfg, bin_fn, "./mulsv.cfg");
    cfg = (qtk_mulsv_cfg_t *)mbin_cfg->cfg;
    cfg->hook = mbin_cfg;
    return cfg;
}

void qtk_mulsv_cfg_delete(qtk_mulsv_cfg_t *cfg) 
{
    wtk_main_cfg_delete((wtk_main_cfg_t *)cfg->hook);
}

void qtk_mulsv_cfg_delete_bin(qtk_mulsv_cfg_t *cfg) {
    wtk_mbin_cfg_delete((wtk_mbin_cfg_t *)cfg->hook);
}
