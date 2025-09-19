#ifndef QTK_WAKE_E2E_CFG_H_
#define QTK_WAKE_E2E_CFG_H_
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_wake_e2e_cfg qtk_wake_e2e_cfg_t;

struct qtk_wake_e2e_cfg
{
	void *hook;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
};

int qtk_wake_e2e_cfg_init(qtk_wake_e2e_cfg_t *cfg);
int qtk_wake_e2e_cfg_clean(qtk_wake_e2e_cfg_t *cfg);
int qtk_wake_e2e_cfg_update_local(qtk_wake_e2e_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_wake_e2e_cfg_update(qtk_wake_e2e_cfg_t *cfg);

/**
 * @brief used for bin loader;
 */
int qtk_wake_e2e_cfg_update2(qtk_wake_e2e_cfg_t *cfg,wtk_source_loader_t *sl);

qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new_bin(char *bin_fn,char *cfg_fn);
qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new_bin2(char *bin_fn);
qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int qtk_wake_e2e_cfg_delete_bin(qtk_wake_e2e_cfg_t *cfg);
int qtk_wake_e2e_cfg_delete_bin2(qtk_wake_e2e_cfg_t *cfg);
qtk_wake_e2e_cfg_t* qtk_wake_e2e_cfg_new(char *cfg_fn);
void qtk_wake_e2e_cfg_delete(qtk_wake_e2e_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
