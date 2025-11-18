#include "qtk_soundfield_syntheis_cfg.h" 

int qtk_soundfield_syntheis_cfg_init(qtk_soundfield_syntheis_cfg_t *cfg){
    cfg->positions = NULL;
    cfg->reference_position = NULL;
    cfg->source_position = NULL;
    cfg->freq_weight = NULL;
    cfg->freq_weight = NULL;
    cfg->hp_fn = NULL;
    cfg->lp_fn = NULL;
    cfg->weight_fn = NULL;
    cfg->n_pos = 0;
    cfg->hop_size = 256;
    cfg->fs = 16000;
    cfg->N = 12;
    cfg->bandsplit_on = 0;
    cfg->lp = 0;
    cfg->hp = 0;
    cfg->cutoff_freq = 2700;
    cfg->nref = 1;
    cfg->neval = 5;
    cfg->n_tap = 512;
    cfg->rt60 = 0.5;
    cfg->lambda = 1e-5;
    cfg->scale = 1.0;
    cfg->ref_spk_index = 7;
    cfg->weight_buf = NULL;
    cfg->nbin = -1;
    cfg->rir_channel = 12;
	return 0;
}

int qtk_soundfield_syntheis_cfg_clean(qtk_soundfield_syntheis_cfg_t *cfg){
	int i;
	if(cfg->positions){
        for(i = 0;i < cfg->n_pos;++i){
            wtk_free(cfg->positions[i]);
        }
        wtk_free(cfg->positions);
    }

    if(cfg->reference_position){
        wtk_free(cfg->reference_position);
    }

    if(cfg->source_position){
        wtk_free(cfg->source_position);
    }

    if(cfg->lambda_vector){
        wtk_free(cfg->lambda_vector);
    }

    if(cfg->freq_weight){
        wtk_free(cfg->freq_weight);
    }

    if(cfg->lp){
        wtk_vecf_delete(cfg->lp);
        wtk_vecf_delete(cfg->hp);
    }

    if(cfg->weight_buf){
        wtk_strbuf_delete(cfg->weight_buf);
    }
    return 0;
}

int qtk_soundfield_syntheis_load_vec(qtk_soundfield_syntheis_cfg_t *cfg, wtk_source_t *src){
    int ret,len;

    ret = wtk_source_read_int_little(src,&len,1,1);
    cfg->tmp = wtk_vecf_new(len);
    ret = wtk_source_read_float_little(src, cfg->tmp->p, cfg->tmp->len, 1);
    return ret;
}

int qtk_soundfield_syntheis_load_weight(wtk_strbuf_t *buf,wtk_source_t *src){
    if(buf){
        wtk_source_read_file2(src,buf);
        return 0;
    }
    return -1;
}


int qtk_soundfield_syntheis_cfg_update_local(qtk_soundfield_syntheis_cfg_t *cfg,wtk_local_cfg_t *m){
    wtk_string_t *v;
    wtk_local_cfg_t *lc;
    wtk_array_t *a;
    int i;

    lc = m;

    wtk_local_cfg_update_cfg_i(lc,cfg,rir_channel,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,hop_size,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,fs,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,N,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,neval,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,nref,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,cutoff_freq,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,n_tap,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,ref_spk_index,v);
    wtk_local_cfg_update_cfg_i(lc,cfg,nbin,v);
    wtk_local_cfg_update_cfg_b(lc,cfg,bandsplit_on,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,rt60,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,lambda,v);
    wtk_local_cfg_update_cfg_f(lc,cfg,scale,v);
    wtk_local_cfg_update_cfg_str_local(lc,cfg,hp_fn,v);
    wtk_local_cfg_update_cfg_str_local(lc,cfg,lp_fn,v);
    wtk_local_cfg_update_cfg_str_local(lc,cfg,weight_fn,v);

    a=wtk_local_cfg_find_array_s(lc,"reference_position");
    if(a){
        cfg->reference_position = (float*)wtk_malloc(sizeof(float)*a->nslot);
        for(i = 0;i < a->nslot;++i){
            v = ((wtk_string_t**)a->slot)[i];
            cfg->reference_position[i] = wtk_str_atof(v->data,v->len);
        }
    }
    a=wtk_local_cfg_find_array_s(lc,"source_position");
    if(a){
        cfg->source_position = (float*)wtk_malloc(sizeof(float)*a->nslot);
        for(i = 0;i < a->nslot;++i){
            v = ((wtk_string_t**)a->slot)[i];
            cfg->source_position[i] = wtk_str_atof(v->data,v->len);
        }
    }

    a=wtk_local_cfg_find_array_s(lc,"lambda_vector");
    if(a){
        cfg->lambda_vector = (float*)wtk_malloc(sizeof(float)*a->nslot);
        for(i = 0;i < a->nslot;++i){
            v = ((wtk_string_t**)a->slot)[i];
            cfg->lambda_vector[i] = wtk_str_atof(v->data,v->len);
        }
    }

    a=wtk_local_cfg_find_array_s(lc,"freq_weight");
    if(a){
        cfg->freq_weight = (float*)wtk_malloc(sizeof(float)*a->nslot);
        for(i = 0;i < a->nslot;++i){
            v = ((wtk_string_t**)a->slot)[i];
            cfg->freq_weight[i] = wtk_str_atof(v->data,v->len);
        }
    }

    lc=wtk_local_cfg_find_lc_s(m,"pos");
    if(lc)
    {
        wtk_queue_node_t *qn;
        wtk_cfg_item_t *item;

        cfg->positions = (float**)wtk_malloc(sizeof(float*)*lc->cfg->queue.length);
        cfg->n_pos = 0;
        for(qn=lc->cfg->queue.pop;qn;qn=qn->next){
            item=data_offset2(qn,wtk_cfg_item_t,n);
            if(item->type!=WTK_CFG_ARRAY || item->value.array->nslot!=3){continue;}
            cfg->positions[cfg->n_pos]=(float*)wtk_malloc(sizeof(float)*3);
            for(i=0;i<3;++i){
                v=((wtk_string_t**)item->value.array->slot)[i];
                cfg->positions[cfg->n_pos][i]=wtk_str_atof(v->data,v->len);
            }
            ++cfg->n_pos;
        }
    }

	return 0;
}

int qtk_soundfield_syntheis_cfg_update(qtk_soundfield_syntheis_cfg_t *cfg){
    int ret = 0;
    wtk_source_loader_t sl;

    sl.hook=0;
    sl.vf=wtk_source_load_file_v;

    if(cfg->lp_fn){
        ret = wtk_source_loader_load(&sl,cfg,(wtk_source_load_handler_t)qtk_soundfield_syntheis_load_vec,cfg->lp_fn);
        cfg->lp = cfg->tmp;
    }

    if(cfg->hp_fn){
        ret = wtk_source_loader_load(&sl,cfg,(wtk_source_load_handler_t)qtk_soundfield_syntheis_load_vec,cfg->hp_fn);
        cfg->hp = cfg->tmp;
    }

    if(cfg->weight_fn){
        cfg->weight_buf = wtk_strbuf_new(1024,1);
        ret = wtk_source_loader_load(&sl,cfg->weight_buf,(wtk_source_load_handler_t)qtk_soundfield_syntheis_load_weight,cfg->weight_fn);
    }
	return ret;
}
