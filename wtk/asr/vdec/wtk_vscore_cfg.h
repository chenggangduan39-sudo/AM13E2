#ifndef WTK_ASR_VDEC_WTK_VSCORE_CFG
#define WTK_ASR_VDEC_WTK_VSCORE_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/fextra/wtk_fextra_cfg.h"
#include "wtk/asr/model/wtk_hmmset_cfg.h"
#include "wtk/asr/vdec/rec/wtk_rec_cfg.h"
#include "wtk/asr/net/wtk_latset.h"
#include "wtk/asr/net/wtk_ebnf.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk_vrec.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vscore_cfg wtk_vscore_cfg_t;
struct wtk_vscore_cfg
{
	wtk_fextra_cfg_t parm;
	wtk_hmmset_cfg_t hmmset;
	wtk_vrec_cfg_t fa;
	wtk_vrec_cfg_t loop;
	wtk_label_t *label;
	wtk_main_cfg_t *cfg;
	wtk_mbin_cfg_t *bin_cfg;
};

int wtk_vscore_cfg_init(wtk_vscore_cfg_t *cfg);
int wtk_vscore_cfg_clean(wtk_vscore_cfg_t *cfg);
int wtk_vscore_cfg_update_local(wtk_vscore_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vscore_cfg_update(wtk_vscore_cfg_t *cfg);
int wtk_vscore_cfg_update2(wtk_vscore_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_vscore_cfg_t* wtk_vscore_cfg_new(char *fn);
void wtk_vscore_cfg_delete(wtk_vscore_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
