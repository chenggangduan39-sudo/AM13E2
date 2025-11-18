#ifndef WTK_TAC_CFG_SYN_LPCNET_H
#define WTK_TAC_CFG_SYN_LPCNET_H
#include "wtk/tts-tac/wtk_tac_common.h"
#ifdef __cplusplus
extern "C" {
#endif

//暂时写在这里的回调函数
typedef void (*wtk_lpcnet_notify_f)(void *user_data,float *data,int len,int is_end);

/* 
用于 gru batch 计算时缓存结果
 */
typedef struct{
    wtk_matf_t *ih_out_mf;
    wtk_matf_t *hh_out_mf;
} wtk_nn_rnngru_batch_cache_t;

typedef struct
{
    wtk_heap_t *heap;

    // lpcnet cfg
    int lpcnet_num_feat_conv;
    int lpcnet_num_feat_dense;
    int lpcnet_num_gru;
    int lpcnet_num_md;
    int lpcnet_pitch_max_period;
    int lpcnet_embed_pitch_size;
    int lpcnet_embed_size;
    int lpcnet_feat_used_size;
    int lpcnet_feat_conv_kernel_size;
    int lpcnet_feat_conv_dim;
    int lpcnet_gru_dim1;
    int lpcnet_gru_dim2;

    int num_subband;

    int nfft;
    int hop_len;
    int win_len;
    int feat_dim;
    //overlop
    int fold_len; 
    int overlap;

    wtk_matf_t **feat_conv_kernel;
    wtk_vecf_t **feat_conv_bias;
    wtk_matf_t **feat_dense_kernel;
    wtk_vecf_t **feat_dense_bias;
    wtk_matf_t *embed_pitch;
    wtk_matf_t **embed_sig_arr;
    wtk_matf_t **embed_exc_arr;
    wtk_nn_rnngru_t **gru_arr;
    wtk_matf_t ***md_kernel;
    wtk_vecf_t ***md_bias;
    wtk_vecf_t ***md_factor;
    // wtk_matf_t *mel_inv_basis;
    wtk_matf_t *pqmf_analysis_filter_real;
    wtk_matf_t *pqmf_analysis_filter_imag;
    wtk_matf_t *pqmf_syn_filter_real;
    wtk_matf_t *pqmf_syn_filter_imag;
    wtk_nn_stft_t *stft;
    
    char **fn_feat_conv_kernel;
    char **fn_feat_conv_bias;
    char **fn_feat_dense_kernel;
    char **fn_feat_dense_bias;
    char *fn_embed_pitch;
    char **fn_embed_sig;
    char **fn_embed_exc;
    char **fn_gru_kernel_gate;
    char **fn_gru_kernel_candidate;
    char **fn_gru_kernel_candidate_hh;
    char **fn_gru_bias_gate;
    char **fn_gru_bias_candidate;
    char **fn_gru_bias_candidate_hh;
    char **fn_gru_weight_ih;
    char **fn_gru_weight_hh;
    char **fn_gru_bias_ih;
    char **fn_gru_bias_hh;
    char ***fn_md_kernel;
    char ***fn_md_bias;
    char ***fn_md_factor;
    char *fn_mel_inv_basis;
    char *fn_pqmf_analysis_filter_real;
    char *fn_pqmf_analysis_filter_imag;
    char *fn_pqmf_syn_filter_real;
    char *fn_pqmf_syn_filter_imag;
    
    //在stream的时候储存数据
    wtk_heap_t *stream_heap;
    wtk_strbuf_t **conv_bufs;
    wtk_strbuf_t *mels_buf;
    wtk_matf_t *stream_fexc;
    wtk_mati_t *stream_iexc;
    wtk_matf_t *stream_gru_a_ih_out;
    wtk_matf_t **stream_h_mf;
    wtk_matf_t **stream_gru_in;
    wtk_matf_t **stream_gru_gin;
    wtk_matf_t **stream_gru_gout;
    wtk_matf_t **stream_gru_cout;
    wtk_matf_t **stream_gru_cout_hh;
    wtk_matf_t **stream_gru_out;
    wtk_matf_t ***stream_md_out;
    wtk_nn_rnngru_batch_cache_t *stream_gru_cache;
    wtk_matdf_t *pre_pcm;
    wtk_strbuf_t **stream_wav_buf;   //在过pqfm前存储wav
    //暂时在这里来做
    void *user_data;
    wtk_lpcnet_notify_f notify;
    int have_reset;
    int stream_skip;
    int pqmf_pad_flag;
    int pqmf_out_n; //已经输出了几帧数据了
    float mem;
    int use_stream;

    //quanti 定点量化
    wtk_matq8_t **feat_conv_quanti_kernel;
    // wtk_vecf_t **feat_conv_bias;
    int use_quanti;
} wtk_tac_cfg_syn_lpcnet_t;

int wtk_tac_cfg_syn_lpcnet_init(wtk_tac_cfg_syn_lpcnet_t *lpcnet);
int wtk_tac_cfg_syn_lpcnet_update_local(wtk_tac_cfg_syn_lpcnet_t *lpcnet, wtk_local_cfg_t *lpcnet_lc);
int wtk_tac_cfg_syn_lpcnet_update(wtk_tac_cfg_syn_lpcnet_t *lpcnet);
int wtk_tac_cfg_syn_lpcnet_update2(wtk_tac_cfg_syn_lpcnet_t *lpcnet, wtk_source_loader_t *sl);
int wtk_tac_cfg_syn_lpcnet_clean(wtk_tac_cfg_syn_lpcnet_t *lpcnet);
void wtk_tac_cfg_syn_lpcnet_notrans_process(wtk_tac_cfg_syn_lpcnet_t *lpcnet);

#ifdef __cplusplus
}
#endif
#endif