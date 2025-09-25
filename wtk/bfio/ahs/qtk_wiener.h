#ifndef WTK_BFIO_AHS_QTK_WIENER
#define WTK_BFIO_AHS_QTK_WIENER
#include "wtk/core/wtk_complex.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_ahs_wiener qtk_ahs_wiener_t;

struct qtk_ahs_wiener {
	wtk_covm_t *covm;
	wtk_bf_t *bf;
    wtk_complex_t *fftx;
    float* gf;
    int nbin;
    int clip_s;
    int clip_e;
    float scale;
    float thresh;
    unsigned int use_fftsbf:1;
};

qtk_ahs_wiener_t *qtk_wiener_new(wtk_covm_cfg_t* covm, wtk_bf_cfg_t* bf, int use_fftsbf, int nbin, int win_sz, float thresh);
void qtk_wiener_delete(qtk_ahs_wiener_t *wiener);
void qtk_wiener_reset(qtk_ahs_wiener_t *wiener);
void qtk_wiener_feed(qtk_ahs_wiener_t *wiener, float *real, float *imag, float *mask_real, float *mask_imag);
#ifdef __cplusplus
};
#endif
#endif