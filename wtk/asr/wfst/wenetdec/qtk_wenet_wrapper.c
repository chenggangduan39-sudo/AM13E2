#include "qtk_wenet_wrapper.h"
void qtk_wenet_wrapper_send_back_feature(qtk_wenet_wrapper_t *warpper,wtk_feat_t *f);
int qtk_wenet_wrapper_feed3(qtk_wenet_wrapper_t* wrapper,short *data,int bytes,int is_end);
void qtk_wenet_on_kxparm(qtk_wenet_wrapper_t* wrapper,wtk_kfeat_t *feat);
void qtk_wenet_on_kxparm_end(qtk_wenet_wrapper_t* wrapper);

float prior[] =
{
#include "prior.txt"
};

qtk_wenet_wrapper_t* qtk_wenet_wrapper_new(qtk_wenet_wrapper_cfg_t* cfg)
{
	qtk_wenet_wrapper_t *wrapper;

	wrapper=(qtk_wenet_wrapper_t*)wtk_malloc(sizeof(*wrapper));
	wrapper->cfg=cfg;
	wrapper->env_parser=wtk_cfg_file_new();
	wrapper->kxparm=wtk_kxparm_new(&(cfg->parm));
	wtk_kxparm_set_notify(wrapper->kxparm,wrapper,(wtk_kxparm_notify_f)qtk_wenet_on_kxparm);
    wtk_kxparm_set_notify_end(wrapper->kxparm,wrapper,(wtk_kxparm_notify_end_f)qtk_wenet_on_kxparm_end);
#ifdef ONNX_DEC
	wrapper->onnx = qtk_onnxruntime_new(&(cfg->onnx));
#endif
	//wrapper->dec=qtk_kwfstdec_new(&(cfg->kwfstdec));
	wtk_queue_init(&(wrapper->parm_q));
	//wrapper->res_buf=wtk_strbuf_new(1024,1);
	wrapper->res_buf=NULL;
	wrapper->hint_buf=NULL;
	wrapper->json_buf=wtk_strbuf_new(1024,1);
	wrapper->json=wtk_json_new();
	wrapper->time_stop=0;
	wrapper->wav_bytes=0;
	wrapper->rec_wav_bytes=0;
	wrapper->index=0;
	wrapper->data_left=0;
	wrapper->chnlike=NULL;
    wtk_queue_init(&(wrapper->vad_q));
    wrapper->num_feats = 0;
    wrapper->shape[0] = 1;
    wrapper->shape2 = 1;
	if(cfg->use_xbnf)
	{
		wrapper->xbnf=wtk_xbnf_rec_new(&(cfg->xbnf));
	}else
	{
		wrapper->xbnf=NULL;
	}

    if(cfg->use_vad)
    {
        wrapper->vad=wtk_vad_new(&(cfg->vad),&(wrapper->vad_q));
        wrapper->last_vframe_state=wtk_vframe_sil;
		wrapper->vad_buf=wtk_strbuf_new(1024,1);
    }else
    {
        wrapper->vad=0;
		wrapper->vad_buf=NULL;
    }

	if(cfg->use_lex)
	{
		wrapper->lex=wtk_lex_new(&(cfg->lex));
		wtk_lex_compile(wrapper->lex,cfg->lex_fn);
	}else
	{
		wrapper->lex=0;
	}

	if (cfg->use_context) {
		wrapper->egram = wtk_egram_new(&(cfg->egram), NULL);
	} else {
		wrapper->egram = NULL;
	}

	wrapper->res_buf = wtk_strbuf_new(1024, 1);
	wrapper->hint_buf=wtk_strbuf_new(1024,1);
	wrapper->feat=wtk_strbuf_new(1024,1);
	if(wrapper->cfg->use_lite)
		wrapper->dec_lite=qtk_kwfstdec_lite_new(&(cfg->kwfstdec));
	else
		wrapper->dec=qtk_kwfstdec_new(&(cfg->kwfstdec));
	//wrapper->dec->trans_model=cfg->extra.nnet3.t_mdl->trans_model; TODO transmodel
	wrapper->dec->onnx_dec = 1;

	return wrapper;
}

