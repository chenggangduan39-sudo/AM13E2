#ifndef WTK_MER_CFG_H_
#define WTK_MER_CFG_H_
#include "tts-mer/wtk_mer_common.h"
#include "tts-mer/cfg/wtk_mer_cfg_syn.h"
#include "tts-mer/cfg/wtk_mer_cfg_tts_syn.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_mer_cfg
{
	wtk_main_cfg_t *main_cfg;
	wtk_mbin_cfg_t *bin_cfg;
	wtk_tts_parser_cfg_t parser;
	wtk_syn_cfg_t syn_tts_cfg;
	wtk_mer_cfg_syn_t syn;
	wtk_strpool_t *pool;
} wtk_mer_cfg_t;

wtk_mer_cfg_t* wtk_mer_cfg_new(char *cfg_fn, int is_bin, int seek_pos);
int wtk_mer_cfg_init(wtk_mer_cfg_t *cfg);
int wtk_mer_cfg_clean(wtk_mer_cfg_t *cfg);
void wtk_mer_cfg_delete(wtk_mer_cfg_t *cfg);
int wtk_mer_cfg_update_local(wtk_mer_cfg_t *cfg, wtk_local_cfg_t *main);
int wtk_mer_cfg_update(wtk_mer_cfg_t *cfg);
int wtk_mer_cfg_update2(wtk_mer_cfg_t *cfg, wtk_source_loader_t *sl);
int wtk_mer_cfg_tts_syn_update3(wtk_syn_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool);

#ifdef __cplusplus
}
#endif
#endif
