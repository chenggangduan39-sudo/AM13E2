
#ifndef _WTK_XVPRINT_H_
#define _WTK_XVPRINT_H_
#include "wtk_xvprint_cfg.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/asr/vad/wtk_vad.h"
#include "wtk/core/wtk_larray.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef int (*wtk_xvprint_score_notify)(void *ths,wtk_string_t *name,float score);

typedef struct wtk_xvprint wtk_xvprint_t;


struct wtk_xvprint
{
    wtk_xvprint_cfg_t *cfg;
    wtk_ivector_plda_scoring_t *scoring;
    wtk_nnet3_xvector_compute_t *x;
    wtk_vad_t *v;
    wtk_queue_t vad_q;
    wtk_queue_t feat_q;
    wtk_queue_t enroll_q;
    wtk_robin_t *vrobin;
    wtk_larray_t *state_l;
    int vad_pop_index;
    int feat_push_index;
    float log_energy;
    wtk_xvprint_score_notify notify_f;
    void *ths;
    unsigned vad_state:1;
};

wtk_xvprint_t* wtk_xvprint_new(wtk_xvprint_cfg_t *cfg);
void  wtk_xvprint_delete(wtk_xvprint_t *v);
int wtk_xvprint_start(wtk_xvprint_t *v);
void wtk_xvprint_reset(wtk_xvprint_t *v);
int wtk_xvprint_feed(wtk_xvprint_t *v,short *data,int len,int is_end);
int wtk_xvprint_enroll2mem(wtk_xvprint_t *v,wtk_string_t *name);
int wtk_xprint_dump_memory2file(wtk_xvprint_t *v);
int wtk_xvprint_enroll2file(wtk_xvprint_t *v,wtk_string_t *name);
wtk_vecf_t* wtk_xvprint_compute_feat(wtk_xvprint_t *v);
wtk_string_t* wtk_xvprint_feat_likelihood(wtk_xvprint_t *v,wtk_vecf_t *feat);
wtk_xvprint_cfg_feat_node_t* wtk_xprint_query_memory_node(wtk_xvprint_t *v,wtk_string_t *name);
int wtk_xvprint_feat_likelihood_node(wtk_xvprint_t *v,wtk_vecf_t *feat,wtk_xvprint_cfg_feat_node_t *node);
void wtk_xvprint_set_score_notify(wtk_xvprint_t *v,void *ths,wtk_xvprint_score_notify notify);
#ifdef __cplusplus
};
#endif
#endif
