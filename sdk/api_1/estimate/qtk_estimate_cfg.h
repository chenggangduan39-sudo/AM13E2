#ifndef __QTK_ESTIMATE_CFG_H__
#define __QTK_ESTIMATE_CFG_H__
#include "wtk/bfio/ahs/estimate/qtk_rir_estimate2_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_estimate_cfg{
	wtk_string_t cfg_fn;
	qtk_rir_estimate2_cfg_t *estimate_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;

	unsigned int use_bin:1;
	unsigned int use_manual:1;
}qtk_estimate_cfg_t;

int qtk_estimate_cfg_init(qtk_estimate_cfg_t *cfg);
int qtk_estimate_cfg_clean(qtk_estimate_cfg_t *cfg);
int qtk_estimate_cfg_update_local(qtk_estimate_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_estimate_cfg_update(qtk_estimate_cfg_t *cfg);
int qtk_estimate_cfg_update2(qtk_estimate_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_estimate_cfg_t *qtk_estimate_cfg_new(char *fn);
void qtk_estimate_cfg_delete(qtk_estimate_cfg_t *cfg);
qtk_estimate_cfg_t *qtk_estimate_cfg_new_bin(char *bin_fn);
void qtk_estimate_cfg_delete_bin(qtk_estimate_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif