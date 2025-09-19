#ifndef WTK_wfstr_WTK_WFSTR_H_
#define WTK_wfstr_WTK_WFSTR_H_
#include "wtk/core/wtk_type.h"
#include "wtk_wfstr_cfg.h"
#include "wtk/asr/wfst/net/wtk_fst_net.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/wtk_vpool.h"
#include "wtk/core/wtk_vpool2.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/model/wtk_hmmset.h"
#include "wtk/core/wtk_queue2.h"
#include "wtk/core/wtk_larray.h"
#include "wtk/asr/wfst/net/wtk_fst_net2.h"
#include "wtk/asr/wfst/net/wtk_fst_net3.h"
#include "wtk/asr/fextra/wtk_feat.h"
#include "wtk/asr/wfst/rec/rescore/wtk_rescore.h"
#include "wtk/asr/net/wtk_lat.h"
#include "wtk/asr/net/wtk_transcription.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wfstr wtk_wfstr_t;
typedef struct wtk_wfst_path wtk_wfst_path_t;
typedef struct wtk_precomp wtk_precomp_t;
#ifdef USE_ALIGN
typedef struct wtk_wfst_nxtpath wtk_wfst_nxtpath_t;
typedef struct wtk_wfst_align wtk_wfst_align_t;
#endif
#define wtk_wfstr_finish2_s(r,buf,s) wtk_wfstr_finish2(r,buf,s,sizeof(s)-1)
#define wtk_wfstr_path_is_s(r,pth,s) wtk_wfstr_path_is(r,pth,s,sizeof(s)-1)

#define USE_EXT_LIKE

typedef struct
{
	wtk_wfst_path_t *path;
#ifdef USE_ALIGN
	wtk_wfst_align_t *align;
#endif
	float like; //-normalize score
#ifdef USE_EXT_LIKE
	float raw_like;
#endif
	float ac_like;
	float ac_lookahead_score;
	float lm_like;
}wtk_fst_tok_t;

#ifdef USE_ALIGN
struct wtk_wfst_align
{
	//wtk_queue_node_t q_n;
	//wtk_queue_t *queue;		//align linked in.
	wtk_wfst_align_t *prev;	//previous align record
	wtk_fst_trans_t *trans;	//node for which alignment information present.
	double like;		//likelihood upon entering state/model end
	int	frame;		//frame number upon entering state/model end
	//int usage;		//times struct refrenced by align or path
	short state;		//state level traceback info.
	//unsigned char used:1;	//refrence to struct by current inst.
};
#endif

struct wtk_wfst_path
{
	wtk_fst_trans_t *trans;
	wtk_wfst_path_t *prev;	//Previous word record
#ifdef USE_ALIGN
	wtk_wfst_align_t *align;
	wtk_wfst_nxtpath_t *nxt_chain;
#endif
	void *hook;						//attach,current arc => to_state; use for resocre
	wtk_fst_float_t ac_like;
	wtk_fst_float_t lm_like;
#ifdef USE_EXT_LIKE
	wtk_fst_float_t raw_like;
#endif
#ifdef USE_ALIGN
	int usage;
#endif
	unsigned int frame;
};
#ifdef USE_ALIGN
struct wtk_wfst_nxtpath
{
	wtk_wfst_path_t *prev;	//previous word record.
	wtk_wfst_align_t *align;
	wtk_wfst_nxtpath_t *nxt_chain;//Next of NBest paths
	wtk_fst_float_t ac_like;
	wtk_fst_float_t lm_like;	//lm score;
};
#endif
#define USE_BIT_MAP

typedef struct
{
	wtk_queue_node_t inst_n;	//waning: inst_n must be the first member,step inst1 use this
	wtk_fst_trans_t *trans;
	wtk_hmm_t *hmm;
	wtk_fst_tok_t *states;	//tok[1,num_state]; => [0,num_state-1];
	//float exit_like;	//use for pruning
#ifdef USE_BIT_MAP
	short bit_map;
#endif
	short n_active_hyps;
}wtk_fst_inst_t;


struct wtk_precomp
{
	int id;		//uniqure identifier for current frame.
	float outp;	//state/mixture output likelihood.
};

typedef float(*wtk_wfstr_dnn_get_value_f)(void *ths,wtk_feat_t *f,int index);

typedef void (*wtk_wfstr_notify_f)(void *ths,wtk_wfst_path_t *pth);

struct wtk_wfstr
{
	wtk_wfstrec_cfg_t *cfg;
	wtk_fst_net_cfg_t *net_cfg;
	wtk_prune_t *prune;
	wtk_prune_t *phn_prune;
	wtk_prune_t *wrd_prune;
	wtk_prune_t *expand_prune;
	int inst1_valid_phns;
	int inst1_valid_wrds;
	float best_expand_score;

	wtk_hmmset_t *hmmset;
	wtk_fst_net_t *net;
	wtk_fst_net_t *ebnf_net;

	wtk_vpool2_t *inst_pool;		//wtk_fst_inst_t;
	wtk_vpool2_t *tokset_pool;	//wtk_fst_tokset_t and reltok*ntok
	wtk_vpool2_t *pth_pool;		//wtk_fst_path_t;
#ifdef USE_ALIGN
	wtk_vpool_t *align_pool;    //align pool
#endif

	wtk_queue_t inst_q;		//wtk_fst_inst_t
	wtk_queue_t new_inst_q;	//wtk_fst_inst_t

	wtk_fst_tok_t init_tokset;
	wtk_fst_tok_t best_final_tokset;
	wtk_fst_tok_t best_ebnf_final_tokset;
	wtk_fst_tok_t gen_max_tok;
	wtk_fst_tok_t exit_max_tok;
	wtk_fst_trans_t *gen_max_trans;

