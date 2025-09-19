#ifndef WTK_TTS_SYN_WTK_SYN_HMM
#define WTK_TTS_SYN_WTK_SYN_HMM
#include "wtk/core/wtk_type.h" 
#include "wtk_syn_hmm_cfg.h"
#include "wtk/core/wtk_strpool.h"
#include "wtk_syn_dtree.h"
#include "wtk_syn_def.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_hmm wtk_syn_hmm_t;

typedef struct
{
	wtk_vector_t *var;
	wtk_vector_t *mean;
}wtk_syn_gv_pdf_t;


struct wtk_syn_hmm
{
	wtk_syn_hmm_cfg_t *cfg;
	wtk_rbin2_t *rbin;
	wtk_rbin2_item_t *rbin_item_dur;
	wtk_rbin2_item_t *rbin_item_f0;
	wtk_rbin2_item_t *rbin_item_f0gv;
	wtk_rbin2_item_t *rbin_item_mcp;
	wtk_rbin2_item_t *rbin_item_bap;
	int f0_pos;
	int bap_pos;

	wtk_strpool_t *pool;
	wtk_syn_dtree_t *dtree;
	int ntree;

	int nstate;					/* # of HMM states for individual HMM */
	int *ndurpdf;				/* # of pdfs for duration */
	wtk_syn_float_t ***durpdf;	//ntree -> npdf -> mean and variance

	int lf0stream;
    int *nlf0tree;            /* # of trees for each state position (log F0) */
    int **nlf0pdf;            /* # of pdfs for each state position (log F0) */
    wtk_syn_float_t *****lf0pdf;	/* read pdfs (mean, variance & weight) */

    int lf0gvdim;
    int nlf0gvtree;
    int *nlf0gvpdf;
    wtk_syn_float_t ***lf0gvpdf;

    int mcepvsize;            /* vector size for mcep modeling */
    int *nmceptree;           /* # of trees for each state position (mcep) */
    int **nmceppdf;           /* # of pdfs for each state position (mcep) */
    wtk_syn_float_t ****mceppdf;

    int mcepgvdim;
    int nmcepgvtree;
    int *nmcepgvpdf;
    wtk_syn_float_t ***mcepgvpdf;

    int bapvsize;
    int *nbaptree;
    int **nbappdf;
    wtk_syn_float_t ****bappdf;

    int bapgvdim;
    int nbapgvtree;
    int *nbapgvpdf;
    wtk_syn_float_t ***bapgvpdf;
};

wtk_syn_hmm_t* wtk_syn_hmm_new(wtk_syn_hmm_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool,wtk_syn_dtree_t *dtree);
void wtk_syn_hmm_delete(wtk_syn_hmm_t *hmm);

wtk_syn_gv_pdf_t* wtk_syn_gv_pdf_new(wtk_heap_t *heap,int vsize);

wtk_syn_float_t* wtk_syn_hmm_get_durpdf(wtk_syn_hmm_t *hmm,int idx1,int idx2);
wtk_syn_float_t** wtk_syn_hmm_get_lf0pdf(wtk_syn_hmm_t *hmm,int state,int idx1,int idx2);
wtk_syn_float_t* wtk_syn_hmm_get_lf0gvpdf(wtk_syn_hmm_t *hmm,int idx1,int idx2);
wtk_syn_float_t* wtk_syn_hmm_get_mcp(wtk_syn_hmm_t *hmm,int state,int idx1,int idx2);
wtk_syn_float_t* wtk_syn_hmm_get_bap(wtk_syn_hmm_t *hmm,int state,int idx1,int idx2);
#ifdef __cplusplus
};
#endif
#endif
