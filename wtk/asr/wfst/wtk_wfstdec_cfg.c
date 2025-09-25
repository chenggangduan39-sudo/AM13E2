#include "wtk_wfstdec_cfg.h"
#include "wtk/core/wtk_os.h"



int wtk_wfstdec_cfg_init(wtk_wfstdec_cfg_t *cfg)
{
	wtk_ebnfdec2_cfg_init(&(cfg->ebnfdec2));
	wtk_rescore_cfg_init(&(cfg->rescore));
	wtk_vad_cfg_init(&(cfg->vad));
	wtk_fextra_cfg_init(&(cfg->extra));
#ifdef USE_DNNC
	wtk_dnnc_cfg_init(&(cfg->dnnc));
#endif
	wtk_hmmset_cfg_init(&(cfg->hmmset));
	wtk_fst_net_cfg_init(&(cfg->net));
	wtk_usrec_cfg_init(&(cfg->usrec));
	wtk_fst_net_cfg_init(&(cfg->usrnet));
	wtk_wfstrec_cfg_init(&(cfg->rec));
	wtk_wfstdec_output_cfg_init(&(cfg->output));
	wtk_xbnf_rec_cfg_init(&(cfg->xbnf));
	cfg->label=0;
	cfg->label_slot_hint=80017;
	cfg->use_vad=0;
	cfg->use_rescore=0;
	cfg->flush_parm=0;
	cfg->use_mt=0;
	cfg->pad_sil_start_time=0;
	cfg->pad_sil_end_time=0;
	cfg->sil_end_data=0;
	cfg->sil_start_data=0;
	cfg->debug=0;
	cfg->pad_sil_vad_end_time=0;
	cfg->sil_vad_end_data=NULL;
	cfg->usr_net=NULL;
	cfg->cfile=NULL;
	cfg->rbin=NULL;
	cfg->use_ebnf=0;
	cfg->use_usrec=0;
	cfg->usrec_is_active=0;
	cfg->usr_bin=NULL;
	cfg->use_ebnfdec2=0;
	cfg->use_dnnc=0;
	cfg->use_xbnf=0;
	return 0;
}

