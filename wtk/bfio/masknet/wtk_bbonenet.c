#include "wtk_bbonenet.h" 

void wtk_bbonenet_on_idnn(wtk_bbonenet_t *masknet, int depth_idx, float *out,int len,int is_end);
void wtk_bbonenet_on_cnn1(wtk_bbonenet_t *masknet, int depth_idx, float *out,int och,int len,int is_end);
void wtk_bbonenet_on_cnn2(wtk_bbonenet_t *masknet, int depth_idx, float *out,int och,int len,int is_end);
void wtk_bbonenet_on_gru1(wtk_bbonenet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_bbonenet_on_gru2(wtk_bbonenet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_bbonenet_on_gru3(wtk_bbonenet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_bbonenet_on_odnn(wtk_bbonenet_t *masknet, int depth_idx, float *out,int len,int is_end);

wtk_bbonenet_t *wtk_bbonenet_new(wtk_bbonenet_cfg_t *cfg)
{
    wtk_bbonenet_t *masknet;

    masknet=(wtk_bbonenet_t *)wtk_malloc(sizeof(wtk_bbonenet_t));
    masknet->cfg=cfg;

    masknet->gru1=NULL;
    masknet->gru2=NULL;
    masknet->gru3=NULL;

    masknet->idnn=NULL;
    masknet->idnn=wtk_dnnnet_new(cfg->idnn, 0);
    wtk_dnnnet_set_notify(masknet->idnn, masknet, (wtk_dnnnet_notify_f)wtk_bbonenet_on_idnn);

    masknet->cnn1=wtk_cnn1dnet_new(cfg->cnn1, cfg->cnn1->dilation*cfg->cnn1->kernel_size, 0);
    wtk_cnn1dnet_set_notify(masknet->cnn1, masknet, (wtk_cnn1dnet_notify_f)wtk_bbonenet_on_cnn1);

    masknet->cnn2=wtk_cnn1dnet_new(cfg->cnn2, cfg->cnn2->dilation*cfg->cnn2->kernel_size, 0);
    wtk_cnn1dnet_set_notify(masknet->cnn2, masknet, (wtk_cnn1dnet_notify_f)wtk_bbonenet_on_cnn2);    

    masknet->gru1=wtk_grunet_new(cfg->gru1, 0);
    wtk_grunet_set_notify(masknet->gru1, masknet, (wtk_grunet_notify_f)wtk_bbonenet_on_gru1);
    masknet->gru2=wtk_grunet_new(cfg->gru2, 0);
    wtk_grunet_set_notify(masknet->gru2, masknet, (wtk_grunet_notify_f)wtk_bbonenet_on_gru2);
    masknet->gru3=wtk_grunet_new(cfg->gru3, 0);
    wtk_grunet_set_notify(masknet->gru3, masknet, (wtk_grunet_notify_f)wtk_bbonenet_on_gru3);
    masknet->odnn=wtk_dnnnet_new(cfg->odnn, 0);
    wtk_dnnnet_set_notify(masknet->odnn, masknet, (wtk_dnnnet_notify_f)wtk_bbonenet_on_odnn);

    masknet->input1=wtk_strbuf_new(256,1);
    masknet->input2=wtk_strbuf_new(256,1);
    masknet->neti=wtk_strbuf_new(256,1);
    masknet->neto=wtk_strbuf_new(256,1);

    masknet->notify=NULL;
    masknet->ths=NULL;
    wtk_bbonenet_reset(masknet);

    return masknet;
}

void wtk_bbonenet_delete(wtk_bbonenet_t *masknet)
{
    wtk_dnnnet_delete(masknet->idnn);
    wtk_cnn1dnet_delete(masknet->cnn1);
    wtk_cnn1dnet_delete(masknet->cnn2);
    wtk_grunet_delete(masknet->gru1);
    wtk_grunet_delete(masknet->gru2);
    wtk_grunet_delete(masknet->gru3);
    wtk_dnnnet_delete(masknet->odnn);

    wtk_strbuf_delete(masknet->input1);
    wtk_strbuf_delete(masknet->input2);
    wtk_strbuf_delete(masknet->neti);
    wtk_strbuf_delete(masknet->neto);

    wtk_free(masknet);
}

void wtk_bbonenet_reset(wtk_bbonenet_t *masknet)
{
    wtk_strbuf_reset(masknet->input1);
    wtk_strbuf_reset(masknet->input2);
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_reset(masknet->neto);

    wtk_dnnnet_reset(masknet->idnn);
    wtk_cnn1dnet_reset(masknet->cnn1);
    wtk_cnn1dnet_reset(masknet->cnn2);
    wtk_grunet_reset(masknet->gru1);
    wtk_grunet_reset(masknet->gru2);
    wtk_grunet_reset(masknet->gru3);
    wtk_dnnnet_reset(masknet->odnn);
}

void wtk_bbonenet_on_idnn(wtk_bbonenet_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    float *ifeat;
    wtk_strbuf_reset(masknet->input1);
    wtk_strbuf_push(masknet->input1, (char *)out, sizeof(float)*len);
    ifeat=(float *)(masknet->input1->data);
    //wtk_grunet_feed(masknet->gru1, out, len, is_end);
    wtk_cnn1dnet_feed(masknet->cnn1, ifeat, len, is_end);
}


void wtk_bbonenet_on_cnn1(wtk_bbonenet_t *masknet, int depth_idx, float *out,int och,int len,int is_end)
{
    //wtk_debug("bbonet_on_cnn1\n");
    //print_float(out,len);
    float *ifeat;

    wtk_strbuf_reset(masknet->input1);
    wtk_strbuf_push(masknet->input1, (char *)out, sizeof(float)*len);
    len=masknet->input1->pos/sizeof(float);
    ifeat=(float *)(masknet->input1->data);
    wtk_cnn1dnet_feed(masknet->cnn2, ifeat, len, is_end);
}

void wtk_bbonenet_on_cnn2(wtk_bbonenet_t *masknet, int depth_idx, float *out,int och,int len,int is_end)
{
    //wtk_debug("bbonet_on_cnn2\n");
    //print_float(out,len);
    wtk_strbuf_reset(masknet->input2);
    wtk_strbuf_push(masknet->input2, (char *)out, sizeof(float)*len);

    len=masknet->input2->pos/sizeof(float);
    wtk_grunet_feed(masknet->gru1, (float *)(masknet->input2->data), len, is_end);
}

void wtk_bbonenet_on_gru1(wtk_bbonenet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    //wtk_debug("bbonet_on_gru1\n");
    //print_float(out,len);
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->input2->data, masknet->input2->pos);
    wtk_strbuf_push(masknet->neti, masknet->input1->data, masknet->input1->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->gru2->layer->input_dim)
    {
        wtk_debug("gru2 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->gru2, (float *)(masknet->neti->data), len, is_end);
}

void wtk_bbonenet_on_gru2(wtk_bbonenet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    //wtk_debug("bbone on gru2\n");
    //print_float(out,len);
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input1->data, masknet->input1->pos);

    //wtk_strbuf_reset(masknet->neto);
    //wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->gru3->layer->input_dim)
    {
        wtk_debug("gru3 ifeat len error\n");
    }
    wtk_grunet_feed(masknet->gru3, (float *)(masknet->neti->data), len, is_end);
}

void wtk_bbonenet_on_gru3(wtk_bbonenet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    //wtk_debug("bbnonet on gru3\n",len);
    //print_float(out,len);
    if(len!=masknet->odnn->layer->input_dim)
    {
        wtk_debug("odnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->odnn, out, len, is_end);
}

void wtk_bbonenet_on_odnn(wtk_bbonenet_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    //wtk_debug("bbnonet on odnn\n",len);
    //print_float(out,len);
    if(masknet->notify)
    {
        masknet->notify(masknet->ths, out, len, is_end);
    }
}

void wtk_bbonenet_feed(wtk_bbonenet_t *masknet, float *data, int len, int len2, int is_end)
{
    wtk_dnnnet_feed(masknet->idnn, data, len, is_end);
}

void wtk_bbonenet_set_notify(wtk_bbonenet_t *masknet, void *ths, wtk_bbonenet_notify_f notify)
{
    masknet->notify=notify;
    masknet->ths=ths;
}