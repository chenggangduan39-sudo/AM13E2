#ifndef WTK_FST_EGRAM_WTK_E2FST_CFG_H_
#define WTK_FST_EGRAM_WTK_E2FST_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/core/wtk_fkv.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_e2fst_cfg wtk_e2fst_cfg_t;

typedef struct
{
	int forward_id;
	int selfloop_id;
	float forward_weight;
	float selfloop_weight;
	int *id;
	float *weight;
}wtk_e2fst_hmm_expand_pdf_t;

typedef struct
{
	wtk_e2fst_hmm_expand_pdf_t *pdf;
	int num_pdfs;
}wtk_e2fst_hmm_expand_t;

struct wtk_e2fst_cfg
{
	wtk_fst_net_cfg_t net;
        wtk_fst_net_cfg_t filler_net;
        char *sym_out_fn;
	wtk_fst_insym_t *sym_out;
	char *phn_map_fn;
	char *hmm_map_fn;
	char *phn_id_fn;
	int phn_hash_hint;
	int sym_hash_hint;
	int sil_id;
	int sil_S_id;
	int type;
	wtk_heap_t *heap;
	wtk_string_t **phn_ids;
	int *hmm_map;
	wtk_e2fst_hmm_expand_t **hmm_maps;
	float selfloop_scale;
	float transition_scale;
	unsigned use_merge:1;
	unsigned use_chain:1;
	unsigned use_txt:1;
	unsigned add_sil:1;
	unsigned remove_dup:1;
	unsigned use_opt_sil:1;
	unsigned use_cross_wrd:1;
	unsigned use_sil_ctx:1;
	unsigned use_pre_wrd:1;
	unsigned use_biphone:1;
	unsigned use_posi:1;
	unsigned use_eval:1;    //for diff net-build need.
        unsigned add_filler : 1;
	unsigned filler : 1;
};


int wtk_e2fst_cfg_init(wtk_e2fst_cfg_t *cfg);
int wtk_e2fst_cfg_clean(wtk_e2fst_cfg_t *cfg);
int wtk_e2fst_cfg_update_local(wtk_e2fst_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_e2fst_cfg_update2(wtk_e2fst_cfg_t *cfg,wtk_label_t *label,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
