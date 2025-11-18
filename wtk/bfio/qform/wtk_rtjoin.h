#ifndef WTK_BFIO_QFORM_WTK_RTJOIN
#define WTK_BFIO_QFORM_WTK_RTJOIN
#include "wtk/core/wtk_strbuf.h"
#include "wtk_rtjoin_cfg.h"
#include "wtk/core/wtk_wavfile.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_fring.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_rtjoin wtk_rtjoin_t;
typedef void(*wtk_rtjoin_notify_f)(void *ths,short *output,int len);

struct wtk_rtjoin
{
	wtk_rtjoin_cfg_t *cfg;

	wtk_strbuf_t **mic;

    float *window;
    float *synthesis_window;
	wtk_drft_t *rfft;
	float *rfft_in;
	int nbin;
    float **analysis_mem;
	float *synthesis_mem;

	wtk_complex_t **fft;

    wtk_complex_t **W;
    wtk_complex_t **X;
	wtk_complex_t *E;
    wtk_complex_t *Y;
	float **power_x;

	float *nchenr;
	float *nchenr2;
	float *nchenr3;
	int chioce_ch;
	int hide_chioce_ch;

	int *change_frame;
	int init_change_frame;

    wtk_complex_t *fftx;
	float *out;

    wtk_equalizer_t *eq;

    float bs_scale;
    float bs_last_scale;
    int bs_max_cnt;

	int change_frame_delay;

	int csd_in_cnt;
	int csd_out_cnt;

	int mean_nchenr_cnt;
	int nchenr_state;
	int nchenr_cnt;
	int change_delay;
	int *nlms_idx;
	int nlms_change_init;
	float *nlms_weight;

	wtk_fring_t **fring;

	void *ths;
	wtk_rtjoin_notify_f notify;
};

wtk_rtjoin_t* wtk_rtjoin_new(wtk_rtjoin_cfg_t *cfg);
void wtk_rtjoin_delete(wtk_rtjoin_t *rtjoin);
void wtk_rtjoin_start(wtk_rtjoin_t *rtjoin);
void wtk_rtjoin_reset(wtk_rtjoin_t *rtjoin);
void wtk_rtjoin_set_notify(wtk_rtjoin_t *rtjoin,void *ths,wtk_rtjoin_notify_f notify);
/**
 * len=mic array samples
 */
void wtk_rtjoin_feed(wtk_rtjoin_t *rtjoin,short *data,int len,int is_end);
void wtk_rtjoin_feed2(wtk_rtjoin_t *rtjoin,short *data,int len,int choice_ch,int is_end);
#ifdef __cplusplus
};
#endif
#endif
