#include "qtk_decoder_wrapper_cfg.h"
#include "wtk/core/wtk_os.h"



int qtk_decoder_wrapper_cfg_init(qtk_decoder_wrapper_cfg_t *cfg)
{
	wtk_xvprint_cfg_init(&(cfg->xvprint));
	wtk_lex_cfg_init(&(cfg->lex));
	qtk_kwfstdec_cfg_init(&(cfg->kwfstdec));
	qtk_kwfstdec_cfg_init(&(cfg->kwfstdec2));
    wtk_vad_cfg_init(&(cfg->vad));
	wtk_ebnfdec2_cfg_init(&(cfg->ebnfdec2));
	wtk_fextra_cfg_init(&(cfg->extra));
	wtk_kxparm_cfg_init(&(cfg->parm));
	wtk_chnlike_cfg_init(&(cfg->chnlike));
	wtk_xbnf_rec_cfg_init(&(cfg->xbnf));
	wtk_version_cfg_init(&(cfg->version),"0.0.13");
	wtk_string_set_s(&(cfg->res),"qdreamer.0.1.1");
	cfg->flush_parm=0;
	cfg->asr_route=1;
	cfg->use_mt=0;
	cfg->use_ebnfdec2=0;
	cfg->cfile=NULL;
	cfg->rbin=NULL;
    cfg->use_vad=0;
    cfg->use_xvprint=0;
    cfg->use_lex=0;
    cfg->use_lite=0;
    cfg->use_xbnf=0;
    cfg->use_kxparm=0;
	return 0;
}

int qtk_decoder_wrapper_cfg_clean(qtk_decoder_wrapper_cfg_t *cfg)
{
	if(cfg->use_ebnfdec2)
	{
		wtk_ebnfdec2_cfg_clean(&(cfg->ebnfdec2));
	}
    if(cfg->use_vad)
    {
        wtk_vad_cfg_clean(&(cfg->vad));
    }
	if(cfg->use_xvprint)
	{
		wtk_xvprint_cfg_clean(&(cfg->xvprint));
	}
	if(cfg->use_lex)
	{
		wtk_lex_cfg_clean(&(cfg->lex));
	}
	if(cfg->use_kxparm)
	{
		wtk_kxparm_cfg_clean(&(cfg->parm));
	}else
	{
		wtk_fextra_cfg_clean(&(cfg->extra));
	}
	qtk_kwfstdec_cfg_clean(&(cfg->kwfstdec));
	qtk_kwfstdec_cfg_clean(&(cfg->kwfstdec2));
	wtk_chnlike_cfg_clean(&(cfg->chnlike));
	wtk_version_cfg_clean(&(cfg->version));
	if(cfg->use_xbnf)
	{
		wtk_xbnf_rec_cfg_clean(&(cfg->xbnf));
	}
	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}
	if(cfg->rbin)
	{
		wtk_rbin2_delete(cfg->rbin);
	}

	return 0;
}


int qtk_decoder_wrapper_cfg_bytes(qtk_decoder_wrapper_cfg_t *cfg)
{
	int bytes=0;

	return bytes;
}


int qtk_decoder_wrapper_cfg_update_local(qtk_decoder_wrapper_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_xvprint,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_lex,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_lite,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_xbnf,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,use_kxparm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,flush_parm,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,asr_route,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mt,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnfdec2,v);
	if(cfg->use_ebnfdec2)
	{
		lc=wtk_local_cfg_find_lc_s(main,"ebnfdec2");
		if(lc)
		{
			ret=wtk_ebnfdec2_cfg_update_local(&(cfg->ebnfdec2),lc);
			if(ret!=0){goto end;}
		}
	}	

    if(cfg->use_vad)
    {
        lc=wtk_local_cfg_find_lc_s(main,"vad");
        if(!lc)
        {
            lc=wtk_local_cfg_find_lc_s(main,"vad2");
        }
        if(lc)
        {
            ret=wtk_vad_cfg_update_local(&(cfg->vad),lc);
            if(ret!=0){wtk_debug("failed load vad\n");goto end;}
        }
    }

	if(cfg->use_xvprint)
	{
		lc=wtk_local_cfg_find_lc_s(main,"xvprint");
	   	if(lc)
 	  	{
			ret=wtk_xvprint_cfg_update_local(&(cfg->xvprint),lc);
			if(ret!=0){wtk_debug("failed load xvprint\n");goto end;}
    	}
	}

	if(cfg->use_lex)
	{
		lc=wtk_local_cfg_find_lc_s(main,"lex");
	   	if(lc)
 	  	{
			ret=wtk_lex_cfg_update_local(&(cfg->lex),lc);
			if(ret!=0){wtk_debug("failed load lex\n");goto end;}
    	}
	}

    if(cfg->use_kxparm)
    {
    	lc=wtk_local_cfg_find_lc_s(main,"kxparm");
    	if(lc)
    	{
    		ret=wtk_kxparm_cfg_update_local(&(cfg->parm),lc);
    		if(ret!=0){goto end;}
    	}
    }else
    {
		lc=wtk_local_cfg_find_lc_s(main,"extra");
		if(!lc)
		{
			lc=wtk_local_cfg_find_lc_s(main,"parm");
		}
		if(lc)
		{
			ret=wtk_fextra_cfg_update_local(&(cfg->extra),lc);
			if(ret!=0){goto end;}
		}
	}

	lc=wtk_local_cfg_find_lc_s(main,"kaldidec");
	if(lc)
	{
		ret=qtk_kwfstdec_cfg_update_local(&(cfg->kwfstdec),lc);
		if(ret!=0){goto end;}
	}

	lc=wtk_local_cfg_find_lc_s(main,"kaldidec2");
	if(lc)
	{
		ret=qtk_kwfstdec_cfg_update_local(&(cfg->kwfstdec2),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"chnlike");
	if(lc)
	{
		ret=wtk_chnlike_cfg_update_local(&(cfg->chnlike),lc);
		if(ret!=0){goto end;}
	}

	if(cfg->use_xbnf)
	{
		lc=wtk_local_cfg_find_lc_s(main,"xbnf_rec");
		if(lc)
		{
			ret=wtk_xbnf_rec_cfg_update_local(&(cfg->xbnf),lc);
			if(ret!=0){goto end;}
		}
	}

	wtk_local_cfg_update_cfg_str(lc,cfg,lex_fn,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,res,v);
	ret=0;
end:
	return ret;
}

