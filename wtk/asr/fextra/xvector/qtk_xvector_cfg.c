#include "qtk_xvector_cfg.h"

int qtk_xvector_cfg_init(qtk_xvector_cfg_t *cfg)
{
    wtk_nnet3_xvector_compute_cfg_init(&cfg->xvector);
    wtk_vad_cfg_init(&cfg->vad);

    // cfg->main_vad_cfg=NULL;
    cfg->vad_energy_thresh=5.5;//5.5;
    cfg->vad_energy_mean_scale=0.5;//0.6;//0.5;
    cfg->vad_proportion_threshold=0.12;//0.12;
    cfg->vad_frames_context=2;
	return 0;
}

int qtk_xvector_cfg_clean(qtk_xvector_cfg_t *cfg)
{
    wtk_vad_cfg_clean(&cfg->vad);

    wtk_nnet3_xvector_compute_cfg_clean(&cfg->xvector);

	return 0;
}

int qtk_xvector_cfg_update_local(qtk_xvector_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *lc2;
	wtk_string_t *v;

    lc2=wtk_local_cfg_find_lc_s(lc,"xvector");
    if(lc2)
    {
        wtk_nnet3_xvector_compute_cfg_update_local(&cfg->xvector,lc2);
    }

    lc2=wtk_local_cfg_find_lc_s(lc,"vad");
    if(lc2)
    {
        wtk_vad_cfg_update_local(&cfg->vad,lc2);
    }

    wtk_local_cfg_update_cfg_f(lc,cfg,vad_energy_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,vad_energy_mean_scale,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,vad_proportion_threshold,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,vad_frames_context,v);

	return 0;
}

int qtk_xvector_cfg_update(qtk_xvector_cfg_t *cfg)
{
	int ret=0;
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

    ret=wtk_nnet3_xvector_compute_cfg_update2(&cfg->xvector,&sl);
    if(ret!=0){goto end;}

    ret=wtk_vad_cfg_update2(&cfg->vad,&sl);
    if(ret!=0){goto end;}

end:
	return ret;
}
