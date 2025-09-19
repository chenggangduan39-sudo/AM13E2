#include "qtk_tts_parse_cfg.h"

int qtk_tts_parse_cfg_init(qtk_tts_parse_cfg_t *cfg)
{
    wtk_tts_norm_cfg_init(&cfg->norm);
    wtk_tts_segsnt_cfg_init(&cfg->segsnt);
    wtk_tts_segwrd_cfg_init(&cfg->segwrd);
    qtk_tts_symbols_id_cfg_init(&cfg->symbols);
    qtk_tts_prosody_cfg_init(&cfg->prosody);
    qtk_tts_polyphone_cfg_init(&cfg->ply);
    qtk_tts_segwrd_cfg_init(&cfg->segwrd2);
    cfg->pool=NULL;
    cfg->forvits = 0;
    cfg->fordevicetts = 0;
    cfg->debug_norm = 0;
    cfg->debug_polyphone = 0;
    cfg->debug_prosody = 0;
    cfg->debug_segsnt = 0;
    cfg->debug_segwrd = 0;
    cfg->is_en = 0;
    cfg->use_segmax=0;
    return 0;
}
int qtk_tts_parse_cfg_clean(qtk_tts_parse_cfg_t *cfg)
{
    wtk_tts_norm_cfg_clean(&cfg->norm);
    wtk_tts_segsnt_cfg_clean(&cfg->segsnt);
    wtk_tts_segwrd_cfg_clean(&cfg->segwrd);
    qtk_tts_symbols_id_cfg_clean(&cfg->symbols);
    qtk_tts_prosody_cfg_clean(&cfg->prosody);
    qtk_tts_polyphone_cfg_clean(&cfg->ply);
    qtk_tts_segwrd_cfg_clean(&cfg->segwrd2);
	if(cfg->pool)
	{
		wtk_strpool_delete(cfg->pool);
	}
    return 0;
}

int qtk_tts_parse_cfg_update_local(qtk_tts_parse_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_local_cfg_t *main_cfg = NULL;
    wtk_string_t *v = NULL;
    main_cfg = lc;
    lc = wtk_local_cfg_find_lc_s(main_cfg,"norm");
    if(lc){
        wtk_tts_norm_cfg_update_local(&cfg->norm,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_cfg,"segsnt");
    if(lc){
        wtk_tts_segsnt_cfg_update_local(&cfg->segsnt,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_cfg,"symbols");
    if(lc){
        qtk_tts_symbols_id_cfg_update_local(&cfg->symbols,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_cfg,"polyphone");
    if(lc){
        qtk_tts_polyphone_cfg_update_local(&cfg->ply,lc);
    }
    lc = wtk_local_cfg_find_lc_s(main_cfg,"prosody");
    if(lc){
        qtk_tts_prosody_cfg_update_local(&cfg->prosody,lc);
    }
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,use_segmax,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,is_en,v);
    if (cfg->use_segmax || cfg->is_en)
    {
        lc = wtk_local_cfg_find_lc_s(main_cfg,"segwrd");
        if(lc){
            wtk_tts_segwrd_cfg_update_local(&cfg->segwrd,lc);
        }
    }else
    {
        lc = wtk_local_cfg_find_lc_s(main_cfg,"segwrd2");
        if(lc){
            qtk_tts_segwrd_cfg_update_local(&cfg->segwrd2,lc);
        }
    }

    wtk_local_cfg_update_cfg_i(main_cfg,cfg,debug_segsnt,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,debug_norm,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,debug_segwrd,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,debug_polyphone,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,debug_prosody,v);
    wtk_local_cfg_update_cfg_i(main_cfg,cfg,debug_id,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,forvits,v);
    wtk_local_cfg_update_cfg_b(main_cfg,cfg,fordevicetts,v);

    return 0;
}

int qtk_tts_parse_cfg_update(qtk_tts_parse_cfg_t *cfg)
{
    wtk_tts_norm_cfg_update(&cfg->norm);
    wtk_tts_segsnt_cfg_update(&cfg->segsnt);
    if(cfg->is_en || cfg->use_segmax){
        wtk_tts_segwrd_cfg_update(&cfg->segwrd);
    }else{
        qtk_tts_segwrd_cfg_update(&cfg->segwrd2);
    }
    qtk_tts_symbols_id_cfg_update(&cfg->symbols);
    qtk_tts_prosody_cfg_update(&cfg->prosody);
    qtk_tts_polyphone_cfg_update(&cfg->ply);
    return 0;
}

int qtk_tts_parse_cfg_update2(qtk_tts_parse_cfg_t *cfg, wtk_source_loader_t *sl)
{
	cfg->rbin = sl->hook;
	if(cfg->pool)
		cfg->pool=wtk_strpool_new(1507);
    wtk_tts_norm_cfg_update2(&cfg->norm, sl);
    wtk_tts_segsnt_cfg_update2(&cfg->segsnt, sl);
    if(cfg->is_en || cfg->use_segmax){
    	//wtk_debug("bytes=%f M\n",wtk_strpool_bytes(cfg->pool)*1.0/(1024*1024));
    	wtk_tts_segwrd_cfg_update2(&(cfg->segwrd),sl, cfg->pool);
    }else{
        qtk_tts_segwrd_cfg_update2(&(cfg->segwrd2), sl);
    }
    qtk_tts_symbols_id_cfg_update(&cfg->symbols);
    qtk_tts_prosody_cfg_update2(&cfg->prosody, sl);
    qtk_tts_polyphone_cfg_update2(&cfg->ply, sl);
    return 0;
}

#include "wtk/core/cfg/wtk_mbin_cfg.h"
qtk_tts_parse_cfg_t* qtk_tts_parse_cfg_new_bin(char *cfg_fn,int seek_pos)
{
	wtk_mbin_cfg_t *main_cfg;
	qtk_tts_parse_cfg_t *cfg;

	cfg=0;
	main_cfg=wtk_mbin_cfg_new_type2(seek_pos,qtk_tts_parse_cfg,cfg_fn,"./cfg");
	if(main_cfg){
		cfg=(qtk_tts_parse_cfg_t*)(main_cfg->cfg);
		cfg->mbin_cfg=main_cfg;
	}

	return cfg;
}

void qtk_tts_parse_cfg_delete_bin(qtk_tts_parse_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->mbin_cfg);
}

int qtk_tts_parse_cfg_delete(qtk_tts_parse_cfg_t *cfg)
{
	if(!cfg){return 0;}
	qtk_tts_parse_cfg_clean(cfg);
	wtk_free(cfg);
	return 0;
}
