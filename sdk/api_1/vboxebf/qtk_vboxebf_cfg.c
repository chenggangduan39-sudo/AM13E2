#include "qtk_vboxebf_cfg.h"

void qtk_vboxebf_cfg_printf(qtk_vboxebf_cfg_t *cfg);

int qtk_vboxebf_cfg_init(qtk_vboxebf_cfg_t *cfg)
{
	wtk_string_set(&cfg->cfg_fn, 0, 0);
	wtk_string_set(&cfg->cache_fn, 0, 0);
	wtk_string_set(&cfg->theta_fn, 0, 0);
	cfg->vebf3_cfg = NULL;
	cfg->vebf4_cfg = NULL;
	cfg->vebf6_cfg = NULL;
#ifdef USE_AHS
	cfg->ahs_cfg = NULL;
#endif
	cfg->skip_channels = NULL;
	cfg->use_bin = 0;
	cfg->mics = 4;
	// cfg->use_vboxebf = 0;
	// cfg->use_vboxebf2 = 0;
	cfg->use_vboxebf3 = 0;
	cfg->use_vboxebf4 = 0;
	cfg->use_vboxebf6 = 0;
	cfg->use_ahs = 0;
	cfg->use_mask_bf_net=0;
	// cfg->use_vboxebf4_multi = 0;
	// cfg->use_vboxebf5 = 0;
	cfg->nskip = 0;
	cfg->mic_shift=1.0f;
	cfg->spk_shift=1.0f;
	cfg->echo_shift=1.0f;
	cfg->use_manual=0;
	cfg->max_extp = -1;
	cfg->online_tms = -1;
	cfg->theta_range = 5;
	cfg->continue_count = 1;
	cfg->use_ssl_and_qform = 1;
	cfg->energy_sum = 0.0;
	cfg->zero_sum = 0;
	cfg->gbias = -1;
	cfg->use_log_wav = 0;
	cfg->channel = 2;
	cfg->spk_channel = 1;
	cfg->use_thread = 0;
	cfg->agca = -1;
	cfg->theta_step = -1;
	cfg->specsum_fs = -1;
	cfg->specsum_fe = -1;
	cfg->online_frame_step = -1;
	cfg->lf = -1;

	cfg->use_ssl_filter = 1;
	cfg->use_fftsbf = -1;
	cfg->bfmu = -1;
	cfg->echo_bfmu = -1;
	cfg->use_ssl = -1;
	cfg->use_maskssl = -1;
	cfg->use_maskssl2 = -1;

	cfg->noise_suppress = 100000;
	cfg->echo_suppress = 100000;
	cfg->echo_suppress_active = 100000;

	cfg->spenr_thresh= -100000;

	cfg->use_cnon=-1;
	cfg->sym=-100000;

	cfg->mic_volume=-1;
	cfg->agc_enable=-1;
	cfg->echo_enable=-1;
	cfg->denoise_enable=-1;
	cfg->ssl_enable = -1;
	cfg->use_erlssingle =-1;
	cfg->suppress = -1;
	cfg->agc_level = -1;
	cfg->aec_level = -1;
	cfg->ans_level = -1;

	cfg->AEC=1;
	cfg->AGC=1;
	cfg->ANS=1;
	cfg->agc_a=-1;
	cfg->mic_shift2=1.0;
	cfg->use_ssl_delay=-1;
	cfg->use_cache_mode=0;

	return 0;
}
int qtk_vboxebf_cfg_clean(qtk_vboxebf_cfg_t *cfg)
{
	if(cfg->use_ahs){
#ifdef USE_AHS
		if(cfg->ahs_cfg){
			cfg->use_bin ? qtk_ahs_cfg_delete_bin(cfg->ahs_cfg) : qtk_ahs_cfg_delete(cfg->ahs_cfg);
		}
#endif
	}else if(cfg->use_vboxebf6){
		if(cfg->vebf6_cfg){
			cfg->use_bin ? wtk_vboxebf6_cfg_delete_bin(cfg->vebf6_cfg) : wtk_vboxebf6_cfg_delete(cfg->vebf6_cfg);
		}
	}else if(cfg->use_vboxebf4){
		if(cfg->vebf4_cfg){
			cfg->use_bin ? wtk_vboxebf4_cfg_delete_bin(cfg->vebf4_cfg) : wtk_vboxebf4_cfg_delete(cfg->vebf4_cfg);
		}
	}else if(cfg->use_vboxebf3){
		if(cfg->vebf3_cfg){
			cfg->use_bin ? wtk_vboxebf3_cfg_delete_bin(cfg->vebf3_cfg) : wtk_vboxebf3_cfg_delete(cfg->vebf3_cfg);
		}
	}else if(cfg->use_mask_bf_net){
		if(cfg->mask_bf_net_cfg){
			cfg->use_bin ? wtk_mask_bf_net_cfg_delete_bin(cfg->mask_bf_net_cfg) : wtk_mask_bf_net_cfg_delete(cfg->mask_bf_net_cfg);
		}
	}

	if(cfg->skip_channels){
		wtk_free(cfg->skip_channels);
	}
	return 0;
}
int qtk_vboxebf_cfg_update_local(qtk_vboxebf_cfg_t  *cfg, wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc = main;
	wtk_array_t *a;
	int i;

	wtk_local_cfg_update_cfg_string_v(lc, cfg, cfg_fn, v);
	printf("qform cfg==>[%.*s]\n",cfg->cfg_fn.len, cfg->cfg_fn.data);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, cache_fn, v);
	wtk_local_cfg_update_cfg_string_v(lc, cfg, theta_fn, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, max_extp, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, channel, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, online_tms, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_range, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, continue_count, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, spk_channel, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, theta_step, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, online_frame_step, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, specsum_fs, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, specsum_fe, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, lf, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_ssl, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_maskssl, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_maskssl2, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_cnon, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_fftsbf, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, agc_enable, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, echo_enable, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, denoise_enable, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, ssl_enable, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_erlssingle, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, use_ssl_delay, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, aec_level, v);
	wtk_local_cfg_update_cfg_i(lc, cfg, ans_level, v);

	wtk_local_cfg_update_cfg_b(lc, cfg, use_ssl_and_qform, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_bin, v);
	// wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf, v);
	// wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf2, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf3, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf4, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf6, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_mask_bf_net, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_ahs, v);
	// wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf4_multi, v);
	// wtk_local_cfg_update_cfg_b(lc, cfg, use_vboxebf5, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_manual, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_thread, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_log_wav, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_ssl_filter, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, use_cache_mode, v);
	
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, spk_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_shift, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, energy_sum, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, zero_sum, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, gbias, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, agca, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, bfmu, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_bfmu, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, noise_suppress, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_suppress, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, echo_suppress_active, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, spenr_thresh, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, sym, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, suppress, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, mic_volume, v);
	wtk_local_cfg_update_cfg_f(lc, cfg, agc_level, v);

	qtk_vboxebf_cfg_printf(cfg);

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
		if(cfg->skip_channels){
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

void qtk_vboxebf_cfg_set_agcanc(qtk_vboxebf_cfg_t *cfg)
{
	// wtk_debug("noise_suppress=%f, echo_suppress=%f, echo_suppress_active=%f, agc_level=%f\n",cfg->noise_suppress,cfg->echo_suppress,cfg->echo_suppress_active,cfg->agc_level);
	if(cfg->use_vboxebf6){
		if(cfg->vebf6_cfg->use_qmmse){
			if(cfg->noise_suppress != 100000){
				cfg->vebf6_cfg->qmmse.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf6_cfg->qmmse.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf6_cfg->qmmse.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf6_cfg->qmmse.use_agc){
					cfg->vebf6_cfg->qmmse.agc_level = cfg->agc_level;
				}
			}
		}
		if(cfg->vebf6_cfg->use_qmmse2){
			if(cfg->noise_suppress != 100000){
				cfg->vebf6_cfg->qmmse2.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf6_cfg->qmmse2.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf6_cfg->qmmse2.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf6_cfg->qmmse2.use_agc){
					cfg->vebf6_cfg->qmmse2.agc_level = cfg->agc_level;
				}
			}
		}
		if(cfg->vebf6_cfg->use_qmmse3){
			if(cfg->noise_suppress != 100000){
				cfg->vebf6_cfg->qmmse3.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf6_cfg->qmmse3.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf6_cfg->qmmse3.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf6_cfg->qmmse3.use_agc){
					cfg->vebf6_cfg->qmmse3.agc_level = cfg->agc_level;
				}
			}
		}
		if(cfg->vebf6_cfg->use_qmmse4){
			if(cfg->noise_suppress != 100000){
				cfg->vebf6_cfg->qmmse4.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf6_cfg->qmmse4.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf6_cfg->qmmse4.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf6_cfg->qmmse4.use_agc){
					cfg->vebf6_cfg->qmmse4.agc_level = cfg->agc_level;
				}
			}
		}
		if(cfg->vebf6_cfg->use_qmmse5){
			if(cfg->noise_suppress != 100000){
				cfg->vebf6_cfg->qmmse5.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf6_cfg->qmmse5.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf6_cfg->qmmse5.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf6_cfg->qmmse5.use_agc){
					cfg->vebf6_cfg->qmmse5.agc_level = cfg->agc_level;
				}
			}
		}
	}
	if(cfg->use_vboxebf4){
		if(cfg->vebf4_cfg->use_qmmse){
			if(cfg->noise_suppress != 100000){
				cfg->vebf4_cfg->qmmse.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf4_cfg->qmmse.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf4_cfg->qmmse.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf4_cfg->qmmse.use_agc){
					cfg->vebf4_cfg->qmmse.agc_level = cfg->agc_level;
				}
			}
		}
	}
	if(cfg->use_mask_bf_net){
		if(cfg->mask_bf_net_cfg->use_qmmse){
			if(cfg->noise_suppress != 100000){
				cfg->mask_bf_net_cfg->qmmse.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->mask_bf_net_cfg->qmmse.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->mask_bf_net_cfg->qmmse.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->mask_bf_net_cfg->qmmse.use_agc){
					cfg->mask_bf_net_cfg->qmmse.agc_level = cfg->agc_level;
				}
			}
		}
	}
	if(cfg->use_vboxebf3){
		if(cfg->vebf3_cfg->use_qmmse){
			if(cfg->noise_suppress != 100000){
				cfg->vebf3_cfg->qmmse.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf3_cfg->qmmse.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf3_cfg->qmmse.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf3_cfg->qmmse.use_agc){
					cfg->vebf3_cfg->qmmse.agc_level = cfg->agc_level;
				}
			}
		}
		if(cfg->vebf3_cfg->use_qmmse2){
			if(cfg->noise_suppress != 100000){
				cfg->vebf3_cfg->qmmse2.noise_suppress=cfg->noise_suppress;
			}
			if(cfg->echo_suppress != 100000){
				cfg->vebf3_cfg->qmmse2.echo_suppress = cfg->echo_suppress;
			}
			if(cfg->echo_suppress_active != 100000){
				cfg->vebf3_cfg->qmmse2.echo_suppress_active = cfg->echo_suppress_active;
			}
			if(cfg->agc_level != -1){
				if(cfg->vebf3_cfg->qmmse2.use_agc){
					cfg->vebf3_cfg->qmmse2.agc_level = cfg->agc_level;
				}
			}
		}
	}
}

