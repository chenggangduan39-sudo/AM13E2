#include "qtk_punctuation_prediction_cfg.h"

#include "wtk/core/wtk_os.h"



int qtk_punctuation_prediction_cfg_init(qtk_punctuation_prediction_cfg_t *cfg){
#ifdef ONNX_DEC
	qtk_onnxruntime_cfg_init(&(cfg->model));
#endif
	cfg->rbin=NULL;
    cfg->cfile=NULL;
	cfg->sym = 0;
    cfg->sym_fn = 0;

	return 0;
}

int qtk_punctuation_prediction_cfg_clean(qtk_punctuation_prediction_cfg_t *cfg){
#ifdef ONNX_DEC
	qtk_onnxruntime_cfg_clean(&(cfg->model));
#endif
	if(cfg->sym){
		//wtk_debug("delete sym_out=%p\n",cfg->sym_out);
		wtk_fst_sym_delete(cfg->sym);
		wtk_label_delete(cfg->label);
	}
	if(cfg->rbin){
		wtk_rbin2_delete(cfg->rbin);
	}
    if(cfg->cfile){
        wtk_cfg_file_delete(cfg->cfile);
    }

	return 0;
}


int qtk_punctuation_prediction_cfg_bytes(qtk_punctuation_prediction_cfg_t *cfg){
	int bytes=0;

	return bytes;
}


int qtk_punctuation_prediction_cfg_update_local(qtk_punctuation_prediction_cfg_t *cfg,wtk_local_cfg_t *main){
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
    wtk_local_cfg_update_cfg_str(lc,cfg,sym_fn,v);

#ifdef ONNX_DEC
	lc=wtk_local_cfg_find_lc_s(main,"model");
	if(lc){
		ret=qtk_onnxruntime_cfg_update_local(&(cfg->model),lc);
		if(ret!=0){goto end;}
	}
#endif
	ret=0;
end:
	return ret;
}

int qtk_punctuation_prediction_cfg_update(qtk_punctuation_prediction_cfg_t *cfg){
	wtk_source_loader_t sl;

	sl.hook=0;
	sl.vf=wtk_source_load_file_v;

	return qtk_punctuation_prediction_cfg_update2(cfg,&(sl));
}


int qtk_punctuation_prediction_cfg_update2(qtk_punctuation_prediction_cfg_t *cfg,wtk_source_loader_t *sl){
	int ret;

	wtk_label_t *label;

	label=wtk_label_new(25007);
	cfg->label=label;

	if(cfg->sym_fn){
		cfg->sym=wtk_fst_sym_new3(label,cfg->sym_fn,sl,0);
		if(!cfg->sym){
			wtk_debug("load sym[%s] failed\n",cfg->sym_fn);
			ret=-1;goto end;
		}
	}
	
#ifdef ONNX_DEC
	ret=qtk_onnxruntime_cfg_update2(&(cfg->model),NULL);
	if(ret!=0){
		wtk_debug("update onnx failed\n");
		goto end;
	}
#endif

	end:
		return ret;
}

int qtk_punctuation_prediction_cfg_update3(qtk_punctuation_prediction_cfg_t *cfg,wtk_source_loader_t *sl,wtk_rbin2_t *rb){
	int ret;

	wtk_label_t *label;

	label=wtk_label_new(25007);
	cfg->label=label;

	if(cfg->sym_fn){
		cfg->sym=wtk_fst_sym_new3(label,cfg->sym_fn,sl,0);
		if(!cfg->sym){
			wtk_debug("load sym[%s] failed\n",cfg->sym_fn);
			ret=-1;goto end;
		}
	}

#ifdef ONNX_DEC
	ret=qtk_onnxruntime_cfg_update2(&(cfg->model),rb);
	if(ret!=0){
		wtk_debug("update onnx failed\n");
		goto end;
	}
#endif
	end:
		return ret;
}

wtk_main_cfg_t* qtk_punctuation_prediction_cfg_new(char *cfg_fn){
	wtk_main_cfg_t *main_cfg;

	main_cfg=wtk_main_cfg_new_type(qtk_punctuation_prediction_cfg,cfg_fn);
	return main_cfg;
}

qtk_punctuation_prediction_cfg_t* qtk_punctuation_prediction_cfg_new_bin(char *bin_fn){
	qtk_punctuation_prediction_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./asr.cfg";

	cfg=(qtk_punctuation_prediction_cfg_t*)wtk_malloc(sizeof(qtk_punctuation_prediction_cfg_t));
	qtk_punctuation_prediction_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read(cfg->rbin,bin_fn);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	qtk_punctuation_prediction_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	qtk_punctuation_prediction_cfg_update3(cfg,&sl,cfg->rbin);
	return cfg;
}

qtk_punctuation_prediction_cfg_t* qtk_punctuation_prediction_cfg_new_bin2(char *bin_fn,unsigned int seek_pos){
	qtk_punctuation_prediction_cfg_t* cfg;
	wtk_rbin2_item_t *item;
	wtk_source_loader_t sl;
	char *cfg_fn="./asr.cfg";

	cfg=(qtk_punctuation_prediction_cfg_t*)wtk_malloc(sizeof(qtk_punctuation_prediction_cfg_t));
	qtk_punctuation_prediction_cfg_init(cfg);
	cfg->rbin=wtk_rbin2_new();
	wtk_rbin2_read2(cfg->rbin,bin_fn,seek_pos);
	item=wtk_rbin2_get2(cfg->rbin,cfg_fn,strlen(cfg_fn));
	cfg->cfile=wtk_cfg_file_new();
	wtk_cfg_file_add_var_ks(cfg->cfile,"pwd",".",1);
	wtk_cfg_file_feed(cfg->cfile,item->data->data,item->data->len);
	qtk_punctuation_prediction_cfg_update_local(cfg,cfg->cfile->main);
	sl.hook=cfg->rbin;
	sl.vf=(wtk_source_loader_v_t)wtk_rbin2_load_file;
	qtk_punctuation_prediction_cfg_update2(cfg,&sl);
	return cfg;
}

void qtk_punctuation_prediction_cfg_delete(wtk_main_cfg_t *main_cfg){
	wtk_main_cfg_delete(main_cfg);
}

void qtk_punctuation_prediction_cfg_delete_bin(qtk_punctuation_prediction_cfg_t *cfg){
	qtk_punctuation_prediction_cfg_clean(cfg);
	wtk_free(cfg);
}
