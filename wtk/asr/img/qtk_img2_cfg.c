#include "qtk_img2_cfg.h"
#include "wtk/core/cfg/wtk_source.h"

qtk_img2_thresh_cfg_t *qtk_img2_thresh_cfg_init(void)
{
    qtk_img2_thresh_cfg_t *img_thresh;
    img_thresh = (qtk_img2_thresh_cfg_t *)wtk_malloc(sizeof(qtk_img2_thresh_cfg_t));
    img_thresh->av_prob0 = 5.5; 
    img_thresh->maxx0 = -0.5; 
    img_thresh->avx0 = -2.0; 
    img_thresh->max_prob0 = 8.8; 
	img_thresh->speech_dur0 = 2;

    img_thresh->av_prob1 = 9.2; 
    img_thresh->maxx1 = -0.5; 
    img_thresh->avx1 = -2.0; 
    img_thresh->max_prob1 = 15.5; 
	img_thresh->speech_dur1 = 2;

    return img_thresh;
}

int qtk_img2_thresh_cfg_clean(qtk_img2_thresh_cfg_t *cfg) {
    wtk_free(cfg);
    return 0;
}

int qtk_img2_cfg_init(qtk_img2_cfg_t *cfg)
{
	cfg->cfg.main_cfg = NULL;
	cfg->avg_vad_thresh = 0.2;
	cfg->max_vad_thresh = 0.5;
	cfg->min_noise_dur = 2;
	cfg->min_vad_thresh = 0;
	// cfg->m_thresh = 9.5;
	// cfg->av_thresh = 8.8;
        cfg->noise_id = 0;
        cfg->channels = 4;
        cfg->rbin = NULL;
	cfg->cfile = NULL;
	/*Default thresh cfg*/
    cfg->thresh = qtk_img2_thresh_cfg_init();
	cfg->thresh_echo = qtk_img2_thresh_cfg_init();
        /*Default thresh cfg*/
        wtk_kparm_cfg_init(&(cfg->kparm));
        qtk_nnet3_cfg_init(&(cfg->nnet3));
        return 0;
}

int qtk_img2_cfg_clean(qtk_img2_cfg_t *cfg)
{
    qtk_img2_thresh_cfg_clean(cfg->thresh);
    qtk_img2_thresh_cfg_clean(cfg->thresh_echo);
    qtk_nnet3_cfg_clean(&(cfg->nnet3));
    wtk_kparm_cfg_clean(&(cfg->kparm));
    if (cfg->cfile) {
        wtk_cfg_file_delete(cfg->cfile);
	}
	return 0;
}

int qtk_img2_thresh_cfg_update_local(qtk_img2_thresh_cfg_t *cfg, wtk_local_cfg_t *lc){
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_f(lc, cfg, av_prob0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, maxx0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, avx0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_prob0, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, speech_dur0, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, av_prob1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, maxx1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, avx1, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_prob1, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, speech_dur1, v);

    return 0;
}

int qtk_img2_cfg_update_local(qtk_img2_cfg_t *cfg, wtk_local_cfg_t *main)
{
    int ret = 0;
    wtk_string_t *v;
    wtk_local_cfg_t *lc = main;

    wtk_local_cfg_update_cfg_f(lc, cfg, avg_vad_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, max_vad_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, min_vad_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, min_noise_dur, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, m_thresh, v);
    wtk_local_cfg_update_cfg_f(lc, cfg, av_thresh, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, noise_id, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, channels, v);
    // wtk_debug("%f %f %f
    // %d\n",cfg->avg_vad_thresh,cfg->max_vad_thresh,cfg->min_vad_thresh,cfg->min_noise_dur);
    lc = wtk_local_cfg_find_lc_s(main, "parm");
    if (lc) {
        ret = wtk_kparm_cfg_update_local(&(cfg->kparm), lc);
        if (ret != 0) {
            goto end;
        }
    }

    lc = wtk_local_cfg_find_lc_s(main, "nnet3");
    if (lc) {
        ret = qtk_nnet3_cfg_update_local(&(cfg->nnet3), lc);
        if (ret != 0) {
            goto end;
        }
    }
    lc = wtk_local_cfg_find_lc_s(main, "thresh");
    if (lc) {
        ret=qtk_img2_thresh_cfg_update_local(cfg->thresh,lc);
    }
	lc=wtk_local_cfg_find_lc_s(main,"thresh_echo");
	if(lc)
	{
        ret=qtk_img2_thresh_cfg_update_local(cfg->thresh_echo,lc);
    }

end:
    return ret;
}
int qtk_img2_cfg_update(qtk_img2_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook = 0;
	sl.vf = wtk_source_load_file_v;

	return qtk_img2_cfg_update2(cfg, &sl);
}
int qtk_img2_cfg_update2(qtk_img2_cfg_t *cfg, wtk_source_loader_t *sl)
{
    int ret = 0;

    ret = wtk_kparm_cfg_update2(&(cfg->kparm), sl);
    if (ret != 0) {
        wtk_debug("update feature failed\n");
        goto end;
    }
    ret = qtk_nnet3_cfg_update2(&(cfg->nnet3), sl);
    if (ret != 0) {
        wtk_debug("update nnet3 failed\n");
        goto end;
    }

end:
    return ret;
}

qtk_img2_cfg_t *qtk_img2_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *bin_cfg;
	qtk_img2_cfg_t *cfg = NULL;

	bin_cfg = wtk_mbin_cfg_new_type(qtk_img2_cfg, fn, "./asr.cfg");
	if (!bin_cfg)
	{
		goto end;
	}
	cfg = (qtk_img2_cfg_t *)bin_cfg->cfg;
	cfg->cfg.bin_cfg = bin_cfg;
end:
	return cfg;
}

void qtk_img2_cfg_delete_bin(qtk_img2_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->cfg.bin_cfg);
}

qtk_img2_cfg_t *qtk_img2_cfg_new_bin3(char *bin_fn, unsigned int seek_pos)
{
	qtk_img2_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./asr.cfg";
	int ret;

	cfg = (qtk_img2_cfg_t *)wtk_malloc(sizeof(qtk_img2_cfg_t));
	qtk_img2_cfg_init(cfg);
	cfg->rbin = wtk_rbin2_new();
	ret = wtk_rbin2_read2(cfg->rbin, bin_fn, seek_pos);
	if (ret != 0)
	{
		wtk_debug("read failed\n");
		goto end;
	}
	item = wtk_rbin2_get2(cfg->rbin, cfg_fn, strlen(cfg_fn));
	if (!item)
	{
		wtk_debug("%s not found %s\n", cfg_fn, bin_fn);
		ret = -1;
		goto end;
	}
	cfg->cfile = wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile, "pwd", ".", 1);
	ret = wtk_cfg_file_feed(cfg->cfile, item->data->data, item->data->len);
	if (ret != 0)
	{
		goto end;
	}
	ret = qtk_img2_cfg_update_local(cfg, cfg->cfile->main);
	if (ret != 0)
	{
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_img2_cfg_update2(cfg, &sl);
	if (ret != 0)
	{
		goto end;
	}
end:
	// wtk_debug("ret=%d\n",ret);
	if (ret != 0)
	{
		qtk_img2_cfg_delete_bin2(cfg);
		cfg = NULL;
	}
	return cfg;
}

void qtk_img2_cfg_delete_bin2(qtk_img2_cfg_t *cfg)
{
	qtk_img2_cfg_clean(cfg);
	wtk_rbin2_delete(cfg->rbin);
	wtk_free(cfg);
}
