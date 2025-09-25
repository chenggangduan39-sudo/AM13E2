#include "qtk_api_img_cfg.h"
#include "wtk/os/wtk_log.h"

int qtk_api_img_cfg_init(qtk_api_img_cfg_t *cfg)
{
	cfg->img_mcfg = NULL;
	cfg->img_fn = NULL;
	cfg->img = NULL;
	cfg->vad_fn = NULL;
	cfg->vad = NULL;
	cfg->left_margin = 20;
	cfg->right_margin = 20;
	cfg->img_use_bin = 0;
	cfg->log_wav = 0;
	

	return 0;
}

int qtk_api_img_cfg_clean(qtk_api_img_cfg_t *cfg)
{
	if (cfg->img) {
		if (cfg->img_use_bin) {
			qtk_img_cfg_delete_bin(cfg->img);
		} else {
			wtk_main_cfg_delete(cfg->img_mcfg);
		}
	}
	if (cfg->vad) {
		wtk_vad_cfg_delete_bin(cfg->vad);
	}
	return 0;
}

int qtk_api_img_cfg_update_local(qtk_api_img_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(main, cfg, img_use_bin, v);
	wtk_local_cfg_update_cfg_b(main, cfg, log_wav, v);
	wtk_local_cfg_update_cfg_i(main, cfg, left_margin, v);
	wtk_local_cfg_update_cfg_i(main, cfg, right_margin, v);
	wtk_local_cfg_update_cfg_str(main, cfg, img_fn, v);
	wtk_local_cfg_update_cfg_str(main, cfg, vad_fn, v);
	return 0;
}

int qtk_api_img_cfg_update(qtk_api_img_cfg_t *cfg)
{
	int ret = 0;

	if (cfg->img_fn) {
		if (cfg->img_use_bin) {
			cfg->img = qtk_img_cfg_new_bin(cfg->img_fn);
			if (!cfg->img) {
				ret = -1;
			}
		} else {
			cfg->img_mcfg = wtk_main_cfg_new_type(qtk_img_cfg, cfg->img_fn);
			if (!cfg->img_mcfg) {
				ret = -1;
			}
			cfg->img = (qtk_img_cfg_t *)cfg->img_mcfg->cfg;
		}
	}
	if (cfg->vad_fn) {
		cfg->vad = wtk_vad_cfg_new_bin2(cfg->vad_fn);
	}

	return ret;
}


void qtk_api_img_cfg_update_params(qtk_api_img_cfg_t *cfg,wtk_local_cfg_t *params)
{

}

qtk_api_img_cfg_t* qtk_api_img_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_api_img_cfg_t *cfg = NULL;

	main_cfg = wtk_main_cfg_new_type(qtk_api_img_cfg,cfg_fn);
	if (!main_cfg) {
		wtk_log_err(glb_log, "api_img cfg new failed. [%s]", cfg_fn);
		goto end;
	}
	cfg = (qtk_api_img_cfg_t*)main_cfg->cfg;
	cfg->hook = main_cfg;
end:
	return cfg;
}

void qtk_api_img_cfg_delete(qtk_api_img_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->hook);
}

