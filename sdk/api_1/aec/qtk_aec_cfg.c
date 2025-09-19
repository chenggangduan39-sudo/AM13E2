#include "qtk_aec_cfg.h"


int qtk_aec_cfg_init(qtk_aec_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	wtk_string_set(&cfg->cache_fn, 0, 0);
	cfg->aec_cfg = NULL;
	cfg->caec_cfg = NULL;
	cfg->caec2_cfg = NULL;
	cfg->noise_suppress = 1000000.0f;
	cfg->use_manual = 0;
	cfg->use_bin = 0;
	cfg->mic_pos = NULL;
	cfg->nmic = 0;
	cfg->channel = 10;
	cfg->spk_channel = 2;
	cfg->mic_shift = 1.0f;
	cfg->echo_shift = 1.0f;
	cfg->spk_shift = 1.0f;
	cfg->use_cmask_aec = 0;
	cfg->use_cmask_aec2 = 0;
	cfg->use_log_wav = 0;
	cfg->energy_sum = 0.0;
	cfg->use_cache_mode = 0;
	return 0;
}
int qtk_aec_cfg_clean(qtk_aec_cfg_t *cfg)
{
	if(cfg->use_cmask_aec){
		if(cfg->caec_cfg){
			cfg->use_bin ? wtk_cmask_aec_cfg_delete_bin(cfg->caec_cfg) : wtk_cmask_aec_cfg_delete(cfg->caec_cfg);
		}
	}else if(cfg->use_cmask_aec2){
		if(cfg->caec2_cfg){
			cfg->use_bin ? wtk_cmask_aec2_cfg_delete_bin(cfg->caec2_cfg) : wtk_cmask_aec2_cfg_delete(cfg->caec2_cfg);
		}
	}else{
		if(cfg->aec_cfg){
			cfg->use_bin ? wtk_aec_cfg_delete_bin(cfg->aec_cfg) : wtk_aec_cfg_delete(cfg->aec_cfg);
		}
	}
	if(cfg->mic_pos)
	{
		int i;
		for(i=0;i<cfg->channel;++i)
		{
			wtk_free(cfg->mic_pos[i]);
		}
		wtk_free(cfg->mic_pos);
	}
	return 0;
}
int qtk_aec_cfg_update_local(qtk_aec_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	printf("aec cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, cache_fn, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, noise_suppress, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, spk_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, energy_sum, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_cmask_aec, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_cmask_aec2, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_log_wav, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_cache_mode, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, spk_channel, v);

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

float ** qtk_aec_cfg_set_mic(qtk_aec_cfg_t *cfg, float **dmic, int dc)
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
			}
		}
	}
	return dmic;
}

int qtk_aec_cfg_update(qtk_aec_cfg_t *cfg)
{
	int ret = -1;
	int i,j;
	if(cfg->cfg_fn.len > 0){
		if(cfg->use_cmask_aec){
			cfg->caec_cfg = cfg->use_bin ? wtk_cmask_aec_cfg_new_bin(cfg->cfg_fn.data) : wtk_cmask_aec_cfg_new(cfg->cfg_fn.data);
			if(cfg->caec_cfg){
				if(cfg->use_manual){
					if(cfg->caec_cfg->use_qmmse && cfg->noise_suppress != 1000000){
						cfg->caec_cfg->qmmse.noise_suppress = cfg->noise_suppress;
					}
				}
				ret = 0;
			}
		}else if(cfg->use_cmask_aec2){
			cfg->caec2_cfg = cfg->use_bin ? wtk_cmask_aec2_cfg_new_bin(cfg->cfg_fn.data) : wtk_cmask_aec2_cfg_new(cfg->cfg_fn.data);
			if(cfg->caec2_cfg){
				if(cfg->use_manual){
					if(cfg->caec2_cfg->use_qmmse && cfg->noise_suppress != 1000000){
						cfg->caec2_cfg->qmmse.noise_suppress = cfg->noise_suppress;
					}
				}
				ret = 0;
			}
		}else{
			cfg->aec_cfg = cfg->use_bin ? wtk_aec_cfg_new_bin(cfg->cfg_fn.data) : wtk_aec_cfg_new(cfg->cfg_fn.data);
			if(cfg->aec_cfg){
				if(cfg->nmic > 0){
					if(cfg->channel > 0){
						cfg->aec_cfg->stft.channel = cfg->channel;
						cfg->aec_cfg->spchannel = cfg->spk_channel;
						wtk_aec_cfg_update(&(cfg->aec_cfg));
					}
				}

				if(cfg->use_manual){
					if(cfg->aec_cfg->use_post && cfg->noise_suppress != 1000000){
						cfg->aec_cfg->qmmse.noise_suppress = cfg->noise_suppress;
					}
				}
				ret = 0;
			}
		}
	}
	return ret;
}
int qtk_aec_cfg_update2(qtk_aec_cfg_t *cfg, wtk_source_loader_t *sl)
{
	qtk_aec_cfg_update(cfg);
	return 0;
}

qtk_aec_cfg_t *qtk_aec_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_aec_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_aec_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_aec_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_aec_cfg_delete(qtk_aec_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_aec_cfg_t *qtk_aec_cfg_new_bin(char *bin_fn)
{
	qtk_aec_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_aec_cfg_t *)wtk_malloc(sizeof(qtk_aec_cfg_t));
	qtk_aec_cfg_init(cfg);
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
	ret = qtk_aec_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_aec_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_aec_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_aec_cfg_delete_bin(qtk_aec_cfg_t *cfg)
{
	qtk_aec_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}
