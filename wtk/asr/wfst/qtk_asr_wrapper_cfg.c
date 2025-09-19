#include "qtk_asr_wrapper_cfg.h"
#include "wtk/core/wtk_os.h"


int qtk_asr_wrapper_cfg_init(qtk_asr_wrapper_cfg_t *cfg)
{
	wtk_lex_cfg_init(&(cfg->lex));
	wtk_vad_cfg_init(&(cfg->vad));
	wtk_chnlike_cfg_init(&(cfg->chnlike));
	wtk_version_cfg_init(&(cfg->version),"0.0.13");
	wtk_string_set_s(&(cfg->res),"qdreamer.0.1.1");
	qtk_k2_wrapper_cfg_init(&(cfg->k2));
	qtk_wenet_wrapper_cfg_init(&(cfg->wenet));
	qtk_decoder_wrapper_cfg_init(&cfg->kaldi);
	qtk_k2_wrapper_cfg_init(&(cfg->kwake));
	cfg->cfile=NULL;
	cfg->rbin=NULL;
	cfg->lex_fn=0;
	cfg->asr_fn = 0;
	cfg->wakeup_fn = 0;
	cfg->asr_type=0;
	cfg->use_vad=0;
	cfg->use_wake=0;
	cfg->use_lex=0;
	cfg->asr_buf = 0;
	cfg->wakeup_buf = 0;
	cfg->hook = NULL;
	cfg->contact_e = 0;
	cfg->contact_b = 0;
	cfg->place_e = 0;
	cfg->place_b = 0;
	cfg->use_hc_asr = 0;
	return 0;
}

int qtk_asr_wrapper_cfg_clean(qtk_asr_wrapper_cfg_t *cfg)
{
	if(cfg->use_vad)
	{
		wtk_vad_cfg_clean(&(cfg->vad));
	}
	if(cfg->use_lex)
	{
		wtk_lex_cfg_clean(&(cfg->lex));
	}

	wtk_chnlike_cfg_clean(&(cfg->chnlike));
	wtk_version_cfg_clean(&(cfg->version));

	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}

	if(cfg->rbin)
	{
		wtk_rbin2_delete(cfg->rbin);
	}

	qtk_k2_wrapper_cfg_clean(&(cfg->k2));
	qtk_wenet_wrapper_cfg_clean(&(cfg->wenet));
	qtk_decoder_wrapper_cfg_clean(&(cfg->kaldi));
	if(cfg->use_wake)
	{
		qtk_k2_wrapper_cfg_clean(&(cfg->kwake));
	}
	if(cfg->asr_buf){
		wtk_strbuf_delete(cfg->asr_buf);
	}
	if(cfg->wakeup_buf){
		wtk_strbuf_delete(cfg->wakeup_buf);
	}
	return 0;
}


int qtk_asr_wrapper_cfg_bytes(qtk_asr_wrapper_cfg_t *cfg)
{
	int bytes=0;

	return bytes;
}

static qtk_asr_decoder_type qtk_asr_wrapper_cfg_dec_init(char *type)
{
	int ret = -1;

    if(wtk_str_equal_s(type,strlen(type),"K2"))
    {
            ret = QTK_K2;
    }else if(wtk_str_equal_s(type,strlen(type),"K"))
    {
            ret = QTK_KALDI;
    }else if(wtk_str_equal_s(type,strlen(type),"WENET"))
    {
            ret = QTK_WENET;
    }else
    {
    	exit(0);
    }


	return ret;
}

