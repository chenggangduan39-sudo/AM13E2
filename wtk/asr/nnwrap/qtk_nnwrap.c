#include "qtk_nnwrap.h"
void qtk_nnwrap_kxparm_notity(qtk_nnwrap_t *kws,wtk_kfeat_t *feat);
void qtk_nnwrape_encoder_flush_notify(qtk_nnwrap_t *kws,qtk_blas_matrix_t *feat);
void qtk_nnwrap_notify_end();

int qtk_nnwrap_pool_read(qtk_nnwrap_t *kws,wtk_source_t *src)
{
    int ret = 0;
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);

    kws->pool = wtk_nnet3_xvector_pool_read2(src,buf,0);
    wtk_strbuf_delete(buf);
    return ret;
}

int nnwrap_col = 0;
int qtk_nnwrap_mdl_read(qtk_torchnn_t **kws,wtk_source_t *src)
{
    int ret = 0;
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);

    //*kws = qtk_torchnn_read(src,buf,0,nnwrap_col,1);
    *kws = qtk_torchnn_read_bin(src,buf,1,nnwrap_col,1);

    wtk_strbuf_delete(buf);
    return ret;
}

qtk_nnwrap_t* qtk_nnwrap_new(qtk_nnwrap_cfg_t *cfg)
{
	qtk_nnwrap_t *kws = (qtk_nnwrap_t*)wtk_malloc(sizeof(qtk_nnwrap_t));

    kws->cfg = cfg;

    kws->parm = wtk_kxparm_new(&cfg->kxparm);
    wtk_kxparm_set_notify(kws->parm,kws,(wtk_kxparm_notify_f)qtk_nnwrap_kxparm_notity);

    kws->vad = NULL;
    kws->vad_state = 0;

	wtk_source_loader_load(&cfg->sl, kws,(wtk_source_load_handler_t)qtk_nnwrap_pool_read, cfg->pool_fn);

	nnwrap_col = kws->parm->parm->melbank->cfg->num_bins;
	wtk_source_loader_load(&cfg->sl, &(kws->encoder),(wtk_source_load_handler_t)qtk_nnwrap_mdl_read, cfg->encoder_fn);
	nnwrap_col = kws->pool->xvector->col;
	qtk_torchnn_set_notify(kws->encoder,kws,(qtk_torchnn_notify_f)qtk_nnwrape_encoder_flush_notify);
	qtk_torchnn_set_notify_end(kws->encoder,(qtk_torchnn_notify_end_f)qtk_nnwrap_notify_end);

	wtk_source_loader_load(&cfg->sl, &(kws->decoder_gender),(wtk_source_load_handler_t)qtk_nnwrap_mdl_read, cfg->gender_fn);
	wtk_source_loader_load(&cfg->sl, &(kws->decoder_age),(wtk_source_load_handler_t)qtk_nnwrap_mdl_read, cfg->age_fn);

	if(cfg->use_vad)
	{
		kws->vad = wtk_kvad_new(&cfg->vad);
	}
	kws->in_buf = wtk_strbuf_new(1024,1);
	kws->xvec_in = wtk_calloc(1,sizeof(qtk_blas_matrix_t));
	kws->age = 0;
	kws->gender = 0;
	return kws;
}

void qtk_nnwrap_delete(qtk_nnwrap_t *kws)
{
	if(kws->parm)
	{
		wtk_kxparm_delete(kws->parm);
	}

    if(kws->vad)
    {
    	wtk_kvad_delete(kws->vad);
    }
    qtk_torchnn_delete(kws->encoder);
    qtk_torchnn_delete(kws->decoder_age);
    qtk_torchnn_delete(kws->decoder_gender);
    wtk_strbuf_delete(kws->in_buf);
    wtk_nnet3_xvector_pool_delete(kws->pool);

    wtk_free(kws->xvec_in);
    wtk_free(kws);
}

int qqtk_nnwrap_start(qtk_nnwrap_t *kws)
{

	if(kws->vad)
	{
		wtk_kvad_start(kws->vad);
	}
	wtk_kxparm_start(kws->parm);

	return 0;
}

int qtk_nnwrap_reset2(qtk_nnwrap_t *kws)
{
	wtk_strbuf_reset(kws->in_buf);
    if(kws->pool)
    {
    	qtk_blas_matrix_zero(kws->pool->mu);
    	qtk_blas_matrix_zero(kws->pool->rh);
    	qtk_blas_matrix_zero(kws->pool->output);
    	qtk_blas_matrix_zero(kws->pool->xvector);
    }
    qtk_torchnn_reset(kws->encoder);
    qtk_torchnn_reset(kws->decoder_age);
    qtk_torchnn_reset(kws->decoder_gender);

	//kws->age = 0;
	//kws->gender = 0;
	return 0;
}

