#ifndef WTK_FST_REC_WTK_WFSTR_PRUNE_H_
#define WTK_FST_REC_WTK_WFSTR_PRUNE_H_
#include "wtk/asr/wfst/rec/wtk_wfstr.h"
#include "wtk/asr/wfst/net/wtk_fst_net3.h"
#ifdef __cplusplus
extern "C" {
#endif

int wtk_wfstr_prune_lat(wtk_wfstr_t *r,float beam,int hg_nodes);
void wtk_wfstr_lat_add_end(wtk_wfstr_t *r);
#ifdef __cplusplus
};
#endif
#endif
