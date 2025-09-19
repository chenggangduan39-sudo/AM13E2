#ifndef WTK_VITE_F0_WTK_F0_H_
#define WTK_VITE_F0_WTK_F0_H_
#include "wtk/core/math/wtk_vector_buffer.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/asr/fextra/f0/post/wtk_fpost.h"
#include "wtk/asr/fextra/f0/avg/wtk_favg.h"
#include "wtk_f0_cfg.h"
#include "getf0.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_f0 wtk_f0_t;

typedef void (*wtk_f0_notify_t)(void *ths,int frame_index,float f0,float fe);

struct wtk_f0
{
	wtk_f0_cfg_t *cfg;
	wtk_fpost_t *post;
	wtk_favg_t *avg;
	wtk_vector_buffer_t *vb;
	wtk_heap_t *local_heap;
	void *notify_ths;
	wtk_f0_notify_t notify;
	wtk_array_t *f0_array;			//float array;
	wtk_array_t *f0e_array;			//float array;
	wtk_array_t *acpkp_array;     //float array;
// ================== RAW Section ==================
	F0_params *par;
	wav_params *wpar;
	/*
	* headF points to current frame in the circular buffer,
	* tailF points to the frame where tracks start
	* cmpthF points to starting frame of converged path to backtrack
	*/
	Frame *headF, *tailF, *cmpthF;
	int *pcands;	/* array for backtracking in convergence check */
	int cir_buff_growth_count;
	int size_cir_buffer,	/* # of frames in circular DP buffer */
		size_frame_hist,	/* # of frames required before convergence test */
		size_frame_out,	/* # of frames before forcing output */
		num_active_frames,	/* # of frames from tailF to headF */
		output_buf_size;	/* # of frames allocated to output buffers */
	/*
	* DP parameters
	*/
	float tcost, tfact_a, tfact_s, frame_int, vbias, fdouble, wdur, ln2,
		freqwt, lagwt;
	int step, size, nlags, start, stop, ncomp, *locs;
	short maxpeaks;
	int wReuse;  /* number of windows seen before resued */
	Windstat *windstat;
	float *f0p, *vuvp, *rms_speech,
		*acpkp, *peaks;
	int first_time, pad,last_time;
	Stat *stat;
	float *mem;
	/************************************************************************/
	/* get_f0s.c                                                             */
	/************************************************************************/
	/*Get_f0()*/
	long sdstep;
	long buff_size;
	/*downsample()*/
	float *b;
	float *foutput;
	int	ncoeff, ncoefft;
	/*do_ffir()*/
	float *co, *mem_do;
	float *state;
	int fsize, resid;
	/*get_stationarity()*/
	int nframes_old, memsize;
	/************************************************************************/
	/* sigproc.c                                                             */
	/************************************************************************/
	/*xget_window()*/
	float *din;
	int n0;
	/*xcwindow()*/
	int wsize_c;
	float *wind_c;
	/*xhwindow()*/
	int wsize_h;
	float *wind_h;
	/*xhnwindow()*/
	int wsize_hn;
	float *wind_hn;
	/*wind_energy()*/
	int nwind_e;
	float *dwind_e;
	/*xlpc()*/
	float *dwind;
	int nwind;
	/*crossf()*/
	float *dbdata;
	int dbsize;
	/*crossfi()*/
	float *dbdata_i;
	int dbsize_i;
	int frame_index;
};

wtk_f0_t* wtk_f0_new(wtk_f0_cfg_t *cfg);
int wtk_f0_delete(wtk_f0_t* f);
int wtk_f0_reset(wtk_f0_t *f);
int wtk_f0_init(wtk_f0_t *f0,wtk_f0_cfg_t *cfg);
int wtk_f0_dp_f0(wtk_f0_t *f,float *fdata,int buff_size,int sdstep,double freq, int *vecsize);
int wtk_f0_feed(wtk_f0_t *f,int state,short *data,int len);
int wtk_f0_feed_char(wtk_f0_t *f,int state,char *data,int len);
void wtk_f0_print(wtk_f0_t *f);
void wtk_f0_print_file(wtk_f0_t *f0,FILE *f);
void wtk_f0_set_notify(wtk_f0_t *f0,void *ths,wtk_f0_notify_t notify);
#ifdef __cplusplus
};
#endif
#endif
