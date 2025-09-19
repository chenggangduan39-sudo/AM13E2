#include "wtk_tts_segsnt_cfg.h" 

int wtk_tts_segsnt_cfg_init(wtk_tts_segsnt_cfg_t *cfg)
{
	cfg->end_tok=NULL;
	cfg->end_cand_tok=NULL;
	cfg->ques_char=NULL;
	cfg->ques_sym=NULL;
	cfg->ques_char2=NULL;
	cfg->sigh_char=NULL;
	cfg->sigh_sym=NULL;
	cfg->ques_start2=NULL;
	cfg->use_random_tone=1;
	cfg->rand_step=3;
	cfg->max_word=60;//50;//20;

	cfg->pick_list=NULL;
	cfg->pick_h=NULL;
	return 0;
}

int wtk_tts_segsnt_cfg_clean(wtk_tts_segsnt_cfg_t *cfg)
{
	if(cfg->pick_h){
		wtk_str_hash_delete(cfg->pick_h);
	}
	return 0;
}

int wtk_tts_segsnt_cfg_update_local(wtk_tts_segsnt_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_random_tone,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,rand_step,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,max_word,v);
	cfg->end_tok=wtk_local_cfg_find_array_s(lc,"end_tok");
	cfg->end_cand_tok=wtk_local_cfg_find_array_s(lc,"end_cand_tok");
	cfg->ques_sym=wtk_local_cfg_find_array_s(lc,"ques_sym");
	cfg->ques_char=wtk_local_cfg_find_array_s(lc,"ques_char");
	cfg->ques_char2=wtk_local_cfg_find_array_s(lc,"ques_char2");
	cfg->ques_start2=wtk_local_cfg_find_array_s(lc,"ques_start2");
	cfg->sigh_sym=wtk_local_cfg_find_array_s(lc,"sigh_sym");
	cfg->sigh_char=wtk_local_cfg_find_array_s(lc,"sigh_char");

	wtk_local_cfg_update_cfg_str(lc,cfg,pick_list,v);
	if (cfg->pick_list){
		cfg->pick_h = wtk_str_hash_new_file(cfg->pick_list);
	}

	return 0;
}

int wtk_tts_segsnt_cfg_update(wtk_tts_segsnt_cfg_t *cfg)
{
	return 0;
}

int wtk_tts_segsnt_cfg_update2(wtk_tts_segsnt_cfg_t *cfg,wtk_source_loader_t *sl)
{
	return 0;
}

