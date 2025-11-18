#ifndef WTK_VITE_F0_GETF0_H_
#define WTK_VITE_F0_GETF0_H_
#include "wtk/core/wtk_type.h"
#include <stdlib.h>
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif
#define ckalloc(x) malloc(x)
#define ckfree(x) free(x)
#define ckrealloc(x,y) realloc(x,y)
/* Possible values returned by the function f0(). */
#define F0_OK		0
#define F0_NO_RETURNS	1
#define F0_TOO_FEW_SAMPLES	2
#define F0_NO_INPUT	3
#define F0_NO_PAR	4
#define F0_BAD_PAR	5
#define F0_BAD_INPUT	6
#define F0_INTERNAL_ERR	7

/* Bits to specify optional pre-conditioning of speech signals by f0() */
/* These may be OR'ed together to specify all preprocessing. */
#define F0_PC_NONE	0x00		/* no pre-processing */
#define F0_PC_DC	0x01		/* remove DC */
#define F0_PC_LP2000	0x02		/* 2000 Hz lowpass */
#define F0_PC_HP100	0x04		/* 100 Hz highpass */
#define F0_PC_AR	0x08		/* inf_order-order LPC inverse filter */
#define F0_PC_DIFF	0x010		/* 1st-order difference */


/* wave parameters */
#define WPAR_RATE		16000			/* sample rate */
#define WPAR_SIZE		2
#define WPAR_LENGTH		0
#define WPAR_SWAP		0
#define WPAR_HEAD_PAD	0
#define WPAR_TAIL_PAD	0
#define WPAR_STARTPOS	0
#define WPAR_NAN		-1
#define WPAR_PADDING	0

/* getf0 parameters
* refer to Talkin, A Robust Algorithm for Pitch Tracking (RAPT)
*/
#define PAR_CAND_THRESH		0.3F				/* minimum acceptable peak value in NCCF */
#define PAR_LAG_WEIGHT		0.3F				/* linear lag taper factor for NCCF */
#define PAR_FREQ_WEIGHT		0.02F				/* cost factor for F0 change */
#define PAR_TRANS_COST		0.005F				/* fixed voicing-state transition cost */
#define	PAR_TRANS_AMP		0.5F				/* delta amplitude modulated transition cost */
#define PAR_TRANS_SPEC		0.5F				/* delta spectrum modulated transition cost */
#define PAR_VOICE_BIAS		0.0F				/* bias to encourage voiced hypotheses */
#define PAR_DOUBLE_COST		0.35F				/* cost of extract F0 doubling or halving */
#define PAR_MIN_F0			50;					/* minimum F0 to search for (Hz) */
#define PAR_MAX_F0			550					/* maximum F0 to search for (Hz) */
#define PAR_FRAME_STEP		0.01F				/* t, analysis frame step size (sec) */
#define PAR_WIND_DUR		0.0075F				/* w, correlation window size (sec) */
#define PAR_N_CANDS			20					/* max. number of hypotheses at each frame */


#define READ_SIZE		0.02 			/* length of input data frame in sec to read */
#define DP_CIRCULAR		1.5				/* determines the initial size of DP circular buffer in sec */
#define DP_HIST			0.1				/* stored frame history in second before checking for common path
 DP_CIRCULAR > READ_SIZE, DP_CIRCULAR at least 2 times of DP_HIST
*/
#define DP_LIMIT		1.0				/* in case no convergence is found, DP frames of DP_LIMIT secs
* are kept before output is forced by simply picking the lowest cost
* path
*/

/*
* stationarity parameters -
*/
#define STAT_WSIZE		0.030			/* window size in sec used in measuring frame energy/stationarity */
#define STAT_AINT		0.020			/* analysis interval in sec in measuring frame energy/stationarity */

#define DOWNSAMPLER_LENGTH 0.005F /* 0.005 is the filter length used in down sampler */


#ifndef TRUE
# define TRUE 1
# define FALSE 0
#endif
#ifndef FLT_MAX
#define FLT_MAX (3.40282347E+36f)    //just a  big float
#endif
#ifndef M_PI
# define M_PI (3.1415926536f)
#endif

/* Some definitions used by the "Pitch Tracker Software". */
typedef struct f0_params {
float cand_thresh,	/* only correlation peaks above this are considered */
      lag_weight,	/* degree to which shorter lags are weighted */
      freq_weight,	/* weighting given to F0 trajectory smoothness */
      trans_cost,	/* fixed cost for a voicing-state transition */
      trans_amp,	/* amplitude-change-modulated VUV trans. cost */
      trans_spec,	/* spectral-change-modulated VUV trans. cost */
      voice_bias,	/* fixed bias towards the voiced hypothesis */
      double_cost,	/* cost for octave F0 jumps */
      mean_f0,		/* talker-specific mean F0 (Hz) */
      mean_f0_weight,	/* weight to be given to deviations from mean F0 */
      min_f0,		/* min. F0 to search for (Hz) */
      max_f0,		/* max. F0 to search for (Hz) */
      frame_step,	/* inter-frame-interval (sec) */
      wind_dur;		/* duration of correlation window (sec) */
int   n_cands,		/* max. # of F0 cands. to consider at each frame */
      conditioning;     /* Specify optional signal pre-conditioning. */
	  float dp_circular;
	  float dp_hist;
} F0_params;

