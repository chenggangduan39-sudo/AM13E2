#ifndef WTK_TAC_COMMON_H
#define WTK_TAC_COMMON_H
#include <regex.h>
#include <stdlib.h>
#include "wtk/tts-mer/wtk-extend/wtk_extend.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct 
{
    // tacotron
    // float tac_zoneout_rate; // zoneout rate for all LSTM cells in the network
    // float tac_dropout_rate; //dropout rate for all convolutional layers + prenet
    char *batch_norm_position; //Can be in ('before', 'after'). Determines whether we use batch norm before or after the activation function (relu). Matter for debate.
    int clip_output; // Whether to clip spectrograms to T2_output_range (even in loss computation). ie: Don't penalize model for exceeding output range and bring back to borders.
    int signal_norm;
    int symmetric_mels;
    int allow_clip_in_norm;

    // input embedding_dim
    int embedding_symbols; //tac2 embedding symbols len
    int embedding_dim; // embedding_dim
    
    // encoder
    int enc_conv_kernel_size;  //size of encoder convolution filters for each layer
    int enc_conv_num_layers; //number of encoder convolutional layers
    int enc_conv_channel; //number of encoder convolutions filters for each layer
    int enc_lstm_units; // number of lstm units for each direction (forward and backward)
    
    // decoder
    int *dec_prenet_layer; // #number of layers and number of units of prenet
    int dec_layers; // #number of decoder lstm layers
    int dec_lstm_units; // number of decoder lstm units on each layer

    // Attention mechanism
    int atten_dim; //dimension of attention space
    int atten_filter; // number of attention convolution filters
    int atten_kernel; // kernel size of attention convolution
    int atten_per_step; // number of frames to generate at each decoding step (increase to speed up computation and allows for higher batch size, decreases G&L audio quality)

    // Residual postnet
    int postnet_num_layer; // number of postnet convolutional layers
    int postnet_kernel_size; // size of postnet convolution kernels for each layer
    int postnet_channel; // number of postnet convolution filters for each layer

    // CBHG mel->linear postnet
    int cbhg_kernel; // All kernel sizes from 1 to cbhg_kernels will be used in the convolution bank of CBHG to act as "K-grams"
    int cbhg_conv_channel; // Channels of the convolution bank
    int cbhg_pool_size; // pooling size of the CBHG
    int cbhg_projection; // projection channels of the CBHG (1st projection, 2nd is automatically set to num_mels)
    int cbhg_projection_kernel_size; // kernel_size of the CBHG projections
    int cbhg_highwaynet_layers; // Number of HighwayNet layers
    int cbhg_highway_units; // Number of units used in HighwayNet fully connected layers
    int cbhg_rnn_units; //Number of GRU units used in bidirectional RNN of CBHG block. CBHG output is 2x rnn_units in shape

    // audio
    int num_mels; // Number of mel-spectrogram channels and local conditioning dimensionality
    int num_freq; // (= n_fft / 2 + 1) only used when adding linear spectrograms post processing network
    float max_abs_value; // max absolute value of data. If symmetric, data will be [-max, max] else [0, max] (Must not be too big to avoid gradient explosion
    // signal_normalization
    float lower_bound_decay; // Small regularizer for noise synthesis by adding small range of penalty for silence regions. Set to 0 to clip in Tacotron range.
    
    // Mel spectrogram
    float magnitude_power; // The power of the spectrogram magnitude (1. for energy, 2. for power)
    int sample_rate;
    int hop_size; // For 22050Hz, 275 ~= 12.5 ms (0.0125 * sample_rate)
    int win_size; // For 22050Hz, 1100 ~= 50 ms (If None, win_size = n_fft) (0.05 * sample_rate)
    // limit
	float min_level_db;
	float ref_level_db;
    
    // Griffin Lim
    float power; // Only used in G&L inversion, usually values between 1.2 and 1.5 are a good choice.
    int griffin_lim_iters; // Number of G&L iterations, typically 30 is enough but we use 60 to ensure convergence.
    float preemphasis; // whether to apply filter

    // wavrnn cfg
    int wavrnn_pad;
    int wavrnn_res_compute_dim;
    int wavrnn_res_in_kernel_size;
    int wavrnn_res_out_kernel_size;
    int wavrnn_kernel_block_size;
    int wavrnn_res_out_dim;
    int wavrnn_aux_dim;
    int wavrnn_num_res_block;
    int wavrnn_rnn_dim;
    int wavrnn_gru_layers;
    int wavrnn_fc_dim;
    int wavrnn_num_fc;
    int wavrnn_bit;
    int wavrnn_hop_size;
    int wavrnn_is_model_small;
    int wavrnn_is_model_multiband;
    int wavrnn_fc_band_dim;
    int wavrnn_fc_band_num;
} wtk_tac_hparams_t;

#ifdef __cplusplus
}
#endif
#endif