int qtk_wenet_wrapper_start3(qtk_wenet_wrapper_t* wrapper);

int qtk_wenet_wrapper_start(qtk_wenet_wrapper_t* wrapper)
{
	return qtk_wenet_wrapper_start2(wrapper,0,0);
}

int qtk_wenet_wrapper_start2(qtk_wenet_wrapper_t* wrapper,char *data,int bytes)
{	
	int ret;
	wtk_wfstenv_cfg_init2(&(wrapper->env));
	if(bytes>0)
	{
		ret=wtk_cfg_file_feed(wrapper->env_parser,data,bytes);
		if(ret!=0){goto end;}
		ret=wtk_wfstenv_cfg_update_local2(&(wrapper->env),wrapper->env_parser->main,0);
		if(ret!=0){goto end;}
	}

    if(wrapper->cfg->use_vad && wrapper->env.use_vad)
    {   
        wtk_vad_start(wrapper->vad);
		wtk_strbuf_reset(wrapper->vad_buf);
    }

	{
		if(wrapper->dec)
			qtk_kwfstdec_start(wrapper->dec);
		else
			qtk_kwfstdec_lite_start(wrapper->dec_lite);
	}
	ret=0;
	end:
		return ret;
}

int qtk_wenet_wrapper_start3(qtk_wenet_wrapper_t* wrapper)
{	
    if(wrapper->cfg->use_vad && wrapper->env.use_vad)
    {   
        wtk_vad_start(wrapper->vad);
    }

	if(wrapper->dec)
		qtk_kwfstdec_start(wrapper->dec);
	else
		qtk_kwfstdec_lite_start(wrapper->dec_lite);

	return 0;
}

void qtk_wenet_on_kxparm_end(qtk_wenet_wrapper_t* wrapper)
{
#ifdef ONNX_DEC
    wrapper->shape[1] = wrapper->num_feats;
    wrapper->shape[2] = 80;
    // wtk_debug("%d %d %d
    // %d\n",wrapper->shape[0],wrapper->shape[1],wrapper->shape[2],wrapper->shape2);
    // wtk_debug("%d %d\n",wrapper->feat->pos,wrapper->num_feats);
    if (wrapper->num_feats <= 10) {
        return;
    }
    qtk_onnxruntime_feed(wrapper->onnx, wrapper->feat->data,
                         wrapper->num_feats * 80,
                         cast(int64_t *, wrapper->shape), 3, 0, 0);
    qtk_onnxruntime_feed(wrapper->onnx, &wrapper->num_feats, 1,
                         cast(int64_t *, &wrapper->shape2), 1, 1, 1);
    qtk_onnxruntime_run(wrapper->onnx);
    float *v2;
    int64_t *shape, size;
    int i, j;
    // v = qtk_onnxruntime_getout(wrapper->onnx,2);
    shape = qtk_onnxruntime_get_outshape(wrapper->onnx, 2, &size);
    // wtk_debug("%d %d %d\n",shape[0],shape[1],shape[2]);
    // print_float(v,10);
    v2 = qtk_onnxruntime_getout(wrapper->onnx, 2);
    // print_float(v2,10);

    float *p1 = v2;
    float *p2, max = -100.0;
    int index = 0;
    int k;
    int cur_best = 0, last_best = 0, last_speech = 0;
    for (i = 0; i < shape[1]; i++) {
        p2 = prior;
        //		print_float(v2 + shape[2]*i, shape[2]);
        for (j = 0; j < shape[2]; j++) {
			*p1 -= *p2;
			p1++;
			p2++;
		}
		//print_float(v2 + shape[2]*i, shape[2]);
		wtk_softmax(v2 + shape[2]*i, shape[2]);
		wtk_add_log(v2 + shape[2]*i, shape[2]);
		//memcpy(v2 + shape[2]*i,py_feat+shape[2]*i,sizeof(float)*shape[2]);
		//print_float(v2 + shape[2]*i, shape[2]);
		if(*(v2 + shape[2]*i) < -0.0202027)
		{
			//wtk_debug("%f\n",*(v2 + shape[2]*i));
			max = -100.0;
			for(k = 0;k < shape[2];k++)
			{
				if(max < *(v2+shape[2]*i+k))
				{
					max = *(v2+shape[2]*i+k);
					cur_best = k;
				}
			}
			if(cur_best!=0 && last_best==cur_best && last_speech != i-1)
			{
				qtk_kwfstdec_feed2(wrapper->dec,v2 + shape[2]*(i-1),index);
				index++;
			}
			qtk_kwfstdec_feed2(wrapper->dec,v2 + shape[2]*i,index);
			index++;
			last_best = cur_best;
			last_speech = i;
		}
	}
	wtk_free(shape);
#endif
}
//int f_cnt = 0;
void qtk_wenet_on_kxparm(qtk_wenet_wrapper_t* wrapper,wtk_kfeat_t *feat)
{
	int v = 80;
        if (wrapper->cfg->speedup == 1 && feat->index % 3 == 0) {
            return;
        }
//	printf(">>>>>>>>>>>> %d\n",f_cnt++);
//	print_float(feat->v,80);
//	memcpy(feat->v,py_feat+80*f_cnt,80*sizeof(float));
//	f_cnt++;
	wtk_strbuf_push_float(wrapper->feat,feat->v,v);
	wrapper->num_feats++;
}

