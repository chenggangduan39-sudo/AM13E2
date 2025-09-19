#include "qtk_wakeup_wrapper_cfg.h"
#include "wtk/core/wtk_os.h"


int qtk_wakeup_wrapper_cfg_init(qtk_wakeup_wrapper_cfg_t *cfg)
{
	wtk_vad_cfg_init(&(cfg->vad));
	wtk_version_cfg_init(&(cfg->version),"0.0.13");
	wtk_string_set_s(&(cfg->res),"qdreamer.0.1.1");
	qtk_k2_wrapper_cfg_init(&(cfg->kwake));
	wtk_kwdec2_cfg_init(&(cfg->kwdec2));
	qtk_img_cfg_init(&(cfg->img));
	cfg->cfile=NULL;
	cfg->rbin=NULL;
	cfg->wakeup_fn = 0;
	cfg->wakeup_type=0;
	cfg->use_vad = 0;
	cfg->use_vad2 = 0;
	cfg->wakeup_buf = 0;
	cfg->hook = NULL;
	return 0;
}

int qtk_wakeup_wrapper_cfg_clean(qtk_wakeup_wrapper_cfg_t *cfg)
{
	if(cfg->use_vad)
	{
		wtk_vad_cfg_clean(&(cfg->vad));
	}

	wtk_version_cfg_clean(&(cfg->version));

	if(cfg->cfile)
	{
		wtk_cfg_file_delete(cfg->cfile);
	}

	if(cfg->rbin)
	{
		wtk_rbin2_delete(cfg->rbin);
	}

	qtk_k2_wrapper_cfg_clean(&(cfg->kwake));
	wtk_kwdec2_cfg_clean(&(cfg->kwdec2));
	qtk_img_cfg_clean(&(cfg->img));
	if(cfg->wakeup_buf){
		wtk_strbuf_delete(cfg->wakeup_buf);
	}
	return 0;
}


int qtk_wakeup_wrapper_cfg_bytes(qtk_wakeup_wrapper_cfg_t *cfg)
{
	int bytes=0;

	return bytes;
}

static qtk_wakeup_type qtk_wakeup_wrapper_cfg_dec_init(char *type)
{
	int ret = -1;

	if(wtk_str_equal_s(type,strlen(type),"K2"))
	{
		ret = QTK_K2;
	}else if(wtk_str_equal_s(type,strlen(type),"KWDEC"))
	{
		ret = QTK_KWDEC;
	}else if(wtk_str_equal_s(type,strlen(type),"IMG"))
	{
		ret = QTK_IMG;
	}else
	{
		exit(0);
	}
	return ret;
}

int qtk_wakeup_wrapper_cfg_update_local(qtk_wakeup_wrapper_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad2,v);

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

	wtk_local_cfg_update_cfg_str(main,cfg,wakeup_fn,v);
	wtk_local_cfg_update_cfg_str(main,cfg,wakeup_type,v);
	wtk_local_cfg_update_cfg_string_v(main,cfg,res,v);
	cfg->type = qtk_wakeup_wrapper_cfg_dec_init(cfg->wakeup_type);
	switch(cfg->type)
	{
	case QTK_K2:
		lc=wtk_local_cfg_find_lc_s(main,"k2wake");
		if(lc)
		{
			ret=qtk_k2_wrapper_cfg_update_local(&(cfg->kwake),lc);
			if(ret!=0){goto end;}
		}
	case QTK_KWDEC:
		lc=wtk_local_cfg_find_lc_s(main,"kwdec2");
		if(lc)
		{
			ret=wtk_kwdec2_cfg_update_local(&(cfg->kwdec2),lc);
			if(ret!=0){goto end;}
		}
		break;
	case QTK_IMG:
		lc=wtk_local_cfg_find_lc_s(main,"img");
		if(lc)
		{
			ret=qtk_img_cfg_update_local(&(cfg->img),lc);
			if(ret!=0){goto end;}
		}
		break;
	default:
		break;
	}

	ret=0;
end:
	return ret;
}

int qtk_wakeup_wrapper_cfg_update(qtk_wakeup_wrapper_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	return qtk_wakeup_wrapper_cfg_update2(cfg,&(sl));
}

