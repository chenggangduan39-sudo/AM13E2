/*
 * qtk_vits_attn.h
 *
 *  Created on: Aug 25, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_ATTN_H_
#define QTK_VITS_QTK_VITS_ATTN_H_
#include "qtk/torchnn/qtk_torchnn.h"
#include "qtk_vits_modules.h"
#ifdef __cplusplus
extern "C" {
#endif
// qtk.attn
//class Multi_attentions.MultiHeadAttention
typedef struct{
//	void* channels;
//	void* out_channels;
//	int n_heads;
//	float p_dropout;
//	void* window_size;
//	int heads_share;
//	void* block_length;
//	int proximal_bias;
//	int proximal_init;
//	void* attn;
//	void*  k_channels;
	qtk_blas_matrix_t* emb_rel_k;           //Note: [1,9,96] in fact, here using [9,96]
	qtk_blas_matrix_t* emb_rel_v;           //Note: [1,9,96] in fact, here using [9,96]
	qtk_torchnn_conv1d_t conv_q;
	qtk_torchnn_conv1d_t conv_k;
	qtk_torchnn_conv1d_t conv_v;
	qtk_torchnn_conv1d_t conv_o;
}qtk_attn_attnlayer_t;

// class Multi_attentions.FFN
typedef struct {
//	void* in_channels;
//	void* out_channels;
//	void* filter_channels;
//	void* kernel_size;
//	float p_dropout;
//	void* activation;
//	int causal;
//	void* padding;

	qtk_torchnn_conv1d_t* conv_1;
	qtk_torchnn_conv1d_t* conv_2;
//	qtk_torchnn_dropout_t* drop;
}qtk_attn_ffnlayer_t;

qtk_attn_ffnlayer_t* qtk_attn_ffnlayer_new(qtk_attn_ffnlayer_t* l, qtk_vits_cfg_t* cfg, int bin);
void qtk_attn_ffnlayer_del(qtk_attn_ffnlayer_t* l);
qtk_attn_ffnlayer_t* qtk_attn_ffnlayer_read(qtk_attn_ffnlayer_t* l, qtk_vits_cfg_t* cfg, wtk_source_t* src, int bin);
// class Multi_attentions.Encoder
typedef struct{
//	int hidden_channels;
//	int filter_channels;
//	int n_heads;
//	int n_layers;
//	int kernel_size;
//	float p_dropout;
//	int window_size;
	qtk_torchnn_dropout_t* drop;
	qtk_attn_attnlayer_t* attnlayers;
	qtk_vits_modules_normlayer_t* normlayers_1;
	qtk_attn_ffnlayer_t* ffnlayers;
	qtk_vits_modules_normlayer_t* normlayers_2;
}qtk_attn_enc_t;

qtk_attn_attnlayer_t* qtk_attn_layer_new(qtk_vits_cfg_t* cfg);
void qtk_attn_layer_del(qtk_attn_attnlayer_t* layer);
qtk_attn_attnlayer_t* qtk_attn_layer_read(qtk_attn_attnlayer_t* layer, qtk_vits_cfg_t* cfg, wtk_source_t* src, int bin);

qtk_attn_enc_t* qtk_attn_enc_new(qtk_vits_syn_cfg_t* cfg);
void qtk_attn_enc_del(qtk_attn_enc_t* enc);
qtk_attn_enc_t* qtk_attn_enc_read(qtk_attn_enc_t* enc, wtk_source_t* src, int bin);
#ifdef __cplusplus
};
#endif

#endif /* QTK_VITS_QTK_VITS_ATTN_H_ */
