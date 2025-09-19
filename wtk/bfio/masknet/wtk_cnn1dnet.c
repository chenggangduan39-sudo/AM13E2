#include "wtk_cnn1dnet.h" 

wtk_cnn1dnet_t *wtk_cnn1dnet_new(wtk_cnn1dnet_layer_t *layer, int ifeat_len,int depth_idx)
{
    wtk_cnn1dnet_t *cnn;

    cnn=(wtk_cnn1dnet_t *)wtk_malloc(sizeof(wtk_cnn1dnet_t));
    cnn->layer=layer;
    cnn->out_len=layer->out_channel; 

    cnn->out=(float *)wtk_malloc(sizeof(float)*layer->out_channel);
    //wtk_debug("%d %d %d\n",layer->out_channel,layer->in_channel,layer->kernel_size + 1);
    cnn->in_q = wtk_strbuf_new(sizeof(float)*(layer->kernel_size + 1) * layer->in_channel,1);

    cnn->depth_idx=depth_idx;

    cnn->ths=NULL;
    cnn->notify=NULL;

    wtk_cnn1dnet_reset(cnn);

    return cnn;
}

void wtk_cnn1dnet_reset(wtk_cnn1dnet_t *cnn)
{
    cnn->start=0;

    wtk_strbuf_reset(cnn->in_q);
    if(cnn->layer->zeropad2d > 0){
        wtk_strbuf_push_float(cnn->in_q,NULL,cnn->layer->zeropad2d[0] * cnn->layer->in_channel);
    }
}

void wtk_cnn1dnet_delete(wtk_cnn1dnet_t *cnn)
{
    wtk_strbuf_delete(cnn->in_q);
    wtk_free(cnn->out);
    wtk_free(cnn);
}

void wtk_cnn1dnet_sigmoid(float *m,int len)
{
    int i;

	for(i=0;i<len;++i)
	{
		*m=1.0/(1.0+expf(-*m));
		++m;
	}
}

void wtk_cnn1dnet_relu(float *m,int len)
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

void wtk_cnn1dnet_tanh(float *f,int len)
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

void wtk_cnn1dnet_calc_layer(wtk_cnn1dnet_t *cnn)
{
    wtk_cnn1dnet_layer_t *layer=cnn->layer;
    float **wei=layer->weight;
    float *bias=layer->bias;
    float *in=(float*)cnn->in_q->data;
    int ks=layer->kernel_size;
    float *out=cnn->out,*p,*p2;
    int out_len=cnn->out_len;
    int i,j,k;

    memset(out,0,sizeof(float)*out_len);

    for(i = 0; i < cnn->out_len; ++i){
        p = in;
        p2 = wei[i];
        *out = bias[i];
        for(j = 0;j < ks; ++j){
            for(k = 0; k < layer->in_channel; ++k){
                *out += *p2 * *p;
                p++;
                p2++;
            }

        }
        out++;
    }
    out = cnn->out;
    if(layer->use_sigmoid){
        wtk_cnn1dnet_sigmoid(out,out_len);
    }else if(layer->use_relu){
        wtk_cnn1dnet_relu(out,out_len);
    }else if(layer->use_tanh){        
        wtk_cnn1dnet_tanh(out,out_len);
    }
}

float * wtk_cnn1dnet_update_layer(wtk_cnn1dnet_t *cnn,float *input, int len)
{
    wtk_cnn1dnet_layer_t *layer=cnn->layer;
    wtk_strbuf_t *in_q=cnn->in_q;
    int len2;
    wtk_strbuf_push_float(in_q, input, len);
    len2 = in_q->pos/sizeof(float);
    if(len2 >= layer->kernel_size * len){
        wtk_cnn1dnet_calc_layer(cnn);
        wtk_strbuf_pop(in_q,NULL,sizeof(float)*len);
        return cnn->out;
    }else{
        return NULL;
    }
}

void wtk_cnn1dnet_feed(wtk_cnn1dnet_t *cnn, float *feat,int len,int is_end)
{
    float *out;
    int i;

    out=wtk_cnn1dnet_update_layer(cnn, feat, len);
    if(is_end)
    {
        if(cnn->layer->zeropad2d[1]<=0)
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
            for(i=0; i<cnn->layer->zeropad2d[1]; ++i)
            {
                wtk_strbuf_push_float(cnn->in_q, feat, len * sizeof(float));
                out=wtk_cnn1dnet_update_layer(cnn, feat, len);
                if(out && cnn->notify)
                {
                    cnn->notify(cnn->ths, cnn->depth_idx, out, cnn->layer->out_channel, cnn->out_len, i==cnn->layer->zeropad2d[1]-1?1:0);
                }
            }   
        }
        wtk_cnn1dnet_reset(cnn);
    }else
    {
        if(out && cnn->notify)
        {
            cnn->notify(cnn->ths, cnn->depth_idx, out, cnn->layer->out_channel, cnn->out_len, 0);
        }        
    }
    
}

void wtk_cnn1dnet_set_notify(wtk_cnn1dnet_t *cnn,void *ths,wtk_cnn1dnet_notify_f notify)
{
    cnn->ths=ths;
    cnn->notify=notify;
}