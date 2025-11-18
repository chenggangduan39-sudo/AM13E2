#include "qtk_soundscreen_cfg.h"

int qtk_soundscreen_cfg_init(qtk_soundscreen_cfg_t *cfg)
{
    wtk_string_set(&cfg->cfg_fn, 0, 0);
	wtk_string_set(&cfg->cfg2_fn, 0, 0);
	wtk_string_set(&cfg->cfg3_fn, 0, 0);
	wtk_string_set(&cfg->cfg4_fn, 0, 0);
	wtk_string_set(&cfg->aec_fn, 0, 0);
	wtk_string_set(&cfg->vbox_fn, 0, 0);	
    cfg->qform9_cfg = NULL;
	cfg->qform9_cfg2 = NULL;
	cfg->beamnet2_cfg = NULL;
	cfg->beamnet3_cfg = NULL;
	cfg->beamnet4_cfg = NULL;
	cfg->beamnet4_cfg2 = NULL;
	cfg->beamnet4_cfg3 = NULL;
	cfg->beamnet4_cfg4 = NULL;
	cfg->cmask_bfse_cfg = NULL;
	cfg->aec_cfg = NULL;
	cfg->vboxebf3_cfg = NULL;
    cfg->main_cfg = NULL;
    cfg->rbin = NULL;
    cfg->cfile = NULL;
	cfg->thetas = NULL;
	cfg->mic_pos = NULL;
    cfg->use_bin = 0;
	cfg->theta_range = -1;
	cfg->noise_suppress = 100000.0f;
	cfg->use_manual = 0;
	cfg->nmic = 0;
	cfg->use_center = 0;
	cfg->use_aec = 0;
	cfg->use_aec_bin = 0;
	cfg->use_vboxebf = 0;
	cfg->use_vbox_bin = 0;
	cfg->theta_range_class1 = -1;
	cfg->theta_range_class2 = -1;
	cfg->use_qform9 = 1;
	cfg->use_beamnet2 = 0;
	cfg->use_beamnet3 = 0;
	cfg->use_beamnet4 = 0;
	cfg->use_cmask_bfse = 0;
	return 0;
}

