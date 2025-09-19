#ifndef WTK_COSYNTHESIS_BACKEND
#define WTK_COSYNTHESIS_BACKEND
#include "wtk/core/wtk_type.h" 
#include "wtk_cosynthesis_backend_cfg.h"
#include "wtk_cosynthesis_lc.h"
#include "wtk/tts/cosynthesis/front_end/interface/ais_hts_engine.h"
#include "wtk_cosynthesis_lexicon_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cosynthesis_backend wtk_cosynthesis_backend_t;


struct wtk_cosynthesis_backend
{
	wtk_cosynthesis_backend_cfg_t *cfg;
	wtk_heap_t *heap;
	wtk_heap_t *glb_heap;
	wtk_cosynthesis_lc_t *lc;
	wtk_cosynthesis_qs_item_t *fixf0_item;
	int snt_idx;
	wtk_queue_node_t *syn_cur;
	wtk_queue_node_t *syn_nxt;
	wtk_strbuf_t *buf;
};


wtk_cosynthesis_backend_t* wtk_cosynthesis_backend_new(wtk_cosynthesis_backend_cfg_t *cfg);
void wtk_cosynthesis_backend_delete(wtk_cosynthesis_backend_t *s);
void wtk_cosynthesis_backend_reset(wtk_cosynthesis_backend_t *s);
int wtk_cosynthesis_backend_process(wtk_cosynthesis_backend_t *s,char *flab,int len,wtk_queue_t *q);
void wtk_cosynthesis_backend_get_candi_unit_dur(wtk_cosynthesis_backend_t *s,float* dur_mean,float* dur_var, int* idx,int nphone);
void wtk_cosynthesis_backend_get_candi_unit_lf0(wtk_cosynthesis_backend_t *s,float* lf0_mean,float* lf0_var, float* lf0_weight, int state,int* idx,int nphone);
void wtk_cosynthesis_backend_get_candi_unit_mcep(wtk_cosynthesis_backend_t *s,float* mcep_mean,float* mcep_var,int state,int* idx,int nphone);


void wtk_cosynthesis_mdl_data_new(float **dur_mean,float **dur_var,float**lf0_mean,
float **lf0_var,float **lf0_weight,float **mcep_mean,float **mcep_var,
float **conca_lf0_mean,float **conca_lf0_var,float **conca_lf0_weight,float **conca_mcep_mean,
float **conca_mcep_var,int nphone,wtk_cosynthesis_feat_cfg_t *cfg);
void wtk_cosynthesis_mdl_data_new2(float **dur_mean,float **dur_var,float**lf0_mean,
float **lf0_var,float **lf0_weight,float **mcep_mean,float **mcep_var,int nphone,wtk_cosynthesis_feat_cfg_t *cfg);
void wtk_cosynthesis_mdl_data_delete(float *dur_mean,float *dur_var,float *lf0_mean,
float *lf0_var,float *lf0_weight,float *mcep_mean,float *mcep_var,
float *conca_mcep_mean,float *conca_mcep_var,float *conca_lf0_mean,float *conca_lf0_var,float *conca_lf0_weight);


#ifdef __cplusplus
};
#endif
#endif
