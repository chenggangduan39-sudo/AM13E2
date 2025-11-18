#include "qtk_svprintc.h"

#define QTK_SVPRINT_COL  (64)
#define QTK_MIN_SPEECH	(5)
int qtk_svprintc_plda_eval(qtk_svprintc_t *svprintc, float *prob,int len);

int qtk_svprintc_read(qtk_torchnn_t **nn,wtk_source_t *src)
{
    int ret = 0;
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);

    *nn = qtk_torchnn_read(src,buf,0,QTK_SVPRINT_COL,1);

    wtk_strbuf_delete(buf);
    return ret;
}

qtk_svprintc_t *qtk_svprintc_new(qtk_svprintc_cfg_t *cfg)
{
    qtk_svprintc_t *svprintc = wtk_malloc(sizeof(*svprintc));
    wtk_source_loader_t sl;

    memset(svprintc,0,sizeof(qtk_svprintc_t));
    svprintc->cfg = cfg;
    svprintc->x = wtk_nnet3_xvector_compute_new2(&cfg->xvector);

    // wtk_queue_init(&svprintc->vad_out);
    svprintc->vad = wtk_kvad_new(&cfg->kvad);
    svprintc->vad_state = 0;
    //svprint_nn
    sl.hook=0;
	sl.vf=wtk_source_load_file_v;
    if(cfg->svprint_nn_fn){
        wtk_source_loader_load(&sl, &(svprintc->svprint_nn),(wtk_source_load_handler_t)qtk_svprintc_read, cfg->svprint_nn_fn);
    }else{
        wtk_debug("svprint nn fn error\n");
    }
    svprintc->x->nn = svprintc->svprint_nn;
    wtk_nnet3_xvector_compute_set_nn_notify(svprintc->x);
    wtk_source_loader_load(&sl, svprintc->x,(wtk_source_load_handler_t)xvector_pool_read, cfg->pool_fn);
    // printf("new %p\n",svprintc);

	svprintc->plda_scoring = wtk_ivector_plda_scoring_new(&cfg->plda);
	svprintc->speech_tmp_buf = wtk_strbuf_new(4096,1.0f);
	return svprintc;
}

int qtk_svprintc_delete(qtk_svprintc_t *svprintc)
{
	// printf("delete %p\n",svprintc);
    if(svprintc == NULL)
        return 0;

    if(svprintc->x)
        wtk_nnet3_xvector_compute_delete(svprintc->x);

    qtk_torchnn_delete(svprintc->svprint_nn);

    if (svprintc->vad) {
        wtk_kvad_delete(svprintc->vad);
    }

    if (svprintc->plda_scoring)
        wtk_ivector_plda_scoring_delete(svprintc->plda_scoring);

    wtk_strbuf_delete(svprintc->speech_tmp_buf);
    wtk_free(svprintc);
    return 0;
}

int qtk_svprintc_start(qtk_svprintc_t *svprintc)
{
    int ret = 0;

    ret=wtk_nnet3_xvector_compute_start(svprintc->x);
    wtk_kvad_start(svprintc->vad);
    return ret;
}

