#include "qtk_sond_cluster.h"
#define SV_WAV_LEN 64000

void qtk_sond_cluster_on_kxparm(qtk_sond_cluster_t* wrapper,wtk_kfeat_t *feat);
void qtk_sond_cluster_on_kxparm_end(qtk_sond_cluster_t* wrapper);

#ifdef ONNX_DEC
static void qtk_sond_cluster_onnx_item_prepare(qtk_onnx_item_t* item, int len){
		item->in_dim = len;

		switch (item->type)
		{
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT:
			
			item->bytes = sizeof(float);
            if(len == 0){
			    item->val = NULL;
                return;
		    }else{
                item->val = (float*)wtk_calloc(len,sizeof(float));
            }
			break;
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32:
			item->bytes = sizeof(int);
            if(len == 0){
			    item->val = NULL;
                return;
		    }else{
                item->val = (int*)wtk_calloc(len,sizeof(int));
            }
			
			break;
		case ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64:
			item->bytes = sizeof(int64_t);
            if(len == 0){
			    item->val = NULL;
                return;
		    }else{
                item->val = (int64_t*)wtk_calloc(len,sizeof(int64_t));
            }            
			break;
		default:
			break;
		}
}

static void qtk_sond_cluster_onnx_prepare(qtk_sond_cluster_t *sond_cluster){
	OrtStatus *status;
    qtk_onnxruntime_t *onnx = sond_cluster->onnx;
	OrtTypeInfo** type_info=(OrtTypeInfo**)wtk_malloc(sizeof(OrtTypeInfo*)*onnx->num_in);
	OrtTensorTypeAndShapeInfo* tensor_info2=NULL;
	int i,j;
	int len,len2;

	sond_cluster->fin_items = (qtk_onnx_item_t*)wtk_calloc(onnx->num_in,sizeof(qtk_onnx_item_t));
    if(!sond_cluster->cfg->use_leak_memory){
        sond_cluster->in_items = (qtk_onnx_item_t*)wtk_calloc(onnx->num_in,sizeof(qtk_onnx_item_t));
    }

	for(i=0; i<onnx->num_in; ++i)
	{
		qtk_onnx_item_t *item = sond_cluster->fin_items + i;
        qtk_onnx_item_t *item2 = sond_cluster->in_items + i;
		len = len2 = 1;
		status = onnx->api->SessionGetInputTypeInfo(onnx->session,i,type_info);
		status = onnx->api->CastTypeInfoToTensorInfo(*type_info,cast(const OrtTensorTypeAndShapeInfo **, &tensor_info2));
		status = onnx->api->SessionGetInputName(onnx->session,i,onnx->allocator,&(item->name));
        item2->name = NULL;
		status = onnx->api->GetDimensionsCount(tensor_info2, cast(size_t *, &(item->shape_len)));
        item2->shape_len = item->shape_len;
		item->shape = (int64_t*)wtk_calloc(item->shape_len,sizeof(int64_t));
    	item2->shape = (int64_t*)wtk_calloc(item->shape_len,sizeof(int64_t));
		status = onnx->api->GetDimensions(tensor_info2,item->shape,item->shape_len);
		status = onnx->api->GetDimensions(tensor_info2,item2->shape,item->shape_len);
		for(j = 0; j < item->shape_len; j++){
			if(item->shape[j] == -1){
				item->shape[j] = 1;
                item2->shape[j] = 1;

				if(i>=2 && i<=35 && i!=6 && i!=10){
					item->shape[3] = 1;
                    item2->shape[3] = 2;
				}

				if(i == 36){
					item->shape[3] = 8;
                    item2->shape[3] = 15;
				}
				if(i > 36 && i < 40){
					item->shape[2] = 15;
                    item2->shape[2] = 30;
				}
				if(i==40||i==41){
					item->shape[1] = 0;
                    item2->shape[1] = 15;
				}
			}
			len *= item->shape[j];
            len2 *= item2->shape[j];
		}
        status = onnx->api->GetTensorElementType(tensor_info2,&(item->type));
        status = onnx->api->GetTensorElementType(tensor_info2,&(item2->type));
        onnx->api->ReleaseTypeInfo(*type_info);
        if(i == 1){
            continue;
        }
        if(i == 0){
            item->val = wtk_calloc(80*sond_cluster->first_chunk_len,sizeof(float));
            item->bytes = sizeof(float);
            continue;
        }
        qtk_sond_cluster_onnx_item_prepare(item,len);
        qtk_sond_cluster_onnx_item_prepare(item2,len2);        
	}
    (void)status;
	wtk_free(type_info);
}
#endif

void qtk_sond_cluster_clu_notify(void *ths, int num, int len, float *embed) {
#ifdef ONNX_DEC
    qtk_sond_cluster_t *sond_cluster = (qtk_sond_cluster_t*)ths;
    qtk_onnx_item_t *item;
    item = sond_cluster->fin_items + 1;
    sond_cluster->spk_cnt = num;
    item->shape[1] = (sond_cluster->spk_cnt/4 + 1)*4;
    sond_cluster->emb_cnts = item->shape[1];
    item->in_dim = item->shape[0]*item->shape[1]*item->shape[2];
    if(item->val){
        wtk_free(item->val);
    }
    item->val = (float*)wtk_calloc(item->in_dim,sizeof(float));
    item->bytes = sizeof(float);
    memcpy(item->val,embed,len*num*sizeof(float));
#endif    
}

char clu_name[] = {
    'A','B','C','D','E','F','G','H','I','J','K','L'
};

