#ifndef WTK_FST_EGRAM_WTK_E2FST_H_
#define WTK_FST_EGRAM_WTK_E2FST_H_
#include "wtk/core/wtk_type.h"
#include "wtk_e2fst_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_e2fst wtk_e2fst_t;
struct wtk_egram;
typedef struct
{
	wtk_slist_node_t s_n;
	wtk_string_t *v;
	int id;
}wtk_e2fst_id_t;

struct wtk_e2fst
{
	wtk_e2fst_cfg_t *cfg;
	struct wtk_egram *egram;
	wtk_fkv_t *fkv;
	wtk_fst_net2_t *net;
	wtk_fst_net2_t *hmm_net;
	wtk_str_hash_t *hash;
	wtk_strbuf_t *buf;
	long sym_out_id;
	wtk_slist_t state_l;	//fst state list;
	wtk_slist_t hmm_l;
        wtk_slist_t filler_l;
        wtk_slist_t sym_l;
        wtk_fst_net2_t *wrd_net;
        wtk_fst_net2_t *mono_net;
        wtk_fst_net2_t *full_mono_net;
        wtk_fst_net_t *filler_net;
        int sil_in_id;
        int sil_S_in_id;
        int snt_end_out_id;
	int hmms_cnt;
        int wdec_cnt;
        int mono_cnt;
        int out_id;
        wtk_fst_net_print_t print;
        wtk_fst_state2_t *sil_snt_end;
        wtk_fst_state2_t *snt_end;
};

void wtk_e2fst_init_bin(wtk_e2fst_t *e);
void wtk_e2fst_init_txt(wtk_e2fst_t *e);
wtk_e2fst_t* wtk_e2fst_new(wtk_e2fst_cfg_t *cfg,struct wtk_egram *egram,wtk_rbin2_t *rbin);
void wtk_e2fst_delete(wtk_e2fst_t *e);
void wtk_e2fst_reset(wtk_e2fst_t *e);
int wtk_hmm_expand(wtk_e2fst_t *e);
int wtk_e2fst_expand(wtk_e2fst_t *e,wtk_fst_net2_t *wrd_net);
int wtk_e2fst_get_symid(wtk_e2fst_t *e,char *data,int bytes);
wtk_string_t* wtk_e2fst_get_outsym(wtk_e2fst_t *e,unsigned int xid);
wtk_string_t* wtk_e2fst_get_insym(wtk_e2fst_t *e,unsigned int id);
wtk_string_t* wtk_e2fst_get_phnstr(wtk_e2fst_t *e,int id);
wtk_string_t* wtk_e2fst_get_phnstr_bit(wtk_e2fst_t *e,int id);
void wtk_e2fst_print_fsm(wtk_e2fst_t *e,wtk_strbuf_t *buf);
void wtk_e2fst_print_fsm_bin(wtk_e2fst_t *e,wtk_strbuf_t *buf);
void wtk_e2fst_get_outsym_txt(wtk_e2fst_t *e,wtk_strbuf_t *buf);
void wtk_e2fst_get_outsym_bin(wtk_e2fst_t *e,wtk_strbuf_t *buf);
void wtk_e2fst_get_outsym_bin2(wtk_e2fst_t *e,wtk_strbuf_t *buf);
int wtk_e2fst_get_phnid(wtk_e2fst_t *e,char *data,int bytes);
int wtk_e2fst_get_phnid2(wtk_e2fst_t *e,int id);
int wtk_e2fst_get_phnid4(wtk_e2fst_t *e,int id);
void wtk_e2fst_print_fsm_hmmexp(wtk_e2fst_t *e,wtk_strbuf_t *buf);
int wtk_e2fst_dump(wtk_e2fst_t *e,wtk_fst_net_t *net);
int wtk_e2fst_dump2(wtk_e2fst_t *e,wtk_fst_net_t *net);
int wtk_e2fst_dump3(wtk_e2fst_t *e,wtk_fst_net_t *net,wtk_fst_sym_t *sym,wtk_slist_t *state_l);
#ifdef __cplusplus
};
#endif
#endif
