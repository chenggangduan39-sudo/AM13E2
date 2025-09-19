
#ifndef _WTK_SVPRINT_CFG_H_
#define _WTK_SVPRINT_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute_cfg.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring_cfg.h"
#include "wtk/asr/vad/wtk_vad_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_svprint_cfg wtk_svprint_cfg_t;

typedef struct wtk_svprint_cfg_feat_node
{
    wtk_queue_node_t qn;
    wtk_string_t *name;
    int num;
    wtk_vecf_t *v;
}wtk_svprint_cfg_feat_node_t;

struct wtk_svprint_cfg
{
    wtk_ivector_plda_scoring_cfg_t plda_scoring;
    wtk_nnet3_xvector_compute_cfg_t xvector;
    // wtk_mbin_cfg_t *main_vad_cfg;
    wtk_vad_cfg_t vad;
    wtk_queue_t enroll_q;
    float score_thresh;
    char *enroll_fn;
    char *pool_fn;

    char *vad_fn;
    float vad_energy_thresh;
    float vad_energy_mean_scale;
    float vad_proportion_threshold;
    int vad_frames_context;
    int max_spks;
    unsigned load_enroll_fn:1;
    unsigned use_vad_cut:1;
    unsigned use_plda:1;
    unsigned use_distance:1;
};

int wtk_svprint_cfg_init(wtk_svprint_cfg_t *cfg);
int wtk_svprint_cfg_clean(wtk_svprint_cfg_t *cfg);
int wtk_svprint_cfg_update_local(wtk_svprint_cfg_t *cfg, wtk_local_cfg_t *main);
int wtk_svprint_cfg_update(wtk_svprint_cfg_t *cfg);
int wtk_svprint_cfg_update2(wtk_svprint_cfg_t *cfg,wtk_source_loader_t *sl);
wtk_svprint_cfg_feat_node_t* wtk_svprint_cfg_node_new(char *name,int len);
wtk_svprint_cfg_feat_node_t* wtk_svprint_cfg_node_new2(wtk_string_t *name,wtk_vecf_t *feat,int cnt);
wtk_svprint_cfg_feat_node_t* wtk_svprint_cfg_node_new3(int name_len);
void wtk_svprint_cfg_node_delete(wtk_svprint_cfg_feat_node_t *node);
int wtk_svprint_cfg_set_vpint_fn(wtk_svprint_cfg_t *cfg,char *fn);
int wtk_svprint_cfg_set_enroll_stats_fn(wtk_svprint_cfg_t *cfg,char *fn);
int wtk_svprint_cfg_set_eval_stats_fn(wtk_svprint_cfg_t *cfg,char *fn);

#ifdef __cplusplus
};
#endif
#endif
