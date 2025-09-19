#ifndef __SDK_OUT_ENGINE_H__
#define __SDK_OUT_ENGINE_H__

#include "qtk_api.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockqueue.h"
#include "wtk/bfio/resample/wtk_resample.h"
#include "wtk/core/wtk_strbuf.h"

#include <speex/speex_resampler.h>
#include <dlfcn.h>
#include <errno.h>

#ifdef __cplusplus
extern "C"{
#endif

typedef enum{
        QTK_ENGINE_PLAYER_START=0,
        QTK_ENGINE_PLAYER_DATA,
        QTK_ENGINE_PLAYER_STOP,
}qtk_out_engine_player_t;

typedef struct qtk_out_engine qtk_out_engine_t;

struct qtk_out_engine
{
	qtk_session_t *session;
	qtk_engine_t *engine;
	wtk_resample_t *sample;
	SpeexResamplerState *outresample;
	SpeexResamplerState *outresample2;
	char *outresamplebuf;
	char *outresamplebuf2;
	int outrslen;
	wtk_strbuf_t *input;
	wtk_strbuf_t *output;
	wtk_strbuf_t *tmpbuf;
	wtk_blockqueue_t bfio_queue;

	pthread_t player_t;
	pthread_attr_t thread_attr;
	qtk_out_engine_player_t player_type;
	int player_run;
	int is_engine_new;
	int is_engine_start;
	int is_resample;
	int is_player_resample;
	int is_player_run;
	int is_engine_run;
	int is_push_zero;
	int pchannel;
	int sample_rate;
	unsigned int qtk_byte_written;
	int hal_channel_mask;
	int mode;
	FILE *pf;
	FILE *sf;
	FILE *echof;
	FILE *outf;
	FILE *out2_f;
	FILE *out3_f;
};

void qtk_out_engine_init_func();
void qtk_out_engine_close_func();

qtk_out_engine_t *qtk_out_engine_new();
void qtk_out_engine_delete(qtk_out_engine_t *oe);
void qtk_out_engine_resample_init(qtk_out_engine_t *oe);
void qtk_out_engine_start(qtk_out_engine_t *oe);
void qtk_out_engine_reset(qtk_out_engine_t *oe);
int qtk_out_engine_feed(qtk_out_engine_t *oe, char *data, int len);
int qtk_out_engine_feed2(qtk_out_engine_t *oe, char *input, int len);
int qtk_out_engine_feedz(qtk_out_engine_t *oe, char *input, int len);
int qtk_out_engine_pop(qtk_out_engine_t *oe, int len);
void qtk_out_engine_set(qtk_out_engine_t *oe, int is_engine);

#ifdef __cplusplus
};
#endif
#endif