int qtk_svprintc_feed(qtk_svprintc_t *svprintc,short *data, int len, int is_end)
{
    int ret = 0;
	wtk_vecf_t *vec = NULL;

	if(0){
	    // wtk_queue_t *q=&svprintc->vad->output_q;
	    // wtk_vframe_t *vf=NULL;
	    // wtk_queue_node_t *qn;
	   
	    // wtk_kvad_feed(svprintc->vad,data,len, is_end);

	    // while(1)
		// {
		// 	qn=wtk_queue_pop(q);
		// 	if(!qn){break;}
		// 	vf=data_offset2(qn,wtk_vframe_t,q_n);
		// switch(svprintc->vad_state)
		// {
		// case 0:
		//     if(vf->state!=wtk_vframe_sil)   
		// 		{
		// 	wtk_nnet3_xvector_compute_reset(svprintc->x);
		// 	wtk_nnet3_xvector_compute_start(svprintc->x);
		// 	wtk_nnet3_xvector_compute_feed(svprintc->x,vf->wav_data,vf->frame_step,0);
		// 	svprintc->vad_state=1;
		//     }
		//     break;
		// case 1:
		//     if(vf->state==wtk_vframe_sil)
		//     {
		// 	wtk_nnet3_xvector_compute_feed(svprintc->x,vf->wav_data,vf->frame_step,1);
		// 	vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
		// 	svprintc->notify(svprintc->user_data,vec->p,vec->len);
		// 	svprintc->vad_state = 0;
		//     }else   
		//     {
		// 	wtk_nnet3_xvector_compute_feed(svprintc->x,vf->wav_data,vf->frame_step,0);
		//     }
		//     break;
		// default:
		//     break;
		// }
	    // }
	    // if(svprintc->vad_state == 1 && is_end){
		// wtk_nnet3_xvector_compute_feed(svprintc->x,NULL,0,1);
		// vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
		// svprintc->notify(svprintc->user_data,vec->p,vec->len);
		// svprintc->vad_state = 0;
	    // }
	}else{
        wtk_debug("compute feed\n");
		wtk_nnet3_xvector_compute_feed(svprintc->x,data,len,is_end);
		// wtk_debug("----> row %d  col %d \n",svprintc->x->pool->xvector->row,svprintc->x->pool->xvector->col);
		// qtk_blas_matrix_print(svprintc->x->pool->xvector);
        if(is_end){
			vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
			if(svprintc->cfg->use_plda){
				qtk_svprintc_plda_eval(svprintc,vec->p,vec->len);
			}
			svprintc->notify(svprintc->user_data,svprintc->x->pool->xvector->m,svprintc->x->pool->xvector->col);	
		}
        wtk_debug("compute feed end %p\n",svprintc);
	}
    return ret;
}

