#ifndef WTK_COSYN_HMM
#define WTK_COSYN_HMM
#include "wtk/core/wtk_type.h" 
#include "wtk_cosynthesis_hmm_cfg.h"
#include "wtk/core/wtk_strpool.h"
#include "wtk_cosynthesis_dtree.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cosynthesis_hmm wtk_cosynthesis_hmm_t;

typedef struct
{
	wtk_vector_t *var;
	wtk_vector_t *mean;
}wtk_syn_gv_pdf_t;

struct wtk_cosynthesis_hmm
{
	wtk_cosynthesis_hmm_cfg_t *cfg;
	wtk_rbin2_t *rbin;
	int f0_pos;
	int bap_pos;
    int conca_f0_pos;
    int conca_nstate;

	wtk_strpool_t *pool;
	wtk_cosynthesis_dtree_t *dtree;
	int ntree;

	int nstate;					/* # of HMM states for individual HMM */
	int *ndurpdf;				/* # of pdfs for duration */
	float ***durpdf;	//ntree -> npdf -> mean and variance

	int lf0stream;
    int *nlf0tree;            /* # of trees for each state position (log F0) */
    int **nlf0pdf;            /* # of pdfs for each state position (log F0) */
    float *****lf0pdf;	/* read pdfs (mean, variance & weight) */

    int lf0gvdim;
    int nlf0gvtree;
    int *nlf0gvpdf;
    float ***lf0gvpdf;

    int mcepvsize;            /* vector size for mcep modeling */
    int *nmceptree;           /* # of trees for each state position (mcep) */
    int **nmceppdf;           /* # of pdfs for each state position (mcep) */
    float ****mceppdf;

    int mcepgvdim;
    int nmcepgvtree;
    int *nmcepgvpdf;
    float ***mcepgvpdf;

    int bapvsize;
    int *nbaptree;
    int **nbappdf;
    float ****bappdf;

    int bapgvdim;
    int nbapgvtree;
    int *nbapgvpdf;
    float ***bapgvpdf;

	int conca_lf0stream;
    int *conca_nlf0tree;            /* # of trees for each state position (log F0) */
    int **conca_nlf0pdf;            /* # of pdfs for each state position (log F0) */
    float *****conca_lf0pdf;	/* read pdfs (mean, variance & weight) */

    int conca_mcepvsize;            /* vector size for mcep modeling */
    int *conca_nmceptree;           /* # of trees for each state position (mcep) */
    int **conca_nmceppdf;           /* # of pdfs for each state position (mcep) */
    float ****conca_mceppdf;
};

wtk_cosynthesis_hmm_t* wtk_cosynthesis_hmm_new(wtk_cosynthesis_hmm_cfg_t *cfg,wtk_source_loader_t *sl,wtk_strpool_t *pool,wtk_cosynthesis_dtree_t *dtree);
void wtk_cosynthesis_hmm_delete(wtk_cosynthesis_hmm_t *hmm);

wtk_syn_gv_pdf_t* wtk_syn_gv_pdf_new(wtk_heap_t *heap,int vsize);

float* wtk_cosynthesis_hmm_get_durpdf(wtk_cosynthesis_hmm_t *hmm,int idx1,int idx2);
float** wtk_cosynthesis_hmm_get_lf0pdf(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2);
float* wtk_cosynthesis_hmm_get_lf0gvpdf(wtk_cosynthesis_hmm_t *hmm,int idx1,int idx2);
float* wtk_cosynthesis_hmm_get_mcp(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2);
float* wtk_cosynthesis_hmm_get_bap(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2);
float** wtk_cosynthesis_hmm_get_conca_lf0pdf(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2);
float* wtk_cosynthesis_hmm_get_conca_mcp(wtk_cosynthesis_hmm_t *hmm,int state,int idx1,int idx2);

#ifdef __cplusplus
};
#endif
#endif
