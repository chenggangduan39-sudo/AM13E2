#ifndef WTK_FST_REC_REL_WTK_RNN_DEC
#define WTK_FST_REC_REL_WTK_RNN_DEC
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk_rnn_dec_cfg.h"
#include <math.h>
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rnn_dec wtk_rnn_dec_t;
typedef struct
{
	wtk_queue_node_t q_n;
	union{
		wtk_matf_t *f;
		wtk_mati_t *i;
	}neu_hid;
	wtk_hs_tree_wrd_t *wrd;
	void *ngram;
	unsigned use_fix:1;
}wtk_rnn_dec_env_t;


struct wtk_rnn_dec
{
	wtk_rnn_dec_cfg_t *cfg;
	wtk_queue_t env_q;
	wtk_rnn_dec_env_t start;
	int hid_size;
	int hid_bytes;
	float fix_r1;
	float fix_r2;
	float fix_r3;
	unsigned use_fix:1;
};

void wtk_rnn_dec_env_cpy(wtk_rnn_dec_env_t *src,wtk_rnn_dec_env_t *dst);

wtk_rnn_dec_t* wtk_rnn_dec_new(wtk_rnn_dec_cfg_t *cfg);
void wtk_rnn_dec_delete(wtk_rnn_dec_t *dec);
void wtk_rnn_dec_reset(wtk_rnn_dec_t *dec);
float wtk_rnn_dec_calc(wtk_rnn_dec_t *r,wtk_rnn_dec_env_t *last,wtk_rnn_dec_env_t *cur);
void wtk_rnn_dec_test(wtk_rnn_dec_t *dec,char *fn);
wtk_rnn_dec_env_t* wtk_rnn_dec_pop_env(wtk_rnn_dec_t *dec,wtk_hs_tree_wrd_t *wrd);
#ifdef __cplusplus
};
#endif
#endif
