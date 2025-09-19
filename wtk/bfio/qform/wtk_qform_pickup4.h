#ifndef WTK_BFIO_QFORM_WTK_QFORM_PICKUP4
#define WTK_BFIO_QFORM_WTK_QFORM_PICKUP4
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/bfio/preemph/wtk_preemph.h"
#include "wtk/core/wtk_wavfile.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void(*wtk_qform_pickup4_notify_f)(void *ths,short *data,int len, int is_end);

#define RATE 16000
#define SPEED 340

#define CHANNEL 2
#define CHANNELX2 4
#define LWINDOW 256
#define FRAME_STEP 128
#define NBIN 129
#define CHANNEL2 2
#define CLASSCNT 2

#define THETA_RANGE 20
#define NCOV_ALPHA 0.01f
#define INIT_NCOVNF 100
#define COHV_THRESH 0

#define MMSE_STEP 128
#define NBANDS 18
#define NM (MMSE_STEP+NBANDS)
#define NOISE_SUPPRESS -30 
#define BETA 0.3f

#define WAVSCALE 1.0f
#define FREQ_SHIFT 8

typedef struct
{
	int *bank_left;
	int *bank_right;
	float *filter_left;
	float *filter_right;
}wtk_qform_pickup4_mmse_xfilterbank_t;

wtk_qform_pickup4_mmse_xfilterbank_t* wtk_qform_pickup4_mmse_xfilterbank_new(void);
void wtk_qform_pickup4_mmse_xfilterbank_delete(wtk_qform_pickup4_mmse_xfilterbank_t *bank);
void wtk_qform_pickup4_mmse_xfilterbank_compute_bank32(wtk_qform_pickup4_mmse_xfilterbank_t *bank, float *ps, float *mel);
void wtk_qform_pickup4_mmse_xfilterbank_compute_psd16(wtk_qform_pickup4_mmse_xfilterbank_t *bank,float *mel,float *ps);


typedef struct wtk_qform_pickup4 wtk_qform_pickup4_t;
struct wtk_qform_pickup4
{
    int mic_class[CLASSCNT][CHANNEL2];
    float micclass_pos[CHANNEL2][3];
    float mic_pos[CHANNEL][3];

	float **input;
	float notch_mem[CHANNEL][2];
	float memD[CHANNEL];
	float memX;

    float *analysis_win;
	float *synthesis_win;
	wtk_rfft_t *rfft;

    int ang_num;
	wtk_complex_t ***gcc_ovec;

    wtk_complex_t **ncov;
    float *ncnt_sum;

    wtk_complex_t **ovec;
    wtk_complex_t **w;

    float *pad;

    float nframe;
    int pos;
    int start_pos;
    
    int correct_count[NBIN-1];
	int freq_howl[NBIN-1];
	int no_howl[NBIN-1];

	wtk_qform_pickup4_mmse_xfilterbank_t *bank;

    float min_noise;
	int *noise_frame;
	float *noise;

	float *old_ps;
	float *zeta;
	float tmp_theta;

	void *ths;
	wtk_qform_pickup4_notify_f notify;
};

wtk_qform_pickup4_t* wtk_qform_pickup4_new(void);

void wtk_qform_pickup4_delete(wtk_qform_pickup4_t *qform_pickup4);

void wtk_qform_pickup4_reset(wtk_qform_pickup4_t *qform_pickup4);

void wtk_qform_pickup4_set_notify(wtk_qform_pickup4_t *qform_pickup4,void *ths,wtk_qform_pickup4_notify_f notify);

void wtk_qform_pickup4_start(wtk_qform_pickup4_t *qform_pickup4, float theta);

void wtk_qform_pickup4_feed(wtk_qform_pickup4_t *qform_pickup4,short **data,int len,int is_end);

void wtk_qform_pickup4_set_grav(wtk_qform_pickup4_t *qform_pickup4,short x,short y,short z);

#ifdef __cplusplus
};
#endif
#endif