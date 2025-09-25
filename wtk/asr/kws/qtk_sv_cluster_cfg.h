#ifndef __QTK_SV_CLUSTER_CFG_H__
#define __QTK_SV_CLUSTER_CFG_H__

#include "wtk/asr/fextra/nnet3/wtk_nnet3_xvector_compute_cfg.h"
#include "wtk/asr/vad/kvad/wtk_kvad_cfg.h"
#include "wtk/asr/fextra/ivector/wtk_ivector_plda_scoring_cfg.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef struct qtk_sv_cluster_cfg qtk_sv_cluster_cfg_t;
struct qtk_sv_cluster_cfg{
	void *hook;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	wtk_source_loader_t sl;
    wtk_nnet3_xvector_compute_cfg_t xvector;
    wtk_ivector_plda_scoring_cfg_t plda;
    wtk_kvad_cfg_t kvad;

    char *svprint_nn_fn;
    char *pool_fn;
    int mode;//mode 0 vad+vad;1 vad;
    int min_len;
    float thresh1;
    float thresh2;
};

int qtk_sv_cluster_cfg_init(qtk_sv_cluster_cfg_t *cfg);
int qtk_sv_cluster_cfg_update_local(qtk_sv_cluster_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_sv_cluster_cfg_update(qtk_sv_cluster_cfg_t *cfg);
int qtk_sv_cluster_cfg_update2(qtk_sv_cluster_cfg_t *cfg,wtk_source_loader_t *sl);
int qtk_sv_cluster_cfg_clean(qtk_sv_cluster_cfg_t *cfg);

qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new_bin2(char *bin_fn,char *cfg_fn);
qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new_bin(char *bin_fn);
qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int qtk_sv_cluster_cfg_delete_bin(qtk_sv_cluster_cfg_t *cfg);
int qtk_sv_cluster_cfg_delete_bin2(qtk_sv_cluster_cfg_t *cfg);
qtk_sv_cluster_cfg_t* qtk_sv_cluster_cfg_new(char *cfg_fn);
void qtk_sv_cluster_cfg_delete(qtk_sv_cluster_cfg_t *cfg);

#ifdef __cplusplus
};
#endif


#endif
