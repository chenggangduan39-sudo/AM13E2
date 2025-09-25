#include "qtk_estimate_cfg.h"


int qtk_estimate_cfg_init(qtk_estimate_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	cfg->estimate_cfg = NULL;
	cfg->use_bin = 0;
	cfg->use_manual=0;

	return 0;
}
int qtk_estimate_cfg_clean(qtk_estimate_cfg_t *cfg)
{
	if(cfg->estimate_cfg){
		cfg->use_bin ? qtk_rir_estimate2_cfg_delete_bin(cfg->estimate_cfg) : qtk_rir_estimate2_cfg_delete(cfg->estimate_cfg);
	}

	return 0;
}
int qtk_estimate_cfg_update_local(qtk_estimate_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;
	wtk_array_t *a;
	int i;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	printf("estimate cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);

	return 0;
}
int qtk_estimate_cfg_update(qtk_estimate_cfg_t *cfg)
{
	int ret = -1;

	if(cfg->cfg_fn.len > 0){
		cfg->estimate_cfg = cfg->use_bin ? qtk_rir_estimate2_cfg_new_bin(cfg->cfg_fn.data) : qtk_rir_estimate2_cfg_new(cfg->cfg_fn.data);
		if(cfg->estimate_cfg){
			ret = 0;
		}
	}
	return ret;
}
int qtk_estimate_cfg_update2(qtk_estimate_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_estimate_cfg_update(cfg);
}

qtk_estimate_cfg_t *qtk_estimate_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_estimate_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_estimate_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_estimate_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_estimate_cfg_delete(qtk_estimate_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_estimate_cfg_t *qtk_estimate_cfg_new_bin(char *bin_fn)
{
	qtk_estimate_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_estimate_cfg_t *)wtk_malloc(sizeof(qtk_estimate_cfg_t));
	qtk_estimate_cfg_init(cfg);
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
	ret = qtk_estimate_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_estimate_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_estimate_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_estimate_cfg_delete_bin(qtk_estimate_cfg_t *cfg)
{
	qtk_estimate_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}
