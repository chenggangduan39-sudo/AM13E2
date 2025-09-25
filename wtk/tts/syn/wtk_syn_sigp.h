#ifndef WTK_TTS_SYN_WTK_SYN_SIGP
#define WTK_TTS_SYN_WTK_SYN_SIGP
#include "wtk/core/wtk_type.h" 
#include "wtk_syn_cfg.h"
#include "wtk_syn_fft.h"
#include "wtk/core/math/wtk_mat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_syn_sigp wtk_syn_sigp_t;

#ifndef PI
#define PI		3.1415926535897932385
#endif
#define SQUARE(x) ((x) * (x))
#define strveq(s1, s2) ((s1 != NULL) && (s2 != NULL) && (strncmp((s1), (s2), strlen(s2)) == 0) ? 1 : 0)
#define sp_warning 0
#define ALITTLE_NUMBER 1.0e-10
#define CABS(xr, xi) sqrt((double)(xr)*(double)(xr)+(double)(xi)*(double)(xi))
#define CSQUARE(xr, xi) ((xr)*(xr)+(xi)*(xi))
#define SYN_MAX(a, b) ((a) > (b) ? (a) : (b))
#define SYN_MIN(a, b) ((a) < (b) ? (a) : (b))

typedef void (*wtk_tts_sigp_f)(void *ths,char *data,int bytes);
typedef void (*wtk_tts_notify_f)(void *ths,char *data,int bytes);

typedef struct
{
	int len;
	wtk_syn_float_t *data;
	wtk_syn_float_t *imag;
}wtk_syn_vector_t;

wtk_syn_vector_t* wtk_syn_vector_new(int len,float v);
wtk_syn_vector_t* wtk_syn_vector_dup(wtk_syn_vector_t *x);
void wtk_syn_vector_delete(wtk_syn_vector_t *sv);
wtk_syn_vector_t* wtk_syn_vector_new_h(wtk_heap_t *h,int len, float v);

typedef struct
{
	int order;
	int iprd;
	int pd;
	wtk_syn_float_t alpha;
	wtk_syn_float_t alpha2;
	wtk_syn_float_t beta;
	wtk_syn_float_t pade[21];
	wtk_syn_float_t *ppade;
	wtk_syn_float_t *pb;
	wtk_syn_float_t *cb;
	wtk_syn_float_t *binc;
	wtk_syn_float_t *d1;
	int irleng;
	wtk_syn_float_t *pfd;
	wtk_syn_float_t *pfg;
	wtk_syn_float_t *pfmc;
	wtk_syn_float_t *pfcep;
	wtk_syn_float_t *pfir;
	int ncpy;
}wtk_syn_vocoder_t;

/* pulse with random phase */
typedef struct
{
	wtk_syn_fft_t *fft;
	wtk_syn_float_t gdbw;	//band width
	wtk_syn_float_t gdsd;	//standard deviation
	wtk_syn_float_t cornf;	//lower corner frequency
	wtk_syn_float_t fs;
	long fftl;
	wtk_syn_vector_t *phstransw;	//smoothing phase transition window;
	wtk_syn_vector_t *fgdsw;		//smoothing window for group delay in frequency domain
	wtk_syn_vector_t *gdwt;
	wtk_syn_vector_t *gd;			//group delay
	wtk_syn_vector_t *apf;			//all-pass filter
}wtk_rphase_t;

typedef struct
{
	long x;
	long a;
	long c;
	long m;
	float dvm;
}wtk_syn_rand_t;



struct wtk_syn_sigp
{
	wtk_syn_cfg_t *cfg;
	wtk_syn_fft_t *fft;
	int len;
	int hlen;
	float f0min;
	float uvf0;
	float volume_scale;
	wtk_syn_vector_t *hann;
	wtk_syn_vector_t *psres;
	wtk_syn_vector_t *pulse;
	wtk_syn_vector_t *noise;
	wtk_syn_vocoder_t *vs;
	wtk_rphase_t *rphase;
	wtk_syn_vector_t *ntbl;
	void *notify_ths;
	wtk_tts_sigp_f notify;
};

wtk_syn_sigp_t* wtk_syn_sigp_new(wtk_syn_cfg_t *cfg,int order);
void wtk_syn_sigp_delete(wtk_syn_sigp_t *p);
void wtk_syn_sigp_reset(wtk_syn_sigp_t *p);
void wtk_syn_sigp_set_volume_scale(wtk_syn_sigp_t *p,float scale);
int wtk_syn_sigp_bytes(wtk_syn_sigp_t* p);
/**
 * f0 sequence
 * mel-cep sequence
 * five-band aperiodicity
 * sigmoid parameter
 */
void wtk_syn_sigp_process(wtk_syn_sigp_t *p,wtk_syn_vector_t *f0v,
		wtk_matf_t *mcep,wtk_matf_t *bndap,float sigp);

#ifdef __cplusplus
};
#endif
#endif
