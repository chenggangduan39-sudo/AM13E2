#ifndef QTK_ASR_TORCH_NN
#define QTK_ASR_TORCH_NN
/* torch nn wrapper
 * prepare input ; call torchnn_cal do the calculation
 * writen by madch*/
#include "qtk_torchnn_cfg.h"
#include "qtk_torchnn_cal.h"
#include "wtk/core/wtk_robin.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_torchnn qtk_torchnn_t;
typedef void(*qtk_torchnn_notify_f)(void *ths,qtk_blas_matrix_t *m);
typedef void(*qtk_torchnn_notify_end_f)(void *ths);

typedef enum
{
	QTK_TORCHNN_CONV2D,
	QTK_TORCHNN_MAXPOOL,
	QTK_TORCHNN_LSTM,
	QTK_TORCHNN_LINEAR,
	QTK_TORCHNN_EMBED,
	QTK_TORCHNN_ACTIVATE,
	QTK_TORCHNN_CONV1D,
	QTK_TORCHNN_BATCH,
	QTK_TORCHNN_AVPOOL,
	QTK_TORCHNN_MEAN,
	QTK_TORCHNN_SE,//se layer, little torchnn inside
	QTK_TORCHNN_BASIC//basic block for res net    x + torchnn(x)/ conv(x) + torchnn(x)
}qtk_torchnn_layer_type_t;

typedef enum
{
	QTK_SIGMOID,
	QTK_TANH,
	QTK_LOGSOFTMAX,
	QTK_SOFTMAX,
	QTK_RELU
}qtk_torchnn_activate_type_t;

typedef struct
{
	qtk_blas_matrix_t *c1;
	qtk_blas_matrix_t *c2;
	qtk_blas_matrix_t *h1;
	qtk_blas_matrix_t *h2;
}qtk_torchnn_lstm_assit_t;

typedef struct
{
	qtk_blas_matrix_t *input;
	qtk_blas_matrix_t *output;
	qtk_torchnn_lstm_assit_t *lstma;
	qtk_torchnn_layer_type_t type;
	qtk_torchnn_activate_type_t a_type;
	void *mdl;
	wtk_robin_t *rb;
	int input_row;
	int input_col;
	int skip;//only for conv now
	int skip_cnt;
	int has_output;
}qtk_torchnn_layer_t;


struct qtk_torchnn
{
	qtk_torchnn_layer_t **layers;
	qtk_torchnn_notify_f notify;
	qtk_torchnn_notify_end_f notify_end;
	void *ths;
	int nlayer;
	int output_row;
	int output_col;
};

qtk_torchnn_t* qtk_torchnn_new(int n);
qtk_torchnn_layer_t* qtk_torchnn_layer_new(void);
qtk_torchnn_t* qtk_torchnn_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin,int feat_col,int feat_row);
qtk_torchnn_t* qtk_torchnn_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin,int feat_col,int feat_row);
void qtk_torchnn_delete(qtk_torchnn_t *torchnn);
void qtk_torchnn_reset(qtk_torchnn_t *torchnn);
void qtk_torchnn_flush(qtk_torchnn_t *torchnn);
qtk_blas_matrix_t* qtk_torchnn_feed(qtk_torchnn_t *torchnn,float *f,int col,int row,int layer_index);

void qtk_torchnn_set_notify(qtk_torchnn_t *torchnn,void *ths,qtk_torchnn_notify_f notify);
void qtk_torchnn_set_notify_end(qtk_torchnn_t *torchnn,qtk_torchnn_notify_end_f notify);

typedef struct
{
	qtk_torchnn_t *mdl;//torch nn
	qtk_blas_matrix_t *input;
	int index;
	int moving_avg;
	unsigned int online : 1;
}qtk_torchnn_se_t;//se
qtk_torchnn_se_t* qtk_torchnn_se_new(void);
void qtk_torchnn_se_delete(qtk_torchnn_se_t *se);
int qtk_torchnn_se_write(void *mdl,FILE *f);
qtk_torchnn_se_t* qtk_torchnn_se_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin, int length);
void qtk_torchnn_se_cal(qtk_torchnn_se_t *se, qtk_blas_matrix_t *in);

typedef struct
{
	qtk_torchnn_t *mdl;//torch nn
	qtk_torchnn_t *mdl_down;//for downsample
	wtk_robin_t *cache_rb;//cache input blas for not downsample
	qtk_blas_matrix_t *mat;
	int downsample;
}qtk_torchnn_block_t;//basic block
qtk_torchnn_block_t* qtk_torchnn_block_new(void);
void qtk_torchnn_block_delete(qtk_torchnn_block_t *se);
int qtk_torchnn_block_write(void *mdl,FILE *f);
qtk_torchnn_block_t* qtk_torchnn_block_read_bin(wtk_source_t *src,wtk_strbuf_t *buf,int bin, int length, int row);
qtk_torchnn_block_t* qtk_torchnn_block_read(wtk_source_t *src,wtk_strbuf_t *buf,int bin, int length, int row);
void qtk_torchnn_block_cal(qtk_torchnn_block_t *se, qtk_blas_matrix_t *in);

int qtk_torchnn_write(qtk_torchnn_t *mdl, FILE *f);
#ifdef __cplusplus
};
#endif
#endif
