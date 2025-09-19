#include "qtk_sv_cluster.h"

#define QTK_SVPRINT_COL  (64)
#define QTK_MIN_SPEECH	(5)
wtk_vecf_t* qtk_sv_cluster_plda_eval(qtk_sv_cluster_t *svprintc, float *prob,int len);
float qtk_sv_cluster_feat_compute_likelihood(qtk_sv_cluster_t *svprint,wtk_vecf_t* feat1, int cnt,wtk_vecf_t*feat2);
int qtk_sv_cluster_read(qtk_torchnn_t **nn,wtk_source_t *src)
{
    int ret = 0;
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);

    *nn = qtk_torchnn_read(src,buf,0,QTK_SVPRINT_COL,1);

    wtk_strbuf_delete(buf);
    return ret;
}

sv_node_t* sv_node_new(qtk_sv_cluster_t *svprintc)
{
	sv_node_t* node = (sv_node_t*)wtk_heap_malloc(svprintc->heap,sizeof(sv_node_t));

	node->start = 0;
	node->end = 0;
	node->vec = wtk_vecf_heap_new(svprintc->heap,svprintc->nvec);
	return node;
}

sv_cluster_t* sv_cluster_new(qtk_sv_cluster_t *svprintc)
{
	sv_cluster_t* cluster = (sv_cluster_t*)wtk_malloc(sizeof(sv_cluster_t));

	cluster->cnt = 0;
	cluster->mean = wtk_vecf_new(svprintc->nvec);
	wtk_queue_init(&(cluster->queue));
	return cluster;
}

qtk_sv_cluster_t *qtk_sv_cluster_new(qtk_sv_cluster_cfg_t *cfg)
{
	qtk_sv_cluster_t *svprintc = wtk_malloc(sizeof(*svprintc));

    memset(svprintc,0,sizeof(qtk_sv_cluster_t));
    svprintc->cfg = cfg;
    svprintc->x = wtk_nnet3_xvector_compute_new2(&cfg->xvector);

    // wtk_queue_init(&svprintc->vad_out);
    svprintc->vad = wtk_kvad_new(&cfg->kvad);
    svprintc->vad_state = 0;
    svprintc->heap = wtk_heap_new(4096);
    //svprint_nn
    if(cfg->svprint_nn_fn){
        wtk_source_loader_load(&(cfg->sl), &(svprintc->svprint_nn),(wtk_source_load_handler_t)qtk_sv_cluster_read, cfg->svprint_nn_fn);
        svprintc->x->nn = svprintc->svprint_nn;
        wtk_nnet3_xvector_compute_set_nn_notify(svprintc->x);
    }
    wtk_source_loader_load(&(cfg->sl), svprintc->x,(wtk_source_load_handler_t)xvector_pool_read, cfg->pool_fn);
    // printf("new %p\n",svprintc);
	svprintc->plda_scoring = wtk_ivector_plda_scoring_new(&cfg->plda);
	svprintc->x->spk_mean=wtk_vecf_new(svprintc->plda_scoring->ivector->len);
	svprintc->speech_tmp_buf = wtk_strbuf_new(4096,1.0f);
	svprintc->nvec = svprintc->x->spk_mean->len;
	svprintc->notify = NULL;
	svprintc->user_data = NULL;
	svprintc->ncluster = 0;
	svprintc->maxn = 4;
	svprintc->clusters = (sv_cluster_t**)wtk_calloc(svprintc->maxn,sizeof(sv_cluster_t));
	int i;
	for(i=0;i<svprintc->maxn;i++)
	{
		svprintc->clusters[i] = sv_cluster_new(svprintc);
	}
	//svprintc->log = wtk_wavfile_new(16000);
	return svprintc;
}

