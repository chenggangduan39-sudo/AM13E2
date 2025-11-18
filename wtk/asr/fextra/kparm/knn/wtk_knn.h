#ifndef WTK_KSR_NNET_WTK_KNN
#define WTK_KSR_NNET_WTK_KNN
#include "wtk/core/wtk_type.h" 
#include "wtk_knn_cfg.h"
#include "wtk/asr/fextra/kparm/wtk_kfeat.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_hoard.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_knn wtk_knn_t;

typedef void(*wtk_knn_notify_f)(void *ths,wtk_kfeat_t *feat);

typedef struct wtk_knn_rte_layer wtk_knn_rte_layer_t;

typedef struct
{
	int index;
	int valid;
	union
	{
		float *f;
		int *i;
		short *s;
	}v;
}wtk_knn_feat_t;

struct wtk_knn_rte_layer
{
	wtk_vecf_t *input;
	wtk_vecf_t *output;

	wtk_veci_t *input_i;
	wtk_vecs2_t *input_s;
	wtk_vecs2_t *output_s;
	wtk_knn_logic_layer_t *layer;
	wtk_robin_t *tdnn_robin;
	int skip_idx;
	int skip;
	int input_idx;
};

typedef struct
{
	//wtk_vecf_t *input;
	wtk_knn_rte_layer_t **layers;
	int nlayer;
}wtk_knn_rte_t;

struct wtk_knn
{
	wtk_knn_cfg_t *cfg;
	wtk_robin_t *rb;
	wtk_knn_rte_t *rte;
	wtk_knn_notify_f notify;
	void *ths;
	wtk_vecf_t *input;
	int nframe;
	int oframe;
	int use_skip:1;
	int skip;
	int shift;
	char* name;
	unsigned char flag;
	int pooling_nframe;
	int pooling_layer_index;
};

int wtk_knn_bytes(wtk_knn_t *knn);
wtk_knn_t* wtk_knn_new(wtk_knn_cfg_t *cfg);
void wtk_knn_delete(wtk_knn_t *knn);
void wtk_knn_set_notify(wtk_knn_t *knn,void *ths,wtk_knn_notify_f notify);
void wtk_knn_reset(wtk_knn_t *knn);
void wtk_knn_fead(wtk_knn_t *knn,float *feat,int index);

void wtk_knn_fead_fix(wtk_knn_t *knn,int *feat,int index);

float wtk_knn_get_prob(wtk_knn_t *knn,int id);
void wtk_knn_flush(wtk_knn_t *knn);
void wtk_knn_flush_pooling(wtk_knn_t *knn);

#ifdef __cplusplus
};
#endif
#endif
