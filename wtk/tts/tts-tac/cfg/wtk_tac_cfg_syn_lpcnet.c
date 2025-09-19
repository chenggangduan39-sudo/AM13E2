#include "wtk_tac_cfg_syn_lpcnet.h"
int wtk_tac_cfg_syn_lpcnet_quanti_update(wtk_tac_cfg_syn_lpcnet_t *lpcnet);

int wtk_tac_cfg_syn_lpcnet_init(wtk_tac_cfg_syn_lpcnet_t *lpcnet)
{
    memset(lpcnet, 0, sizeof(wtk_tac_cfg_syn_lpcnet_t));
    lpcnet->nfft = 1024;
    // lpcnet->hop_len = 256;
    lpcnet->hop_len = 200;
    lpcnet->win_len = 800;
    lpcnet->feat_dim = 80;
    lpcnet->num_subband = 4;
    lpcnet->fold_len = 50;
    lpcnet->overlap = 1;
    lpcnet->conv_bufs = NULL;
    lpcnet->have_reset = 1;
    lpcnet->use_stream = 0;
    return 0;
}
int wtk_tac_cfg_syn_lpcnet_update_local(wtk_tac_cfg_syn_lpcnet_t *lpcnet, wtk_local_cfg_t *lpcnet_lc)
{
    wtk_heap_t *heap=wtk_heap_new(1024);
    wtk_string_t *v;
    char fn[256];
    int i, j;

    // -------------------  lpcnet cfg  ------------------
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_num_feat_conv, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_num_feat_dense, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_num_gru, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_num_md, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_pitch_max_period, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_embed_pitch_size, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_embed_size, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_feat_used_size, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_feat_conv_kernel_size, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_feat_conv_dim, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_gru_dim1, v);
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lpcnet_lc, lpcnet, lpcnet_gru_dim2, v);
    
    wtk_rfft_t *rf=wtk_rfft_new(lpcnet->nfft/2);
    wtk_nn_stft_t *stft=wtk_nn_stft_heap_new( heap, rf, lpcnet->nfft, lpcnet->hop_len, lpcnet->win_len);
    wtk_matf_t
        **feat_conv_kernel=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_conv),
        **feat_dense_kernel=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_dense),
        // *embed_pitch=wtk_matf_heap_new( heap, pitch_max_period, embed_pitch_size),
        **embed_sig_arr=wtk_heap_malloc( heap, sizeof(void*)*8),
        **embed_exc_arr=wtk_heap_malloc( heap, sizeof(void*)*4),
        ***md_kernel=wtk_heap_malloc( heap, sizeof(void**)*lpcnet->num_subband),
        // *mel_inv_basis=wtk_matf_heap_new( heap, stft->nfreq, lpcnet->feat_dim),
        *pqmf_analysis_filter_real=wtk_matf_heap_new( heap, lpcnet->num_subband, stft->nfreq),
        *pqmf_analysis_filter_imag=wtk_matf_heap_new( heap, lpcnet->num_subband, stft->nfreq),
        *pqmf_syn_filter_real=wtk_matf_heap_new( heap, lpcnet->num_subband, stft->nfreq),
        *pqmf_syn_filter_imag=wtk_matf_heap_new( heap, lpcnet->num_subband, stft->nfreq);
    wtk_vecf_t
        **feat_conv_bias=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_conv),
        **feat_dense_bias=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_dense),
        ***md_bias=wtk_heap_malloc( heap, sizeof(void**)*lpcnet->num_subband),
        ***md_factor=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->num_subband);
    wtk_nn_rnngru_t **gru_arr=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    
    lpcnet->fn_feat_conv_kernel=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_conv);
    lpcnet->fn_feat_conv_bias=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_conv);
    lpcnet->fn_feat_dense_kernel=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_dense);
    lpcnet->fn_feat_dense_bias=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_feat_dense);
    lpcnet->fn_gru_kernel_gate=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_kernel_candidate=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_kernel_candidate_hh=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_bias_gate=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_bias_candidate=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_bias_candidate_hh=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_weight_ih=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_weight_hh=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_bias_ih=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_gru_bias_hh=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_gru);
    lpcnet->fn_md_kernel=wtk_heap_malloc( heap, sizeof(void**)*4);
    lpcnet->fn_md_bias=wtk_heap_malloc( heap, sizeof(void**)*4);
    lpcnet->fn_md_factor=wtk_heap_malloc( heap, sizeof(void**)*4);


    // wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_embed_pitch, "embed_pitch");
    lpcnet->fn_embed_sig=wtk_heap_malloc( heap, sizeof(void*)*8);
    lpcnet->fn_embed_exc=wtk_heap_malloc( heap, sizeof(void*)*8);
    for (i=0; i<8; ++i)
    {
        embed_sig_arr[i]=wtk_matf_heap_new( heap, 256, lpcnet->lpcnet_gru_dim1*3);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_embed_sig[i], "embed_sig", i);

        if (i<4)
        {
            embed_exc_arr[i]=wtk_matf_heap_new( heap, 256, lpcnet->lpcnet_gru_dim1*3);
            wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_embed_exc[i], "embed_exc", i);
        }
    }

    feat_conv_kernel[0]=wtk_matf_heap_new( heap, lpcnet->lpcnet_feat_conv_dim, (lpcnet->feat_dim)*lpcnet->lpcnet_feat_conv_kernel_size);
    feat_conv_kernel[1]=wtk_matf_heap_new( heap,lpcnet->feat_dim, lpcnet->lpcnet_feat_conv_dim*lpcnet->lpcnet_feat_conv_kernel_size);
    feat_conv_bias[0]=wtk_vecf_heap_new( heap, lpcnet->lpcnet_feat_conv_dim);
    feat_conv_bias[1]=wtk_vecf_heap_new( heap, lpcnet->feat_dim);

    feat_dense_kernel[0]=wtk_matf_heap_new( heap, 128, lpcnet->feat_dim);
    feat_dense_kernel[1]=wtk_matf_heap_new( heap, 128, 128);
    feat_dense_bias[0]=wtk_vecf_heap_new( heap, 128);
    feat_dense_bias[1]=wtk_vecf_heap_new( heap, 128);

    gru_arr[0]=wtk_nn_rnngru_new2( wtk_nn_enum_type_pytorch, 1, 320, lpcnet->lpcnet_gru_dim1, NULL);
    gru_arr[1]=wtk_nn_rnngru_new2( wtk_nn_enum_type_pytorch, 1, 512, lpcnet->lpcnet_gru_dim2, NULL);

    for (i=0; i<lpcnet->lpcnet_num_md; ++i)
    {
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_feat_conv_kernel[i], "feat_conv_kernel", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_feat_conv_bias[i], "feat_conv_bias", i);

        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_feat_dense_kernel[i], "feat_dense_kernel", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_feat_dense_bias[i], "feat_dense_bias", i);
    }

    for (j=0; j<4; ++j)
    {
        lpcnet->fn_md_kernel[j]=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_md);
        lpcnet->fn_md_bias[j]=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_md);
        lpcnet->fn_md_factor[j]=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_md);
        md_kernel[j]=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_md);
        md_bias[j]=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_md);
        md_factor[j]=wtk_heap_malloc( heap, sizeof(void*)*lpcnet->lpcnet_num_md);

        for (i=0; i<lpcnet->lpcnet_num_md; ++i)
        {
            wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_md_kernel[j][i], "md_kernel", j, i+1);
            wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_md_bias[j][i], "md_bias", j, i+1);
            wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_md_factor[j][i], "md_factor", j, i+1);

            md_kernel[j][i]=wtk_matf_heap_new( heap, 256, lpcnet->lpcnet_gru_dim2/4); 
            md_bias[j][i]=wtk_vecf_heap_new( heap, 256);
            md_factor[j][i]=wtk_vecf_heap_new( heap, 256);
        }
    }

    for (i=0; i<lpcnet->lpcnet_num_gru; ++i)
    {
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_kernel_gate[i], "gru_kernel_gate", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_kernel_candidate[i], "gru_kernel_candidate", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_kernel_candidate_hh[i], "gru_kernel_candidate_hh", i);

        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_bias_gate[i], "gru_bias_gate", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_bias_candidate[i], "gru_bias_candidate", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_bias_candidate_hh[i], "gru_bias_candidate_hh", i);

        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_weight_ih[i], "gru_weight_ih", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_weight_hh[i], "gru_weight_hh", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_bias_ih[i], "gru_bias_ih", i);
        wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_gru_bias_hh[i], "gru_bias_hh", i);
    }

    /* pqmf load */
    wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_mel_inv_basis, "mel_inv_basis");
    wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_pqmf_analysis_filter_real, "pqmf_analysis_filter_real");
    wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_pqmf_analysis_filter_imag, "pqmf_analysis_filter_imag");
    wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_pqmf_syn_filter_real, "pqmf_syn_filter_real");
    wtk_cfg_def_param_require_str( heap, lpcnet_lc, fn, v, lpcnet->fn_pqmf_syn_filter_imag, "pqmf_syn_filter_imag");

    /*是否使用流式*/
    wtk_local_cfg_update_cfg_i(lpcnet_lc,lpcnet,use_stream,v);

    lpcnet->heap=heap;
    // lpcnet->embed_pitch=embed_pitch;
    lpcnet->embed_sig_arr=embed_sig_arr;
    lpcnet->embed_exc_arr=embed_exc_arr;
    lpcnet->feat_conv_kernel=feat_conv_kernel;
    lpcnet->feat_conv_bias=feat_conv_bias;
    lpcnet->feat_dense_kernel=feat_dense_kernel;
    lpcnet->feat_dense_bias=feat_dense_bias;
    lpcnet->gru_arr=gru_arr;
    lpcnet->md_kernel=md_kernel;
    lpcnet->md_bias=md_bias;
    lpcnet->md_factor=md_factor;
    // lpcnet->mel_inv_basis=mel_inv_basis;
    lpcnet->pqmf_analysis_filter_real=pqmf_analysis_filter_real;
    lpcnet->pqmf_analysis_filter_imag=pqmf_analysis_filter_imag;
    lpcnet->pqmf_syn_filter_real=pqmf_syn_filter_real;
    lpcnet->pqmf_syn_filter_imag=pqmf_syn_filter_imag;
    lpcnet->stft=stft;
    lpcnet->stream_heap = wtk_heap_new(2048);

    // 时候使用量化
    wtk_local_cfg_update_cfg_i(lpcnet_lc,lpcnet,use_quanti,v);
    return 0;
}
//无正常load  cfg 简单搞一个cfg配置 
int wtk_tac_cfg_syn_lpcnet_update(wtk_tac_cfg_syn_lpcnet_t *lpcnet)
{
    wtk_source_loader_t l,*sl = NULL;
    wtk_source_t s, *src=&s;
    int i
      , j
      , num=lpcnet->lpcnet_num_md;

    wtk_source_loader_init_file(&l);
    sl = &l;
    // wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_embed_pitch, lpcnet->embed_pitch);
    for (i=0; i<8; ++i)
    {
        wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_embed_sig[i], lpcnet->embed_sig_arr[i]);
        if (i<4)
        {
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_embed_exc[i], lpcnet->embed_exc_arr[i]);
        }
    }

    for(i=0; i<num; ++i)
    {
        wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_feat_conv_kernel[i], lpcnet->feat_conv_kernel[i]);
        wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_feat_conv_bias[i], lpcnet->feat_conv_bias[i]);

        wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_feat_dense_kernel[i], lpcnet->feat_dense_kernel[i]);
        wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_feat_dense_bias[i], lpcnet->feat_dense_bias[i]);

        // gru
        if (i==1)
        {
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_kernel_gate[i], lpcnet->gru_arr[i]->gate_kernel);
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_kernel_candidate[i], lpcnet->gru_arr[i]->candidate_kernel);
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_kernel_candidate_hh[i], lpcnet->gru_arr[i]->candidate_kernel_hh);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_gate[i], lpcnet->gru_arr[i]->gate_bias);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_candidate[i], lpcnet->gru_arr[i]->candidate_bias);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_candidate_hh[i], lpcnet->gru_arr[i]->candidate_bias_hh);
        } else if (i==0) 
        {// gru weight
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_weight_ih[i], lpcnet->gru_arr[i]->weight_ih);
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_weight_hh[i], lpcnet->gru_arr[i]->weight_hh);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_ih[i], lpcnet->gru_arr[i]->bias_ih);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_hh[i], lpcnet->gru_arr[i]->bias_hh);
        }

        for (j=0; j<4; ++j)
        {
            // md
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_md_kernel[j][i], lpcnet->md_kernel[j][i]);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_md_bias[j][i], lpcnet->md_bias[j][i]);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_md_factor[j][i], lpcnet->md_factor[j][i]);
        }
    }

    // wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_mel_inv_basis, lpcnet->mel_inv_basis);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_analysis_filter_real, lpcnet->pqmf_analysis_filter_real);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_analysis_filter_imag, lpcnet->pqmf_analysis_filter_imag);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_syn_filter_real, lpcnet->pqmf_syn_filter_real);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_syn_filter_imag, lpcnet->pqmf_syn_filter_imag);

    /* lpcnet 部分矩阵no transpose 处理 */
    wtk_tac_cfg_syn_lpcnet_notrans_process( lpcnet);

    lpcnet->conv_bufs = wtk_strbufs_new(lpcnet->lpcnet_num_feat_conv);
    lpcnet->mels_buf = wtk_strbuf_new(1024,1.0f);
    lpcnet->stream_wav_buf = wtk_strbufs_new(lpcnet->num_subband);

    if(lpcnet->use_quanti){
        wtk_tac_cfg_syn_lpcnet_quanti_update(lpcnet);
    }
    return 0;
}

