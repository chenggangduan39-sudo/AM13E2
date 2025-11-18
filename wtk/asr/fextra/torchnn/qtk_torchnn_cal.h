#ifndef QTK_ASR_TORCH_NN_CAL
#define QTK_ASR_TORCH_NN_CAL
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/wtk_strbuf.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	qtk_blas_matrix_t *weight;
	qtk_blas_matrix_t *bias;
	qtk_blas_matrix_t *trans;
	int i;
	int j;
	int kernel_row;
	int kernel_col;
	int padding_row;
	int padding_col;
	int stride1;
	int stride2;
	int dilation1;
	int dilation2;
}qtk_torchnn_conv2d_t;

qtk_torchnn_conv2d_t* qtk_torchnn_conv2d_new(int ker_row,int ker_col);
void qtk_torchnn_conv2d_delete(qtk_torchnn_conv2d_t *conv);
int qtk_torchnn_conv2d_write(void *mdl,FILE *f);
qtk_torchnn_conv2d_t* qtk_torchnn_conv2d_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
qtk_torchnn_conv2d_t* qtk_torchnn_conv2d_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_conv2d_cal(qtk_torchnn_conv2d_t *conv, qtk_blas_matrix_t *input, qtk_blas_matrix_t *output,int batch_row, int batch_col);

typedef struct
{
	qtk_blas_matrix_t *ih_w;
	qtk_blas_matrix_t *hh_w;
	qtk_blas_matrix_t *ih_b;
	qtk_blas_matrix_t *hh_b;
	qtk_blas_matrix_t *ht;
	qtk_blas_matrix_t *ct;
}qtk_torchnn_lstm_t;//TODO

qtk_torchnn_lstm_t* qtk_torchnn_lstm_new(void);
void qtk_torchnn_lstm_delete(qtk_torchnn_lstm_t *lstm);
int qtk_torchnn_lstm_write(void *mdl,FILE *f);
qtk_torchnn_lstm_t* qtk_torchnn_lstm_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_lstm_cal(qtk_torchnn_lstm_t *lstm,qtk_blas_matrix_t *input,qtk_blas_matrix_t *c1,qtk_blas_matrix_t *h1,
		qtk_blas_matrix_t *c2,qtk_blas_matrix_t *h2);

typedef struct
{
	int kernel_size;
	int stride;
}qtk_torchnn_maxpool_t;

qtk_torchnn_maxpool_t* qtk_torchnn_maxpool_new(void);
void qtk_torchnn_maxpool_delete(qtk_torchnn_maxpool_t *pool);
int qtk_torchnn_maxpool_write(void *mdl,FILE *f);
qtk_torchnn_maxpool_t* qtk_torchnn_maxpool_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_maxpool_cal(qtk_torchnn_maxpool_t *pool,qtk_blas_matrix_t *input,qtk_blas_matrix_t *output,int batch_row);

typedef struct
{
	int kernel_row;
	int kernel_col;
	int stride_row;
	int stride_col;
}qtk_torchnn_avpool_t;

qtk_torchnn_avpool_t* qtk_torchnn_avpool_new(void);
void qtk_torchnn_avpool_delete(qtk_torchnn_avpool_t *pool);
int qtk_torchnn_avpool_write(void *mdl,FILE *f);
qtk_torchnn_avpool_t* qtk_torchnn_avpool_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_avpool_cal(qtk_torchnn_avpool_t *pool,qtk_blas_matrix_t *input,qtk_blas_matrix_t *output);

void qtk_torchnn_mean(qtk_blas_matrix_t *input,qtk_blas_matrix_t *output);

typedef struct
{
	qtk_blas_matrix_t *weight;
	qtk_blas_matrix_t *bias;
}qtk_torchnn_linear_t;

qtk_torchnn_linear_t* qtk_torchnn_linear_new(void);
void qtk_torchnn_linear_delete(qtk_torchnn_linear_t *linear);
int qtk_torchnn_linear_write(void *mdl,FILE *f);
qtk_torchnn_linear_t* qtk_torchnn_linear_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
qtk_torchnn_linear_t* qtk_torchnn_linear_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_linear_cal(qtk_torchnn_linear_t *linear,qtk_blas_matrix_t *input,qtk_blas_matrix_t *output);

typedef struct
{
	qtk_blas_matrix_t *weight;
}qtk_torchnn_embed_t;

qtk_torchnn_embed_t* qtk_torchnn_embed_new(void);
void qtk_torchnn_embed_delete(qtk_torchnn_embed_t *embed);
int qtk_torchnn_embed_write(void *mdl,FILE *f);
qtk_torchnn_embed_t* qtk_torchnn_embed_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
qtk_torchnn_embed_t* qtk_torchnn_embed_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_embed_cal(qtk_torchnn_embed_t *embed,qtk_blas_matrix_t *output,int index);

typedef struct
{
	qtk_blas_matrix_t *weight;
	qtk_blas_matrix_t *weight_t;
	qtk_blas_matrix_t *bias;
	int kernel_row;
	int padding_row;
	int stride1;
	int dilation1;
	int i;
	int j;
}qtk_torchnn_conv1d_t;

qtk_torchnn_conv1d_t* qtk_torchnn_conv1d_new(int ker_row);
void qtk_torchnn_conv1d_delete(qtk_torchnn_conv1d_t *conv);
int qtk_torchnn_conv1d_write(void *mdl,FILE *f);
qtk_torchnn_conv1d_t* qtk_torchnn_conv1d_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_conv1d_cal(qtk_torchnn_conv1d_t *conv, qtk_blas_matrix_t *input, qtk_blas_matrix_t *output);

typedef struct
{
	float *weight;
	float *bias;
	float *mean;
	float *var;
	qtk_blas_matrix_t *scale;
	qtk_blas_matrix_t *offset;

}qtk_torchnn_batch_t;

qtk_torchnn_batch_t* qtk_torchnn_batch_new(void);
void qtk_torchnn_batch_delete(qtk_torchnn_batch_t *batch);
int qtk_torchnn_batch_write(void *mdl,FILE *f);
qtk_torchnn_batch_t* qtk_torchnn_batch_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
qtk_torchnn_batch_t* qtk_torchnn_batch_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin);
void qtk_torchnn_batch_cal(qtk_torchnn_batch_t *batch, qtk_blas_matrix_t *in);

#ifdef __cplusplus
};
#endif
#endif