int qtk_vboxebf_cfg_update(qtk_vboxebf_cfg_t *cfg)
{
	int ret = -1;

	if(cfg->use_ahs){
		if(cfg->cfg_fn.len > 0){
#ifdef USE_AHS
			cfg->ahs_cfg = cfg->use_bin ? qtk_ahs_cfg_new_bin(cfg->cfg_fn.data) : qtk_ahs_cfg_new(cfg->cfg_fn.data);
			if(cfg->ahs_cfg){ret = 0;}
#endif
		}
	}
	if(cfg->use_vboxebf6){
		if(cfg->cfg_fn.len > 0){
			cfg->vebf6_cfg = cfg->use_bin ? wtk_vboxebf6_cfg_new_bin(cfg->cfg_fn.data) : wtk_vboxebf6_cfg_new(cfg->cfg_fn.data);
			if(cfg->vebf6_cfg){
				if(cfg->use_maskssl != -1){
					cfg->vebf6_cfg->use_maskssl = cfg->use_maskssl;
					wtk_debug("maskssl=%d\n",cfg->vebf6_cfg->use_maskssl);
				}else{
					cfg->use_maskssl = cfg->vebf6_cfg->use_maskssl;
				}
				if(cfg->use_maskssl2 != -1){
					cfg->vebf6_cfg->use_maskssl2 = cfg->use_maskssl2;
					wtk_debug("maskssl2=%d\n",cfg->vebf6_cfg->use_maskssl2);
				}else{
					cfg->use_maskssl2 = cfg->vebf6_cfg->use_maskssl2;
				}
				if(cfg->use_maskssl2 <= 0 && cfg->use_maskssl <= 0){
					cfg->max_extp=1;
				}

				if(cfg->vebf6_cfg->use_maskssl){
					if(cfg->max_extp != -1){
						cfg->vebf6_cfg->maskssl.max_extp = cfg->max_extp;
					}else{
						cfg->max_extp=cfg->vebf6_cfg->maskssl.max_extp;
					}
					if(cfg->online_tms != -1){
						cfg->vebf6_cfg->maskssl.online_tms = cfg->online_tms;
						cfg->vebf6_cfg->maskssl.online_frame = floor(cfg->online_tms*1.0/1000*cfg->vebf6_cfg->rate/(cfg->vebf6_cfg->wins/2));
					}
					if(cfg->theta_step != -1){
						cfg->vebf6_cfg->maskssl.theta_step = cfg->theta_step;
					}

					if(cfg->specsum_fs != -1){
						cfg->vebf6_cfg->maskssl.specsum_ns=floor(cfg->specsum_fs/(cfg->vebf6_cfg->maskssl.rate*1.0/cfg->vebf6_cfg->maskssl.wins));
						cfg->vebf6_cfg->maskssl.specsum_ns=max(1, cfg->vebf6_cfg->maskssl.specsum_ns);
					}

					if(cfg->specsum_fe != -1){
						cfg->vebf6_cfg->maskssl.specsum_ne=floor(cfg->specsum_fe/(cfg->vebf6_cfg->maskssl.rate*1.0/cfg->vebf6_cfg->maskssl.wins));
						cfg->vebf6_cfg->maskssl.specsum_ne=min(cfg->vebf6_cfg->maskssl.wins/2-1, cfg->vebf6_cfg->maskssl.specsum_ne);
					}
				}
				if(cfg->vebf6_cfg->use_maskssl2){
					if(cfg->max_extp != -1){
						cfg->vebf6_cfg->maskssl2.max_extp = cfg->max_extp;
					}else{
						cfg->max_extp=cfg->vebf6_cfg->maskssl2.max_extp;
					}
					if(cfg->online_tms != -1){
						cfg->vebf6_cfg->maskssl2.online_frame = floor(cfg->online_tms*1.0/1000*cfg->vebf6_cfg->rate/(cfg->vebf6_cfg->wins/2));
					}
					if(cfg->theta_step != -1){
						cfg->vebf6_cfg->maskssl2.theta_step = cfg->theta_step;
					}
					if(cfg->online_frame_step != -1){
						cfg->vebf6_cfg->maskssl2.online_frame_step = cfg->online_frame_step;
					}
					if(cfg->specsum_fs != -1){
						cfg->vebf6_cfg->maskssl2.specsum_ns=floor(cfg->specsum_fs/(cfg->vebf6_cfg->maskssl2.rate*1.0/cfg->vebf6_cfg->maskssl2.wins));
						cfg->vebf6_cfg->maskssl2.specsum_ns=max(1, cfg->vebf6_cfg->maskssl2.specsum_ns);
					}

					if(cfg->specsum_fe != -1){
						cfg->vebf6_cfg->maskssl2.specsum_ne=floor(cfg->specsum_fe/(cfg->vebf6_cfg->maskssl2.rate*1.0/cfg->vebf6_cfg->maskssl2.wins));
						cfg->vebf6_cfg->maskssl2.specsum_ne=min(cfg->vebf6_cfg->maskssl2.wins/2-1, cfg->vebf6_cfg->maskssl2.specsum_ne);
					}
				}	

				qtk_vboxebf_cfg_set_agcanc(cfg);
				if(cfg->aec_level != -1){
					
				}
				if(cfg->ans_level != -1){

				}

				if(cfg->gbias != -1){
					cfg->vebf6_cfg->gbias = cfg->gbias;
				}
				if(cfg->agca != -1){
					cfg->vebf6_cfg->agc_a = cfg->agca;
				}
				if(cfg->bfmu != -1){
					cfg->vebf6_cfg->bfmu = cfg->bfmu;
				}
				if(cfg->echo_bfmu != -1){
					cfg->vebf6_cfg->echo_bfmu = cfg->echo_bfmu;
				}
				if(cfg->use_fftsbf != -1){
					cfg->vebf6_cfg->use_fftsbf = cfg->use_fftsbf;
				}
				if(cfg->spenr_thresh != -100000){
					cfg->vebf6_cfg->spenr_thresh = cfg->spenr_thresh;
				}
				if(cfg->use_cnon != -1){
					cfg->vebf6_cfg->use_cnon=cfg->use_cnon;
				}
				if(cfg->sym != -100000){
					cfg->vebf6_cfg->sym=cfg->sym;
				}
				if(cfg->use_erlssingle != -1){
					cfg->vebf6_cfg->use_erlssingle = cfg->use_erlssingle;
				}
				if(cfg->use_ssl_delay != -1){
					cfg->vebf6_cfg->use_ssl_delay = cfg->use_ssl_delay;
				}
				ret = 0;
			}
		}
	}else if(cfg->use_vboxebf4){
		if(cfg->cfg_fn.len > 0){
			cfg->vebf4_cfg = cfg->use_bin ? wtk_vboxebf4_cfg_new_bin(cfg->cfg_fn.data) : wtk_vboxebf4_cfg_new(cfg->cfg_fn.data);
			if(cfg->vebf4_cfg){
				if(cfg->use_ssl!=-1){
					cfg->vebf4_cfg->use_ssl = cfg->use_ssl;
					wtk_debug("ssl=%d\n",cfg->vebf4_cfg->use_ssl);
				}else{
					cfg->use_ssl = cfg->vebf4_cfg->use_ssl;
				}
				if(cfg->use_maskssl != -1){
					cfg->vebf4_cfg->use_maskssl = cfg->use_maskssl;
					wtk_debug("maskssl=%d\n",cfg->vebf4_cfg->use_maskssl);
				}else{
					cfg->use_maskssl = cfg->vebf4_cfg->use_maskssl;
				}
				if(cfg->use_maskssl2 != -1){
					cfg->vebf4_cfg->use_maskssl2 = cfg->use_maskssl2;
					wtk_debug("maskssl2=%d\n",cfg->vebf4_cfg->use_maskssl2);
				}else{
					cfg->use_maskssl2 = cfg->vebf4_cfg->use_maskssl2;
				}
				if(cfg->use_maskssl2 <= 0 && cfg->use_maskssl <= 0 && cfg->use_ssl <= 0){
					cfg->max_extp=1;
				}
				if(cfg->vebf4_cfg->use_ssl){
					if(cfg->max_extp != -1){
						cfg->vebf4_cfg->ssl2.max_extp = cfg->max_extp;
					}else{
						cfg->max_extp = cfg->vebf4_cfg->ssl2.max_extp;
					}
					if(cfg->online_tms != -1){
						cfg->vebf4_cfg->ssl2.online_frame = floor(cfg->online_tms*1.0/1000*cfg->vebf4_cfg->rate/(cfg->vebf4_cfg->wins/2));
					}
					if(cfg->theta_step != -1){
						cfg->vebf4_cfg->ssl2.theta_step = cfg->theta_step;
					}
					if(cfg->specsum_fs != -1){
						cfg->vebf4_cfg->ssl2.specsum_ns=floor(cfg->specsum_fs/(cfg->vebf4_cfg->ssl2.rate*1.0/cfg->vebf4_cfg->ssl2.wins));
						cfg->vebf4_cfg->ssl2.specsum_ns=max(1, cfg->vebf4_cfg->ssl2.specsum_ns);
					}
					if(cfg->specsum_fe != -1){
						cfg->vebf4_cfg->ssl2.specsum_ne=floor(cfg->specsum_fe/(cfg->vebf4_cfg->ssl2.rate*1.0/cfg->vebf4_cfg->ssl2.wins));
						cfg->vebf4_cfg->ssl2.specsum_ne=min(cfg->vebf4_cfg->ssl2.wins/2-1, cfg->vebf4_cfg->ssl2.specsum_ne);
					}
					if(cfg->lf != -1){
						cfg->vebf4_cfg->ssl2.lf = cfg->lf;
					}
				}
				if(cfg->vebf4_cfg->use_maskssl){
					if(cfg->max_extp != -1){
						cfg->vebf4_cfg->maskssl.max_extp = cfg->max_extp;
					}else{
						cfg->max_extp = cfg->vebf4_cfg->maskssl.max_extp;
					}
					if(cfg->online_tms != -1){
						cfg->vebf4_cfg->maskssl.online_tms = cfg->online_tms;
						cfg->vebf4_cfg->maskssl.online_frame = floor(cfg->online_tms*1.0/1000*cfg->vebf4_cfg->rate/(cfg->vebf4_cfg->wins/2));
					}
					if(cfg->theta_step != -1){
						cfg->vebf4_cfg->maskssl.theta_step = cfg->theta_step;
					}
					if(cfg->specsum_fs != -1){
						cfg->vebf4_cfg->maskssl.specsum_ns=floor(cfg->specsum_fs/(cfg->vebf4_cfg->maskssl.rate*1.0/cfg->vebf4_cfg->maskssl.wins));
						cfg->vebf4_cfg->maskssl.specsum_ns=max(1, cfg->vebf4_cfg->maskssl.specsum_ns);
					}
					if(cfg->specsum_fe != -1){
						cfg->vebf4_cfg->maskssl.specsum_ne=floor(cfg->specsum_fe/(cfg->vebf4_cfg->maskssl.rate*1.0/cfg->vebf4_cfg->maskssl.wins));
						cfg->vebf4_cfg->maskssl.specsum_ne=min(cfg->vebf4_cfg->maskssl.wins/2-1, cfg->vebf4_cfg->maskssl.specsum_ne);
					}
				}
				if(cfg->vebf4_cfg->use_maskssl2){
					if(cfg->max_extp != -1){
						cfg->vebf4_cfg->maskssl2.max_extp = cfg->max_extp;
					}else{
						cfg->max_extp=cfg->vebf4_cfg->maskssl2.max_extp;
					}
					if(cfg->online_tms != -1){
						cfg->vebf4_cfg->maskssl2.online_frame = floor(cfg->online_tms*1.0/1000*cfg->vebf4_cfg->rate/(cfg->vebf4_cfg->wins/2));
					}
					if(cfg->theta_step != -1){
						cfg->vebf4_cfg->maskssl2.theta_step = cfg->theta_step;
					}
					if(cfg->online_frame_step != -1){
						cfg->vebf4_cfg->maskssl2.online_frame_step = cfg->online_frame_step;
					}
					if(cfg->specsum_fs != -1){
						cfg->vebf4_cfg->maskssl2.specsum_ns=floor(cfg->specsum_fs/(cfg->vebf4_cfg->maskssl2.rate*1.0/cfg->vebf4_cfg->maskssl2.wins));
						cfg->vebf4_cfg->maskssl2.specsum_ns=max(1, cfg->vebf4_cfg->maskssl2.specsum_ns);
					}
					if(cfg->specsum_fe != -1){
						cfg->vebf4_cfg->maskssl2.specsum_ne=floor(cfg->specsum_fe/(cfg->vebf4_cfg->maskssl2.rate*1.0/cfg->vebf4_cfg->maskssl2.wins));
						cfg->vebf4_cfg->maskssl2.specsum_ne=min(cfg->vebf4_cfg->maskssl2.wins/2-1, cfg->vebf4_cfg->maskssl2.specsum_ne);
					}
				}
				qtk_vboxebf_cfg_set_agcanc(cfg);
				if(cfg->gbias != -1){
					cfg->vebf4_cfg->gbias = cfg->gbias;
				}
				if(cfg->agca != -1){
					cfg->vebf4_cfg->agc_a = cfg->agca;
				}
				if(cfg->bfmu != -1){
					cfg->vebf4_cfg->bfmu = cfg->bfmu;
				}
				if(cfg->echo_bfmu != -1){
					cfg->vebf4_cfg->echo_bfmu = cfg->echo_bfmu;
				}
				if(cfg->use_fftsbf != -1){
					cfg->vebf4_cfg->use_fftsbf = cfg->use_fftsbf;
				}
				if(cfg->spenr_thresh != -100000){
					cfg->vebf4_cfg->spenr_thresh = cfg->spenr_thresh;
				}
				if(cfg->use_cnon != -1){
					cfg->vebf4_cfg->use_cnon=cfg->use_cnon;
				}
				if(cfg->sym != -100000){
					cfg->vebf4_cfg->sym=cfg->sym;
				}
				if(cfg->use_ssl_delay != -1){
					cfg->vebf4_cfg->use_ssl_delay = cfg->use_ssl_delay;
				}
				ret = 0;
			}
		}
	}else if(cfg->use_vboxebf3){
		if(cfg->cfg_fn.len > 0){
			cfg->vebf3_cfg = cfg->use_bin ? wtk_vboxebf3_cfg_new_bin(cfg->cfg_fn.data) : wtk_vboxebf3_cfg_new(cfg->cfg_fn.data);
			if(cfg->vebf3_cfg){
				if(cfg->use_maskssl != -1){
					cfg->vebf3_cfg->use_maskssl = cfg->use_maskssl;
					wtk_debug("maskssl=%d\n",cfg->vebf3_cfg->use_maskssl);
				}else{
					cfg->use_maskssl = cfg->vebf3_cfg->use_maskssl;
				}
				if(cfg->use_maskssl2 != -1){
					cfg->vebf3_cfg->use_maskssl2 = cfg->use_maskssl2;
					wtk_debug("maskssl2=%d\n",cfg->vebf3_cfg->use_maskssl2);
				}else{
					cfg->use_maskssl2 = cfg->vebf3_cfg->use_maskssl2;
				}
				if(cfg->use_maskssl2 <= 0 && cfg->use_maskssl <= 0){
					cfg->max_extp=1;
				}
				if(cfg->vebf3_cfg->use_maskssl){
					if(cfg->max_extp != -1){
						cfg->vebf3_cfg->maskssl.max_extp = cfg->max_extp;
					}else{
						cfg->max_extp=cfg->vebf3_cfg->maskssl.max_extp;
					}
					if(cfg->online_tms != -1){
						cfg->vebf3_cfg->maskssl.online_tms = cfg->online_tms;
						cfg->vebf3_cfg->maskssl.online_frame = floor(cfg->online_tms*1.0/1000*cfg->vebf3_cfg->rate/(cfg->vebf3_cfg->wins/2));
					}
					if(cfg->theta_step != -1){
						cfg->vebf3_cfg->maskssl.theta_step = cfg->theta_step;
					}
					if(cfg->specsum_fs != -1){
						cfg->vebf3_cfg->maskssl.specsum_ns=floor(cfg->specsum_fs/(cfg->vebf3_cfg->maskssl.rate*1.0/cfg->vebf3_cfg->maskssl.wins));
						cfg->vebf3_cfg->maskssl.specsum_ns=max(1, cfg->vebf3_cfg->maskssl.specsum_ns);
					}
					if(cfg->specsum_fe != -1){
						cfg->vebf3_cfg->maskssl.specsum_ne=floor(cfg->specsum_fe/(cfg->vebf3_cfg->maskssl.rate*1.0/cfg->vebf3_cfg->maskssl.wins));
						cfg->vebf3_cfg->maskssl.specsum_ne=min(cfg->vebf3_cfg->maskssl.wins/2-1, cfg->vebf3_cfg->maskssl.specsum_ne);
					}
				}
				if(cfg->vebf3_cfg->use_maskssl2){
					if(cfg->max_extp != -1){
						cfg->vebf3_cfg->maskssl2.max_extp = cfg->max_extp;
					}else{
						cfg->max_extp=cfg->vebf3_cfg->maskssl2.max_extp;
					}
					if(cfg->online_tms != -1){
						cfg->vebf3_cfg->maskssl2.online_frame = floor(cfg->online_tms*1.0/1000*cfg->vebf3_cfg->rate/(cfg->vebf3_cfg->wins/2));
					}
					if(cfg->theta_step != -1){
						cfg->vebf3_cfg->maskssl2.theta_step = cfg->theta_step;
					}
					if(cfg->online_frame_step != -1){
						cfg->vebf3_cfg->maskssl2.online_frame_step = cfg->online_frame_step;
					}
					if(cfg->specsum_fs != -1){
						cfg->vebf3_cfg->maskssl2.specsum_ns=floor(cfg->specsum_fs/(cfg->vebf3_cfg->maskssl2.rate*1.0/cfg->vebf3_cfg->maskssl2.wins));
						cfg->vebf3_cfg->maskssl2.specsum_ns=max(1, cfg->vebf3_cfg->maskssl2.specsum_ns);
					}
					if(cfg->specsum_fe != -1){
						cfg->vebf3_cfg->maskssl2.specsum_ne=floor(cfg->specsum_fe/(cfg->vebf3_cfg->maskssl2.rate*1.0/cfg->vebf3_cfg->maskssl2.wins));
						cfg->vebf3_cfg->maskssl2.specsum_ne=min(cfg->vebf3_cfg->maskssl2.wins/2-1, cfg->vebf3_cfg->maskssl2.specsum_ne);
					}
				}

				qtk_vboxebf_cfg_set_agcanc(cfg);
				if(cfg->aec_level != -1){
					
				}
				if(cfg->ans_level != -1){

				}
				if(cfg->gbias != -1){
					cfg->vebf3_cfg->gbias = cfg->gbias;
				}
				if(cfg->agca != -1){
					cfg->vebf3_cfg->agc_a = cfg->agca;
				}
				if(cfg->bfmu != -1){
					cfg->vebf3_cfg->bfmu = cfg->bfmu;
				}
				if(cfg->echo_bfmu != -1){
					cfg->vebf3_cfg->echo_bfmu = cfg->echo_bfmu;
				}
				if(cfg->use_fftsbf != -1){
					cfg->vebf3_cfg->use_fftsbf = cfg->use_fftsbf;
				}
				if(cfg->spenr_thresh != -100000){
					cfg->vebf3_cfg->spenr_thresh = cfg->spenr_thresh;
				}
				if(cfg->use_cnon != -1){
					cfg->vebf3_cfg->use_cnon=cfg->use_cnon;
				}
				if(cfg->sym != -100000){
					cfg->vebf3_cfg->sym=cfg->sym;
				}
				if(cfg->use_erlssingle != -1){
					cfg->vebf3_cfg->use_erlssingle = cfg->use_erlssingle;
				}
				if(cfg->use_ssl_delay != -1){
					cfg->vebf3_cfg->use_ssl_delay = cfg->use_ssl_delay;
				}
				ret = 0;
			}
		}
	}else if(cfg->use_mask_bf_net){
		if(cfg->cfg_fn.len > 0){
			cfg->mask_bf_net_cfg = cfg->use_bin ? wtk_mask_bf_net_cfg_new_bin(cfg->cfg_fn.data) : wtk_mask_bf_net_cfg_new(cfg->cfg_fn.data);
			if(cfg->mask_bf_net_cfg){

				qtk_vboxebf_cfg_set_agcanc(cfg);
				if(cfg->aec_level != -1){
					
				}
				if(cfg->ans_level != -1){

				}
				if(cfg->bfmu != -1){
					cfg->mask_bf_net_cfg->bfmu = cfg->bfmu;
				}
				if(cfg->echo_bfmu != -1){
					cfg->mask_bf_net_cfg->echo_bfmu = cfg->echo_bfmu;
				}
				if(cfg->spenr_thresh != -100000){
					cfg->mask_bf_net_cfg->spenr_thresh = cfg->spenr_thresh;
				}
				if(cfg->use_cnon != -1){
					cfg->mask_bf_net_cfg->use_cnon=cfg->use_cnon;
				}
				if(cfg->sym != -100000){
					cfg->mask_bf_net_cfg->sym=cfg->sym;
				}
				ret = 0;
			}
		}
	}
	return ret;
}
int qtk_vboxebf_cfg_update2(qtk_vboxebf_cfg_t *cfg, wtk_source_loader_t *sl)
{
	return qtk_vboxebf_cfg_update(cfg);
}

