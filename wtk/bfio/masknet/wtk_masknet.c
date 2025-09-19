#include "wtk_masknet.h" 
void wtk_masknet_on_cnn(wtk_masknet_t *masknet, int depth_idx, float **out, int ch, int len, int is_end);
void wtk_masknet_on_lstm(wtk_masknet_t *masknet, int depth_idx, float *out, int len, int is_end);
void wtk_masknet_on_tdnn_or_dnn(wtk_masknet_t *masknet, int depth_idx, float *out,int len,int is_end);

wtk_masknet_net_t *wtk_masknet_net_new(wtk_masknet_net_type_t type)
{
	wtk_masknet_net_t *ml;

	ml=(wtk_masknet_net_t *)wtk_malloc(sizeof(wtk_masknet_net_t));
	ml->type=type;

	ml->cnn=NULL;
	ml->lstm=NULL;
    ml->gru=NULL;
	ml->tdnn=NULL;
	ml->dnn=NULL;

	return ml;
}

void wtk_masknet_net_delete(wtk_masknet_net_t *ml)
{
	wtk_free(ml);
}

wtk_masknet_t *wtk_masknet_new(wtk_masknet_cfg_t *cfg)
{
    wtk_masknet_t *masknet;
    wtk_queue_node_t *qn;
    wtk_masknet_layer_t *layer;
    wtk_masknet_net_t *net;
    wtk_queue_t *net_q;
    int depth_idx=0;
    int ifeat_len;

    masknet=(wtk_masknet_t *)wtk_malloc(sizeof(wtk_masknet_t));
    masknet->cfg=cfg;

    masknet->ifeat_size[0]=masknet->ifeat_size[1]=1;
    ifeat_len=cfg->ifeat_len;
    net_q=&(masknet->net_q);
    wtk_queue_init(net_q);
    for(qn=cfg->layer_q.pop; qn; qn=qn->next)
    {
        layer=(wtk_masknet_layer_t *)data_offset2(qn, wtk_masknet_layer_t, q_n);
        switch(layer->type)
        {
            case WTK_CNNNET_LAYER:
                net=wtk_masknet_net_new(WTK_CNNNET_NET);
                net->cnn=wtk_cnnnet_new(layer->cnn, ifeat_len, ++depth_idx);
                masknet->ifeat_size[0]=max(masknet->ifeat_size[0], layer->cnn->out_channel);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], net->cnn->out_len);
                ifeat_len=net->cnn->out_len;
                wtk_cnnnet_set_notify(net->cnn, masknet, (wtk_cnnnet_notify_f)wtk_masknet_on_cnn);
                wtk_queue_push(net_q, &(net->q_n));
                break;
            case WTK_LSTMNET_LAYER:
                net=wtk_masknet_net_new(WTK_LSTMNET_NET);
                net->lstm=wtk_lstmnet_new(layer->lstm, ++depth_idx);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->lstm->input_dim);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->lstm->lstm_hidden);
                ifeat_len=layer->lstm->lstm_hidden;
                wtk_lstmnet_set_notify(net->lstm, masknet, (wtk_lstmnet_notify_f)wtk_masknet_on_lstm);
                wtk_queue_push(net_q, &(net->q_n));
                break;
            case WTK_GRUNET_LAYER:
                net=wtk_masknet_net_new(WTK_GRUNET_NET);
                net->gru=wtk_grunet_new(layer->gru, ++depth_idx);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->gru->input_dim);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->gru->gru_hidden);
                ifeat_len=layer->gru->gru_hidden;
                wtk_grunet_set_notify(net->gru, masknet, (wtk_grunet_notify_f)wtk_masknet_on_lstm);
                wtk_queue_push(net_q, &(net->q_n));
                break;
            case WTK_TDNNNET_LAYER:
                net=wtk_masknet_net_new(WTK_TDNNNET_NET);
                net->tdnn=wtk_tdnnnet_new(layer->tdnn, ++depth_idx);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->tdnn->input_dim);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->tdnn->output_dim);
                ifeat_len=layer->tdnn->output_dim;
                wtk_tdnnnet_set_notify(net->tdnn, masknet, (wtk_tdnnnet_notify_f)wtk_masknet_on_tdnn_or_dnn);
                wtk_queue_push(net_q, &(net->q_n));
                break;
            case WTK_DNNNET_LAYER:
                net=wtk_masknet_net_new(WTK_DNNNET_NET);
                net->dnn=wtk_dnnnet_new(layer->dnn, ++depth_idx);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->dnn->input_dim);
                masknet->ifeat_size[1]=max(masknet->ifeat_size[1], layer->dnn->output_dim);
                ifeat_len=layer->dnn->output_dim;
                wtk_dnnnet_set_notify(net->dnn, masknet, (wtk_dnnnet_notify_f)wtk_masknet_on_tdnn_or_dnn);
                wtk_queue_push(net_q, &(net->q_n));
                break;
        }
    }
    masknet->ifeat=wtk_float_new_p2(masknet->ifeat_size[0], masknet->ifeat_size[1]);

    // wtk_source_loader_t sl;

    // masknet->lstm_in=wtk_float_new_p2(1000, 516);

    // sl.hook=0;
    // sl.vf=wtk_source_load_file_v;
    // wtk_source_loader_load(&sl, masknet, (wtk_source_load_handler_t) read_lstm_in, "./lstm_in2.txt");

    masknet->notify=NULL;
    masknet->ths=NULL;
    masknet->notify2=NULL;
    wtk_masknet_reset(masknet);

    return masknet;
}