int qtk_decoder_wrapper_cfg_update(qtk_decoder_wrapper_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	return qtk_decoder_wrapper_cfg_update2(cfg,&(sl));
}

int qtk_decoder_wrapper_cfg_update2(qtk_decoder_wrapper_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	
    if(cfg->use_vad)
    {
        ret=wtk_vad_cfg_update2(&(cfg->vad),sl);
        if(ret!=0)
        {
            wtk_debug("update vad2 failed\n");
            goto end;
        }
    }

	if(cfg->use_xvprint)
	{
		ret=wtk_xvprint_cfg_update2(&(cfg->xvprint),sl);
	}

	if(cfg->use_lex)
	{
		ret=wtk_lex_cfg_update2(&(cfg->lex),sl);
	}

	if(cfg->use_ebnfdec2)
	{
		ret=wtk_ebnfdec2_cfg_update(&(cfg->ebnfdec2));
		if(ret!=0)
		{
       		wtk_debug("update ebnfdec2 failed\n");
			goto end;
		}
	}

	if(cfg->use_kxparm)
	{
		ret=wtk_kxparm_cfg_update2(&(cfg->parm),sl);
	    if(ret!=0)
		{
			wtk_debug("update kxparm failed\n");
	        	goto end;
		}
	}else
	{
		//wtk_hmmset_transpose_trans_matrix(cfg->hmmset.hmmset);
		ret=wtk_fextra_cfg_update2(&(cfg->extra),sl);
	    if(ret!=0)
		{
			wtk_debug("update parm failed\n");
	        	goto end;
		}
	}

	ret=qtk_kwfstdec_cfg_update(&(cfg->kwfstdec), sl);
	if(ret!=0)
	{
        	wtk_debug("update kwfstdec failed\n");
        	goto end;
   	}

	ret=qtk_kwfstdec_cfg_update(&(cfg->kwfstdec2), sl);
	if(ret!=0)
	{
        	wtk_debug("update kwfstdec2 failed\n");
        	goto end;
   	}
	ret=wtk_chnlike_cfg_update(&(cfg->chnlike));
	if(ret!=0)
	{
        	wtk_debug("update chnlike failed\n");
        	goto end;
	}

	if(cfg->use_xbnf)
	{
		ret=wtk_xbnf_rec_cfg_update(&(cfg->xbnf));
		if(ret!=0)
		{
            wtk_debug("update xbnf rec failed\n");
			goto end;
		}
	}

	end:
		return ret;
}

wtk_main_cfg_t* qtk_decoder_wrapper_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_decoder_wrapper_cfg,cfg_fn);
	return main_cfg;
}

qtk_decoder_wrapper_cfg_t* qtk_decoder_wrapper_cfg_new_bin(char *bin_fn)
{
	qtk_decoder_wrapper_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
        char *cfg_fn = "./main.cfg";

        cfg=(qtk_decoder_wrapper_cfg_t*)wtk_malloc(sizeof(qtk_decoder_wrapper_cfg_t));
	qtk_decoder_wrapper_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read(cfg->rbin,bin_fn);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	qtk_decoder_wrapper_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	qtk_decoder_wrapper_cfg_update2(cfg,&sl);
	return cfg;
}

qtk_decoder_wrapper_cfg_t* qtk_decoder_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos)
{
    qtk_decoder_wrapper_cfg_t* cfg;
    wtk_rbin2_item_t *item;
    wtk_source_loader_t sl;
    char *cfg_fn = "./main.cfg";

    cfg=(qtk_decoder_wrapper_cfg_t*)wtk_malloc(sizeof(qtk_decoder_wrapper_cfg_t));
    qtk_decoder_wrapper_cfg_init(cfg);
    cfg->rbin=wtk_rbin2_new();
    wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
    item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
    cfg->cfile=wtk_cfg_file_new();
    wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
    wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
    qtk_decoder_wrapper_cfg_update_local(cfg,cfg->cfile->main);
    sl.hook=cfg->rbin;
    sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
    qtk_decoder_wrapper_cfg_update2(cfg,&sl);
    return cfg;
}

void qtk_decoder_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg)
{
	wtk_main_cfg_delete(main_cfg);
}

void qtk_decoder_wrapper_cfg_delete_bin(qtk_decoder_wrapper_cfg_t *cfg)
{
	qtk_decoder_wrapper_cfg_clean(cfg);
	wtk_free(cfg);
}