int wtk_tac_cfg_syn_lpcnet_update2(wtk_tac_cfg_syn_lpcnet_t *lpcnet, wtk_source_loader_t *sl)
{
    wtk_source_t s, *src=&s;
    int i
      , j
      , num=lpcnet->lpcnet_num_md;

    // wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_embed_pitch, lpcnet->embed_pitch);
    for (i=0; i<8; ++i)
    {
        wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_embed_sig[i], lpcnet->embed_sig_arr[i]);
        if (i<4)
        {
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_embed_exc[i], lpcnet->embed_exc_arr[i]);
        }
    }

    for(i=0; i<num; ++i)
    {
        wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_feat_conv_kernel[i], lpcnet->feat_conv_kernel[i]);
        wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_feat_conv_bias[i], lpcnet->feat_conv_bias[i]);

        wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_feat_dense_kernel[i], lpcnet->feat_dense_kernel[i]);
        wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_feat_dense_bias[i], lpcnet->feat_dense_bias[i]);

        // gru
        if (i==1)
        {
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_kernel_gate[i], lpcnet->gru_arr[i]->gate_kernel);
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_kernel_candidate[i], lpcnet->gru_arr[i]->candidate_kernel);
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_kernel_candidate_hh[i], lpcnet->gru_arr[i]->candidate_kernel_hh);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_gate[i], lpcnet->gru_arr[i]->gate_bias);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_candidate[i], lpcnet->gru_arr[i]->candidate_bias);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_candidate_hh[i], lpcnet->gru_arr[i]->candidate_bias_hh);
        } else if (i==0) 
        {// gru weight
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_weight_ih[i], lpcnet->gru_arr[i]->weight_ih);
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_gru_weight_hh[i], lpcnet->gru_arr[i]->weight_hh);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_ih[i], lpcnet->gru_arr[i]->bias_ih);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_gru_bias_hh[i], lpcnet->gru_arr[i]->bias_hh);
        }

        for (j=0; j<4; ++j)
        {
            // md
            wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_md_kernel[j][i], lpcnet->md_kernel[j][i]);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_md_bias[j][i], lpcnet->md_bias[j][i]);
            wtk_mer_source_loader_load_vecf( sl, src, lpcnet->fn_md_factor[j][i], lpcnet->md_factor[j][i]);
        }
    }

    // wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_mel_inv_basis, lpcnet->mel_inv_basis);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_analysis_filter_real, lpcnet->pqmf_analysis_filter_real);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_analysis_filter_imag, lpcnet->pqmf_analysis_filter_imag);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_syn_filter_real, lpcnet->pqmf_syn_filter_real);
    wtk_mer_source_loader_load_matf( sl, src, lpcnet->fn_pqmf_syn_filter_imag, lpcnet->pqmf_syn_filter_imag);

    /* lpcnet 部分矩阵no transpose 处理 */
    wtk_tac_cfg_syn_lpcnet_notrans_process( lpcnet);

    return 0;
}

