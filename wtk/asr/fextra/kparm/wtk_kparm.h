#ifndef WTK_ASR_PARM_WTK_KPARM
#define WTK_ASR_PARM_WTK_KPARM
#include "wtk/core/wtk_type.h" 
#include "wtk_kparm_cfg.h"
#include "wtk/asr/fextra/kcmn/qtk_kcmn.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk_kfeat.h"
#include "wtk/asr/fextra/kcmn/wtk_kcmn.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kparm wtk_kparm_t;

#define M_PI		3.14159265358979323846
#define WTK_TPI  6.28318530717959     /* PI*2 */

typedef struct
{
	short s;
	short e;
	float *w;
}wtk_melbin_t;

typedef struct
{
	short *loChan;
	float *loWt;
	int klo;
	int khi;         /* lopass to hipass cut-off fft indices */
}wtk_hmelbin_t;

typedef struct
{
	wtk_melbank_cfg_t *cfg;
	wtk_melbin_t *bins;//numbins;
	wtk_hmelbin_t *hbin;
	float *mel_energy;
	double *mel_energy_double;
}wtk_melbank_t;

typedef struct
{
	wtk_matf_t *dct;
	float *lifter_coeffs;
}wtk_mfcc_post_t;

typedef struct
{
	wtk_mati_t *dct;
	wtk_fix_t *lifter_coeffs;
}wtk_fix_mfcc_post_t;

typedef struct
{
	short s;
	short e;
	wtk_fix_t *w;
}wtk_fix_melbin_t;

typedef struct
{
	wtk_melbank_cfg_t *cfg;
	wtk_fix_melbin_t *bins;
	wtk_fix_t *mel_energy;
}wtk_fix_melbank_t;

struct wtk_kparm
{
	wtk_kparm_cfg_t *cfg;
	wtk_melbank_t *melbank;
	wtk_rfft_t *rfft;
	wtk_mfcc_post_t *mfcc;

	wtk_cmvn_t *cmvn;
	wtk_pcen_t *pcen;
	qtk_kcmn_t *kcmn;
	qtk_ivector_t* ivector;//need use kcmn
	wtk_kcmn_t *kcmvn;
	wtk_vector_t *cur_cmvn_stats;
	wtk_delta_t *delta;
	wtk_lda_t *lda;
	wtk_hoard_t hoard;
	float log_energy_pre_window;
	wtk_kfeat_t **feat;
	float *ivector_feat;
	float *input;
	float *input2;
	float *input3;
	float *fft;
	float *win_torch;
	int ivector_dim;
	int fft_order;
	int win;
	int ivector_index;
	int wav_bytes;
	wtk_fix_t *fix_input;
	wtk_fix_t *fix_input2;
	wtk_fix_t *ccc;
	wtk_fix_t *sss;
	wtk_fix_t *spec;
	wtk_fix_t *mfspec;
	wtk_fix_melbank_t* fix_melbank;
	wtk_fix_mfcc_post_t *fix_mfcc;
	short pre_emphasis_prior;
	int pos;
	float last_point;//for pre_emphasis when use torch
	int start_pos;
	int cache_index;
	int nframe;
	uint32_t idle_hint;
	void *notify_ths;
	wtk_kfeat_notify_f notify;

	void *cmvn_raise_ths;
	wtk_kfeat_notify_f cmvn_raise;

	void *kind_raise_ths;
	wtk_kfeat_notify_f kind_raise;

	int is_end;
	int feed_idx;
	//wtk_ivector_notify_f ivector_notify;
	unsigned want_reset:1;
	unsigned stop_flag:1;
	unsigned start:1;
};

int wtk_kparm_bytes(wtk_kparm_t *parm);
wtk_kparm_t* wtk_kparm_new(wtk_kparm_cfg_t *cfg);
void wtk_kparm_delete(wtk_kparm_t *parm);
void wtk_kparm_start(wtk_kparm_t *parm);
void wtk_kparm_reset(wtk_kparm_t *parm);
void wtk_kparm_set_notify(wtk_kparm_t *parm,void *ths,wtk_kfeat_notify_f notify);
//void wtk_kparm_set_ivector_notify(wtk_kparm_t *parm,void *ths,wtk_ivector_notify_f notify);
void wtk_kparm_feed(wtk_kparm_t *parm,short *data,int len,int is_end);
void wtk_kparm_feed_float(wtk_kparm_t *parm,float *data,int len,int is_end);

void wtk_kparm_push_feat(wtk_kparm_t *p,wtk_kfeat_t *f);
void wtk_kparm_on_delta(wtk_kparm_t *parm,wtk_kfeat_t *feat);
void wtk_kparm_raise(wtk_kparm_t *parm,wtk_kfeat_t *feat);

void wtk_kparm_set_cmvn_raise(wtk_kparm_t *parm,void *ths,wtk_kfeat_notify_f notify);
void wtk_kparm_set_kind_raise(wtk_kparm_t *parm,void *ths,wtk_kfeat_notify_f notify);
#ifdef __cplusplus
};
#endif
#endif