int qtk_wenet_wrapper_feed3(qtk_wenet_wrapper_t* wrapper,short *data,int bytes,int is_end)
{
	int ret=0;
	if(is_end)
	{
		wrapper->time_stop=time_get_ms();
   	}

	wtk_kxparm_feed(wrapper->kxparm,data,bytes,is_end);

	return ret;
}

int qtk_wenet_wrapper_feed(qtk_wenet_wrapper_t* wrapper,char *data,int bytes,int is_end)
{
	int ret = 0;
	wrapper->wav_bytes+=bytes;
	//wtk_debug("%p %d\n",wrapper->vad,wrapper->env.use_vad);
    if(wrapper->vad && wrapper->env.use_vad)
    {
        //ret = qtk_wenet_wrapper_feed_vad(wrapper,data,bytes,is_end);
    }else if(wrapper->kxparm)
    {
        qtk_wenet_wrapper_feed3(wrapper,(short*)data,bytes/2,is_end);
    }
	return ret;
}

void qtk_wenet_wrapper_reset(qtk_wenet_wrapper_t* wrapper)
{	
	wtk_cfg_file_reset(wrapper->env_parser);
    wtk_queue_init(&(wrapper->vad_q));
    wrapper->last_vframe_state=wtk_vframe_sil;
	wrapper->data_left=0;
	wrapper->num_feats=0;
	{
		wtk_strbuf_reset(wrapper->res_buf);
		wtk_strbuf_reset(wrapper->hint_buf);
        if(wrapper->dec){
        	if (wrapper->dec->cfg->use_eval)
        		qtk_kwfstdec_reset3(wrapper->dec);
        	else
        		qtk_kwfstdec_reset(wrapper->dec);
        }
        else
            qtk_kwfstdec_lite_reset(wrapper->dec_lite);

	}
    if(wrapper->vad && wrapper->env.use_vad)
    {
        wtk_vad_reset(wrapper->vad);
//		wtk_strbuf_reset(wrapper->vad_buf);
//		wtk_strbuf_reset(wrapper->whole_buf);
    }
	wtk_wfstenv_cfg_init2(&(wrapper->env));
	wrapper->time_stop=0;
	wrapper->wav_bytes=0;
    wrapper->rec_wav_bytes=0;
	wrapper->index=0;
//	wrapper->sil=1;
	if(wrapper->kxparm)
	{
		wtk_kxparm_reset(wrapper->kxparm);
	}
	if(wrapper->chnlike!=NULL)
	{
		wtk_chnlike_reset(wrapper->chnlike);
	}
	if(wrapper->xbnf)
	{
		wtk_xbnf_rec_reset(wrapper->xbnf);
	}
#ifdef ONNX_DEC
	qtk_onnxruntime_reset(wrapper->onnx);
#endif
	wtk_queue_init(&(wrapper->parm_q));
	wtk_json_reset(wrapper->json);
	wtk_strbuf_reset(wrapper->json_buf);
	wtk_strbuf_reset(wrapper->feat);
}

