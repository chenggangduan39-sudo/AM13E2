#include "wtk_dnnnet.h" 

wtk_dnnnet_t *wtk_dnnnet_new(wtk_dnnnet_layer_t *layer, int depth_idx)
{
    wtk_dnnnet_t *dnn;

    dnn=(wtk_dnnnet_t *)wtk_malloc(sizeof(wtk_dnnnet_t));
    dnn->layer=layer;
    dnn->depth_idx=depth_idx;

    dnn->out=(float *)wtk_malloc(sizeof(float)*layer->output_dim);

    dnn->ths=NULL;
    dnn->notify=NULL;

    wtk_dnnnet_reset(dnn);

    return dnn;
}

void wtk_dnnnet_reset(wtk_dnnnet_t *dnn)
{

}

void wtk_dnnnet_delete(wtk_dnnnet_t *dnn)
{
    wtk_free(dnn->out);
    wtk_free(dnn);
}

void wtk_dnnnet_sigmoid(float *m,int len)
{
    int i;

	for(i=0;i<len;++i)
	{
		*m=1.0/(1.0+expf(-*m));
		++m;
	}
}

void wtk_dnnnet_relu(float *m,int len)
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

void wtk_dnnnet_prelu(float *m,int len, float w)
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

void wtk_dnnnet_tanh(float *f,int len)
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

