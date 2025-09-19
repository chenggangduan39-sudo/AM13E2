#include "qtk_bfio_cfg.h"


int qtk_bfio_cfg_init(qtk_bfio_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	wtk_string_set(&cfg->vboxebf_fn, 0, 0);
	wtk_string_set(&cfg->soundscreen_fn, 0, 0);
	wtk_string_set(&cfg->wake_word, 0, 0);
	cfg->qform_cfg = NULL;
	cfg->bfio5_cfg = NULL;
	cfg->mic_pos = NULL;
	cfg->nmic = 0;
	cfg->use_manual = 0;
	cfg->fix_theta = 180.0;
	cfg->phi = 0;
	cfg->theta_range = -1;
	cfg->noise_suppress = -1;
	cfg->use_bin = 0;
	cfg->mics = 4;
	cfg->mic_shift = 1.0;
	cfg->echo_shift = 1.0;
	cfg->low_thresh = -1.0;
	cfg->use_bfio = 1;
	cfg->use_bfio5 = 0;
	cfg->use_soundscreen = 1;
	cfg->use_vboxebf = 0;
	return 0;
}
int qtk_bfio_cfg_clean(qtk_bfio_cfg_t *cfg)
{
	if(cfg->qform_cfg){
		cfg->use_bin ? wtk_bfio_cfg_delete_bin(cfg->qform_cfg) : wtk_bfio_cfg_delete(cfg->qform_cfg);
	}

	if(cfg->bfio5_cfg){
		cfg->use_bin ? wtk_bfio5_cfg_delete_bin(cfg->bfio5_cfg) : wtk_bfio5_cfg_delete(cfg->bfio5_cfg);
	}

	return 0;
}
int qtk_bfio_cfg_update_local(qtk_bfio_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	printf("qform cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, wake_word, v);
	printf("=====>>>wake_word=[%.*s]\n",cfg->wake_word.len, cfg->wake_word.data);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, soundscreen_fn, v);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, vboxebf_fn, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, mics, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, fix_theta, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, phi, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, theta_range, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, noise_suppress, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, low_thresh, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bfio, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bfio5, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_soundscreen, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf, v);

	lc=wtk_local_cfg_find_lc_s(main,"mic");
	if(lc){
		wtk_queue_node_t *qn;
		wtk_cfg_item_t *item;
		int i;

		cfg->mic_pos=(float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
		cfg->nmic=0;
		for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
		{
			item=data_offset2(qn,wtk_cfg_item_t,n);
			if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
			cfg->mic_pos[cfg->nmic]=(float*)wtk_malloc(sizeof(float)*3);
			for(i=0;i<3;++i)
			{
				v=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[cfg->nmic][i]=wtk_str_atof(v->data,v->len);
				//wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
			++cfg->nmic;
		}
	}


	return 0;
}

int qtk_bfio_cfg_update(qtk_bfio_cfg_t *cfg)
{
	int ret = -1;
	if(cfg->cfg_fn.len > 0){
		if(cfg->use_bfio){
			cfg->qform_cfg = cfg->use_bin ? wtk_bfio_cfg_new_bin(cfg->cfg_fn.data) : wtk_bfio_cfg_new(cfg->cfg_fn.data);
			if(cfg->qform_cfg){
				ret = 0;
				if(cfg->low_thresh != -1.0){
					cfg->qform_cfg->low_thresh = cfg->low_thresh;
				}
				// if(cfg->wake_word.len > 0)
				// {
				// 	wtk_bfio_cfg_set_wakeword(cfg->qform_cfg, cfg->wake_word.data);
				// }
			}
		}else if(cfg->use_bfio5){
			cfg->bfio5_cfg = cfg->use_bin ? wtk_bfio5_cfg_new_bin(cfg->cfg_fn.data) : wtk_bfio5_cfg_new(cfg->cfg_fn.data);
			if(cfg->bfio5_cfg){
				ret = 0;
				if(cfg->low_thresh != -1.0){
					cfg->bfio5_cfg->low_thresh = cfg->low_thresh;
				}
				// if(cfg->wake_word.len > 0)
				// {
				// 	wtk_bfio_cfg_set_wakeword(cfg->qform_cfg, cfg->wake_word.data);
				// }
			}
		}
	}
	return ret;
}

int qtk_bfio_cfg_update2(qtk_bfio_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_bfio_cfg_update(cfg);
}

qtk_bfio_cfg_t *qtk_bfio_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_bfio_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_bfio_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_bfio_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_bfio_cfg_delete(qtk_bfio_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_bfio_cfg_t *qtk_bfio_cfg_new_bin(char *bin_fn)
{
	qtk_bfio_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_bfio_cfg_t *)wtk_malloc(sizeof(qtk_bfio_cfg_t));
	qtk_bfio_cfg_init(cfg);
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
	ret = qtk_bfio_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_bfio_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_bfio_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_bfio_cfg_delete_bin(qtk_bfio_cfg_t *cfg)
{
	qtk_bfio_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}
