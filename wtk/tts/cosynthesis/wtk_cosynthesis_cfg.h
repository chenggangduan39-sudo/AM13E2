#ifndef __WTK_COSYN_CFG_H__
#define __WTK_COSYN_CFG_H__
#include <stdio.h>
#include "wtk_cosynthesis_lexicon.h"
#include "wtk_cosynthesis_backend.h"
#include "wtk_cosynthesis_phrase_cfg.h"
#include "wtk_wsola.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk_trietree_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct 
{
	wtk_rbin2_t *rbin;
    wtk_wsola_cfg_t wsola_cfg;
    wtk_cosynthesis_lexicon_cfg_t lex_cfg;
    wtk_cosynthesis_backend_cfg_t be_cfg;
    wtk_trietree_cfg_t trie_cfg;
    wtk_cosynthesis_phrase_cfg_t phr_cfg;
    wtk_heap_t *heap;
    wtk_strpool_t *pool;
    int n_context_presele;
    int n_kld_presele;
    char *user_lexicon;
    char *crf_model_fn;
    float max_loss;
    float inset_thd;
    char *use_auth_fn;
    unsigned int use_auth:1;
    wtk_mbin_cfg_t *mbin_cfg;
    wtk_cfg_file_t *cfile;
    wtk_array_t *mgc_floor;
    wtk_array_t *lf0_floor;
}wtk_cosynthesis_cfg_t;

int wtk_cosynthesis_cfg_init(wtk_cosynthesis_cfg_t *cfg);
int wtk_cosynthesis_cfg_update_local(wtk_cosynthesis_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_cosynthesis_cfg_clean(wtk_cosynthesis_cfg_t *cfg);
int wtk_cosynthesis_cfg_update(wtk_cosynthesis_cfg_t *cfg);
int wtk_cosynthesis_cfg_update2(wtk_cosynthesis_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_cosynthesis_cfg_t* wtk_cosynthesis_cfg_new_bin(char *cfg_fn,int seek_pos);
void wtk_cosynthesis_cfg_delete_bin(wtk_cosynthesis_cfg_t *cfg);
#ifdef __cplusplus
};
#endif

#endif
