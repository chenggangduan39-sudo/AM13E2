#ifndef WTK_MODEL_WTK_HMMLIST_H_
#define WTK_MODEL_WTK_HMMLIST_H_
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/fextra/wtk_fkind.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_macro.h"
#include "wtk/core/wtk_label.h"
#include "wtk/core/wtk_larray.h"
#include "wtk_hmmset_cfg.h"
#include "wtk/core/math/wtk_fixmath.h"
#ifdef __cplusplus
extern "C" {
#endif
#define wtk_hmmset_find_hmm_s(hl,s) wtk_hmmset_find_hmm(hl,s,sizeof(s)-1)
#define MAX_STREAM_NUMBER 5
enum _DurKind {NULLD, POISSOND, GAMMAD, RELD, GEND};
typedef enum _DurKind DurKind;
typedef enum {     /* Various forms of covariance matrix */
	DIAGC,         /* diagonal covariance */
	INVDIAGC,      /* inverse diagonal covariance */
	FULLC,         /* inverse full rank covariance */
	XFORMC,        /* arbitrary rectangular transform */
	LLTC,          /* L' part of Choleski decomposition */
	NULLC,         /* none - implies Euclidean in distance metrics */
	NUMCKIND       /* DON'T TOUCH -- always leave as final element */
} CovKind;

typedef struct
{
	int vec_size;	//must be matched stream width.
	int nUse;						//usage counter
	float det;						//determinant of lin-xform.
	wtk_int_vector_t* block_size;	//block sizes in the linear transform.
	wtk_smatrix_t** xform;			//1 .. numblocks matrix transfrom.
	wtk_svector_t *bias;			//bias vector
	wtk_svector_t *vFloor;			//used for SEMIT variance flooring.
}wtk_linxform_t;

typedef struct
{
	wtk_string_t *mmfIdMask;		//mask of model sets that appropriate for
	wtk_linxform_t *xform;	//actual transform to applied.
	int nUse;				//usage counter.
	wtk_fkind_t pkind;
	unsigned preQual:1;		//is this applied prior to qualifiers.
}wtk_inputxform_t;

typedef struct
{
	wtk_svector_t *mean;		//mean vector
	wtk_svector_t *variance;	//varian vector, when loaded it will *-0.5f by wtk_mixpdf_post_process
	float fGconst;			//Precomputed component of b(x) when loaded it will *-0.5f by wtk_mixpdf_post_process
	//int used;			// mixture referenced number
	//int index;
	unsigned short used;
	unsigned short index;
}wtk_mixpdf_t;

typedef struct
{
	wtk_mixpdf_t *pdf;		//mixture pinter
	float fWeight;			//mixture weight
}wtk_mixture_t;

typedef struct
{
	wtk_mixture_t *pmixture;	///AIMixture array [0,..nNumMixture-1]
	int nMixture;				//number of AIMixture
}wtk_stream_t;

typedef struct
{
	wtk_string_t *name;
	void *dnn;
	wtk_stream_t *pStream;	//stream info
	wtk_svector_t *pfStreamWeight; /*	steam weight,VectorSize(pfStreamWeght)  is stream width.pfStreamWeighjt[1]...[n] is stream weight for every stream*/
	int used;
	int index;
}wtk_state_t;

typedef struct
{
	wtk_string_t *name;
	wtk_state_t **pState;		//array[2..numStates-1] of StateElem
	wtk_matrix_t *transP;		//transition matrix (logs)
	//float max_trans;
	int tIdx;					//Transition matrix index.
	short num_state;
	//unsigned sil:1;
	//short **seIndex;
}wtk_hmm_t;

typedef struct wtk_hmmset wtk_hmmset_t;

struct wtk_hmmset
{
	wtk_hmmset_cfg_t *cfg;
	double mix_log_exp;
	wtk_str_hash_t *hmm_hash;		//wtk_macro_t => wtk_hmm_t*
	wtk_str_hash_t *mac_hash;
	wtk_heap_t *heap;
	wtk_label_t *label;
	wtk_larray_t *hmm_array;		//wtk_hmm_t* array;
	//wtk_array_t *hmm_array;
	wtk_inputxform_t *xform;
	short stream_width[MAX_STREAM_NUMBER];     /* [0]=num streams,[i]=width of stream i */
	DurKind dkind;
	CovKind ckind;
	short 	pkind;
	short 	vec_size;
	int num_phy_hmm;
	int num_phy_mix;
	int num_states;
	int max_hmm_state;						//!< the max hmm state, used for rec to make tokenset heap.
	short ***seIndexes;
	unsigned load_hmm_from_hmmlist:1;
	unsigned option_set:1;
	unsigned allow_tmods:1;
	unsigned use_le:1;		//MMF is little-endian or big-endian;
};

wtk_hmmset_t* wtk_hmmset_new(wtk_label_t *label);
wtk_hmmset_t* wtk_hmmset_new2(wtk_label_t *l,int hmmlist_hint);
int wtk_hmmset_delete(wtk_hmmset_t *hs);
int wtk_hmmset_init(wtk_hmmset_t *hl,wtk_label_t *label,int hmmlist_hint);
int wtk_hmmset_clean(wtk_hmmset_t *hl);
void wtk_hmmset_set_hmm_load_type(wtk_hmmset_t *h,int load_hmm_from_hmmlist);
int wtk_hmmset_load_list(wtk_hmmset_t *hl,wtk_source_t *s);
int wtk_hmmset_load_list2(wtk_hmmset_t *hl,wtk_source_t *s);
int wtk_hmmset_load_model(wtk_hmmset_t *hl,wtk_source_t *s);
int wtk_hmmset_load_xform(wtk_hmmset_t *hl,wtk_source_t *s);
wtk_macro_t* wtk_hmmset_find_macro(wtk_hmmset_t *hl,char type,char* n,int nl);
void* wtk_hmmset_find_macro_hook(wtk_hmmset_t *hl,char type,char* n,int nl);
int wtk_hmmset_add_macro(wtk_hmmset_t *hl,char type,char* n,int nl,void* hook);
wtk_mixpdf_t* wtk_hmmset_new_mixpdf(wtk_hmmset_t *hl);
wtk_state_t* wtk_hmmset_new_state(wtk_hmmset_t *hl);
wtk_stream_t* wtk_hmmset_new_streams(wtk_hmmset_t *hl,int n);
wtk_mixture_t* wtk_hmmset_new_mixtures(wtk_hmmset_t *hl,int n);
wtk_string_t* wtk_hmmset_find_name(wtk_hmmset_t *hl,char *n,int nl);
wtk_hmm_t* wtk_hmmset_find_hmm(wtk_hmmset_t *hl,char* n,int nl);
float wtk_mixpdf_calc_dia_prob(wtk_mixpdf_t *pdf,wtk_vector_t *obs);
float wtk_mixpdf_calc_dia_prob_fix(wtk_mixpdf_t *pdf,wtk_fixi_t *fix,float scale);
double wtk_hmmset_calc_prob_fix(wtk_hmmset_t *hl,wtk_state_t *state,wtk_fixi_t *fix,float scale);
double wtk_hmmset_calc_prob(wtk_hmmset_t *hl,wtk_state_t *state,wtk_vector_t *obs);
int wtk_hmmset_add_hmm(wtk_hmmset_t *hl,char* l,int lb,char* p,int pb);
int wtk_hmmset_add_hmm2(wtk_hmmset_t *hl,char* p,int pb);
void wtk_hmmset_set_index(wtk_hmmset_t *hl);
/*
 *this function pre mulit -0.5 to variance and save time in wtk_mixpdf_calc_dia_prob
 */
void wtk_mixpdf_post_process(wtk_hmmset_t *set,wtk_mixpdf_t *pdf);

void wtk_hmmset_print(wtk_hmmset_t *hl);
void wtk_inputxform_print(wtk_inputxform_t* xf);
void wtk_hmm_print(wtk_hmm_t *hmm);
double wtk_log_add(double x, double y,double min_log_exp);
void wtk_hmmset_transpose_trans_matrix(wtk_hmmset_t *hl);
void wtk_hmmset_transpose_trans_matrix2(wtk_hmmset_t *hl,float trans_scale);
int wtk_hmmset_bytes(wtk_hmmset_t *hs);

wtk_stream_t* wtk_stream_dup(wtk_stream_t *src,wtk_heap_t *heap);

void wtk_mixpdf_print(wtk_mixpdf_t *pdf);
#ifdef __cplusplus
};
#endif
#endif