qtk_vboxebf_cfg_t *qtk_vboxebf_cfg_new(char *fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_vboxebf_cfg_t *cfg;

	main_cfg = wtk_main_cfg_new_type(qtk_vboxebf_cfg, fn);
	if(!main_cfg){
		return NULL;
	}
	cfg = (qtk_vboxebf_cfg_t *)main_cfg->cfg;
	cfg->main_cfg = main_cfg;

	return cfg;
}

void qtk_vboxebf_cfg_delete(qtk_vboxebf_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->main_cfg);
}

qtk_vboxebf_cfg_t *qtk_vboxebf_cfg_new_bin(char *bin_fn)
{
	qtk_vboxebf_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn = "./cfg";
	int ret;

	cfg = (qtk_vboxebf_cfg_t *)wtk_malloc(sizeof(qtk_vboxebf_cfg_t));
	qtk_vboxebf_cfg_init(cfg);
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
	ret = qtk_vboxebf_cfg_update_local(cfg, cfg->cfile->main);
	if(ret != 0){
		goto end;
	}
	sl.hook = cfg->rbin;
	sl.vf = (wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret = qtk_vboxebf_cfg_update2(cfg, &sl);
	if(ret != 0){
		goto end;
	}
	ret = 0;
end:
	if(ret != 0){
		qtk_vboxebf_cfg_delete_bin(cfg);
		cfg = NULL;
	}
	return cfg;
}
void qtk_vboxebf_cfg_delete_bin(qtk_vboxebf_cfg_t *cfg)
{
	qtk_vboxebf_cfg_clean(cfg);
	if(cfg->cfile){
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
	wtk_free(cfg);
}

void qtk_vboxebf_cfg_printf(qtk_vboxebf_cfg_t *cfg)
{
	printf("SDK cfg set:\n");
	// printf("use_vboxebf        =%d\n",cfg->use_vboxebf);
	// printf("use_vboxebf2         =%d\n",cfg->use_vboxebf2);
	printf("use_vboxebf3         =%d\n",cfg->use_vboxebf3);
	printf("use_vboxebf4         =%d\n",cfg->use_vboxebf4);
	printf("use_vboxebf6         =%d\n",cfg->use_vboxebf6);
	printf("use_mask_bf_net      =%d\n",cfg->use_mask_bf_net);
	printf("use_ahs              =%d\n",cfg->use_ahs);
	// printf("use_vboxebf4_multi   =%d\n",cfg->use_vboxebf4_multi);
	// printf("use_vboxebf5         =%d\n",cfg->use_vboxebf5);

	printf("spk_channel          =%d\n",cfg->spk_channel);
	printf("mic_shift            =%f\n",cfg->mic_shift);
	printf("spk_shift            =%f\n",cfg->spk_shift);
	printf("echo_shift           =%f\n",cfg->echo_shift);

	printf("max_extp             =%d\n",cfg->max_extp);
	printf("online_tms           =%d\n",cfg->online_tms);
	printf("theta_range          =%d\n",cfg->theta_range);
	printf("continue_count       =%d\n",cfg->continue_count);
	printf("energy_sum           =%f\n",cfg->energy_sum);
	printf("zero_sum             =%f\n",cfg->zero_sum);
	printf("gbias                =%f\n",cfg->gbias);
	printf("agca                 =%f\n",cfg->agca);

	printf("use_ssl              =%d\n",cfg->use_ssl);
	printf("use_maskssl          =%d\n",cfg->use_maskssl);
	printf("use_maskssl2         =%d\n",cfg->use_maskssl2);

	printf("theta_step           =%d\n",cfg->theta_step);
	printf("online_frame_step    =%d\n",cfg->online_frame_step);
	printf("specsum_fs           =%d\n",cfg->specsum_fs);
	printf("specsum_fe           =%d\n",cfg->specsum_fe);
	printf("lf                   =%d\n",cfg->lf);
	printf("use_ssl_filter       =%d\n",cfg->use_ssl_filter);
	printf("use_fftsbf           =%d\n",cfg->use_fftsbf);
	printf("bfmu                 =%f\n",cfg->bfmu);
	printf("echo_bfmu            =%f\n",cfg->echo_bfmu);
	printf("spenr_thresh         =%f\n",cfg->spenr_thresh);
	printf("noise_suppress       =%f\n",cfg->noise_suppress);
	printf("echo_suppress        =%f\n",cfg->echo_suppress);
	printf("echo_suppress_active =%f\n",cfg->echo_suppress_active);
	printf("use_cnon             =%d\n",cfg->use_cnon);
	printf("sym                  =%f\n",cfg->sym);
	printf("mic_volume           =%f\n",cfg->mic_volume);
	printf("agc_level            =%f\n",cfg->agc_level);
	printf("agc_enable           =%d\n",cfg->agc_enable);
	printf("echo_enable          =%d\n",cfg->echo_enable);
	printf("denoise_enable       =%d\n",cfg->denoise_enable);
	printf("suppress             =%f\n",cfg->suppress);
	printf("use_erlssingle       =%d\n",cfg->use_erlssingle);
	printf("use_ssl_delay        =%d\n",cfg->use_ssl_delay);

	printf("---------------end-----------------\n");
}