int qtk_sv_cluster_delete(qtk_sv_cluster_t *svprintc)
{
	int i;
	// printf("delete %p\n",svprintc);
    if(svprintc == NULL)
        return 0;

    if(svprintc->x)
        wtk_nnet3_xvector_compute_delete(svprintc->x);

    //qtk_torchnn_delete(svprintc->svprint_nn);

    if(svprintc->vad)
	{
        	wtk_kvad_delete(svprintc->vad);
	}

	if(svprintc->plda_scoring)
	{
		wtk_ivector_plda_scoring_delete(svprintc->plda_scoring);
	}
	for(i=0;i<svprintc->maxn;i++)
	{
		//qtk_sv_cluster_clean(&(svprintc->clusters[i]->queue));
		wtk_vecf_delete(svprintc->clusters[i]->mean);
		wtk_free(svprintc->clusters[i]);
	}
	wtk_free(svprintc->clusters);
	wtk_heap_delete(svprintc->heap);
	wtk_strbuf_delete(svprintc->speech_tmp_buf);
    wtk_free(svprintc);
    return 0;
}

void qtk_sv_cluster_set_notify(qtk_sv_cluster_t *svprintc,void *user_data,qtk_sv_cluster_notify_f *notify)
{
    svprintc->user_data = user_data;
    svprintc->notify = notify;
    return;
}

void qtk_sv_cluster_reset(qtk_sv_cluster_t *svprint)
{
	int i;

    svprint->vad_state = 0;
    wtk_nnet3_xvector_compute_reset(svprint->x);
	//qtk_torchnn_reset(svprint->svprint_nn);
    wtk_kvad_reset(svprint->vad);
	wtk_strbuf_reset(svprint->speech_tmp_buf);
	for(i=0;i<svprint->ncluster;i++)
	{
		//qtk_sv_cluster_clean(&(svprintc->clusters[i]->queue));
		wtk_vecf_zero(svprint->clusters[i]->mean);
		svprint->clusters[i]->cnt=0;
		wtk_queue_init(&(svprint->clusters[i]->queue));
	}
	wtk_heap_reset(svprint->heap);
	svprint->ncluster = 0;
    return;
}

void qtk_sv_cluster_reset2(qtk_sv_cluster_t *svprint)
{
    //svprint->vad_state = 0;
    wtk_nnet3_xvector_compute_reset(svprint->x);
    //wtk_kvad_reset(svprint->vad);
	wtk_strbuf_reset(svprint->speech_tmp_buf);
	//wtk_heap_reset(svprint->heap);
	//svprint->ncluster = 0;
    return;
}

void qtk_sv_cluster_reset3(qtk_sv_cluster_t *svprint)
{
    //svprint->vad_state = 0;
    wtk_nnet3_xvector_compute_reset(svprint->x);
    wtk_kvad_reset(svprint->vad);
	wtk_strbuf_reset(svprint->speech_tmp_buf);
	//wtk_heap_reset(svprint->heap);
	//svprint->ncluster = 0;
    return;
}

int qtk_sv_cluster_start(qtk_sv_cluster_t *svprintc)
{
    int ret = 0;

    ret=wtk_nnet3_xvector_compute_start(svprintc->x);
    wtk_kvad_start(svprintc->vad);
    return ret;
}

//当音频不到3s的时候补成3s
short* qtk_sv_cluster_pad_data(qtk_sv_cluster_t *svprintc,short *data, int len)
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

void qtk_sv_cluster_update(sv_cluster_t *cluster,sv_node_t *node)
{
	int i;
	int n = cluster->mean->len;
	int cnt = cluster->cnt;
	float *p = cluster->mean->p;
	float *v = node->vec->p;

	if(cnt < 6)
	{
		for(i = 0; i < n; i++)
		{
			*p = ((*p)*cnt + (*v))/(cnt + 1);
			p++;
			v++;
		}
		cluster->cnt++;
	}
	wtk_queue_push(&(cluster->queue),&(node->node));
}