int wtk_tac_cfg_syn_lpcnet_clean(wtk_tac_cfg_syn_lpcnet_t *lpcnet)
{
    int num_gru=lpcnet->lpcnet_num_gru, i;
    if(lpcnet->gru_arr){
        for (i=0; i<num_gru; ++i)
        {
            if(lpcnet->gru_arr[i]) wtk_nn_rnngru_delete( lpcnet->gru_arr[i]);
        }
    }
    if(lpcnet->conv_bufs){
        wtk_strbufs_delete(lpcnet->conv_bufs,lpcnet->lpcnet_num_feat_conv);
    }
    if (lpcnet->mels_buf)
    	wtk_strbuf_delete(lpcnet->mels_buf);
    if(lpcnet->stft)
    	wtk_rfft_delete(lpcnet->stft->rf);
    if (lpcnet->heap)
    	wtk_heap_delete(lpcnet->heap);
    if(lpcnet->stream_heap)
    	wtk_heap_delete(lpcnet->stream_heap);
    if(lpcnet->stream_wav_buf)
    	wtk_strbufs_delete(lpcnet->stream_wav_buf,lpcnet->num_subband);
    return 0;
}

/* 
因为大部分矩阵都是通过转置加载进来的, 无法使用 wtk_mer_unblas_notrans计算.
所以需要再转置回来
 */