int qtk_soundscreen_cfg_clean(qtk_soundscreen_cfg_t *cfg)
{
	if(cfg->qform9_cfg){
		cfg->use_bin ? wtk_qform9_cfg_delete_bin(cfg->qform9_cfg): wtk_qform9_cfg_delete(cfg->qform9_cfg);
	}
	if(cfg->qform9_cfg2){
		cfg->use_bin ? wtk_qform9_cfg_delete_bin(cfg->qform9_cfg2): wtk_qform9_cfg_delete(cfg->qform9_cfg2);
	}
	if(cfg->beamnet2_cfg){
		cfg->use_bin ? wtk_beamnet2_cfg_delete_bin(cfg->beamnet2_cfg) : wtk_beamnet2_cfg_delete(cfg->beamnet2_cfg);
	}
	if(cfg->beamnet3_cfg){
		cfg->use_bin ? wtk_beamnet3_cfg_delete_bin(cfg->beamnet3_cfg) : wtk_beamnet3_cfg_delete(cfg->beamnet3_cfg);
	}
	if(cfg->beamnet4_cfg){
		cfg->use_bin ? wtk_beamnet4_cfg_delete_bin(cfg->beamnet4_cfg) : wtk_beamnet4_cfg_delete(cfg->beamnet4_cfg);
	}
	if(cfg->beamnet4_cfg2){
		cfg->use_bin ? wtk_beamnet4_cfg_delete_bin(cfg->beamnet4_cfg2) : wtk_beamnet4_cfg_delete(cfg->beamnet4_cfg2);
	}
	if(cfg->beamnet4_cfg3){
		cfg->use_bin ? wtk_beamnet4_cfg_delete_bin(cfg->beamnet4_cfg3) : wtk_beamnet4_cfg_delete(cfg->beamnet4_cfg3);
	}
	if(cfg->beamnet4_cfg4){
		cfg->use_bin ? wtk_beamnet4_cfg_delete_bin(cfg->beamnet4_cfg4) : wtk_beamnet4_cfg_delete(cfg->beamnet4_cfg4);
	}
	if(cfg->cmask_bfse_cfg){
		cfg->use_bin ? wtk_cmask_bfse_cfg_delete_bin(cfg->cmask_bfse_cfg) : wtk_cmask_bfse_cfg_delete(cfg->cmask_bfse_cfg);
	}
	if(cfg->aec_cfg){
		cfg->use_aec_bin ? wtk_aec_cfg_delete_bin(cfg->aec_cfg) : wtk_aec_cfg_delete(cfg->aec_cfg);
	}
	if(cfg->vboxebf3_cfg){
		cfg->use_vbox_bin ? wtk_vboxebf3_cfg_delete_bin(cfg->vboxebf3_cfg) : wtk_vboxebf3_cfg_delete(cfg->vboxebf3_cfg);
	}
	if(cfg->thetas){
		wtk_free(cfg->thetas);
	}
	if(cfg->mic_pos){
		int i;
		for(i=0;i<cfg->nmic;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
	return 0;
}

int qtk_soundscreen_cfg_update_local(qtk_soundscreen_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_array_t *theta_array;
	wtk_string_t **v;
    wtk_string_t *v1;
	int i;

	lc = main;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v1);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg2_fn, v1);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg3_fn, v1);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg4_fn, v1);
	printf("soundscreen cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, aec_fn, v1);
	printf("aec cfg==>[%.*s]\n",cfg->aec_fn.len, cfg->aec_fn.data);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, vbox_fn, v1);
	printf("vboxebf cfg==>[%.*s]\n",cfg->vbox_fn.len, cfg->vbox_fn.data);	
    wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_center, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_aec, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_aec_bin, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_vbox_bin, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_qform9, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_cmask_bfse, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_beamnet2, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_beamnet3, v1);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_beamnet4, v1);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_range, v1);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_range_class1, v1);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_range_class2, v1);
	wtk_local_cfg_update_cfg_f(lc, cfg, noise_suppress, v1);
	theta_array = wtk_local_cfg_find_array_s(lc, "fix_theta");
	if(theta_array && theta_array->nslot > 0){
		cfg->n_theta = theta_array->nslot;
		cfg->thetas = (int *)wtk_malloc(sizeof(int) * cfg->n_theta);
		v = (wtk_string_t **)theta_array->slot;
		for(i = 0; i < cfg->n_theta; i++){
			cfg->thetas[i] = wtk_str_atoi(v[i]->data, v[i]->len);
		}
	}

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
				v1=((wtk_string_t**)item->value.array->slot)[i];
				cfg->mic_pos[cfg->nmic][i]=wtk_str_atof(v1->data,v1->len);
				// wtk_debug("v[%d][%d]=%f\n",cfg->nmic,i,cfg->mic_pos[cfg->nmic][i]);
			}
			++cfg->nmic;
		}
	}

	return 0;
}

float **qtk_soundscreen_cfg_set_mic(qtk_soundscreen_cfg_t *cfg, float **dmic, int dc)
{
	int i,j;

	if(cfg->nmic == dc){
		for(i=0;i<cfg->nmic;++i)
		{
			for(j=0;j<3;++j)
			{
				dmic[i][j]=cfg->mic_pos[i][j];
			}
		}
	}else{
		for(i=0;i<dc;++i)
		{
			wtk_free(dmic[i]);
		}
		wtk_free(dmic);
		dmic=(float**)wtk_malloc(sizeof(float*)*cfg->nmic);
		for(i=0;i<cfg->nmic;++i)
		{
			dmic[i]=(float *)wtk_malloc(sizeof(float) * 3);
		}
		for(i=0;i<cfg->nmic;++i)
		{
			for(j=0;j<3;++j)
			{
				dmic[i][j]=cfg->mic_pos[i][j];
				// wtk_debug("v[%d][%d]=%f\n",i,j,cfg->mic_pos[i][j]);
			}
		}
	}
	return dmic;
}

