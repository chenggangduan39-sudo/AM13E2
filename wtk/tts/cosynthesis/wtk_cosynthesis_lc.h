#ifndef WTK_COSYN_LC
#define WTK_COSYN_LC
#include "wtk/core/wtk_type.h" 
#include "wtk_cosynthesis_hmm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cosynthesis_lc wtk_cosynthesis_lc_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_cosynthesis_hmm_t *hmm;
	wtk_string_t *name;

    wtk_matrix_t* durmean; /*5 state dur mean*/
    wtk_matrix_t* durvariance; /*5 state dur var*/
    int totaldur;	/* total duration */

    wtk_matrix_t* lf0mean;	/* mean vector of log f0 pdfs for each state */
    wtk_matrix_t* lf0variance; /* variance vector of log f0 for each state */
    wtk_matrix_t* lf0weight;

    wtk_matrix_t *mcepmean;	/* mean vector of mel-cepstrum pdfs for each state */
    wtk_matrix_t *mcepvariance;/* variance vector of mel-cepstrum for each state */

    wtk_matrix_t *bapmean;	  /* mean vector of aperiodicity pdfs for each state */
    wtk_matrix_t *bapvariance; /* variance vector of aperiodicity for each state */

    wtk_matrix_t* conca_lf0mean;	/* mean vector of log f0 pdfs for each state */
    wtk_matrix_t* conca_lf0variance; /* variance vector of log f0 for each state */
    wtk_matrix_t* conca_lf0weight;

    wtk_matrix_t *conca_mcepmean;	/* mean vector of mel-cepstrum pdfs for each state */
    wtk_matrix_t *conca_mcepvariance;/* variance vector of mel-cepstrum for each state */
    // wtk_tts_xphn_t *phn;
}wtk_cosynthesis_hmm_lc_t;

struct wtk_cosynthesis_lc
{
	wtk_cosynthesis_hmm_t *hmm;
	wtk_queue_t lc_q;
	int totframe;
};

wtk_cosynthesis_hmm_lc_t* wtk_cosynthesis_hmm_lc_new(wtk_cosynthesis_hmm_t *hmm,wtk_heap_t *heap,char *name,int name_bytes);
void wtk_cosynthesis_hmm_lc_print(wtk_cosynthesis_hmm_lc_t *lc);
void wtk_cosynthesis_hmm_lc_set_output_pdfs(wtk_cosynthesis_hmm_lc_t *lc,char *s,int s_bytes);
void wtk_cosynthesis_hmm_lc_print(wtk_cosynthesis_hmm_lc_t *lc);
void wtk_cosynthesis_hmm_lc_find_durpdf(wtk_cosynthesis_hmm_lc_t *lc,int *idx);


wtk_cosynthesis_lc_t* wtk_cosynthesis_lc_new(wtk_heap_t *heap,wtk_cosynthesis_hmm_t *hmm);
void wtk_cosynthesis_lc_reset(wtk_cosynthesis_lc_t *lc);
void wtk_cosynthesis_lc_set_gv_pdfs(wtk_cosynthesis_lc_t *lc,char *s,int s_bytes);


wtk_queue_node_t* wtk_cosynthesis_hmm_lc_find_setdur(wtk_cosynthesis_hmm_lc_t *lc, wtk_queue_node_t *n);

#ifdef __cplusplus
};
#endif
#endif
