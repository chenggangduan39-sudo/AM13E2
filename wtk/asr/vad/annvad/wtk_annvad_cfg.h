#ifndef WTK_ANNVAD_WTK_ANNVAD_CFG_H_
#define WTK_ANNVAD_WTK_ANNVAD_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/fextra/ann/wtk_ann_cfg.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_annvad_cfg wtk_annvad_cfg_t;

typedef struct
{
	wtk_annvad_cfg_t *cfg;
	wtk_vector_t *parm_mean;
	wtk_vector_t *parm_var;
	wtk_ann_wb_t wb;	//weight and bias matrixs;
}wtk_annvad_res_t;

struct wtk_annvad_cfg
{
	wtk_ann_wb_cfg_t wbcfg;
	wtk_annvad_res_t *res;
	wtk_fextra_cfg_t parm;
	//int win;	//left and right spaned frames;
	int left_win;
	int right_win;
	int cache;
	char *norm_fn;
	char *weight_fn;
	int siltrap;
	int speechtrap;
};

int wtk_annvad_cfg_init(wtk_annvad_cfg_t *cfg);
int wtk_annvad_cfg_clean(wtk_annvad_cfg_t *cfg);
int wtk_annvad_cfg_update_local(wtk_annvad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_annvad_cfg_update(wtk_annvad_cfg_t *cfg,wtk_source_loader_t *sl);
int wtk_annvad_cfg_update2(wtk_annvad_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
