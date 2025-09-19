#include "wtk_cnnnet.h" 

wtk_cnnnet_in_t *wtk_cnnnet_in_new(float **feat, int in_channel, int len, int zeropad_left, int zeropad_right)
{
    wtk_cnnnet_in_t *tdi;
    int i;

    tdi=(wtk_cnnnet_in_t *)wtk_malloc(sizeof(wtk_cnnnet_in_t));
    tdi->in_channel=in_channel;
    tdi->feat_len=len+zeropad_left+zeropad_right;

    tdi->ifeat=(float **)wtk_malloc(sizeof(float *)*in_channel);
    for(i=0; i<in_channel; ++i)
    {
        tdi->ifeat[i]=(float *)wtk_malloc(sizeof(float)*tdi->feat_len);
    }
    if(feat)
    {
        for(i=0; i<in_channel; ++i)
        {
            memset(tdi->ifeat[i],0,sizeof(float)*zeropad_left);
            memcpy(tdi->ifeat[i]+zeropad_left,feat[i],sizeof(float)*len);
            memset(tdi->ifeat[i]+len+zeropad_left,0,sizeof(float)*zeropad_right);
        }
    }else
    {
        for(i=0; i<in_channel; ++i)
        {
           memset(tdi->ifeat[i],0,sizeof(float)*tdi->feat_len);
        }
    }

    return tdi;
}

void wtk_cnnnet_in_delete(wtk_cnnnet_in_t *tdi)
{
    int i;

    for(i=0; i<tdi->in_channel; ++i)
    {
        wtk_free(tdi->ifeat[i]);
    }
    wtk_free(tdi->ifeat);
    wtk_free(tdi);
}


wtk_cnnnet_t *wtk_cnnnet_new(wtk_cnnnet_layer_t *layer, int ifeat_len,int depth_idx)
{
    wtk_cnnnet_t *cnn;
    int i;

    cnn=(wtk_cnnnet_t *)wtk_malloc(sizeof(wtk_cnnnet_t));
    cnn->layer=layer;
    cnn->out_len=(ifeat_len+layer->zeropad2d[0]+layer->zeropad2d[1]-(layer->kernel_size[1]-1)*layer->dilation[1]-1)/layer->stride[1]+1; 

    cnn->out=(float **)wtk_malloc(sizeof(float*)*layer->out_channel);
    for(i=0; i<layer->out_channel; ++i)
    {
        cnn->out[i]=(float *)wtk_malloc(sizeof(float)*cnn->out_len);
    }
    wtk_queue_init(&(cnn->in_q));

    cnn->depth_idx=depth_idx;

    cnn->ths=NULL;
    cnn->notify=NULL;

    wtk_cnnnet_reset(cnn);

    return cnn;
}

void wtk_cnnnet_reset(wtk_cnnnet_t *cnn)
{
    wtk_queue_node_t *qn;
    wtk_cnnnet_in_t *tdi;

    cnn->start=0;

    while(cnn->in_q.length>0)
    {
        qn=wtk_queue_pop(&(cnn->in_q));
        tdi=(wtk_cnnnet_in_t *)data_offset2(qn,wtk_cnnnet_in_t,q_n);
        wtk_cnnnet_in_delete(tdi);
    }
}

void wtk_cnnnet_delete(wtk_cnnnet_t *cnn)
{
    wtk_queue_node_t *qn;
    wtk_cnnnet_in_t *tdi;
    int i;

    while(cnn->in_q.length>0)
    {
        qn=wtk_queue_pop(&(cnn->in_q));
        tdi=(wtk_cnnnet_in_t *)data_offset2(qn,wtk_cnnnet_in_t,q_n);
        wtk_cnnnet_in_delete(tdi);
    }
    for(i=0; i<cnn->layer->out_channel; ++i)
    {
        wtk_free(cnn->out[i]);
    }
    wtk_free(cnn->out);
    wtk_free(cnn);
}

void wtk_cnnnet_sigmoid(float *m,int len)
{
    int i;

	for(i=0;i<len;++i)
	{
		*m=1.0/(1.0+expf(-*m));
		++m;
	}
}

void wtk_cnnnet_relu(float *m,int len)
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

void wtk_cnnnet_prelu(float *m,int len, float w)
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

void wtk_cnnnet_tanh(float *f,int len)
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

