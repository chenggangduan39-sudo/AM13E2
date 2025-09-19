#include "qtk_eqform_cfg.h"


int qtk_eqform_cfg_init(qtk_eqform_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	cfg->eqform_cfg = NULL;
	cfg->mic_pos = NULL;
	cfg->nmic = 0;
	cfg->use_manual = 0;
	cfg->fix_theta = 180.0;
	cfg->phi = 0;
	cfg->theta_range = -1;
	cfg->noise_suppress = 100000;
	cfg->use_bin = 0;
	cfg->mics = 4;
	cfg->mic_shift = 1.0;
	cfg->echo_shit = 1.0;
	return 0;
}
int qtk_eqform_cfg_clean(qtk_eqform_cfg_t *cfg)
{
	if(cfg->eqform_cfg){
		cfg->use_bin ? wtk_eqform_cfg_delete_bin(cfg->eqform_cfg) : wtk_eqform_cfg_delete(cfg->eqform_cfg);
	}
	return 0;
}
int qtk_eqform_cfg_update_local(qtk_eqform_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	printf("qform cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	wtk_local_cfg_update_cfg_i(lc, cfg, mics, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, fix_theta, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, phi, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, theta_range, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, noise_suppress, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shit, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);

	lc=wtk_local_cfg_find_lc_s(main,"mic");
	if(lc)
	{
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

float ** qtk_eqform_cfg_set_mic(qtk_eqform_cfg_t *cfg, float **dmic, int dc)
{
	int i,j;

	if(cfg->nmic == dc)
	{
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

int qtk_eqform_cfg_update(qtk_eqform_cfg_t *cfg)
{
	int ret = -1;	
	if(cfg->cfg_fn.len > 0){
		cfg->eqform_cfg = cfg->use_bin ? wtk_eqform_cfg_new_bin(cfg->cfg_fn.data) : wtk_eqform_cfg_new(cfg->cfg_fn.data);
		if(cfg->eqform_cfg){
			if(cfg->eqform_cfg->use_qform3)
			{
				if(cfg->theta_range > 0)
				{					
					cfg->eqform_cfg->qform3.theta_range = cfg->theta_range;
				}
				if(cfg->noise_suppress != 100000)
				{
					cfg->eqform_cfg->qform3.qmmse.noise_suppress = cfg->noise_suppress;
				}
			}
			if(cfg->eqform_cfg->use_qform9)
			{
				if(cfg->theta_range > 0)
				{
					cfg->eqform_cfg->qform9.theta_range = cfg->theta_range;
				}
				if(cfg->noise_suppress != 100000)
				{
					cfg->eqform_cfg->qform9.qmmse.noise_suppress = cfg->noise_suppress;
				}
			}
			if(cfg->eqform_cfg->use_qform11)
			{
				if(cfg->theta_range > 0)
				{
					cfg->eqform_cfg->qform11.theta_range = cfg->theta_range;
				}
				if(cfg->noise_suppress != 100000)
				{
					cfg->eqform_cfg->qform11.qmmse.noise_suppress = cfg->noise_suppress;
				}
			}
			ret = 0;
		}
	}
	return ret;
}

int qtk_eqform_cfg_update2(qtk_eqform_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_eqform_cfg_update(cfg);
}

qtk_eqform_cfg_t *qtk_eqform_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_eqform_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_eqform_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_eqform_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_eqform_cfg_delete(qtk_eqform_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_eqform_cfg_t *qtk_eqform_cfg_new_bin(char *bin_fn)
{
	qtk_eqform_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_eqform_cfg_t *)wtk_malloc(sizeof(qtk_eqform_cfg_t));
	qtk_eqform_cfg_init(cfg);
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
	ret = qtk_eqform_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_eqform_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_eqform_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_eqform_cfg_delete_bin(qtk_eqform_cfg_t *cfg)
{
	qtk_eqform_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}
