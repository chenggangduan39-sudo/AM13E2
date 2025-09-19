#include "wtk_gainnet4.h" 

void wtk_gainnet4_on_ns_dnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end);
void wtk_gainnet4_on_ns_gru(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet4_on_ns_gru2(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet4_on_ns_gru3(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet4_on_ns_odnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end);

void wtk_gainnet4_on_xv_dnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end);
void wtk_gainnet4_on_der_cnn(wtk_gainnet4_t *masknet, int depth_idx, float **out,int och,int len,int is_end);
void wtk_gainnet4_on_der_dnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end);
void wtk_gainnet4_on_der_gru(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet4_on_der_gru2(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet4_on_der_gru3(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet4_on_der_odnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end);


wtk_gainnet4_t *wtk_gainnet4_new(wtk_gainnet4_cfg_t *cfg)
{
    wtk_gainnet4_t *masknet;

    masknet=(wtk_gainnet4_t *)wtk_malloc(sizeof(wtk_gainnet4_t));
    masknet->cfg=cfg;

    masknet->ns_dnn=wtk_dnnnet_new(cfg->ns_dnn, 0);
    wtk_dnnnet_set_notify(masknet->ns_dnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet4_on_ns_dnn);
    masknet->ns_gru=wtk_grunet_new(cfg->ns_gru, 0);
    wtk_grunet_set_notify(masknet->ns_gru, masknet, (wtk_grunet_notify_f)wtk_gainnet4_on_ns_gru);
    masknet->ns_gru2=wtk_grunet_new(cfg->ns_gru2, 0);
    wtk_grunet_set_notify(masknet->ns_gru2, masknet, (wtk_grunet_notify_f)wtk_gainnet4_on_ns_gru2);
    masknet->ns_gru3=wtk_grunet_new(cfg->ns_gru3, 0);
    wtk_grunet_set_notify(masknet->ns_gru3, masknet, (wtk_grunet_notify_f)wtk_gainnet4_on_ns_gru3);
    masknet->ns_odnn=wtk_dnnnet_new(cfg->ns_odnn, 0);
    wtk_dnnnet_set_notify(masknet->ns_odnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet4_on_ns_odnn);

    masknet->xv_dnn=wtk_dnnnet_new(cfg->xv_dnn, 0);
    wtk_dnnnet_set_notify(masknet->xv_dnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet4_on_xv_dnn);
    masknet->der_cnn=wtk_cnnnet_new(cfg->der_cnn,cfg->der_cnn->dilation[1]*cfg->der_cnn->kernel_size[1], 0);
    wtk_cnnnet_set_notify(masknet->der_cnn, masknet, (wtk_cnnnet_notify_f)wtk_gainnet4_on_der_cnn);
    masknet->der_dnn=wtk_dnnnet_new(cfg->der_dnn, 0);
    wtk_dnnnet_set_notify(masknet->der_dnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet4_on_der_dnn);
    masknet->der_gru=wtk_grunet_new(cfg->der_gru, 0);
    wtk_grunet_set_notify(masknet->der_gru, masknet, (wtk_grunet_notify_f)wtk_gainnet4_on_der_gru);
    masknet->der_gru2=wtk_grunet_new(cfg->der_gru2, 0);
    wtk_grunet_set_notify(masknet->der_gru2, masknet, (wtk_grunet_notify_f)wtk_gainnet4_on_der_gru2);
    masknet->der_gru3=wtk_grunet_new(cfg->der_gru3, 0);
    wtk_grunet_set_notify(masknet->der_gru3, masknet, (wtk_grunet_notify_f)wtk_gainnet4_on_der_gru3);
    masknet->der_odnn=wtk_dnnnet_new(cfg->der_odnn, 0);
    wtk_dnnnet_set_notify(masknet->der_odnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet4_on_der_odnn);

    masknet->mici=wtk_strbuf_new(256,1);
    masknet->x=wtk_strbuf_new(256,1);
    masknet->x2=wtk_strbuf_new(256,1);
    masknet->neti=wtk_strbuf_new(256,1);
    masknet->neto=wtk_strbuf_new(256,1);

    masknet->notify=NULL;
    masknet->ths=NULL;
    masknet->notify2=NULL;
    masknet->ths2=NULL;
    wtk_gainnet4_reset(masknet);

    return masknet;
}

void wtk_gainnet4_delete(wtk_gainnet4_t *masknet)
{
    wtk_strbuf_delete(masknet->mici);

    wtk_strbuf_delete(masknet->x);
    wtk_strbuf_delete(masknet->x2);
    wtk_strbuf_delete(masknet->neti);
    wtk_strbuf_delete(masknet->neto);

    wtk_dnnnet_delete(masknet->ns_dnn);
    wtk_grunet_delete(masknet->ns_gru);
    wtk_grunet_delete(masknet->ns_gru2);
    wtk_grunet_delete(masknet->ns_gru3);
    wtk_dnnnet_delete(masknet->ns_odnn);

    wtk_dnnnet_delete(masknet->xv_dnn);
    wtk_cnnnet_delete(masknet->der_cnn);
    wtk_dnnnet_delete(masknet->der_dnn);
    wtk_grunet_delete(masknet->der_gru);
    wtk_grunet_delete(masknet->der_gru2);
    wtk_grunet_delete(masknet->der_gru3);
    wtk_dnnnet_delete(masknet->der_odnn);

    wtk_free(masknet);
}

void wtk_gainnet4_reset(wtk_gainnet4_t *masknet)
{
    wtk_strbuf_reset(masknet->mici);

    wtk_strbuf_reset(masknet->x);
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_reset(masknet->neto);

    wtk_dnnnet_reset(masknet->ns_dnn);
    wtk_grunet_reset(masknet->ns_gru);
    wtk_grunet_reset(masknet->ns_gru2);
    wtk_grunet_reset(masknet->ns_gru3);
    wtk_dnnnet_reset(masknet->ns_odnn);

    wtk_dnnnet_reset(masknet->xv_dnn);
    wtk_cnnnet_reset(masknet->der_cnn);
    wtk_dnnnet_reset(masknet->der_dnn);
    wtk_grunet_reset(masknet->der_gru);
    wtk_grunet_reset(masknet->der_gru2);
    wtk_grunet_reset(masknet->der_gru3);
    wtk_dnnnet_reset(masknet->der_odnn);
}

void wtk_gainnet4_on_ns_dnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);
    
    if(len!=masknet->ns_gru->layer->input_dim)
    {
        wtk_debug("ns_gru ifeat len error\n");
    }
    wtk_grunet_feed(masknet->ns_gru, out, len, is_end);
}

