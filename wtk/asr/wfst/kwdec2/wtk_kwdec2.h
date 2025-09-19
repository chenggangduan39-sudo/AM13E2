#ifndef WTK_KWDEC2_H_
#define WTK_KWDEC2_H_
#include "wtk/asr/wfst/egram/wtk_egram.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk_kwdec2_cfg.h"
#include <float.h>
#include <limits.h>
#include <math.h>
//#include "wtk/asr/wfst/kwdec/qtk_wakeup_trans_model_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/asr/wfst/net/wtk_fst_net3.h"
#include "wtk/core/wtk_vpool2.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_kwdec2_token wtk_kwdec2_token_t;
typedef struct wtk_kwdec2_pth wtk_kwdec2_pth_t;
typedef struct wtk_kwdec2 wtk_kwdec2_t;
typedef struct wtk_kwdec2_words wtk_kwdec2_words_t;
extern int wtk_kwdec2_post_feed(wtk_kwdec2_t* dec, wtk_vector_t* f);

struct wtk_kwdec2_token
{
	wtk_queue_node_t q_n;
	float tot_cost;
	//wtk_kwdec2_link_t *link;//forward link for lattice
	wtk_kwdec2_pth_t *pth;//backward path for one best path
	wtk_fst_state_t *state;
};

struct wtk_kwdec2_pth
{
	int out_label;
	int in_label;
	float like;
	wtk_kwdec2_pth_t *lbest;
	int frame;
};

struct wtk_kwdec2_words
{
	char* word;
	int key_id;
	float pdf_conf;
	float pdf_conf2;
	int min_kws;
};

struct wtk_kwdec2
{
    wtk_kxparm_t *parm;
	wtk_kwdec2_cfg_t* cfg;
	//wtk_queue_t state_q;
	wtk_queue_t *cur_tokq;
	wtk_queue_t *pre_tokq;
	wtk_fst_net_t *net; //get fst from this
	wtk_kwdec2_trans_model_t* trans_model;
	wtk_kwdec2_token_t *best_token;
	wtk_heap_t *heap;
	wtk_vpool2_t *tok_pool;
	wtk_kwdec2_words_t **words_set;
	wtk_egram_t *egram;
	wtk_strbuf_t *ebnf_buf;
	int cur_frame;
	unsigned int *bins;
	float best_weight;
	float conf;
	int found;
	int mode_frame_cnt;
	int switch_frame_cnt;
	int index;
	int wake_flag;
	// int index;
//  wtk_vpool_t *feat_pool;
	wtk_robin_t *feat_rb;
	int wake_beg_idx;
	int wake_end_idx;
	int key_id;
	int min_kws;
	float pdf_conf;
	int out_col;
	unsigned int reset;
	int reset_frame;
	int shift;
	int flag;
	unsigned break_flag;
	float trick_pdf_conf;
	int trick_min_kws;
	int idle;
};

wtk_kwdec2_t* wtk_kwdec2_new(wtk_kwdec2_cfg_t* cfg);
int wtk_kwdec2_start(wtk_kwdec2_t* dec);
//int wtk_kwdec2_feed(wtk_kwdec2_t* dec,wtk_vector_t *v,int index);
void wtk_kwdec2_reset(wtk_kwdec2_t* dec);
void wtk_kwdec2_delete(wtk_kwdec2_t* dec);
int wtk_kwdec2_feed(wtk_kwdec2_t* dec, char *data,int len, int is_end);
int wtk_kwdec2_feed2(wtk_kwdec2_t* dec, short *data,int len, int is_end);
void wtk_kwdec2_get_wake_time(wtk_kwdec2_t *dec,float *fs,float *fe);
float wtk_kwdec2_get_conf(wtk_kwdec2_t *dec);
int wtk_kwdec2_set_words(wtk_kwdec2_t *dec, char *words, int len);
void wtk_kwdec2_set_words_cfg(wtk_kwdec2_t* dec,char *words,int len);
void wtk_kwdec2_decwords_set(wtk_kwdec2_t* dec);
int wtk_kwdec2_set_context(wtk_kwdec2_t* dec,char *words,int len);
#ifdef __cplusplus
}
;
#endif
#endif