qtk_sond_cluster_t* qtk_sond_cluster_new(qtk_sond_cluster_cfg_t *cfg){
	qtk_sond_cluster_t *sond_cluster = (qtk_sond_cluster_t*)wtk_malloc(sizeof(qtk_sond_cluster_t));

    sond_cluster->cfg = cfg;
    sond_cluster->svprint = NULL;
    sond_cluster->vad = NULL;
    sond_cluster->notify = NULL;
    sond_cluster->notify_ths = NULL;
    sond_cluster->vad_state = 0;
    sond_cluster->first_chunk_len = 550;
    sond_cluster->chunk_len = 200;
	if(cfg->use_svprint){
	    sond_cluster->svprint = wtk_svprint_new(&cfg->svprint);
	}

	if(cfg->use_vad){
		sond_cluster->vad = wtk_kvad_new(&cfg->vad);
	}

    sond_cluster->clu = NULL;
    if(cfg->use_clu){
        sond_cluster->clu = wtk_clu_new(&cfg->clu);
        wtk_clu_set_notify(sond_cluster->clu, (wtk_clu_notify_f)qtk_sond_cluster_clu_notify, sond_cluster);
        int i;
        for(i = 0; i < 12; i++){
            sond_cluster->name[i] = wtk_string_dup_data(&(clu_name[i]),1);
        }
    }
    
    sond_cluster->parm = wtk_kxparm_new(&cfg->kxparm);
    wtk_kxparm_set_notify(sond_cluster->parm,sond_cluster,(wtk_kxparm_notify_f)qtk_sond_cluster_on_kxparm);
    wtk_kxparm_set_notify_end(sond_cluster->parm,sond_cluster,(wtk_kxparm_notify_end_f)qtk_sond_cluster_on_kxparm_end);
    sond_cluster->frame_dur = sond_cluster->parm->cfg->parm.frame_step_ms;
#ifdef ONNX_DEC
    sond_cluster->onnx = qtk_onnxruntime_new(&(cfg->onnx));
    qtk_sond_cluster_onnx_prepare(sond_cluster);
#endif

    sond_cluster->enroll = 0;
    sond_cluster->eval = 1;
    sond_cluster->frame = 0;
    sond_cluster->frames = 0;
    sond_cluster->nframes = 0;
    sond_cluster->user = wtk_strbuf_new(1024,1);
    sond_cluster->feat = wtk_strbuf_new(1024,1);
    sond_cluster->result_buf = wtk_strbuf_new(1024,1);
    sond_cluster->spk_cnt = 0;
    sond_cluster->spk_feat = NULL;
    sond_cluster->filter_feat2 = NULL;
    sond_cluster->dframes = 0;
    sond_cluster->end = 0;

    sond_cluster->wav_bytes = 0;
    sond_cluster->vad_plus = 0.0;
    if(cfg->clu_len > 0){
        sond_cluster->clu_len = cfg->clu_len * 32000 * 60;
    }else{
        sond_cluster->clu_len = -1;
    }

    sond_cluster->vad_buf = wtk_strbuf_new(100,1);
    sond_cluster->border_buf = wtk_strbuf_new(100,1);
    sond_cluster->timestamp = 0.0;
    sond_cluster->has_tsvad = 0;
    sond_cluster->has_clu = 0;

    sond_cluster->wav_cache = wtk_strbuf_new(100,1);

    if(sond_cluster->cfg->use_window_pad){
    	sond_cluster->wav_buf = wtk_strbuf_new(1024,1);
    }
	return sond_cluster;
}

void qtk_sond_cluster_delete(qtk_sond_cluster_t *sond_cluster){
    int i;
	if(sond_cluster->parm){
		wtk_kxparm_delete(sond_cluster->parm);
	}
    wtk_strbuf_delete(sond_cluster->user);
    if(sond_cluster->cfg->use_window_pad){
    	wtk_strbuf_delete(sond_cluster->wav_buf);
    }

    if(sond_cluster->svprint){
    	wtk_svprint_delete(sond_cluster->svprint);
    }

    if(sond_cluster->vad){
    	wtk_kvad_delete(sond_cluster->vad);
    }

    if(sond_cluster->clu){
        wtk_clu_delete(sond_cluster->clu);
        for(i = 0; i < 12; i++){
            wtk_string_delete(sond_cluster->name[i]);
        }
    }
#ifdef ONNX_DEC
	qtk_onnx_item_t *item;

	for(i=0; i<sond_cluster->onnx->num_in; ++i)
	{
		item = sond_cluster->in_items + i;
		wtk_free(item->val);
		wtk_free(item->shape);
        if(item->name){
            sond_cluster->onnx->allocator->Free(sond_cluster->onnx->allocator, item->name);
        }
        item = sond_cluster->fin_items + i;
        wtk_free(item->val);
		wtk_free(item->shape);
        if(item->name){
            sond_cluster->onnx->allocator->Free(sond_cluster->onnx->allocator, item->name);
        }
	}
	wtk_free(sond_cluster->fin_items);
    wtk_free(sond_cluster->in_items);
    qtk_onnxruntime_delete(sond_cluster->onnx);
#endif

    wtk_strbuf_delete(sond_cluster->vad_buf);
    wtk_strbuf_delete(sond_cluster->border_buf);
    wtk_strbuf_delete(sond_cluster->wav_cache);

    if(sond_cluster->filter_feat2){
        for(i = 0;i <sond_cluster->spk_cnt;i++){
            wtk_strbuf_delete(sond_cluster->filter_feat2[i]);
        }
        wtk_free(sond_cluster->filter_feat2);
    }
    wtk_strbuf_delete(sond_cluster->feat);
    wtk_strbuf_delete(sond_cluster->result_buf);
    if(sond_cluster->spk_feat){
        wtk_strbufs_delete(sond_cluster->spk_feat,sond_cluster->spk_cnt);
    }
    wtk_free(sond_cluster);
}

