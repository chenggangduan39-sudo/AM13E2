#ifndef WTK_ASR_PARM_WTK_HLDA_CFG
#define WTK_ASR_PARM_WTK_HLDA_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/wtk_larray.h"
#include "wtk/core/wtk_fixpoint.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_hlda_cfg wtk_hlda_cfg_t;

struct wtk_hlda_cfg
{
	wtk_matf_t *hlda;
	wtk_mati_t *fix_hlda;
	char *fn;
};

int wtk_hlda_cfg_bytes(wtk_hlda_cfg_t *cfg);
int wtk_hlda_cfg_init(wtk_hlda_cfg_t *cfg);
int wtk_hlda_cfg_clean(wtk_hlda_cfg_t *cfg);
int wtk_hlda_cfg_update_local(wtk_hlda_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_hlda_cfg_update(wtk_hlda_cfg_t *cfg);
int wtk_hlda_cfg_update2(wtk_hlda_cfg_t *cfg,wtk_source_loader_t *sl);
void wtk_hlda_cfg_update_fix(wtk_hlda_cfg_t *cfg);

void wtk_hlda_cfg_write_fix(wtk_hlda_cfg_t *cfg,FILE *f);
int wtk_hlda_cfg_read_fix(wtk_hlda_cfg_t *cfg,wtk_source_t *src);
#ifdef __cplusplus
};
#endif
#endif
