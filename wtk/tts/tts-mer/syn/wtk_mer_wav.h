#ifndef WTK_MER_WAV_H_
#define WTK_MER_WAV_H_
#include "tts-mer/wtk_mer_common.h"
#include "tts-mer/sptk/wtk_sptk_sopr.h"
#include "tts-mer/sptk/wtk_sptk_mgc2sp.h"
#include "tts-mer/sptk/freqt/freqt.h"
#include "tts-mer/sptk/c2acr/c2acr.h"
#include "tts-mer/sptk/vopr/vopr.h"
#include "tts-mer/sptk/mc2b/mc2b.h"
#include "tts-mer/sptk/bcp/bcp.h"
#include "tts-mer/sptk/merge/merge.h"
#include "tts-mer/sptk/b2mc/b2mc.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    wtk_matf_t *lf0;
    wtk_matf_t *mgc;
    wtk_matf_t *bap;
    wtk_heap_t *heap;
} wtk_mer_wav_t;

typedef struct
{
    wtk_rfft_t *rf;
    double *sintbl;
    float *win;
    int win_len;
} wtk_mer_wav_param_t;

wtk_mer_wav_param_t* wtk_mer_wav_param_new(int fft_size, int fs);
void wtk_mer_wav_param_delete(wtk_mer_wav_param_t *p);

wtk_mer_wav_t* wtk_mer_wav_new(wtk_heap_t *heap);
void wtk_mer_wav_delete(wtk_mer_wav_t *v);
void wtk_mer_wav_add(wtk_mer_wav_t *wav, char *fea, wtk_matf_t *v);

int wtk_world_synth( wtk_rfft_t *rf, float *win, int fft_size, int fs, float frame_period, float *f0, int f0_len, float *sp, int sp_len, float *ap, int ap_len, char *fn_out, void *outio);
wtk_matf_t* wtk_mer_process_lf0(wtk_matf_t *mf);
wtk_matf_t* wtk_mer_process_bap(wtk_matf_t *mf);
wtk_matf_t* wtk_mer_process_mgc(double *sintbl, wtk_matf_t *mf);
size_t wtk_mer_generate_wav(wtk_mer_wav_param_t *wparam, wtk_mer_wav_t *hash, wtk_mer_wav_stream_t *wav);

#ifdef __cplusplus
}
#endif
#endif
