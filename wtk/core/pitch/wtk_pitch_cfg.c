#include "wtk_pitch_cfg.h" 
#include "wtk/core/math/wtk_math.h"

int wtk_pitch_cfg_init(wtk_pitch_cfg_t *cfg)
{
	cfg->main_cfg = NULL;
	cfg->mbin_cfg = NULL;

	cfg->sample_rate=16000;
	cfg->fft_frame_size=512;
	cfg->over_sampling=4;
	cfg->max_v=32768;
	cfg->thresh=0.8;
	wtk_pitch_cfg_update(cfg);
	return 0;
}

int wtk_pitch_cfg_clean(wtk_pitch_cfg_t *cfg)
{
	return 0;
}

int wtk_pitch_cfg_update_local(wtk_pitch_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,sample_rate,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,fft_frame_size,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,over_sampling,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_v,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,thresh,v);
	return 0;
}

int wtk_pitch_cfg_update(wtk_pitch_cfg_t *cfg)
{
	cfg->scale=1.0/cfg->max_v;
	cfg->step_size=cfg->fft_frame_size/cfg->over_sampling;
	cfg->latency=cfg->fft_frame_size-cfg->step_size;
	cfg->expct=(2.0*M_PI*cfg->step_size)/cfg->fft_frame_size;
	return 0;
}

int wtk_pitch_cfg_update2(wtk_pitch_cfg_t *cfg,wtk_source_loader_t  *sl)
{
	return wtk_pitch_cfg_update(cfg);
}

wtk_pitch_cfg_t* wtk_pitch_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	wtk_pitch_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(wtk_pitch_cfg,fn);
	if(!main_cfg) {
		return NULL;
	}
	cfg = (wtk_pitch_cfg_t*)main_cfg->cfg;
	cfg->main_cfg = main_cfg;
	return cfg;
}

void wtk_pitch_cfg_delete(wtk_pitch_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

wtk_pitch_cfg_t* wtk_pitch_cfg_new_bin(char *bin_fn)
{
	wtk_pitch_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (wtk_pitch_cfg_t*)wtk_malloc(sizeof(wtk_pitch_cfg_t));
	wtk_pitch_cfg_init(cfg);

	cfg->rbin = wtk_rbin2_new();
	ret = wtk_rbin2_read(cfg->rbin,bin_fn);
	if(ret != 0) {
		wtk_debug("read failed %s\n",bin_fn);
		goto end;
	}

	item = wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item) {
		wtk_debug("%s not found %s\n",cfg_fn,bin_fn);
		ret = -1;
		goto end;
	}

	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret = wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret != 0) {
		goto end;
	}

	ret = wtk_pitch_cfg_update_local(cfg,cfg->cfile->main);
	if(ret != 0) {
		goto end;
	}

	sl.hook = cfg->rbin;
	sl.vf   = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = wtk_pitch_cfg_update2(cfg,&sl);
	if(ret != 0) {
		goto end;
	}

	ret = 0;
end:
	if(ret != 0) {
		wtk_pitch_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;

}

void wtk_pitch_cfg_delete_bin(wtk_pitch_cfg_t *cfg)
{
	wtk_pitch_cfg_clean(cfg);

	if(cfg->cfile) {
		wtk_cfg_file_delete(cfg->cfile);
	}

	if(cfg->rbin) {
		wtk_rbin2_delete(cfg->rbin);
	}

	wtk_free(cfg);
}
