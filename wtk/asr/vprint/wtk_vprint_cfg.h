#ifndef WTK_VITE_VPRINT_WTK_VPRINT_CFG
#define WTK_VITE_VPRINT_WTK_VPRINT_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/vprint/parm/wtk_vparm_cfg.h"
#include "wtk/asr/vprint/train/wtk_vtrain.h"
#include "wtk/asr/vprint/detect/wtk_vdetect.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vprint_cfg wtk_vprint_cfg_t;
struct wtk_vprint_cfg
{
	wtk_vparm_cfg_t vparm;
	wtk_vtrain_cfg_t train;
	wtk_vdetect_cfg_t detect;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	int train_update_cnt;
	unsigned use_share_vparm:1;
};

int wtk_vprint_cfg_init(wtk_vprint_cfg_t *cfg);
int wtk_vprint_cfg_clean(wtk_vprint_cfg_t *cfg);
int wtk_vprint_cfg_update_local(wtk_vprint_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vprint_cfg_update(wtk_vprint_cfg_t *cfg);

wtk_vprint_cfg_t* wtk_vprint_cfg_new(char *cfg);
void wtk_vprint_cfg_delete(wtk_vprint_cfg_t *cfg);

wtk_vprint_cfg_t* wtk_vprint_cfg_new_bin(char *vprint_fn);
wtk_vprint_cfg_t* wtk_vprint_cfg_new_bin2(char *vprint_fn,unsigned int seek_pos);
void wtk_vprint_cfg_delete_bin(wtk_vprint_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
