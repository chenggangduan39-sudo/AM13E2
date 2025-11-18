#ifndef WTK_CLU_CFG_H
#define WTK_CLU_CFG_H
#include "qtk/sci/clustering/qtk_clustering_spectral.h"
#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_clu_cfg wtk_clu_cfg_t;

struct wtk_clu_cfg {
    wtk_nnet3_xvector_compute_cfg_t xvector;
    float hop_s;
    float chunk_s;
    float vprint_s;
    int vprint_num_byte;
    int vprint_size;
    int hop_num_byte;
    int chunk_num_byte;
    int sample_rate;
    int byte_per_sample;
    int chunk_dur;
    void *hook;
};

int wtk_clu_cfg_init(wtk_clu_cfg_t *cfg);
int wtk_clu_cfg_clean(wtk_clu_cfg_t *cfg);
int wtk_clu_cfg_update(wtk_clu_cfg_t *cfg);
int wtk_clu_cfg_update2(wtk_clu_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_clu_cfg_update_local(wtk_clu_cfg_t *cfg, wtk_local_cfg_t *lc);
#ifdef __cplusplus
};
#endif
#endif