void wtk_gainnet4_on_ns_gru(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->mici->data, masknet->mici->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->ns_gru2->layer->input_dim)
    {
        wtk_debug("ns_gru2 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->ns_gru2, (float *)(masknet->neti->data), len, is_end);
}


void wtk_gainnet4_on_ns_gru2(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->mici->data, masknet->mici->pos);
    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->ns_gru3->layer->input_dim)
    {
        wtk_debug("ns_gru3 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->ns_gru3, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet4_on_ns_gru3(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    if(len!=masknet->ns_odnn->layer->input_dim)
    {
        wtk_debug("ns_odnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->ns_odnn, out, len, is_end);
}


void wtk_gainnet4_on_xv_dnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    wtk_strbuf_reset(masknet->x2);
    wtk_strbuf_push(masknet->x2, masknet->mici->data, masknet->mici->pos);
    wtk_strbuf_push(masknet->x2, (char *)out, sizeof(float)*len);
}

void wtk_gainnet4_on_ns_odnn(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    float *ifeat[1];

    wtk_strbuf_push(masknet->x2, (char *)out, sizeof(float)*len);

    ifeat[0]=(float *)(masknet->x2->data);
    len=masknet->x2->pos/sizeof(float);
    wtk_cnnnet_feed(masknet->der_cnn, ifeat, len, is_end);

}

void wtk_gainnet4_on_der_cnn(wtk_gainnet4_t *masknet, int depth_idx, float **out,int och,int len,int is_end)
{
    int i;

    wtk_strbuf_reset(masknet->x);
    for(i=0; i<och; ++i)
    {
        wtk_strbuf_push(masknet->x, (char *)out[i], sizeof(float)*len);
    }
    len=masknet->x->pos/sizeof(float);
    if(len!=masknet->der_dnn->layer->input_dim)
    {
        wtk_debug("der_dnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->der_dnn, (float *)(masknet->x->data), len, is_end);
}


void wtk_gainnet4_on_der_dnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);
    if(len!=masknet->der_gru->layer->input_dim)
    {
        wtk_debug("der_gru ifeat len error\n");
    }
    wtk_grunet_feed(masknet->der_gru, out, len, is_end);
}

void wtk_gainnet4_on_der_gru(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->x->data, masknet->x->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->der_gru2->layer->input_dim)
    {
        wtk_debug("der_gru2 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->der_gru2, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet4_on_der_gru2(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->x->data, masknet->x->pos);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->der_gru3->layer->input_dim)
    {
        wtk_debug("der_gru3 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->der_gru3, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet4_on_der_gru3(wtk_gainnet4_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    if(len!=masknet->der_odnn->layer->input_dim)
    {
        wtk_debug("der_odnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->der_odnn, out, len, is_end);
}

void wtk_gainnet4_on_der_odnn(wtk_gainnet4_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    if(masknet->notify)
    {
        masknet->notify(masknet->ths, out, len, is_end);
    }
}



void wtk_gainnet4_feed(wtk_gainnet4_t *masknet, float *data, int len, float *xv, int xv_len, int is_end)
{
    wtk_strbuf_reset(masknet->mici);
    wtk_strbuf_push(masknet->mici, (char *)data, sizeof(float)*len);

    if(xv_len!=masknet->xv_dnn->layer->input_dim)
    {
        wtk_debug("xv_dnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->xv_dnn, xv, xv_len, is_end);
    if(len!=masknet->ns_dnn->layer->input_dim)
    {
        wtk_debug("ns_dnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->ns_dnn, data, len, is_end);
}

void wtk_gainnet4_set_notify(wtk_gainnet4_t *masknet, void *ths, wtk_gainnet4_notify_f notify)
{
    masknet->notify=notify;
    masknet->ths=ths;   
}

void wtk_gainnet4_set_notify2(wtk_gainnet4_t *masknet, void *ths2, wtk_gainnet4_notify_f2 notify2)
{
    masknet->notify2=notify2;
    masknet->ths2=ths2;   
}