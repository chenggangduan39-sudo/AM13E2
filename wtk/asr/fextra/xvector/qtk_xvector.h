#ifndef QTK_XVECTOR_H_
#define QTK_XVECTOR_H_

#include "qtk_xvector_cfg.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring.h"
#include "wtk/asr/vad/wtk_vad.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_xvector qtk_xvector_t;

struct qtk_xvector
{
    qtk_xvector_cfg_t *cfg;
    wtk_nnet3_xvector_compute_t *x;
    wtk_vad_t *v;
    wtk_queue_t vad_q;
    wtk_queue_t feat_q;
    wtk_robin_t *vrobin;
    wtk_larray_t *state_l;
    int vad_pop_index;
    int feat_push_index;
    float log_energy;
    unsigned vad_state:1;
};

qtk_xvector_t* qtk_xvector_new(qtk_xvector_cfg_t *cfg);
int qtk_xvector_delete(qtk_xvector_t *i);
void qtk_xvector_reset(qtk_xvector_t *x);
void qtk_xvector_normalize_reset(qtk_xvector_t *x);
void qtk_xvector_start(qtk_xvector_t *x);
void qtk_xvector_feed(qtk_xvector_t *x, short *data, int len, int is_end);
wtk_vecf_t* qtk_xvector_get_result(qtk_xvector_t *x);
void qtk_xvector_write_xvec(wtk_vecf_t *v,char *fn);

#ifdef __cplusplus
};
#endif
#endif