void wtk_cnnnet_calc_layer(wtk_cnnnet_t *cnn)
{
    wtk_cnnnet_layer_t *layer=cnn->layer;
    float **wei=layer->weight, *wei1;
    float *bias=layer->bias;
    wtk_queue_node_t *qn;
    wtk_queue_t *in_q=&(cnn->in_q);
    wtk_cnnnet_in_t *cnnin;
    float *ifeat;
    int in_channl=layer->in_channel;
    int out_channel=layer->out_channel;
    int d1=layer->dilation[0];
    int d2=layer->dilation[1];
    int ks1=layer->kernel_size[0]*d1;
    int ks2=layer->kernel_size[1];
    int s2=layer->stride[1];
    int i,j,k,k1,n,m,idx,idx2;
    float **out=cnn->out, *out1;
    int out_len=cnn->out_len;

    for(n=0,k1=0; n<out_channel; ++n)
    {
        out1=out[n];
        memset(out1, 0, sizeof(float)*out_len);
        for(m=0; m<in_channl; ++m)
        {
            for(k=0; k<ks1; k+=d1,++k1)
            {
                qn=wtk_queue_peek(in_q, k);
                cnnin=(wtk_cnnnet_in_t *)data_offset2(qn, wtk_cnnnet_in_t, q_n);
                ifeat=cnnin->ifeat[m];

                wei1=wei[k1];
                for(j=0,idx=0; j<out_len; ++j,idx+=s2)
                {
                    for(i=0,idx2=idx; i<ks2; ++i,idx2+=d2)
                    {
                        out1[j]+=ifeat[idx2]*wei1[i];
                    }
                }
            }
        }
    }
    for(n=0; n<out_channel; ++n)
    {
        for(m=0; m<out_len; ++m)
        {
            out[n][m]+=bias[n];
        }
    }
    
    if(layer->use_sigmoid)
    {
        for(n=0; n<out_channel; ++n)
        {
            wtk_cnnnet_sigmoid(out[n],out_len);
        }
    }else if(layer->use_relu)
    {
        for(n=0; n<out_channel; ++n)
        {
            wtk_cnnnet_relu(out[n],out_len);
        }
    }else if(layer->use_prelu)
    {
        for(n=0; n<out_channel; ++n)
        {
            wtk_cnnnet_prelu(out[n],out_len,layer->prelu_w);
        }
    }else if(layer->use_tanh)
    {        
        for(n=0; n<out_channel; ++n)
        {
            wtk_cnnnet_tanh(out[n],out_len);
        }
    }
}

float ** wtk_cnnnet_update_layer(wtk_cnnnet_t *cnn,float **input, int len)
{
    wtk_cnnnet_layer_t *layer=cnn->layer;
    wtk_queue_node_t *qn;
    wtk_queue_t *in_q=&(cnn->in_q);
    wtk_cnnnet_in_t *cnnin;
    int i;

    if(cnn->start==0)
    {
        for(i=0; i<layer->zeropad2d[2]; ++i)
        {
            cnnin=wtk_cnnnet_in_new(NULL, layer->in_channel, len, layer->zeropad2d[0], layer->zeropad2d[1]);
            wtk_queue_push(in_q, &(cnnin->q_n));
        }
        cnn->start=1;
    }
    cnnin=wtk_cnnnet_in_new(input, layer->in_channel, len, layer->zeropad2d[0], layer->zeropad2d[1]);
    wtk_queue_push(in_q, &(cnnin->q_n));


    if(in_q->length==(layer->kernel_size[0]-1)*layer->dilation[0]+1)
    {
        wtk_cnnnet_calc_layer(cnn);

        for(i=0; i<layer->stride[0]; ++i)
        {
            qn=wtk_queue_pop(in_q);
            cnnin=(wtk_cnnnet_in_t *)data_offset2(qn,wtk_cnnnet_in_t,q_n);
            wtk_cnnnet_in_delete(cnnin);
        }

        return cnn->out;
    }else
    {
        return NULL;
    }
}

void wtk_cnnnet_feed(wtk_cnnnet_t *cnn, float **feat,int len,int is_end)
{
    float **out;
    int i;

    out=wtk_cnnnet_update_layer(cnn, feat, len);
    if(is_end)
    {
        if(cnn->layer->zeropad2d[3]<=0)
        {
            if(out && cnn->notify)
            {
                cnn->notify(cnn->ths, cnn->depth_idx, out, cnn->layer->out_channel, cnn->out_len, 1);
            }
        }else
        {
            if(out && cnn->notify)
            {
                cnn->notify(cnn->ths, cnn->depth_idx, out, cnn->layer->out_channel, cnn->out_len, 0);
            }
            for(i=0; i<cnn->layer->in_channel; ++i)
            {
                memset(feat[i],0,len*sizeof(float));
            }
            for(i=0; i<cnn->layer->zeropad2d[3]; ++i)
            {
                out=wtk_cnnnet_update_layer(cnn, feat, len);
                if(out && cnn->notify)
                {
                    cnn->notify(cnn->ths, cnn->depth_idx, out, cnn->layer->out_channel, cnn->out_len, i==cnn->layer->zeropad2d[3]-1?1:0);
                }
            }   
        }
        wtk_cnnnet_reset(cnn);
    }else
    {
        if(out && cnn->notify)
        {
            cnn->notify(cnn->ths, cnn->depth_idx, out, cnn->layer->out_channel, cnn->out_len, 0);
        }        
    }
    
}

void wtk_cnnnet_set_notify(wtk_cnnnet_t *cnn,void *ths,wtk_cnnnet_notify_f notify)
{
    cnn->ths=ths;
    cnn->notify=notify;
}