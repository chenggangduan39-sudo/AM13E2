#ifndef WTK_MER_ENGTTS_API_H_
#define WTK_MER_ENGTTS_API_H_
#include "tts-mer/syn/wtk_mer_tts.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    wtk_mer_tts_t *tts;
    void (*callback)(wtk_mer_wav_stream_t *p, void *param);
    void *param;
    int is_stop:1;
} wtk_mer_engtts_t;

wtk_mer_engtts_t* wtk_mer_engtts_new(char *cfg_fn, int is_rbin, int seek_pos, void* callback, void *param);
wtk_mer_wav_stream_t* wtk_mer_engtts_start(wtk_mer_engtts_t *eng, char *txt);
void wtk_mer_engtts_stop(wtk_mer_engtts_t *eng);
void wtk_mer_engtts_delete(wtk_mer_engtts_t *eng);

#ifdef __cplusplus
}
#endif
#endif