void wtk_dnnnet_log_softmax(float *f, int len)
{
    int i;
    float tmp=0;
    for(i=0;i<len;++i){
        f[i] = expf(f[i]);
        tmp += f[i];
    }
    for(i=0;i<len;++i){
        f[i] = logf(f[i]/tmp);
    }
}
#ifdef USE_NEON
float *wtk_dnnnet_update_layer_neon(wtk_dnnnet_t *dnn,float *feat,int len)
{
    int ou;
    int i;
    wtk_dnnnet_layer_t *layer=dnn->layer;
    float *out=dnn->out;
    float **wei2=layer->weight;
    float *bias=layer->bias;

    ou=layer->output_dim;
    memset(out,0,ou*sizeof(float));
/////////////////////////////////////////////////
    float32x4_t *f1, *o1, *w1, *b1;
    float32_t *f2, *o2, *w2, *b2;
    float32x4_t t1;
    float t2;
    int l_tmp, o_tmp;

    if(bias){
        for(i=0;i<ou;++i)
        {
            f1=(float32x4_t *)feat;
            f2=(float32_t *)feat;
            w1=(float32x4_t *)wei2[i];
            w2=(float32_t *)wei2[i];
            t2=0;
            l_tmp = len;
            while (l_tmp>=4)
            {
                t1=vmulq_f32(f1[0], w1[0]);
                t2+=vgetq_lane_f32(t1, 0)+vgetq_lane_f32(t1, 1)+vgetq_lane_f32(t1, 2)+vgetq_lane_f32(t1, 3);
                ++f1;
                ++w1;
                f2+=4;
                w2+=4;
                l_tmp-=4;
            }
            while(l_tmp>0)
            {
                t2+=*f2++ * *w2++;
                --l_tmp;
            }
            out[i]+=t2;
        }

        o1=(float32x4_t *)out;
        o2=(float32_t *)out;
        b1=(float32x4_t *)bias;
        b2=(float32_t *)bias;
        o_tmp = ou;
        while(o_tmp>=4){
            vst1q_f32(o2, vaddq_f32(o1[0], b1[0]));
            ++o1;
            ++b1;
            o2+=4;
            b2+=4;
            o_tmp-=4;
        }
        while(o_tmp>0){
            *o2++ += *b2++;
            --o_tmp;
        }
    }else{
        for(i=0;i<ou;++i)
        {
            f1=(float32x4_t *)feat;
            f2=(float32_t *)feat;
            w1=(float32x4_t *)wei2[i];
            w2=(float32_t *)wei2[i];
            t2=0;
            l_tmp = len;
            while (l_tmp>=4)
            {
                t1=vmulq_f32(f1[0], w1[0]);
                t2+=vgetq_lane_f32(t1, 0)+vgetq_lane_f32(t1, 1)+vgetq_lane_f32(t1, 2)+vgetq_lane_f32(t1, 3);
                ++f1;
                ++w1;
                f2+=4;
                w2+=4;
                l_tmp-=4;
            }
            while(l_tmp>0)
            {
                t2+=*f2++ * *w2++;
                --l_tmp;
            }
            out[i]+=t2;
        }
    }

    if(layer->use_sigmoid)
    {
        wtk_dnnnet_sigmoid(out,ou);
    }else if(layer->use_relu)
    {
        wtk_dnnnet_relu(out,ou);
    }else if(layer->use_prelu)
    {
        wtk_dnnnet_prelu(out,ou,layer->prelu_w);
    }else if(layer->use_tanh)
    {
        wtk_dnnnet_tanh(out,ou);
    }else if(layer->use_log_softmax)
    {
        wtk_dnnnet_log_softmax(out,ou); 
    }

    return dnn->out;
}
#else
float *wtk_dnnnet_update_layer(wtk_dnnnet_t *dnn,float *feat,int len)
{
    int ou;
    int i,k;
    wtk_dnnnet_layer_t *layer=dnn->layer;
    float *out=dnn->out;
    float *wei;
    float **wei2=layer->weight;
    float *bias=layer->bias;

    ou=layer->output_dim;
    memset(out,0,ou*sizeof(float));

/////////////////////////////////////////////////
    int len_4;
    len_4 = (int)(len / 4) * 4;

    float tmp1, tmp2, tmp3, tmp4, tmp5;
    if(bias)
    {
        for(i=0;i<ou;++i)
        {
            wei=wei2[i];
            tmp1 = tmp2 = tmp3 = tmp4 = tmp5 = 0;
            for(k=0;k<len_4;k += 4)
            {
                tmp1+=feat[k]*wei[k];
                tmp2+=feat[k + 1]*wei[k + 1];
                tmp3+=feat[k + 2]*wei[k + 2];
                tmp4+=feat[k + 3]*wei[k + 3];
            }
            tmp5 += (tmp1 + tmp2) + (tmp3 + tmp4);
            for(k=len_4;k<len;++k){
                tmp5 += feat[k]*wei[k];
            }
            out[i]+=bias[i] + tmp5;
        }
    }else
    {
        for(i=0;i<ou;++i)
        {
            wei=wei2[i];
            tmp1 = tmp2 = tmp3 = tmp4 = tmp5 = 0;
            for(k=0;k<len_4;k += 4)
            {
                tmp1+=feat[k]*wei[k];
                tmp2+=feat[k + 1]*wei[k + 1];
                tmp3+=feat[k + 2]*wei[k + 2];
                tmp4+=feat[k + 3]*wei[k + 3];
            }
            tmp5 += (tmp1 + tmp2) + (tmp3 + tmp4);
            for(k=len_4;k<len;++k){
                tmp5 += feat[k]*wei[k];
            }
            out[i]+=tmp5;
        }
    }

/////////////////////////////////////////////////
    // if(bias)
    // {
    //     for(i=0;i<ou;++i)
    //     {
    //         wei=wei2[i];
    //         for(k=0;k<len;++k)
    //         {
    //             out[i]+=feat[k]*wei[k];
    //         }
    //         out[i]+=bias[i];
    //     }
    // }else
    // {
    //     for(i=0;i<ou;++i)
    //     {
    //         wei=wei2[i];
    //         for(k=0;k<len;++k)
    //         {
    //             out[i]+=feat[k]*wei[k];
    //         }
    //     }
    // }
/////////////////////////////////////////////////

    if(layer->use_sigmoid)
    {
        wtk_dnnnet_sigmoid(out,ou);
    }else if(layer->use_relu)
    {
        wtk_dnnnet_relu(out,ou);
    }else if(layer->use_prelu)
    {
        wtk_dnnnet_prelu(out,ou,layer->prelu_w);
    }else if(layer->use_tanh)
    {
        wtk_dnnnet_tanh(out,ou);
    }else if(layer->use_log_softmax)
    {
        wtk_dnnnet_log_softmax(out,ou); 
    }

    return dnn->out;
}
#endif

void wtk_dnnnet_feed(wtk_dnnnet_t *dnn, float *feat,int len,int is_end)
{
    float *out;

#ifdef USE_NEON
    out=wtk_dnnnet_update_layer_neon(dnn, feat, len);
#else
    out=wtk_dnnnet_update_layer(dnn, feat, len);
#endif
    if(out && dnn->notify)
    {
        dnn->notify(dnn->ths, dnn->depth_idx, out, dnn->layer->output_dim, is_end);
    }   
}

void wtk_dnnnet_set_notify(wtk_dnnnet_t *dnn,void *ths,wtk_dnnnet_notify_f notify)
{
    dnn->ths=ths;
    dnn->notify=notify;
}