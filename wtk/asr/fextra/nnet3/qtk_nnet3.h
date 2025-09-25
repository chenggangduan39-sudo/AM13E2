#ifndef QTK_NNET3_H_
#define QTK_NNET3_H_
#include "qtk_nnet3_cfg.h"
#include "qtk_nnet3_component.h"
#include "qtk_nnet3_compution.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/asr/fextra/kparm/wtk_kfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_nnet3 qtk_nnet3_t;

typedef void (*qtk_nnet3_feature_notify_f)(void *ths,qtk_blas_matrix_t *f,int end, int plus);

typedef struct QtkNnet3LayerInfo {
    unsigned int LayerId;
    nnet3_component_t *LayerPoint;
} QtkNnet3LayerInfo_t;

struct qtk_nnet3
{
	qtk_nnet3_cfg_t *cfg;
	void *notify_ths;
	qtk_nnet3_feature_notify_f notify;
	wtk_feat_t *cur_feat;
	wtk_kfeat_t *cur_kfeat;
	qtk_blas_matrix_t **gc;
        qtk_blas_matrix_t *m[1000];
        float *ivector;
        int ivector_dim;
	int use_ivector;
	int *input_index;
	int left;
	int right;
	int frames_per_chunk;
	int program_counter;
	int begin_input_frame;
	int end_input_frame;
	int chunk_counter;
	int row;
	int irow;
	int ac_index;
	int hasac;
	int gc_count;
	int input_num;
	int input_col;
	int chns;
	int delta;
	int extra;
	int output_frames;
	int need_out;
        QtkNnet3LayerInfo_t **LayerInfo;
};

qtk_nnet3_t* qtk_nnet3_new(qtk_nnet3_cfg_t* cfg, int col,int use_ivec);
void qtk_nnet3_delete(qtk_nnet3_t* nnet3);
void qtk_nnet3_reset(qtk_nnet3_t* nnet3);
void qtk_nnet3_run(qtk_nnet3_t* nnet3,wtk_feat_t *feature,int end);
void qtk_nnet3_run_kfeat(qtk_nnet3_t* nnet3,wtk_kfeat_t *feature,int end);
void qtk_nnet3_set_notify(qtk_nnet3_t* nnet3,qtk_nnet3_feature_notify_f notify,void*  notify_ths);
void qtk_nnet3_execute_command(qtk_nnet3_t* nnet3, qtk_nnet3_command_t* command);
void qtk_nnet3_del(qtk_nnet3_t* nnet3);
#ifdef __cplusplus
};
#endif
#endif