void qtk_sv_cluster_process(qtk_sv_cluster_t *svprintc, float *vec, int len, int start_index, int end_index, int wav_len)
{
	sv_node_t *node = sv_node_new(svprintc);
	sv_cluster_t *cluster;
	float score,max_score=-500.0;
	int i,id=0;
	//wtk_debug("%d %d %d\n",len,node->vec->len,svprintc->plda_scoring->ivector->len);
	memcpy(node->vec->p,vec,sizeof(float)*len);
	node->start = start_index;
	node->end = end_index;
	//print_float(vec,len);
	if(svprintc->ncluster == 0)
	{
		node->index = 0;
		cluster = svprintc->clusters[0];
		memcpy(cluster->mean->p,vec,sizeof(float)*len);
		wtk_queue_push(&(cluster->queue),&(node->node));
		cluster->cnt++;
		svprintc->ncluster = 1;
	}else
	{
		for(i = 0; i < svprintc->ncluster; i++)
		{
			cluster = svprintc->clusters[i];
			//wtk_debug("%d\n",cluster->cnt);
			//wtk_vecf_print(cluster->mean);
			//wtk_debug("=================\n");
			//wtk_vecf_print(node->vec);
			score = qtk_sv_cluster_feat_compute_likelihood(svprintc,cluster->mean,cluster->cnt,node->vec);
			if(score > max_score)
			{
				max_score = score;
				id = i;
			}
			//wtk_debug("%f\n",score);
		}

		if(max_score >= svprintc->cfg->thresh2)
		{
			if(max_score >= svprintc->cfg->thresh1 && wav_len > 40000)
			{
				cluster = svprintc->clusters[id];
				qtk_sv_cluster_update(cluster, node);
			}
		}else
		{
			if(wav_len < 40000)
			{
				id = svprintc->ncluster;
			}else if(svprintc->ncluster < svprintc->maxn)
			{
				cluster = svprintc->clusters[svprintc->ncluster];
				memcpy(cluster->mean->p,vec,sizeof(float)*len);
				wtk_queue_push(&(cluster->queue),&(node->node));
				cluster->cnt++;
				svprintc->ncluster++;
				id = svprintc->ncluster - 1;
			}
		}
	}
	if(svprintc->notify)
	{
		svprintc->notify(svprintc->user_data,id,start_index*0.01,end_index*0.01);
	}
}

//自身填充
short* qtk_sv_cluster_pad_data2(qtk_sv_cluster_t *svprintc,short *data, int len)
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
int qtk_sv_cluster_feed2(qtk_sv_cluster_t *svprintc,short *data, int len, int is_end)
{
    int ret = 0;
	wtk_vecf_t *vec = NULL;
	short *to_data = NULL;
	int u_len = 0;
	//qtk_blas_matrix_t *out_x = qtk_blas_matrix_new(svprintc->x->pool->xvector->row,svprintc->x->pool->xvector->col);
	
	//qtk_blas_matrix_zero(out_x);
	if(1){
	    wtk_queue_t *q=&svprintc->vad->output_q;
	    wtk_vframe_t *vf=NULL;
	    wtk_queue_node_t *qn;
	   	wtk_strbuf_t *tmp_buf = svprintc->speech_tmp_buf;
	    // wtk_kvad_feed(svprintc->vad,to_data,u_len, is_end);
		wtk_kvad_feed(svprintc->vad,data,len,is_end);
		//wtk_strbuf_reset(tmp_buf);
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
	    	if(tmp_buf->pos/2 > svprintc->cfg->min_len)
	    	{
				if( tmp_buf->pos/2 < 16000*QTK_MIN_SPEECH){
					to_data = qtk_sv_cluster_pad_data2(svprintc,(short*)tmp_buf->data,tmp_buf->pos >> 1);
					u_len = 16000*QTK_MIN_SPEECH;
				}else{
					to_data = (short*)wtk_malloc(sizeof(short)*(tmp_buf->pos>>1));
					memcpy(to_data,(char*)tmp_buf->data,tmp_buf->pos*sizeof(char));
					u_len = tmp_buf->pos>>1;
				}
				wtk_nnet3_xvector_compute_feed(svprintc->x,to_data,u_len,1);
				vec = wtk_nnet3_xvector_compute_normalize(svprintc->x);
				//wtk_vecf_print(vec);
				vec = qtk_sv_cluster_plda_eval(svprintc,vec->p,vec->len);
				//wtk_vecf_print(vec);
				qtk_sv_cluster_process(svprintc,vec->p,vec->len,0,0,tmp_buf->pos);
	    	}
			qtk_sv_cluster_reset3(svprintc);
	    }
	}else{
        wtk_debug("compute feed\n");
		if(len < 16000*QTK_MIN_SPEECH){	//最小是ns音频
			to_data = qtk_sv_cluster_pad_data2(svprintc,data,len);
			u_len = 16000*QTK_MIN_SPEECH;
		}else{
			to_data = malloc(sizeof(short)*len);
			memcpy(to_data,data,len*sizeof(short));
			u_len = len;
		}
		wtk_nnet3_xvector_compute_feed(svprintc->x,to_data,u_len,is_end);
		if(is_end){
			// qtk_blas_matrix_scale2(out_x,1.0f/con);
			vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
			//memcpy(out_x->m,vec->p,vec->len*sizeof(float));
			//qtk_sv_cluster_plda_eval(svprintc,out_x->m,out_x->col*out_x->row);
			//svprintc->notify(svprintc->user_data,out_x->m,out_x->row*out_x->col);
		}
        wtk_debug("compute feed end\n");
	}
	wtk_free(to_data);
	//qtk_blas_matrix_delete(out_x);
    return ret;
}

