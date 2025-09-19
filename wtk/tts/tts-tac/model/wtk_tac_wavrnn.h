#ifndef WTK_TAC_WAVRNN_H
#define WTK_TAC_WAVRNN_H
#include "tts-tac/wtk_tac_common.h"
#include "tts-tac/model/wtk_tac.h"
#include "tts-tac/cfg/wtk_tac_cfg_syn_wavrnn.h"
#ifdef __cplusplus
extern "C" {
#endif

void wtk_tac_wavrnn( wtk_tac_hparams_t *hp, wtk_tac_cfg_syn_wavrnn_t *cfg, wtk_matf_t *mel, wtk_mer_wav_stream_t *wav);
wtk_matf_t* wtk_tac_fold_with_overlap( int target, int overlap, wtk_matf_t *x, int *padding);
void wtk_nn_rnngru_batch_wavrnn(wtk_nn_rnngru_t *cell, wtk_matf_t *gin, wtk_matf_t *gout, wtk_matf_t *cout, wtk_matf_t *cout2, int batch_size, wtk_matf_t *in, wtk_matf_t *out, wtk_matf_t *h_inmf, wtk_matf_t *h_outmf);
int wtk_nn_random_weight(int n, float *weight);
void wtk_tac_wavrnn_xfade_unfold(int overlap, wtk_matf_t *x, float *wav_data);
void wtk_tac_wavrnn_multiband_xfade_unfold(int overlap, int subband, int padding, wtk_matf_t *x, float *wav_data, float **subband_data, int *wav_data_len_out/* 单个wav_data长度 */);
void wtk_tac_wavrnn_pqmf( wtk_matf_t *spec_filter_real, wtk_matf_t *spec_filter_imag, wtk_nn_stft_t *stft, wtk_matf_t *in, int subbnad, float *out, int *out_len);

#ifdef __cplusplus
}
#endif
#endif
