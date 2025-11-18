
#ifndef _WTK_SVPRINT_H_
#define _WTK_SVPRINT_H_
#include "wtk_svprint_cfg.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/core/wtk_larray.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*wtk_svprint_score_notify)(void *ths,wtk_string_t *name,float score);

typedef struct wtk_svprint wtk_svprint_t;

struct wtk_svprint
{
    wtk_svprint_cfg_t *cfg;
    wtk_ivector_plda_scoring_t *scoring;
    wtk_nnet3_xvector_compute_t *x;
    wtk_vad_t *v;
    wtk_queue_t vad_q;
    wtk_queue_t feat_q;
    wtk_queue_t enroll_q;
    wtk_robin_t *vrobin;
    wtk_larray_t *state_l;
    wtk_string_t **names;//max 60
    wtk_string_t **nbest_res;
    float score[4];
    int res_cnt;
    int max_spk;
    int spk_cnt;
    int vad_pop_index;
    int feat_push_index;
    float log_energy;
    wtk_svprint_score_notify notify_f;
    void *ths;
    unsigned vad_state:1;
    unsigned enroll_available:1;
    unsigned eval_available:1;
};

void wtk_svprint_set_nnet3_xvec_notify(wtk_svprint_t *v);
wtk_svprint_t* wtk_svprint_new(wtk_svprint_cfg_t *cfg);
void  wtk_svprint_delete(wtk_svprint_t *v);
int wtk_svprint_start(wtk_svprint_t *v);
void wtk_svprint_reset(wtk_svprint_t *v);
void wtk_svprint_clean(wtk_svprint_t *v);//remove all enroll file
void wtk_svprint_reload(wtk_svprint_t *v);//reload enroll resource
int wtk_svprint_feed(wtk_svprint_t *v,short *data,int len,int is_end);
int wtk_svprint_enrollvec(wtk_svprint_t *v,wtk_string_t *name,wtk_vecf_t *vec);
int wtk_svprint_enroll2mem(wtk_svprint_t *v,wtk_string_t *name);
int wtk_sprint_dump_memory2file(wtk_svprint_t *v);
int wtk_svprint_enroll2file(wtk_svprint_t *v,wtk_string_t *name);
wtk_vecf_t* wtk_svprint_compute_feat(wtk_svprint_t *v);
wtk_vecf_t* wtk_svprint_compute_feat2(wtk_svprint_t *v,int *spk_cnt);
wtk_string_t* wtk_svprint_feat_likelihood(wtk_svprint_t *v,wtk_vecf_t *feat,float *prob);
wtk_string_t* wtk_svprint_feat_likelihood_plda(wtk_svprint_t *v,wtk_vecf_t *feat,float *prob);
wtk_string_t* wtk_svprint_eval(wtk_svprint_t *v,float *prob);
wtk_string_t* wtk_svprint_eval2(wtk_svprint_t *v, wtk_vecf_t *vec, wtk_vecf_t *dst, int cnt, float *prob);
void wtk_svprint_eval_Nbest(wtk_svprint_t *v, wtk_vecf_t *vec, wtk_vecf_t *dst, int cnt, float *prob);
wtk_svprint_cfg_feat_node_t* wtk_svprint_query_memory_node(wtk_svprint_t *v,wtk_string_t *name);
int wtk_svprint_feat_likelihood_node(wtk_svprint_t *v,wtk_vecf_t *feat,wtk_svprint_cfg_feat_node_t *node);
void wtk_svprint_set_score_notify(wtk_svprint_t *v,void *ths,wtk_svprint_score_notify notify);
float wtk_svprint_feat_compute_likelihood(wtk_vecf_t *feat1, wtk_vecf_t *feat2);
float wtk_svprint_feat_compute_likelihood2(wtk_vecf_t *feat1,
                                           wtk_vecf_t *feat2);
int wtk_svprint_L2_normalization(wtk_vecf_t *mean, int cnt);
int wtk_svprint_L2_normalization2(wtk_vecf_t *mean,wtk_vecf_t *dst, int cnt);
#ifdef __cplusplus
};
#endif
#endif