void wtk_masknet_delete(wtk_masknet_t *masknet)
{
    wtk_queue_t *net_q=&(masknet->net_q);
    wtk_queue_node_t *qn;
    wtk_masknet_net_t *net;
    
    while(net_q->length>0)
    {
        qn=wtk_queue_pop(net_q);
        net=(wtk_masknet_net_t *)data_offset2(qn, wtk_masknet_net_t, q_n);
        switch(net->type)
        {
            case WTK_CNNNET_NET:
                wtk_cnnnet_delete(net->cnn);
                break;
            case WTK_LSTMNET_NET:
                wtk_lstmnet_delete(net->lstm);
                break;
            case WTK_GRUNET_NET:
                wtk_grunet_delete(net->gru);
                break;
            case WTK_TDNNNET_NET:
                wtk_tdnnnet_delete(net->tdnn);
                break;
            case WTK_DNNNET_NET:
                wtk_dnnnet_delete(net->dnn);
                break;
        }
        wtk_masknet_net_delete(net);
    }
    wtk_float_delete_p2(masknet->ifeat, masknet->ifeat_size[0]);
    wtk_free(masknet);
}

void wtk_masknet_reset(wtk_masknet_t *masknet)
{
    wtk_queue_t *net_q=&(masknet->net_q);
    wtk_queue_node_t *qn;
    wtk_masknet_net_t *net;
    
    for(qn=net_q->pop; qn; qn=qn->next)
    {
        net=(wtk_masknet_net_t *)data_offset2(qn, wtk_masknet_net_t, q_n);
        switch(net->type)
        {
            case WTK_CNNNET_NET:
                wtk_cnnnet_reset(net->cnn);
                break;
            case WTK_LSTMNET_NET:
                wtk_lstmnet_reset(net->lstm);
                break;
            case WTK_GRUNET_NET:
                wtk_grunet_reset(net->gru);
                break;
            case WTK_TDNNNET_NET:
                wtk_tdnnnet_reset(net->tdnn);
                break;
            case WTK_DNNNET_NET:
                wtk_dnnnet_reset(net->dnn);
                break;
        }
    }
}