int qtk_asr_wrapper_cfg_update_local(qtk_asr_wrapper_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lex,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_wake,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hc_asr,v);
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

	if(cfg->use_lex)
	{
		lc=wtk_local_cfg_find_lc_s(main,"lex");
	   	if(lc)
 	  	{
			ret=wtk_lex_cfg_update_local(&(cfg->lex),lc);
			if(ret!=0){wtk_debug("failed load lex\n");goto end;}
	    	}
	}

	lc=wtk_local_cfg_find_lc_s(main,"chnlike");
	if(lc)
	{
		ret=wtk_chnlike_cfg_update_local(&(cfg->chnlike),lc);
		if(ret!=0){goto end;}
	}

	wtk_local_cfg_update_cfg_str(main,cfg,lex_fn,v);
	wtk_local_cfg_update_cfg_str(main,cfg,asr_fn,v);
	wtk_local_cfg_update_cfg_str(main,cfg,wakeup_fn,v);
	wtk_local_cfg_update_cfg_str(main,cfg,asr_type,v);
	wtk_local_cfg_update_cfg_string_v(main,cfg,res,v);
	cfg->type = qtk_asr_wrapper_cfg_dec_init(cfg->asr_type);
	switch(cfg->type)
	{
	case QTK_K2:
		lc=wtk_local_cfg_find_lc_s(main,"k2");
		if(lc)
		{
			ret=qtk_k2_wrapper_cfg_update_local(&(cfg->k2),lc);
			if(ret!=0){goto end;}
		}
		break;
	case QTK_KALDI:
		lc=wtk_local_cfg_find_lc_s(main,"k");
		if(lc)
		{
			ret=qtk_decoder_wrapper_cfg_update_local(&(cfg->kaldi),lc);
			if(ret!=0){goto end;}
		}
		break;
		break;
	case QTK_WENET:
		lc=wtk_local_cfg_find_lc_s(main,"wenet");
		if(lc)
		{
			ret=qtk_wenet_wrapper_cfg_update_local(&(cfg->wenet),lc);
			if(ret!=0){goto end;}
		}
		break;
	default:
		break;
	}

	if(cfg->use_wake){
		lc=wtk_local_cfg_find_lc_s(main,"kwake");
		if(lc)
		{
			ret=qtk_k2_wrapper_cfg_update_local(&(cfg->kwake),lc);
			if(ret!=0){goto end;}
		}
	}

	cfg->contact_e=wtk_local_cfg_find_array_s(lc,"contact_e");
	cfg->contact_b=wtk_local_cfg_find_array_s(lc,"contact_b");
	cfg->place_e=wtk_local_cfg_find_array_s(lc,"place_e");
	cfg->place_b=wtk_local_cfg_find_array_s(lc,"place_b");

	ret=0;
end:
	return ret;
}

int qtk_asr_wrapper_cfg_update(qtk_asr_wrapper_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	return qtk_asr_wrapper_cfg_update2(cfg,&(sl));
}

int qtk_asr_wrapper_cfg_keyword_load(wtk_strbuf_t *buf,wtk_source_t *src){
	if(buf){
		wtk_source_read_file2(src,buf);
		return 0;
	}
	return -1;
}

int qtk_asr_wrapper_cfg_update2(qtk_asr_wrapper_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;
	
	if(cfg->use_vad)
	{
		ret=wtk_vad_cfg_update2(&(cfg->vad),sl);
		if(ret!=0)
		{
			wtk_debug("update vad failed\n");
			goto end;
		}
	}

	if(cfg->use_lex)
	{
		ret=wtk_lex_cfg_update2(&(cfg->lex),sl);
	}

	if(cfg->use_wake){
		ret = qtk_k2_wrapper_cfg_update2(&(cfg->kwake),sl);
		if(ret!=0)
		{
			wtk_debug("update wake failed\n");
			goto end;
		}
		if(cfg->wakeup_fn){
			cfg->wakeup_buf = wtk_strbuf_new(1024,1);
			ret = wtk_source_loader_load(sl,cfg->wakeup_buf,(wtk_source_load_handler_t)qtk_asr_wrapper_cfg_keyword_load,cfg->wakeup_fn);
		}
	}

	if(cfg->asr_fn){
		cfg->asr_buf = wtk_strbuf_new(1024,1);
		ret = wtk_source_loader_load(sl,cfg->asr_buf,(wtk_source_load_handler_t)qtk_asr_wrapper_cfg_keyword_load,cfg->asr_fn);
	}

	ret=wtk_chnlike_cfg_update(&(cfg->chnlike));
	if(ret!=0)
	{
        	wtk_debug("update chnlike failed\n");
        	goto end;
	}

	switch(cfg->type)
	{
	case QTK_K2:
		qtk_k2_wrapper_cfg_update2(&(cfg->k2),sl);
		break;
	case QTK_KALDI:
		qtk_decoder_wrapper_cfg_update2(&(cfg->kaldi),sl);
		break;
	case QTK_WENET:
		qtk_wenet_wrapper_cfg_update2(&(cfg->wenet),sl);
		break;
	default:
		break;
	}

	end:
		return ret;
}