int qtk_sond_cluster_start(qtk_sond_cluster_t *sond_cluster){
	if(sond_cluster->enroll || sond_cluster->eval){
		wtk_svprint_start(sond_cluster->svprint);
	}

	if(!sond_cluster->enroll){
		if(sond_cluster->vad){
			wtk_kvad_start(sond_cluster->vad);
		}
		wtk_kxparm_start(sond_cluster->parm);
        sond_cluster->spk_feat = wtk_strbufs_new(sond_cluster->spk_cnt);
        sond_cluster->filter_feat2 = wtk_strbufs_new(sond_cluster->spk_cnt);
        int i,j;
        short val = 0;
        for(i = 0; i < sond_cluster->cfg->smooth_size; i++){
            for(j = 0; j < sond_cluster->spk_cnt; j++){
                wtk_strbuf_push(sond_cluster->spk_feat[j],(char*)&val,sizeof(short));
            }
        }

        wtk_strbuf_reset(sond_cluster->result_buf);
	}
	return 0;
}

int qtk_sond_cluster_reset(qtk_sond_cluster_t *sond_cluster){
    int i;
	if(sond_cluster->parm)
	{
		wtk_kxparm_reset(sond_cluster->parm);
	}
    sond_cluster->nframes = 0;
	if(sond_cluster->cfg->use_svprint)
	{
		wtk_svprint_reset(sond_cluster->svprint);
	}
    sond_cluster->vad_state = 0;
	if(sond_cluster->vad)
	{
		wtk_kvad_reset(sond_cluster->vad);
	}

    if(sond_cluster->clu){
        wtk_clu_reset(sond_cluster->clu);
    }

	if(sond_cluster->cfg->use_window_pad)
	{
		wtk_strbuf_reset(sond_cluster->wav_buf);
	}
#ifdef ONNX_DEC
    qtk_onnxruntime_reset(sond_cluster->onnx);
    //qtk_onnxruntime_item_reset(sond_cluster->onnx);
#endif
    sond_cluster->frame = 0;
    sond_cluster->dframes = 0;
    sond_cluster->end = 0;
    sond_cluster->timestamp = 0.0;
    sond_cluster->wav_bytes = 0;
    sond_cluster->vad_plus = 0;
    sond_cluster->has_tsvad = 0;
    sond_cluster->has_clu = 0;
    wtk_strbuf_reset(sond_cluster->feat);
    wtk_strbuf_reset(sond_cluster->vad_buf);
    wtk_strbuf_reset(sond_cluster->border_buf);
    wtk_strbuf_reset(sond_cluster->wav_cache);
    if(sond_cluster->spk_feat){
        wtk_strbufs_reset(sond_cluster->spk_feat,sond_cluster->spk_cnt);
    }

    if(sond_cluster->filter_feat2){
        for(i = 0;i <sond_cluster->spk_cnt;i++){
            if(sond_cluster->filter_feat2[i]){
                wtk_strbuf_delete(sond_cluster->filter_feat2[i]);
            }
        }
        wtk_free(sond_cluster->filter_feat2);
        sond_cluster->filter_feat2 = NULL;
    }
	return 0;
}

int qtk_sond_cluster_reset2(qtk_sond_cluster_t *sond_cluster){
    sond_cluster->vad_state = 0;
    sond_cluster->nframes = 0;
    if(sond_cluster->parm)
    {
    	wtk_kxparm_reset(sond_cluster->parm);
    }

	if(sond_cluster->cfg->use_svprint)
	{
		wtk_svprint_reset(sond_cluster->svprint);
	}
#ifdef ONNX_DEC
    qtk_onnxruntime_reset(sond_cluster->onnx);
#endif
    wtk_strbuf_reset(sond_cluster->feat);
    if(sond_cluster->spk_feat){
        wtk_strbufs_reset(sond_cluster->spk_feat,sond_cluster->spk_cnt);
    }
    sond_cluster->frame = 0;
	return 0;
}

void qtk_sond_cluster_clean(qtk_sond_cluster_t *sond_cluster){
	if(sond_cluster->svprint)
	{
		wtk_svprint_clean(sond_cluster->svprint);
        //wtk_svprint_reload(sond_cluster->svprint);
	}
}

void qtk_sond_cluster_reload(qtk_sond_cluster_t *sond_cluster){
	if(sond_cluster->svprint)
	{
		wtk_svprint_reload(sond_cluster->svprint);
        qtk_sond_cluster_prepare(sond_cluster);
	}
}

int qtk_sond_cluster_set_vad_time(qtk_sond_cluster_t *sond_cluster, float start, float end){
    float tmp;
    if(start >= 0){
        wtk_strbuf_push_float(sond_cluster->vad_buf,&start,1);
        //wtk_debug("%f\n",start);
        wtk_strbuf_push_float(sond_cluster->border_buf,&sond_cluster->timestamp,1);
        wtk_debug("%f\n",sond_cluster->timestamp);
    }else{
        return -1;
    }

    if(end > 0){
        tmp = sond_cluster->timestamp + end - start;
        wtk_strbuf_push_float(sond_cluster->vad_buf,&end,1);
        //wtk_debug("%f\n",end);
        wtk_strbuf_push_float(sond_cluster->border_buf,&tmp,1);
        wtk_debug("%f\n",tmp);
        sond_cluster->timestamp += end - start;
    }else{
        return -1;
    }
    return 0;
}