void wtk_masknet_on_cnn(wtk_masknet_t *masknet, int depth_idx, float **out, int ch, int len, int is_end)
{
    wtk_queue_t *net_q=&(masknet->net_q);
    wtk_queue_node_t *qn;
    wtk_masknet_net_t *net;
    float **ifeat=masknet->ifeat;
    int i;
    
    if(depth_idx == masknet->cfg->layer_depth)
    {
        if(masknet->notify)
        {
            masknet->notify(masknet->ths, out? out[0]:NULL, len, is_end);
        }
        if(masknet->notify2)
        {
            masknet->notify2(masknet->ths, out? out[0]:NULL, len, masknet->idx, is_end);
        }
        return;
    }
    if(!out){return;}

    qn=wtk_queue_peek(net_q, depth_idx);
    net=(wtk_masknet_net_t *)data_offset2(qn, wtk_masknet_net_t, q_n);
    switch(net->type)
    {
        case WTK_CNNNET_NET:
            if(ch != net->cnn->layer->in_channel)
            {
                wtk_debug("error: depth %d cnn in!=ich %d/%d\n",depth_idx+1, ch,  net->cnn->layer->in_channel);
            }
            for(i=0; i<ch; ++i)
            {
                memcpy(ifeat[i], out[i], sizeof(float)*len);
            }
            wtk_cnnnet_feed(net->cnn, ifeat, len, is_end);
            break;
        case WTK_LSTMNET_NET:
            for(i=0; i<ch; ++i)
            {
                memcpy(ifeat[0]+len*i, out[i], sizeof(float)*len);
            }
            wtk_lstmnet_feed(net->lstm, ifeat[0], len*ch, is_end);
            break;
        case WTK_GRUNET_NET:
            for(i=0; i<ch; ++i)
            {
                memcpy(ifeat[0]+len*i, out[i], sizeof(float)*len);
            }
            wtk_grunet_feed(net->gru, ifeat[0], len*ch, is_end);
            break;

        case WTK_TDNNNET_NET:
            for(i=0; i<ch; ++i)
            {
                memcpy(ifeat[0]+len*i, out[i], sizeof(float)*len);
            }
            wtk_tdnnnet_feed(net->tdnn, ifeat[0], ch*len, is_end);
            break;
        case WTK_DNNNET_NET:
            for(i=0; i<ch; ++i)
            {
                memcpy(ifeat[0]+len*i, out[i], sizeof(float)*len);
            }            
            wtk_dnnnet_feed(net->dnn, ifeat[0], ch*len, is_end);
            break;
    }
}

void wtk_masknet_on_lstm(wtk_masknet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_queue_t *net_q=&(masknet->net_q);
    wtk_queue_node_t *qn;
    wtk_masknet_net_t *net;
    float **ifeat=masknet->ifeat;
    
    if(depth_idx == masknet->cfg->layer_depth)
    {
        if(masknet->notify)
        {
            masknet->notify(masknet->ths, out, len, is_end);
        }
        if(masknet->notify2)
        {
            masknet->notify2(masknet->ths, out, len, masknet->idx, is_end);
        }
        return;
    }
    if(!out){return;}

    qn=wtk_queue_peek(net_q, depth_idx);
    net=(wtk_masknet_net_t *)data_offset2(qn, wtk_masknet_net_t, q_n);
    switch(net->type)
    {
        case WTK_CNNNET_NET:
            if(1 != net->cnn->layer->in_channel)
            {
                wtk_debug("error: depth %d cnn in!=ich 1/%d\n",depth_idx+1,  net->cnn->layer->in_channel);
            }
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_cnnnet_feed(net->cnn, ifeat, len, is_end);
            break;
        case WTK_LSTMNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_lstmnet_feed(net->lstm, ifeat[0], len, is_end);
            break;
        case WTK_GRUNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_grunet_feed(net->gru, ifeat[0], len, is_end);
            break;
        case WTK_TDNNNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_tdnnnet_feed(net->tdnn, ifeat[0], len, is_end);
            break;
        case WTK_DNNNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);            
            wtk_dnnnet_feed(net->dnn, ifeat[0], len, is_end);
            break;
    }
}