int qtk_nnwrap_reset(qtk_nnwrap_t *kws)
{
	if(kws->parm)
	{
		wtk_kxparm_reset(kws->parm);
	}
    kws->vad_state = 0;
	if(kws->vad)
	{
		wtk_kvad_reset(kws->vad);
	}

	wtk_strbuf_reset(kws->in_buf);
    if(kws->pool)
    {
    	qtk_blas_matrix_zero(kws->pool->mu);
    	qtk_blas_matrix_zero(kws->pool->rh);
    	qtk_blas_matrix_zero(kws->pool->output);
    	qtk_blas_matrix_zero(kws->pool->xvector);
    }
	kws->age = 0;
	kws->gender = 0;
	return 0;
}

int qtk_nnwrap_start(qtk_nnwrap_t *kws)
{
	wtk_kxparm_start(kws->parm);

	return 0;
}

void qtk_nnwrap_kxparm_notity(qtk_nnwrap_t *kws,wtk_kfeat_t *feat)
{
	//qtk_blas_matrix_t *output;
	//wtk_string_t *name;
	//memcpy(feat->v,ppen+p_idx*30,sizeof(float)*30);
	//p_idx++;
	qtk_torchnn_feed(kws->encoder,feat->v,30,1,0);
}

int nnidx = 0;
void qtk_nnwrape_encoder_flush_notify(qtk_nnwrap_t *kws,qtk_blas_matrix_t *m)
{	
	//wtk_debug("%d\n",nnidx++);
	//qtk_blas_matrix_print(m);
    wtk_strbuf_push(kws->in_buf,(char*)m->m,sizeof(float)*m->col*m->row);
}

int qtk_nnwrap_get_max(float *a,int n)
{
        int i,j;
        float f;

        f=a[0];
        j=0;
        for(i=1;i<n;++i)
        {
                if(a[i]>f)
                {
                        f=a[i];
                        j=i;
                }
        }
        return j;
}


void qtk_nnwrap_notify_end(qtk_nnwrap_t *x)
{
    x->xvec_in->m = (float*)x->in_buf->data;
    x->xvec_in->col = x->pool->asp->bias->col;
    x->xvec_in->row = x->in_buf->pos/(x->xvec_in->col*sizeof(float));
	wtk_nnet3_xvector_compute_pool2(x->pool,x->xvec_in);

	//qtk_blas_matrix_print(x->xvec_in);
	//wtk_debug("%d %d\n",x->pool->xvector->row,x->pool->xvector->col);
	qtk_blas_matrix_t *age = qtk_torchnn_feed(x->decoder_age,x->pool->xvector->m,512,1,0);
	//wtk_debug("feed gender nn\n");
	x->age = qtk_nnwrap_get_max(age->m,age->row*age->col);
	qtk_blas_matrix_t *gender = qtk_torchnn_feed(x->decoder_gender,x->pool->xvector->m,512,1,0);
	//qtk_blas_matrix_print(gender);
	x->gender = qtk_nnwrap_get_max(gender->m,gender->row*gender->col);
	//wtk_debug("%d %d\n",x->age,x->gender);
}

int qtk_nnwrap_feed(qtk_nnwrap_t *kws,char *data,int bytes,int is_end)
{
	wtk_queue_t *q = &(kws->vad->output_q);
	wtk_vframe_t *vf = NULL;
	wtk_queue_node_t *qn;

	if(kws->vad)
	{
		wtk_kvad_feed(kws->vad,(short*)data,bytes>>1,is_end);
		//wtk_debug("%d\n",kws->vad->output_q.length);
		while(1)
		{
			qn = wtk_queue_pop(q);
			if(!qn){break;}
			vf = data_offset2(qn,wtk_vframe_t,q_n);
			//wtk_debug("----------vf state-------- %d\n",vf->state);
			switch(kws->vad_state)
			{
			case 0:
				if(vf->state != wtk_vframe_sil)
				{
					if(kws->parm)
					{
						wtk_kxparm_feed(kws->parm,vf->wav_data,vf->frame_step,0);
					}
					kws->vad_state = 1;
				}
				break;
			case 1:
				if(vf->state == wtk_vframe_sil)
				{
					qtk_torchnn_flush(kws->encoder);
					qtk_nnwrap_reset2(kws);
					kws->vad_state = 0;
				}else
				{
					if(kws->parm)
					{
						wtk_kxparm_feed(kws->parm,vf->wav_data,vf->frame_step,0);
					}
				}
				break;

			default:
				break;
			}
		}

		if(is_end && kws->parm)
		{
			wtk_kxparm_feed(kws->parm,NULL,0,is_end);
		}

	}else
	{
		if(kws->parm)
		{
			wtk_kxparm_feed(kws->parm,(short*)data,bytes>>1,is_end);
			if(is_end)
			{
				qtk_torchnn_flush(kws->encoder);
			}
		}
	}

	return 0;
}

void qtk_nnwrap_get_result(qtk_nnwrap_t *nw, int *age, int *gender) {
    *age = nw->age;
    *gender = nw->gender;
}