int qtk_sv_cluster_feed3(qtk_sv_cluster_t *svprintc,short *data, int len, int is_end)
{
    int ret = 0;
	wtk_vecf_t *vec = NULL;
	short *to_data = NULL;
	int u_len = 0;
	int st=0,ed=0;
	int to_short=0;
	//qtk_blas_matrix_t *out_x = qtk_blas_matrix_new(svprintc->x->pool->xvector->row,svprintc->x->pool->xvector->col);
	//qtk_blas_matrix_zero(out_x);
	if(1){
	    wtk_queue_t *q=&svprintc->vad->output_q;
	    wtk_vframe_t *vf=NULL;
	    wtk_queue_node_t *qn;
	   	wtk_strbuf_t *tmp_buf = svprintc->speech_tmp_buf;
	    // wtk_kvad_feed(svprintc->vad,to_data,u_len, is_end);
		wtk_kvad_feed(svprintc->vad,data,len,is_end);
		//wtk_strbuf_reset(tmp_buf);
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
					if(to_short==0)
					{
						st = vf->index;
					}
				}
				break;
			case 1:
				if(vf->state==wtk_vframe_sil)
				{
					svprintc->vad_state = 0;
					if(tmp_buf->pos/2 > 1600*4)
					{
						if( tmp_buf->pos/2 < 16000*QTK_MIN_SPEECH){
							to_data = qtk_sv_cluster_pad_data2(svprintc,(short*)tmp_buf->data,tmp_buf->pos >> 1);
							u_len = 16000*QTK_MIN_SPEECH;
						}else{
							to_data = (short*)wtk_malloc(sizeof(short)*(tmp_buf->pos>>1));
							//memcpy(to_data,data,len*sizeof(short));
							memcpy(to_data,(char*)tmp_buf->data,tmp_buf->pos*sizeof(char));
							u_len = tmp_buf->pos>>1;
						}
						//wtk_wavfile_open2(svprintc->log,"cl/xx");
						//wtk_wavfile_write(svprintc->log,(char *)to_data,u_len*2);
						//wtk_wavfile_close(svprintc->log);
						wtk_nnet3_xvector_compute_feed(svprintc->x,to_data,u_len,1);
						vec = wtk_nnet3_xvector_compute_normalize(svprintc->x);
						//wtk_vecf_print(vec);
						vec = qtk_sv_cluster_plda_eval(svprintc,vec->p,vec->len);
						//wtk_vecf_print(vec);
						ed = vf->index;
						qtk_sv_cluster_process(svprintc,vec->p,vec->len,st,ed,tmp_buf->pos);
						qtk_sv_cluster_reset2(svprintc);
						to_short = 0;
					}else
					{
						to_short = 1;
					}
					//svprintc->notify(svprintc->user_data,out_x->m,out_x->row*out_x->col);
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
	    	if(tmp_buf->pos > 1600*3)
	    	{
				if( tmp_buf->pos/2 < 16000*QTK_MIN_SPEECH){
					to_data = qtk_sv_cluster_pad_data2(svprintc,(short*)tmp_buf->data,tmp_buf->pos >> 1);
					u_len = 16000*QTK_MIN_SPEECH;
				}else{
					to_data = (short*)wtk_malloc(sizeof(short)*(tmp_buf->pos>>1));
					memcpy(to_data,(char*)tmp_buf->data,tmp_buf->pos*sizeof(char));
					u_len = tmp_buf->pos>>1;
				}
				// wtk_nnet3_xvector_compute_feed(svprintc->x,(short*)tmp_buf->data,tmp_buf->pos >> 1,1);
				wtk_nnet3_xvector_compute_feed(svprintc->x,to_data,u_len,1);
				vec = wtk_nnet3_xvector_compute_normalize(svprintc->x);
				//memcpy(out_x->m,vec->p,vec->len*sizeof(float));
				vec = qtk_sv_cluster_plda_eval(svprintc,vec->p,vec->len);
				ed = vf->index;
				qtk_sv_cluster_process(svprintc,vec->p,vec->len,st,ed,tmp_buf->pos);
				//svprintc->notify(svprintc->user_data,vec->p,vec->len);
	    	}
	    }
	}else{
        wtk_debug("compute feed\n");
		if(len < 16000*QTK_MIN_SPEECH){	//最小是ns音频
			to_data = qtk_sv_cluster_pad_data2(svprintc,data,len);
			u_len = 16000*QTK_MIN_SPEECH;
		}else{
			to_data = malloc(sizeof(short)*len);
			memcpy(to_data,data,len*sizeof(short));
			u_len = len;
		}
		wtk_nnet3_xvector_compute_feed(svprintc->x,to_data,u_len,is_end);
		if(is_end){
			// qtk_blas_matrix_scale2(out_x,1.0f/con);
			vec = wtk_nnet3_xvector_compute_normalize(svprintc->x);
			//memcpy(out_x->m,vec->p,vec->len*sizeof(float));
			vec = qtk_sv_cluster_plda_eval(svprintc,vec->p,vec->len);
			//svprintc->notify(svprintc->user_data,vec->p,vec->len);
		}
        wtk_debug("compute feed end\n");
	}
	wtk_free(to_data);
