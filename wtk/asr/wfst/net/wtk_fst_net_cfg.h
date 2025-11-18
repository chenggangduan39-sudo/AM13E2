#ifndef WTK_FST_NET_FST_NET_CFG_H_
#define WTK_FST_NET_FST_NET_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/wfst/net/kv/wtk_fst_binet_cfg.h"
#include "wtk/core/wtk_label.h"
#include "wtk/asr/wfst/net/sym/wtk_fst_insym.h"
#include "wtk_fst_sym.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_net_cfg wtk_fst_net_cfg_t;

typedef struct
{
	float *probs;		//prob of word
	unsigned int n;
}wtk_fst_net_nwlp_t;


struct wtk_fst_net_cfg
{
	wtk_fst_binet_cfg_t bin;
	wtk_label_t *label;
	int trans_cache;
	int state_cache;
	int trans_reset_cache;
	int state_reset_cache;
	//---------- configure -----------
	char *symbol_in_fn;
	char *symbol_out_fn;
	char *lm_look_ahead_fn;
	char *fsm_fn;
	wtk_fst_net_nwlp_t *nwlp;
	float lm_look_scale;
	//---------- resource ------------
	int eps_id;
	int snt_end_id;
	int snt_start_id;
	int n_state;
	int n_final_state;
	int n_trans;
	int array_nslot2;
	int hash_scale;
	int min_hash_size;
	float lmscale;
	float wordpen;
	float reset_max_bytes;
	//-------------- state -------------
	//-------------------kv configure ----------------
	wtk_fst_insym_t *sym_in;
	//wtk_fst_sym_t *sym_in;
	wtk_fst_sym_t *sym_out;
	//------------------------------------------------
	unsigned use_in_bin:1;
	unsigned use_out_bin:1;
	unsigned use_bin:1;
	unsigned use_shash:1;
	unsigned use_cheap:1;
	unsigned symin_use_hash:1;
	unsigned use_rbin:1;
	unsigned load_all:1;
	unsigned use_dynamic_reset:1;
};

int wtk_fst_net_cfg_init(wtk_fst_net_cfg_t *cfg);
int wtk_fst_net_cfg_clean(wtk_fst_net_cfg_t *cfg);
int wtk_fst_net_cfg_update_local(wtk_fst_net_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_fst_net_cfg_update(wtk_fst_net_cfg_t *cfg);
int wtk_fst_net_cfg_update2(wtk_fst_net_cfg_t *cfg,wtk_label_t *label);
int wtk_fst_net_cfg_update3(wtk_fst_net_cfg_t *cfg,wtk_label_t *label,wtk_source_loader_t *sl);
void wtk_fst_net_cfg_print(wtk_fst_net_cfg_t *cfg);
int wtk_fst_net_cfg_bytes(wtk_fst_net_cfg_t *cfg);
int wtk_fst_net_nwlp_bytes(wtk_fst_net_nwlp_t *n);
int wtk_fst_net_cfg_is_visible_wrd(wtk_fst_net_cfg_t *cfg,unsigned int id);

#ifdef __cplusplus
};
#endif
#endif