char* qtk_sond_cluster_get_fn(qtk_sond_cluster_t *sond_cluster){
	if(sond_cluster->svprint)
	{
		return sond_cluster->svprint->cfg->enroll_fn;
	}
	return NULL;
}

short *qtk_sond_cluster_pad_data(short *data, int len) {
    short *to_data = NULL;
    short *s;
    int s3l = 16000 * 2; //最低2s
    int pad = s3l / len;
    int i, j = s3l % len;

    to_data = (short *)wtk_calloc(s3l, sizeof(short));
    s = to_data;
    for (i = 0; i < pad; ++i) {
        memcpy(s, data, len * sizeof(short));
        s = s + len;
    }
    memcpy(s, data, j * sizeof(short));

    return to_data;
}

float qtk_sond_cluster_wav_scale(char *data, int len){
	short *wav = (short*)data;
	int l = len>>1;
	int max = wtk_short_abs_max(wav,l);

	return 0.99/max;
}

int qtk_sond_cluster_enroll_normal(qtk_sond_cluster_t *sond_cluster,char *data,int bytes,int is_end){
    wtk_queue_t *q = &(sond_cluster->vad->output_q);
    wtk_vframe_t *vf = NULL;
    wtk_queue_node_t *qn;
    wtk_vecf_t *vec, *mean;
    int cnt, i;

    if (sond_cluster->vad) {
        wtk_kvad_feed(sond_cluster->vad, (short *)data, bytes >> 1, is_end);
        while (1) {
            qn = wtk_queue_pop(q);
            if (!qn) {
                break;
            }
            vf = data_offset2(qn, wtk_vframe_t, q_n);

            switch (vf->state) {
            case wtk_vframe_sil:
                break;
            case wtk_vframe_speech:
                if (!sond_cluster->cfg->use_window_pad) {
                    wtk_svprint_feed(sond_cluster->svprint, vf->wav_data,
                                        vf->frame_step, 0);
                } else {
                    wtk_strbuf_push(sond_cluster->wav_buf, (char *)vf->wav_data,
                                    vf->frame_step * sizeof(short));
                }
                break;
            case wtk_vframe_speech_end:
                if (sond_cluster->cfg->use_window_pad) {
                    wtk_strbuf_push(sond_cluster->wav_buf, (char *)vf->wav_data,
                                    vf->frame_step * sizeof(short));
                }
                break;
            default:
                break;
            }
            wtk_kvad_push_frame(sond_cluster->vad, vf);
        }
    } else {
        wtk_svprint_feed(sond_cluster->svprint, (short *)data, (bytes) >> 1, 0);
    }
    if (is_end == 1) {
        if (sond_cluster->cfg->use_window_pad) {
            if (sond_cluster->wav_buf->pos < SV_WAV_LEN) {
                return -1;
            } else {
                if (sond_cluster->wav_buf->pos > SV_WAV_LEN * 2) {
                    cnt = sond_cluster->wav_buf->pos / SV_WAV_LEN;
                    char *s, *e;
                    int nx;

                    s = sond_cluster->wav_buf->data;
                    e = sond_cluster->wav_buf->data + sond_cluster->wav_buf->pos;

                    while (s < e) {
                    nx = min(SV_WAV_LEN, e - SV_WAV_LEN - s);
                    if (nx < SV_WAV_LEN) {
                        nx = e - s;
                        wtk_svprint_feed(sond_cluster->svprint, (short *)s,
                                            nx >> 1, 0);
                        wtk_svprint_feed(sond_cluster->svprint, NULL, 0, 1);
                        s += nx;
                    } else {
                        wtk_svprint_feed(sond_cluster->svprint, (short *)s,
                                            SV_WAV_LEN >> 1, 0);
                        wtk_svprint_feed(sond_cluster->svprint, NULL, 0, 1);
                        s += SV_WAV_LEN;
                    }
                    sond_cluster->svprint->x->spk_cnt++;
                    vec = wtk_svprint_compute_feat2(sond_cluster->svprint, &cnt);
                    mean = sond_cluster->svprint->x->spk_mean;

                    for (i = 0; i < mean->len; i++) {
                        mean->p[i] += vec->p[i];
                    }
                    qtk_sond_cluster_reset2(sond_cluster);
                    }
                } else {
                    wtk_svprint_feed(sond_cluster->svprint,
                                        (short *)sond_cluster->wav_buf->data,
                                        sond_cluster->wav_buf->pos >> 1, 0);
                    wtk_svprint_feed(sond_cluster->svprint, NULL, 0, 1);
                    sond_cluster->svprint->x->spk_cnt++;
                    vec = wtk_svprint_compute_feat2(sond_cluster->svprint, &cnt);
                    mean = sond_cluster->svprint->x->spk_mean;

                    for (i = 0; i < mean->len; i++) {
                        mean->p[i] += vec->p[i];
                    }
                }
            }
        } else {
            wtk_svprint_feed(sond_cluster->svprint, NULL, 0, 1);
            sond_cluster->svprint->x->spk_cnt++;
            vec = wtk_svprint_compute_feat2(sond_cluster->svprint, &cnt);
            mean = sond_cluster->svprint->x->spk_mean;
            for (i = 0; i < mean->len; i++) {
                mean->p[i] += vec->p[i];
            }
        }
    }
    return 0;
}

