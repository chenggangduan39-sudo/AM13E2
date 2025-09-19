#ifndef WTK_FST_USREC_WTK_USREC_CFG
#define WTK_FST_USREC_WTK_USREC_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/asr/wfst/rec/wtk_wfstr.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_usrec_cfg wtk_usrec_cfg_t;
struct wtk_usrec_cfg
{
	wtk_hmmset_cfg_t hmmset;
	wtk_fst_net_cfg_t net;
	wtk_wfstrec_cfg_t rec;
	wtk_fst_net_t *usr_net;
};

int wtk_usrec_cfg_init(wtk_usrec_cfg_t *cfg);
int wtk_usrec_cfg_clean(wtk_usrec_cfg_t *cfg);
int wtk_usrec_cfg_update_local(wtk_usrec_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_usrec_cfg_update(wtk_usrec_cfg_t *cfg);
int wtk_usrec_cfg_update2(wtk_usrec_cfg_t *cfg,wtk_source_loader_t *sl,wtk_label_t *label);
#ifdef __cplusplus
};
#endif
#endif