void wtk_tac_cfg_syn_lpcnet_notrans_process(wtk_tac_cfg_syn_lpcnet_t *lpcnet)
{
    #define wtk_mer_matf_transpose_replace( heap, s, name) {\
    wtk_matf_t *a=wtk_matf_heap_new(heap, ((s)->name)->col, ((s)->name)->row); \
    wtk_matf_init_transpose(  ((s)->name), a);\
     ((s)->name)=a;\
    }

    wtk_debug(" weight_ih 转置\n");
    wtk_heap_t *heap=lpcnet->heap;
    wtk_nn_rnngru_t 
        **gru_arr=lpcnet->gru_arr;
    wtk_matf_t 
        ***md_kernel_arr=lpcnet->md_kernel,
        *md=NULL;

    wtk_mer_matf_transpose_replace( heap, gru_arr[0], weight_ih);
    wtk_mer_matf_transpose_replace( heap, gru_arr[1], gate_kernel);
    wtk_mer_matf_transpose_replace( heap, gru_arr[1], candidate_kernel);
    wtk_mer_matf_transpose_replace( heap, gru_arr[1], candidate_kernel_hh);

    int i, j;
    for (i=0; i<4; ++i)
    {
        for (j=0; j<2; ++j)
        {
            md = wtk_matf_heap_new( heap, md_kernel_arr[i][j]->col, md_kernel_arr[i][j]->row);
            wtk_matf_init_transpose( md_kernel_arr[i][j], md);
            md_kernel_arr[i][j] = md;
        }
    }
}

//配置量化 前量化 模型在训练的时候就需要做量化
int wtk_tac_cfg_syn_lpcnet_quanti_update(wtk_tac_cfg_syn_lpcnet_t *lpcnet)
{
    wtk_debug("use quanti\n");
    wtk_heap_t *heap = lpcnet->heap;

    lpcnet->feat_conv_quanti_kernel = wtk_heap_malloc(heap,sizeof(void*)*lpcnet->lpcnet_num_feat_conv);

    lpcnet->feat_conv_quanti_kernel[0] = wtk_matq8_heap_new(heap,lpcnet->feat_conv_kernel[0]->row,lpcnet->feat_conv_kernel[0]->col);
    wtk_mat_f32toi8_transfer(lpcnet->feat_conv_kernel[0],lpcnet->feat_conv_quanti_kernel[0]);
    lpcnet->feat_conv_quanti_kernel[1] = wtk_matq8_heap_new(heap,lpcnet->feat_conv_kernel[1]->row,lpcnet->feat_conv_kernel[1]->col);
    wtk_mat_f32toi8_transfer(lpcnet->feat_conv_kernel[1],lpcnet->feat_conv_quanti_kernel[1]);

    return 0;
}
