#include "wtk_bfio3_cfg.h" 

int wtk_bfio3_cfg_init(wtk_bfio3_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
	wtk_stft2_cfg_init(&(cfg->sp_stft2));

	wtk_aec_cfg_init(&(cfg->aec));
	wtk_wbf_cfg_init(&(cfg->wbf));
	wtk_wbf2_cfg_init(&(cfg->wbf2));
	wtk_kvadwake_cfg_init(&(cfg->vwake));

	wtk_ssl_cfg_init(&(cfg->ssl));
	wtk_qform9_cfg_init(&(cfg->qform9));
	wtk_qform2_cfg_init(&(cfg->qform2));
    qtk_decoder_wrapper_cfg_init(&cfg->decoder);
	
	cfg->vad_fn=NULL;
	cfg->vad=NULL;

	cfg->stft2_hist=1800;  //ms

	cfg->hook=NULL;
	cfg->use_rbin_res=0;

	cfg->rate=16000;

	cfg->use_vad_start=0;

	cfg->vad_left_margin=20;
	cfg->vad_right_margin=20;

	cfg->use_asr=1;
	cfg->use_aec=1;
    cfg->use_offline_asr = 0;

	cfg->wake_ssl_fs=0.1;
	cfg->wake_ssl_fe=-0.3;

	cfg->use_wbf2=0;
	cfg->use_qform2=0;

	cfg->use_preemph=0;
    cfg->use_raw_audio = 0;
    cfg->debug = 1;
    cfg->vad_fn_use_bin = 1;
    cfg->use_trick = 0;
	cfg->use_thread = 0;

    cfg->idle_conf = 1.35;
    cfg->norm_conf = 1.15;

    return 0;
}

int wtk_bfio3_cfg_clean(wtk_bfio3_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft2));
	wtk_stft2_cfg_clean(&(cfg->sp_stft2));
	wtk_aec_cfg_clean(&(cfg->aec));
	if(cfg->vad)
	{
		if(cfg->use_rbin_res)
		{
			wtk_vad_cfg_delete_bin2(cfg->vad);
		}else
		{
                    if (cfg->vad_fn_use_bin) {
                        wtk_vad_cfg_delete_bin(cfg->vad);
                    } else {
                        wtk_vad_cfg_delete(cfg->vad);
                    }
                }
	}
	wtk_wbf_cfg_clean(&(cfg->wbf));
	wtk_wbf2_cfg_clean(&(cfg->wbf2));
	wtk_kvadwake_cfg_clean(&(cfg->vwake));
	wtk_ssl_cfg_clean(&(cfg->ssl));
	wtk_qform9_cfg_clean(&(cfg->qform9));
	wtk_qform2_cfg_clean(&(cfg->qform2));
    qtk_decoder_wrapper_cfg_clean(&cfg->decoder);
	
	return 0;
}

