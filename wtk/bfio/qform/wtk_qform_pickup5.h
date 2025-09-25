#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICKUP5
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP5
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_qform_pickup5_notify_f)(void *ths,short *data,int len, int is_end);

#define RATE 16000
#define SPEED 340

#define CHANNEL 3
#define CHANNELX2 9
#define OUT_CHANNEL 2
#define LWINDOW 256
#define FRAME_STEP 128
#define NBIN 129
#define CHANNEL2 2
#define CLASSCNT 2

#define THETA_RANGE 35
#define NCOV_ALPHA 0.01f
#define INIT_NCOVNF 100
#define COHV_THRESH 0

#define MMSE_STEP 128
#define NBANDS 18
#define NM (MMSE_STEP+NBANDS)
#define NOISE_SUPPRESS -30 
#define BETA 0.3f

#define WAVSCALE 2.0f

typedef struct
{
	int *bank_left;
	int *bank_right;
	float *filter_left;
	float *filter_right;
}wtk_qform_pickup5_mmse_xfilterbank_t;

wtk_qform_pickup5_mmse_xfilterbank_t* wtk_qform_pickup5_mmse_xfilterbank_new(void);
void wtk_qform_pickup5_mmse_xfilterbank_delete(wtk_qform_pickup5_mmse_xfilterbank_t *bank);
void wtk_qform_pickup5_mmse_xfilterbank_compute_bank32(wtk_qform_pickup5_mmse_xfilterbank_t *bank, float *ps, float *mel);
void wtk_qform_pickup5_mmse_xfilterbank_compute_psd16(wtk_qform_pickup5_mmse_xfilterbank_t *bank,float *mel,float *ps);



typedef struct wtk_qform_pickup5 wtk_qform_pickup5_t;
struct wtk_qform_pickup5
{
    int mic_class[CLASSCNT][CHANNEL2];
    float micclass_pos[CHANNEL2][3];
    float mic_pos[CHANNEL][3];

	float **input;
	float notch_mem[CHANNEL][2];
	float memD[CHANNEL];
	float memX[OUT_CHANNEL];

    float *analysis_win;
	float *synthesis_win;
	wtk_rfft_t *rfft;

    int ang_num;
	wtk_complex_t ***gcc_ovec;

    wtk_complex_t **ncov[OUT_CHANNEL];
    float *ncnt_sum[OUT_CHANNEL];

    wtk_complex_t **ovec[OUT_CHANNEL];
    wtk_complex_t **w[OUT_CHANNEL];

    float *pad[OUT_CHANNEL];

    float nframe;
    int pos;
    int start_pos;

	wtk_qform_pickup5_mmse_xfilterbank_t *bank;

    float min_noise;
	int *noise_frame[OUT_CHANNEL];
	float *noise[OUT_CHANNEL];

	float *old_ps[OUT_CHANNEL];
	float *zeta[OUT_CHANNEL];
	short out_data[OUT_CHANNEL*LWINDOW];

	void *ths;
	wtk_qform_pickup5_notify_f notify;
};

wtk_qform_pickup5_t* wtk_qform_pickup5_new(void);

void wtk_qform_pickup5_delete(wtk_qform_pickup5_t *qform_pickup5);

void wtk_qform_pickup5_reset(wtk_qform_pickup5_t *qform_pickup5);

void wtk_qform_pickup5_set_notify(wtk_qform_pickup5_t *qform_pickup5,void *ths,wtk_qform_pickup5_notify_f notify);

void wtk_qform_pickup5_start(wtk_qform_pickup5_t *qform_pickup5);

void wtk_qform_pickup5_feed(wtk_qform_pickup5_t *qform_pickup5,short **data,int len,int is_end);


#ifdef __cplusplus
};
#endif
#endif
