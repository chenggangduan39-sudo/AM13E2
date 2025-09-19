#ifndef WTK_MATH_WTK_SIGP_H_
#define WTK_MATH_WTK_SIGP_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk_fextra_cfg.h"
#include "wtk/core/fft/wtk_rfft.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_sigp wtk_sigp_t;

typedef struct
{
	int frameSize;       /* speech frameSize */
	int numChans;        /* Number of filter bank channels */
	long sampPeriod;     /* sample period */
	int fftN;            /* fft size */
	int klo,khi;         /* lopass to hipass cut-off fft indices */
	float fres;			 /* scaled fft resolution */
	wtk_vector_t* cf;           /* array[1..pOrder+1] of centre freqs */
	wtk_short_vector_t* loChan;     /* array[1..fftN/2] of loChan index */
	wtk_vector_t* loWt;         /* array[1..fftN/2] of loChan weighting */
	wtk_vector_t* x;            /* array[1..fftN] of fftchans */
	unsigned usePower:1;		 /* use power rather than magnitude */
	unsigned takeLogs:1;		 /* log filterbank channels */
}FBankInfo;

struct wtk_sigp
{
	wtk_fextra_cfg_t *cfg;
	wtk_vector_t *ham;				/* ham vector */
    wtk_vector_t *povey;                /* povey vector */
	wtk_vector_t* fbank;      				/* filterbank vector */
	FBankInfo fbInfo;  				/* FBank info used for filterbank analysis */
	wtk_vector_t * c;            			/* cepstral vector */
	wtk_vector_t *cepWin;       /* Current cepstral weight window */
	int cepWinSize;            /* Size of current cepstral weight window */
	int cepWinL;               /* Current liftering coeff */

	wtk_rfft_t *rfft;
	float *rfft_fft;
	wtk_vector_t *as;   		/* Auditory*/
	wtk_vector_t *ac;			//autocorrelation
	wtk_vector_t *acb;			//autocorrelation back
	wtk_vector_t *lp;			//lp vectors for PLP
	wtk_vector_t *eql;  		/* Equal loundness curve */
	wtk_double_matrix_t *cm;	/* Cosine matrix for IDFT */
};

int wtk_sigp_init(wtk_sigp_t *s,wtk_fextra_cfg_t *cfg);
int wtk_sigp_clean(wtk_sigp_t *s);
int wtk_sigp_procss(wtk_sigp_t *s,wtk_vector_t *v,float *feature);
#ifdef __cplusplus
};
#endif
#endif
