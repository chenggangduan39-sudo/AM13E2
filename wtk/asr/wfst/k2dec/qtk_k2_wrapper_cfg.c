#include "qtk_k2_wrapper_cfg.h"

#include "wtk/core/wtk_os.h"



int qtk_k2_wrapper_cfg_init(qtk_k2_wrapper_cfg_t *cfg)
{
#ifdef ONNX_DEC
	qtk_onnxruntime_cfg_init(&(cfg->encoder));
	qtk_onnxruntime_cfg_init(&(cfg->decoder));
	qtk_onnxruntime_cfg_init(&(cfg->joiner));
#endif
	wtk_xbnfnet_cfg_init(&(cfg->xbnfnet));
	wtk_lex_cfg_init(&(cfg->lex));
	qtk_kwfstdec_cfg_init(&(cfg->kwfstdec));
	wtk_vad_cfg_init(&(cfg->vad));
	wtk_kxparm_cfg_init(&(cfg->parm));
	wtk_chnlike_cfg_init(&(cfg->chnlike));
	wtk_xbnf_rec_cfg_init(&(cfg->xbnf));
	wtk_version_cfg_init(&(cfg->version),"0.0.13");
	wtk_fst_net_cfg_init(&(cfg->net));
	qtk_punctuation_prediction_cfg_init(&(cfg->pp));
	wtk_string_set_s(&(cfg->res),"qdreamer.0.1.1");
	cfg->cfile=NULL;
	cfg->rbin=NULL;
	cfg->rbin2=NULL;
	cfg->use_vad=0;
	cfg->use_lex=0;
	cfg->use_lite=0;
	cfg->use_xbnf=0;
	cfg->use_context=0;
	cfg->keyword_detect = 0;
	cfg->use_stream=0;
	cfg->use_pp=0;
	cfg->beam=4;
	cfg->chunk=32;
	cfg->sym_fn = 0;
	cfg->sym = 0;
	cfg->sym2_fn = 0;
	cfg->sym2 = 0;
	cfg->subsample = 4;
	cfg->right_context = 0;
	cfg->left_context = 256;
	cfg->hint_len = 320 * 32;
	cfg->search_method = 0;
	cfg->blank_penalty = -3.0;
	cfg->hot_gain = 1.0;
	cfg->filter_result = 0;
	cfg->use_eps_feat = 0;
	cfg->method = QTK_K2_BEAM_SEARCH;//default: beam search
	cfg->label = 0;
	cfg->label2 = 0;
	cfg->dict = 0;
	cfg->dict_fn = 0;

	cfg->conf = -4.5;
	cfg->wrd_speed = 2.0;
	cfg->aver_amprob = -4.0;
	cfg->interval = 1.2;

	cfg->use_trick = 0;
	cfg->use_last_state = 0;
	cfg->norm_conf = -4.5;
	cfg->idle_conf = -3.5;
	cfg->idle_hint = 8000;
	cfg->need_reset = 0;
	cfg->last_outid = -1;

	cfg->phn_hash_hint=257;
	cfg->wrd_hash_hint=25007;
	cfg->use_hc_wakeup = 0;
	cfg->use_hc_asr = 0;
	cfg->use_ebnf = 0;
	return 0;
}

