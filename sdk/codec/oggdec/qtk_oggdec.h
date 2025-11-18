#ifndef QTK_UTIL_OGGDEC_QTK_OGGDEC_H
#define QTK_UTIL_OGGDEC_QTK_OGGDEC_H
#include "wtk/core/wtk_type.h"
#include <ogg/ogg.h>
#include <speex/speex.h>
#include <speex/speex_callbacks.h>
#include <speex/speex_header.h>
#include <speex/speex_stereo.h>
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FRAME_SIZE 2000
#define readint(buf, base)                                                     \
    (((buf[base + 3] << 24) & 0xff000000) |                                    \
     ((buf[base + 2] << 16) & 0xff0000) | ((buf[base + 1] << 8) & 0xff00) |    \
     (buf[base] & 0xff))

typedef struct qtk_oggdec qtk_oggdec_t;
typedef int (*qtk_oggdec_write_f)(void *hook, char *buf, int size);
struct qtk_oggdec {
    ogg_sync_state oy;
    ogg_page og;
    ogg_packet op;
    ogg_stream_state os;
    SpeexBits bits;
    void *st;
    ogg_int64_t page_granule;
    ogg_int64_t last_granule;
    int frame_size;
    int granule_frame_size;
    int nframes;
    int packet_count;
    int rate;
    int channels;
    int extra_headers;
    int lookahead;
    int speex_serialno;
    SpeexStereoState stereo;
    unsigned eos : 1;
    unsigned stream_init : 1;
    // configure section
    int forceMode;
    float loss_percent;
    unsigned enh_enabled : 1;
    qtk_oggdec_write_f write_f;
    void *write_hook;
};

qtk_oggdec_t *qtk_oggdec_new();
int qtk_oggdec_delete(qtk_oggdec_t *dec);
void qtk_oggdec_start(qtk_oggdec_t *dec, qtk_oggdec_write_f write, void *hook);
int qtk_oggdec_feed(qtk_oggdec_t *dec, char *data, int len);
void qtk_oggdec_stop(qtk_oggdec_t *dec);

#ifdef __cplusplus
}
#endif
#endif
