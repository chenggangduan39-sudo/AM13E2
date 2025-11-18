#include "wtk_tdnnnet.h" 

wtk_tdnnnet_in_t *wtk_tdnnnet_in_new(float *feat, int len)
{
    wtk_tdnnnet_in_t *tdi;

    tdi=(wtk_tdnnnet_in_t *)wtk_malloc(sizeof(wtk_tdnnnet_in_t));
    tdi->ifeat=(float *)wtk_malloc(sizeof(float)*len);
    if(feat)
    {
        memcpy(tdi->ifeat,feat,sizeof(float)*len);
    }else
    {
        memset(tdi->ifeat,0,sizeof(float)*len);
    }

    tdi->feat_len=len;

    return tdi;
}

void wtk_tdnnnet_in_delete(wtk_tdnnnet_in_t *tdi)
{
    wtk_free(tdi->ifeat);
    wtk_free(tdi);
}

wtk_tdnnnet_t *wtk_tdnnnet_new(wtk_tdnnnet_layer_t *layer, int depth_idx)
{
    wtk_tdnnnet_t *tdnn;

    tdnn=(wtk_tdnnnet_t *)wtk_malloc(sizeof(wtk_tdnnnet_t));
    tdnn->layer=layer;

    wtk_queue_init(&(tdnn->in_q));
    tdnn->out=(float *)wtk_malloc(sizeof(float)*layer->output_dim);

    tdnn->depth_idx=depth_idx;

    tdnn->ths=NULL;
    tdnn->notify=NULL;

    wtk_tdnnnet_reset(tdnn);

    return tdnn;
}

void wtk_tdnnnet_reset(wtk_tdnnnet_t *tdnn)
{
    wtk_queue_node_t *qn;
    wtk_tdnnnet_in_t *tdi;

    while(tdnn->in_q.length>0)
    {
        qn=wtk_queue_pop(&(tdnn->in_q));
        tdi=(wtk_tdnnnet_in_t *)data_offset2(qn,wtk_tdnnnet_in_t,q_n);
        wtk_tdnnnet_in_delete(tdi);
    }
}

void wtk_tdnnnet_delete(wtk_tdnnnet_t *tdnn)
{
    wtk_queue_node_t *qn;
    wtk_tdnnnet_in_t *tdi;

    while(tdnn->in_q.length>0)
    {
        qn=wtk_queue_pop(&(tdnn->in_q));
        tdi=(wtk_tdnnnet_in_t *)data_offset2(qn,wtk_tdnnnet_in_t,q_n);
        wtk_tdnnnet_in_delete(tdi);
    }
    wtk_free(tdnn->out);
    wtk_free(tdnn);
}

void wtk_tdnnnet_sigmoid(float *m,int len)
{
    int i;

	for(i=0;i<len;++i)
	{
		*m=1.0/(1.0+expf(-*m));
		++m;
	}
}

void wtk_tdnnnet_relu(float *m,int len)
{
    int i;
    float tmp;

	for(i=0;i<len;++i)
	{
		tmp=*m;
        *m=tmp>0? tmp:0;
		++m;
	}
}

void wtk_tdnnnet_prelu(float *m,int len,float w)
{
    int i;
    float tmp;

	for(i=0;i<len;++i)
	{
		tmp=*m;
        *m=tmp>0? tmp:tmp*w;
		++m;
	}
}

void wtk_tdnnnet_tanh(float *f,int len)
{
	float *p;
	float *e;
	float inv_expx,expx;
	p=f;e=p+len;
	while(p<e)
	{
		if(*p>0.0)
		{
			inv_expx=expf(-(*p));
			*p=-1.0+2.0/(1.0+inv_expx*inv_expx);
		}else
		{
			expx=expf(*p);
			*p=1.0-2.0/(1.0+expx*expx);
		}
		++p;
	}
}



