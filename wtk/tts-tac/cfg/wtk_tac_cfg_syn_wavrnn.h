#ifndef WTK_TAC_CFG_SYN_WAVRNN_H
#define WTK_TAC_CFG_SYN_WAVRNN_H
#include "wtk/tts-tac/wtk_tac_common.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{/* 
bk == block
 */
    wtk_heap_t *heap;
    wtk_matf_t *kernel_res_in;
    wtk_vecf_t *gamma_res;
    wtk_vecf_t *beta_res;
    wtk_vecf_t *mean_res;
    wtk_vecf_t *var_res;
    wtk_matf_t *kernel_res_out;
    wtk_vecf_t *bias_res_out;
    wtk_matf_t **kernel_bk; // len == 2*num_res_block
    wtk_vecf_t **gamma_bk;
    wtk_vecf_t **beta_bk;
    wtk_vecf_t **mean_bk;
    wtk_vecf_t **var_bk;
    wtk_matf_t **up_layer_kernel;
    wtk_matf_t *linear_kernel;
    wtk_vecf_t *linear_bias;
    wtk_nn_rnngru_t **gru;
    wtk_matf_t **fc_kernel;
    wtk_vecf_t **fc_bias;
    wtk_matf_t **fc_band_kernel;
    wtk_vecf_t **fc_band_bias;
    wtk_array_t *upsample_scale;
    wtk_rfft_t *rf;
    wtk_nn_stft_t *stft;
    wtk_matf_t *pqmf_spec_filter_real;
    wtk_matf_t *pqmf_spec_filter_imag;
    char *fn_kernel_res_in;
    char *fn_gamma_res;
    char *fn_beta_res;
    char *fn_mean_res;
    char *fn_var_res;
    char *fn_kernel_res_out;
    char *fn_bias_res_out;
    char **fn_kernel_bk;
    char **fn_gamma_bk;
    char **fn_beta_bk;
    char **fn_mean_bk;
    char **fn_var_bk;
    char **fn_up_layer_kernel;
    char *fn_linear_kernel;
    char *fn_linear_bias;
    char **fn_gru_kernel_gate;
    char **fn_gru_kernel_candidate;
    char **fn_gru_kernel_candidate_hh;
    char **fn_gru_bias_gate;
    char **fn_gru_bias_candidate;
    char **fn_gru_bias_candidate_hh;
    char **fn_fc_kernel;
    char **fn_fc_bias;
    char **fn_fc_band_kernel;
    char **fn_fc_band_bias;
    char *fn_pqmf_spec_filter_real;
    char *fn_pqmf_spec_filter_imag;
} wtk_tac_cfg_syn_wavrnn_t;

void wtk_tac_cfg_syn_wavrnn_init(wtk_tac_cfg_syn_wavrnn_t *wavrnn);
void wtk_tac_cfg_syn_wavrnn_update_local(wtk_tac_cfg_syn_wavrnn_t *wavrnn, wtk_local_cfg_t *wavrnn_lc, wtk_tac_hparams_t *hp);
void wtk_tac_cfg_syn_wavrnn_update( wtk_tac_hparams_t *hp, wtk_tac_cfg_syn_wavrnn_t *wavrnn, wtk_source_loader_t *sl);
void wtk_tac_cfg_syn_wavrnn_clean(wtk_tac_cfg_syn_wavrnn_t *wavrnn, wtk_tac_hparams_t *hp);
#ifdef __cplusplus
}
#endif
#endif