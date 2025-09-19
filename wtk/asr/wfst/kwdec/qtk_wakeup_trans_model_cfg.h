#ifndef QTK_WAKEUP_TRANS_MODEL_CFG_H_
#define QTK_WAKEUP_TRANS_MODEL_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_wakeup_trans_model_cfg qtk_wakeup_trans_model_cfg_t;
typedef struct qtk_wakeup_trans_triples qtk_wakeup_trans_triples_t;
typedef struct qtk_wakeup_trans_tuples qtk_wakeup_trans_tuples_t;
typedef struct qtk_wakeup_trans_phone qtk_wakeup_trans_phone_t;
typedef struct qtk_wakeup_trans_model qtk_wakeup_trans_model_t;
typedef struct qtk_wakeup_trans_topology_entry qtk_wakeup_trans_topology_entry_t;

struct qtk_wakeup_trans_topology_entry
{
	wtk_queue_node_t q_n;
	int state_cnt;
	int *state_trans;
};

struct qtk_wakeup_trans_triples
{
	wtk_queue_node_t q_n;
	int phone_id;
	int pdf_id;
	int transition_id;
};

struct qtk_wakeup_trans_tuples
{
	wtk_queue_node_t q_n;
	int phone_id;
	int pdf_id;
	int forward_trans;
	int loop_trans;
};

 struct qtk_wakeup_trans_phone
{
	wtk_queue_node_t q_n;
	int phone_id;
	qtk_wakeup_trans_topology_entry_t* t_entry;
};

struct qtk_wakeup_trans_model
{
	wtk_queue_t entry_q;//for hmm entry
	wtk_queue_t triple_q;//for triples
	wtk_queue_t tuple_q;//for tuples
	wtk_queue_t phone_q;//give phone number get trans entry
	int* id2pdf_id_;//give in label get dnn
	int* id2forword;//for chain mdl
	int* id2loop;//for chain mdl
	float *logprob;//not use currenty
    int* id2phone_id_; //inid to phone
    int** phone2pdf_id_; //phoneid to pdfid
    int phone_tot_cnt;//total phoneid count
    int* phone_cnt; //each phone count
};

struct qtk_wakeup_trans_model_cfg
{
	char* trans_model_fn;
	qtk_wakeup_trans_model_t* trans_model;
	unsigned use_chain;//
};

int qtk_wakeup_trans_model_cfg_init(qtk_wakeup_trans_model_cfg_t *cfg);
int qtk_wakeup_trans_model_cfg_clean(qtk_wakeup_trans_model_cfg_t *cfg,int use_chain);
int qtk_wakeup_trans_model_cfg_update_local(qtk_wakeup_trans_model_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_wakeup_trans_model_cfg_update(qtk_wakeup_trans_model_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_wakeup_trans_model_load_chain(qtk_wakeup_trans_model_cfg_t *cfg,wtk_source_t *src);
void qtk_wakeup_trans_model_cfg_print(qtk_wakeup_trans_model_cfg_t *cfg);
int qtk_wakeup_trans_model_cal_id2pdf_chain(qtk_wakeup_trans_model_t* t_model);
qtk_wakeup_trans_model_t* qtk_wakeup_trans_model_new(void);
int qtk_wakeup_trans_model_load_normal(qtk_wakeup_trans_model_cfg_t *cfg, wtk_source_t *src);

#ifdef __cplusplus
};
#endif
#endif
