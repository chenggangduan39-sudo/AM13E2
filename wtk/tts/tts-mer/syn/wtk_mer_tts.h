#ifndef WTK_MER_TTS_H_
#define WTK_MER_TTS_H_
//#include "wtk/tts-mer/wtk_mer_common.h"
#include "tts-mer/bandmat/wtk_mer_bandmat.h"
#include "tts-mer/syn/wtk_mer_min_max_norm.h"
#include "tts-mer/syn/wtk_mer_state_align.h"
#include "tts-mer/syn/wtk_mer_dnn.h"
#include "tts-mer/syn/wtk_mer_wav.h"
#include "tts-mer/syn/wtk_mer_thread.h"
#include "tts-mer/cfg/wtk_mer_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    wtk_mer_cfg_t *cfg;
    wtk_tts_parser_t *parser;
    wtk_syn_t *syn_tts;
    wtk_mer_wav_stream_t *wav;
    wtk_mer_wav_param_t *wparam;
    // wtk_thread_tpool_t *tpool;
} wtk_mer_tts_t;

typedef struct
{
    wtk_mer_tts_t *tts;
    wtk_strbuf_t ***lab_arrs;
    int lab_len;
    char *wav_fn;
    int i;
} wtk_mer_process_t;

wtk_mer_tts_t* wtk_mer_tts_new(char *cfg_fn, int is_rbin, int seek_pos );
void wtk_mer_tts_reset(wtk_mer_tts_t *tts);
void wtk_mer_tts_delete(wtk_mer_tts_t *tts);
void wtk_mer_process(char *wav_fn, wtk_strbuf_t ***lab_arrs, int lab_len, wtk_mer_tts_t *tts);
void wtk_mer_process2(wtk_mer_process_t *pt);
void wtk_mer_process_thread(char *wav_fn, wtk_strbuf_t ***lab_arrs, int lab_len, wtk_mer_tts_t *tts);

#ifdef __cplusplus
}
#endif
#endif
