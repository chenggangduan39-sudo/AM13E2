/*
 * qtk_vits_modules.h
 *
 *  Created on: Aug 25, 2022
 *      Author: dm
 */

#ifndef QTK_VITS_QTK_VITS_MODULES_H_
#define QTK_VITS_QTK_VITS_MODULES_H_
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_vits_modules_ewaffine qtk_vits_modules_ewaffine_t;
typedef struct qtk_vits_modules_ddsconv qtk_vits_modules_ddsconv_t;
typedef struct qtk_vits_modules_resclayer qtk_vits_modules_resclayer_t;
typedef struct qtk_syntrn_flip qtk_syntrn_flip_t;

//class modules.LayerNorm
typedef struct{
	void* channels;
	float* eps;
	float* gamma;
	float* beta;
	int dim;
}qtk_vits_modules_normlayer_t;

qtk_vits_modules_normlayer_t* qtk_vits_modules_normlayer_new(qtk_vits_cfg_t *cfg);
void qtk_vits_modules_normlayer_delete(qtk_vits_modules_normlayer_t* l);
qtk_vits_modules_normlayer_t* qtk_vits_modules_normlayer_read(qtk_vits_modules_normlayer_t* l, qtk_vits_cfg_t* cfg, wtk_source_t* src, int bin);


// class modules.ResBlock1
// class modlues.ResBlock2
typedef struct{
	qtk_torchnn_wnorm_t* convs;  //nn.ModuleList  //ResBlock2
	qtk_torchnn_wnorm_t* convs1;  //nn.ModuleList //ResBlock1
	qtk_torchnn_wnorm_t* convs2;  //nn.ModuleList //ResBlock1
	int nconvs;
	int nconvs1;
	int nconvs2;
}qtk_syntrn_resblocks_t;

qtk_syntrn_resblocks_t* qtk_syntrn_resblocks_new(qtk_vits_cfg_t* cfg);
void qtk_syntrn_resblocks_delete(qtk_syntrn_resblocks_t* rb);
qtk_syntrn_resblocks_t* qtk_syntrn_resblocks_read(qtk_syntrn_resblocks_t* rb, qtk_vits_cfg_t* cfg, wtk_source_t* src, int bin);

//class modules.WN
typedef struct{
//	void* hidden_channels;
//	void* kernel_size;
//	void* dilation_rate;
	int n_layers;
//	int gin_channels;
//	int p_dropout;

	qtk_torchnn_wnorm_t* inlayers;
	qtk_torchnn_wnorm_t* res_skip_layers;
//	qtk_torchnn_dropout_t* drop;
//	qtk_torchnn_wnorm_t* cond_layer;
}qtk_syntrn_modules_wn_t;

qtk_syntrn_modules_wn_t* qtk_syntrn_modules_wn_new();
void qtk_syntrn_modules_wn_delete(qtk_syntrn_modules_wn_t* wn);
qtk_syntrn_modules_wn_t* qtk_syntrn_modules_wn_read(wtk_source_t* src, int bin);

//class modules.ResidualCouplingLayer
struct qtk_vits_modules_resclayer{
	void* channels;
	void* hidden_channels;
	void* kernel_size;
	void* dilation_rate;
	int n_layers;
	int half_channels;
	int mean_only;

	qtk_torchnn_conv1d_t* pre;
	qtk_syntrn_wn_t* enc;
	qtk_torchnn_conv1d_t* post;
};

// class modules.Flip
struct qtk_syntrn_flip{
	float** m;
	float** logs;
};

//class modules.DDSConv
struct qtk_vits_modules_ddsconv{
	void* channels;
	void* kernel_size;
	int n_layers;
	float p_dropout;

	qtk_torchnn_dropout_t* drop;
//	qtk_syntrn_conv_t* pre;
	qtk_torchnn_conv1d_t* convs_sep;
	qtk_torchnn_conv1d_t* convs_1x1;
	qtk_vits_modules_normlayer_t* norms_1;
	qtk_vits_modules_normlayer_t* norms_2;
};

//class modules.ElementwiseAffine
struct qtk_vits_modules_ewaffine{
	void* channels;
	qtk_torchnn_param_t** m;
	qtk_torchnn_param_t** logs;
};

struct qtk_syntrn_modulelist{
	union{
		qtk_syntrn_resclayer_t* resclayer;
		qtk_syntrn_flip_t* flip;
		qtk_torchnn_weight_norm_t* wn;
		qtk_vits_modules_ewaffine_t* ewa;
	}m;
};

#ifdef __cplusplus
};
#endif
#endif /* QTK_VITS_QTK_VITS_MODULES_H_ */