int wtk_wfstdec_cfg_clean(wtk_wfstdec_cfg_t *cfg)
{
	if(cfg->use_xbnf)
	{
		wtk_xbnf_rec_cfg_clean(&(cfg->xbnf));
	}
#ifdef USE_DNNC
	if(cfg->use_dnnc)
	{
		wtk_dnnc_cfg_clean(&(cfg->dnnc));
	}
#endif
	if(cfg->use_ebnfdec2)
	{
		wtk_ebnfdec2_cfg_clean(&(cfg->ebnfdec2));
	}
	if(cfg->usr_net)
	{
		wtk_fst_net_delete(cfg->usr_net);
	}
	wtk_wfstdec_output_cfg_clean(&(cfg->output));
	if(cfg->sil_start_data)
	{
		wtk_string_delete(cfg->sil_start_data);
	}
	if(cfg->sil_end_data)
	{
		wtk_string_delete(cfg->sil_end_data);
	}
	if(cfg->sil_vad_end_data)
	{
		wtk_string_delete(cfg->sil_vad_end_data);
	}
	wtk_rescore_cfg_clean(&(cfg->rescore));
	if(cfg->use_vad)
	{
		wtk_vad_cfg_clean(&(cfg->vad));
	}
	wtk_fextra_cfg_clean(&(cfg->extra));
	wtk_hmmset_cfg_clean(&(cfg->hmmset));
	wtk_fst_net_cfg_clean(&(cfg->net));
	cfg->usrnet.sym_in=NULL;
	wtk_fst_net_cfg_clean(&(cfg->usrnet));
	wtk_wfstrec_cfg_clean(&(cfg->rec));
	wtk_usrec_cfg_clean(&(cfg->usrec));
	if(cfg->label)
	{
		wtk_label_delete(cfg->label);
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


int wtk_wfstdec_cfg_bytes(wtk_wfstdec_cfg_t *cfg)
{
	int bytes=0;
	int t;

	//t=wtk_fst_rescore_cfg_bytes(&(cfg->rescore));
	//wtk_debug("rescore: %fMB\n",t*1.0/(1024*1024));
	bytes=0;
	t=wtk_fextra_cfg_bytes(&(cfg->extra));
	wtk_debug("parm: %fMB\n",t*1.0/(1024*1024));
	bytes+=t;
	t=wtk_hmmset_cfg_bytes(&(cfg->hmmset));
	wtk_debug("hmmset: %fMB\n",t*1.0/(1024*1024));
	bytes+=t;
	t=wtk_fst_net_cfg_bytes(&(cfg->net));
	wtk_debug("net: %fMB\n",t*1.0/(1024*1024));
	bytes+=t;
	t=wtk_wfstrec_cfg_bytes(&(cfg->rec));
	wtk_debug("rec: %fMB\n",t*1.0/(1024*1024));
	bytes+=t;
	if(cfg->label)
	{
		t=wtk_label_bytes(cfg->label);
		wtk_debug("label: %fMB\n",t*1.0/(1024*1024));
		bytes+=t;
	}
	wtk_debug("bytes=%fMB\n",bytes*1.0/(1024*1024));
	return bytes;
}


int wtk_wfstdec_cfg_update_local(wtk_wfstdec_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	//wtk_local_cfg_print(lc);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_xbnf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnfdec2,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_usrec,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,label_slot_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_rescore,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,flush_parm,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_mt,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,usr_bin,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pad_sil_end_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pad_sil_start_time,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,pad_sil_vad_end_time,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,debug,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_dnnc,v);
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
	if(cfg->use_dnnc)
	{
		cfg->extra.use_dnn=0;
	}
#ifdef USE_DNNC
	lc=wtk_local_cfg_find_lc_s(main,"dnnc");
	if(lc)
	{
		ret=wtk_dnnc_cfg_update_local(&(cfg->dnnc),lc);
		if(ret!=0){goto end;}
	}
#endif
	if(cfg->use_rescore)
	{
		lc=wtk_local_cfg_find_lc_s(main,"rescore");
		if(lc)
		{
			ret=wtk_rescore_cfg_update_local(&(cfg->rescore),lc);
			if(ret!=0){goto end;}
		}
	}
	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"usrnet");
	//wtk_debug("lc=%p\n",lc);
	//wtk_local_cfg_print(main);
	//wtk_local_cfg_print(lc);
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->usrnet),lc);
		if(ret!=0){goto end;}
		//wtk_debug("usr_net=%f\n",cfg->usrnet.lmscale);
	}
	lc=wtk_local_cfg_find_lc_s(main,"rec");
	if(lc)
	{
		ret=wtk_wfstrec_cfg_update_local(&(cfg->rec),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"hmmset");
	if(lc)
	{
		ret=wtk_hmmset_cfg_update_local(&(cfg->hmmset),lc);
		if(ret!=0){goto end;}
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
			if(ret!=0){goto end;}
		}
	}
	lc=wtk_local_cfg_find_lc_s(main,"output");
	if(lc)
	{
		ret=wtk_wfstdec_output_cfg_update_local(&(cfg->output),lc);
		if(ret!=0){goto end;}
	}else
	{
		ret=wtk_wfstdec_output_cfg_update_local(&(cfg->output),main);
		if(ret!=0){goto end;}
	}
	if(cfg->use_usrec)
	{
		lc=wtk_local_cfg_find_lc_s(main,"usrec");
		if(lc)
		{
			ret=wtk_usrec_cfg_update_local(&(cfg->usrec),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_ebnfdec2)
	{
		lc=wtk_local_cfg_find_lc_s(main,"ebnfdec2");
		if(lc)
		{
			ret=wtk_ebnfdec2_cfg_update_local(&(cfg->ebnfdec2),lc);
			if(ret!=0){goto end;}
		}
	}
	if(cfg->use_xbnf)
	{
		lc=wtk_local_cfg_find_lc_s(main,"xbnf");
		if(lc)
		{
			ret=wtk_xbnf_rec_cfg_update_local(&(cfg->xbnf),lc);
			if(ret!=0){goto end;}
		}
	}
	ret=0;
end:
	//wtk_local_cfg_print(main);
	//wtk_debug("start=%d/%d\n",cfg->pad_sil_start_time,cfg->pad_sil_end_time);
	//exit(0);
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

int wtk_wfstdec_cfg_update(wtk_wfstdec_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;
	return wtk_wfstdec_cfg_update2(cfg,&(sl));
}

int wtk_wfstdec_cfg_update2(wtk_wfstdec_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	if(cfg->use_vad)
	{
		ret=wtk_vad_cfg_update2(&(cfg->vad),sl);
		if(ret!=0)
		{
			//wtk_debug("update vad2 failed\n");
			goto end;
		}
	}
	cfg->label=wtk_label_new(cfg->label_slot_hint);
	ret=wtk_hmmset_cfg_update2(&(cfg->hmmset),cfg->label,sl);
	if(ret!=0)
    {
        wtk_debug("update hmmset failed\n");
        goto end;
    }
	//wtk_hmmset_transpose_trans_matrix(cfg->hmmset.hmmset);
	ret=wtk_fextra_cfg_update2(&(cfg->extra),sl);
	if(ret!=0)
    {
        wtk_debug("update parm failed\n");
        goto end;
    }
#ifdef USE_DNNC
	if(cfg->use_dnnc)
	{
		ret=wtk_dnnc_cfg_update(&(cfg->dnnc));
		if(ret!=0){goto end;}
	}
#endif
	ret=wtk_fst_net_cfg_update3(&(cfg->net),cfg->label,sl);
	if(ret!=0)
    {
        wtk_debug("update net failed\n");
        goto end;
    }
	cfg->rec.frame_dur=cfg->extra.frame_dur;
	ret=wtk_wfstrec_cfg_update2(&(cfg->rec),sl);
	if(ret!=0)
    {
        wtk_debug("update rec failed\n");
        goto end;
    }
	wtk_wfstrec_cfg_update_hmmset(&(cfg->rec),cfg->hmmset.hmmset);
	if(cfg->use_rescore)
	{
		ret=wtk_rescore_cfg_update2(&(cfg->rescore),sl);
		if(ret!=0)
		{
            wtk_debug("update rescore failed\n");
			goto end;
		}
		wtk_rescore_set_sym_out(&(cfg->rescore),cfg->net.sym_out);
	}
	if(cfg->use_usrec)
	{
		ret=wtk_usrec_cfg_update2(&(cfg->usrec),sl,cfg->label);
		if(ret!=0)
		{
            wtk_debug("update usrec failed\n");
			goto end;
		}
		cfg->usrec.rec.frame_dur=cfg->extra.frame_dur;
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
	if(cfg->use_xbnf)
	{
		ret=wtk_xbnf_rec_cfg_update(&(cfg->xbnf));
		if(ret!=0)
		{
            wtk_debug("update xbnf rec failed\n");
			goto end;
		}
	}
	ret=wtk_wfstdec_output_cfg_update2(&(cfg->output),sl);
	if(ret!=0){goto end;}
	if(cfg->pad_sil_end_time>0)
	{
		int len;

		len=cfg->pad_sil_end_time*wtk_fextra_cfg_get_sample_rate(&(cfg->extra))/1000.0;
		cfg->sil_end_data=wtk_string_new(len);
		memset(cfg->sil_end_data->data,0,cfg->sil_end_data->len);
	}

	if(cfg->pad_sil_start_time>0)
	{
		int len;

		len=cfg->pad_sil_start_time*wtk_fextra_cfg_get_sample_rate(&(cfg->extra))/1000.0;
		cfg->sil_start_data=wtk_string_new(len);
		memset(cfg->sil_start_data->data,0,cfg->sil_start_data->len);
	}
	if(cfg->pad_sil_vad_end_time>0)
	{
		int len;

		len=cfg->pad_sil_vad_end_time*wtk_fextra_cfg_get_sample_rate(&(cfg->extra))/1000.0;
		cfg->sil_vad_end_data=wtk_string_new(len);
		memset(cfg->sil_vad_end_data->data,0,cfg->sil_vad_end_data->len);
	}
	if(cfg->usr_bin)
	{
		ret=wtk_wfstdec_cfg_set_ebnf_net(cfg,cfg->usr_bin);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_wfstdec_cfg_set_usrnet_ebnf_net(wtk_wfstdec_cfg_t *cfg,char *fn)
{
	wtk_rbin2_t *rb;
	wtk_source_loader_t sl;
	int ret;
	wtk_fst_net_cfg_t *net_cfg=&(cfg->usrnet);
	//wtk_fst_net_cfg_t *net_cfg=&(cfg->net);

	//wtk_debug("net_cfg=%f\n",net_cfg->lmscale);
	//wtk_debug("use_bin=%d/%d\n",cfg->usrnet.use_bin,cfg->net.use_bin);
	//net_cfg->use_bin=0;
	rb=wtk_rbin2_new();
	ret=wtk_rbin2_read(rb,fn);
	if(ret!=0){goto end;}
	sl.hook=rb;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	//wtk_debug("load sym out\n");
	net_cfg->symbol_out_fn="./final.out.r";
	net_cfg->sym_out=wtk_fst_sym_new3(cfg->label,net_cfg->symbol_out_fn,&sl,1);
	net_cfg->use_bin=0;
	net_cfg->use_rbin=1;
	net_cfg->bin.rbin=rb;
	if(!net_cfg->sym_out){ret=-1;goto end;}
	net_cfg->sym_in=cfg->net.sym_in;
	cfg->usr_net=wtk_fst_net_new(net_cfg);
	//wtk_debug("load fsm\n");
	ret=wtk_rbin2_load_file(rb,cfg->usr_net,(wtk_source_load_handler_t)wtk_fst_net_load_rbin,"./fsm");
	if(ret!=0){goto end;}
	ret=0;
end:
	//wtk_debug("load usr net end\n");
	//exit(0);
	wtk_rbin2_delete(rb);
	return ret;
}

int wtk_wfstdec_cfg_set_usrec_ebnf_net(wtk_wfstdec_cfg_t *cfg,char *fn)
{
	wtk_rbin2_t *rb;
	wtk_source_loader_t sl;
	int ret;
	wtk_fst_net_cfg_t *net_cfg=&(cfg->usrec.net);
	//wtk_fst_net_cfg_t *net_cfg=&(cfg->net);
	wtk_fst_net_t *net;

	//wtk_debug("net_cfg=%f\n",net_cfg->lmscale);
	//wtk_debug("use_bin=%d/%d\n",cfg->usrnet.use_bin,cfg->net.use_bin);
	//net_cfg->use_bin=0;
	rb=wtk_rbin2_new();
	ret=wtk_rbin2_read(rb,fn);
	if(ret!=0){goto end;}
	sl.hook=rb;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	//wtk_debug("load sym out\n");
	net_cfg->symbol_out_fn="./final.out.r";
	net_cfg->sym_out=wtk_fst_sym_new3(cfg->label,net_cfg->symbol_out_fn,&sl,1);
	net_cfg->use_bin=0;
	net_cfg->use_rbin=1;
	net_cfg->bin.rbin=rb;
	if(!net_cfg->sym_out){ret=-1;goto end;}
	//net_cfg->sym_in=cfg->net.sym_in;
	cfg->usrec.usr_net=net=wtk_fst_net_new(net_cfg);
	//wtk_debug("load fsm\n");
	ret=wtk_rbin2_load_file(rb,net,(wtk_source_load_handler_t)wtk_fst_net_load_rbin,"./fsm");
	if(ret!=0){goto end;}
	ret=0;
end:
	//wtk_debug("load usr net end\n");
	//exit(0);
	wtk_rbin2_delete(rb);
	return ret;
}


int wtk_wfstdec_cfg_set_ebnf_net(wtk_wfstdec_cfg_t *cfg,char *fn)
{
	int ret;

	//wtk_debug("fn=%s use_usrec=%d\n",fn,cfg->use_usrec);
	if(cfg->use_usrec)
	{
		ret=wtk_wfstdec_cfg_set_usrec_ebnf_net(cfg,fn);
		if(ret==0)
		{
			cfg->usrec_is_active=1;
		}else
		{
			wtk_debug("[compile %s failed]\n",fn);
		}
	}else
	{
		ret=wtk_wfstdec_cfg_set_usrnet_ebnf_net(cfg,fn);
	}
	return ret;
}


void wtk_wfstdec_cfg_delete_bin(wtk_wfstdec_cfg_t *cfg)
{
	wtk_wfstdec_cfg_clean(cfg);
	wtk_free(cfg);
}

wtk_wfstdec_cfg_t* wtk_wfstdec_cfg_new_bin(char *bin_fn)
{
	wtk_wfstdec_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./fst.cfg.r";
	int ret;

	//wtk_debug("read bin %s.",bin_fn);
	cfg=(wtk_wfstdec_cfg_t*)wtk_malloc(sizeof(wtk_wfstdec_cfg_t));
	wtk_wfstdec_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read(cfg->rbin,bin_fn);
	if(ret!=0)
	{
		wtk_debug("read failed %s\n",bin_fn);
		goto end;
	}
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item)
	{
		cfg_fn="./wfst.r";
		item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	}
	if(!item)
	{
		wtk_debug("%s not found %s\n",cfg_fn,bin_fn);
		ret=-1;goto end;
	}
	cfg->cfile=wtk_cfg_file_new();
	//wtk_debug("f=%p\n",cfg->rbin->f);
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=wtk_wfstdec_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	//cfg->net.use_rbin=1;
	cfg->net.bin.use_rbin=1;
	cfg->usrnet.bin.use_rbin=1;
	//wtk_debug("bin=%p\n",&(cfg->net.bin));
	cfg->rescore.lm.main_lm.kv.nglm.rbin=cfg->rbin;
	cfg->rescore.lm.main_lm.kv.rnn.rbin=cfg->rbin;
	//cfg->use_rescore=0;
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_wfstdec_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_wfstdec_cfg_delete_bin(cfg);
		cfg=NULL;
	}
	return cfg;
}

wtk_wfstdec_cfg_t* wtk_wfstdec_cfg_new_bin2(char *bin_fn,unsigned int seek_pos)
{
	wtk_wfstdec_cfg_t *cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./fst.cfg.r";
	int ret;

	cfg=(wtk_wfstdec_cfg_t*)wtk_malloc(sizeof(wtk_wfstdec_cfg_t));
	wtk_wfstdec_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	ret=wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
	if(ret!=0)
	{
		wtk_debug("read failed\n");
		goto end;
	}
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	if(!item)
	{
		wtk_debug("%s not found %s\n",cfg_fn,bin_fn);
		ret=-1;goto end;
	}
	cfg->cfile=wtk_cfg_file_new();
	//wtk_debug("f=%p\n",cfg->rbin->f);
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	ret=wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	if(ret!=0){goto end;}
	ret=wtk_wfstdec_cfg_update_local(cfg,cfg->cfile->main);
	if(ret!=0){goto end;}
	//cfg->net.use_rbin=1;
	cfg->net.bin.use_rbin=1;
	cfg->usrnet.bin.use_rbin=1;
	//wtk_debug("bin=%p\n",&(cfg->net.bin));
	cfg->rescore.lm.main_lm.kv.nglm.rbin=cfg->rbin;
	cfg->rescore.lm.main_lm.kv.rnn.rbin=cfg->rbin;
	//cfg->use_rescore=0;
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	ret=wtk_wfstdec_cfg_update2(cfg,&sl);
	if(ret!=0){goto end;}
end:
	//wtk_debug("ret=%d\n",ret);
	if(ret!=0)
	{
		wtk_wfstdec_cfg_delete_bin(cfg);
		cfg=NULL;
	}
	return cfg;
}

wtk_main_cfg_t* wtk_wfstdec_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;

	main_cfg=wtk_main_cfg_new_type(wtk_wfstdec_cfg,cfg_fn);
	return main_cfg;
}

void wtk_wfstdec_cfg_delete(wtk_main_cfg_t *main_cfg)
{
	wtk_main_cfg_delete(main_cfg);
}

