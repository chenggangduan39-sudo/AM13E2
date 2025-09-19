#include "wtk_wbf2_cfg.h" 

int wtk_wbf2_cfg_init(wtk_wbf2_cfg_t *cfg)
{
	wtk_stft2_cfg_init(&(cfg->stft2));
    wtk_qform2_cfg_init(&(cfg->qform2));
    wtk_qform3_cfg_init(&(cfg->qform3));

	cfg->wbf2_cnt=4;
    cfg->use_line=0;
    cfg->theta = NULL;

    cfg->use_qform3=0;
    return 0;
}

int wtk_wbf2_cfg_clean(wtk_wbf2_cfg_t *cfg)
{
	wtk_stft2_cfg_clean(&(cfg->stft2));
    wtk_qform2_cfg_clean(&(cfg->qform2));
    wtk_qform3_cfg_clean(&(cfg->qform3));
    if (cfg->theta) {
        wtk_free(cfg->theta);
    }

        return 0;
}

int wtk_wbf2_cfg_update_local(wtk_wbf2_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;
    wtk_local_cfg_t *m;
    wtk_array_t *theta;
    int ret, i;

    wtk_local_cfg_update_cfg_i(lc, cfg, wbf2_cnt, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_line,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_qform3,v);

    m=lc;
	lc=wtk_local_cfg_find_lc_s(m,"qform2");
	if(lc)
	{
        ret=wtk_qform2_cfg_update_local(&(cfg->qform2),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"qform3");
	if(lc)
	{
        ret=wtk_qform3_cfg_update_local(&(cfg->qform3),lc);
        if(ret!=0){goto end;}
    }
	lc=wtk_local_cfg_find_lc_s(m,"stft2");
	if(lc)
	{
        ret=wtk_stft2_cfg_update_local(&(cfg->stft2),lc);
        if(ret!=0){goto end;}
    }
    theta = wtk_local_cfg_find_array_s(m, "theta");
    if (theta) {
        if (theta->nslot != cfg->wbf2_cnt) {
            ret = -1;
            wtk_debug("Wbf2 Cnt Not Eql N-Theta\n");
            goto end;
        }
        cfg->theta = wtk_malloc(sizeof(int) * theta->nslot);
        for (i = 0; i < theta->nslot; i++) {
            v = cast(wtk_string_t **, theta->slot)[i];
            cfg->theta[i] = wtk_str_atoi(v->data, v->len);
        }
    }

        ret=0;
end:
    return ret;
}

static void _update_theta(wtk_wbf2_cfg_t *cfg) {
    int theta_step, i, theta;
    if (cfg->theta) {
        return;
    }
    theta_step = cfg->use_line ? floor(180.0 / (cfg->wbf2_cnt - 1))
                               : floor(359.0 / cfg->wbf2_cnt) + 1;
    cfg->theta = wtk_malloc(sizeof(float) * cfg->wbf2_cnt);
    for (theta = 0, i = 0; i < cfg->wbf2_cnt; theta += theta_step, i++) {
        cfg->theta[i] = theta;
    }
}

int wtk_wbf2_cfg_update(wtk_wbf2_cfg_t *cfg)
{
    int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
    ret=wtk_qform2_cfg_update(&(cfg->qform2));
    if(ret!=0){goto end;}
    ret=wtk_qform3_cfg_update(&(cfg->qform3));
    if(ret!=0){goto end;}

    _update_theta(cfg);
    ret = 0;
end:
	return ret;
}


int wtk_wbf2_cfg_update2(wtk_wbf2_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

    ret=wtk_stft2_cfg_update(&(cfg->stft2));
    if(ret!=0){goto end;}
    ret=wtk_qform2_cfg_update(&(cfg->qform2));
    if(ret!=0){goto end;}

    ret=wtk_qform3_cfg_update(&(cfg->qform3));
    if(ret!=0){goto end;}
    
    _update_theta(cfg);
    ret = 0;
end:
	return ret;
}