int qtk_wakeup_wrapper_cfg_keyword_load(wtk_strbuf_t *buf,wtk_source_t *src){
	if(buf){
		wtk_source_read_file2(src,buf);
		return 0;
	}
	return -1;
}

int qtk_wakeup_wrapper_cfg_update2(qtk_wakeup_wrapper_cfg_t *cfg,wtk_source_loader_t *sl)
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

	switch(cfg->type)
	{
	case QTK_K2:
		ret = qtk_k2_wrapper_cfg_update2(&(cfg->kwake),sl);
		if(ret!=0)
		{
			wtk_debug("update wake failed\n");
			goto end;
		}
		if(cfg->wakeup_fn){
			cfg->wakeup_buf = wtk_strbuf_new(1024,1);
			ret = wtk_source_loader_load(sl,cfg->wakeup_buf,(wtk_source_load_handler_t)qtk_wakeup_wrapper_cfg_keyword_load,cfg->wakeup_fn);
		}
		break;
	case QTK_KWDEC:
		ret = wtk_kwdec2_cfg_update2(&(cfg->kwdec2),sl);
		if(ret!=0)
		{
			wtk_debug("update kwdec2 failed\n");
			goto end;
		}
		break;
	case QTK_IMG:
		ret = qtk_img_cfg_update2(&(cfg->img),sl);
		if(ret!=0)
		{
			wtk_debug("update img failed\n");
			goto end;
		}
		break;
	default:
		break;
	}

	end:
		return ret;
}

int qtk_wakeup_wrapper_cfg_update3(qtk_wakeup_wrapper_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb)
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

	switch(cfg->type)
	{
	case QTK_K2:
		ret = qtk_k2_wrapper_cfg_update3(&(cfg->kwake),sl,rb);
		if(ret!=0)
		{
			wtk_debug("update wake failed\n");
			goto end;
		}
		if(cfg->wakeup_fn){
			cfg->wakeup_buf = wtk_strbuf_new(1024,1);
			ret = wtk_source_loader_load(sl,cfg->wakeup_buf,(wtk_source_load_handler_t)qtk_wakeup_wrapper_cfg_keyword_load,cfg->wakeup_fn);
		}
		break;
	case QTK_KWDEC:
		ret = wtk_kwdec2_cfg_update2(&(cfg->kwdec2),sl);
		if(ret!=0)
		{
			wtk_debug("update kwdec2 failed\n");
			goto end;
		}
		break;
	case QTK_IMG:
		ret = qtk_img_cfg_update2(&(cfg->img),sl);
		if(ret!=0)
		{
			wtk_debug("update img failed\n");
			goto end;
		}
		break;
	default:
		break;
	}

	end:
		return ret;
}

wtk_main_cfg_t* qtk_wakeup_wrapper_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_wakeup_wrapper_cfg,cfg_fn);
	return main_cfg;
}

qtk_wakeup_wrapper_cfg_t* qtk_wakeup_wrapper_cfg_new_bin(char *bin_fn)
{
	wtk_mbin_cfg_t *mbin_cfg;
	qtk_wakeup_wrapper_cfg_t *cfg;

	mbin_cfg = wtk_mbin_cfg_new_type(qtk_wakeup_wrapper_cfg, bin_fn, "./wakeup.cfg");
	cfg = (qtk_wakeup_wrapper_cfg_t *)mbin_cfg->cfg;
	cfg->hook = mbin_cfg;
	return cfg;
}

qtk_wakeup_wrapper_cfg_t* qtk_wakeup_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos)
{
	qtk_wakeup_wrapper_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./wakeup.cfg";

	cfg=(qtk_wakeup_wrapper_cfg_t*)wtk_malloc(sizeof(qtk_wakeup_wrapper_cfg_t));
	qtk_wakeup_wrapper_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	qtk_wakeup_wrapper_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	qtk_wakeup_wrapper_cfg_update2(cfg,&sl);
	return cfg;
}

void qtk_wakeup_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg)
{
	wtk_main_cfg_delete(main_cfg);
}

void qtk_wakeup_wrapper_cfg_delete_bin(qtk_wakeup_wrapper_cfg_t *cfg)
{
	wtk_mbin_cfg_delete((wtk_mbin_cfg_t *)cfg->hook);
}
