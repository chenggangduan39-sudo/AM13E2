#include "wtk_qenvelope_cfg.h"


int wtk_qenvelope_cfg_init(wtk_qenvelope_cfg_t *cfg)
{
    cfg->envelope_nf=12;
    cfg->envelope_thresh=600;

    cfg->use_envelope2=0;
    cfg->min_speccrest=1000.0;

    cfg->right_min_thresh=100;
    cfg->max_rise_nf=5;

    cfg->left_nf=4;
    cfg->right_nf=4;

    cfg->idx=0;

    cfg->debug=0;

    return 0;
}

int wtk_qenvelope_cfg_clean(wtk_qenvelope_cfg_t *cfg)
{
	return 0;
}

int wtk_qenvelope_cfg_update_local(wtk_qenvelope_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_envelope2,v);

    wtk_local_cfg_update_cfg_i(lc,cfg,envelope_nf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,envelope_thresh,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,min_speccrest,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,left_nf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,right_nf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,right_min_thresh,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,max_rise_nf,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,idx,v);

    return 0;
}

int wtk_qenvelope_cfg_update(wtk_qenvelope_cfg_t *cfg)
{
    return 0;
}

int wtk_qenvelope_cfg_set_idx(wtk_qenvelope_cfg_t *cfg, int idx)
{
    cfg->idx=idx;
    return 0;
}
