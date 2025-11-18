#ifndef WTK_FST_USREC_WTK_USREC
#define WTK_FST_USREC_WTK_USREC
#include "wtk/core/wtk_type.h" 
#include "wtk_usrec_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_usrec wtk_usrec_t;
struct wtk_usrec
{
	wtk_usrec_cfg_t *cfg;
	wtk_wfstr_t *rec;
};

wtk_usrec_t* wtk_usrec_new(wtk_usrec_cfg_t *cfg);
void wtk_usrec_delete(wtk_usrec_t *r);
void wtk_usrec_reset(wtk_usrec_t *r);
int wtk_usrec_start(wtk_usrec_t *r);
void wtk_usrec_finish(wtk_usrec_t *r);
void wtk_usrec_feed(wtk_usrec_t *u,wtk_feat_t *f);
#ifdef __cplusplus
};
#endif
#endif
