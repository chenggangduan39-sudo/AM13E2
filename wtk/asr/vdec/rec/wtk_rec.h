#ifndef WTK_DECODER_VITERBI_WTK_REC_H_
#define WTK_DECODER_VITERBI_WTK_REC_H_
#include "wtk/asr/net/wtk_latset.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/wtk_bit_heap.h"
#include "wtk_rec_cfg.h"
#include "wtk/asr/net/wtk_transcription.h"
#include "wtk/asr/fextra/wtk_feat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rec wtk_rec_t;
typedef struct wtk_token wtk_token_t;
typedef struct wtk_path wtk_path_t;
typedef struct wtk_align wtk_align_t;
typedef struct wtk_nxtpath wtk_nxtpath_t;
typedef struct wtk_tokenset wtk_tokenset_t;
typedef struct wtk_netinst wtk_netinst_t;
typedef struct wtk_precomp2 wtk_precomp2_t;
typedef struct wtk_hsetinfo wtk_hsetinfo_t;
typedef struct wtk_observation wtk_observation_t;
typedef struct wtk_reltoken wtk_reltoken_t;

struct wtk_token
{
	double like;		//Likelihood of token
	float lm;			//LM likelihood of token.
	wtk_path_t* path;	//route (word level) through network.
	wtk_align_t *align;	//route(state/model level) through network.
};

struct wtk_reltoken
{
	double like;		//Relative Likelihood of token
	float lm;			//LM likelihood of token.
	wtk_path_t* path;	//route (word level) through network.
	wtk_align_t *align;	//route(state/model level) through network.
};

struct wtk_path
{
	wtk_queue_node_t q_n;
	wtk_queue_t* queue;		//path linked in.
	wtk_path_t *prev;		//Previous word record
	wtk_netnode_t *node;	//word level traceback info.
	wtk_nxtpath_t *chain;	//Next of NBest Paths
	wtk_align_t *align;		//state/model traceback for this word.
	double like;		//likelihood at boundary.
	float lm;			//likelihood of current word.
	int frame;				//time of boundary(end of word)
	int usage;			//times struct refrenced by next path
	unsigned char used:1;	//reference to struct by current inst.
};

struct wtk_nxtpath
{
	wtk_path_t *prev;		//previous word record.
	wtk_align_t *align;
	wtk_nxtpath_t *chain;		//Next of NBest paths
	double like;		//likelihood at boundary.
	float	lm;			//lm likelihood of current word.
};

struct wtk_align
{
	wtk_queue_node_t q_n;
	wtk_queue_t *queue;		//align linked in.
	wtk_align_t *prev;	//previous align record
	wtk_netnode_t *node;	//node for which alignment information present.
	double like;		//likelihood upon entering state/model end
	int	frame;		//frame number upon entering state/model end
	int usage;		//times struct refrenced by align or path
	short state;		//state level traceback info.
	unsigned char used:1;	//refrence to struct by current inst.
};

struct wtk_tokenset
{
	wtk_token_t tok;	//most likely token in state.
	wtk_reltoken_t *set;	//likelihood sorted array[0 ..nToks]
	short n;	//number or valid tokens.
};


struct wtk_netinst
{
	wtk_queue_node_t q_n;
	wtk_netnode_t* node;
	wtk_tokenset_t *state;	//TokenSet[0..N-2] in state [1..N-1] for hmm
	wtk_tokenset_t *exit;	//TokenSet in exit state.
	double wdlk;				//Max likelihood of t=0 path to word end node
	double max;				//Likelihood for pruning of instance
	unsigned char pxd:1;	//external propagation done this frame.
	unsigned char ooo:1;	//instance potentially out of order.
};

struct wtk_precomp2
{
	int id;		//uniqure identifier for current frame.
	float outp;	//state/mixture output likelihood.
};

typedef float(*wtk_rec_dnn_get_value_f)(void *ths, wtk_feat_t *f, int index);

