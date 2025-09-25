#ifndef WTK_TTS_SYN_WTK_SYN_LC
#define WTK_TTS_SYN_WTK_SYN_LC
#include "wtk/core/wtk_type.h" 
#include "wtk/tts/parser/wtk_tts_def.h"
#include "wtk_syn_hmm.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_lc wtk_syn_lc_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_syn_hmm_t *hmm;
	wtk_string_t *name;
    int *dur;		/* duration for each state */
    int *durpdf;	/* duration pdf index */
    int totaldur;	/* total duration */

    wtk_matrix_t* lf0mean;	/* mean vector of log f0 pdfs for each state */
    wtk_matrix_t* lf0variance; /* variance vector of log f0 for each state */
    char *voiced;/* voiced/unvoiced decision for each state */

    wtk_matrix_t *mcepmean;	/* mean vector of mel-cepstrum pdfs for each state */
    wtk_matrix_t *mcepvariance;/* variance vector of mel-cepstrum for each state */

    wtk_matrix_t *bapmean;	  /* mean vector of aperiodicity pdfs for each state */
    wtk_matrix_t *bapvariance; /* variance vector of aperiodicity for each state */

    wtk_tts_xphn_t *phn;
}wtk_syn_hmm_lc_t;

struct wtk_syn_lc
{
	wtk_syn_hmm_t *hmm;
	wtk_queue_t lc_q;
	int totframe;
	wtk_syn_gv_pdf_t *lf0gv;
	wtk_syn_gv_pdf_t *mcegv;
	wtk_syn_gv_pdf_t *bapgv;
	unsigned use_lf0gv:1;
	unsigned use_mcegv:1;
	unsigned use_bapgv:1;
};

wtk_syn_hmm_lc_t* wtk_syn_hmm_lc_new(wtk_syn_hmm_t *hmm,wtk_heap_t *heap,char *name,int name_bytes);
void wtk_syn_hmm_lc_print(wtk_syn_hmm_lc_t *lc);
void wtk_syn_hmm_lc_set_output_pdfs(wtk_syn_hmm_lc_t *lc,char *s,int s_bytes);
void wtk_syn_hmm_lc_print(wtk_syn_hmm_lc_t *lc);
void wtk_syn_hmm_lc_find_durpdf(wtk_syn_hmm_lc_t *lc,float rho,int *idx,float *diff);


wtk_syn_lc_t* wtk_syn_lc_new(wtk_heap_t *heap,wtk_syn_hmm_t *hmm);
void wtk_syn_lc_reset(wtk_syn_lc_t *lc);
void wtk_syn_lc_set_gv_pdfs(wtk_syn_lc_t *lc,char *s,int s_bytes);


wtk_queue_node_t* wtk_syn_hmm_lc_find_setdur(wtk_syn_hmm_lc_t *lc, wtk_queue_node_t *n);

#ifdef __cplusplus
};
#endif
#endif