//end:
	//qtk_blas_matrix_delete(out_x);
    return ret;
}

int qtk_sv_cluster_feed(qtk_sv_cluster_t *svprintc,short *data, int len, int is_end)
{
    int ret = 0;

    if(svprintc->cfg->mode == 1)
    {
    	qtk_sv_cluster_feed2(svprintc, data, len, is_end);//mode1 outside vad
    }else
    {
    	qtk_sv_cluster_feed3(svprintc, data, len, is_end);//mode2 inside vad only
    }
    return ret;
}

wtk_vecf_t* qtk_sv_cluster_plda_eval(qtk_sv_cluster_t *svprintc, float *prob,int len)
{
	wtk_vecf_t vec,*out = NULL;

	vec.p = prob;
	vec.len = len;
	// vec=wtk_nnet3_xvector_compute_normalize(svprintc->x);
    out = wtk_ivector_plda_scoring_transfrom_ivector(svprintc->plda_scoring,&vec);//t2
    //print_float(v->scoring->ivector->p,v->scoring->ivector->len);
    //print_float(out->p,out->len);
    // out=wtk_ivector_plda_scoring_ivector_normalization(svprintc->plda_scoring,spk_cnt);

	//memcpy(prob,out->p,len*sizeof(float));
    wtk_nnet3_xvector_compute_normalize_reset(svprintc->x);
    // v->x->spk_cnt = 0;
	return out;
}

//大于0.65是置信
float qtk_sv_cluster_feat_compute_likelihood(qtk_sv_cluster_t *svprint,wtk_vecf_t* feat1, int cnt,wtk_vecf_t*feat2)
{
	return wtk_ivector_plda_scoring_loglikelihood(svprint->plda_scoring,feat1,cnt,feat2);
}

float qtk_sv_cluster_get_likehood(qtk_sv_cluster_t *svprint,float *data1,int len,int cnt, float *data2,int len2)
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
    ret = qtk_sv_cluster_feat_compute_likelihood(svprint,vec1,cnt,vec2);
    // wtk_debug("=====> %f\n",ret);
    wtk_vecf_delete(vec1);
    wtk_vecf_delete(vec2);
    return ret ;
}