	int cache_frame;
	int frame;
	int nsp;
	int nmp;
	wtk_precomp_t *sPre;	//array[0...nsp-1] state precomps
	wtk_precomp_t *mPre;	//array[0...nmp-1] shared mixture precomps.

	wtk_fst_float_t cur_end_thresh;
	wtk_fst_float_t cur_wrd_thresh;
	wtk_fst_float_t cur_start_thresh;
	wtk_fst_float_t cur_emit_thresh;
	wtk_fst_float_t cur_emit_raw_thresh;
	wtk_fst_float_t cur_lat_thresh;

	wtk_fst_float_t best_exit_score;
	wtk_fst_float_t best_emit_score;		//hightest score of each frame;
	wtk_fst_float_t best_start_score;
	wtk_fst_float_t best_end_score;
	wtk_fst_float_t normalize_score;		//highest normalised emiiting score from last score;

	wtk_vector_t *obs;
	//---------- hlda matrix section ----------------
	union
	{
		wtk_fixi_t *fix;
		wtk_vector_t *vector;
	}hlda;

	wtk_fst_tok_t *null_tokset_buf;
	int null_tokset_buf_bytes;

        wtk_fst_tok_t *step_inst1_tokset;
        int tokset_buf_bytes;	//sizeof(wtk_fst_tok_t)*(hmm->num_state-1)

	wtk_strbuf_t *buf;
	wtk_heap_t *heap;

	int sil_id;
	int nsil;
	int nphn;
	int nwrd;
	int end_hint_frame;
	wtk_wfst_path_t *max_pth;
	wtk_wfst_path_t *last_pth;
	wtk_fst_net3_t *lat_net;
	wtk_rescore_t *lat_rescore;
	float emit_beam;
	double lat_prune_time;
	double fix_scale;
	float conf;
	//=========== post function ==============
	wtk_wfstr_dnn_get_value_f dnn_get;
	void *dnn_ths;
//	wtk_wfstr_notify_f notify;
//	void *notify_ths;
	wtk_feat_t *f;
	//unsigned use_ntok:1;
	unsigned is_forceout:1;
	unsigned use_rescore:1;
	unsigned use_prune:1;
	unsigned end_hint:1;
};

wtk_wfstr_t* wtk_wfstr_new(wtk_wfstrec_cfg_t *cfg,wtk_fst_net_cfg_t *net_cfg,wtk_hmmset_t *hmm);
void wtk_wfstr_delete(wtk_wfstr_t *r);
int wtk_wfstr_bytes(wtk_wfstr_t *r);
void wtk_wfstr_reset(wtk_wfstr_t *r);
int wtk_wfstr_start(wtk_wfstr_t *r,wtk_fst_net_t *net);
int wtk_wfstr_start2(wtk_wfstr_t *r,wtk_fst_net_t *comm_net,wtk_fst_net_t *ebnf_net);
void wtk_wfstr_feed(wtk_wfstr_t *r,wtk_vector_t *obs);
void wtk_wfstr_feed2(wtk_wfstr_t *r,wtk_feat_t *f);
void wtk_wfstr_feed3(wtk_wfstr_t *r,wtk_vector_t *obs);
wtk_string_t* wtk_wfstr_finish(wtk_wfstr_t *r);
void wtk_wfstr_finish2(wtk_wfstr_t *r,wtk_strbuf_t *buf,char *sep,int sep_bytes);
void wtk_wfstr_finish4(wtk_wfstr_t *r,wtk_wfst_path_t *pth,wtk_strbuf_t *buf,char *sep,int sep_bytes);

int wtk_wfstr_create_lat_fst(wtk_wfstr_t *rec,wtk_fst_net2_t *net);
#ifdef USE_ALIGN
wtk_lat_t* wtk_wfst_create_lat(wtk_wfstr_t *rec);
#endif
wtk_wfst_path_t *wtk_wfstr_get_path(wtk_wfstr_t *r);

int wtk_wfstr_bytes(wtk_wfstr_t *r);

int wtk_wfst_path_has_out_id(wtk_wfst_path_t *pth,unsigned int out_id,int frame);
int wtk_wfst_path_has_out_id2(wtk_wfst_path_t *pth,unsigned int out_id);

void wtk_wfstr_set_dnn_handler(wtk_wfstr_t *rec,void *ths,wtk_wfstr_dnn_get_value_f f);

double wtk_wfstr_log_add(wtk_wfstr_t *rec, double x, double y);

void wtk_wfstr_touch_end(wtk_wfstr_t *rec);


void wtk_wfstr_push_inst(wtk_wfstr_t *rec,wtk_fst_inst_t *inst);


float wtk_wfstr_get_conf(wtk_wfstr_t *r);
float wtk_wfstr_get_path_conf(wtk_wfstr_t *r,wtk_wfst_path_t *pth);

wtk_fst_tok_t* wtk_wfstr_get_tok(wtk_wfstr_t *r);
int wtk_wfst_path_last_wrd(wtk_wfst_path_t *pth);

#ifdef USE_ALIGN
void wtk_fst_rec_print_align(wtk_wfstr_t *r,wtk_wfst_path_t *pth);
wtk_transcription_t* wtk_wfst_rec_finish3(wtk_wfstr_t *r);
wtk_transcription_t* wtk_wfstr_get_trans(wtk_wfstr_t *r,float frame_scale);
#endif
int wtk_wfstr_print(wtk_wfstr_t *r);
void wtk_wfstr_print_path(wtk_wfstr_t *r,wtk_wfst_path_t *pth);
void wtk_wfstr_print_path3(wtk_wfstr_t *r,wtk_wfst_path_t *pth);
extern wtk_fst_tok_t fst_null_tok;
#ifdef __cplusplus
};
#endif
#endif
