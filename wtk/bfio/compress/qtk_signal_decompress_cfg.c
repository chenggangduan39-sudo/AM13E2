#include "qtk_signal_decompress_cfg.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"

int qtk_signal_decompress_cfg_init(qtk_signal_decompress_cfg_t *cfg)
{
	cfg->mbin_cfg=NULL;
	cfg->main_cfg=NULL;

	qtk_nnrt_cfg_init(&cfg->decoder);
	qtk_nnrt_cfg_init(&cfg->rvq_decoder);
	qtk_nnrt_cfg_init(&cfg->head);
	cfg->window = NULL;
	cfg->win_fn = NULL;

	return 0;
}

int qtk_signal_decompress_cfg_clean(qtk_signal_decompress_cfg_t *cfg)
{
    if (cfg->window) {
        wtk_string_delete(cfg->window);
    }

	qtk_nnrt_cfg_clean(&cfg->decoder);
	qtk_nnrt_cfg_clean(&cfg->rvq_decoder);
	qtk_nnrt_cfg_clean(&cfg->head);
	return 0;
}

int qtk_signal_decompress_cfg_update_local(qtk_signal_decompress_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	wtk_local_cfg_update_cfg_str_local(main, cfg, win_fn, v);	
    lc = wtk_local_cfg_find_lc_s(main, "dec");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->decoder, lc);
		if(ret!=0){goto end;}
    }

    lc = wtk_local_cfg_find_lc_s(main, "rvq_dec");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->rvq_decoder, lc);
		if(ret!=0){goto end;}
    }
    lc = wtk_local_cfg_find_lc_s(main, "head");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->head, lc);
		if(ret!=0){goto end;}
    }
	ret=0;
end:
	return ret;
}

int qtk_signal_decompress_cfg_update(qtk_signal_decompress_cfg_t *cfg)
{
    wtk_source_loader_t sl;

    sl.hook=0;
    sl.vf=wtk_source_load_file_v;
	return qtk_signal_decompress_cfg_update2(cfg,&sl);
}

int qtk_signal_decompress_load_window(qtk_signal_decompress_cfg_t *cfg, wtk_source_t *src) {
    cfg->window = wtk_source_read_file(src);
    return 0;
}


int qtk_signal_decompress_cfg_update2(qtk_signal_decompress_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	wtk_source_loader_load(sl, cfg, (wtk_source_load_handler_t)qtk_signal_decompress_load_window,
                               cfg->win_fn);

	ret=qtk_nnrt_cfg_update2(&cfg->decoder,sl);
	if(ret!=0){goto end;}

	ret=qtk_nnrt_cfg_update2(&cfg->rvq_decoder,sl);
	if(ret!=0){goto end;}

	ret=qtk_nnrt_cfg_update2(&cfg->head,sl);
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

qtk_signal_decompress_cfg_t* qtk_signal_decompress_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_signal_decompress_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_signal_decompress_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(qtk_signal_decompress_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void qtk_signal_decompress_cfg_delete(qtk_signal_decompress_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_signal_decompress_cfg_t* qtk_signal_decompress_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_signal_decompress_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(qtk_signal_decompress_cfg,fn,"./d.cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(qtk_signal_decompress_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void qtk_signal_decompress_cfg_delete_bin(qtk_signal_decompress_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