int qtk_sond_cluster_feed(qtk_sond_cluster_t *sond_cluster,char *data,int bytes,int is_end){
    qtk_sond_cluster_cfg_t *cfg = sond_cluster->cfg;

    if(cfg->use_clu){
        if(sond_cluster->clu_len != -1){
            if(sond_cluster->wav_bytes <= sond_cluster->clu_len){
                wtk_clu_feed(sond_cluster->clu, data, bytes, is_end);
                sond_cluster->wav_bytes += bytes;
                if(cfg->use_tsvad){
                    wtk_strbuf_push(sond_cluster->wav_cache,data,bytes);
                    if(is_end){
                        sond_cluster->end = 1;
                        sond_cluster->has_tsvad = 1;
                        wtk_kxparm_feed(sond_cluster->parm, (short *)(sond_cluster->wav_cache->data), (sond_cluster->wav_cache->pos) >> 1,1);
                    }else if(sond_cluster->wav_bytes > sond_cluster->clu_len){
                        wtk_clu_feed(sond_cluster->clu, NULL, 0, 1);
                        wtk_kxparm_feed(sond_cluster->parm, (short *)(sond_cluster->wav_cache->data), (sond_cluster->wav_cache->pos) >> 1,0);
                    }              
                }else{
                    if(sond_cluster->wav_bytes > sond_cluster->clu_len){
                        wtk_clu_feed(sond_cluster->clu, NULL, 0, 1);
                    }  
                }
            }else{
                if(is_end){
                    sond_cluster->end = 1;
                }
                sond_cluster->has_tsvad = 1;
                if(!cfg->use_tsvad){
                    sond_cluster->has_clu = 1;
                }
                wtk_kxparm_feed(sond_cluster->parm, (short *)data, bytes >> 1,is_end);
            }
        }else{
            wtk_clu_feed(sond_cluster->clu, data, bytes, is_end);
            if(cfg->use_tsvad){
                wtk_strbuf_push(sond_cluster->wav_cache,data,bytes);
                if(is_end){
                    sond_cluster->end = 1;
                    sond_cluster->has_tsvad = 1;
                    wtk_kxparm_feed(sond_cluster->parm, (short *)(sond_cluster->wav_cache->data), (sond_cluster->wav_cache->pos) >> 1,1);
                }
            }
        }
    }else{
        if(sond_cluster->enroll) {
            if (sond_cluster->svprint->enroll_available == 0) {
                return -1;
            }
            return qtk_sond_cluster_enroll_normal(sond_cluster,data,bytes,is_end);
        } else {
            if(is_end){
                sond_cluster->end = 1;
            }
            sond_cluster->has_tsvad = 1;
            wtk_kxparm_feed(sond_cluster->parm, (short *)data, bytes >> 1,is_end);
        }
    }

    return 0;
}

int qtk_sond_cluster_enroll(qtk_sond_cluster_t *sond_cluster,char *name,int len){
	if(len > 0)
	{
		sond_cluster->enroll = 1;
		wtk_strbuf_reset(sond_cluster->user);
		wtk_strbuf_push(sond_cluster->user,name,len);
	}else
	{
		sond_cluster->eval = 1;
	}

	return 0;
}

int qtk_sond_cluster_enroll_end(qtk_sond_cluster_t *sond_cluster) {
    int ret = 0;
    wtk_string_t v;

    v.data = sond_cluster->user->data;
    v.len = sond_cluster->user->pos;
    ret = wtk_svprint_enroll2file(sond_cluster->svprint, &v);

    sond_cluster->enroll = 0;
    sond_cluster->eval = 1;
    wtk_svprint_reload(sond_cluster->svprint);
    sond_cluster->spk_cnt = sond_cluster->svprint->enroll_q.length;
    return ret;
}

int qtk_sond_cluster_set_enroll_fn(qtk_sond_cluster_t *sond_cluster,char *fn, int len)
{
	if(sond_cluster->svprint)
	{
		sond_cluster->svprint->cfg->enroll_fn=fn;
		wtk_svprint_reload(sond_cluster->svprint);
	}
	return 0;
}

static float qtk_float_max(float *a,int n, int *index){
    int i;
    float f;

    *index = 0;
    f=a[0];

    for(i=1;i<n;++i)
    {
        if(a[i]>f)
        {
            f=a[i];
            *index = i;
        }
    }
    return f;
}

void sc_debug(qtk_sond_cluster_t *sond_cluster){
    int i;
    short *a,*b,*c,*d;
    a = (short*)sond_cluster->spk_feat[0]->data;
    b = (short*)sond_cluster->spk_feat[1]->data;
    c = (short*)sond_cluster->spk_feat[2]->data;
    d = (short*)sond_cluster->spk_feat[3]->data;
    for(i = 0; i < sond_cluster->spk_feat[0]->pos/2; i++){
        printf("[%d %d %d %d]\n",*a,*b,*c,*d);
        a++;
        b++;
        c++;
        d++;
    }
}

