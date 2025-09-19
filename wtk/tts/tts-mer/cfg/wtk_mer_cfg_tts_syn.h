#ifndef WTK_MER_CFG_TTS_SYN_H
#define WTK_MER_CFG_TTS_SYN_H
#include "tts-mer/wtk_mer_common.h"
#include "syn/wtk_syn_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

wtk_syn_dtree_t* wtk_mer_syn_dtree_new(wtk_syn_dtree_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);
wtk_syn_hmm_t* wtk_mer_syn_hmm_new(wtk_syn_hmm_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool,wtk_syn_dtree_t *dtree);

#ifdef __cplusplus
}
#endif
#endif
