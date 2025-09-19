/*
 * wtk_pron.h
 *
 *  Created on: Mar 1, 2017
 *      Author: dm
 */

#ifndef WTK_DEFPRON_H_
#define WTK_DEFPRON_H_
#include "wtk/core/wtk_heap.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_kdict.h"
#include "wtk_tts_def.h"
#include "wtk_tts_segwrd_cfg.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef struct wtk_defpron wtk_defpron_t;
struct wtk_defpron
{
	wtk_heap_t *heap;
	wtk_strbuf_t *buf;
	wtk_kdict_t *wrds;
};
wtk_defpron_t* wtk_defpron_new();
int wtk_defpron_snt(wtk_defpron_t* defpron, wtk_string_t* sylls);
int wtk_defpron_setwrd(wtk_defpron_t* defpron, wtk_string_t* k, wtk_string_t* v);
int wtk_defpron_reset(wtk_defpron_t* defpron);
void wtk_defpron_delete(wtk_defpron_t* defpron);
int wtk_defpron_bytes(wtk_defpron_t* defpron);

wtk_tts_wrd_pron_t* wtk_defpron_find(wtk_defpron_t* defpron, char* data, int len);

#ifdef __cplusplus
};
#endif
#endif /* WTK_DEFPRON_H_ */
