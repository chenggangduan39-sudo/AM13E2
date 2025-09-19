#ifndef WTK_BFIO_QFORM_WTK_QMMSE
#define WTK_BFIO_QFORM_WTK_QMMSE
#include "wtk_qmmse_cfg.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct wtk_qmmse wtk_qmmse_t;

typedef struct
{
	int *bank_left;
	int *bank_right;
	float *filter_left;
	float *filter_right;
	int nb_banks;
	int len;
}wtk_qmmse_xfilterbank_t;

wtk_qmmse_xfilterbank_t* wtk_qmmse_xfilterbank_new(int bands,int rate,int len);
void wtk_qmmse_xfilterbank_delete(wtk_qmmse_xfilterbank_t *bank);
void wtk_qmmse_xfilterbank_compute_bank32(wtk_qmmse_xfilterbank_t *bank, float *ps, float *mel);
void wtk_qmmse_xfilterbank_compute_psd16(wtk_qmmse_xfilterbank_t *bank,float *mel,float *ps);

struct wtk_qmmse
{
	wtk_qmmse_cfg_t *cfg;

	int nframe;

	wtk_qmmse_xfilterbank_t *bank;
	int nm;

	float min_noise;
	int *noise_frame;
	float *noise;

	float *echo_noise;

	float *ps;
	float *old_ps;
	float *prior;
	float *post;
	float *gain;
	float *gain2;
	float *gain_floor;
	float *zeta;

	int nb_adapt;
	int den_cnt;
	float *S;
	float *Smin;
	float *Stmp;
	char *update_prob;

	float *pn1;
	float *sym;

	float *loudness_weight;
	float loudness;
	float loudness_accum;
	float prev_loudness;
	float init_max;
	float agc_gain;
	float *Se;
	float *Sd;
	wtk_complex_t *Sed;
	float *leak;
	float *tgt_eng_frame;
	float tgt_eng;
	int init_f;
	int smooth_f;
	int smooth_cnt;
	float out_agc_gain;

	float *down_eng_frame;
	float down_eng;
	int down_smooth_f;
	int down_smooth_cnt;
	int down_cnt;

	int loudness_frame;
	int loudness_frame2;

	int agc_init_frame;

	float pframe;

	float *entropy_E;
	float *entropy_Eb;

	int agc_mask_frame;
	int echo_agc_mask_frame;

	unsigned int sp_sil : 1;
};

wtk_qmmse_t* wtk_qmmse_new(wtk_qmmse_cfg_t *cfg);
void wtk_qmmse_delete(wtk_qmmse_t *qmmse);
void wtk_qmmse_reset(wtk_qmmse_t *qmmse);

void wtk_qmmse_feed_cohv(wtk_qmmse_t *qmmse,wtk_complex_t *io,float *cohv);
void wtk_qmmse_feed_cohv2(wtk_qmmse_t *qmmse,wtk_complex_t *io,char *cohv);

void wtk_qmmse_feed_mask(wtk_qmmse_t *qmmse,wtk_complex_t *io,float *mask);
void wtk_qmmse_update_mask(wtk_qmmse_t *qmmse,wtk_complex_t *specs,float *mask_x);

void wtk_qmmse_flush_mask(wtk_qmmse_t *qmmse,wtk_complex_t *specs,float *yf);
void wtk_qmmse_flush_echo_mask(wtk_qmmse_t *qmmse,wtk_complex_t *io, wtk_complex_t *err, int sp_sil);

void wtk_qmmse_feed_echo_denoise(wtk_qmmse_t *qmmse,wtk_complex_t *io, float *yf);
void wtk_qmmse_feed_echo_denoise2(wtk_qmmse_t *qmmse,wtk_complex_t *io, float *yf, float *mask);
void wtk_qmmse_feed_echo_denoise3(wtk_qmmse_t *qmmse,wtk_complex_t *io, wtk_complex_t *err, int sp_sil);
void wtk_qmmse_feed_echo_denoise4(wtk_qmmse_t *qmmse,wtk_complex_t *io, float *yf, float *mask);
void wtk_qmmse_sed(wtk_qmmse_t *qmmse,wtk_complex_t *io, wtk_complex_t *err, float *yf);

void wtk_qmmse_denoise(wtk_qmmse_t *qmmse,wtk_complex_t *io);
void wtk_qmmse_flush_denoise_mask(wtk_qmmse_t *qmmse,wtk_complex_t *specs);

void wtk_qmmse_set_sp_sil(wtk_qmmse_t *qmmse, int sp_sil);
#ifdef __cplusplus
};
#endif
#endif
