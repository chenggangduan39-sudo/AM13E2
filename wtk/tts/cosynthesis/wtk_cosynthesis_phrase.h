#ifndef __WTK_COSYNTHESIS_PHRASE_H__
#define __WTK_COSYNTHESIS_PHRASE_H__
#include "wtk/core/wtk_str_hash.h"
#include "wtk/tts/utils/crf/wtk_crf.h"
#include "wtk_cosynthesis_phrase_cfg.h"
#include "wtk/tts/utils/pos/qtk_nltkpos.h"
#ifdef __cplusplus
extern "C"{
#endif

typedef struct wtk_cosynthesis_phrase{
    wtk_heap_t *heap;
    qtk_nltkpos_t *pos;
    wtk_crf_t *crfpp;
    int *forward;   //position of this word at forward
    int *backward;  //position of this word at backward
    const char **BMES;
    char *output;
}wtk_cosynthesis_phrase_t;

wtk_cosynthesis_phrase_t* wtk_cosynthesis_phrase_new(wtk_cosynthesis_phrase_cfg_t* cfg);
int wtk_cosynthesis_phrase_delete(wtk_cosynthesis_phrase_t* phrase);
int wtk_cosynthesis_phrase_process(wtk_cosynthesis_phrase_t* phrase, char **seq_word, int len);

#ifdef __cplusplus
};
#endif
#endif
