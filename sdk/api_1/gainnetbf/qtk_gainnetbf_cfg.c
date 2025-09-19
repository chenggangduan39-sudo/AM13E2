#include "qtk_gainnetbf_cfg.h"


int qtk_gainnetbf_cfg_init(qtk_gainnetbf_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	cfg->gainnet_bf_cfg = NULL;
	cfg->gainnet_bf3_cfg = NULL;
	cfg->gainnet_bf4_cfg = NULL;
	cfg->rtjoin2_cfg = NULL;
	cfg->gainnet_bf6_cfg = NULL;
	cfg->mix_speech_cfg = NULL;
	cfg->skip_channels = NULL;
	cfg->use_bin = 0;
	cfg->mics = 4;
	cfg->nskip = 0;
	cfg->mic_shift=1.0f;
	cfg->echo_shift=1.0f;
	cfg->use_manual=0;
	cfg->use_gainnet_bf=0;
	cfg->use_gainnet_bf3=0;
	cfg->use_gainnet_bf4=0;
	cfg->use_rtjoin2=0;
	cfg->use_gainnet_bf6=0;
	cfg->use_mix_speech=0;
	cfg->theta = 90.0f;
	cfg->phi = 0.0f;
	cfg->max_extp = 1;
	cfg->online_tms = 500;
	cfg->theta_range = 5;
	cfg->continue_count = 1;
	cfg->energy_sum = 1000.0f;
	cfg->zero_sum = 2000.0f;
	cfg->energy_thr = 10000.0f;
	cfg->energy_thr_time = 1;
	cfg->use_min_sil = 0;
	cfg->use_energy_debug = 0;
	cfg->energy_thr_count = 2;
	cfg->use_log_wav = 0;
	cfg->left_count = 2;
	cfg->input_fn="./mic.wav";
	cfg->out_fn="./echo.wav";
	cfg->use_ssl_filter = 0;
	cfg->use_ssl_delay = -1;
	cfg->use_cachebuf = 0;
	cfg->cache_len = 128*32;
	cfg->join_channel = 4;
	cfg->main_channel = 0;
	cfg->mic_scale = 1.0;
	return 0;
}
int qtk_gainnetbf_cfg_clean(qtk_gainnetbf_cfg_t *cfg)
{
	// if(cfg->use_gainnet_bf)
	// {
	// 	if(cfg->gainnet_bf_cfg){
	// 		cfg->use_bin ? wtk_gainnet_bf_cfg_delete_bin(cfg->gainnet_bf_cfg) : wtk_gainnet_bf_cfg_delete(cfg->gainnet_bf_cfg);
	// 	}
	// }

	if(cfg->use_gainnet_bf3)
	{
		if(cfg->gainnet_bf3_cfg){
			cfg->use_bin ? wtk_gainnet_bf3_cfg_delete_bin(cfg->gainnet_bf3_cfg) : wtk_gainnet_bf3_cfg_delete(cfg->gainnet_bf3_cfg);
		}
	}

	if(cfg->use_gainnet_bf4)
	{
		if(cfg->gainnet_bf4_cfg){
			cfg->use_bin ? wtk_gainnet_bf4_cfg_delete_bin(cfg->gainnet_bf4_cfg) : wtk_gainnet_bf4_cfg_delete(cfg->gainnet_bf4_cfg);
		}
	}
	if(cfg->use_rtjoin2)
	{
		if(cfg->rtjoin2_cfg){
			cfg->use_bin ? wtk_rtjoin2_cfg_delete_bin(cfg->rtjoin2_cfg) : wtk_rtjoin2_cfg_delete(cfg->rtjoin2_cfg);
		}
	}
	if(cfg->use_gainnet_bf6)
	{
		if(cfg->gainnet_bf6_cfg){
			cfg->use_bin ? wtk_gainnet_bf6_cfg_delete_bin(cfg->gainnet_bf6_cfg) : wtk_gainnet_bf6_cfg_delete(cfg->gainnet_bf6_cfg);
		}
	}

	if(cfg->use_mix_speech)
	{
		if(cfg->mix_speech_cfg)
		{
			cfg->use_bin ? wtk_mix_speech_cfg_delete_bin(cfg->mix_speech_cfg) : wtk_mix_speech_cfg_delete(cfg->mix_speech_cfg);
		}
	}

	if(cfg->skip_channels)
	{
		wtk_free(cfg->skip_channels);
	}
	return 0;
}
int qtk_gainnetbf_cfg_update_local(qtk_gainnetbf_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;
	wtk_array_t *a;
	int i;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	if(cfg->cfg_fn.len > 0)
	{
		printf("gainnet_denoise cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	}
	wtk_local_cfg_update_cfg_str(lc, cfg, input_fn, v);
	wtk_local_cfg_update_cfg_str(lc, cfg, out_fn, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet_bf, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet_bf3, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet_bf4, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_rtjoin2, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_gainnet_bf6, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_mix_speech, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_min_sil, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_energy_debug, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_log_wav, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_ssl_filter, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_cachebuf, v);

	wtk_local_cfg_update_cfg_f(lc, cfg, mic_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, energy_sum, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, zero_sum, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, energy_thr, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_scale, v);

	wtk_local_cfg_update_cfg_f(lc, cfg, theta, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, phi, v);

	wtk_local_cfg_update_cfg_i(lc, cfg, energy_thr_count, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, energy_thr_time, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, max_extp, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, online_tms, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_range, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, continue_count, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, left_count, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_ssl_delay, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, cache_len, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, join_channel, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, main_channel, v);
	
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
int qtk_gainnetbf_cfg_update(qtk_gainnetbf_cfg_t *cfg)
{
	int ret = -1;

	// if(cfg->use_gainnet_bf)
	// {
	// 	if(cfg->cfg_fn.len > 0){
	// 		cfg->gainnet_bf_cfg = cfg->use_bin ? wtk_gainnet_bf_cfg_new_bin(cfg->cfg_fn.data) : wtk_gainnet_bf_cfg_new(cfg->cfg_fn.data);
	// 		if(cfg->gainnet_bf_cfg){
	// 			cfg->gainnet_bf_cfg->ssl2.max_extp = cfg->max_extp;
	// 			cfg->gainnet_bf_cfg->ssl2.online_tms = cfg->online_tms;
	// 			ret = 0;
	// 		}
	// 	}
	// }

	if(cfg->use_gainnet_bf3)
	{
		if(cfg->cfg_fn.len > 0){
			cfg->gainnet_bf3_cfg = cfg->use_bin ? wtk_gainnet_bf3_cfg_new_bin(cfg->cfg_fn.data) : wtk_gainnet_bf3_cfg_new(cfg->cfg_fn.data);
			if(cfg->gainnet_bf3_cfg){
				// cfg->gainnet_bf3_cfg->ssl2.max_extp = cfg->max_extp;
				// cfg->gainnet_bf3_cfg->ssl2.online_tms = cfg->online_tms;
				ret = 0;
			}
		}
	}

	if(cfg->use_gainnet_bf4)
	{
		if(cfg->cfg_fn.len > 0){
			cfg->gainnet_bf4_cfg = cfg->use_bin ? wtk_gainnet_bf4_cfg_new_bin(cfg->cfg_fn.data) : wtk_gainnet_bf4_cfg_new(cfg->cfg_fn.data);
			if(cfg->gainnet_bf4_cfg){
				ret = 0;
			}
		}
	}
	if(cfg->use_rtjoin2)
	{
		wtk_debug("--------------------------------------------\n");
		if(cfg->cfg_fn.len > 0){
			// wtk_debug("-------------------------------------------->cfg_fn=%s\n",cfg->cfg_fn.da);
			wtk_debug("------------------------------------cfg->use_bin =  %d\n",cfg->use_bin);
			cfg->rtjoin2_cfg = cfg->use_bin ? wtk_rtjoin2_cfg_new_bin(cfg->cfg_fn.data) : wtk_rtjoin2_cfg_new(cfg->cfg_fn.data);
			if(cfg->rtjoin2_cfg){
				ret = 0;
			}
		}
	}
	if(cfg->use_gainnet_bf6)
	{
		if(cfg->cfg_fn.len > 0){
			cfg->gainnet_bf6_cfg = cfg->use_bin ? wtk_gainnet_bf6_cfg_new_bin(cfg->cfg_fn.data) : wtk_gainnet_bf6_cfg_new(cfg->cfg_fn.data);
			if(cfg->gainnet_bf6_cfg)
			{
				ret = 0;
			}
		}
	}

	if(cfg->use_mix_speech)
	{
		if(cfg->cfg_fn.len > 0)
		{
			cfg->mix_speech_cfg = cfg->use_bin ? wtk_mix_speech_cfg_new_bin(cfg->cfg_fn.data) : wtk_mix_speech_cfg_new(cfg->cfg_fn.data);
			if(cfg->mix_speech_cfg)
			{
				ret=0;
			}
		}
	}

	return ret;
}
int qtk_gainnetbf_cfg_update2(qtk_gainnetbf_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_gainnetbf_cfg_update(cfg);
}

qtk_gainnetbf_cfg_t *qtk_gainnetbf_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_gainnetbf_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_gainnetbf_cfg, fn);
	if(!main_cfg){
		return NULL;
	}

	cfg = (qtk_gainnetbf_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_gainnetbf_cfg_delete(qtk_gainnetbf_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_gainnetbf_cfg_t *qtk_gainnetbf_cfg_new_bin(char *bin_fn)
{
	qtk_gainnetbf_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_gainnetbf_cfg_t *)wtk_malloc(sizeof(qtk_gainnetbf_cfg_t));
	qtk_gainnetbf_cfg_init(cfg);
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
	ret = qtk_gainnetbf_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_gainnetbf_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_gainnetbf_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_gainnetbf_cfg_delete_bin(qtk_gainnetbf_cfg_t *cfg)
{
	qtk_gainnetbf_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}
