#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICKUP_LINE2
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP_LINE2
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_qform_pickup_line2_notify_f)(void *ths,short *data,int len, int is_end);

#define RATE 16000
#define SPEED 340

#define CHANNEL 2
#define CHANNELX2 4
#define LWINDOW 1024
#define FRAME_STEP 512
#define NBIN 513

// #define THETA_RANGE 45
#define NCOV_ALPHA 0.01f
#define INIT_NCOVNF 100
#define COHV_THRESH 0

#define MMSE_STEP 512
#define NBANDS 24
#define NM (MMSE_STEP+NBANDS)
#define NOISE_SUPPRESS -30 
#define BETA 0.3f

#define WAVSCALE 1.0f

typedef struct
{
	int *bank_left;
	int *bank_right;
	float *filter_left;
	float *filter_right;
}wtk_qform_pickup_line2_mmse_xfilterbank_t;

wtk_qform_pickup_line2_mmse_xfilterbank_t* wtk_qform_pickup_line2_mmse_xfilterbank_new(void);
void wtk_qform_pickup_line2_mmse_xfilterbank_delete(wtk_qform_pickup_line2_mmse_xfilterbank_t *bank);
void wtk_qform_pickup_line2_mmse_xfilterbank_compute_bank32(wtk_qform_pickup_line2_mmse_xfilterbank_t *bank, float *ps, float *mel);
void wtk_qform_pickup_line2_mmse_xfilterbank_compute_psd16(wtk_qform_pickup_line2_mmse_xfilterbank_t *bank,float *mel,float *ps);



typedef struct wtk_qform_pickup_line2 wtk_qform_pickup_line2_t;
struct wtk_qform_pickup_line2
{
    float mic_pos[CHANNEL][3];

	float **input;
	float notch_mem[CHANNEL][2];
	float memD[CHANNEL];
	float memX;

    float *analysis_win;
	float *synthesis_win;
	wtk_rfft_t *rfft;

	wtk_complex_t **gcc_ovec;
	wtk_complex_t **gcc_ovec2;

    wtk_complex_t **ncov;
    float *ncnt_sum;

    wtk_complex_t **ovec;
    wtk_complex_t **w;

    float *pad;

    float nframe;
    int pos;
    int start_pos;

	wtk_qform_pickup_line2_mmse_xfilterbank_t *bank;

    float min_noise;
	int *noise_frame;
	float *noise;

	float *old_ps;
	float *zeta;

	void *ths;
	wtk_qform_pickup_line2_notify_f notify;
};

wtk_qform_pickup_line2_t* wtk_qform_pickup_line2_new(void);

void wtk_qform_pickup_line2_delete(wtk_qform_pickup_line2_t *qform_pickup_line2);

void wtk_qform_pickup_line2_reset(wtk_qform_pickup_line2_t *qform_pickup_line2);

void wtk_qform_pickup_line2_set_notify(wtk_qform_pickup_line2_t *qform_pickup_line2,void *ths,wtk_qform_pickup_line2_notify_f notify);

void wtk_qform_pickup_line2_start(wtk_qform_pickup_line2_t *qform_pickup_line2);

void wtk_qform_pickup_line2_feed(wtk_qform_pickup_line2_t *qform_pickup_line2,short **data,int len,int is_end);


#ifdef __cplusplus
};
#endif
#endif