typedef struct cross_rec { /* for storing the crosscorrelation information */
	float	rms;	/* rms energy in the reference window */
	float	maxval;	/* max in the crosscorr. fun. q15 */
	short	maxloc; /* lag # at which max occured	*/
	short	firstlag; /* the first non-zero lag computed */
	float	*correl; /* the normalized corsscor. fun. q15 */
} Cross;

typedef struct dp_rec { /* for storing the DP information */
	short	ncands;	/* # of candidate pitch intervals in the frame */
	short	*locs; /* locations of the candidates */
	float	*pvals; /* peak values of the candidates */
	float	*mpvals; /* modified peak values of the candidates */
	short	*prept; /* pointers to best previous cands. */
	float	*dpvals; /* cumulative error for each candidate */
} Dprec;

#define BIGSORD 100

typedef struct windstat_rec {  /* for lpc stat measure in a window */
    float rho[BIGSORD+1];
    float err;
    float rms;
} Windstat;

typedef struct sta_rec {  /* for stationarity measure */
  float *stat;
  float *rms;
  float *rms_ratio;
} Stat;


typedef struct frame_rec{
  Cross *cp;
  Dprec *dp;
  float rms;
  struct frame_rec *next;
  struct frame_rec *prev;
} Frame;

/* wave data structure */
typedef struct _wav_params {
  char *file;
  int rate;
  int startpos;
  int nan;
  int size;
  int swap;
  int padding;
  int length;
  int head_pad;
  int tail_pad;
  float *data;
} wav_params;

/* output data structure */
typedef struct _out_params {
  int nframe;
  float *f0p, *vuvp, *rms_speech, *acpkp;
} out_params;

/* unit size of IO buffer */
#define INBUF_LEN 1024

/* IO buffer structure */
typedef struct _bufcell{
  int len;
  short *data;
  struct _bufcell *next;
} bufcell;

#define eround(flnum) (((flnum) >= 0.0) ? (int)((flnum) + 0.5) : (int)((flnum) - 0.5))
/* A fundamental frequency estimation algorithm using the normalized
cross correlation function and dynamic programming.  The algorithm
implemented here is similar to that presented by B. Secrest and
G. Doddington, "An integrated pitch tracking algorithm for speech
systems", Proc. ICASSP-83, pp.1352-1355.  It is fully described
by D. Talkin, "A robust algorithm for ptich tracking (RAPT)", in
W. B. Kleijn & K. K. Paliwal (eds.) Speech Coding and Synthesis,
(New York: Elsevier, 1995). */

/* For each frame, up to par->n_cands cross correlation peaks are
considered as F0 intervals.  Each is scored according to its within-
frame properties (relative amplitude, relative location), and
according to its connectivity with each of the candidates in the
previous frame.  An unvoiced hypothesis is also generated at each
frame and is considered in the light of voicing state change cost,
the quality of the cross correlation peak, and frequency continuity. */

/* At each frame, each candidate has associated with it the following
items:
its peak value
its peak value modified by its within-frame properties
its location
the candidate # in the previous frame yielding the min. err.
(this is the optimum path pointer!)
its cumulative cost: (local cost + connectivity cost +
cumulative cost of its best-previous-frame-match). */

/* Dynamic programming is then used to pick the best F0 trajectory and voicing
state given the local and transition costs for the entire utterance. */

/* To avoid the necessity of computing the full crosscorrelation at
the input sample rate, the signal is downsampled; a full ccf is
computed at the lower frequency; interpolation is used to estimate the
location of the peaks at the higher sample rate; and the fine-grained
ccf is computed only in the vicinity of these estimated peak
locations. */

void xautoc(int windowsize, float *s, int p, float *r, float *e);
void xdurbin(float *r, float *k, float *a, int p, float *ex);
void xa_to_aca(float *a, float *b, float *c, int p);
float xitakura(int p, float *b, float *c, float *r, float *gain);
int lc_lin_fir(float fc, int *nf, float *coef);
void peak(float *y, float *xp, float *yp);
int get_Nframes(long buffsize,int pad,int step);
void xrwindow(float *din, float *dout, int n, float preemp);
void get_cand(Cross *cross,float *peak,int *loc,int nlags,int *ncand,float cand_thresh);
#ifdef __cplusplus
};
#endif
#endif
