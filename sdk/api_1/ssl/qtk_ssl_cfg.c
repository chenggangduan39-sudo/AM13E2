#include "qtk_ssl_cfg.h"


int qtk_ssl_cfg_init(qtk_ssl_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	cfg->skip_channels = NULL;
	cfg->ssl_cfg = NULL;
	cfg->gainnet_ssl_cfg = NULL;
	cfg->use_bin = 0;
	cfg->mics = 4;
	cfg->nskip = 0;
	cfg->mic_shift=1.0f;
	cfg->echo_shift=1.0f;
	cfg->use_manual=0;
	cfg->continue_time=500;//ms
	cfg->use_ssl = 0;
	cfg->use_gainnet_ssl = 0;
	cfg->max_extp = -1;
	cfg->online_tms = -1;
	cfg->online_frame_step = -1;
	cfg->theta_step = -1;
	cfg->theta_range = 5;
	cfg->energy_sum = 0.75;
	cfg->specsum_fs = -1;
	cfg->specsum_fe = -1;
	cfg->use_maskssl = -1;
	cfg->use_maskssl2 = -1;
	cfg->use_echoenable = -1;

	return 0;
}
int qtk_ssl_cfg_clean(qtk_ssl_cfg_t *cfg)
{
	if(cfg->ssl_cfg){
		cfg->use_bin ? wtk_ssl_cfg_delete_bin(cfg->ssl_cfg) : wtk_ssl_cfg_delete(cfg->ssl_cfg);
	}
	if(cfg->gainnet_ssl_cfg){
		cfg->use_bin ? wtk_gainnet_ssl_cfg_delete_bin(cfg->gainnet_ssl_cfg) : wtk_gainnet_ssl_cfg_delete(cfg->gainnet_ssl_cfg);
	}
	if(cfg->skip_channels)
	{
		wtk_free(cfg->skip_channels);
	}
	return 0;
}
int qtk_ssl_cfg_update_local(qtk_ssl_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;
	wtk_array_t *a;
	int i;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	printf("ssl cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_ssl, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet_ssl, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, energy_sum, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, continue_time, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, max_extp, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, online_tms, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, online_frame_step, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_range, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_step, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, specsum_fs, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, specsum_fe, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_maskssl, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_maskssl2, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_echoenable, v);

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
int qtk_ssl_cfg_update(qtk_ssl_cfg_t *cfg)
{
	int ret = -1;

	if(cfg->use_ssl && cfg->cfg_fn.len > 0){
		cfg->ssl_cfg = cfg->use_bin ? wtk_ssl_cfg_new_bin(cfg->cfg_fn.data) : wtk_ssl_cfg_new(cfg->cfg_fn.data);
		if(cfg->ssl_cfg){
			cfg->ssl_cfg->notify_time = cfg->continue_time;
			cfg->ssl_cfg->min_energy = cfg->energy_sum;
			ret = 0;
		}
	}
	if(cfg->use_gainnet_ssl && cfg->cfg_fn.len > 0){
		cfg->gainnet_ssl_cfg = cfg->use_bin ? wtk_gainnet_ssl_cfg_new_bin(cfg->cfg_fn.data) : wtk_gainnet_ssl_cfg_new(cfg->cfg_fn.data);
		if(cfg->gainnet_ssl_cfg){
			if(cfg->use_maskssl != -1)
			{
				cfg->gainnet_ssl_cfg->use_maskssl = cfg->use_maskssl;
				wtk_debug("maskssl=%d\n",cfg->gainnet_ssl_cfg->use_maskssl);
			}
			if(cfg->use_maskssl2 != -1)
			{
				cfg->gainnet_ssl_cfg->use_maskssl2 = cfg->use_maskssl2;
				wtk_debug("maskssl2=%d\n",cfg->gainnet_ssl_cfg->use_maskssl2);
			}
			if(cfg->use_maskssl2 == -1 && cfg->use_maskssl == -1)
			{
				if(cfg->gainnet_ssl_cfg->use_maskssl)
				{
					cfg->max_extp=cfg->gainnet_ssl_cfg->maskssl.max_extp;
				}
				if(cfg->gainnet_ssl_cfg->use_maskssl2)
				{
					cfg->max_extp=cfg->gainnet_ssl_cfg->maskssl2.max_extp;
				}
			}

			if(cfg->gainnet_ssl_cfg->use_maskssl)
			{
				if(cfg->max_extp != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl.max_extp = cfg->max_extp;
				}else{
					cfg->max_extp=cfg->gainnet_ssl_cfg->maskssl.max_extp;
				}
				if(cfg->online_tms != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl.online_tms = cfg->online_tms;
					cfg->gainnet_ssl_cfg->maskssl.online_frame = floor(cfg->online_tms*1.0/1000*cfg->gainnet_ssl_cfg->rate/(cfg->gainnet_ssl_cfg->wins/2));
				}
				if(cfg->theta_step != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl.theta_step = cfg->theta_step;
				}
				
				if(cfg->specsum_fs != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl.specsum_ns=floor(cfg->specsum_fs/(cfg->gainnet_ssl_cfg->maskssl.rate*1.0/cfg->gainnet_ssl_cfg->maskssl.wins));
					cfg->gainnet_ssl_cfg->maskssl.specsum_ns=max(1, cfg->gainnet_ssl_cfg->maskssl.specsum_ns);
				}

				if(cfg->specsum_fe != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl.specsum_ne=floor(cfg->specsum_fe/(cfg->gainnet_ssl_cfg->maskssl.rate*1.0/cfg->gainnet_ssl_cfg->maskssl.wins));
					cfg->gainnet_ssl_cfg->maskssl.specsum_ne=min(cfg->gainnet_ssl_cfg->maskssl.wins/2-1, cfg->gainnet_ssl_cfg->maskssl.specsum_ne);
				}
			}
			if(cfg->gainnet_ssl_cfg->use_maskssl2)
			{
				if(cfg->max_extp != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl2.max_extp = cfg->max_extp;
				}else{
					cfg->max_extp=cfg->gainnet_ssl_cfg->maskssl2.max_extp;
				}
				if(cfg->online_tms != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl2.online_frame = floor(cfg->online_tms*1.0/1000*cfg->gainnet_ssl_cfg->rate/(cfg->gainnet_ssl_cfg->wins/2));
				}
				if(cfg->theta_step != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl2.theta_step = cfg->theta_step;
				}
				if(cfg->online_frame_step != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl2.online_frame_step = cfg->online_frame_step;
				}
				if(cfg->specsum_fs != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl2.specsum_ns=floor(cfg->specsum_fs/(cfg->gainnet_ssl_cfg->maskssl2.rate*1.0/cfg->gainnet_ssl_cfg->maskssl2.wins));
					cfg->gainnet_ssl_cfg->maskssl2.specsum_ns=max(1, cfg->gainnet_ssl_cfg->maskssl2.specsum_ns);
				}

				if(cfg->specsum_fe != -1)
				{
					cfg->gainnet_ssl_cfg->maskssl2.specsum_ne=floor(cfg->specsum_fe/(cfg->gainnet_ssl_cfg->maskssl2.rate*1.0/cfg->gainnet_ssl_cfg->maskssl2.wins));
					cfg->gainnet_ssl_cfg->maskssl2.specsum_ne=min(cfg->gainnet_ssl_cfg->maskssl2.wins/2-1, cfg->gainnet_ssl_cfg->maskssl2.specsum_ne);
				}
			}
			ret = 0;
		}
	}
	return ret;
}
int qtk_ssl_cfg_update2(qtk_ssl_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_ssl_cfg_update(cfg);
}

qtk_ssl_cfg_t *qtk_ssl_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_ssl_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_ssl_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_ssl_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_ssl_cfg_delete(qtk_ssl_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_ssl_cfg_t *qtk_ssl_cfg_new_bin(char *bin_fn)
{
	qtk_ssl_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_ssl_cfg_t *)wtk_malloc(sizeof(qtk_ssl_cfg_t));
	qtk_ssl_cfg_init(cfg);
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
	ret = qtk_ssl_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_ssl_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_ssl_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_ssl_cfg_delete_bin(qtk_ssl_cfg_t *cfg)
{
	qtk_ssl_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}
