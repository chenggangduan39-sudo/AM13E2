#ifndef WTK_TTS_SYN_WTK_SYN_PSTREAM
#define WTK_TTS_SYN_WTK_SYN_PSTREAM
#include "wtk/core/wtk_type.h" 
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk_syn_hmm.h"
#include "wtk_syn_dwin.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	//wtk_matrix_t *R;/* W' U^-1 W  */
	wtk_matf_t *R;
	wtk_vector_t *r;/* W' U^-1 mu */
	wtk_vector_t *g;/* for forward substitution */
	wtk_vector_t *b;/* for GV gradient */
}wtk_syn_smat_t;

wtk_syn_smat_t* wtk_syn_smat_new(int t,int width);

typedef struct
{
	int vsize;/* vector size of observation vector (include static and dynamic features) */
	int dim;/* vector size of static features */
	int T;/* length */
	int width;/* width of dynamic window */
	wtk_syn_dwin_t *dw;/* dynamic window */
	wtk_syn_smat_t *sm;/* matrices for parameter generation */
	wtk_vector_t *vm;
	wtk_vector_t *vv;

	wtk_matf_t *mseq;/* sequence of mean vector */
	wtk_matf_t *ivseq;/* sequence of invarsed variance vector */
	wtk_matf_t *par;/* output parameter vector */

//	wtk_matf_t *mseq;/* sequence of mean vector */
//	wtk_matf_t *ivseq;/* sequence of invarsed variance vector */
//	wtk_matf_t *par;/* output parameter vector */

	float coef1;
	float coef2;
}wtk_syn_pstream_t;

wtk_syn_pstream_t* wtk_syn_pstream_new(int vsize,int t,wtk_syn_dwin_t *dwin,wtk_syn_gv_pdf_t *gvpdf);
void wtk_syn_pstream_delete(wtk_syn_pstream_t *s);

// generate parameter sequence from pdf sequence using gradient
void wtk_syn_pstream_mlpg_grannw(wtk_syn_pstream_t *s,int max,float th,float e,float alpha,int nrmflag,int vcflag);
#ifdef __cplusplus
};
#endif
#endif
