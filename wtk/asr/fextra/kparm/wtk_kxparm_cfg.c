#include "wtk_kxparm_cfg.h" 

int wtk_kxparm_cfg_bytes(wtk_kxparm_cfg_t *cfg)
{
	int bytes;

	bytes=sizeof(wtk_kxparm_cfg_t);
	bytes+=wtk_kparm_cfg_bytes(&(cfg->parm));
	if(cfg->use_knn)
	{
		bytes+=wtk_knn_cfg_bytes(&(cfg->knn));
	}
	//TODO nnet3 bytes
	return bytes;
}

int wtk_kxparm_cfg_get_win(wtk_kxparm_cfg_t *cfg)
{
	if(cfg->use_htk)
	{
		return cfg->htk.frame_step;
	}else
	{
		return cfg->parm.frame_step;
	}
}

int wtk_kxparm_cfg_get_rate(wtk_kxparm_cfg_t *cfg)
{
	if(cfg->use_htk)
	{
		return wtk_fextra_cfg_get_sample_rate(&(cfg->htk));
	}else
	{
		return  cfg->parm.rate;
	}
}

int wtk_kxparm_cfg_init(wtk_kxparm_cfg_t *cfg)
{
	wtk_fextra_cfg_init(&(cfg->htk));
	wtk_kparm_cfg_init(&(cfg->parm));
	wtk_knn_cfg_init(&(cfg->knn));
	qtk_nnet3_cfg_init(&(cfg->nnet3));
	cfg->use_htk=0;
	cfg->use_knn=0;
	cfg->use_nnet3=0;
	return 0;
}

int wtk_kxparm_cfg_clean(wtk_kxparm_cfg_t *cfg)
{
	wtk_fextra_cfg_clean(&(cfg->htk));
	wtk_kparm_cfg_clean(&(cfg->parm));
	wtk_knn_cfg_clean(&(cfg->knn));
	qtk_nnet3_cfg_clean(&(cfg->nnet3));
	return 0;
}

int wtk_kxparm_cfg_update_local(wtk_kxparm_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_knn,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_htk,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_nnet3,v);
	//wtk_debug("use_knn=%d\n",cfg->use_knn);
	if(cfg->use_htk)
	{
		lc=wtk_local_cfg_find_lc_s(main,"htk");
		if(lc)
		{
			ret=wtk_fextra_cfg_update_local(&(cfg->htk),lc);
			if(ret!=0){goto end;}
		}
	}else
	{
		lc=wtk_local_cfg_find_lc_s(main,"parm");
		if(lc)
		{
			ret=wtk_kparm_cfg_update_local(&(cfg->parm),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_knn)
	{
		lc=wtk_local_cfg_find_lc_s(main,"knn");
		if(lc)
		{
			ret=wtk_knn_cfg_update_local(&(cfg->knn),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_nnet3)
	{
		lc=wtk_local_cfg_find_lc_s(main,"nnet3");
		if(lc)
		{
			ret=qtk_nnet3_cfg_update_local(&(cfg->nnet3),lc);
			if(ret!=0){goto end;}
		}
	}

	ret=0;
end:
	return ret;
}

int wtk_kxparm_cfg_update(wtk_kxparm_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_kxparm_cfg_update2(cfg,&sl);
}

int wtk_kxparm_cfg_update2(wtk_kxparm_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->use_htk)
	{
		ret=wtk_fextra_cfg_update2(&(cfg->htk),sl);
	}else
	{
		ret=wtk_kparm_cfg_update2(&(cfg->parm),sl);
		cfg->knn.use_fixpoint=cfg->parm.use_fixpoint;
	}
	if(ret!=0)
	{
		wtk_debug("update feature failed\n");
		goto end;
	}
	if(cfg->use_knn)
	{
		ret=wtk_knn_cfg_update2(&(cfg->knn),sl);
		if(ret!=0){goto end;}
	}
	if(cfg->use_nnet3)
	{
            ret = qtk_nnet3_cfg_update2(&(cfg->nnet3), sl);
        }

	ret=0;
end:
	return ret;
}
