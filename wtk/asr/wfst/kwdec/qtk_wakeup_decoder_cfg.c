#include "qtk_wakeup_decoder_cfg.h"
#include "wtk/core/wtk_os.h"



int qtk_wakeup_decoder_cfg_init(qtk_wakeup_decoder_cfg_t *cfg)
{
	qtk_kwdec_cfg_init(&(cfg->kwdec));
	qtk_kwdec_cfg_init(&(cfg->kwdec2));
    wtk_vad_cfg_init(&(cfg->vad));
	wtk_fextra_cfg_init(&(cfg->extra));
	wtk_version_cfg_init(&(cfg->version),"0.0.13");
	wtk_string_set_s(&(cfg->res),"qdreamer.0.1.1");
	cfg->flush_parm=0;
	cfg->asr_route=1;
	cfg->use_mt=0;
	cfg->cfile=NULL;
	cfg->rbin=NULL;
    cfg->use_vad=0;

	return 0;
}

int qtk_wakeup_decoder_cfg_clean(qtk_wakeup_decoder_cfg_t *cfg)
{
	if(cfg->feat_fn&&cfg->m)
	{
		wtk_free(cfg->m);
	}
    if(cfg->use_vad)
    {
        wtk_vad_cfg_clean(&(cfg->vad));
    }
	wtk_fextra_cfg_clean(&(cfg->extra));
	qtk_kwdec_cfg_clean(&(cfg->kwdec));
	qtk_kwdec_cfg_clean(&(cfg->kwdec2));
	wtk_version_cfg_clean(&(cfg->version));
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


int qtk_wakeup_decoder_cfg_bytes(qtk_wakeup_decoder_cfg_t *cfg)
{
	int bytes=0;

	return bytes;
}


int qtk_wakeup_decoder_cfg_update_local(qtk_wakeup_decoder_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

    wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,flush_parm,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,asr_route,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mt,v);

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
            if(ret!=0){wtk_debug("failed\n");goto end;}
        }
    }

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

	lc=wtk_local_cfg_find_lc_s(main,"kaldidec");
	if(lc)
	{
		ret=qtk_kwdec_cfg_update_local(&(cfg->kwdec),lc);
		if(ret!=0){goto end;}
	}

	lc=wtk_local_cfg_find_lc_s(main,"kaldidec2");
	if(lc)
	{
		ret=qtk_kwdec_cfg_update_local(&(cfg->kwdec2),lc);
		if(ret!=0){goto end;}
	}
	wtk_local_cfg_update_cfg_str(lc,cfg,feat_fn,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,res,v);
	ret=0;
end:
	return ret;
}

int qtk_wakeup_decoder_cfg_load_feat(qtk_wakeup_decoder_cfg_t *cfg,wtk_source_t *src)
{
	int ret;
	int cnt;
	//int v;
	wtk_matrix_t *m=0;
	//wtk_strbuf_t *buf;

	//buf=wtk_strbuf_new(256,1);
	ret=wtk_source_read_int(src,&cnt,1,0);
	cfg->f_cnt=cnt;

	m=wtk_matrix_new2(cnt,5373);
	ret=wtk_source_read_matrix(src,m,0);
//	wtk_matrix_print(m);
	cfg->m=m;

	return ret;
}

int qtk_wakeup_decoder_cfg_update(qtk_wakeup_decoder_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	if(cfg->feat_fn)
	{
		wtk_source_loader_load(&sl,cfg,(wtk_source_load_handler_t)qtk_wakeup_decoder_cfg_load_feat,cfg->feat_fn);
	}
	//wtk_debug("failed2.\n");

	return qtk_wakeup_decoder_cfg_update2(cfg,&(sl));
}

int qtk_wakeup_decoder_cfg_update2(qtk_wakeup_decoder_cfg_t *cfg,wtk_source_loader_t *sl)
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
	ret=wtk_fextra_cfg_update2(&(cfg->extra),sl);
        if(ret!=0)
	{
		wtk_debug("update parm failed\n");
        	goto end;
	}
	ret=qtk_kwdec_cfg_update(&(cfg->kwdec), sl);
	if(ret!=0)
	{
        	wtk_debug("update kwdec failed\n");
        	goto end;
   	}

	ret=qtk_kwdec_cfg_update(&(cfg->kwdec2), sl);
	if(ret!=0)
	{
        	wtk_debug("update kwdec2 failed\n");
        	goto end;
   	}
	end:
		return ret;
}

wtk_main_cfg_t* qtk_wakeup_decoder_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_wakeup_decoder_cfg,cfg_fn);
	return main_cfg;
}

qtk_wakeup_decoder_cfg_t* qtk_wakeup_decoder_cfg_new2(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;
	qtk_wakeup_decoder_cfg_t *cfg=NULL;

	main_cfg=wtk_main_cfg_new_type(qtk_wakeup_decoder_cfg,cfg_fn);
	if(!main_cfg){goto end;}
	cfg=(qtk_wakeup_decoder_cfg_t*)main_cfg->cfg;
	cfg->cfg.main_cfg=main_cfg;
end:
	return cfg;
}

qtk_wakeup_decoder_cfg_t* qtk_wakeup_decoder_cfg_new_bin(char *bin_fn)
{
	qtk_wakeup_decoder_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./kwdec.cfg";

	cfg=(qtk_wakeup_decoder_cfg_t*)wtk_malloc(sizeof(qtk_wakeup_decoder_cfg_t));
	qtk_wakeup_decoder_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read(cfg->rbin,bin_fn);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	qtk_wakeup_decoder_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	qtk_wakeup_decoder_cfg_update2(cfg,&sl);
	return cfg;
}

qtk_wakeup_decoder_cfg_t* qtk_wakeup_decoder_cfg_new_bin2(char *bin_fn,unsigned int seek_pos)
{
    qtk_wakeup_decoder_cfg_t* cfg;
    wtk_rbin2_item_t *item;
    wtk_source_loader_t sl;
    char *cfg_fn="./kwdec.cfg";

    cfg=(qtk_wakeup_decoder_cfg_t*)wtk_malloc(sizeof(qtk_wakeup_decoder_cfg_t));
    qtk_wakeup_decoder_cfg_init(cfg);
    cfg->rbin=wtk_rbin2_new();
    wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
    item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
    cfg->cfile=wtk_cfg_file_new();
    wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
    wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
    qtk_wakeup_decoder_cfg_update_local(cfg,cfg->cfile->main);
    sl.hook=cfg->rbin;
    sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
    qtk_wakeup_decoder_cfg_update2(cfg,&sl);
    return cfg;
}

void qtk_wakeup_decoder_cfg_delete(wtk_main_cfg_t *main_cfg)
{
	wtk_main_cfg_delete(main_cfg);
}

void qtk_wakeup_decoder_cfg_delete2(qtk_wakeup_decoder_cfg_t *cfg)
{
	wtk_main_cfg_delete(cfg->cfg.main_cfg);
}

void qtk_wakeup_decoder_cfg_delete_bin(qtk_wakeup_decoder_cfg_t *cfg)
{
	qtk_wakeup_decoder_cfg_clean(cfg);
	wtk_free(cfg);
}

void qtk_wakeup_decoder_cfg_delete_bin2(qtk_wakeup_decoder_cfg_t *cfg)
{
	wtk_mbin_cfg_delete(cfg->cfg.bin_cfg);
}
