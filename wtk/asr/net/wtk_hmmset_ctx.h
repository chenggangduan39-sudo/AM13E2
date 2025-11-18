#ifndef WTK_DECODER_NET_WTK_HMMSET_CTX_H_
#define WTK_DECODER_NET_WTK_HMMSET_CTX_H_
#include "wtk/asr/model/wtk_hmmset.h"
#include "wtk/core/wtk_hoard.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_hmmset_ctx wtk_hmmset_ctx_t;
typedef struct wtk_hmmref wtk_hmmref_t;

struct wtk_hmmref
{
	hash_str_node_t hash_n;
	wtk_queue_node_t ctx_n;
	int count;
};

struct wtk_hmmset_ctx
{
	wtk_hoard_t cis_hoard;
	wtk_hmmset_t *hl;
	wtk_str_hash_t *cxs_hash; //context label hash.
	wtk_str_hash_t *cis_hash; //independent label hash.
	wtk_str_hash_t *cfs_hash; //free label hash.
	int nc;		//number of contexts.
	int xc;		//number of cross word context.
	int nci;	//number of context independent models.
	int ncf;	//number of context free models.
	unsigned s_left:1;	//seen left context dependency.
	unsigned s_right:1;	//seen right context dependency.
};

wtk_hmmset_ctx_t* wtk_hmmset_ctx_new(wtk_hmmset_t *hl,int ctx_ind);
int wtk_hmmset_ctx_delete(wtk_hmmset_ctx_t *ctx);
int wtk_hmmset_ctx_init(wtk_hmmset_ctx_t *hc,wtk_hmmset_t *hl,int ctx_ind);
int wtk_hmmset_ctx_clean(wtk_hmmset_ctx_t *hc);
int wtk_hmmset_ctx_define_ctx(wtk_hmmset_ctx_t *hc);
void wtk_hmm_strip_name(wtk_string_t *src,wtk_string_t *dst);
int wtk_hmmset_ctx_is_hic_ind(wtk_hmmset_ctx_t *hc,wtk_string_t *name);
#ifdef __cplusplus
};
#endif
#endif