void  qtk_sond_cluster_logits_pre_compute(qtk_sond_cluster_t *sond_cluster,int shape, float *src,int cnt){
    int i,j,index,x;
    float *p = src;
    short val = 0,val2 = 1;
    //printf("--------\n");
    //printf("(1, %d, 4)\n[[",shape);
    for(i = 0; i < shape; i++){
        //printf("[%.8f %.8f %.8f %.8f]\n",*p,*(p+1),*(p+2),*(p+3));
        qtk_float_max(p,cnt,&index);
        p += sond_cluster->emb_cnts;
        for(j = 0; j < sond_cluster->cfg->encoder_downsample_ratio; j++){
            for(x = 0; x < cnt; x++){
                if(x == index){
                    wtk_strbuf_push(sond_cluster->spk_feat[x],(char*)&val2,sizeof(short));
                }else{
                    wtk_strbuf_push(sond_cluster->spk_feat[x],(char*)&val,sizeof(short));
                }
            }
        }
    }
    //sc_debug(sond_cluster);
}

void qtk_sond_cluster_logits_process(qtk_sond_cluster_t *sond_cluster){
    int i,j,len;
    short *logits,*filter_feat;
    short val = 0;
    if(sond_cluster->end){
        for(i = 0; i < sond_cluster->cfg->smooth_size;i++){
            for(j = 0; j < sond_cluster->spk_cnt; j++){
                wtk_strbuf_push(sond_cluster->spk_feat[j],(char*)&val,sizeof(short));
            }
        }
    }

    len = sond_cluster->spk_feat[0]->pos/sizeof(short) - sond_cluster->dframes;
    sond_cluster->frames = sond_cluster->spk_feat[0]->pos/sizeof(short) - sond_cluster->cfg->smooth_size*2;
    for(i = 0; i < sond_cluster->spk_cnt; i++){
        logits = (short*)(sond_cluster->spk_feat[i]->data);
        wtk_strbuf_push(sond_cluster->filter_feat2[i],NULL,len*sizeof(short));
        filter_feat = (short*)sond_cluster->filter_feat2[i]->data;
        wtk_median_filter_process(logits + sond_cluster->dframes,filter_feat + sond_cluster->dframes,len,sond_cluster->cfg->smooth_size);
    }
    sond_cluster->dframes += (len - sond_cluster->cfg->smooth_size);
    //sc_debug2(sond_cluster);
}

void qtk_sond_cluster_prepare2(qtk_sond_cluster_t* sond_cluster){//for wav test,multiple speakers
    wtk_queue_node_t *qn;
    wtk_svprint_cfg_feat_node_t *node;
#ifdef ONNX_DEC
    qtk_onnx_item_t *item;
    float *tmp;
    item = sond_cluster->fin_items + 1;

    // item->shape[1] = 4;
    // item->in_dim = item->shape[0]*item->shape[1]*item->shape[2];
    // item->val = (float*)wtk_calloc(item->in_dim,sizeof(float));
    // sond_cluster->emb_cnts = item->shape[1];
    // sond_cluster->spk_cnt = 2;
    // tmp = (float*)item->val;
    // wtk_string_set_s(&name1,"221012_003_白");
    // wtk_string_set_s(&name2,"221012_003_郭");
    // sond_cluster->name[0] = &name1;
    // sond_cluster->name[1] = &name2;
    // memcpy(tmp,sc_tmp,sizeof(float)*512);
#endif
    int i;
    for(qn=sond_cluster->svprint->enroll_q.pop,i=0;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        if(wtk_str_equal(sond_cluster->wav->data,sond_cluster->wav->len-4,node->name->data,sond_cluster->wav->len-4))
        {
            sond_cluster->name[i] = node->name;
            sond_cluster->spk_cnt++;
            i++;
        }
    }

#ifdef ONNX_DEC
    item->shape[1] = (sond_cluster->spk_cnt/4 + 1)*4;
    sond_cluster->emb_cnts = item->shape[1];
    item->in_dim = item->shape[0]*item->shape[1]*item->shape[2];
    item->val = (float*)wtk_calloc(item->in_dim,sizeof(float));
    item->bytes = sizeof(float);
    tmp = (float*)item->val;//TODO
    
#endif
    for(qn=sond_cluster->svprint->enroll_q.pop,i=0;qn;qn=qn->next)
    {
        node = data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        wtk_svprint_L2_normalization(node->v, node->num);
        if(wtk_str_equal(sond_cluster->wav->data,sond_cluster->wav->len-4,node->name->data,sond_cluster->wav->len-4))
        {
#ifdef ONNX_DEC
            memcpy(tmp + node->v->len*i,node->v->p,node->v->len*sizeof(float));
#endif
            i++;
        }
    }
}

int qtk_sond_cluster_prepare(qtk_sond_cluster_t* sond_cluster){
    wtk_queue_node_t *qn;
    wtk_svprint_cfg_feat_node_t *node;
#ifdef ONNX_DEC
    qtk_onnx_item_t *item;
    float *tmp;
    item = sond_cluster->fin_items + 1;

#endif
    int i;
    for(qn=sond_cluster->svprint->enroll_q.pop,i=0;qn;qn=qn->next)
    {
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        sond_cluster->name[i] = node->name;
        i++;
    }
    if(i == 0){
        return -1;
    }
#ifdef ONNX_DEC
    if(sond_cluster->spk_cnt != i || !item->val){
        sond_cluster->spk_cnt = i;
        item->shape[1] = (sond_cluster->spk_cnt/4 + 1)*4;
        sond_cluster->emb_cnts = item->shape[1];
        item->in_dim = item->shape[0]*item->shape[1]*item->shape[2];
        if(item->val){
            wtk_free(item->val);
        }
        item->val = (float*)wtk_calloc(item->in_dim,sizeof(float));
        item->bytes = sizeof(float);
    }
    tmp = (float*)item->val;
#endif
    for(qn=sond_cluster->svprint->enroll_q.pop,i=0;qn;qn=qn->next,i++)
    {
        node=data_offset2(qn,wtk_svprint_cfg_feat_node_t,qn);
        wtk_svprint_L2_normalization(node->v, node->num);
#ifdef ONNX_DEC
        memcpy(tmp + item->shape[2]*i,node->v->p,item->shape[2]*sizeof(float));
#endif
    }
    return 0;
}

