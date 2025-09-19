/*
 * qtk_vits_syn_cfg.h
 *
 *  Created on: Sep 28, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_SYN_CFG_H_
#define QTK_VITS_QTK_VITS_SYN_CFG_H_
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_vits_syn_cfg qtk_vits_syn_cfg_t;

struct qtk_vits_syn_cfg
{
	wtk_heap_t *h;

	//train
	int segment_size;

	//data
	float max_wav_value;
	float sampling_rate;
	float filter_length;
	float hop_length;
	float win_length;
	unsigned int add_blank:1;
	int n_speakers;

	//model
//	int n_vocab;
	int spec_channels;
	int inter_channels;
	int hidder_channels;
	int filter_channels;
	int n_heads;
	int n_labyers;
	int kernel_size;
	float p_dropput;
	char * resblock;
	wtk_array_t resblock_kernel_sizes;
	qtk_blas_matrix_t* resblock_dilation_sizes;
	int n_resblock_dilation_sizes;
	wtk_array_t upsample_rates;
	int upsample_initial_channel;
	wtk_array_t upsample_kernel_sizes;
	int n_layers_q;
	unsigned int use_spectral_norm:1;

	char *mdl_fn;
//	int gin_channels;
//	int use_sdp;
};

int qtk_vits_syn_cfg_init(qtk_vits_syn_cfg_t *cfg);
int qtk_vits_syn_cfg_clean(qtk_vits_syn_cfg_t *cfg);
int qtk_vits_syn_cfg_update_local(qtk_vits_syn_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_vits_syn_cfg_update(qtk_vits_syn_cfg_t *cfg);
int qtk_vits_syn_cfg_update2(qtk_vits_syn_cfg_t *cfg,wtk_source_loader_t *sl);

#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_SYN_CFG_H_ */
