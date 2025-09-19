#include "wtk_gainnet2.h" 

void wtk_gainnet2_on_icnn(wtk_gainnet2_t *masknet, int depth_idx, float **out,int och,int len,int is_end);

void wtk_gainnet2_on_sdnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end);
void wtk_gainnet2_on_dnn1(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end);


void wtk_gainnet2_on_gru1(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end);

void wtk_gainnet2_on_gru2(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet2_on_gru3(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet2_on_odnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end);

void wtk_gainnet2_on_agc_dnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end);
void wtk_gainnet2_on_agc_gru(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet2_on_agc_gru2(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet2_on_agc_gru3(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet2_on_agc_odnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end);


wtk_gainnet2_t *wtk_gainnet2_new(wtk_gainnet2_cfg_t *cfg)
{
    wtk_gainnet2_t *masknet;

    masknet=(wtk_gainnet2_t *)wtk_malloc(sizeof(wtk_gainnet2_t));
    masknet->cfg=cfg;

    masknet->gru1=NULL;
    masknet->gru2=NULL;
    masknet->gru3=NULL;

    masknet->icnn=wtk_cnnnet_new(cfg->icnn, cfg->icnn->dilation[1]*cfg->icnn->kernel_size[1], 0);
    wtk_cnnnet_set_notify(masknet->icnn, masknet, (wtk_cnnnet_notify_f)wtk_gainnet2_on_icnn);

    masknet->sdnn=NULL;
    if(cfg->use_sdnn)
    {
        masknet->sdnn=wtk_dnnnet_new(cfg->sdnn, 0);
        wtk_dnnnet_set_notify(masknet->sdnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet2_on_sdnn);
    }
    masknet->dnn1=wtk_dnnnet_new(cfg->dnn1, 0);
    wtk_dnnnet_set_notify(masknet->dnn1, masknet, (wtk_dnnnet_notify_f)wtk_gainnet2_on_dnn1);

    masknet->gru1=wtk_grunet_new(cfg->gru1, 0);
    wtk_grunet_set_notify(masknet->gru1, masknet, (wtk_grunet_notify_f)wtk_gainnet2_on_gru1);
    masknet->gru2=wtk_grunet_new(cfg->gru2, 0);
    wtk_grunet_set_notify(masknet->gru2, masknet, (wtk_grunet_notify_f)wtk_gainnet2_on_gru2);
    masknet->gru3=wtk_grunet_new(cfg->gru3, 0);
    wtk_grunet_set_notify(masknet->gru3, masknet, (wtk_grunet_notify_f)wtk_gainnet2_on_gru3);
    masknet->odnn=wtk_dnnnet_new(cfg->odnn, 0);
    wtk_dnnnet_set_notify(masknet->odnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet2_on_odnn);

    masknet->agc_dnn=NULL;
    masknet->agc_gru=masknet->agc_gru2=masknet->agc_gru3=NULL;
    masknet->agc_odnn=NULL;
    masknet->mici=NULL;
    if(cfg->use_agc)
    {
        masknet->agc_dnn=wtk_dnnnet_new(cfg->agc_dnn, 0);
        wtk_dnnnet_set_notify(masknet->agc_dnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet2_on_agc_dnn);
        masknet->agc_gru=wtk_grunet_new(cfg->agc_gru, 0);
        wtk_grunet_set_notify(masknet->agc_gru, masknet, (wtk_grunet_notify_f)wtk_gainnet2_on_agc_gru);
        masknet->agc_gru2=wtk_grunet_new(cfg->agc_gru2, 0);
        wtk_grunet_set_notify(masknet->agc_gru2, masknet, (wtk_grunet_notify_f)wtk_gainnet2_on_agc_gru2);
        masknet->agc_gru3=wtk_grunet_new(cfg->agc_gru3, 0);
        wtk_grunet_set_notify(masknet->agc_gru3, masknet, (wtk_grunet_notify_f)wtk_gainnet2_on_agc_gru3);
        masknet->agc_odnn=wtk_dnnnet_new(cfg->agc_odnn, 0);
        wtk_dnnnet_set_notify(masknet->agc_odnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet2_on_agc_odnn);

        masknet->mici=wtk_strbuf_new(256,1);
    }

    masknet->input=wtk_strbuf_new(256,1);
    masknet->neti=wtk_strbuf_new(256,1);
    masknet->neto=wtk_strbuf_new(256,1);

    masknet->notify=NULL;
    masknet->ths=NULL;
    masknet->notify2=NULL;
    masknet->ths2=NULL;
    wtk_gainnet2_reset(masknet);

    return masknet;
}

void wtk_gainnet2_delete(wtk_gainnet2_t *masknet)
{
    wtk_cnnnet_delete(masknet->icnn);
    if(masknet->sdnn)
    {
        wtk_dnnnet_delete(masknet->sdnn);
    }
    wtk_dnnnet_delete(masknet->dnn1);
    wtk_grunet_delete(masknet->gru1);
    wtk_grunet_delete(masknet->gru2);
    wtk_grunet_delete(masknet->gru3);
    wtk_dnnnet_delete(masknet->odnn);

    wtk_strbuf_delete(masknet->input);
    wtk_strbuf_delete(masknet->neti);
    wtk_strbuf_delete(masknet->neto);

    if(masknet->agc_dnn)
    {
        wtk_strbuf_delete(masknet->mici);
        wtk_dnnnet_delete(masknet->agc_dnn);
        wtk_grunet_delete(masknet->agc_gru);
        wtk_grunet_delete(masknet->agc_gru2);
        wtk_grunet_delete(masknet->agc_gru3);
        wtk_dnnnet_delete(masknet->agc_odnn);
    }

    wtk_free(masknet);
}

void wtk_gainnet2_reset(wtk_gainnet2_t *masknet)
{
    wtk_strbuf_reset(masknet->input);
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_reset(masknet->neto);

    wtk_cnnnet_reset(masknet->icnn);
    if(masknet->sdnn)
    {
        wtk_dnnnet_reset(masknet->sdnn);
    }
    wtk_dnnnet_reset(masknet->dnn1);
    wtk_grunet_reset(masknet->gru1);
    wtk_grunet_reset(masknet->gru2);
    wtk_grunet_reset(masknet->gru3);
    wtk_dnnnet_reset(masknet->odnn);

    if(masknet->agc_dnn)
    {
        wtk_strbuf_reset(masknet->mici);
        wtk_dnnnet_reset(masknet->agc_dnn);
        wtk_grunet_reset(masknet->agc_gru);
        wtk_grunet_reset(masknet->agc_gru2);
        wtk_grunet_reset(masknet->agc_gru3);
        wtk_dnnnet_reset(masknet->agc_odnn);
    }
}


void wtk_gainnet2_on_icnn(wtk_gainnet2_t *masknet, int depth_idx, float **out,int och,int len,int is_end)
{
    int i;

    wtk_strbuf_reset(masknet->input);
    for(i=0; i<och; ++i)
    {
        wtk_strbuf_push(masknet->input, (char *)out[i], sizeof(float)*len);
    }

    len=masknet->input->pos/sizeof(float);
    if(masknet->sdnn)
    {
        if(len!=masknet->sdnn->layer->input_dim)
        {
            wtk_debug("sdnn ifeat len error\n");
        }
        wtk_dnnnet_feed(masknet->sdnn, (float *)(masknet->input->data), len, is_end);
    }else
    {
        if(len!=masknet->dnn1->layer->input_dim)
        {
            wtk_debug("dnn1 ifeat len error\n");
        }
        wtk_dnnnet_feed(masknet->dnn1, (float *)(masknet->input->data), len, is_end);
    }
}

void wtk_gainnet2_on_sdnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    wtk_strbuf_reset(masknet->input);
    wtk_strbuf_push(masknet->input, (char *)out, sizeof(float)*len);

    len=masknet->input->pos/sizeof(float);
    if(len!=masknet->dnn1->layer->input_dim)
    {
        wtk_debug("dnn1 ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->dnn1, (float *)(masknet->input->data), len, is_end);
}


void wtk_gainnet2_on_dnn1(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    if(len!=masknet->gru1->layer->input_dim)
    {
        wtk_debug("gru1 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->gru1, out, len, is_end);
}

void wtk_gainnet2_on_gru1(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->gru2->layer->input_dim)
    {
        wtk_debug("gru2 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->gru2, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet2_on_gru2(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->gru3->layer->input_dim)
    {
        wtk_debug("gru3 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->gru3, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet2_on_gru3(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    if(len!=masknet->odnn->layer->input_dim)
    {
        wtk_debug("odnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->odnn, out, len, is_end);
}


void wtk_gainnet2_on_odnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    if(masknet->notify)
    {
        masknet->notify(masknet->ths, out, len, is_end);
    }

    if(masknet->agc_dnn)
    {
        wtk_strbuf_reset(masknet->input);
        wtk_strbuf_push(masknet->input, masknet->mici->data, masknet->mici->pos);
        wtk_strbuf_push(masknet->input, (char *)out, sizeof(float)*len);
        len=masknet->input->pos/sizeof(float);
        if(len!=masknet->agc_dnn->layer->input_dim)
        {
            wtk_debug("agc_dnn ifeat len error\n");
        }
        wtk_dnnnet_feed(masknet->agc_dnn, (float *)(masknet->input->data), len, is_end);
    }
}


void wtk_gainnet2_on_agc_dnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);
    
    if(len!=masknet->agc_gru->layer->input_dim)
    {
        wtk_debug("agc_gru ifeat len error\n");
    }
    wtk_grunet_feed(masknet->agc_gru, out, len, is_end);
}

void wtk_gainnet2_on_agc_gru(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);
    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->agc_gru2->layer->input_dim)
    {
        wtk_debug("agc_gru2 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->agc_gru2, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet2_on_agc_gru2(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->agc_gru3->layer->input_dim)
    {
        wtk_debug("agc_gru3 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->agc_gru3, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet2_on_agc_gru3(wtk_gainnet2_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    if(len!=masknet->agc_odnn->layer->input_dim)
    {
        wtk_debug("agc_odnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->agc_odnn, out, len, is_end);
}

void wtk_gainnet2_on_agc_odnn(wtk_gainnet2_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    if(masknet->notify2)
    {
        masknet->notify2(masknet->ths2, out, len, is_end);
    }
}

void wtk_gainnet2_feed(wtk_gainnet2_t *masknet, float *data, int len, int len2, int is_end)
{
    float *ifeat[1];

    if(masknet->mici)
    {
        wtk_strbuf_reset(masknet->mici);
        wtk_strbuf_push(masknet->mici, (char *)data, sizeof(float)*len);
    }

    ifeat[0]=data;
    wtk_cnnnet_feed(masknet->icnn, ifeat, len2, is_end);
}

void wtk_gainnet2_feed2(wtk_gainnet2_t *masknet, float *data, int len, float *data2, int len2, int is_end)
{
    float *ifeat[1];

    if(masknet->mici)
    {
        wtk_strbuf_reset(masknet->mici);
        wtk_strbuf_push(masknet->mici, (char *)data, sizeof(float)*len);
    }

    wtk_strbuf_reset(masknet->input);
    if(len>0)
    {
        wtk_strbuf_push(masknet->input, (char *)data, sizeof(float)*len);
    }
    if(len2>0)
    {
        wtk_strbuf_push(masknet->input, (char *)data2, sizeof(float)*len2);
    }
    ifeat[0]=(float *)(masknet->input->data);
    wtk_cnnnet_feed(masknet->icnn, ifeat, len+len2, is_end);
}

void wtk_gainnet2_feed3(wtk_gainnet2_t *masknet, float *data, int len, float *data2, int len2, float *data3, int len3, float *data4, int len4, int is_end)
{
    float *ifeat[1];

    if(masknet->mici)
    {
        wtk_strbuf_reset(masknet->mici);
        wtk_strbuf_push(masknet->mici, (char *)data, sizeof(float)*len);
    }

    wtk_strbuf_reset(masknet->input);
    wtk_strbuf_push(masknet->input, (char *)data, sizeof(float)*len);
    wtk_strbuf_push(masknet->input, (char *)data2, sizeof(float)*len2);
    wtk_strbuf_push(masknet->input, (char *)data3, sizeof(float)*len3);
    wtk_strbuf_push(masknet->input, (char *)data4, sizeof(float)*len4);
    ifeat[0]=(float *)(masknet->input->data);
    wtk_cnnnet_feed(masknet->icnn, ifeat, len+len2+len3+len4, is_end);
}

void wtk_gainnet2_set_notify(wtk_gainnet2_t *masknet, void *ths, wtk_gainnet2_notify_f notify)
{
    masknet->notify=notify;
    masknet->ths=ths;   
}

void wtk_gainnet2_set_notify2(wtk_gainnet2_t *masknet, void *ths, wtk_gainnet2_notify_f2 notify)
{
    masknet->notify2=notify;
    masknet->ths2=ths;   
}
