#ifndef WTK_BFIO_SIGNAL_COMPRESS
#define WTK_BFIO_SIGNAL_COMPRESS
#include "wtk/core/wtk_strbuf.h"
#include "qtk_signal_compress_cfg.h"
#include "qtk/nnrt/qtk_nnrt.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_signal_compress qtk_signal_compress_t;

struct qtk_signal_compress
{
	qtk_signal_compress_cfg_t *cfg;

	qtk_nnrt_t *encoder;
	qtk_nnrt_t *rvq_encoder;
	wtk_strbuf_t *wav;
	wtk_strbuf_t *codec;
	int64_t len[4];
};
void qtk_signal_compress_push_long(wtk_strbuf_t *buf,int64_t *p,int n);
qtk_signal_compress_t* qtk_signal_compress_new(qtk_signal_compress_cfg_t *cfg);
void qtk_signal_compress_delete(qtk_signal_compress_t *cp);
void qtk_signal_compress_reset(qtk_signal_compress_t *cp);
void qtk_signal_compress_feed(qtk_signal_compress_t *cp, short *data, int len);
//int qtk_signal_compress(qtk_signal_compress_t *cp);
int64_t* qtk_signal_compress_get_result(qtk_signal_compress_t *cp, int64_t **len, int *codec_len);

#ifdef __cplusplus
};
#endif
#endif
