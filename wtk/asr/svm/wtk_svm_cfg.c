#include "wtk_svm_cfg.h"

int wtk_svm_cfg_init(wtk_svm_cfg_t *cfg)
{
	cfg->svm=0;
	cfg->svm_fn=0;
	return 0;
}

int wtk_svm_cfg_clean(wtk_svm_cfg_t *cfg)
{
	if(cfg->svm)
	{
		wtk_svm_delete(cfg->svm);
	}
	return 0;
}

int wtk_svm_cfg_update_local(wtk_svm_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_str(lc,cfg,svm_fn,v);
	return 0;
}

int wtk_svm_cfg_update(wtk_svm_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	/*
	svm_model_t *x;

	x=svm_load_model(cfg->svm_fn);
	wtk_debug("x=%p\n",x);
	exit(0);
	*/
	cfg->svm=wtk_svm_new(0);
	ret=wtk_source_loader_load(sl,cfg->svm,(wtk_source_load_handler_t)wtk_svm_load,cfg->svm_fn);
	return ret;
}
