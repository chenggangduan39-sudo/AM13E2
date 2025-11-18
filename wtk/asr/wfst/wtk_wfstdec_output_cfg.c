#include "wtk_wfstdec_output_cfg.h" 

int wtk_wfstdec_output_cfg_init(wtk_wfstdec_output_cfg_t *cfg)
{
	wtk_version_cfg_init(&(cfg->version),"0.0.13");
	wtk_string_set_s(&(cfg->res),"0.0.3");
	wtk_string_set_s(&(cfg->sep)," ");
	return 0;
}

int wtk_wfstdec_output_cfg_clean(wtk_wfstdec_output_cfg_t *cfg)
{
	wtk_version_cfg_clean(&(cfg->version));
	return 0;
}

int wtk_wfstdec_output_cfg_update_local(wtk_wfstdec_output_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_string_v(lc,cfg,res,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,sep,v);
	return 0;
}

int wtk_wfstdec_output_cfg_update(wtk_wfstdec_output_cfg_t *cfg)
{
	return 0;
}


int wtk_wfstdec_output_cfg_update2(wtk_wfstdec_output_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}