int wtk_bfio3_cfg_update_local(wtk_bfio3_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_local_cfg_t *m;
	int ret;

	wtk_local_cfg_update_cfg_i(lc, cfg, rate, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, stft2_hist, v);
	wtk_local_cfg_update_cfg_str(lc, cfg, vad_fn, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_vad_start, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, vad_left_margin, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, vad_right_margin, v);

	wtk_local_cfg_update_cfg_b(lc, cfg, use_asr, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_offline_asr, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_aec, v);
        wtk_local_cfg_update_cfg_b(lc, cfg, vad_fn_use_bin, v);

    wtk_local_cfg_update_cfg_b(lc, cfg, use_wbf2, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_qform2, v);

	wtk_local_cfg_update_cfg_f(lc, cfg, wake_ssl_fs, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, wake_ssl_fe, v);

	wtk_local_cfg_update_cfg_b(lc, cfg, use_preemph, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, debug, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_raw_audio, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_trick, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_thread, v);

    m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"aec");
	if(lc)
	{
        ret=wtk_aec_cfg_update_local(&(cfg->aec),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"stft2");
	if(lc)
	{
        ret=wtk_stft2_cfg_update_local(&(cfg->stft2),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"sp_stft2");
	if(lc)
	{
        ret=wtk_stft2_cfg_update_local(&(cfg->sp_stft2),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"wbf");
	if(lc)
	{
        ret=wtk_wbf_cfg_update_local(&(cfg->wbf),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"wbf2");
	if(lc)
	{
        ret=wtk_wbf2_cfg_update_local(&(cfg->wbf2),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"kvadwake");
	if(lc)
	{
        ret=wtk_kvadwake_cfg_update_local(&(cfg->vwake),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"ssl");
	if(lc)
	{
        ret=wtk_ssl_cfg_update_local(&(cfg->ssl),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"qform9");
	if(lc)
	{
        ret=wtk_qform9_cfg_update_local(&(cfg->qform9),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"qform2");
	if(lc)
	{
        ret=wtk_qform2_cfg_update_local(&(cfg->qform2),lc);
        if(ret!=0){goto end;}
    }
    lc = wtk_local_cfg_find_lc_s(m, "asr");
    if (lc) {
        ret = qtk_decoder_wrapper_cfg_update_local(&cfg->decoder, lc);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
	return ret;
}

int wtk_bfio3_cfg_update(wtk_bfio3_cfg_t *cfg)
{
	int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
	ret=wtk_stft2_cfg_update(&(cfg->sp_stft2));
    if(ret!=0){goto end;}
    ret=wtk_aec_cfg_update(&(cfg->aec));
    if(ret!=0){goto end;}
	ret=wtk_wbf_cfg_update(&(cfg->wbf));
	if(ret!=0){goto end;}
	ret=wtk_wbf2_cfg_update(&(cfg->wbf2));
	if(ret!=0){goto end;}
	ret=wtk_kvadwake_cfg_update(&(cfg->vwake));
    if(ret!=0){goto end;}
    ret=wtk_ssl_cfg_update(&(cfg->ssl));
    if(ret!=0){goto end;}
	ret=wtk_qform9_cfg_update(&(cfg->qform9));
	if(ret!=0){goto end;}
	ret=wtk_qform2_cfg_update(&(cfg->qform2));
	if(ret!=0){goto end;}
        if (cfg->use_asr) {
            if (cfg->vad_fn) {
                cfg->vad = cfg->vad_fn_use_bin
                               ? wtk_vad_cfg_new_bin2(cfg->vad_fn)
                               : wtk_vad_cfg_new(cfg->vad_fn);
                if (!cfg->vad) {
                    ret = -1;
                    goto end;
                }
            } else {
                ret = -1;
                wtk_debug("Missing Vad Cfg\n");
                goto end;
            }
        }
        cfg->stft2_hist=cfg->stft2_hist/1000.0*cfg->rate/cfg->stft2.step;	
    if (cfg->use_offline_asr && cfg->use_asr == 0) {
        wtk_debug("!!! Err: use_offline_asr depend on use_asr\n");
        ret = -1;
        goto end;
    }
    if (cfg->use_offline_asr) {
        qtk_decoder_wrapper_cfg_update(&cfg->decoder);
    }
    cfg->vwake.debug = cfg->debug;

    ret = 0;
end:
	return ret;
}

int wtk_bfio3_cfg_update2(wtk_bfio3_cfg_t *cfg, wtk_source_loader_t *sl)
{
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);
	int ret;

	cfg->use_rbin_res=1;
	if(cfg->vad_fn)
	{
		item=wtk_rbin2_get2(rbin,cfg->vad_fn,strlen(cfg->vad_fn));
		if(!item){ret=-1;goto end;}
		cfg->vad=wtk_vad_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->vad){ret=-1;goto end;}
	}
	ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
	ret=wtk_stft2_cfg_update(&(cfg->sp_stft2));
    if(ret!=0){goto end;}
	ret=wtk_aec_cfg_update(&(cfg->aec));
    if(ret!=0){goto end;}
	ret=wtk_wbf_cfg_update2(&(cfg->wbf),sl);
	if(ret!=0){goto end;}
	ret=wtk_wbf2_cfg_update2(&(cfg->wbf2),sl);
	if(ret!=0){goto end;}
	ret=wtk_kvadwake_cfg_update2(&(cfg->vwake),sl);
    if(ret!=0){goto end;}
	ret=wtk_ssl_cfg_update(&(cfg->ssl));
    if(ret!=0){goto end;}
	ret=wtk_qform9_cfg_update2(&(cfg->qform9),sl);
	if(ret!=0){goto end;}
	ret=wtk_qform2_cfg_update2(&(cfg->qform2),sl);
	if(ret!=0){goto end;}

	cfg->stft2_hist=cfg->stft2_hist/1000.0*cfg->rate/cfg->stft2.step;
    if (cfg->use_offline_asr && cfg->use_asr == 0) {
        wtk_debug("!!! Err: use_offline_asr depend on use_asr\n");
        ret = -1;
        goto end;
    }
    if (cfg->use_offline_asr) {
        qtk_decoder_wrapper_cfg_update2(&cfg->decoder, sl);
    }
    cfg->vwake.debug = cfg->debug;

    ret = 0;
end:
	return ret;
}

void wtk_bfio3_cfg_set_wakeword(wtk_bfio3_cfg_t *bfio3, char *wrd)
{
	wtk_kvadwake_cfg_set_wakeword(&(bfio3->vwake), wrd);
}


wtk_bfio3_cfg_t* wtk_bfio3_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_bfio3_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_bfio3_cfg,cfg_fn);
	cfg=(wtk_bfio3_cfg_t*)main_cfg->cfg;
	cfg->hook=main_cfg;
	return cfg;
}

void wtk_bfio3_cfg_delete(wtk_bfio3_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t *)cfg->hook);
}

wtk_bfio3_cfg_t* wtk_bfio3_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_bfio3_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_bfio3_cfg,bin_fn,"./bfio3.cfg");
	cfg=(wtk_bfio3_cfg_t*)mbin_cfg->cfg;
	cfg->hook=mbin_cfg;
	return cfg;
}

void wtk_bfio3_cfg_delete_bin(wtk_bfio3_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t *)cfg->hook);
}
