#include "qtk_devicetts_duration_predictor.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"
#include "tts-mer/wtk-extend/nn/wtk_nn.h"
#include "tts-mer/wtk-extend/nn/wtk_nn_activation.h"

wtk_matf_t* qtk_devicetts_duration_predictor_length_regulator(qtk_devicetts_duration_predictor_t *dur,wtk_matf_t *encoder_in,wtk_matf_t *alis);

qtk_devicetts_duration_predictor_t *qtk_devicetts_duration_predictor_new(qtk_devicetts_duration_predictor_cfg_t *cfg,wtk_heap_t *heap)
{
    qtk_devicetts_duration_predictor_t *dur = NULL;
    int layer = cfg->conv1d_layer,i  =0;
    int *args = NULL;

    dur = wtk_malloc(sizeof(qtk_devicetts_duration_predictor_t));
    dur->heap = heap;
    dur->cfg = cfg;
    
    dur->conv = wtk_malloc(sizeof(qtk_nn_conv1d_t*)*layer);
    dur->layernorm = wtk_malloc(sizeof(qtk_nn_layernorm_t*)*layer);
    args = (int*)cfg->conv1d_dim_num->slot;
    for(i = 0; i < layer; ++i){
        args = ((int*)cfg->conv1d_dim_num->slot)+i*3;
        dur->conv[i] = qtk_nn_conv1d_new(args[0],args[1],args[2],1,1);
        qtk_nn_conv1d_load_file(dur->conv[i],cfg->conv1d_kernel_fn[i],cfg->conv1d_bias_fn[i]);
        if (cfg->use_blk)
        	wtk_matf_reshape_block(dur->conv[i]->kernel, 4);
        args = ((int*)cfg->layernorm_dim_num->slot)+i;
        dur->layernorm[i] = qtk_nn_layernorm_new(args[0],1E-5,1);
        qtk_nn_layernorm_load_file(dur->layernorm[i],cfg->layernorm_gamm_fn[i],cfg->layernorm_beta_fn[i]);
    }
    args = (int*)cfg->linear_dim_num->slot;
    dur->fc = qtk_nn_fc_new(args[0],args[1],QTK_NN_ACTINATION_NULL,1,1);
    qtk_nn_fc_load_file(dur->fc,cfg->linear_weight_fn,cfg->linear_bias_fn);

    return dur;
}

wtk_matf_t* qtk_devicetts_duration_predictor_forward(qtk_devicetts_duration_predictor_t *dur,wtk_matf_t *encoder_in)
{
    int i = 0,j = 0;
    int layer = dur->cfg->conv1d_layer;
    wtk_heap_t *heap = dur->heap;
    wtk_matf_t *conv_out = NULL;
    float speed = dur->cfg->speed_factor;
    float tmp = 0.0f;
    wtk_matf_t *in = encoder_in;

    for(i = 0; i < layer; ++i){
        conv_out = wtk_matf_heap_new(heap,in->row,dur->conv[i]->out_dim);
        if (dur->cfg->use_blk)
        	qtk_nn_conv1d_forward_blk(dur->conv[i],in,conv_out);
        else
        	qtk_nn_conv1d_forward(dur->conv[i],in,conv_out);
        wtk_nn_relu(conv_out->p,conv_out->row*conv_out->col);
        qtk_nn_layernorm_forward_inplace(dur->layernorm[i],conv_out);
        in = conv_out;
    }
    wtk_matf_t *fc_out = wtk_matf_heap_new(heap,conv_out->row,dur->fc->kernel->row);
    qtk_nn_fc_forward(dur->fc,conv_out,fc_out);
    
    wtk_matf_t *alis = wtk_matf_heap_new(heap,fc_out->row,fc_out->col);
    for(i = 0; i < fc_out->row * fc_out->col; ++i){
        tmp = roundf(expf(fc_out->p[i])*speed-1);
        alis->p[i] = tmp < 0 ? 0:tmp;
    }
    wtk_matf_t *dur_LR = qtk_devicetts_duration_predictor_length_regulator(dur,encoder_in,alis);
    wtk_matf_t *LR_positions = wtk_matf_heap_new(heap,alis->col,dur_LR->row);
    float *posip = LR_positions->p;
    for(i = 0; i < alis->row;++i){
        int n = alis->p[i];
        if(n == 1 || n == 0) continue;
        tmp = 1.0f/(n-1);
        for(j = 0; j < n; ++j){
            posip[j] = tmp * j;
        }
        posip += n;
    }
    //在dur_LR 后面增加一维
    wtk_matf_t *output = wtk_matf_heap_new(heap,dur_LR->row,dur_LR->col+1);
    for(i = 0; i < dur_LR->row; ++i){
        memcpy(output->p+i*output->col,dur_LR->p+i*dur_LR->col,sizeof(float)*dur_LR->col);
        output->p[i*output->col+dur_LR->col] = LR_positions->p[i];
    }
    return output;
}

int qtk_devicetts_duration_predictor_delete(qtk_devicetts_duration_predictor_t *dur)
{
    int layer = dur->cfg->conv1d_layer, i = 0;
    for(i = 0; i < layer; ++i){
        qtk_nn_conv1d_delete(dur->conv[i]);
        qtk_nn_layernorm_delete(dur->layernorm[i]);
    }
    wtk_free(dur->conv);
    wtk_free(dur->layernorm);
    qtk_nn_fc_delete(dur->fc);
    wtk_free(dur);
    return 0;
}

wtk_matf_t* qtk_devicetts_duration_predictor_length_regulator(qtk_devicetts_duration_predictor_t *dur,wtk_matf_t *encoder_in,wtk_matf_t *alis)
{
    wtk_heap_t *heap = dur->heap;
    int dur_len = 0;
    int i = 0,j = 0;
    float dur_lenf = 0.0f;
    int e_row = encoder_in->row,e_col = encoder_in->col;
    int n = 0;
    float *ep = encoder_in->p;
    float *dp = NULL;
    float *ap = alis->p;
    int mix_resolution_factor = dur->cfg->mix_resolution_factor;

    for(i = 0; i < alis->row; ++i){
        dur_lenf += alis->p[i];
    }
    
    dur_len = dur_lenf;
    // dur->num_pad_for_mix_resolution = (mix_resolution_factor-(dur_len%mix_resolution_factor));
    // dur_len += dur->num_pad_for_mix_resolution;    //必须和mix_rf取整
    dur_len += mix_resolution_factor-(dur_len%mix_resolution_factor);
    wtk_matf_t *dur_expend = wtk_matf_heap_new(heap,dur_len,encoder_in->col);
    dp = dur_expend->p;
    for(i = 0; i < e_row; ++i){
        n = ap[i];
        for(j = 0; j < n; ++j){
            memcpy(dp,ep,sizeof(float)*e_col);
            dp += e_col;    
        }
        ep += e_col;
    }

    return dur_expend;
}
