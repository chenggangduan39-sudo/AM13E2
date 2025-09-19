#include "qtk_gdenoise_cfg.h"


int qtk_gdenoise_cfg_init(qtk_gdenoise_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	wtk_string_set(&cfg->aec_cfg_fn, 0, 0);
	cfg->aec_cfg = NULL;
	// cfg->gaec3_cfg = NULL;
	cfg->gdenoise_cfg = NULL;
	cfg->skip_channels = NULL;
	cfg->use_bin = 0;
	cfg->use_aec_bin = 0;
	cfg->mics = 4;
	cfg->use_aec = 1;
	// cfg->use_gainnet_aec3 = 0;
	cfg->nskip = 0;
	cfg->mic_shift=1.0f;
	cfg->echo_shift=1.0f;
	cfg->use_manual=0;

	return 0;
}
int qtk_gdenoise_cfg_clean(qtk_gdenoise_cfg_t *cfg)
{
	if(cfg->use_aec)
	{
		if(cfg->aec_cfg){
			cfg->use_aec_bin ? wtk_aec_cfg_delete_bin(cfg->aec_cfg) : wtk_aec_cfg_delete(cfg->aec_cfg);
		}
	}
	if(cfg->gdenoise_cfg){
		cfg->use_bin ? wtk_gainnet_denoise_cfg_delete_bin(cfg->gdenoise_cfg) : wtk_gainnet_denoise_cfg_delete(cfg->gdenoise_cfg);
	}

	if(cfg->skip_channels)
	{
		wtk_free(cfg->skip_channels);
	}
	return 0;
}
int qtk_gdenoise_cfg_update_local(qtk_gdenoise_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;
	wtk_array_t *a;
	int i;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, aec_cfg_fn, v);
	printf("gainnet_denoise cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	printf("aec cfg==>[%.*s]\n",cfg->aec_cfg_fn.len, cfg->aec_cfg_fn.data);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_aec_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_aec, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shift, v);

	if(cfg->use_manual==0){
		int nskip=5;
		cfg->skip_channels = (int*)wtk_malloc(sizeof(int) * nskip);
		cfg->nskip = nskip;

		cfg->skip_channels[0] = 4;
		cfg->skip_channels[1] = 5;
		cfg->skip_channels[2] = 6;
		cfg->skip_channels[3] = 7;
		cfg->skip_channels[4] = 9;
	}

	a = wtk_local_cfg_find_array_s(main,"skip_channels");
	if(a) {
		if(cfg->skip_channels)
		{
			wtk_free(cfg->skip_channels);
		}
		cfg->skip_channels = (int*)wtk_malloc(sizeof(int) * a->nslot);
		cfg->nskip = a->nslot;

		for(i=0;i < a->nslot; ++i) {
			v=((wtk_string_t**)a->slot)[i];
			cfg->skip_channels[i] = wtk_str_atoi(v->data,v->len);
		}
	}

	return 0;
}
int qtk_gdenoise_cfg_update(qtk_gdenoise_cfg_t *cfg)
{
	int ret = -1;

	if(cfg->use_aec)
	{
		if(cfg->aec_cfg_fn.len > 0){
			cfg->aec_cfg = cfg->use_bin ? wtk_aec_cfg_new_bin(cfg->aec_cfg_fn.data) : wtk_aec_cfg_new(cfg->aec_cfg_fn.data);
			if(cfg->aec_cfg){
				ret = 0;
			}
		}
	}
	if(cfg->cfg_fn.len > 0){
		cfg->gdenoise_cfg = cfg->use_bin ? wtk_gainnet_denoise_cfg_new_bin(cfg->cfg_fn.data) : wtk_gainnet_denoise_cfg_new(cfg->cfg_fn.data);
		if(cfg->gdenoise_cfg){
			ret = 0;
		}
	}
	return ret;
}
int qtk_gdenoise_cfg_update2(qtk_gdenoise_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_gdenoise_cfg_update(cfg);
}

qtk_gdenoise_cfg_t *qtk_gdenoise_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_gdenoise_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_gdenoise_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_gdenoise_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_gdenoise_cfg_delete(qtk_gdenoise_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_gdenoise_cfg_t *qtk_gdenoise_cfg_new_bin(char *bin_fn)
{
	qtk_gdenoise_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_gdenoise_cfg_t *)wtk_malloc(sizeof(qtk_gdenoise_cfg_t));
	qtk_gdenoise_cfg_init(cfg);
	cfg->rbin = wtk_rbin2_new();
	ret = wtk_rbin2_read(cfg->rbin, bin_fn);
	if(ret != 0){
		wtk_debug("read failed:%s\n", bin_fn);
		goto end;
	}
	item = wtk_rbin2_get2(cfg->rbin, cfg_fn, strlen(cfg_fn));
	if(!item){
		wtk_debug("%s not found %s\n", cfg_fn, bin_fn);
		ret = -1;
		goto end;
	}
	cfg->cfile = wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile, "pwd", ".", 1);
	ret = wtk_cfg_file_feed(cfg->cfile, item->data->data, item->data->len);
	if(ret != 0){
		goto end;
	}
	ret = qtk_gdenoise_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_gdenoise_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_gdenoise_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_gdenoise_cfg_delete_bin(qtk_gdenoise_cfg_t *cfg)
{
	qtk_gdenoise_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}
