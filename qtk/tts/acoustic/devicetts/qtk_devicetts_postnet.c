#include "qtk_devicetts_postnet.h"

qtk_devicetts_postnet_t *qtk_devicetts_postnet_new(qtk_devicetts_postnet_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_devicetts_postnet_t *postnet = NULL;
    int n = cfg->convrbn,i = 0;
    int *dim = NULL;
    
    postnet = wtk_malloc(sizeof(qtk_devicetts_postnet_t));
	memset(postnet,0,sizeof(*postnet));
    postnet->cfg = cfg;
    postnet->heap = heap;
    postnet->convrb_conv1d = wtk_malloc(sizeof(qtk_nn_conv1d_t*)*n);
    postnet->convrb_batchnorm = wtk_malloc(sizeof(qtk_nn_batchnorm_t*)*n);
    postnet->convrb_residual_conv1d = wtk_malloc(sizeof(qtk_nn_conv1d_t*)*n);
    dim = (int*)cfg->convrb_dim_num->slot;
    for(i = 0; i < n; ++i){
        postnet->convrb_conv1d[i] = qtk_nn_conv1d_new(dim[0],dim[1],dim[2],dim[2]/2,1);
        qtk_nn_conv1d_load_file(postnet->convrb_conv1d[i],cfg->convrb_conv_weight_fn[i],cfg->convrb_conv_bias_fn[i]);
        if (cfg->useblk) wtk_matf_reshape_block(postnet->convrb_conv1d[i]->kernel, 4);
        postnet->convrb_batchnorm[i] = qtk_nn_batchnorm_new(dim[1],1E-5);
        qtk_nn_batchnorm_load_file(postnet->convrb_batchnorm[i],cfg->convrb_batchnorm_gamma_fn[i],cfg->convrb_batchnorm_beta_fn[i],
                                                                    cfg->convrb_batchnorm_mean_fn[i],cfg->convrb_batchnorm_var_fn[i]);
        postnet->convrb_residual_conv1d[i] = qtk_nn_conv1d_new(dim[0],dim[1],1,0,1);
        qtk_nn_conv1d_load_file(postnet->convrb_residual_conv1d[i],cfg->convrb_conv1d_residual_weight_fn[i],
                                        cfg->convrb_conv1d_residual_bias_fn[i]);
        if (cfg->useblk) wtk_matf_reshape_block(postnet->convrb_residual_conv1d[i]->kernel, 4);
        dim+=3;
    }
    dim = (int*)cfg->conv1d_dim_num->slot;
    postnet->conv1d = qtk_nn_conv1d_new(dim[0],dim[1],dim[2],0,1);
    qtk_nn_conv1d_load_file(postnet->conv1d,cfg->conv1d_weight_fn,cfg->conv1d_bias_fn);
    if (cfg->useblk) wtk_matf_reshape_block(postnet->conv1d->kernel, 4);
    return postnet;
}

int qtk_devicetts_postnet_delete(qtk_devicetts_postnet_t *postnet)
{
    int n = postnet->cfg->convrbn;
    int i = 0;
    for(i = 0; i < n; ++i){
        qtk_nn_conv1d_delete(postnet->convrb_conv1d[i]);
        qtk_nn_batchnorm_delete(postnet->convrb_batchnorm[i]);
        qtk_nn_conv1d_delete(postnet->convrb_residual_conv1d[i]);
    }
    qtk_nn_conv1d_delete(postnet->conv1d);
    wtk_free(postnet->convrb_residual_conv1d);
    wtk_free(postnet->convrb_conv1d);
    wtk_free(postnet->convrb_batchnorm);
    wtk_free(postnet);
    return 0;
}

wtk_matf_t* qtk_devicetts_postnet_process(qtk_devicetts_postnet_t *postnet,wtk_matf_t *in)
{
    int i = 0, n = postnet->cfg->convrbn;
    wtk_matf_t *loop_in=in;
    wtk_matf_t *conv_out = NULL;
    wtk_matf_t *residual_out = NULL;
    wtk_heap_t *heap = postnet->heap;
    qtk_nn_conv1d_t **convs = postnet->convrb_conv1d;

    for(i = 0; i < n;++i){
        conv_out = wtk_matf_heap_new(heap,loop_in->row,convs[i]->out_dim);
        if (postnet->cfg->useblk)
        	qtk_nn_conv1d_forward_blk(postnet->convrb_conv1d[i],loop_in,conv_out);
        else
        	qtk_nn_conv1d_forward(postnet->convrb_conv1d[i],loop_in,conv_out);
        qtk_nn_batchnorm_forward_inplace(postnet->convrb_batchnorm[i],conv_out);
        wtk_nn_relu(conv_out->p,conv_out->row*conv_out->col);
        residual_out = wtk_matf_heap_new(heap,loop_in->row,postnet->convrb_residual_conv1d[i]->out_dim);
        if (postnet->cfg->useblk)
        	qtk_nn_conv1d_forward_blk(postnet->convrb_residual_conv1d[i],loop_in,residual_out);
        else
        	qtk_nn_conv1d_forward(postnet->convrb_residual_conv1d[i],loop_in,residual_out);
        wtk_matf_add(conv_out,conv_out,residual_out);
        loop_in = conv_out;
    }
    wtk_matf_t *postnet_out = wtk_matf_heap_new(heap,conv_out->row,postnet->conv1d->out_dim);
    if (postnet->cfg->useblk)
    	qtk_nn_conv1d_forward_blk(postnet->conv1d,conv_out,postnet_out);
    else
    	qtk_nn_conv1d_forward(postnet->conv1d,conv_out,postnet_out);

    return postnet_out;
}
