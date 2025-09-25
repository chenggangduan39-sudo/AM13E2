#ifndef WTK_MATH_WTK_FEXTRA_CFG_H_
#define WTK_MATH_WTK_FEXTRA_CFG_H_
#include "wtk/core/math/wtk_vector.h"
#include "wtk_fkind.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "cmn/wtk_cmn_cfg.h"
#include "wtk/core/math/wtk_matrix.h"
#include "fnn/wtk_fnn.h"
#include "nnet3/qtk_nnet3_cfg.h"
#include "wtk_feat_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fextra_cfg wtk_fextra_cfg_t;
#define MAX_PARM_DIFFER_WIN 11

struct wtk_fextra_cfg
{
	wtk_feat_cfg_t feat;
	wtk_cmn_cfg_t cmn;
	wtk_fnn_cfg_t dnn;
	qtk_nnet3_cfg_t nnet3;
	// dependent cfg
	float window_size;		//WINDOWSIZE in 100ns
	float window_step;		//TARGETRATE in 100ns
	float frame_dur;			//frame duration, in seconds.
	float src_sample_rate;		//SOURCERATE	100ns unit ns=10*-9
	// calc cfg
	int frame_size;				//samples for window
	int frame_step;				//samples	for frame step;
	int NUMCHNAS;		//NUMCHANS
	int NUMCEPS;           	/* NUMCEPS: Number of cepstral coef */
	int CEPLIFTER;				//CEPLIFTER
	int LPCORDER;				//LPCORDER
	int	DELTAWINDOW;				//DELTAWINDOW
	int ACCWINDOW;				//ACCWINDOW
	int THIRDWINDOW;				//THIRDWINDOW
	//fbankinfo configure.
	float PREMCOEF;				//PREEMCOEF
	float CEPSCALE;           	//CEPSCALE: Scaling factor to avoid arithmetic problems */
	float LOFREQ;           	/* LOFREQ: Fbank lo frequency cut-off */
	float HIFREQ;        	 	/* HIREQ: Fbank hi frequency cut-off */
	float WARPFREQ;          	 	/* WARPFREQ: Warp freq axis for vocal tract normalisation */
	float WARPLCUTOFF;    	   	/* WARPLCUTOFF: lower and upper threshold frequencies */
	float WARPUCUTOFF;     		/* WARPUCUTOFF: for linear frequency warping */
	float COMPRESSFACT;    			//COMPRESSFACT  Compression factor for PLP */
	float ESCALE;
	float SILFLOOR;
	float ADDDITHER;				//ADDDITHER
	//================= information section ===========
	float esilfloor;
	double sigma[3];
	int feature_basic_cols;		//basic feature cols;
	int feature_cols;
	int static_feature_cols;
	int cache_size;
	int align;
	wtk_string_t target_kind;	//TARGETKIND
	wtk_fkind_t pkind;			//parm kind.
	wtk_bfkind_t base_kind;
	unsigned use_z:1;
	unsigned use_cmn:1;
	unsigned  use_cmn2:1;
	unsigned use_dnn:1;
	unsigned use_d:1;
	unsigned use_nnet3:1;
	//----------------------- sigp section---------------
	unsigned ZMEANSOURCE:1; 			//ZMEANSOURCE:  Zero mean source waveform before analysis */
	unsigned RAWENERGY:1;  			//RAWENERGY: Use raw energy before preEmp and ham */
	unsigned USEHAMMING:1;     			//USEHAMMING: Use Hamming Window */
    unsigned USEPOVEY:1;                //USEPOVEY: Use Povey Window */
	unsigned USEPOWER:1;			//USEPOWER
	unsigned DOUBLEFFT:1;			//DOUBLEFFT
	unsigned ENERGY:1;				//TARGETKIND: E
	unsigned DELTA:1;				//D
	unsigned ACCS:1;				//A
	unsigned THIRD:1;				//T
	unsigned ZMEAN:1;				//Z
	unsigned Zero:1;				//0
	unsigned ENORMALISE:1;
	unsigned SIMPLEDIFFS:1;
	unsigned use_e:1;
};

int wtk_fextra_cfg_init(wtk_fextra_cfg_t *cfg);
int wtk_fextra_cfg_clean(wtk_fextra_cfg_t *cfg);
int wtk_fextra_cfg_update(wtk_fextra_cfg_t *cfg);
void wtk_fextra_cfg_update_dep(wtk_fextra_cfg_t *cfg);

/**
 * @brief used for other loader;
 */
int wtk_fextra_cfg_update2(wtk_fextra_cfg_t *cfg,wtk_source_loader_t *sl);
void wtk_fextra_cfg_print(wtk_fextra_cfg_t *p);
int wtk_fextra_cfg_update_local(wtk_fextra_cfg_t *cfg,wtk_local_cfg_t *lc);
float wtk_random_value(void);
int wtk_fextra_cfg_get_sample_rate(wtk_fextra_cfg_t *cfg);
int wtk_fextra_cfg_bytes(wtk_fextra_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