void wtk_tdnnnet_calc_layer(wtk_tdnnnet_t *tdnn, float *out)
{
    int ou;
    int i,j,k,n;
    wtk_queue_node_t *qn;
    wtk_tdnnnet_in_t *tdin;
    wtk_tdnnnet_layer_t *layer=tdnn->layer;
    wtk_queue_t *in_q=&(tdnn->in_q);
    float *feat,*wei;
    int feat_len=layer->input_dim/(layer->left_context+layer->right_context+1);
    int dilation=layer->dilation;
    int nf=dilation*(layer->left_context+layer->right_context)+1;

    ou=layer->output_dim;
    memset(out,0,ou*sizeof(float));

    for(i=0;i<ou;++i)
    {
        wei=layer->weight[i];
        for(j=0,n=0;j<nf;j+=dilation)
        {
            qn=wtk_queue_peek(in_q, j);
            tdin=(wtk_tdnnnet_in_t *)data_offset2(qn,wtk_tdnnnet_in_t,q_n);
            feat=tdin->ifeat;
            for(k=0;k<feat_len;++k,++n)
            {
                out[i]+=feat[k]*wei[n];
            }
        }
    }
    if(layer->bias)
    {
        for(i=0;i<ou;++i)
        {
            out[i]+=layer->bias[i];
        }
    }
    if(layer->use_sigmoid)
    {
        wtk_tdnnnet_sigmoid(out,ou);
    }else if(layer->use_relu)
    {
        wtk_tdnnnet_relu(out,ou);
    }else if(layer->use_prelu)
    {
        wtk_tdnnnet_prelu(out,ou,layer->prelu_w);
    }else if(layer->use_tanh)
    {
        wtk_tdnnnet_tanh(out,ou);
    }
}

float *wtk_tdnnnet_update_layer(wtk_tdnnnet_t *tdnn,float *feat,int len)
{
    wtk_queue_node_t *qn;
    int in,left_context,right_context,dilation;
    wtk_tdnnnet_layer_t *layer=tdnn->layer;
    wtk_tdnnnet_in_t *tdin;
    int i;
    wtk_queue_t *in_q=&(tdnn->in_q);

    in=layer->input_dim;
    left_context=layer->left_context;
    right_context=layer->right_context;
    dilation=layer->dilation;

    if(len != in/(left_context+right_context+1))
    {
        wtk_debug("error feat_len != input_dim\n");
        exit(0);
    }

    if(left_context>0 && in_q->length==0)
    {
        for(i=0;i<left_context*dilation;++i)
        {
            tdin=wtk_tdnnnet_in_new(NULL,len);
            wtk_queue_push(in_q,&(tdin->q_n));
        }
    }
    tdin=wtk_tdnnnet_in_new(feat, len);
    wtk_queue_push(in_q,&(tdin->q_n));
    
    if(in_q->length>=dilation*(left_context+right_context)+1)
    {
        wtk_tdnnnet_calc_layer(tdnn, tdnn->out);
        qn=wtk_queue_pop(in_q);
        tdin=(wtk_tdnnnet_in_t *)data_offset2(qn,wtk_tdnnnet_in_t,q_n);
        wtk_tdnnnet_in_delete(tdin);

        return tdnn->out;
    }else
    {
        return NULL;
    }    
}

void wtk_tdnnnet_feed(wtk_tdnnnet_t *tdnn, float *feat,int len,int is_end)
{
    float *out;
    int i;
    int end_nf;

    out=wtk_tdnnnet_update_layer(tdnn, feat, len);
    if(is_end)
    {
        end_nf=tdnn->layer->right_context*tdnn->layer->dilation;
        if(end_nf<=0)
        {
            if(out && tdnn->notify)
            {
                tdnn->notify(tdnn->ths,  tdnn->depth_idx, out, tdnn->layer->output_dim, 1);
            }
        }else
        {
            if(out && tdnn->notify)
            {
                tdnn->notify(tdnn->ths,  tdnn->depth_idx, out, tdnn->layer->output_dim, 0);
            }
            memset(feat,0,len*sizeof(float));
            for(i=0;i<end_nf;++i)
            {
                out=wtk_tdnnnet_update_layer(tdnn, feat, len);
                if(out && tdnn->notify)
                {
                    tdnn->notify(tdnn->ths,  tdnn->depth_idx, out, tdnn->layer->output_dim, i==end_nf-1?1:0);
                }
            }   
        }
        wtk_tdnnnet_reset(tdnn);
    }else
    {
        if(out && tdnn->notify)
        {
            tdnn->notify(tdnn->ths, tdnn->depth_idx, out, tdnn->layer->output_dim, 0);
        }
    }
    
}

void wtk_tdnnnet_set_notify(wtk_tdnnnet_t *tdnn,void *ths,wtk_tdnnnet_notify_f notify)
{
    tdnn->ths=ths;
    tdnn->notify=notify;
}