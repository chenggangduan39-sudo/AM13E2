/*
 * qtk_vits_models.h
 *
 *  Created on: Aug 25, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_MODELS_H_
#define QTK_VITS_QTK_VITS_MODELS_H_
#include "qtk_vits_attn.h"
#ifdef __cplusplus
extern "C" {
#endif
// class models.TextEncoder
typedef struct qtk_vits_models_enctxt{
//	int n_vocab;
//	int out_channels;
//	int hidden_channels;
//	int filter_channels;
//	int n_heads;
//	int n_layers;
//	int kernel_size;
//	void* p_dropout;

	qtk_torchnn_emb_t* emb;
	qtk_attn_enc_t* enc;
	qtk_torchnn_conv1d_t* proj;
}qtk_vits_models_enctxt_t;

qtk_vits_models_enctxt_t* qtk_vits_models_enctxt_new(qtk_vits_cfg_t* cfg);
void qtk_vits_models_enctxt_delete(qtk_vits_models_enctxt_t* enct);
qtk_vits_models_enctxt_t* qtk_vits_models_enctxt_read(qtk_vits_models_enctxt_t* enct, wtk_source_t* src, int bin);
int qtk_vits_models_enctxt_forward(qtk_vits_models_enctxt_t* enc, int* x, int x_len);

//class models.generator
typedef struct{
	int num_kernels;
	int num_upsamples;
	qtk_torchnn_conv1d_t* conv_pre;
	//float* conv_pre_bias;
	qtk_torchnn_weight_norm_t* ups;   //nn.ModuleList
	int nups;
	qtk_syntrn_resblocks_t* resblocks; //nn.ModuleList
	int nresblocks;
	qtk_torchnn_conv1d_t* conv_post;
//	float*** conv_post_weight;
	qtk_torchnn_conv1d_t* cond;
}qtk_vits_models_gen_t;

qtk_vits_models_gen_t* qtk_vits_models_gen_new(qtk_vits_syn_cfg_t* cfg);
void qtk_vits_models_gen_delete(qtk_vits_models_gen_t* dec);
qtk_vits_models_gen_t* qtk_vits_models_gen_read(qtk_vits_models_gen_t* dec, qtk_vits_syn_cfg_t* cfg, wtk_source_t* src, int bin);
void qtk_vits_models_gen_fw(qtk_vits_models_gen_t* dec);

// class models.PosteriorEncoder
typedef struct{
	void* in_channels;
	void* out_channels;
	void* hidden_channels;
	void* kernel_size;
	void* dilation_rate;
	int n_layers;
	int gin_channels;

	qtk_torchnn_conv1d_t* pre;
	qtk_syntrn_modules_wn_t* enc;
	qtk_torchnn_conv1d_t* proj;
//	float* proj_bias;
}qtk_vits_models_encpost_t;

qtk_vits_models_encpost_t* qtk_vits_model_encpost_new(qtk_vits_syn_cfg_t* cfg);
void qtk_vits_model_encpost_fw(qtk_vits_models_encpost_t* encpost);
qtk_vits_models_encpost_t* qtk_vits_model_encpost_read(qtk_vits_models_encpost_t* encpost, qtk_vits_syn_cfg_t* cfg, wtk_source_t* src, int bin);

// class models.ResidualCouplingBlock
typedef struct{
	void* channels;
	void* hidden_channels;
	void* kernel_size;
	void* dilation_rate;
	int n_layers;
	int n_flows;
	int gin_channels;
	qtk_syntrn_modulelist_t* flows;
}qtk_vits_models_flow_t;
qtk_vits_models_flow_t* qtk_vits_models_flow_new(qtk_vits_syn_cfg_t* cfg);
void qtk_vits_models_flow_delete(qtk_vits_models_flow_t* flow);
qtk_vits_models_flow_t* qtk_vits_models_flow_read(qtk_vits_models_flow_t* flow, qtk_vits_syn_cfg_t* cfg, wtk_source_t* src, int bin);
void qtk_vits_models_flow_fw(qtk_vits_models_flow_t* flow);

// class models.StochasticDurationPredictor
typedef struct{
	void* in_channels;
	void* filter_channels;
	void* kernel_size;
	void* p_dropout;
	int n_flows;
	int gin_channels;

	void* log_flow;
	qtk_syntrn_modulelist* flows;   //qtk_vits_modules_ewaffine_t
	qtk_torchnn_conv1d_t* post_pre;
	qtk_torchnn_conv1d_t* post_proj;
	qtk_vits_modules_ddsconv_t* post_conv;
	qtk_vits_modules_ewaffine_t* post_flows;
	qtk_torchnn_conv1d_t* pre;
	qtk_torchnn_conv1d_t* proj;
	qtk_vits_modules_ddsconv_t* convs;
	qtk_torchnn_conv1d_t* cond;
}qtk_vits_models_stocdp_t;
qtk_vits_models_stocdp_t* qtk_vits_models_stocdp_new(qtk_vits_syn_cfg_t* cfg);
void qtk_vits_models_stocdp_fw(qtk_vits_models_stocdp_t* dp);


// class models.DurationPredictor
typedef struct{
//	void* in_channels;
//	void* filter_channels;
//	void* kernel_size;
//	void* p_dropout;
//	int gin_channels;
//	qtk_torchnn_dropout_t* drop;
	qtk_torchnn_conv1d_t* conv_1;
	qtk_torchnn_conv1d_t* conv_2;
	qtk_torchnn_conv1d_t* proj;
	qtk_torchnn_conv1d_t* cond;

}qtk_vits_models_dp_t;

struct qtk_vits_dp{
	union{
		qtk_vits_models_stocdp_t* stodp;
		qtk_vits_models_dp_t* dp;
	};
//	float** m;
//	float** logs;
};

// class models.SynthesizerTrn
typedef struct{
//	int n_vocab;
//	int spec_channels;
//	int inter_channels;
//	int hidder_channels;
//	int filter_channels;
//	int n_heads;
//	int n_labyers;
//	int kernel_size;
//	float p_dropput;
//	char * resblock;
//	int*resblock_kernel_sizes;
//	int** resblock_dilation_sizes;
//	int* upsample_rates;
//	int upsample_initial_channel;
//	int* upsample_kernel_sizes;
//	int n_speakers;
//	int gin_channels;
//	int use_sdp;

	qtk_vits_syn_cfg_t* cfg;

	wtk_matf_t *emb;
	qtk_vits_models_enctxt_t* enc_p;
	qtk_vits_models_gen_t* dec;
	qtk_vits_models_encpost_t* enc_q;
	qtk_vits_models_flow_t* flow;
	qtk_vits_models_dp_t* dp;
	qtk_torchnn_emb_t* emb_g;
}qtk_vits_models_syntrn_t;

qtk_vits_models_syntrn_t* qtk_vits_models_syntrn_new(qtk_vits_syn_cfg_t *cfg);
int qtk_vits_models_syntrn_forward(qtk_vits_models_syntrn_t* syntrn, int x, int x_len);
#ifdef _cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_MODELS_H_ */
