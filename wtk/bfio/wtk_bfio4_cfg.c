#include "wtk_bfio4_cfg.h" 

int bfio4_img_thresh_cfg_init(bfio4_img_thresh_cfg_t *img_thresh){
    img_thresh->av_prob0 = 5.5; 
    img_thresh->maxx0 = -0.5; 
    img_thresh->avx0 = -2.0; 
    img_thresh->max_prob0 = 8.8; 

    img_thresh->av_prob1 = 9.2; 
    img_thresh->maxx1 = -0.5; 
    img_thresh->avx1 = -2.0; 
    img_thresh->max_prob1 = 15.5; 

    return 0;
}

int bfio4_img_thresh_cfg_clean(bfio4_img_thresh_cfg_t *cfg){
    return 0;
}

int bfio4_img_thresh_cfg_update_local(bfio4_img_thresh_cfg_t *cfg, wtk_local_cfg_t *lc){
	wtk_string_t *v;

    wtk_local_cfg_update_cfg_f(lc, cfg, av_prob0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, maxx0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, avx0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_prob0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, av_prob1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, maxx1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, avx1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_prob1, v);

    return 0;
}

int bfio4_img_thresh_cfg_update(bfio4_img_thresh_cfg_t *cfg){
    return 0;
}

int wtk_bfio4_cfg_init(wtk_bfio4_cfg_t *cfg)
{
	wtk_gainnet_denoise_cfg_init(&(cfg->denoise));
	qtk_img_thresh_cfg_init(&(cfg->img_denoise));
	bfio4_img_thresh_cfg_init(&(cfg->img2_denoise));
	cfg->img=NULL;
	cfg->img_fn=NULL;
	cfg->img2=NULL;
	cfg->img2_fn=NULL;

	cfg->hook=NULL;
	cfg->use_rbin_res=0;

	cfg->min_pvlen=32*16;
    cfg->use_wake=0;
    cfg->use_asr=0;
	cfg->use_denoise_wake=0;
	cfg->use_denoise_asr=0;
	cfg->use_thread=0;
    cfg->debug=0;

    return 0;
}

int wtk_bfio4_cfg_clean(wtk_bfio4_cfg_t *cfg)
{
	wtk_gainnet_denoise_cfg_clean(&(cfg->denoise));
	if(cfg->img)
	{
		if(cfg->use_rbin_res)
		{
			qtk_img_cfg_delete_bin2(cfg->img);
		}else
		{
			qtk_img_cfg_delete_bin(cfg->img);
		}
	}
	if(cfg->img2)
	{
		if(cfg->use_rbin_res)
		{
			qtk_img2_cfg_delete_bin2(cfg->img2);
		}else
		{
			qtk_img2_cfg_delete_bin(cfg->img2);
		}
	}
	return 0;
}

int wtk_bfio4_cfg_update_local(wtk_bfio4_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
	wtk_local_cfg_t *m;
	int ret;

    wtk_local_cfg_update_cfg_i(lc, cfg, min_pvlen, v);
	wtk_local_cfg_update_cfg_str(lc, cfg, img_fn, v);
	wtk_local_cfg_update_cfg_str(lc, cfg, img2_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_wake, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_asr, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_denoise_wake, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_denoise_asr, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_thread, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, debug, v);

    m=lc;
	lc=wtk_local_cfg_find_lc_s(m, "denoise");
	if(lc)
	{
        ret=wtk_gainnet_denoise_cfg_update_local(&(cfg->denoise),lc);
        if(ret!=0){goto end;}
    }

	lc=wtk_local_cfg_find_lc_s(m,"img_denoise");
	if(lc)
	{
        ret=qtk_img_thresh_cfg_update_local(&(cfg->img_denoise),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"img2_denoise");
	if(lc)
	{
        ret=bfio4_img_thresh_cfg_update_local(&(cfg->img2_denoise),lc);
        if(ret!=0){goto end;}
    }

	ret=0;
end:
	return ret;
}

int wtk_bfio4_cfg_update(wtk_bfio4_cfg_t *cfg)
{
	int ret;

	ret=wtk_gainnet_denoise_cfg_update(&(cfg->denoise));
	if(ret!=0){goto end;}
	if(cfg->img_fn)
	{
		cfg->img=qtk_img_cfg_new_bin(cfg->img_fn);
		if(!cfg->img){ret=-1;goto end;}
	}
	if(cfg->img2_fn)
	{
		cfg->img2=qtk_img2_cfg_new_bin(cfg->img2_fn);
		if(!cfg->img2){ret=-1;goto end;}
	}


    ret = 0;
end:
	return ret;
}

int wtk_bfio4_cfg_update2(wtk_bfio4_cfg_t *cfg, wtk_source_loader_t *sl)
{
	int ret;
	wtk_rbin2_item_t *item;
	wtk_rbin2_t *rbin=(wtk_rbin2_t*)(sl->hook);

	cfg->use_rbin_res=1;
	ret=wtk_gainnet_denoise_cfg_update2(&(cfg->denoise),sl);
	if(ret!=0){goto end;}
	if(cfg->img_fn)
	{
		item=wtk_rbin2_get2(rbin,cfg->img_fn,strlen(cfg->img_fn));
		if(!item){ret=-1;goto end;}
		cfg->img=qtk_img_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->img){ret=-1;goto end;}
	}
	if(cfg->img2_fn)
	{
		item=wtk_rbin2_get2(rbin,cfg->img2_fn,strlen(cfg->img2_fn));
		if(!item){ret=-1;goto end;}
		cfg->img2=qtk_img2_cfg_new_bin3(rbin->fn,item->pos);
		if(!cfg->img2){ret=-1;goto end;}
	}

    ret = 0;
end:
	return ret;
}

// void wtk_bfio4_cfg_set_wakeword(wtk_bfio4_cfg_t *bfio4, char *wrd)
// {
// 	wtk_kvadwake_cfg_set_wakeword(&(bfio4->vwake), wrd);
// }


wtk_bfio4_cfg_t* wtk_bfio4_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_bfio4_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_bfio4_cfg,cfg_fn);
	cfg=(wtk_bfio4_cfg_t*)main_cfg->cfg;
	cfg->hook=main_cfg;
	return cfg;
}

void wtk_bfio4_cfg_delete(wtk_bfio4_cfg_t *cfg)
{
	wtk_main_cfg_delete((wtk_main_cfg_t *)cfg->hook);
}

wtk_bfio4_cfg_t* wtk_bfio4_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_bfio4_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(wtk_bfio4_cfg,bin_fn,"./bfio4.cfg");
	cfg=(wtk_bfio4_cfg_t*)mbin_cfg->cfg;
	cfg->hook=mbin_cfg;
	return cfg;
}

void wtk_bfio4_cfg_delete_bin(wtk_bfio4_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t *)cfg->hook);
}