int qtk_k2_wrapper_cfg_clean(qtk_k2_wrapper_cfg_t *cfg)
{
	wtk_xbnfnet_cfg_clean(&(cfg->xbnfnet));
	if(cfg->use_vad)
	{
		wtk_vad_cfg_clean(&(cfg->vad));
	}
	if(cfg->use_lex)
	{
		wtk_lex_cfg_clean(&(cfg->lex));
	}
	if(cfg->dict)
	{
		wtk_dict_delete(cfg->dict);
	}
#ifdef ONNX_DEC
	qtk_onnxruntime_cfg_clean(&(cfg->encoder));
	qtk_onnxruntime_cfg_clean(&(cfg->decoder));
	qtk_onnxruntime_cfg_clean(&(cfg->joiner));
#endif
	wtk_kxparm_cfg_clean(&(cfg->parm));

	qtk_kwfstdec_cfg_clean(&(cfg->kwfstdec));
	wtk_chnlike_cfg_clean(&(cfg->chnlike));
	wtk_version_cfg_clean(&(cfg->version));
	wtk_fst_net_cfg_clean(&(cfg->net));
	qtk_punctuation_prediction_cfg_clean(&(cfg->pp));
	if(cfg->use_xbnf)
	{
		wtk_xbnf_rec_cfg_clean(&(cfg->xbnf));
	}

	if(cfg->sym)
	{
		//wtk_debug("delete sym_out=%p\n",cfg->sym_out);
		wtk_fst_sym_delete(cfg->sym);
		wtk_label_delete(cfg->label);
	}

	if(cfg->sym2)
	{
		//wtk_debug("delete sym_out=%p\n",cfg->sym_out);
		wtk_fst_sym_delete(cfg->sym2);
		wtk_label_delete(cfg->label2);
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


int qtk_k2_wrapper_cfg_bytes(qtk_k2_wrapper_cfg_t *cfg)
{
	int bytes=0;

	return bytes;
}


int qtk_k2_wrapper_cfg_update_local(qtk_k2_wrapper_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;

	wtk_local_cfg_update_cfg_b(lc,cfg,use_vad,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lex,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_lite,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_xbnf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_context,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,keyword_detect,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_stream,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_pp,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,filter_result,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eps_feat,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,sym_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,sym2_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,dict_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,search_method,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,chunk,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,hint_len,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,blank_penalty,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,hot_gain,v);

	wtk_local_cfg_update_cfg_f(lc,cfg,conf,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,wrd_speed,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,aver_amprob,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,interval,v);

    wtk_local_cfg_update_cfg_f(lc,cfg,norm_conf,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,idle_conf,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_trick,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,idle_hint,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_last_state,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,need_reset,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hc_wakeup,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_hc_asr,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ebnf,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,last_outid,v);

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

	lc=wtk_local_cfg_find_lc_s(main,"kxparm");
	if(lc)
	{
		ret=wtk_kxparm_cfg_update_local(&(cfg->parm),lc);
		if(ret!=0){goto end;}
	}
#ifdef ONNX_DEC
	lc=wtk_local_cfg_find_lc_s(main,"encoder");
	if(lc)
	{
		ret=qtk_onnxruntime_cfg_update_local(&(cfg->encoder),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"decoder");
	if(lc)
	{
		ret=qtk_onnxruntime_cfg_update_local(&(cfg->decoder),lc);
		if(ret!=0){goto end;}
	}
	lc=wtk_local_cfg_find_lc_s(main,"joiner");
	if(lc)
	{
		ret=qtk_onnxruntime_cfg_update_local(&(cfg->joiner),lc);
		if(ret!=0){goto end;}
	}
#endif
	lc=wtk_local_cfg_find_lc_s(main,"dec");
	if(lc)
	{
		ret=qtk_kwfstdec_cfg_update_local(&(cfg->kwfstdec),lc);
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

	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}

	if(cfg->use_pp)
	{
		lc=wtk_local_cfg_find_lc_s(main,"pp");
		if(lc)
		{
			ret=qtk_punctuation_prediction_cfg_update_local(&(cfg->pp),lc);
			if(ret!=0){goto end;}
		}
	}

	if(cfg->use_ebnf){
		lc=wtk_local_cfg_find_lc_s(main,"xbnfnet");
		if(lc)
		{
			ret=wtk_xbnfnet_cfg_update_local(&(cfg->xbnfnet),lc);
			if(ret!=0){goto end;}
		}
	}

	wtk_local_cfg_update_cfg_str(lc,cfg,lex_fn,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,res,v);
	ret=0;
end:
	return ret;
}

int qtk_k2_wrapper_cfg_update(qtk_k2_wrapper_cfg_t *cfg)
{
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	return qtk_k2_wrapper_cfg_update2(cfg,&(sl));
}

void qtk_k2_wrapper_cfg_update_method(qtk_k2_wrapper_cfg_t *cfg){
		
	cfg->method = QTK_K2_WFST_SEARCH;
	switch(cfg->search_method)
	{
	case 0:
		cfg->method = QTK_K2_BEAM_SEARCH;
		break;
	case 1:
		cfg->method = QTK_K2_WFST_SEARCH;
		break;
	case 3:
		cfg->method = QTK_K2_KWS_ASR;
		break;
	default:
		break;
	}
}

int qtk_k2_wrapper_cfg_update2(qtk_k2_wrapper_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret;

	qtk_k2_wrapper_cfg_update_method(cfg);
	cfg->rbin2 = sl->hook;
	if(cfg->use_vad)
	{
		ret=wtk_vad_cfg_update2(&(cfg->vad),sl);
		if(ret!=0)
		{
			wtk_debug("update vad2 failed\n");
			goto end;
		}
	}

	wtk_label_t *label;

	if(cfg->sym_fn)
	{
		label=wtk_label_new(25007);
		cfg->label=label;
		cfg->sym=wtk_fst_sym_new3(label,cfg->sym_fn,sl,0);
		if(!cfg->sym)
		{
			wtk_debug("load sym[%s] failed\n",cfg->sym_fn);
			ret=-1;goto end;
		}
	}
	wtk_label_t *label2;

	if(cfg->sym2_fn)
	{
		label2=wtk_label_new(25007);
		cfg->label2=label2;
		cfg->sym2=wtk_fst_sym_new3(label2,cfg->sym2_fn,sl,0);
		if(!cfg->sym2)
		{
			wtk_debug("load sym[%s] failed\n",cfg->sym2_fn);
			ret=-1;goto end;
		}
	}

	if(cfg->use_lex)
	{
		ret=wtk_lex_cfg_update2(&(cfg->lex),sl);
	}

	ret=wtk_kxparm_cfg_update2(&(cfg->parm),sl);
	if(ret!=0)
	{
		wtk_debug("update kxparm failed\n");
			goto end;
	}

	ret=qtk_kwfstdec_cfg_update(&(cfg->kwfstdec), sl);
	if(ret!=0)
	{
			wtk_debug("update kwfstdec failed\n");
			goto end;
	}

	ret=wtk_chnlike_cfg_update(&(cfg->chnlike));
	if(ret!=0)
	{
			wtk_debug("update chnlike failed\n");
			goto end;
	}

#ifdef ONNX_DEC
	ret=qtk_onnxruntime_cfg_update2(&(cfg->encoder),sl->hook);
	ret=qtk_onnxruntime_cfg_update2(&(cfg->decoder),sl->hook);
	ret=qtk_onnxruntime_cfg_update2(&(cfg->joiner),sl->hook);
	if(ret!=0)
	{
		wtk_debug("update onnx failed\n");
		goto end;
	}
#endif
	if(cfg->use_xbnf)
	{
		ret=wtk_xbnf_rec_cfg_update(&(cfg->xbnf));
		if(ret!=0)
		{
			wtk_debug("update xbnf rec failed\n");
			goto end;
		}
	}

	if(cfg->use_pp)
	{
		ret=qtk_punctuation_prediction_cfg_update2(&(cfg->pp),sl);
		if(ret!=0)
		{
			wtk_debug("update pp rec failed\n");
			goto end;
		}
	}

	if(cfg->use_ebnf){
		ret=wtk_xbnfnet_cfg_update2(&(cfg->xbnfnet), sl);
		if(ret!=0){goto end;}
	}

	ret=wtk_fst_net_cfg_update3(&(cfg->net),NULL,sl);
	if(ret!=0)
	{
		wtk_debug("update net failed\n");
		goto end;
	}

	end:
		return ret;
}

int qtk_k2_wrapper_cfg_update3(qtk_k2_wrapper_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb)
{
	int ret;

	qtk_k2_wrapper_cfg_update_method(cfg);
	cfg->rbin2 = sl->hook;
	if(cfg->use_vad)
	{
		ret=wtk_vad_cfg_update2(&(cfg->vad),sl);
		if(ret!=0)
		{
			wtk_debug("update vad2 failed\n");
			goto end;
		}
	}

	wtk_label_t *label;

	if(cfg->sym_fn)
	{
		label=wtk_label_new(25007);
		cfg->label=label;
		cfg->sym=wtk_fst_sym_new3(label,cfg->sym_fn,sl,0);
		if(!cfg->sym)
		{
			wtk_debug("load sym[%s] failed\n",cfg->sym_fn);
			ret=-1;goto end;
		}
	}
	wtk_label_t *label2;

	if(cfg->sym2_fn)
	{
		label2=wtk_label_new(25007);
		cfg->label2=label2;
		cfg->sym2=wtk_fst_sym_new3(label2,cfg->sym2_fn,sl,0);
		if(!cfg->sym2)
		{
			wtk_debug("load sym[%s] failed\n",cfg->sym2_fn);
			ret=-1;goto end;
		}
	}

	if(cfg->use_lex)
	{
		ret=wtk_lex_cfg_update2(&(cfg->lex),sl);
	}

	ret=wtk_kxparm_cfg_update2(&(cfg->parm),sl);
	if(ret!=0)
	{
		wtk_debug("update kxparm failed\n");
			goto end;
	}


	ret=qtk_kwfstdec_cfg_update(&(cfg->kwfstdec), sl);
	if(ret!=0)
	{
			wtk_debug("update kwfstdec failed\n");
			goto end;
	}

	ret=wtk_chnlike_cfg_update(&(cfg->chnlike));
	if(ret!=0)
	{
			wtk_debug("update chnlike failed\n");
			goto end;
	}
#ifdef ONNX_DEC
	ret=qtk_onnxruntime_cfg_update2(&(cfg->encoder),rb);
	ret=qtk_onnxruntime_cfg_update2(&(cfg->decoder),rb);
	ret=qtk_onnxruntime_cfg_update2(&(cfg->joiner),rb);
	if(ret!=0)
	{
			wtk_debug("update onnx failed\n");
			goto end;
	}
#endif
	if(cfg->use_xbnf)
	{
		ret=wtk_xbnf_rec_cfg_update(&(cfg->xbnf));
		if(ret!=0)
		{
			wtk_debug("update xbnf rec failed\n");
			goto end;
		}
	}

	if(cfg->use_pp)
	{
		ret=qtk_punctuation_prediction_cfg_update3(&(cfg->pp),sl,rb);
		if(ret!=0)
		{
			wtk_debug("update pp failed\n");
			goto end;
		}
	}

	if(cfg->use_ebnf){
		ret=wtk_xbnfnet_cfg_update2(&(cfg->xbnfnet), sl);
		if(ret!=0){goto end;}
	}

	ret=wtk_fst_net_cfg_update3(&(cfg->net),NULL,sl);
	if(ret!=0)
	{
		wtk_debug("update net failed\n");
		goto end;
	}

	end:
		return ret;
}

wtk_main_cfg_t* qtk_k2_wrapper_cfg_new(char *cfg_fn)
{
	wtk_main_cfg_t *main_cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_k2_wrapper_cfg,cfg_fn);
	return main_cfg;
}

qtk_k2_wrapper_cfg_t* qtk_k2_wrapper_cfg_new_bin(char *bin_fn)
{
	qtk_k2_wrapper_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./asr.cfg";

	cfg=(qtk_k2_wrapper_cfg_t*)wtk_malloc(sizeof(qtk_k2_wrapper_cfg_t));
	qtk_k2_wrapper_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read(cfg->rbin,bin_fn);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	qtk_k2_wrapper_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	qtk_k2_wrapper_cfg_update3(cfg,&sl,cfg->rbin);
	return cfg;
}

qtk_k2_wrapper_cfg_t* qtk_k2_wrapper_cfg_new_bin2(char *bin_fn,unsigned int seek_pos)
{
	qtk_k2_wrapper_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./asr.cfg";

	cfg=(qtk_k2_wrapper_cfg_t*)wtk_malloc(sizeof(qtk_k2_wrapper_cfg_t));
	qtk_k2_wrapper_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	qtk_k2_wrapper_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	qtk_k2_wrapper_cfg_update2(cfg,&sl);
	return cfg;
}

void qtk_k2_wrapper_cfg_delete(wtk_main_cfg_t *main_cfg)
{
	wtk_main_cfg_delete(main_cfg);
}

void qtk_k2_wrapper_cfg_delete_bin(qtk_k2_wrapper_cfg_t *cfg)
{
	qtk_k2_wrapper_cfg_clean(cfg);
	wtk_free(cfg);
}