//当音频不到3s的时候补成3s
short* qtk_svprintc_pad_data(qtk_svprintc_t *svprintc,short *data, int len)
{
	short* to_data = NULL;
	int s3l = 16000*QTK_MIN_SPEECH;	//最低3s
	int pad = (s3l-len)/2;
	int i = 0;

	to_data = (short*)wtk_malloc(s3l*sizeof(short));
	memset(to_data,0,s3l*sizeof(short));
	memcpy(to_data+pad,data,sizeof(short)*len);
	for(i = 0; i < pad; ++i){
		to_data[pad-i-1] = to_data[pad+i];
		to_data[pad+len-1+i] = to_data[pad+len-1-i];
	}

	return to_data;
}
//自身填充
short* qtk_svprintc_pad_data2(qtk_svprintc_t *svprintc,short *data, int len)
{
	short* to_data = NULL;
	int s3l = 16000*QTK_MIN_SPEECH;	//最低3s
	// int s3l = 160482/2;
	int pad = s3l;
	int step = 0,i = 0;

	to_data = (short*)wtk_malloc(s3l*sizeof(short));
	memset(to_data,0,s3l*sizeof(short));
	do{
		step = min(pad,len);
		memcpy(to_data+i,data,step*sizeof(short));
		pad -= step;
		i += step;
	}while(pad > 0);

	return to_data;
}
// static int outn = 1;
int qtk_svprintc_feed2(qtk_svprintc_t *svprintc,short *data, int len, int is_end)
{
    int ret = 0;
	wtk_vecf_t *vec = NULL;
	short *to_data = NULL;
	int u_len = 0;

        qtk_blas_matrix_t *out_x = qtk_blas_matrix_new(
            svprintc->x->pool->xvector->row, svprintc->x->pool->xvector->col);

        qtk_blas_matrix_zero(out_x);
	// if( len < 16000*QTK_MIN_SPEECH){
	// 	to_data = qtk_svprintc_pad_data2(svprintc,(short*)data,len);
	// 	u_len = 16000*QTK_MIN_SPEECH;
	// }else{
	// 	to_data = malloc(len*sizeof(short));
	// 	memcpy(to_data,data,len*sizeof(short));
	// 	u_len = len;
	// }
	if(1){
	    wtk_queue_t *q=&svprintc->vad->output_q;
	    wtk_vframe_t *vf=NULL;
	    wtk_queue_node_t *qn;
	   	wtk_strbuf_t *tmp_buf = svprintc->speech_tmp_buf;
	    // wtk_kvad_feed(svprintc->vad,to_data,u_len, is_end);
		wtk_kvad_feed(svprintc->vad,data,len,is_end);
		wtk_strbuf_reset(tmp_buf);
	    while(1)
		{
			qn=wtk_queue_pop(q);
			if(!qn){break;}
			vf=data_offset2(qn,wtk_vframe_t,q_n);
			switch(svprintc->vad_state){
			case 0:
				if(vf->state!=wtk_vframe_sil)   
				{
					wtk_strbuf_push(tmp_buf,(char*)vf->wav_data,vf->frame_step*sizeof(short));
					svprintc->vad_state=1;
				}
				break;
			case 1:
				if(vf->state==wtk_vframe_sil)
				{
					svprintc->vad_state = 0;
				}else   
				{
					wtk_strbuf_push(tmp_buf,(char*)vf->wav_data,vf->frame_step*sizeof(short));
				}
				break;
			default:
				break;
			}
	    }
	    if(is_end){
			// char cccfn[124] = {0};
			// sprintf(cccfn,"%d.pcm",outn); 
			// FILE *outf = fopen(cccfn,"wb");
			// fwrite(tmp_buf->data,tmp_buf->pos,1,outf);
			// fclose(outf);
			// outn+=1;
			if( tmp_buf->pos/2 < 16000*QTK_MIN_SPEECH){
				to_data = qtk_svprintc_pad_data2(svprintc,(short*)tmp_buf->data,tmp_buf->pos >> 1);
				u_len = 16000*QTK_MIN_SPEECH;
			}else{
				to_data = malloc(tmp_buf->pos);
				memcpy(to_data,tmp_buf->data,tmp_buf->pos);
				u_len = len;
			}
			// wtk_nnet3_xvector_compute_feed(svprintc->x,(short*)tmp_buf->data,tmp_buf->pos >> 1,1);
			wtk_nnet3_xvector_compute_feed(svprintc->x,to_data,u_len,1);
			vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
			memcpy(out_x->m,vec->p,vec->len*sizeof(float));
			if(svprintc->cfg->use_plda){
				qtk_svprintc_plda_eval(svprintc,out_x->m,out_x->col*out_x->row);
			}
			svprintc->notify(svprintc->user_data,out_x->m,out_x->row*out_x->col);
	    }
	}else{
        wtk_debug("compute feed\n");
		// if(len < 16000){
		// 	wtk_debug("the data to short\n");
		// 	goto end;
		// }
		if(len < 16000*QTK_MIN_SPEECH){	//最小是ns音频
			to_data = qtk_svprintc_pad_data2(svprintc,data,len);
			u_len = 16000*QTK_MIN_SPEECH;
		}else{
			to_data = malloc(sizeof(short)*len);
			memcpy(to_data,data,len*sizeof(short));
			u_len = len;
		}
		// int i = 0;
		// char *c = NULL;
		// c = to_data;
		// for(i = 0; i < 16000*QTK_MIN_SPEECH*2; ++i){
		// 	printf("%d\n",c[i]);
		// }
		// exit(1);
		// do{
		// 	step = min(16000*QTK_MIN_SPEECH,u_len-use);
		// 	qtk_svprintc_start(svprintc);
		// 	wtk_nnet3_xvector_compute_feed(svprintc->x,to_data+use,step,1);
		// 	qtk_blas_matrix_add_mat(out_x,svprintc->x->pool->xvector);
		// 	qtk_svprintc_reset(svprintc);
		// 	use+=16000;	//认为每次走16000个点
		// 	con++;
		// }while((u_len-use)>QTK_MIN_SPEECH*16000);
		wtk_nnet3_xvector_compute_feed(svprintc->x,to_data,u_len,is_end);
		if(is_end){
			// qtk_blas_matrix_scale2(out_x,1.0f/con);
			vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
			memcpy(out_x->m,vec->p,vec->len*sizeof(float));
			if(svprintc->cfg->use_plda){
				qtk_svprintc_plda_eval(svprintc,out_x->m,out_x->col*out_x->row);
			}
			svprintc->notify(svprintc->user_data,out_x->m,out_x->row*out_x->col);
		}
        wtk_debug("compute feed end\n");
	}
        wtk_free(to_data);
        qtk_blas_matrix_delete(out_x);
        return ret;
}

