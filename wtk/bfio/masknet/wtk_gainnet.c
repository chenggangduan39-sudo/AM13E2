#include "wtk_gainnet.h" 

void wtk_gainnet_on_in_dnn(wtk_gainnet_t *masknet, int depth_idx, float *out,int len,int is_end);
void wtk_gainnet_on_vad_gru(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet_on_vad_odnn(wtk_gainnet_t *masknet, int depth_idx, float *out,int len,int is_end);

void wtk_gainnet_on_noise_gru(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet_on_denoise_gru(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet_on_denoise_odnn(wtk_gainnet_t *masknet, int depth_idx, float *out,int len,int is_end);

void wtk_gainnet_on_vad_lstm(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet_on_noise_lstm(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_gainnet_on_denoise_lstm(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end);


wtk_gainnet_t *wtk_gainnet_new(wtk_gainnet_cfg_t *cfg)
{
    wtk_gainnet_t *masknet;

    masknet=(wtk_gainnet_t *)wtk_malloc(sizeof(wtk_gainnet_t));
    masknet->cfg=cfg;

    masknet->in_dnn=wtk_dnnnet_new(cfg->in_dnn, 0);
    wtk_dnnnet_set_notify(masknet->in_dnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet_on_in_dnn);

    masknet->vad_gru=NULL;
    masknet->vad_lstm=NULL;
    masknet->noise_gru=NULL;
    masknet->noise_lstm=NULL;
    masknet->denoise_gru=NULL;
    masknet->denoise_lstm=NULL;
    if(cfg->vad_gru)
    {
        masknet->vad_gru=wtk_grunet_new(cfg->vad_gru, 0);
        wtk_grunet_set_notify(masknet->vad_gru, masknet, (wtk_grunet_notify_f)wtk_gainnet_on_vad_gru);
        masknet->noise_gru=wtk_grunet_new(cfg->noise_gru, 0);
        wtk_grunet_set_notify(masknet->noise_gru, masknet, (wtk_grunet_notify_f)wtk_gainnet_on_noise_gru);
        masknet->denoise_gru=wtk_grunet_new(cfg->denoise_gru, 0);
        wtk_grunet_set_notify(masknet->denoise_gru, masknet, (wtk_grunet_notify_f)wtk_gainnet_on_denoise_gru);

    }else if(cfg->vad_lstm)
    {
        masknet->vad_lstm=wtk_lstmnet_new(cfg->vad_lstm, 0);
        wtk_lstmnet_set_notify(masknet->vad_lstm, masknet, (wtk_lstmnet_notify_f)wtk_gainnet_on_vad_lstm);
        masknet->noise_lstm=wtk_lstmnet_new(cfg->noise_lstm, 0);
        wtk_lstmnet_set_notify(masknet->noise_lstm, masknet, (wtk_lstmnet_notify_f)wtk_gainnet_on_noise_lstm);
        masknet->denoise_lstm=wtk_lstmnet_new(cfg->denoise_lstm, 0);
        wtk_lstmnet_set_notify(masknet->denoise_lstm, masknet, (wtk_lstmnet_notify_f)wtk_gainnet_on_denoise_lstm);
    }
    // masknet->vad_odnn=wtk_dnnnet_new(cfg->vad_odnn, 0);
    // wtk_dnnnet_set_notify(masknet->vad_odnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet_on_vad_odnn);
    masknet->denoise_odnn=wtk_dnnnet_new(cfg->denoise_odnn, 0);
    wtk_dnnnet_set_notify(masknet->denoise_odnn, masknet, (wtk_dnnnet_notify_f)wtk_gainnet_on_denoise_odnn);

    masknet->input=wtk_strbuf_new(256,1);
    masknet->neti=wtk_strbuf_new(256,1);
    masknet->neto=wtk_strbuf_new(256,1);

    masknet->notify=NULL;
    masknet->ths=NULL;
    wtk_gainnet_reset(masknet);

    return masknet;
}

void wtk_gainnet_delete(wtk_gainnet_t *masknet)
{
    wtk_dnnnet_delete(masknet->in_dnn);
    // wtk_dnnnet_delete(masknet->vad_odnn);

    if(masknet->vad_gru)
    {
        wtk_grunet_delete(masknet->vad_gru);
        wtk_grunet_delete(masknet->noise_gru);
        wtk_grunet_delete(masknet->denoise_gru);
    }else if(masknet->vad_lstm)
    {
        wtk_lstmnet_delete(masknet->vad_lstm);
        wtk_lstmnet_delete(masknet->noise_lstm);
        wtk_lstmnet_delete(masknet->denoise_lstm);
    }
    wtk_dnnnet_delete(masknet->denoise_odnn);

    wtk_strbuf_delete(masknet->input);
    wtk_strbuf_delete(masknet->neti);
    wtk_strbuf_delete(masknet->neto);

    wtk_free(masknet);
}

void wtk_gainnet_reset(wtk_gainnet_t *masknet)
{
    masknet->vad=0;

    wtk_strbuf_reset(masknet->input);
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_reset(masknet->neto);

    wtk_dnnnet_reset(masknet->in_dnn);
    // wtk_dnnnet_reset(masknet->vad_odnn);
    if(masknet->vad_gru)
    {
        wtk_grunet_reset(masknet->vad_gru);
        wtk_grunet_reset(masknet->noise_gru);
        wtk_grunet_reset(masknet->denoise_gru);
    }else if(masknet->vad_lstm)
    {
        wtk_lstmnet_reset(masknet->vad_lstm);
        wtk_lstmnet_reset(masknet->noise_lstm);
        wtk_lstmnet_reset(masknet->denoise_lstm);
    }
    wtk_dnnnet_reset(masknet->denoise_odnn);
}

void wtk_gainnet_on_in_dnn(wtk_gainnet_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    if(masknet->vad_gru)
    {
        if(len!=masknet->vad_gru->layer->input_dim)
        {
            wtk_debug("vad_gru ifeat len error\n");
        }
        wtk_grunet_feed(masknet->vad_gru, out, len, is_end);
    }else if(masknet->vad_lstm)
    {
        if(len!=masknet->vad_lstm->layer->input_dim)
        {
            wtk_debug("vad_lstm ifeat len error\n");
        }
        wtk_lstmnet_feed(masknet->vad_lstm, out, len, is_end);
    }
}

void wtk_gainnet_on_vad_gru(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->noise_gru->layer->input_dim)
    {
        wtk_debug("noise_gru ifeat len error\n");
    }
    wtk_grunet_feed(masknet->noise_gru, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet_on_vad_lstm(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->noise_lstm->layer->input_dim)
    {
        wtk_debug("noise_lstm ifeat len error\n");
    }
    wtk_lstmnet_feed(masknet->noise_lstm, (float *)(masknet->neti->data), len, is_end);
}


void wtk_gainnet_on_vad_odnn(wtk_gainnet_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    masknet->vad=out[0];
}

void wtk_gainnet_on_noise_gru(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->denoise_gru->layer->input_dim)
    {
        wtk_debug("denoise_gru ifeat len error\n");
    }
    wtk_grunet_feed(masknet->denoise_gru, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet_on_denoise_gru(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    if(len!=masknet->denoise_odnn->layer->input_dim)
    {
        wtk_debug("denoise_odnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->denoise_odnn, out, len, is_end);
}

void wtk_gainnet_on_noise_lstm(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_strbuf_reset(masknet->neti);
    wtk_strbuf_push(masknet->neti, (char *)out, sizeof(float)*len);
    wtk_strbuf_push(masknet->neti, masknet->neto->data, masknet->neto->pos);
    wtk_strbuf_push(masknet->neti, masknet->input->data, masknet->input->pos);

    wtk_strbuf_reset(masknet->neto);
    wtk_strbuf_push(masknet->neto, (char *)out, sizeof(float)*len);

    len=masknet->neti->pos/sizeof(float);
    if(len!=masknet->denoise_lstm->layer->input_dim)
    {
        wtk_debug("denoise_lstm ifeat len error\n");
    }
    wtk_lstmnet_feed(masknet->denoise_lstm, (float *)(masknet->neti->data), len, is_end);
}

void wtk_gainnet_on_denoise_lstm(wtk_gainnet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    if(len!=masknet->denoise_odnn->layer->input_dim)
    {
        wtk_debug("denoise_odnn ifeat len error\n");
    }
    wtk_dnnnet_feed(masknet->denoise_odnn, out, len, is_end);
}


void wtk_gainnet_on_denoise_odnn(wtk_gainnet_t *masknet, int depth_idx, float *out,int len,int is_end)
{
    if(masknet->notify)
    {
        masknet->notify(masknet->ths, out, len, is_end);
    }
}

void wtk_gainnet_feed(wtk_gainnet_t *masknet, float *data, int len, int is_end)
{
    if(len!=masknet->in_dnn->layer->input_dim)
    {
        wtk_debug("in_dnn ifeat len error\n");
    }
    wtk_strbuf_reset(masknet->input);
    wtk_strbuf_push(masknet->input, (char *)data, sizeof(float)*len);
    wtk_dnnnet_feed(masknet->in_dnn, data, len, is_end);
}

void wtk_gainnet_feed2(wtk_gainnet_t *masknet, float *data, int len, float *data2, int len2, int is_end)
{
    if((len+len2)!=masknet->in_dnn->layer->input_dim)
    {
        wtk_debug("in_dnn ifeat len error\n");
    }
    wtk_strbuf_reset(masknet->input);
    wtk_strbuf_push(masknet->input, (char *)data, sizeof(float)*len);
    wtk_strbuf_push(masknet->input, (char *)data2, sizeof(float)*len2);
    wtk_dnnnet_feed(masknet->in_dnn, (float *)(masknet->input->data), len+len2, is_end);
}

void wtk_gainnet_set_notify(wtk_gainnet_t *masknet, void *ths, wtk_gainnet_notify_f notify)
{
    masknet->notify=notify;
    masknet->ths=ths;   
}