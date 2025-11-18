#include "qtk_signal_compress_cfg.h"

int qtk_signal_compress_cfg_init(qtk_signal_compress_cfg_t *cfg)
{
	cfg->mbin_cfg=NULL;
	cfg->main_cfg=NULL;

	qtk_nnrt_cfg_init(&cfg->encoder);
	qtk_nnrt_cfg_init(&cfg->rvq_encoder);

	return 0;
}

int qtk_signal_compress_cfg_clean(qtk_signal_compress_cfg_t *cfg)
{
	qtk_nnrt_cfg_clean(&cfg->encoder);
	qtk_nnrt_cfg_clean(&cfg->rvq_encoder);
	return 0;
}

int qtk_signal_compress_cfg_update_local(qtk_signal_compress_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	int ret;

    lc = wtk_local_cfg_find_lc_s(main, "enc");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->encoder, lc);
		if(ret!=0){goto end;}
    }

    lc = wtk_local_cfg_find_lc_s(main, "rvq_enc");
    if (lc) {
        ret = qtk_nnrt_cfg_update_local(&cfg->rvq_encoder, lc);
		if(ret!=0){goto end;}
    }
	ret=0;
end:
	return ret;
}

int qtk_signal_compress_cfg_update(qtk_signal_compress_cfg_t *cfg)
{
	wtk_source_loader_t sl;

    sl.hook=0;
    sl.vf=wtk_source_load_file_v;
	return qtk_signal_compress_cfg_update2(cfg,&sl);
}

int qtk_signal_compress_cfg_update2(qtk_signal_compress_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=qtk_nnrt_cfg_update2(&cfg->encoder,sl);
	if(ret!=0){goto end;}

	ret=qtk_nnrt_cfg_update2(&cfg->rvq_encoder,sl);
	if(ret!=0){goto end;}

	ret=0;
end:
	return ret;
}

qtk_signal_compress_cfg_t* qtk_signal_compress_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_signal_compress_cfg_t *cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_signal_compress_cfg,fn);
	if(!main_cfg)
	{
		return NULL;
	}
	cfg=(qtk_signal_compress_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void qtk_signal_compress_cfg_delete(qtk_signal_compress_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_signal_compress_cfg_t* qtk_signal_compress_cfg_new_bin(char *fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_signal_compress_cfg_t *cfg;

	mbin_cfg=wtk_mbin_cfg_new_type(qtk_signal_compress_cfg,fn,"./c.cfg");
	if(!mbin_cfg)
	{
		return NULL;
	}
	cfg=(qtk_signal_compress_cfg_t*)mbin_cfg->cfg;
	cfg->mbin_cfg=mbin_cfg;
	return cfg;
}

void qtk_signal_compress_cfg_delete_bin(qtk_signal_compress_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

