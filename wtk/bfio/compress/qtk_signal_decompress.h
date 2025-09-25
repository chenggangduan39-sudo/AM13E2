#ifndef WTK_BFIO_SIGNAL_DECOMPRESS
#define WTK_BFIO_SIGNAL_DECOMPRESS
#include "wtk/core/wtk_strbuf.h"
#include "qtk_signal_decompress_cfg.h"
#include "qtk_signal_compress.h"
#include "qtk/nnrt/qtk_nnrt.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/bfio/maskdenoise/wtk_drft.h"
#ifdef __dplusplus
extern "C" {
#endif
typedef struct qtk_signal_decompress qtk_signal_decompress_t;
typedef void(*qtk_signal_decompress_notify_f)(void *ths,short *output,int len);

struct qtk_signal_decompress
{
	qtk_signal_decompress_cfg_t *cfg;

	qtk_nnrt_t *decoder;
	qtk_nnrt_t *rvq_decoder;
	qtk_nnrt_t *head;
	qtk_signal_decompress_notify_f notify;
	void *ths;
	wtk_strbuf_t *in[4];
	wtk_drft_t *fft;
    float *tmp;
};

qtk_signal_decompress_t* qtk_signal_decompress_new(qtk_signal_decompress_cfg_t *cfg);
void qtk_signal_decompress_delete(qtk_signal_decompress_t *dp);
void qtk_signal_decompress_reset(qtk_signal_decompress_t *dp);
void qtk_signal_decompress_feed(qtk_signal_decompress_t *dp, int64_t *data, int64_t* len);
void qtk_signal_decompress_get_result(qtk_signal_decompress_t *dp);
void qtk_signal_decompress_set_notify(qtk_signal_decompress_t *dp,void *ths,qtk_signal_decompress_notify_f notify);
#ifdef __dplusplus
};
#endif
#endif
