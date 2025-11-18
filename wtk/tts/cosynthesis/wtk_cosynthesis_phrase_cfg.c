/*
 * wtk_cosynthesis_phrase_cfg.c
 *
 *  Created on: Jan 28, 2022
 *      Author: dm
 */
#include "wtk_cosynthesis_phrase_cfg.h"

int wtk_cosynthesis_phrase_cfg_init(wtk_cosynthesis_phrase_cfg_t *cfg)
{
	cfg->crf_path=NULL;
	cfg->weight_path=NULL;
	cfg->rbin=NULL;

	return 0;
}

int wtk_cosynthesis_phrase_cfg_update_local(wtk_cosynthesis_phrase_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_str(lc, cfg, crf_path, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, weight_path, v);

	return 0;
}

int wtk_cosynthesis_phrase_cfg_clean(wtk_cosynthesis_phrase_cfg_t *cfg)
{
	if (cfg->rbin)
	{
		wtk_free(cfg->crf_path);
		wtk_free(cfg->weight_path);
	}

	return 0;
}

int wtk_cosynthesis_phrase_cfg_update(wtk_cosynthesis_phrase_cfg_t *cfg)
{
	return 0;
}

int wtk_cosynthesis_phrase_cfg_update2(wtk_cosynthesis_phrase_cfg_t *cfg,wtk_source_loader_t *sl)
{
	wtk_string_t *pwd;
	char *np;

	cfg->rbin = sl->hook;
	if (cfg->rbin)
	{
		pwd=wtk_dir_name(cfg->rbin->fn, '/');
		np = (char*)wtk_calloc(pwd->len+1+strlen(cfg->crf_path)+1, sizeof(char));
		memcpy(np, pwd->data, pwd->len);
		memcpy(np+pwd->len, "/", 1);
		memcpy(np+pwd->len+1, cfg->crf_path, strlen(cfg->crf_path));
		cfg->crf_path = np;

		np = (char*)wtk_calloc(pwd->len+1+strlen(cfg->weight_path)+1, sizeof(char));
		memcpy(np, pwd->data, pwd->len);
		memcpy(np+pwd->len, "/", 1);
		memcpy(np+pwd->len+1, cfg->weight_path, strlen(cfg->weight_path));
		cfg->weight_path = np;
		wtk_free(pwd);

	}
	return 0;
}