struct wtk_rec
{
	wtk_rec_cfg_t *cfg;
	//----------------------------
	wtk_prune_t *prune;
	wtk_queue_t inst_queue;
	wtk_queue_t path_no_queue;
	wtk_queue_t path_yes_queue;
	wtk_queue_t align_no_queue;
	wtk_queue_t align_yes_queue;
	int nsp;
	int nmp;
	wtk_precomp2_t *sPre;	//array[0...nsp-1] state precomps
	wtk_precomp2_t *mPre;	//array[0...nmp-1] shared mixture precomps.
	wtk_tokenset_t *sBuf;	//buffer array[2..N-1] of tokset for StepHMM1_N
	wtk_netinst_t *nxt_inst;		//Inst used to select next in step sequence.
	wtk_reltoken_t *rtoks_inst2;				//used for step inst2 tokens cache.
	wtk_reltoken_t *rtoks_merge;				//used for merge cahce;
	wtk_nxtpath_t *rths;				//NxtPath Cache.
	//wtk_netnode_t **nodes;				//use for node cache;
	wtk_array_t *node_array;			//used for merge token.
	wtk_vector_t *obs;
	wtk_netnode_t *p_gen_max_node;	//Most likely node in network
	wtk_netnode_t *p_word_max_node;	//Most likely word end node in network
	wtk_token_t p_gen_max_tok;		//Most likely token.
	wtk_token_t p_word_max_tok;		//Most likely word end token.
	int frame;
	int max_state;			//max states in hmm set
	short ***seIndexes;
	double min_log_exp;
	int calign;		//Number of align records after last collection.
	int cpath;		//NUmber of path records after last collection.
	//resource section
	wtk_hmmset_t *hmmlist;
	wtk_lat_t *lat;
	wtk_dict_t *dict;
	//memory management section.
	wtk_bit_heap_t *rel_tok_heap;
	wtk_bit_heap_t *path_heap;
	wtk_bit_heap_t *align_heap;
	wtk_bit_heap_t *nxt_path_heap;
	wtk_heap_t *local_heap;
	wtk_heap_t *global_heap;
	wtk_bit_heap_t *inst_heap;
	wtk_bit_heap_t **tokenset_heap;		//!< valid index from 1 to max_num_state-1(tokenset_heap_maxi-1)
	int tokenset_heap_maxi;
	//---------- hlda matrix section ----------------
	wtk_vector_t *hlda_vector;
	//---------- thread section ----------------------
	float best_emit_score;
	double frame_dur;		//frame duration in seconds.
	double gen_thresh;     //cutoff from global beam.
	double word_thresh;    //cutoff from global beam.
	double n_thresh;       //cutoff for non-best tokens
	unsigned char is_forceout:1;	//MLF is complete or not.
	//---------- post function ------------------------
	wtk_rec_dnn_get_value_f dnn_get;
	void *dnn_ths;
	wtk_feat_t *f;
};

wtk_rec_t* wtk_rec_new(wtk_rec_cfg_t *cfg,wtk_hmmset_t *h,wtk_dict_t *d,double frame_dur);
int wtk_rec_init(wtk_rec_t *rec,wtk_rec_cfg_t *cfg,wtk_hmmset_t *h,wtk_dict_t *d,double frame_dur);
int wtk_rec_delete(wtk_rec_t *rec);

int wtk_rec_clean(wtk_rec_t *rec);

/**
 *	@brief clean temporary memory.
 */
int wtk_rec_reset(wtk_rec_t *rec);

/**
 * @brief initialize network.
 */
int wtk_rec_start(wtk_rec_t *rec,wtk_lat_t *lat);

/**
 * @brief feed observation and do viterbi.
 */
void wtk_rec_feed(wtk_rec_t *rec,wtk_vector_t* obs);
void wtk_rec_feed2(wtk_rec_t *rec,wtk_feat_t* f);

wtk_transcription_t* wtk_rec_finish(wtk_rec_t *rec);
wtk_lat_t* wtk_rec_finish2(wtk_rec_t *rec);
wtk_transcription_t* wtk_rec_transcription_from_lat2(wtk_rec_t *rec,wtk_lat_t *lat);

/**
 *	@return time of wave consumed in seconds;
 */
double wtk_rec_time(wtk_rec_t *rec);
/**
 * @return path index of the tok.
 */
int wtk_token_path_index(wtk_token_t *tok);
void wtk_netnode_trace(wtk_netnode_t *node);
void wtk_rec_trace_best_path(wtk_rec_t *rec);
wtk_transcription_t* wtk_lat_to_transcription(wtk_lat_t *lat,wtk_heap_t *heap,wtk_dict_t *dict,int N,int trace_model,int trace_state);
wtk_transcription_t* wtk_rec_transcription_from_lat(wtk_rec_t *rec,wtk_lat_t *lat);
int wtk_rec_max_is_sil(wtk_rec_t *rec);
void wtk_rec_trace_inst(wtk_rec_t *rec);
void wtk_path_print(wtk_path_t *path);
/**
 * @brief seg dnn post function
 */
void wtk_rec_set_dnn_handler(wtk_rec_t *rec,void *ths,wtk_rec_dnn_get_value_f f);
#ifdef __cplusplus
};
#endif
#endif
