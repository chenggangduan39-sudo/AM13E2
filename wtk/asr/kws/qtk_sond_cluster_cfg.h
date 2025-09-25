#ifndef QTK_SOND_CLUSTER_CFG_H_
#define QTK_SOND_CLUSTER_CFG_H_
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk_svprint.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#include "wtk/asr/fextra/onnxruntime/qtk_onnxruntime.h"
#include "clu/wtk_clu.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_sond_cluster_cfg qtk_sond_cluster_cfg_t;

struct qtk_sond_cluster_cfg
{
#ifdef ONNX_DEC
    qtk_onnxruntime_cfg_t onnx;
#endif
	void *hook;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	wtk_kxparm_cfg_t kxparm;
	wtk_kvad_cfg_t vad;
	wtk_svprint_cfg_t svprint;
	wtk_clu_cfg_t clu;
	char *wake_fn;
	char *e2e_fn;
	char *svprint_fn;
	wtk_source_loader_t sl;
	int encoder_downsample_ratio;
	int smooth_size;
	int seg_length_threshold;
	int clu_len;
	float logit_threshold;
	float frame_hop;
	float min_duration_off;
	float min_duration_on;
	unsigned int use_svprint:1;
	unsigned int use_vad:1;
	unsigned int use_window_pad:1;
	unsigned int use_leak_memory:1;
	unsigned int use_clu:1;
	unsigned int use_tsvad:1;
};

int qtk_sond_cluster_cfg_init(qtk_sond_cluster_cfg_t *cfg);
int qtk_sond_cluster_cfg_clean(qtk_sond_cluster_cfg_t *cfg);
int qtk_sond_cluster_cfg_update_local(qtk_sond_cluster_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_sond_cluster_cfg_update(qtk_sond_cluster_cfg_t *cfg);

/**
 * @brief used for bin loader;
 */
int qtk_sond_cluster_cfg_update2(qtk_sond_cluster_cfg_t *cfg,wtk_source_loader_t *sl);

qtk_sond_cluster_cfg_t* qtk_sond_cluster_cfg_new_bin(char *bin_fn,char *cfg_fn);
qtk_sond_cluster_cfg_t* qtk_sond_cluster_cfg_new_bin2(char *bin_fn);
qtk_sond_cluster_cfg_t* qtk_sond_cluster_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int qtk_sond_cluster_cfg_delete_bin(qtk_sond_cluster_cfg_t *cfg);
int qtk_sond_cluster_cfg_delete_bin2(qtk_sond_cluster_cfg_t *cfg);
qtk_sond_cluster_cfg_t* qtk_sond_cluster_cfg_new(char *cfg_fn);
void qtk_sond_cluster_cfg_delete(qtk_sond_cluster_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
