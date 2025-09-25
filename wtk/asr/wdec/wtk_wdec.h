#ifndef WTK_ASR_WDEC_WTK_WDEC
#define WTK_ASR_WDEC_WTK_WDEC
#include "wtk/core/wtk_type.h" 
#include "wtk_wdec_cfg.h"
#include "wtk_wdec_net.h"
#include "wtk/core/wtk_vpool.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_wdec wtk_wdec_t;

typedef struct wtk_wdec_pth wtk_wdec_pth_t;
typedef struct wtk_wdec_tok wtk_wdec_tok_t;

struct wtk_wdec_tok
{
	wtk_wdec_pth_t *path;
	float like;
	float dnn_like;
};


struct wtk_wdec_pth
{
	wtk_wdec_arc_t *arc;
	wtk_wdec_pth_t *prev;	//Previous word record
	float like;
	float dnn_like;
	unsigned int frame;
};


typedef struct
{
	wtk_queue_node_t q_n;
	wtk_queue_node_t arc_n;
	wtk_wdec_arc_t *arc;
	wtk_wdec_tok_t *state;
	int sframe;
	int nactive;
}wtk_wdec_inst_t;


struct wtk_wdec
{
	wtk_wdec_cfg_t *cfg;
	wtk_wdec_post_cfg_t *post;
	wtk_wdec_net_t *net;
	wtk_vpool_t *inst_pool;
	wtk_vpool_t *pth_pool;
	wtk_vpool_t *tok_pool;
	//wtk_heap_t *heap;
	wtk_prune_t *prune;
	wtk_fextra_t *fextra;
	wtk_queue_t feat_q;
	wtk_queue_t inst_q;
	wtk_vector_t *obs;
	wtk_wdec_tok_t *bk_tok;
	wtk_wdec_pth_t *final_pth;
	wtk_hmm_t *sil_hmm;
	int max_hmm_state;
	float conf;
	int wake_sframe;
	int wake_eframe;
	int nframe;
	float emit_thresh;
	unsigned found:1;
};

wtk_wdec_t* wtk_wdec_new(wtk_wdec_cfg_t *cfg);
int wtk_wdec_bytes(wtk_wdec_t *wdec);
void wtk_wdec_delete(wtk_wdec_t *w);
void wtk_wdec_start(wtk_wdec_t *w,wtk_wdec_post_cfg_t *post);
void wtk_wdec_reset(wtk_wdec_t *w);
void wtk_wdec_reset2(wtk_wdec_t *w);
void wtk_wdec_reset_post(wtk_wdec_t *w);
int wtk_wdec_feed(wtk_wdec_t *w,char *data,int len,int is_end);
void wtk_wdec_pth_print(wtk_wdec_pth_t *pth);
int wtk_wdec_get_final_time(wtk_wdec_t *w,float *fs,float *fe);
void wtk_wdec_print(wtk_wdec_t *w);
float wtk_wdec_get_conf(wtk_wdec_t *w);
void wtk_wdec_set_words(wtk_wdec_t *w, char *words,int n);
int wtk_wdec_reset_check(wtk_wdec_t *w);
#ifdef __cplusplus
};
#endif
#endif
