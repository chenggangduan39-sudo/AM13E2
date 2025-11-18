#ifndef WTK_FST_EGRAM_WTK_EGRAM_CFG_H_
#define WTK_FST_EGRAM_WTK_EGRAM_CFG_H_
#include "wtk/asr/net/wtk_ebnf.h"
#include "wtk/asr/wfst/egram/xbnf/wtk_xbnfnet.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk_e2fst_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_egram_cfg wtk_egram_cfg_t;

struct wtk_egram_cfg
{
    union {
        wtk_main_cfg_t *main_cfg;
        wtk_mbin_cfg_t *bin_cfg;
    } cfg;
    wtk_string_t symout_fn;
    wtk_string_t fsm_fn;
    wtk_string_t ebnf_fn;
    wtk_e2fst_cfg_t e2fst;
    wtk_xbnfnet_cfg_t xbnfnet;
    wtk_label_t *label; // dict label
    wtk_dict_t *dict;   // text dict;
    char *dict_fn;
    int phn_hash_hint;
    int wrd_hash_hint;
    int type; // for germany only now , dict and tri.id kv is short when type =
              // 1;
    wtk_cfg_file_t *cfile;
    wtk_rbin2_t *rbin;
    unsigned lower : 1;
    unsigned use_ebnf : 1;
    unsigned use_wordnet : 1;
    unsigned use_bin : 1;
    unsigned hmm_expand : 1;
    unsigned use_leak : 1;
    unsigned use_kwdec : 1;
    // add by dmd 2020.01.06
    wtk_heap_t *heap;
    unsigned use_apron : 1; // for auto gen pron base existed word of dict. add
                            // by dmd 2020.01.06
    wtk_array_t *apron_conn; // connector for apron
    char *mdl_fn;
    char *sym_fn;
    char *fst_fn;
};

int wtk_egram_cfg_init(wtk_egram_cfg_t *cfg);
int wtk_egram_cfg_clean(wtk_egram_cfg_t *cfg);
int wtk_egram_cfg_update_local(wtk_egram_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_egram_cfg_update(wtk_egram_cfg_t *cfg);
int wtk_egram_cfg_update2(wtk_egram_cfg_t *cfg,wtk_source_loader_t *sl);
wtk_egram_cfg_t* wtk_egram_cfg_new_bin(char *bin_fn,unsigned int seek_pos);
wtk_egram_cfg_t *wtk_egram_cfg_new_bin2(char *fn);
int wtk_egram_cfg_delete_bin(wtk_egram_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
