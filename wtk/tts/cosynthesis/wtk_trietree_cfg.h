/*
 * wtk_trietree_cfg.h
 *
 *  Created on: Jan 27, 2022
 *      Author: dm
 */

#ifndef WTK_COSYNTHESIS_WTK_TRIETREE_CFG_H_
#define WTK_COSYNTHESIS_WTK_TRIETREE_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk_trietree.h"
#include "wtk/core/rbin/wtk_rbin2.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_trietree_cfg
{
	wtk_rbin2_t *rbin;
    char *trie_fn;
    char *trie_inset_fn;
    unsigned char *g_trie_res;
    unsigned char *g_trie_inset_res;
    unsigned int load_all:1;
    wtk_trieroot_t **root;
    wtk_trieroot_t **root_inset;
    wtk_trieitem_t* root_pos;
    wtk_trieitem_t* root_inset_pos;
    int nwrds;
}wtk_trietree_cfg_t;
int wtk_trietree_cfg_init(wtk_trietree_cfg_t *cfg);
int wtk_trietree_cfg_update_local(wtk_trietree_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_trietree_cfg_clean(wtk_trietree_cfg_t *cfg);
int wtk_trietree_cfg_update(wtk_trietree_cfg_t *cfg);
int wtk_trietree_cfg_update2(wtk_trietree_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif /* WTK_COSYNTHESIS_WTK_TRIETREE_CFG_H_ */
