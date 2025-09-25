#include "wtk_wfstenv_cfg.h"
#include "wtk_wfstdec_cfg.h"

int wtk_wfstenv_cfg_init(wtk_wfstenv_cfg_t *cfg,wtk_wfstdec_cfg_t *dec_cfg)
{
	wtk_string_set(&(cfg->custom),0,0);
	cfg->cfg=dec_cfg;
	cfg->use_vad=dec_cfg->use_vad;
	cfg->sep=dec_cfg->output.sep;
	//wtk_debug("split=%d link=%d\n",cfg->use_split,cfg->use_link);
	cfg->use_usrec=0;
	cfg->use_ebnfdec=0;
	cfg->use_hint=0;
	cfg->use_dec2=1;
	cfg->use_timestamp = 0;
	cfg->use_laststate = 0;
	cfg->use_pp = 0;
	return 0;
}

void wtk_wfstenv_cfg_init2(wtk_wfstenv_cfg_t *cfg)
{
	wtk_string_set(&(cfg->custom),0,0);
	cfg->cfg=0;
	cfg->use_vad=0;
	cfg->use_usrec=0;
	cfg->use_ebnfdec=0;
	cfg->use_hint=0;
	cfg->use_dec2=1;
	cfg->use_timestamp = 0;
	cfg->use_laststate = 0;
	cfg->use_pp = 0;
	//wtk_string_set(&(cfg->sep),0,0);
}

int wtk_wfstenv_cfg_update_local(wtk_wfstenv_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	if(cfg->cfg->use_ebnfdec2)
	{
		wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnfdec,v);
	}
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_usrec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_timestamp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_laststate,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_pp,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,sep,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,custom,v);
	return 0;
}

int wtk_wfstenv_cfg_update_local2(wtk_wfstenv_cfg_t *cfg,wtk_local_cfg_t *lc,int ebnf2)
{
	wtk_string_t *v;

	if(ebnf2)
	{
			wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnfdec,v);
	}
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_usrec,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_timestamp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_laststate,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_pp,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,sep,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,custom,v);
	return 0;
}
