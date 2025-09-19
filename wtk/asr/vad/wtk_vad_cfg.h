#ifndef WTK_VAD_WTK_VAD_CFG_H_
#define WTK_VAD_WTK_VAD_CFG_H_
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/asr/vad/annvad/wtk_annvad_cfg.h"
#include "wtk/asr/vad/fnnvad/wtk_fnnvad_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/vad/gmmvad/wtk_gmmvad2.h"
#include "wtk/asr/fextra/f0/wtk_f0.h"
#include "wtk/asr/vad/fevad/wtk_fevad.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vad_cfg wtk_vad_cfg_t;

typedef enum
{
	WTK_ANN_VAD,
	WTK_DNN_VAD,
	WTK_GMM_VAD,
	WTK_FE_VAD,
	WTK_K_VAD,
}wtk_vad_type_t;

struct wtk_vad_cfg
{
	wtk_vad_type_t type;
	wtk_annvad_cfg_t annvad;
	wtk_fnnvad_cfg_t dnnvad;
	wtk_gmmvad_cfg_t gmmvad2;
	wtk_fevad_cfg_t fevad;
	wtk_kvad_cfg_t kvad;
	int left_margin;		//left margin frame count;
	int right_margin;		//right margin frame count;
	int min_speech;
	void *hook;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
	unsigned use_margin_check:1;
	unsigned use_ann:1;
	unsigned use_dnn:1;
	unsigned use_gmm2:1;
	unsigned use_fe:1;
	unsigned use_k:1;
	unsigned margin_proc:1;
	unsigned prob_proc:1;
};

int wtk_vad_cfg_init(wtk_vad_cfg_t *cfg);
int wtk_vad_cfg_clean(wtk_vad_cfg_t *cfg);
int wtk_vad_cfg_update_local(wtk_vad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vad_cfg_update(wtk_vad_cfg_t *cfg);

/**
 * @brief used for bin loader;
 */
int wtk_vad_cfg_update2(wtk_vad_cfg_t *cfg,wtk_source_loader_t *sl);

/**
 * wtk_vad_cfg_new_bin2
 * wtk_vad_cfg_delete_bin2
 */
wtk_vad_cfg_t* wtk_vad_cfg_new_bin(char *bin_fn,char *cfg_fn);
wtk_vad_cfg_t* wtk_vad_cfg_new_bin2(char *bin_fn);
wtk_vad_cfg_t* wtk_vad_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int wtk_vad_cfg_delete_bin(wtk_vad_cfg_t *cfg);
int wtk_vad_cfg_delete_bin2(wtk_vad_cfg_t *cfg);
wtk_vad_cfg_t* wtk_vad_cfg_new(char *cfg_fn);
void wtk_vad_cfg_delete(wtk_vad_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
