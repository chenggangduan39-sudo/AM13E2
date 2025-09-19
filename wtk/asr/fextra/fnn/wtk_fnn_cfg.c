#include <math.h>
#include "wtk_fnn_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_math.h"
//#include <iostream>

int wtk_fnn_cfg_init(wtk_fnn_cfg_t *cfg)
{
	cfg->padding_sil_reset_cnt=20;
	cfg->padding_sil_set_cnt=10;
    cfg->win = 5;
    //cfg->in_cols=0;
    //cfg->out_cols=0;
    cfg->use_delta=1;
    cfg->use_blas = 0;
    cfg->min_flush_frame=0;
    cfg->use_linear_output=0;
    cfg->use_mlat=0;
    cfg->use_qlas=0;
    wtk_qlas_cfg_init(&(cfg->qlas));
    wtk_mlat_cfg_init(&(cfg->mlat));
    //cfg->cache_size=0;
#ifdef USE_BLAS
    wtk_blas_cfg_init(&(cfg->blas));
#endif
    wtk_flat_cfg_init(&(cfg->flat));
    cfg->attach_htk_log=0;
    cfg->skip_frame=0;
    cfg->use_lazy_out=0;
    cfg->use_expand_vector=0;
    cfg->sil_thresh=0.3;
    cfg->speech_thresh=0.5;
    cfg->padding_frame=0;
    cfg->use_hack_output=0;
    return 0;
}

int wtk_fnn_cfg_clean(wtk_fnn_cfg_t *cfg)
{
	if(cfg->use_qlas)
	{
		wtk_qlas_cfg_clean(&(cfg->qlas));
	}else if(cfg->use_mlat)
	{
		wtk_mlat_cfg_clean(&(cfg->mlat));
	}else  if (cfg->use_blas) {
#ifdef USE_BLAS
        wtk_blas_cfg_clean(&(cfg->blas));
#endif
    }else {
        wtk_flat_cfg_clean(&(cfg->flat));
    }
    return 0;
}

int wtk_fnn_cfg_bytes(wtk_fnn_cfg_t *cfg)
{
	int bytes=0;

    if (cfg->use_blas) {
#ifdef USE_BLAS
        bytes+=wtk_blas_cfg_bytes(&(cfg->blas));
#endif
    } else {
//        bytes=wtk_flat_cfg_bytes(&(cfg->flat));
    }
	return bytes;
}

int wtk_fnn_cfg_update_local(wtk_fnn_cfg_t *cfg, wtk_local_cfg_t *main)
{
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    int ret = 0;

    lc = main;
    wtk_local_cfg_update_cfg_i(lc,cfg,padding_sil_set_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,padding_sil_reset_cnt,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,skip_frame,v);
    wtk_local_cfg_update_cfg_i(lc, cfg, win, v);
    wtk_local_cfg_update_cfg_i(lc,cfg,min_flush_frame,v);
    //wtk_local_cfg_update_cfg_i(lc,cfg,cache_size,v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_hack_output, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_blas, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_mlat, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_qlas, v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_linear_output,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,attach_htk_log,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_delta,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_lazy_out,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_expand_vector,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,padding_frame,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,sil_thresh,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,speech_thresh,v);
    //wtk_local_cfg_update_cfg_i(lc,cfg,cache_size,v);
    //wtk_local_cfg_update_cfg_str(lc,cfg,state_fn,v);
    if(cfg->use_qlas)
    {
        lc = wtk_local_cfg_find_lc_s(main, "qlas");
        if (!lc)
        {
            lc = main;
        }
        ret = wtk_qlas_cfg_update_local(&(cfg->qlas), lc);
        if (ret != 0)
        {
            goto end;
        }
    }else if(cfg->use_mlat)
    {
        lc = wtk_local_cfg_find_lc_s(main, "mlat");
        if (!lc) {
            lc = main;
        }
        ret = wtk_mlat_cfg_update_local(&(cfg->mlat), lc);
        if (ret != 0) {
            goto end;
        }
    }else if (cfg->use_blas) {
#ifdef USE_BLAS
        lc = wtk_local_cfg_find_lc_s(main, "blas");
        if (!lc) {
            lc = main;
        }
        ret = wtk_blas_cfg_update_local(&(cfg->blas), lc);
        if (ret != 0) {
            goto end;
        }
#endif
    }else {
        lc = wtk_local_cfg_find_lc_s(main, "flat");
        if (!lc) {
            lc = main;
        }
        ret = wtk_flat_cfg_update_local(&(cfg->flat), lc);
        if (ret != 0) {
            goto end;
        }
    }
end:
    //wtk_debug("ret=%d\n",ret);
    return ret;
}

int wtk_fnn_cfg_update2(wtk_fnn_cfg_t *cfg, wtk_source_loader_t *sl)
{
    int ret;

    if(cfg->use_qlas)
    {
    	ret=wtk_qlas_cfg_update2(&(cfg->qlas),sl);
    	if(ret!=0){goto end;}
        cfg->out_cols = wtk_qlas_cfg_out_cols(&(cfg->qlas));
    }else  if(cfg->use_mlat)
    {
        ret = wtk_mlat_cfg_update2(&(cfg->mlat), sl);
        if (ret != 0) {
            goto end;
        }
        //cfg->in_cols=wtk_blas_cfg_in_cols(&(cfg->blas));
        cfg->out_cols = wtk_mlat_cfg_out_cols(&(cfg->mlat));
    }else if (cfg->use_blas) {
#ifdef USE_BLAS
        ret = wtk_blas_cfg_update2(&(cfg->blas), sl);
        if (ret != 0) {
            goto end;
        }
        //cfg->in_cols=wtk_blas_cfg_in_cols(&(cfg->blas));
        cfg->out_cols = wtk_blas_cfg_out_cols(&(cfg->blas));
#endif
    } else
	{
        ret = wtk_flat_cfg_update2(&(cfg->flat), sl);
        if (ret != 0) {
            goto end;
        }
        //cfg->in_cols=wtk_blas_cfg_in_cols(&(cfg->blas));
        cfg->out_cols = wtk_flat_cfg_out_cols(&(cfg->flat));
	}
    if(cfg->skip_frame>0)
    {
    	++cfg->skip_frame;
    	cfg->min_flush_frame*=cfg->skip_frame;
    	//cfg->cache_size*=cfg->skip_frame;
    }
    ret=0;
end:
    return ret;
}

int wtk_fnn_cfg_update(wtk_fnn_cfg_t *cfg)
{
    wtk_source_loader_t file_sl;
    int ret;

    file_sl.hook = 0;
    file_sl.vf = wtk_source_load_file_v;
    ret = wtk_fnn_cfg_update2(cfg, &(file_sl));
    return ret;
}
