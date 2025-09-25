#include "wtk_vparm_cfg.h" 
#include <math.h>

int wtk_vparm_cfg_init(wtk_vparm_cfg_t *cfg)
{
	wtk_vad_cfg_init(&(cfg->vad2));
	wtk_fextra_cfg_init(&(cfg->parm));
	wtk_hmmset_cfg_init(&(cfg->hmmset));
	cfg->speech=NULL;
	cfg->label=NULL;//wtk_label_new(107);
	cfg->min_log_exp=-log(-LZERO);
	cfg->use_vad=1;
	cfg->skip_frame=0;
	return 0;
}

int wtk_vparm_cfg_clean(wtk_vparm_cfg_t *cfg)
{
	if(cfg->label)
	{
		wtk_label_delete(cfg->label);
	}
	wtk_vad_cfg_clean(&(cfg->vad2));
	wtk_fextra_cfg_clean(&(cfg->parm));
	wtk_hmmset_cfg_clean(&(cfg->hmmset));
	return 0;
}

int wtk_vparm_cfg_update_local(wtk_vparm_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_string_t *v;
	wtk_local_cfg_t *lc;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,skip_frame,v);
	lc=wtk_local_cfg_find_lc_s(main,"vad2");
	if(lc)
	{
		ret=wtk_vad_cfg_update_local(&(cfg->vad2),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"fextra");
	if(!lc)
	{
		lc=wtk_local_cfg_find_lc_s(main,"parm");
	}
	if(lc)
	{
		ret=wtk_fextra_cfg_update_local(&(cfg->parm),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"hmmset");
	if(lc)
	{
		ret=wtk_hmmset_cfg_update_local(&(cfg->hmmset),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_vparm_cfg_update(wtk_vparm_cfg_t *cfg)
{
	int ret;

	if(cfg->use_vad)
	{
		ret=wtk_vad_cfg_update(&(cfg->vad2));
		if(ret!=0)
		{
			wtk_debug("update vad2 failed\n");
			goto end;
		}
	}
	ret=wtk_fextra_cfg_update(&(cfg->parm));
	if(ret!=0)
	{
		wtk_debug("update parm failed\n");
		goto end;
	}
	cfg->label=wtk_label_new(107);
	ret=wtk_hmmset_cfg_update(&(cfg->hmmset),cfg->label);
	if(ret!=0)
	{
		wtk_debug("update hmmset failed\n");
		goto end;
	}
	cfg->speech=wtk_hmmset_find_hmm_s(cfg->hmmset.hmmset,"speech");
	ret=0;
end:
	return ret;
}

int wtk_vparm_cfg_update2(wtk_vparm_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;

	if(cfg->use_vad){
		ret=wtk_vad_cfg_update2(&(cfg->vad2),sl);
		if(ret!=0){goto end;}
	}
	ret=wtk_fextra_cfg_update2(&(cfg->parm),sl);
	if(ret!=0){goto end;}
	cfg->label=wtk_label_new(107);
	ret=wtk_hmmset_cfg_update2(&(cfg->hmmset),cfg->label,sl);
	if(ret!=0){goto end;}
	cfg->speech=wtk_hmmset_find_hmm_s(cfg->hmmset.hmmset,"speech");

end:
	return ret;
}