int qtk_soundscreen_cfg_update(qtk_soundscreen_cfg_t *cfg)
{
    int ret = -1;
	int i,j;

	if(cfg->cfg_fn.len > 0){
		if(cfg->use_qform9){
			cfg->qform9_cfg = cfg->use_bin ? wtk_qform9_cfg_new_bin(cfg->cfg_fn.data) : wtk_qform9_cfg_new(cfg->cfg_fn.data);
			if(cfg->qform9_cfg){
				if(cfg->n_theta != 2){
					cfg->use_center=0;
					cfg->qform9_cfg->use_two_channel=0;
				}else{
					if(cfg->use_center){
						cfg->qform9_cfg->use_two_channel = 1;
						if(cfg->thetas){
							cfg->qform9_cfg->theta_center_class2 = cfg->thetas[1];
							cfg->qform9_cfg->theta_center_class1 = cfg->thetas[0];
						}
						if(cfg->theta_range_class1 != -1){
							cfg->qform9_cfg->theta_range_class1 = cfg->theta_range_class1;
						}
						if(cfg->theta_range_class2 != -1){
							cfg->qform9_cfg->theta_range_class2 = cfg->theta_range_class2;
						}
						if(cfg->theta_range != -1){
							cfg->qform9_cfg->theta_range = cfg->theta_range;
						}
					}
				}
				// if(cfg->nmic > 0)
				// {
				// 	if(cfg->qform9_cfg->use_two_aspecclass)
				// 	{
				// 		cfg->qform9_cfg->aspec_class1.mic_pos=qtk_soundscreen_cfg_set_mic(cfg, cfg->qform9_cfg->aspec_class1.mic_pos, cfg->qform9_cfg->aspec_class1.channel);
				// 		cfg->qform9_cfg->aspec_class1.channel = cfg->nmic;
				// 		cfg->qform9_cfg->aspec_class2.mic_pos=qtk_soundscreen_cfg_set_mic(cfg, cfg->qform9_cfg->aspec_class2.mic_pos, cfg->qform9_cfg->aspec_class2.channel);
				// 		cfg->qform9_cfg->aspec_class2.channel = cfg->nmic;
				// 	}else
				// 	{
				// 		cfg->qform9_cfg->aspec.mic_pos=qtk_soundscreen_cfg_set_mic(cfg, cfg->qform9_cfg->aspec.mic_pos, cfg->qform9_cfg->aspec.channel);
				// 		cfg->qform9_cfg->aspec.channel = cfg->nmic;
				// 	}

				// 	cfg->qform9_cfg->bf.mic_pos = qtk_soundscreen_cfg_set_mic(cfg, cfg->qform9_cfg->bf.mic_pos, cfg->qform9_cfg->bf.nmic);
				// 	cfg->qform9_cfg->bf.nmic = cfg->nmic;

				// 	cfg->qform9_cfg->stft2.channel = cfg->nmic;
				// }

				if(cfg->use_manual){
					if(cfg->theta_range > 0 && cfg->use_center == 0){
						cfg->qform9_cfg->theta_range = cfg->theta_range;
					}
					if(cfg->noise_suppress != 100000.0f){
						cfg->qform9_cfg->qmmse.noise_suppress = cfg->noise_suppress;
					}
				}
				ret = 0;
			}
		}
		if(cfg->use_beamnet2){
			cfg->beamnet2_cfg = cfg->use_bin ? wtk_beamnet2_cfg_new_bin(cfg->cfg_fn.data) : wtk_beamnet2_cfg_new(cfg->cfg_fn.data);
			if(cfg->beamnet2_cfg){
				ret = 0;
			}
		}
		if(cfg->use_beamnet3){
			cfg->beamnet3_cfg = cfg->use_bin ? wtk_beamnet3_cfg_new_bin(cfg->cfg_fn.data) : wtk_beamnet3_cfg_new(cfg->cfg_fn.data);
			if(cfg->beamnet3_cfg){
				ret = 0;
			}
		}
		if(cfg->use_beamnet4){
			cfg->beamnet4_cfg = cfg->use_bin ? wtk_beamnet4_cfg_new_bin(cfg->cfg_fn.data) : wtk_beamnet4_cfg_new(cfg->cfg_fn.data);
			if(cfg->beamnet4_cfg){
				ret = 0;
			}

			cfg->beamnet4_cfg2 = cfg->use_bin ? wtk_beamnet4_cfg_new_bin(cfg->cfg2_fn.data) : wtk_beamnet4_cfg_new(cfg->cfg2_fn.data);
			if(cfg->beamnet4_cfg2){
				ret = 0;
			}

			cfg->beamnet4_cfg3 = cfg->use_bin ? wtk_beamnet4_cfg_new_bin(cfg->cfg3_fn.data) : wtk_beamnet4_cfg_new(cfg->cfg3_fn.data);
			if(cfg->beamnet4_cfg3){
				ret = 0;
			}

			cfg->beamnet4_cfg4 = cfg->use_bin ? wtk_beamnet4_cfg_new_bin(cfg->cfg4_fn.data) : wtk_beamnet4_cfg_new(cfg->cfg4_fn.data);
			if(cfg->beamnet4_cfg4){
				ret = 0;
			}
		}
		if(cfg->use_cmask_bfse){
			cfg->cmask_bfse_cfg = cfg->use_bin ? wtk_cmask_bfse_cfg_new_bin(cfg->cfg_fn.data) : wtk_cmask_bfse_cfg_new(cfg->cfg_fn.data);
			if(cfg->cmask_bfse_cfg){
				ret = 0;
			}
		}
	}
	
	if(cfg->cfg2_fn.len > 0){
		if(cfg->use_qform9){
			cfg->qform9_cfg2 = cfg->use_bin ? wtk_qform9_cfg_new_bin(cfg->cfg2_fn.data) : wtk_qform9_cfg_new(cfg->cfg2_fn.data);
			if(cfg->qform9_cfg2){
				if(cfg->use_manual){
					if(cfg->theta_range > 0 && cfg->use_center == 0){
						cfg->qform9_cfg2->theta_range = cfg->theta_range;
					}
					if(cfg->noise_suppress != 100000.0f){
						cfg->qform9_cfg2->qmmse.noise_suppress = cfg->noise_suppress;
					}
				}
				ret = 0;
			}
		}
	}

	if(cfg->aec_fn.len > 0 && cfg->use_aec){
		ret = -1;
		cfg->aec_cfg = cfg->use_aec_bin ? wtk_aec_cfg_new_bin(cfg->aec_fn.data) : wtk_aec_cfg_new(cfg->aec_fn.data);
		if(cfg->aec_cfg){
			ret = 0;
		}
	}
	if(cfg->vbox_fn.len > 0 && cfg->use_vboxebf){
		ret=-1;
		cfg->vboxebf3_cfg = cfg->use_vbox_bin ? wtk_vboxebf3_cfg_new_bin(cfg->vbox_fn.data) : wtk_vboxebf3_cfg_new(cfg->vbox_fn.data);
		if(cfg->vboxebf3_cfg){ret=0;}
	}
	return ret;
}

int qtk_soundscreen_cfg_update2(qtk_soundscreen_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_soundscreen_cfg_update(cfg);
}

qtk_soundscreen_cfg_t *qtk_soundscreen_cfg_new(char *fn)
{
    wtk_main_cfg_t *main_cfg;
	qtk_soundscreen_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_soundscreen_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_soundscreen_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_soundscreen_cfg_delete(qtk_soundscreen_cfg_t *cfg)
{
    wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_soundscreen_cfg_t *qtk_soundscreen_cfg_new_bin(char *bin_fn)
{
	qtk_soundscreen_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_soundscreen_cfg_t *)wtk_malloc(sizeof(qtk_soundscreen_cfg_t));
	qtk_soundscreen_cfg_init(cfg);
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
	ret = qtk_soundscreen_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_soundscreen_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_soundscreen_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_soundscreen_cfg_delete_bin(qtk_soundscreen_cfg_t *cfg)
{
	qtk_soundscreen_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}