void qtk_wenet_wrapper_reset2(qtk_wenet_wrapper_t* wrapper)
{	
    wtk_strbuf_reset(wrapper->hint_buf);

    qtk_kwfstdec_reset(wrapper->dec);
#ifdef ONNX_DEC
    qtk_onnxruntime_reset(wrapper->onnx);
#endif
}

void qtk_wenet_wrapper_delete(qtk_wenet_wrapper_t* wrapper)
{
	wtk_cfg_file_delete(wrapper->env_parser);

	{
		wtk_strbuf_delete(wrapper->res_buf);
		wtk_strbuf_delete(wrapper->hint_buf);
        if(wrapper->dec)
        {
        	if (wrapper->dec->cfg->use_eval)
        		qtk_kwfstdec_delete3(wrapper->dec);
        	else
        		qtk_kwfstdec_delete(wrapper->dec);
        }
        else
            qtk_kwfstdec_lite_delete(wrapper->dec_lite);

	}
    if(wrapper->vad)
    {
        wtk_vad_delete(wrapper->vad);
		wtk_strbuf_delete(wrapper->vad_buf);
    }

	if(wrapper->lex)
	{
		wtk_lex_delete(wrapper->lex);
	}

	wtk_json_delete(wrapper->json);
	wtk_strbuf_delete(wrapper->json_buf);
#ifdef ONNX_DEC
	qtk_onnxruntime_delete(wrapper->onnx);
#endif
	if(wrapper->kxparm)
	{
		wtk_kxparm_delete(wrapper->kxparm);
	}
	if(wrapper->chnlike)
	{
		wtk_chnlike_delete(wrapper->chnlike);
	}
	if(wrapper->xbnf)
	{
		wtk_xbnf_rec_delete(wrapper->xbnf);
	}
        if (wrapper->egram) {
            wtk_egram_delete(wrapper->egram);
        }
        wtk_strbuf_delete(wrapper->feat);

        wtk_free(wrapper);
}

void qtk_wenet_wrapper_send_back_feature(qtk_wenet_wrapper_t *warpper,wtk_feat_t *f)
{
	wtk_feat_t *f2;

	if(f->app_hook)
	{
		f2=(wtk_feat_t*)f->app_hook;
		wtk_feat_push_back(f2);
	}
	wtk_feat_push_back(f);
}

void qtk_wenet_wrapper_get_result(qtk_wenet_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_json_item_t *item,*item2;
	wtk_json_t *json=wrapper->json;
	wtk_strbuf_t *buf=wrapper->json_buf;
	double t,f;
	//float strlike=0.0;
	//float best_final,conf;
	qtk_kwfstdec_get_result(wrapper->dec,wrapper->res_buf);
	item=wtk_json_new_object(json);
	item2=wtk_json_new_object(json);
	wtk_json_obj_add_ref_str_s(json,item,"version",&(wrapper->cfg->version.ver));
	wtk_json_obj_add_ref_str_s(json,item,"res",&(wrapper->cfg->res));
	if(wrapper->res_buf)
	{
		wtk_json_obj_add_str2_s(json,item,"rec",wrapper->res_buf->data,wrapper->res_buf->pos);
		wtk_json_obj_add_ref_number_s(json,item,"conf",wrapper->dec->conf);
	}

	//t=1000.0/(2*wtk_fextra_cfg_get_sample_rate(&(wrapper->cfg->extra)));
	f=wrapper->wav_bytes;
	wtk_json_obj_add_ref_number_s(json,item2,"wav",(int)f);
	if(wrapper->vad && wrapper->env.use_vad)
	{
		f=wrapper->rec_wav_bytes;
	}
	wtk_json_obj_add_ref_number_s(json,item2,"vad",(int)f);
	wtk_json_obj_add_ref_number_s(json,item2,"sys",0);

	t=time_get_ms();
	f=t-wrapper->time_stop;
//	wtk_json_obj_add_ref_number_s(json,item2,"dly",(int)f);
//	wtk_json_obj_add_ref_number_s(json,item2,"dfm",0);
//	wtk_json_obj_add_item2_s(json,item,"time",item2);

	wtk_json_item_print(item,buf);
    wtk_string_set(v, buf->data, buf->pos);
    //	wtk_string_set(v,wrapper->res_buf->data,wrapper->res_buf->pos);
}

