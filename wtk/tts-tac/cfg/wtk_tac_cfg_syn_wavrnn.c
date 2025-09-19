#include "wtk_tac_cfg_syn_wavrnn.h"

void wtk_tac_cfg_syn_wavrnn_init(wtk_tac_cfg_syn_wavrnn_t *wavrnn)
{
    wavrnn->heap=NULL;
}
void wtk_tac_cfg_syn_wavrnn_update_local(wtk_tac_cfg_syn_wavrnn_t *wavrnn, wtk_local_cfg_t *wavrnn_lc, wtk_tac_hparams_t *hp)
{
    wtk_heap_t *heap=wtk_heap_new(4096);
    wtk_string_t *v;
    char fn[512];
    int num_mel=hp->num_mels
      , res_compute_dim=hp->wavrnn_res_compute_dim
      , res_in_kernel_size=hp->wavrnn_res_in_kernel_size
      , res_out_kernel_size=hp->wavrnn_res_out_kernel_size
      , kernel_block_size=hp->wavrnn_kernel_block_size
      , res_out_dim=hp->wavrnn_res_out_dim
      , num_res_block=hp->wavrnn_num_res_block
      , num_res_block2=num_res_block*2
      , *upsample_scale_iarr
      , upsample_scale_len
      , is_model_small=hp->wavrnn_is_model_small
      , is_model_multiband=hp->wavrnn_is_model_multiband
      , aux_dim=hp->wavrnn_aux_dim
      , rnn_dim=hp->wavrnn_rnn_dim
      , gru_layers=hp->wavrnn_gru_layers
      , fc_dim=hp->wavrnn_fc_dim
      , fc_num=hp->wavrnn_num_fc
      , fc_band_dim=hp->wavrnn_fc_band_dim
      , fc_band_num=hp->wavrnn_fc_band_num
      , bit=hp->wavrnn_bit
      , nfft=1024
      , hop_len=256
      , win_len=800
      , i;
    wtk_rfft_t *rf=wtk_rfft_new(nfft/2);
    wtk_nn_stft_t *stft=wtk_nn_stft_heap_new( heap, rf, nfft, hop_len, win_len);
    wtk_matf_t
        *kernel_res_in=wtk_matf_heap_new( heap, res_compute_dim, num_mel*res_in_kernel_size),
        *kernel_res_out=wtk_matf_heap_new( heap, res_out_dim, res_compute_dim*res_out_kernel_size),
        **kernel_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2),
        **up_layer_kernel,
        *linear_kernel=wtk_matf_heap_new( heap, rnn_dim,(is_model_multiband?3:(is_model_small?0:1))+num_mel+aux_dim),
        **fc_kernel=wtk_heap_malloc( heap, sizeof(void*)*fc_num),
        **fc_band_kernel=wtk_heap_malloc( heap, sizeof(void*)*fc_band_num),
        *pqmf_spec_filter_real=wtk_matf_heap_new( heap, 4, stft->nfreq),
        *pqmf_spec_filter_imag=wtk_matf_heap_new( heap, 4, stft->nfreq);
    wtk_vecf_t
        *gamma_res=wtk_vecf_heap_new( heap, res_compute_dim),
        *beta_res=wtk_vecf_heap_new( heap, res_compute_dim),
        *mean_res=wtk_vecf_heap_new( heap, res_compute_dim),
        *var_res=wtk_vecf_heap_new( heap, res_compute_dim),
        *bias_res_out=wtk_vecf_heap_new( heap, res_compute_dim),
        **gamma_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2),
        **beta_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2),
        **mean_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2),
        **var_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2),
        *linear_bias=wtk_vecf_heap_new( heap, rnn_dim),
        **fc_bias=wtk_heap_malloc( heap, sizeof(void*)*fc_num),
        **fc_band_bias=wtk_heap_malloc( heap, sizeof(void*)*fc_band_num);
    wtk_array_t *upsample_scale;
    wtk_nn_rnngru_t **gru=wtk_heap_malloc( heap, sizeof(void*)*gru_layers);

    for (i=0; i<gru_layers; ++i)
    {
        if (i==0)
        {
            gru[0]=wtk_nn_rnngru_new2(wtk_nn_enum_type_pytorch, 1, rnn_dim, rnn_dim, NULL);
        } else if (i==1)
        {
            gru[1]=wtk_nn_rnngru_new2(wtk_nn_enum_type_pytorch, 1, rnn_dim+aux_dim, rnn_dim, NULL);
        }
    }

    wavrnn->fn_kernel_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2);
    wavrnn->fn_gamma_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2);
    wavrnn->fn_beta_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2);
    wavrnn->fn_mean_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2);
    wavrnn->fn_var_bk=wtk_heap_malloc( heap, sizeof(void*)*num_res_block2);
    wavrnn->fn_fc_kernel=wtk_heap_malloc( heap, sizeof(void*)*fc_num);
    wavrnn->fn_fc_bias=wtk_heap_malloc( heap, sizeof(void*)*fc_num);

    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_kernel_res_in, "kernel_res_in");
    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gamma_res, "gamma_res");
    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_beta_res, "beta_res");
    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_mean_res, "mean_res");
    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_var_res, "var_res");
    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_kernel_res_out, "kernel_res_out");
    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_bias_res_out, "bias_res_out");

    for (i=0; i<num_res_block2; ++i)
    {
        kernel_bk[i]=wtk_matf_heap_new( heap, res_out_dim, kernel_block_size*res_out_dim);
        gamma_bk[i]=wtk_vecf_heap_new( heap, res_out_dim);
        beta_bk[i]=wtk_vecf_heap_new( heap, res_out_dim);
        mean_bk[i]=wtk_vecf_heap_new( heap, res_out_dim);
        var_bk[i]=wtk_vecf_heap_new( heap, res_out_dim);

        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_kernel_bk[i], "kernel_bk", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gamma_bk[i], "gamma_bk", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_beta_bk[i], "beta_bk", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_mean_bk[i], "mean_bk", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_var_bk[i], "var_bk", i);
    }

    upsample_scale=wtk_local_cfg_find_int_array_s(wavrnn_lc, "upsample_scale");
    upsample_scale_iarr=upsample_scale->slot;
    upsample_scale_len=upsample_scale->nslot;
    up_layer_kernel=wtk_heap_malloc( heap, sizeof(void*)*upsample_scale_len);
    wavrnn->fn_up_layer_kernel=wtk_heap_malloc( heap, sizeof(void*)*upsample_scale_len);

    for (i=0; i<upsample_scale_len; ++i)
    {
        up_layer_kernel[i]=wtk_matf_heap_new( heap, 1, upsample_scale_iarr[i]*2+1);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_up_layer_kernel[i], "up_layer_kernel", i);
    }

    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_linear_kernel, "linear_kernel");
    wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_linear_bias, "linear_bias");
    
    fc_kernel[0]=wtk_matf_heap_new( heap, fc_dim, rnn_dim+aux_dim);
    fc_bias[0]=wtk_vecf_heap_new( heap, fc_dim);
    if (is_model_multiband) {}
    else if (is_model_small)
    {
        fc_kernel[1]=wtk_matf_heap_new( heap, bit, fc_dim);
        fc_bias[1]=wtk_vecf_heap_new( heap, bit);
    }
    else {
        fc_kernel[1]=wtk_matf_heap_new( heap, fc_dim, bit+aux_dim);
        fc_kernel[2]=wtk_matf_heap_new( heap, bit, fc_dim);
        fc_bias[1]=wtk_vecf_heap_new( heap, fc_dim);
        fc_bias[2]=wtk_vecf_heap_new( heap, bit);
    }

    for (i=0; i<fc_num; ++i)
    {
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_fc_kernel[i], "fc_kernel", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_fc_bias[i], "fc_bias", i);
    }

    wavrnn->fn_gru_kernel_gate=wtk_heap_malloc( heap, sizeof(void*)*gru_layers);
    wavrnn->fn_gru_kernel_candidate=wtk_heap_malloc( heap, sizeof(void*)*gru_layers);
    wavrnn->fn_gru_kernel_candidate_hh=wtk_heap_malloc( heap, sizeof(void*)*gru_layers);
    wavrnn->fn_gru_bias_gate=wtk_heap_malloc( heap, sizeof(void*)*gru_layers);
    wavrnn->fn_gru_bias_candidate=wtk_heap_malloc( heap, sizeof(void*)*gru_layers);
    wavrnn->fn_gru_bias_candidate_hh=wtk_heap_malloc( heap, sizeof(void*)*gru_layers);

    for (i=0; i<gru_layers; ++i)
    {
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gru_kernel_gate[i], "gru_kernel_gate", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gru_kernel_candidate[i], "gru_kernel_candidate", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gru_kernel_candidate_hh[i], "gru_kernel_candidate_hh", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gru_bias_gate[i], "gru_bias_gate", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gru_bias_candidate[i], "gru_bias_candidate", i);
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_gru_bias_candidate_hh[i], "gru_bias_candidate_hh", i);
    }

    if (is_model_multiband)
    {
        wavrnn->fn_fc_band_kernel=wtk_heap_malloc( heap, sizeof(void*)*fc_band_num);
        wavrnn->fn_fc_band_bias=wtk_heap_malloc( heap, sizeof(void*)*fc_band_num);
        for (i=0; i<fc_band_num; ++i)
        {
            fc_band_kernel[i]=wtk_matf_heap_new( heap, fc_band_dim, fc_dim);
            fc_band_bias[i]=wtk_vecf_heap_new( heap, fc_band_dim);

            wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_fc_band_kernel[i], "fc_band_kernel", i);
            wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_fc_band_bias[i], "fc_band_bias", i);
        }
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_pqmf_spec_filter_real, "pqmf_spec_filter_real");
        wtk_cfg_def_param_require_str( heap, wavrnn_lc, fn, v, wavrnn->fn_pqmf_spec_filter_imag, "pqmf_spec_filter_imag");
    }

    wavrnn->heap=heap;
    wavrnn->kernel_res_in=kernel_res_in;
    wavrnn->gamma_res=gamma_res;
    wavrnn->beta_res=beta_res;
    wavrnn->mean_res=mean_res;
    wavrnn->var_res=var_res;
    wavrnn->kernel_res_out=kernel_res_out;
    wavrnn->bias_res_out=bias_res_out;
    wavrnn->kernel_bk=kernel_bk;
    wavrnn->gamma_bk=gamma_bk;
    wavrnn->beta_bk=beta_bk;
    wavrnn->mean_bk=mean_bk;
    wavrnn->var_bk=var_bk;
    wavrnn->up_layer_kernel=up_layer_kernel;
    wavrnn->upsample_scale=upsample_scale;
    wavrnn->linear_kernel=linear_kernel;
    wavrnn->linear_bias=linear_bias;
    wavrnn->gru=gru;
    wavrnn->fc_kernel=fc_kernel;
    wavrnn->fc_bias=fc_bias;
    wavrnn->fc_band_kernel=fc_band_kernel;
    wavrnn->fc_band_bias=fc_band_bias;
    wavrnn->rf=rf;
    wavrnn->stft=stft;
    wavrnn->pqmf_spec_filter_real=pqmf_spec_filter_real;
    wavrnn->pqmf_spec_filter_imag=pqmf_spec_filter_imag;
}
void wtk_tac_cfg_syn_wavrnn_update( wtk_tac_hparams_t *hp, wtk_tac_cfg_syn_wavrnn_t *wavrnn, wtk_source_loader_t *sl)
{
    wtk_source_t s, *src=&s;
    wtk_heap_t *heap=wavrnn->heap;
    wtk_array_t *upsample_scale=wavrnn->upsample_scale;
    int num_res_block=hp->wavrnn_num_res_block*2
      , fc_size=hp->wavrnn_num_fc
      , gru_layers=hp->wavrnn_gru_layers
      , is_model_multiband=hp->wavrnn_is_model_multiband
      , fc_band_num=hp->wavrnn_fc_band_num
      , i;

    wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_kernel_res_in,wavrnn->kernel_res_in);
    wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_gamma_res, wavrnn->gamma_res);
    wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_beta_res, wavrnn->beta_res);
    wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_mean_res, wavrnn->mean_res);
    wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_var_res, wavrnn->var_res);
    wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_kernel_res_out, wavrnn->kernel_res_out);
    wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_bias_res_out, wavrnn->bias_res_out);

    
    for (i=0; i<num_res_block; ++i)
    {
        wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_kernel_bk[i], wavrnn->kernel_bk[i]);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_gamma_bk[i], wavrnn->gamma_bk[i]);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_beta_bk[i], wavrnn->beta_bk[i]);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_mean_bk[i], wavrnn->mean_bk[i]);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_var_bk[i], wavrnn->var_bk[i]);
    }

    for (i=0; i<upsample_scale->nslot; ++i)
    {
        wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_up_layer_kernel[i], wavrnn->up_layer_kernel[i]);
    }

    wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_linear_kernel, wavrnn->linear_kernel);
    wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_linear_bias, wavrnn->linear_bias);

    for (i=0; i<fc_size; ++i)
    {
        wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_fc_kernel[i],wavrnn->fc_kernel[i]);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_fc_bias[i], wavrnn->fc_bias[i]);
    }

    for (i=0; i<gru_layers; ++i)
    {
        wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_gru_kernel_gate[i],wavrnn->gru[i]->gate_kernel);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_gru_bias_gate[i], wavrnn->gru[i]->gate_bias);

        wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_gru_kernel_candidate[i],wavrnn->gru[i]->candidate_kernel);
        wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_gru_kernel_candidate_hh[i],wavrnn->gru[i]->candidate_kernel_hh);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_gru_bias_candidate[i], wavrnn->gru[i]->candidate_bias);
        wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_gru_bias_candidate_hh[i], wavrnn->gru[i]->candidate_bias_hh);
    }

    if (is_model_multiband)
    {
        for (i=0; i<fc_band_num; ++i)
        {
            wtk_mer_source_loader_load_matf2( heap, sl, src, wavrnn->fn_fc_band_kernel[i],wavrnn->fc_band_kernel[i]);
            wtk_mer_source_loader_load_vecf( sl, src, wavrnn->fn_fc_band_bias[i], wavrnn->fc_band_bias[i]);
        }
        wtk_mer_source_loader_load_matf( sl, src, wavrnn->fn_pqmf_spec_filter_real, wavrnn->pqmf_spec_filter_real);
        wtk_mer_source_loader_load_matf( sl, src, wavrnn->fn_pqmf_spec_filter_imag, wavrnn->pqmf_spec_filter_imag);
    }
}
void wtk_tac_cfg_syn_wavrnn_clean(wtk_tac_cfg_syn_wavrnn_t *wavrnn, wtk_tac_hparams_t *hp)
{
    int i;
    for (i=0; i<hp->wavrnn_gru_layers; ++i)
    {
        wtk_nn_rnngru_delete(wavrnn->gru[i]);
    }
    wtk_rfft_delete( wavrnn->rf);
    wtk_heap_delete( wavrnn->heap);
}