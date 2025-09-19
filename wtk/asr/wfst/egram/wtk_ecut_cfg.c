#include "wtk_ecut_cfg.h"
#include "wtk/core/cfg/wtk_source.h"

int wtk_ecut_cfg_init(wtk_ecut_cfg_t *cfg)
{
	cfg->min = -144;
	cfg->shift = 7;
	cfg->sil_id1 = 1;
	cfg->sil_id2 = 45;
	return 0;
}

int wtk_ecut_cfg_clean(wtk_ecut_cfg_t *cfg)
{

	return 0;
}

int wtk_ecut_cfg_update_local(wtk_ecut_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_i(lc,cfg,sil_id1,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sil_id2,v);

	ret=0;
	return ret;
}

int wtk_ecut_cfg_update(wtk_ecut_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_ecut_cfg_update2(cfg,&sl);
}

int wtk_ecut_cfg_update2(wtk_ecut_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=0;
	return ret;
}