int qtk_sond_cluster_set_spk_nums(qtk_sond_cluster_t* sond_cluster, int cnt){
    if(sond_cluster->cfg->use_clu){
        sond_cluster->spk_cnt = cnt;
        wtk_clu_set_oracle_num(sond_cluster->clu, cnt);
        return 0;
    }
    return -1;
}

float* qtk_sond_cluster_run(qtk_sond_cluster_t* sond_cluster, int st_frame, int num_frames, int *shape){
	float *e_out = NULL;
#ifdef ONNX_DEC
	const OrtApi *api = sond_cluster->onnx->api;
	OrtMemoryInfo *meminfo = sond_cluster->onnx->meminfo;
	qtk_onnxruntime_t *onnx = sond_cluster->onnx;
	qtk_onnx_item_t *item;
	OrtStatus *status;
	int i;
	int64_t size;
    void *encoder_out;
    int64_t *shap;
    item = sond_cluster->fin_items;
	memcpy(item->val,
			sond_cluster->feat->data + st_frame * 80 * sizeof(float),
			num_frames * 80 * sizeof(float));
    item->in_dim = 80 * num_frames;
    item->bytes = sizeof(float);
    item->shape[1] = num_frames;
    float xx = 1.0;
	for(i = 0; i < onnx->num_in;i++){
        if(i < 2 || st_frame == 0){
            item = sond_cluster->fin_items + i;
        }else{
            item = sond_cluster->in_items + i;
        }
        if(i == onnx->num_in -2){
            memcpy(item->val,&xx,sizeof(float));
        }
        //wtk_debug("%d %d %d %d\n",item->bytes,item->in_dim,item->shape_len,item->type);
		status = api->CreateTensorWithDataAsOrtValue(meminfo,item->val,item->bytes*item->in_dim,item->shape,item->shape_len,item->type,onnx->in+i);
	}
	status = api->Run(
		onnx->session, NULL, cast(const char *const *, onnx->name_in),
		cast(const OrtValue *const *, onnx->in), onnx->num_in,
		cast(const char *const *, onnx->name_out), onnx->num_out,
		onnx->out);
	(void)status;

    shap = qtk_onnxruntime_get_outshape(sond_cluster->onnx,0,&size);
    *shape  = shap[1];
	e_out = qtk_onnxruntime_getout(sond_cluster->onnx,0);
    for(i = 1; i < onnx->num_out;i++){
        if(i == 39){
            continue;
        }
        if(i > 39){
            item = sond_cluster->in_items + i;
        }else{
            item = sond_cluster->in_items + i + 1;    
        }

        encoder_out = qtk_onnxruntime_getout(onnx,i);
        memcpy(item->val,encoder_out,item->bytes*item->in_dim);
    }
    wtk_free(shap);
#endif
    return e_out;
}

void qtk_sond_cluster_res2rttm(qtk_sond_cluster_t *qtk_sond_cluster){
    //TODO
}

static void qtk_sond_cluster_border_map(qtk_sond_cluster_t* sond_cluster,float start,float end,int cnt){
    float *vad = (float*)sond_cluster->vad_buf->data;
    float *border = (float*)sond_cluster->border_buf->data;
    int len = sond_cluster->vad_buf->pos/sizeof(float),i,flag = 0;
    float st,ed,left,right;
    for(i = 0; i < len - 1; i+=2){
        left = border[i];
        right = border[i+1];
        if(start >= left && start < right){
            st = start - left + vad[i];
            flag = 1;
            if(end > left && end <= right){
                ed = end - left + vad[i];
                wtk_strbuf_push_f(sond_cluster->result_buf,"%.2f %.2f %.*s\n",st,ed,
                        sond_cluster->name[cnt]->len,sond_cluster->name[cnt]->data);
                return;
            }
            wtk_strbuf_push_f(sond_cluster->result_buf,"%.2f %.2f %.*s\n",st,vad[i+1],
                        sond_cluster->name[cnt]->len,sond_cluster->name[cnt]->data);
            continue;
        }

        if(flag == 1){
            if(end > left && end <= right){
                st = vad[i];
                ed = end - left + vad[i];
                wtk_strbuf_push_f(sond_cluster->result_buf,"%.2f %.2f %.*s\n",st,ed,
                        sond_cluster->name[cnt]->len,sond_cluster->name[cnt]->data);
                return;
            }else{
                wtk_strbuf_push_f(sond_cluster->result_buf,"%.2f %.2f %.*s\n",vad[i],vad[i+1],
                        sond_cluster->name[cnt]->len,sond_cluster->name[cnt]->data);
            }
        }
    }
}

