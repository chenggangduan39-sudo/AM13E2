#ifndef WTK_BFIO_MASKDENOISE_WTK_FBANK_H
#define WTK_BFIO_MASKDENOISE_WTK_FBANK_H
#include "wtk_fbank_cfg.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/fft/wtk_rfft.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fbank wtk_fbank_t;
typedef void(*wtk_fbank_notify_f)(void *ths,float *output,int len);

struct wtk_fbank
{
	wtk_fbank_cfg_t *cfg;
    wtk_strbuf_t *mic;

    wtk_rfft_t *rfft;
    int wins;
    int nbin;
    
    float *strided_input;
    float *offset_strided_input;
    float signal_log_energy;

    float *fft_data;
    float *spectrum;
    float *mel_energies;

	void *ths;
	wtk_fbank_notify_f notify;
};

wtk_fbank_t *wtk_fbank_new(wtk_fbank_cfg_t *cfg);
void wtk_fbank_delete(wtk_fbank_t *fbank);
void wtk_fbank_reset(wtk_fbank_t *fbank);
void wtk_fbank_feed(wtk_fbank_t *fbank, short *data, int len, int is_end);
void wtk_fbank_set_notify(wtk_fbank_t *fbank,void *ths,wtk_fbank_notify_f notify);

#ifdef __cplusplus
};
#endif
#endif