void qtk_wenet_wrapper_get_hint_result(qtk_wenet_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_strbuf_t *tmp_rec;
	wtk_strbuf_t *hint=wrapper->hint_buf;
	wtk_string_t v2;

	qtk_wenet_on_kxparm_end(wrapper);
	tmp_rec=wtk_strbuf_new(256,1);
	wtk_strbuf_reset(hint);
	wtk_strbuf_push_s(hint,"{\"hint\":\"");
	if(wrapper->dec)
		qtk_kwfstdec_get_hint_result(wrapper->dec,tmp_rec);
	else
		qtk_kwfstdec_lite_get_result(wrapper->dec_lite,tmp_rec);
	if(wrapper->vad_buf && wrapper->vad_buf->pos!=0)
	{
		wtk_strbuf_push(hint,wrapper->vad_buf->data,wrapper->vad_buf->pos);
	}
	if(wrapper->lex)
	{
    	v2=wtk_lex_process(wrapper->lex,tmp_rec->data,tmp_rec->pos);
		wtk_strbuf_push(hint,v2.data,v2.len);
		wtk_lex_reset(wrapper->lex);	
	}else
	{
		wtk_strbuf_push(hint,tmp_rec->data,tmp_rec->pos);
	}
	wtk_strbuf_push_s(hint,"\"}");
	wtk_string_set(v,hint->data,hint->pos);

	wtk_strbuf_delete(tmp_rec);
	qtk_wenet_wrapper_reset2(wrapper);
	qtk_kwfstdec_start(wrapper->dec);
}

void qtk_wenet_wrapper_get_vad_result(qtk_wenet_wrapper_t *wrapper,wtk_string_t *v)
{
	wtk_strbuf_t *tmp_rec;
	wtk_strbuf_t *hint=wrapper->vad_buf;

	tmp_rec=wtk_strbuf_new(256,1);
	wtk_strbuf_push_s(tmp_rec,"{\"vad\":\"");
	wtk_strbuf_push(tmp_rec,hint->data,hint->pos);
	wtk_strbuf_push_s(tmp_rec,"\"}");
	wtk_string_set(v,tmp_rec->data,tmp_rec->pos);

	wtk_strbuf_reset(hint);
	wtk_strbuf_delete(tmp_rec);
}

void qtk_wenet_wrapper_set_xbnf(qtk_wenet_wrapper_t *wrapper,char* buf,int len)
{
	if(wrapper->xbnf)
	{
		wtk_xbnf_reset(wrapper->cfg->xbnf.xb);
		wtk_xbnf_compile(wrapper->cfg->xbnf.xb,buf,len);
	}
}

int qtk_wenet_wrapper_set_context_fn(qtk_wenet_wrapper_t *wrapper, char *fn) {
    int ret = -1;
    char *data;
    int len;

    if (wrapper->dec && wrapper->cfg->use_context) {
        data = file_read_buf(fn, &len);
        wtk_egram_ebnf2fst(wrapper->egram, data, len);
        wtk_egram_dump3(wrapper->egram, wrapper->dec->context_net,
                        wrapper->dec->net->cfg->sym_out);
        ret = 0;
        wtk_free(data);
    }

    return ret;
}

int qtk_wenet_wrapper_set_context_str(qtk_wenet_wrapper_t *wrapper, char *buf,
                                      int len) {
    int ret = -1;

    if (wrapper->dec && wrapper->cfg->use_context) {
        wtk_egram_ebnf2fst(wrapper->egram, buf, len);
        wtk_egram_dump3(wrapper->egram, wrapper->dec->context_net,
                        wrapper->dec->net->cfg->sym_out);
        ret = 0;
    }
    return ret;
}
