#ifndef WTK_KWDEC2_TRANS_MODEL_CFG_H_
#define WTK_KWDEC2_TRANS_MODEL_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_source.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kwdec2_trans_model_cfg wtk_kwdec2_trans_model_cfg_t;
typedef struct wtk_kwdec2_trans_triples wtk_kwdec2_trans_triples_t;
typedef struct wtk_kwdec2_trans_tuples wtk_kwdec2_trans_tuples_t;
typedef struct wtk_kwdec2_trans_phone wtk_kwdec2_trans_phone_t;
typedef struct wtk_kwdec2_trans_model wtk_kwdec2_trans_model_t;
typedef struct wtk_kwdec2_trans_topology_entry wtk_kwdec2_trans_topology_entry_t;

struct wtk_kwdec2_trans_topology_entry
{
	wtk_queue_node_t q_n;
	int state_cnt;
	int *state_trans;
};

struct wtk_kwdec2_trans_triples
{
	wtk_queue_node_t q_n;
	int phone_id;
	int pdf_id;
	int transition_id;
};

struct wtk_kwdec2_trans_tuples
{
	wtk_queue_node_t q_n;
	int phone_id;
	int pdf_id;
	int forward_trans;
	int loop_trans;
};

 struct wtk_kwdec2_trans_phone
{
	wtk_queue_node_t q_n;
	int phone_id;
	wtk_kwdec2_trans_topology_entry_t* t_entry;
};

struct wtk_kwdec2_trans_model
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

struct wtk_kwdec2_trans_model_cfg
{
	char* trans_model_fn;
	wtk_kwdec2_trans_model_t* trans_model;
	unsigned use_chain;//
};

int wtk_kwdec2_trans_model_cfg_init(wtk_kwdec2_trans_model_cfg_t *cfg);
int wtk_kwdec2_trans_model_cfg_clean(wtk_kwdec2_trans_model_cfg_t *cfg,int use_chain);
int wtk_kwdec2_trans_model_cfg_update_local(wtk_kwdec2_trans_model_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kwdec2_trans_model_cfg_update(wtk_kwdec2_trans_model_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_kwdec2_trans_model_load_chain(wtk_kwdec2_trans_model_cfg_t *cfg,wtk_source_t *src);
void wtk_kwdec2_trans_model_cfg_print(wtk_kwdec2_trans_model_cfg_t *cfg);
int wtk_kwdec2_trans_model_cal_id2pdf_chain(wtk_kwdec2_trans_model_t* t_model);
wtk_kwdec2_trans_model_t* wtk_kwdec2_trans_model_new();
int wtk_kwdec2_trans_model_load_normal(wtk_kwdec2_trans_model_cfg_t *cfg, wtk_source_t *src);

#ifdef __cplusplus
};
#endif
#endif