int qtk_asr_wrapper_cfg_update3(qtk_asr_wrapper_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb)
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

	if(cfg->use_lex)
	{
		ret=wtk_lex_cfg_update2(&(cfg->lex),sl);
	}

	if(cfg->use_wake){
		ret = qtk_k2_wrapper_cfg_update3(&(cfg->kwake),sl,rb);
		if(ret!=0)
		{
			wtk_debug("update wake failed\n");
			goto end;
		}
		if(cfg->wakeup_fn){
			cfg->wakeup_buf = wtk_strbuf_new(1024,1);
			ret = wtk_source_loader_load(sl,cfg->wakeup_buf,(wtk_source_load_handler_t)qtk_asr_wrapper_cfg_keyword_load,cfg->wakeup_fn);
		}
	}

	if(cfg->asr_fn){
		cfg->asr_buf = wtk_strbuf_new(1024,1);
		ret = wtk_source_loader_load(sl,cfg->asr_buf,(wtk_source_load_handler_t)qtk_asr_wrapper_cfg_keyword_load,cfg->asr_fn);
	}

	ret=wtk_chnlike_cfg_update(&(cfg->chnlike));
	if(ret!=0)
	{
        	wtk_debug("update chnlike failed\n");
        	goto end;
	}

	switch(cfg->type)
	{
	case QTK_K2:
		qtk_k2_wrapper_cfg_update3(&(cfg->k2),sl,rb);
		break;
	case QTK_KALDI:
		qtk_decoder_wrapper_cfg_update2(&(cfg->kaldi),sl);
		break;
	case QTK_WENET:
		qtk_wenet_wrapper_cfg_update3(&(cfg->wenet),sl,rb);
		break;
	default:
		break;
	}

	end:
		return ret;
}

wtk_main_cfg_t* qtk_asr_wrapper_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_asr_wrapper_cfg,cfg_fn);
	return main_cfg;
}

qtk_asr_wrapper_cfg_t* qtk_asr_wrapper_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_asr_wrapper_cfg_t *cfg;

	mbin_cfg = wtk_mbin_cfg_new_type(qtk_asr_wrapper_cfg, bin_fn, "./asr.cfg");
	cfg = (qtk_asr_wrapper_cfg_t *)mbin_cfg->cfg;
	cfg->hook = mbin_cfg;
	return cfg;
}

qtk_asr_wrapper_cfg_t* qtk_asr_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos)
{
    qtk_asr_wrapper_cfg_t* cfg;
    wtk_rbin2_item_t *item;
    wtk_source_loader_t sl;
    char *cfg_fn="./asr.cfg";

    cfg=(qtk_asr_wrapper_cfg_t*)wtk_malloc(sizeof(qtk_asr_wrapper_cfg_t));
    qtk_asr_wrapper_cfg_init(cfg);
    cfg->rbin=wtk_rbin2_new();
    wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
    item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
    cfg->cfile=wtk_cfg_file_new();
    wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
    wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
    qtk_asr_wrapper_cfg_update_local(cfg,cfg->cfile->main);
    sl.hook=cfg->rbin;
    sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
    qtk_asr_wrapper_cfg_update2(cfg,&sl);
    return cfg;
}

void qtk_asr_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg)
{
	wtk_main_cfg_delete(main_cfg);
}

void qtk_asr_wrapper_cfg_delete_bin(qtk_asr_wrapper_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t *)cfg->hook);
}