void wtk_masknet_on_tdnn_or_dnn(wtk_masknet_t *masknet, int depth_idx, float *out, int len, int is_end)
{
    wtk_queue_t *net_q=&(masknet->net_q);
    wtk_queue_node_t *qn;
    wtk_masknet_net_t *net;
    float **ifeat=masknet->ifeat;
    
    if(depth_idx == masknet->cfg->layer_depth)
    {
        if(masknet->notify)
        {
            masknet->notify(masknet->ths, out, len, is_end);
        }
        if(masknet->notify2)
        {
            masknet->notify2(masknet->ths, out, len, masknet->idx, is_end);
        }
        return;
    }
    if(!out){return;}

    qn=wtk_queue_peek(net_q, depth_idx);
    net=(wtk_masknet_net_t *)data_offset2(qn, wtk_masknet_net_t, q_n);
    switch(net->type)
    {
        case WTK_CNNNET_NET:
            if(1 != net->cnn->layer->in_channel)
            {
                wtk_debug("error: depth %d cnn in!=ich 1/%d\n",depth_idx+1,  net->cnn->layer->in_channel);
            }
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_cnnnet_feed(net->cnn, ifeat, len, is_end);
            break;
        case WTK_LSTMNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_lstmnet_feed(net->lstm, ifeat[0], len, is_end);
            break;
        case WTK_GRUNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_grunet_feed(net->gru, ifeat[0], len, is_end);
            break;
        case WTK_TDNNNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_tdnnnet_feed(net->tdnn, ifeat[0], len, is_end);
            break;
        case WTK_DNNNET_NET:
            memcpy(ifeat[0], out, sizeof(float)*len);
            wtk_dnnnet_feed(net->dnn, ifeat[0], len, is_end);
            break;
    }
}

void wtk_masknet_feed(wtk_masknet_t *masknet, float *data, int len, int is_end)
{
    wtk_queue_t *net_q=&(masknet->net_q);
    wtk_queue_node_t *qn;
    wtk_masknet_net_t *net;
    float **ifeat=masknet->ifeat;
    
    qn=wtk_queue_peek(net_q, 0);
    net=(wtk_masknet_net_t *)data_offset2(qn, wtk_masknet_net_t, q_n);
    switch(net->type)
    {
        case WTK_CNNNET_NET:
            if(1 != net->cnn->layer->in_channel)
            {
                wtk_debug("error: depth %d cnn in!=ich 1/%d\n", 1,  net->cnn->layer->in_channel);
            }
            memcpy(ifeat[0], data, sizeof(float)*len);
            wtk_cnnnet_feed(net->cnn, ifeat, len, is_end);
            break;
        case WTK_LSTMNET_NET:
            if(len!=net->lstm->layer->input_dim)
            {
                printf("ifeat len error  %d/%d\n",len,net->lstm->layer->input_dim);
                exit(0);
            }
            memcpy(ifeat[0], data, sizeof(float)*len);
            wtk_lstmnet_feed(net->lstm, ifeat[0], len, is_end);
            break;
        case WTK_GRUNET_NET:
            if(len!=net->gru->layer->input_dim)
            {
                printf("ifeat len error  %d/%d\n",len,net->gru->layer->input_dim);
                exit(0);
            }
            memcpy(ifeat[0], data, sizeof(float)*len);
            wtk_grunet_feed(net->gru, ifeat[0], len, is_end);
            break;
        case WTK_TDNNNET_NET:
            memcpy(ifeat[0], data, sizeof(float)*len);
            wtk_tdnnnet_feed(net->tdnn, ifeat[0], len, is_end);
            break;
        case WTK_DNNNET_NET:
            memcpy(ifeat[0], data, sizeof(float)*len);
            wtk_dnnnet_feed(net->dnn, ifeat[0], len, is_end);
            break;
    }
}

void wtk_masknet_set_notify(wtk_masknet_t *masknet, void *ths, wtk_masknet_notify_f notify)
{
    masknet->notify=notify;
    masknet->ths=ths;   
}

void wtk_masknet_set_notify2(wtk_masknet_t *masknet, void *ths, wtk_masknet_notify_f2 notify2)
{
    masknet->notify2=notify2;
    masknet->ths=ths;   
}