int qtk_svprintc_plda_eval(qtk_svprintc_t *svprintc, float *prob,int len)
{
        wtk_vecf_t vec, *out = NULL;

        vec.p = prob;
        vec.len = len;
        // vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
        out = wtk_ivector_plda_scoring_transfrom_ivector(svprintc->plda_scoring,
                                                         &vec); // t2
        // print_float(v->scoring->ivector->p,v->scoring->ivector->len);
        //  out=wtk_ivector_plda_scoring_ivector_normalization(svprintc->plda_scoring,spk_cnt);

	memcpy(prob,out->p,len*sizeof(float));
    wtk_nnet3_xvector_compute_normalize_reset(svprintc->x);
    // v->x->spk_cnt = 0;
	return 0;
}

void qtk_svprintc_set_notify(qtk_svprintc_t *svprintc,void *user_data,qtk_svprintc_notify_f *notify)
{
    svprintc->user_data = user_data;
    svprintc->notify = notify;
    return;
}

void qtk_svprintc_reset(qtk_svprintc_t *svprint)
{
    svprint->vad_state = 0;
    wtk_nnet3_xvector_compute_reset(svprint->x);
	qtk_torchnn_reset(svprint->svprint_nn);
    wtk_kvad_reset(svprint->vad);
	wtk_strbuf_reset(svprint->speech_tmp_buf);
    return;
}
//大于0.65是置信
float svprintc_feat_compute_likelihood(qtk_svprintc_t *svprint,wtk_vecf_t* feat1, int cnt,wtk_vecf_t*feat2)
{
#if 0
	float res = 0.0,tmp;
	int i;
	float *p1 = feat1->p;
	float *p2 = feat2->p;
	float norm1 = 0.0f,norm2 = 0.0f;

	for(i = 0; i < feat1->len; ++i){
		norm1 += p1[i] * p1[i];
		norm2 += p2[i] * p2[i];
	}
	norm1 = powf(norm1,0.5);
	norm2 = powf(norm2,0.5);

	for(i = 0; i < feat1->len; ++i){
		p1[i] = p1[i]/norm1;
		p2[i] = p2[i]/norm2;
	}
	
	for(i = 0;i < feat1->len; i++,p1++,p2++)
	{
		tmp = (*p1) - (*p2);
		res += tmp*tmp;
	}
	// printf("res %f\n",res);
	//res = pow(res,0.5);

	return 1.0f - (0.5f*res);
#else
	return wtk_ivector_plda_scoring_loglikelihood(svprint->plda_scoring,feat1,cnt,feat2);
#endif
}

float qtk_svprintc_get_likehood(qtk_svprintc_t *svprint,float *data1,int len,int cnt, float *data2,int len2)
{
    float ret = 0.0f;
    wtk_vecf_t *vec1 = NULL;
    wtk_vecf_t *vec2 = NULL;

	// wtk_debug("like hood start\n");
    if(!data1 || !data2 || !len || !len2){return 0.0f;}
	vec1 = wtk_vecf_new(len);
    vec2 = wtk_vecf_new(len2);

	if(!vec1 || !vec2){
		wtk_debug("mem error %d %d\n",len,len2);
	}

    memcpy(vec1->p,data1,sizeof(float)*len);
    memcpy(vec2->p,data2,sizeof(float)*len2);
    ret = svprintc_feat_compute_likelihood(svprint,vec1,cnt,vec2);
    // wtk_debug("=====> %f\n",ret);
    wtk_vecf_delete(vec1);
    wtk_vecf_delete(vec2);
    return ret ;
}

// int qtk_svprint_test(qtk_svprintc_t *svprintc,float *f,int in_col,int in_row,int is_end)
// {
// 	if(is_end){
// 		qtk_torchnn_flush(svprintc->x->nn);
// 		goto end;
// 	}
// 	qtk_blas_matrix_t*out = qtk_torchnn_feed(svprintc->x->nn,f,in_col,in_row,0);
// 	// wtk_debug("%p\n",out);
// 	if(out){
// 		svprintc->x->nn->notify(svprintc->x->nn->ths,out);
// 	}
// 	// wtk_debug("out %d %d\n",out->row,out->col);
// end:
// 	return 0;
// }