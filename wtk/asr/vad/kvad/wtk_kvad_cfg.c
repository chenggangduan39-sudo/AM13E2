#include "wtk_kvad_cfg.h" 

int wtk_kvad_cfg_bytes(wtk_kvad_cfg_t *cfg)
{
	int bytes;

	bytes=wtk_kxparm_cfg_bytes(&(cfg->parm));
	return bytes;
}

int wtk_kvad_cfg_init(wtk_kvad_cfg_t *cfg)
{
	wtk_kxparm_cfg_init(&(cfg->parm));
	cfg->sil_trap=3;
	cfg->speech_trap=3;
	cfg->left_margin=0;
	cfg->right_margin=0;
	cfg->speech_thresh=0;
	cfg->speech_enter_prob=0;
	cfg->speech_leave_prob=0;
	cfg->shift=12;
	cfg->use_fixpoint=0;
	cfg->cache=5;
	cfg->use_bin = 0;
	cfg->use_prob = 0;
	cfg->use_nnprob = 0;
	return 0;
}

int wtk_kvad_cfg_clean(wtk_kvad_cfg_t *cfg)
{

	wtk_kxparm_cfg_clean(&(cfg->parm));
	return 0;
}

int wtk_kvad_cfg_update_local(wtk_kvad_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_i(lc,cfg,sil_trap,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,speech_trap,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,left_margin,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,right_margin,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speech_thresh,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speech_enter_prob,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,speech_leave_prob,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_fixpoint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,cache,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_prob,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_nnprob,v);
	lc=wtk_local_cfg_find_lc_s(main,"parm");
	if(lc)
	{
		ret=wtk_kxparm_cfg_update_local(&(cfg->parm),lc);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_kvad_cfg_update(wtk_kvad_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_kvad_cfg_update2(cfg,&sl);
}

int wtk_kvad_cfg_update2(wtk_kvad_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	ret=wtk_kxparm_cfg_update2(&(cfg->parm),sl);
	if(ret!=0){goto end;}

	cfg->use_fixpoint=cfg->parm.knn.use_fixpoint;
	if(cfg->use_fixpoint)
	{
		cfg->shift=cfg->parm.knn.softmax_fixl->shift;
		cfg->fix_speech_enter_prob=FLOAT2FIX_ANY(cfg->speech_enter_prob,cfg->shift);
		cfg->fix_speech_leave_prob=FLOAT2FIX_ANY(cfg->speech_leave_prob,cfg->shift);
		cfg->fix_speech_thresh=cfg->speech_thresh;//FIX2FLOAT_ANY(cfg->speech_thresh,cfg->shift);
	}
	ret=0;
end:
	return ret;
}