void qtk_sond_cluster_get_result_tsvad(qtk_sond_cluster_t* sond_cluster, wtk_string_t *v){
    int i,j,flag=0;
    int st=0,end=0;
    short *f;
    float dur = sond_cluster->frame_dur;
    float vad_plus = 0.0;

    if(sond_cluster->has_clu){
        vad_plus = sond_cluster->vad_plus;
    }
    qtk_sond_cluster_logits_process(sond_cluster);
    for(i = 0; i < sond_cluster->spk_cnt; i++){
        f = (short*)sond_cluster->filter_feat2[i]->data + sond_cluster->cfg->smooth_size/2 + 1;
        for(j = 0; j < sond_cluster->frames; j++,f++){
            if(!flag && *f == 1){
                st = j;
                flag = 1;
            }else if(flag && (*f == 0 || j == sond_cluster->frames - 1)){
                end = j;
                flag = 0;
                if((end - st) > sond_cluster->cfg->seg_length_threshold){
                    if(sond_cluster->vad_buf->pos > 0){
                        qtk_sond_cluster_border_map(sond_cluster,st*dur/1000 + vad_plus,end*dur/1000 + vad_plus,i);
                    }else{
                        wtk_strbuf_push_f(sond_cluster->result_buf,"%.2f %.2f %.*s\n",st*dur/1000 + vad_plus,end*dur/1000 + vad_plus,
                            sond_cluster->name[i]->len,sond_cluster->name[i]->data);
                        // wtk_strbuf_push_f(sond_cluster->result_buf,
                        // "SPEAKER %.*s 1 %.2f %.2f <NA> <NA> %.*s <NA> <NA>\n",
                        // sond_cluster->vv->len-4,sond_cluster->vv->data,st*dur/1000+vad_plus,
                        // (end-st)*dur/1000,sond_cluster->name[i]->len,sond_cluster->name[i]->data);
                    }
                }
            }
        }
    }
}

void qtk_sond_cluster_get_result_clu(qtk_sond_cluster_t* sond_cluster, wtk_string_t *v){
    int i,j,len;
    float *p, st, end = 0;
    wtk_clu_t *clu = sond_cluster->clu;
    for(i = 0; i < clu->vprint_idx_num; i++){
        p = (float*)clu->result[i]->data;
        len = clu->result[i]->pos/sizeof(float);
        for(j = 0; j < len; j+=2){
            st = *p++;
            end = *p++;
            if(sond_cluster->vad_buf->pos > 0){
                qtk_sond_cluster_border_map(sond_cluster,st,end,i);
            }else{
                wtk_strbuf_push_f(sond_cluster->result_buf,"%.2f %.2f %.*s\n",st,end,
                   sond_cluster->name[i]->len,sond_cluster->name[i]->data);
                // wtk_strbuf_push_f(sond_cluster->result_buf,
                //     "SPEAKER %.*s 1 %.2f %.2f <NA> <NA> %.*s <NA> <NA>\n",
                //     sond_cluster->vv->len-4,sond_cluster->vv->data,st,(end-st),
                //     sond_cluster->name[i]->len,sond_cluster->name[i]->data);
            }
        }
        if(!sond_cluster->cfg->use_tsvad && sond_cluster->vad_plus < end){
            sond_cluster->vad_plus = end;
        }
    }
}

void qtk_sond_cluster_get_result(qtk_sond_cluster_t* sond_cluster, wtk_string_t *v){
    wtk_strbuf_reset(sond_cluster->result_buf);
    if(sond_cluster->has_tsvad){
        if(!sond_cluster->has_clu){
            qtk_sond_cluster_get_result_tsvad(sond_cluster,v);
        }else{
            qtk_sond_cluster_get_result_clu(sond_cluster,v);
            qtk_sond_cluster_get_result_tsvad(sond_cluster,v);
        }
    }else{
        qtk_sond_cluster_get_result_clu(sond_cluster,v);
    }
    wtk_string_set(v,sond_cluster->result_buf->data,sond_cluster->result_buf->pos);
}


void qtk_sond_cluster_on_kxparm(qtk_sond_cluster_t* sond_cluster,wtk_kfeat_t *feat){
    int shape = 0;
    float *out;
    if(feat){
        wtk_strbuf_push_float(sond_cluster->feat,feat->v,feat->len);
        sond_cluster->nframes++;
        if(sond_cluster->nframes == sond_cluster->first_chunk_len){
            out = qtk_sond_cluster_run(sond_cluster,sond_cluster->frame,sond_cluster->first_chunk_len,&shape);
            sond_cluster->frame += sond_cluster->first_chunk_len;
            qtk_sond_cluster_logits_pre_compute(sond_cluster,shape,out,sond_cluster->spk_cnt);
        }else if((sond_cluster->nframes > sond_cluster->first_chunk_len)&&(sond_cluster->nframes - sond_cluster->first_chunk_len)%sond_cluster->chunk_len == 0){
            out = qtk_sond_cluster_run(sond_cluster,sond_cluster->frame,sond_cluster->chunk_len,&shape);
            sond_cluster->frame += sond_cluster->chunk_len;
            qtk_sond_cluster_logits_pre_compute(sond_cluster,shape,out,sond_cluster->spk_cnt);
        }
#ifdef ONNX_DEC
        qtk_onnxruntime_reset(sond_cluster->onnx);
#endif
    }
}

void qtk_sond_cluster_on_kxparm_end(qtk_sond_cluster_t* sond_cluster){
    if(sond_cluster->nframes < sond_cluster->first_chunk_len){
        return;
    }
    int shape = 0;
    float *out;
    int len = sond_cluster->nframes - sond_cluster->frame;
    {
        out = qtk_sond_cluster_run(sond_cluster,sond_cluster->frame,len,&shape);
        qtk_sond_cluster_logits_pre_compute(sond_cluster,shape,out,sond_cluster->spk_cnt);
    }
}
