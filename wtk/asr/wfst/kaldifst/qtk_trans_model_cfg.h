#ifndef QTK_TRANS_MODEL_CFG_H_
#define QTK_TRANS_MODEL_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_trans_model_cfg qtk_trans_model_cfg_t;
typedef struct qtk_trans_triples qtk_trans_triples_t;
typedef struct qtk_trans_tuples qtk_trans_tuples_t;
typedef struct qtk_trans_phone qtk_trans_phone_t;
typedef struct qtk_trans_model qtk_trans_model_t;
typedef struct qtk_trans_topology_entry qtk_trans_topology_entry_t;

struct qtk_trans_topology_entry
{
	int state_cnt;
	int *state_trans;
};

struct qtk_trans_triples
{
	wtk_queue_node_t q_n;
	int phone_id;
	int pdf_id;
	int transition_id;
	int repeat;
};

struct qtk_trans_tuples
{
	wtk_queue_node_t q_n;
	int phone_id;
	int pdf_id;
	int forward_trans;
	int loop_trans;
};

 struct qtk_trans_phone
{
	int phone_id;
	int entry_id;
};

struct qtk_trans_model
{
	qtk_trans_topology_entry_t *entries;//for hmm entry
	qtk_trans_phone_t *phones;//give phone number get trans entry
	wtk_queue_t triple_q;//for triples
	wtk_queue_t tuple_q;//for tuples
	int* id2pdf_id_;//give in label get dnn
	int* id2phone_id_;//give in label get phone
	int* id2forword;//for chain mdl
	int* id2loop;//for chain mdl
	float *logprob;//not use currenty
	int nentries;
	int trans_num;
};

struct qtk_trans_model_cfg
{
	char* trans_model_fn;
	qtk_trans_model_t* trans_model;
	unsigned use_chain;//
};

int qtk_trans_model_cfg_init(qtk_trans_model_cfg_t *cfg);
int qtk_trans_model_cfg_clean(qtk_trans_model_cfg_t *cfg,int use_chain);
int qtk_trans_model_cfg_update_local(qtk_trans_model_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_trans_model_cfg_update(qtk_trans_model_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_trans_model_load_chain(qtk_trans_model_cfg_t *cfg,wtk_source_t *src);
int qtk_trans_model_load_normal(qtk_trans_model_cfg_t *cfg, wtk_source_t *src);
void qtk_trans_model_cfg_print(qtk_trans_model_cfg_t *cfg);
int qtk_trans_model_cal_id2pdf_chain(qtk_trans_model_t* t_model);
qtk_trans_model_t* qtk_trans_model_new(void);

#ifdef __cplusplus
};
#endif
#endif
